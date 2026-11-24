#include "MyHeadFile.h"

static const pwm_channel_enum Motor_pwm_pin[M_MOTOR_WHEEL_COUNT] = {
    M_MOTOR_UL_PWM_PIN, M_MOTOR_UR_PWM_PIN, M_MOTOR_LL_PWM_PIN, M_MOTOR_LR_PWM_PIN
};

/* 左上、右上、左下、右下对应编码器 J4、J3、J1、J2。 */
static const uint8 Motor_encoder_channel[M_MOTOR_WHEEL_COUNT] = {3U, 2U, 0U, 1U};

static volatile uint8 Active_wheel = M_MOTOR_WHEEL_COUNT;
static volatile uint8 Active_mask;
static volatile float Speed_pi_output[M_MOTOR_WHEEL_COUNT];
static volatile float Speed_pi_last_error[M_MOTOR_WHEEL_COUNT];
static volatile uint8 Speed_pi_started[M_MOTOR_WHEEL_COUNT];
static volatile uint32 Motor_pwm_output[M_MOTOR_WHEEL_COUNT];

/* 函数简介       初始化四路 PWM 和方向引脚，PWM 从零启动。
 * 参数说明       无。
 * return         无。
 *date  : 2026-11-24
 *author: Q
 */
void M_Motor_Init(void)
{
    uint8 Wheel;
    for (Wheel = 0U; Wheel < M_MOTOR_WHEEL_COUNT; ++Wheel)
    {
        pwm_init(Motor_pwm_pin[Wheel], M_MOTOR_PWM_HZ, 0U);
        Speed_pi_output[Wheel] = 0.0f;
        Speed_pi_last_error[Wheel] = 0.0f;
        Speed_pi_started[Wheel] = 0U;
        Motor_pwm_output[Wheel] = 0U;
    }
    Dir_Init();
    Active_wheel = M_MOTOR_WHEEL_COUNT;
    Active_mask = 0U;
}

/* 函数简介       停止所有轮的速度闭环并清零 PWM，等待输出生效后恢复方向引脚高电平。
 * 参数说明       无。
 * return         无。
 *date  : 2026-11-24
 *author: Q
 */
void M_Motor_Stop_All(void)
{
    uint8 Wheel;
    Active_wheel = M_MOTOR_WHEEL_COUNT;
    Active_mask = 0U;

    for (Wheel = 0U; Wheel < M_MOTOR_WHEEL_COUNT; ++Wheel)
    {
        pwm_set_duty(Motor_pwm_pin[Wheel], 0U);
        Motor_pwm_output[Wheel] = 0U;
        Speed_pi_output[Wheel] = 0.0f;
        Speed_pi_last_error[Wheel] = 0.0f;
        Speed_pi_started[Wheel] = 0U;
    }

    system_delay_ms(M_MOTOR_DIRECTION_DELAY_MS);
    for (Wheel = 0U; Wheel < M_MOTOR_WHEEL_COUNT; ++Wheel)
    {
        Dir_Set_Level(Wheel, GPIO_HIGH);
    }
}

/* 函数简介       单独将所有 PWM 置零并清除 PI 状态，保留方向函数刚设置的 DIR。
 * 参数说明       无。
 * return         无。
 *date  : 2026-11-24
 *author: Q
 */
static void M_Motor_Clear_PWM_Keep_Direction(void)
{
    uint8 Wheel;

    Active_wheel = M_MOTOR_WHEEL_COUNT;
    Active_mask = 0U;
    for (Wheel = 0U; Wheel < M_MOTOR_WHEEL_COUNT; ++Wheel)
    {
        pwm_set_duty(Motor_pwm_pin[Wheel], 0U);
        Motor_pwm_output[Wheel] = 0U;
        Speed_pi_output[Wheel] = 0.0f;
        Speed_pi_last_error[Wheel] = 0.0f;
        Speed_pi_started[Wheel] = 0U;
    }
}

/* 函数简介       启动指定单轮速度闭环，再次选择同一轮时切换其正反转方向。
 * 参数说明       Wheel：电机轮位编号。
 * return         无。
 *date  : 2026-11-24
 *author: Q
 */
void M_Motor_Test_Select(uint8 Wheel)
{
    uint8 Previous_wheel;
    uint8 Forward;

    if (Wheel >= M_MOTOR_WHEEL_COUNT)
    {
        return;
    }

    Previous_wheel = Active_wheel;
    Forward = 1U;
    if (Previous_wheel == Wheel && Dir_Is_Forward(Wheel) != 0U)
    {
        Forward = 0U;
    }

    M_Motor_Stop_All();
    Dir_Set_Wheel(Wheel, Forward);
    system_delay_ms(M_MOTOR_DIRECTION_DELAY_MS);

    Speed_pi_output[Wheel] = (float)M_MOTOR_START_PWM;
    Motor_pwm_output[Wheel] = M_MOTOR_START_PWM;
    pwm_set_duty(Motor_pwm_pin[Wheel], M_MOTOR_START_PWM);

    Active_wheel = Wheel;
    Active_mask = (uint8)(1U << Wheel);
}

/* 函数简介       启动整车方向测试的闭环，保留方向函数设好的四路 DIR。
 * 参数说明       Wheel_mask：方向函数返回的活动轮位掩码。
 * return         无。
 *date  : 2026-11-24
 *author: Q
 */
