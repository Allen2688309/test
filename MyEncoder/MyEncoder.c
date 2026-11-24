#include "MyHeadFile.h"

static const encoder_index_enum Encoder_timer[ENCODER_CHANNEL_COUNT] = {
    ENCODER_J1_TIMER, ENCODER_J2_TIMER,
    ENCODER_J3_TIMER, ENCODER_J4_TIMER
};
static const encoder_channel1_enum Encoder_count_pin[ENCODER_CHANNEL_COUNT] = {
    ENCODER_J1_COUNT_PIN, ENCODER_J2_COUNT_PIN,
    ENCODER_J3_COUNT_PIN, ENCODER_J4_COUNT_PIN
};
static const encoder_channel2_enum Encoder_dir_pin[ENCODER_CHANNEL_COUNT] = {
    ENCODER_J1_DIR_PIN, ENCODER_J2_DIR_PIN,
    ENCODER_J3_DIR_PIN, ENCODER_J4_DIR_PIN
};
static const int8 Encoder_sign[ENCODER_CHANNEL_COUNT] = {
    ENCODER_J1_SIGN, ENCODER_J2_SIGN,
    ENCODER_J3_SIGN, ENCODER_J4_SIGN
};

static const uint16 Encoder_counts_per_rev[ENCODER_CHANNEL_COUNT] = {
    ENCODER_J1_COUNTS_PER_REV, ENCODER_J2_COUNTS_PER_REV,
    ENCODER_J3_COUNTS_PER_REV, ENCODER_J4_COUNTS_PER_REV
};
static const float Encoder_mm_per_count[ENCODER_CHANNEL_COUNT] = {
    ENCODER_J1_MM_PER_COUNT, ENCODER_J2_MM_PER_COUNT,
    ENCODER_J3_MM_PER_COUNT, ENCODER_J4_MM_PER_COUNT
};

static uint16 Encoder_last_count[ENCODER_CHANNEL_COUNT];
static volatile uint32 Encoder_total[ENCODER_CHANNEL_COUNT];
static volatile int32 Encoder_speed[ENCODER_CHANNEL_COUNT];
static volatile float Encoder_speed_rps[ENCODER_CHANNEL_COUNT];
static uint32 Segment_start_count[ENCODER_CHANNEL_COUNT];
static uint32 Stm_frequency;
static uint32 Last_sample_tick;

// 函数简介       初始化四路编码器计数，清零软件累计量并记录首个采样时间。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
void Encoder_Init(void)
{
    uint8 Index;

    for (Index = 0; Index < ENCODER_CHANNEL_COUNT; ++Index)
    {
        encoder_dir_init(Encoder_timer[Index], Encoder_count_pin[Index], Encoder_dir_pin[Index]);
        Encoder_last_count[Index] = (uint16)encoder_get_count(Encoder_timer[Index]);
        Encoder_total[Index] = 0;
        Encoder_speed[Index] = 0;
        Encoder_speed_rps[Index] = 0.0f;
    }
    for (Index = 0; Index < ENCODER_CHANNEL_COUNT; ++Index)
    {
        Segment_start_count[Index] = 0;
    }
    Stm_frequency = (uint32)IfxStm_getFrequency(&MODULE_STM0);
    Last_sample_tick = IfxStm_getLower(&MODULE_STM0);
}

// 函数简介       读取四路计数增量并修正方向，按实际采样间隔换算脉冲速度和转速。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
void Encoder_Update(void)
{
    uint8 Index;
    uint32 Now_tick = IfxStm_getLower(&MODULE_STM0);
    uint32 Elapsed_ticks = Now_tick - Last_sample_tick;
    uint32 Elapsed_us = (uint32)((uint64)Elapsed_ticks * 1000000U / Stm_frequency);

    float Samples_per_second;

    if (Elapsed_us == 0U)
    {
        return;
    }
    Last_sample_tick = Now_tick;
    Samples_per_second = 1000000.0f / (float)Elapsed_us;

    for (Index = 0; Index < ENCODER_CHANNEL_COUNT; ++Index)
    {
        uint16 Count = (uint16)encoder_get_count(Encoder_timer[Index]);
        /* 相邻采样差小于半个计数范围时，按 16 位差值处理硬件回绕。 */
        int32 Delta = (int32)(int16)(uint16)(Count - Encoder_last_count[Index]);
        int32 Speed;

        Encoder_last_count[Index] = Count;
        Delta *= Encoder_sign[Index];
        Encoder_total[Index] += (uint32)Delta;

        /* 用实际间隔校正采样抖动，带符号计算保留反转时的负速度。 */
        Speed = (int32)((int64)Delta * ENCODER_SPEED_PERIOD_US / Elapsed_us);
        Encoder_speed[Index] = Speed;

        /* 直接从本次计数增量算转速，保留整数脉冲速度换算时会截去的小数。 */
        Encoder_speed_rps[Index] = (float)Delta * Samples_per_second
                                 / (float)Encoder_counts_per_rev[Index];
    }
}

