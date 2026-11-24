#ifndef M_MOTOR_H
#define M_MOTOR_H

#include "zf_common_typedef.h"
#include "zf_driver_gpio.h"
#include "zf_driver_pwm.h"

#define M_MOTOR_WHEEL_UL       (0U)
#define M_MOTOR_WHEEL_UR       (1U)
#define M_MOTOR_WHEEL_LL       (2U)
#define M_MOTOR_WHEEL_LR       (3U)
#define M_MOTOR_WHEEL_COUNT    (4U)

#define M_MOTOR_PWM_HZ         (17000U)
#define M_MOTOR_PWM_MIN        (800U)
#define M_MOTOR_PWM_MAX        (4000U)
#define M_MOTOR_TARGET_RPS     (60U)
#define M_MOTOR_START_PWM      (1000U)

/* 四个轮位共用一套增量式 PI 参数，误差单位为转/秒，积分系数每 3 ms 使用一次。 */
#define M_MOTOR_KP             (2.0f)
#define M_MOTOR_KI             (0.40f)
#define M_MOTOR_DIRECTION_DELAY_MS (2U)

#if M_MOTOR_PWM_MIN > M_MOTOR_START_PWM || M_MOTOR_START_PWM > M_MOTOR_PWM_MAX || M_MOTOR_PWM_MAX > PWM_DUTY_MAX
#error Motor PWM exceeds configured limit
#endif

/* 四轮电机引脚映射，调整接线时在这里修改。 */
#define M_MOTOR_UL_PWM_PIN     (ATOM0_CH1_P33_9)
#define M_MOTOR_UR_PWM_PIN     (ATOM2_CH1_P33_5)
#define M_MOTOR_LL_PWM_PIN     (ATOM0_CH2_P33_11)
#define M_MOTOR_LR_PWM_PIN     (ATOM2_CH5_P33_13)

#define M_MOTOR_UL_DIR_PIN     (P22_2)
#define M_MOTOR_UR_DIR_PIN     (P22_3)
#define M_MOTOR_LL_DIR_PIN     (P21_3)
#define M_MOTOR_LR_DIR_PIN     (P22_0)

void M_Motor_Init(void);
void M_Motor_Stop_All(void);
void M_Motor_Test_Select(uint8 Wheel);
void M_Motor_Start_Motion(uint8 Wheel_mask);
void M_Motor_Speed_Tick(void);
uint8 M_Motor_Get_Active_Wheel(void);
uint8 M_Motor_Get_Active_Mask(void);
uint8 M_Motor_Get_Direction(uint8 Wheel);
int32 M_Motor_Get_Single_Target(void);    /* 单轮带符号目标转速，单位为转/秒。 */
uint32 M_Motor_Get_Target(uint8 Wheel);   /* 指定轮位的目标转速大小，单位为转/秒。 */
uint32 M_Motor_Get_Target_RPS(void);
uint32 M_Motor_Get_PWM(uint8 Wheel);


#endif
