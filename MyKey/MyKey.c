/* 旋转编码器和按键扫描：正交解码、消抖、短按及长按。 */
#include "MyKey.h"

/* 内部变量 */
int16 switch_encoder_change_num = 0;        /* 这一帧转了多少格 */

/* 注意：这里必须用 gpio_pin_enum，不能图省事写 uint8！
   逐飞库的引脚枚举值是"端口号*32 + 脚号"：
       P02_5 = 69        -> uint8 装得下
       P20_9 = 20*32+9 = 649  -> uint8 装不下，会被截断成 137，读的就是错的引脚了 */
static gpio_pin_enum key_pin[MY_KEY_NUM] =  /* 每个按键对应的引脚 */
{
    SWITCH_ENCODER_KEY_PIN,                 /* MY_KEY_1 */
    MY_BTN_SW5_PIN,                         /* MY_KEY_2 */
    MY_BTN_SW1_PIN,                         /* MY_KEY_3 */
    MY_BTN_SW2_PIN                          /* MY_KEY_4 */
};

static uint8  key_stable[MY_KEY_NUM];       /* 消抖后的电平：1=松开 0=按下 */
static uint8  key_cnt[MY_KEY_NUM];          /* 连续读到不同电平的帧数 */
static uint32 key_press_us[MY_KEY_NUM];     /* 按下去的时刻 */
static uint8  key_long_fired[MY_KEY_NUM];   /* 这一次按住有没有已经报过长按 */
static uint8  key_event[MY_KEY_NUM];        /* 待读取的动作 */

static uint8  enc_last_state   = 0;         /* 上一帧的 A/B 状态 */
static int16  enc_edge_accum   = 0;         /* 累计了多少个边沿 */
static uint8  enc_change_flag  = 0;         /* 这一帧编码器动过没有 */

/* 正交解码表：[上一状态][当前状态] -> +1 / -1 / 0
    (prev << 2) | curr 作为下标 */