// 函数简介       复制同一次采样的四路累计计数和速度，读取期间暂时屏蔽本核中断。
// 参数说明       Count：输出数组，按 J1 到 J4 写入带符号的累计脉冲数。
// 参数说明       Speed_rps：输出数组，按 J1 到 J4 写入带符号的转速，单位为转/秒。
// return         无。
//date  : 2026-11-24
//author: Q
void Encoder_GetSnapshot(int32 Count[ENCODER_CHANNEL_COUNT], float Speed_rps[ENCODER_CHANNEL_COUNT])
{
    uint8 Index;
    boolean Interrupt_state = IfxCpu_disableInterrupts();

    for (Index = 0; Index < ENCODER_CHANNEL_COUNT; ++Index)
    {
        Count[Index] = (int32)Encoder_total[Index];
        Speed_rps[Index] = Encoder_speed_rps[Index];
    }
    IfxCpu_restoreInterrupts(Interrupt_state);
}

// 函数简介       读取指定通道最近一次采样的机械转速。
// 参数说明       Channel：编码器通道索引，0 到 3 依次对应 J1 到 J4。
// return         带符号的转速，单位为转/秒。无效通道返回零。
//date  : 2026-11-24
//author: Q
float Encoder_GetSpeedRPS(uint8 Channel)
{
    if (Channel >= ENCODER_CHANNEL_COUNT)
    {
        return 0.0f;
    }

    return Encoder_speed_rps[Channel];
}

// 函数简介       读取指定通道最近一次采样的速度。
// 参数说明       Channel：编码器通道索引，0 到 3 依次对应 J1 到 J4。
// return         带符号的速度，单位为脉冲/标称采样周期。无效通道返回零。
//date  : 2026-11-24
//author: Q
int32 Encoder_GetSpeed(uint8 Channel)
{
    if (Channel >= ENCODER_CHANNEL_COUNT)
    {
        return 0;
    }
    return Encoder_speed[Channel];
}

// 函数简介       读取 J1 的最新速度，供旧接口调用。
// 参数说明       无。
// return         带符号的速度，单位为脉冲/标称采样周期。
//date  : 2026-11-24
//author: Q
int16 Encoder_GetSpeedL(void)
{
    return (int16)Encoder_speed[0];
}

// 函数简介       读取 J4 的最新速度，供旧接口调用。
// 参数说明       无。
// return         带符号的速度，单位为脉冲/标称采样周期。
//date  : 2026-11-24
//author: Q
int16 Encoder_GetSpeedR(void)
{
    return (int16)Encoder_speed[ENCODER_CHANNEL_COUNT - 1U];
}

// 函数简介       读取 J1 从初始化起累计的脉冲数。
// 参数说明       无。
// return         带符号的累计脉冲数。
//date  : 2026-11-24
//author: Q
int32 Encoder_GetCountL(void)
{
    return (int32)Encoder_total[0];
}

// 函数简介       读取 J4 从初始化起累计的脉冲数。
// 参数说明       无。
// return         带符号的累计脉冲数。
//date  : 2026-11-24
//author: Q
int32 Encoder_GetCountR(void)
{
    return (int32)Encoder_total[ENCODER_CHANNEL_COUNT - 1U];
}

// 函数简介       将各轮分段计数按各自的每脉冲行程换算，再求平均净位移。
// 参数说明       无。
// return         带符号的平均净位移，单位为毫米。
//date  : 2026-11-24
//author: Q
float Encoder_GetDistanceMM(void)
{
    uint8 Index;
    boolean Interrupt_state = IfxCpu_disableInterrupts();
    int32 Distance_count[ENCODER_CHANNEL_COUNT];
    float Distance_mm = 0.0f;

    for (Index = 0; Index < ENCODER_CHANNEL_COUNT; ++Index)
    {
        Distance_count[Index] = (int32)(Encoder_total[Index] - Segment_start_count[Index]);
    }
    IfxCpu_restoreInterrupts(Interrupt_state);

    for (Index = 0; Index < ENCODER_CHANNEL_COUNT; ++Index)
    {
        Distance_mm += (float)Distance_count[Index] * Encoder_mm_per_count[Index];
    }
    return Distance_mm * 0.25f;
}

// 函数简介       将当前四轮累计计数记为新的分段起点，用于重新计算分段净位移。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
void Encoder_ClearCount(void)
{
    uint8 Index;
    boolean Interrupt_state = IfxCpu_disableInterrupts();

    for (Index = 0; Index < ENCODER_CHANNEL_COUNT; ++Index)
    {
        Segment_start_count[Index] = Encoder_total[Index];
    }
    IfxCpu_restoreInterrupts(Interrupt_state);
}
