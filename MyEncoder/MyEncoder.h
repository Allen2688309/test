/* 四路麦轮编码器按插座编号读取。正计数统一表示车轮带动车辆向前。 */
#ifndef __MY_ENCODER_H__
#define __MY_ENCODER_H__

#include "zf_common_headfile.h"

/* J1 左下：P02.8 计数、P00.9 方向，使用 GPT12 T4。 */
#define ENCODER_J1_TIMER        (TIM4_ENCODER)
#define ENCODER_J1_COUNT_PIN    (TIM4_ENCODER_CH1_P02_8)
#define ENCODER_J1_DIR_PIN      (TIM4_ENCODER_CH2_P00_9)

/* J2 右下：P33.7 计数、P33.6 方向，使用 GPT12 T2。 */
#define ENCODER_J2_TIMER        (TIM2_ENCODER)
#define ENCODER_J2_COUNT_PIN    (TIM2_ENCODER_CH1_P33_7)
#define ENCODER_J2_DIR_PIN      (TIM2_ENCODER_CH2_P33_6)

/* J3 右上：P21.7 计数、P21.6 方向，使用 GPT12 T5。 */
#define ENCODER_J3_TIMER        (TIM5_ENCODER)
#define ENCODER_J3_COUNT_PIN    (TIM5_ENCODER_CH1_P21_7)
#define ENCODER_J3_DIR_PIN      (TIM5_ENCODER_CH2_P21_6)

/* J4 左上：P20.3 计数、P20.0 方向，使用 GPT12 T6。 */
#define ENCODER_J4_TIMER        (TIM6_ENCODER)
#define ENCODER_J4_COUNT_PIN    (TIM6_ENCODER_CH1_P20_3)
#define ENCODER_J4_DIR_PIN      (TIM6_ENCODER_CH2_P20_0)

/* 轮子按车辆前进方向转动时，计数显示为正。 */
#define ENCODER_J1_SIGN         (-1)
#define ENCODER_J2_SIGN         (1)
#define ENCODER_J3_SIGN         (-1)
#define ENCODER_J4_SIGN         (-1)

#define ENCODER_CHANNEL_COUNT   (4U)
#define ENCODER_SAMPLE_MS       (3U)

/* 速度按标称采样周期换算，默认单位为脉冲/3 ms。 */
#define ENCODER_SPEED_PERIOD_US (ENCODER_SAMPLE_MS * 1000U)

/* 轮径单位为毫米，每圈计数按编码器插座分别标定。 */
#define ENCODER_WHEEL_DIAMETER_MM (60.0f)
#define ENCODER_J1_COUNTS_PER_REV (2400U)
#define ENCODER_J2_COUNTS_PER_REV (2400U)
#define ENCODER_J3_COUNTS_PER_REV (2500U)
#define ENCODER_J4_COUNTS_PER_REV (2400U)

/* 每脉冲轮缘行程等于轮周长除以每圈计数，单位为毫米。 */
#define ENCODER_WHEEL_CIRCUMFERENCE_MM (3.14159265f * ENCODER_WHEEL_DIAMETER_MM)
#define ENCODER_J1_MM_PER_COUNT (ENCODER_WHEEL_CIRCUMFERENCE_MM / ENCODER_J1_COUNTS_PER_REV)
#define ENCODER_J2_MM_PER_COUNT (ENCODER_WHEEL_CIRCUMFERENCE_MM / ENCODER_J2_COUNTS_PER_REV)
#define ENCODER_J3_MM_PER_COUNT (ENCODER_WHEEL_CIRCUMFERENCE_MM / ENCODER_J3_COUNTS_PER_REV)
#define ENCODER_J4_MM_PER_COUNT (ENCODER_WHEEL_CIRCUMFERENCE_MM / ENCODER_J4_COUNTS_PER_REV)

/* 对外函数 */
void  Encoder_Init(void);
void  Encoder_Update(void);
void  Encoder_GetSnapshot(int32 Count[ENCODER_CHANNEL_COUNT], float Speed_rps[ENCODER_CHANNEL_COUNT]);
float Encoder_GetSpeedRPS(uint8 Channel);

int32 Encoder_GetSpeed(uint8 Channel);  /* 通道 0～3 对应 J1～J4 */
int16 Encoder_GetSpeedL(void);           /* J1 速度，单位为脉冲/标称采样周期 */
int16 Encoder_GetSpeedR(void);           /* J4 速度，单位为脉冲/标称采样周期 */

int32 Encoder_GetCountL(void);           /* 左轮累计脉冲（带正负，往前转变大） */
int32 Encoder_GetCountR(void);           /* 右轮累计脉冲 */

float Encoder_GetDistanceMM(void);       /* 四轮平均的分段净位移，单位毫米 */
void  Encoder_ClearCount(void);          /* 清零分段里程，保留累计脉冲 */

#endif /* __MY_ENCODER_H__ */