static const int8 enc_rot_table[16] =
{
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

// 函数简介       将旋钮和板载按键设为上拉输入，并记录旋钮起始相位。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
void MyKey_Init(void)
{
    uint8 i;
    uint8 a, b;

    gpio_init(SWITCH_ENCODER_A_PIN,   GPI, GPIO_HIGH, GPI_PULL_UP);
    gpio_init(SWITCH_ENCODER_B_PIN,   GPI, GPIO_HIGH, GPI_PULL_UP);
    gpio_init(SWITCH_ENCODER_KEY_PIN, GPI, GPIO_HIGH, GPI_PULL_UP);
    gpio_init(MY_BTN_SW5_PIN,         GPI, GPIO_HIGH, GPI_PULL_UP);
    gpio_init(MY_BTN_SW1_PIN,         GPI, GPIO_HIGH, GPI_PULL_UP);
    gpio_init(MY_BTN_SW2_PIN,         GPI, GPIO_HIGH, GPI_PULL_UP);

    for(i = 0; i < MY_KEY_NUM; i++)
    {
        key_stable[i]     = 1;
        key_cnt[i]        = 0;
        key_press_us[i]   = 0;
        key_long_fired[i] = 0;
        key_event[i]      = MY_KEY_RELEASE;
    }

    /* 记下编码器当前状态，免得一上电就把初始状态当成一次转动 */
    a = gpio_get_level(SWITCH_ENCODER_A_PIN) ? 1 : 0;
    b = gpio_get_level(SWITCH_ENCODER_B_PIN) ? 1 : 0;
    enc_last_state  = (uint8)((a << 1) | b);
    enc_edge_accum  = 0;
    enc_change_flag = 0;
    switch_encoder_change_num = 0;
}

// 函数简介       按配置间隔采样旋钮和按键，更新旋钮步数，并根据消抖结果生成短按或长按事件。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
void MyKey_Scan(void)
{
    static uint32 last_scan_us = 0;
    uint32 now_us = system_getval_us();
    uint8  i;
    uint8  a, b, st;
    int8   d;

    /* 每次调用先清上次按键动作，避免短按在快速主循环中重复执行。 */
    for(i = 0; i < MY_KEY_NUM; i++)
    {
        key_event[i] = MY_KEY_RELEASE;
    }

    /* ----------  同帧去重闸  ---------- */
    if(last_scan_us != 0 && (uint32)(now_us - last_scan_us) < MY_KEY_MIN_GAP_US)
    {
        return;
    }
    last_scan_us = now_us;

    /* 每一帧都从 0 开始重新统计这一帧转了多少格 */
    switch_encoder_change_num = 0;

    /* ---------------- ① 旋转编码器 ---------------- */
    a  = gpio_get_level(SWITCH_ENCODER_A_PIN) ? 1 : 0;
    b  = gpio_get_level(SWITCH_ENCODER_B_PIN) ? 1 : 0;
    st = (uint8)((a << 1) | b);

    d = enc_rot_table[(uint8)((enc_last_state << 2) | st)];
    enc_last_state = st;

    if(d != 0)
    {
        enc_edge_accum += d;

        if(enc_edge_accum >= ENC_EDGES_PER_STEP)
        {
            switch_encoder_change_num += 1;
            enc_edge_accum = 0;
            enc_change_flag = 1;
        }
        else if(enc_edge_accum <= -ENC_EDGES_PER_STEP)
        {
            switch_encoder_change_num -= 1;
            enc_edge_accum = 0;
            enc_change_flag = 1;
        }
    }

    /* ---------------- ② 按键 ---------------- */
    for(i = 0; i < MY_KEY_NUM; i++)
    {
        uint8 raw_level = (gpio_get_level(key_pin[i]) == 0) ? 0 : 1;   /* 低电平=按下 */

        if(raw_level != key_stable[i])
        {
            key_cnt[i]++;
            if(key_cnt[i] >= MY_KEY_DEBOUNCE)
            {
                key_stable[i] = raw_level;
                key_cnt[i]    = 0;

                if(key_stable[i] == 0)
                {
                    /* 刚按下：记时刻，等它松手或者按够长 */
                    key_press_us[i]   = now_us;
                    key_long_fired[i] = 0;
                }
                else
                {
                    /* 刚松手：如果一直没报过长按，那就算一次短按 */
                    if(key_long_fired[i] == 0)
                    {
                        key_event[i] = MY_KEY_SHORT_PRESS;
                    }
                }
            }
        }
        else
        {
            key_cnt[i] = 0;
        }

        /* 按住不放：够时间就报一次长按（只报一次，松手才算完） */
        if(key_stable[i] == 0 && key_long_fired[i] == 0)
        {
            if((now_us - key_press_us[i]) >= (uint32)MY_KEY_LONG_MS * 1000U)
            {
                key_event[i]      = MY_KEY_LONG_PRESS;
                key_long_fired[i] = 1;
            }
        }
    }
}

// 函数简介       查询本轮扫描生成的按键事件，允许重复读取同一个事件。
// 参数说明       key：按键编号，范围为零到 MY_KEY_NUM 减一。
// return         短按、长按或无事件。无效编号返回无事件。
//date  : 2026-11-24
//author: Q
uint8 my_key_get_state(uint8 key)
{
    if(key >= MY_KEY_NUM) return MY_KEY_RELEASE;
    return key_event[key];
}

// 函数简介       读取并清除旋钮转动标志，转动格数由 switch_encoder_change_num 提供。
// 参数说明       无。
// return         本轮产生旋钮步进时返回 1，否则返回 0。
//date  : 2026-11-24
//author: Q
uint8 If_Switch_Encoder_Change(void)
{
    uint8 r = enc_change_flag;
    enc_change_flag = 0;
    return r;
}

/* 菜单里的 key_clear_all_state() 用逐飞库自带的那个（zf_device_key.c），
 * 这里不能再定义，否则链接报 multiple definitions。本文件的按键状态在 MyKey_Scan 里已经清过了。 */