void M_Motor_Start_Motion(uint8 Wheel_mask)
{
    uint8 Wheel;
    Wheel_mask &= (uint8)((1U << M_MOTOR_WHEEL_COUNT) - 1U);
    if (Wheel_mask == 0U)
    {
        return;
    }

    /* 方向函数已停掉旧运动并配置本次 DIR，这里清 PWM 但保留方向。 */
    M_Motor_Clear_PWM_Keep_Direction();
    system_delay_ms(M_MOTOR_DIRECTION_DELAY_MS);

    for (Wheel = 0U; Wheel < M_MOTOR_WHEEL_COUNT; ++Wheel)
    {
        if ((Wheel_mask & (1U << Wheel)) == 0U)
        {
            continue;
        }
        Speed_pi_output[Wheel] = (float)M_MOTOR_START_PWM;
        Motor_pwm_output[Wheel] = M_MOTOR_START_PWM;
        pwm_set_duty(Motor_pwm_pin[Wheel], M_MOTOR_START_PWM);
    }

    Active_mask = Wheel_mask;
}

// 函数简介       每 3 ms 按各活动轮的机械转速误差独立更新 PI 和 PWM。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
void M_Motor_Speed_Tick(void)
{
    uint8 Wheel;
    uint8 Mask = Active_mask;

    for (Wheel = 0U; Wheel < M_MOTOR_WHEEL_COUNT; ++Wheel)
    {
        float Actual;
        float Error;
        float Output;

        if ((Mask & (1U << Wheel)) == 0U)
        {
            continue;
        }

        Actual = Encoder_GetSpeedRPS(Motor_encoder_channel[Wheel]);
        /* 反转时以指令方向为正，和统一的正目标转速比较。 */
        if (Dir_Is_Forward(Wheel) == 0U)
        {
            Actual = -Actual;
        }

        Error = (float)M_MOTOR_TARGET_RPS - Actual;
        if (Speed_pi_started[Wheel] == 0U)
        {
            Speed_pi_last_error[Wheel] = Error;
            Speed_pi_started[Wheel] = 1U;
            continue;
        }

        Output = Speed_pi_output[Wheel]
               + M_MOTOR_KP * (Error - Speed_pi_last_error[Wheel])
               + M_MOTOR_KI * Error;
        Speed_pi_last_error[Wheel] = Error;

        if (Output > (float)M_MOTOR_PWM_MAX)
        {
            Output = (float)M_MOTOR_PWM_MAX;
        }
        else if (Output < (float)M_MOTOR_PWM_MIN)
        {
            Output = (float)M_MOTOR_PWM_MIN;
        }

        Speed_pi_output[Wheel] = Output;
        Motor_pwm_output[Wheel] = (uint32)Output;
        pwm_set_duty(Motor_pwm_pin[Wheel], Motor_pwm_output[Wheel]);
    }
}

/* 函数简介       查询当前单轮测试编号，整车测试时返回哨兵值。
 * 参数说明       无。
 * return         单轮编号或 M_MOTOR_WHEEL_COUNT。
 *date  : 2026-11-24
 *author: Q
 */
uint8 M_Motor_Get_Active_Wheel(void)
{
    return Active_wheel;
}

/* 函数简介       查询参与速度闭环的轮位掩码。
 * 参数说明       无。
 * return         轮位掩码。
 *date  : 2026-11-24
 *author: Q
 */
uint8 M_Motor_Get_Active_Mask(void)
{
    return Active_mask;
}

/* 函数简介       查询指定轮当前的 PWM 输出。
 * 参数说明       Wheel：电机轮位编号。
 * return         PWM 设定值，无效轮位返回零。
 *date  : 2026-11-24
 *author: Q
 */
uint32 M_Motor_Get_PWM(uint8 Wheel)
{
    if (Wheel >= M_MOTOR_WHEEL_COUNT)
    {
        return 0U;
    }
    return Motor_pwm_output[Wheel];
}

// 函数简介       查询指定轮位的目标转速大小。
// 参数说明       Wheel：电机轮位编号。
// return         转/秒，无效轮位返回零。
//date  : 2026-11-24
//author: Q
uint32 M_Motor_Get_Target(uint8 Wheel)
{
    if (Wheel >= M_MOTOR_WHEEL_COUNT)
    {
        return 0U;
    }

    return M_MOTOR_TARGET_RPS;
}

/* 函数简介       返回所有轮共用的目标转速。
 * 参数说明       无。
 * return         转/秒。
 *date  : 2026-11-24
 *author: Q
 */
uint32 M_Motor_Get_Target_RPS(void)
{
    return M_MOTOR_TARGET_RPS;
}

// 函数简介       查询单轮闭环的带符号目标转速。
// 参数说明       无。
// return         转/秒，整车测试或停机时返回零。
//date  : 2026-11-24
//author: Q
int32 M_Motor_Get_Single_Target(void)
{
    uint32 Target;

    if (Active_wheel >= M_MOTOR_WHEEL_COUNT)
    {
        return 0;
    }

    Target = M_Motor_Get_Target(Active_wheel);
    if (Dir_Is_Forward(Active_wheel) == 0U)
    {
        return -(int32)Target;
    }
    return (int32)Target;
}

/* 函数简介       读取指定轮的 DIR 引脚电平。
 * 参数说明       Wheel：电机轮位编号。
 * return         引脚电平，无效轮位返回高电平。
 *date  : 2026-11-24
 *author: Q
 */
uint8 M_Motor_Get_Direction(uint8 Wheel)
{
    return Dir_Get_Level(Wheel);
}