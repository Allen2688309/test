/* 菜单旋转编码器与按键接口。
 * EC11：A=P02.6、B=P02.7、按压=P02.5，公共端接地。
 * 转动选行或改值，短按确认，长按返回。 */
#ifndef __MY_KEY_H__
#define __MY_KEY_H__

#include "zf_common_headfile.h"

/* 一、接线 */
#define SWITCH_ENCODER_A_PIN    P02_6       /* 编码器 A 相 */
#define SWITCH_ENCODER_B_PIN    P02_7       /* 编码器 B 相 */
#define SWITCH_ENCODER_KEY_PIN  P02_5       /* 编码器自带的按压开关 */

/* 板载三个按键，一起管起来，menu 那边可以直接当辅助键用 */
#define MY_BTN_SW5_PIN          P20_9
#define MY_BTN_SW1_PIN          P20_2
#define MY_BTN_SW2_PIN          P20_7

/* 二、按键编号 + 状态 */
#define MY_KEY_1    (0)     /* 旋转编码器的按压开关（菜单主用键） */
#define MY_KEY_2    (1)     /* 板载 SW5  P20.9 */
#define MY_KEY_3    (2)     /* 板载 SW1  P20.2 */
#define MY_KEY_4    (3)     /* 板载 SW2  P20.7 */
#define MY_KEY_NUM  (4)

#define MY_KEY_RELEASE      (0)     /* 没动作 */
#define MY_KEY_SHORT_PRESS  (1)     /* 短按 */
#define MY_KEY_LONG_PRESS   (2)     /* 长按 */

/* 注意：my_key_get_state() 读到的动作只在"这一次 MyKey_Scan 之后、
   下一次 MyKey_Scan 之前"有效；同一帧里重复读结果一样（不是读走即清）。 */

/* 三、可调参数 */
#define MY_KEY_DEBOUNCE     (3U)        /* 连续几帧读到同一个电平才算数（消抖） */
/* 硬件采样的最小间隔，按键事件在每次调用扫描函数时更新。 */
#define MY_KEY_MIN_GAP_US   (5000U)
#define MY_KEY_LONG_MS      (600U)      /* 按住超过多少毫秒算长按 */
#define ENC_EDGES_PER_STEP  (4)         /* 编码器走几个边沿算"转了一格"
                                           EC11 一格通常是 4 个边沿；如果转一格跳好几行，
                                           把这个值调大（比如 8） */

/* 四、对外变量 / 函数 */
extern int16 switch_encoder_change_num;     /* 这一帧转了几格，正=顺时针 */

void  MyKey_Init(void);                     /* 上电初始化一次 */
void  MyKey_Scan(void);                     /* 主循环里每帧调一次 */

uint8 my_key_get_state(uint8 key);          /* 读按键动作（短的/长的，本帧有效、不消费） */

/* 注意：key_clear_all_state() 用的是逐飞库自带的那个（zf_device_key.h 里），
   本模块不重复定义 —— 不然链接会报 multiple definitions。
   我们自己的按键状态每次 MyKey_Scan() 都会清干净，不需要另外清。 */

uint8 If_Switch_Encoder_Change(void);       /* 这一帧编码器有没有转：1=转了（读走就清） */

#endif /* __MY_KEY_H__ */
