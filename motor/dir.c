#include "MyHeadFile.h"

static const gpio_pin_enum Dir_pin[M_MOTOR_WHEEL_COUNT] = {
    M_MOTOR_UL_DIR_PIN, M_MOTOR_UR_DIR_PIN, M_MOTOR_LL_DIR_PIN, M_MOTOR_LR_DIR_PIN
};
static const uint8 Dir_forward_level[M_MOTOR_WHEEL_COUNT] = {
    DIR_UL_FORWARD_LEVEL, DIR_UR_FORWARD_LEVEL, DIR_LL_FORWARD_LEVEL, DIR_LR_FORWARD_LEVEL
};

// 函数简介       将四路方向引脚初始化为高电平推挽输出。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
void Dir_Init(void)
{
    uint8 Wheel;

    for (Wheel = 0U; Wheel < M_MOTOR_WHEEL_COUNT; ++Wheel)
    {
        gpio_init(Dir_pin[Wheel], GPO, GPIO_HIGH, GPO_PUSH_PULL);
    }
}

// 函数简介       写入指定轮位的方向引脚电平。
// 参数说明       Wheel：电机轮位编号。
// 参数说明       Level：零为低电平，一为高电平。
// return         无。
//date  : 2026-11-24
//author: Q
void Dir_Set_Level(uint8 Wheel, uint8 Level)
{
    if (Wheel < M_MOTOR_WHEEL_COUNT)
    {
        gpio_set_level(Dir_pin[Wheel], Level);
    }
}

// 函数简介       读取指定轮位的方向引脚电平。
// 参数说明       Wheel：电机轮位编号。
// return         引脚电平，无效轮位返回高电平。
//date  : 2026-11-24
//author: Q
uint8 Dir_Get_Level(uint8 Wheel)
{
    if (Wheel >= M_MOTOR_WHEEL_COUNT)
    {
        return GPIO_HIGH;
    }
    return gpio_get_level(Dir_pin[Wheel]);
}

/* 函数简介       判断当前方向电平是否对应指定轮的正转。
 * 参数说明       Wheel：电机轮位编号。
 * return         正转返回一，反转或无效轮位返回零。
 *date  : 2026-11-24
 *author: Q
 */
uint8 Dir_Is_Forward(uint8 Wheel)
{
    if (Wheel >= M_MOTOR_WHEEL_COUNT)
    {
        return 0U;
    }
    return Dir_Get_Level(Wheel) == Dir_forward_level[Wheel];
}

/* 函数简介       按轮位的正转极性设置方向引脚。
 * 参数说明       Wheel：电机轮位编号。
 * 参数说明       Forward：一表示正转，零表示反转。
 * return         无。
 *date  : 2026-11-24
 *author: Q
 */void Dir_Set_Wheel(uint8 Wheel, uint8 Forward)
{
    uint8 Level;

    if (Wheel >= M_MOTOR_WHEEL_COUNT)
    {
        return;
    }

    Level = Dir_forward_level[Wheel];
    if (Forward == 0U)
    {
        Level = 1U - Level;
    }
    Dir_Set_Level(Wheel, Level);
}

// 函数简介       停止当前单轮测试，再写入标准 X 型布置的四轮方向。
// 参数说明       Left_front：左上轮，一为正转，负一为反转，零为停用。
// 参数说明       Right_front：右上轮，取值含义同左上轮。
// 参数说明       Left_rear：左下轮，取值含义同左上轮。
// 参数说明       Right_rear：右下轮，取值含义同左上轮。
// return         参与运动的轮位掩码，对应位为一的轮子需要给 PWM。
//date  : 2026-11-24
//author: Q
static uint8 Dir_Apply(int8 Left_front, int8 Right_front, int8 Left_rear, int8 Right_rear)
{
    const int8 Wheel_direction[M_MOTOR_WHEEL_COUNT] = {
        Left_front, Right_front, Left_rear, Right_rear
    };
    uint8 Active_mask = 0U;
    uint8 Wheel;

    M_Motor_Stop_All();
    for (Wheel = 0U; Wheel < M_MOTOR_WHEEL_COUNT; ++Wheel)
    {
        /* 停用轮也预置正转电平，调用方按返回的掩码让它保持零占空比。 */
        Dir_Set_Wheel(Wheel, Wheel_direction[Wheel] >= 0);
        if (Wheel_direction[Wheel] != 0)
        {
            Active_mask |= (uint8)(1U << Wheel);
        }
    }
    return Active_mask;
}

// 函数简介       设置车身前进时的四轮转向。
// 参数说明       无。
// return         参与运动的轮位掩码。
//date  : 2026-11-24
//author: Q
uint8 Dir_Forward(void)
{
    return Dir_Apply(1, 1, 1, 1);
}

// 函数简介       设置车身后退时的四轮转向。
// 参数说明       无。
// return         参与运动的轮位掩码。
//date  : 2026-11-24
//author: Q
uint8 Dir_Backward(void)
{
    return Dir_Apply(-1, -1, -1, -1);
}

// 函数简介       设置车身左横移时的四轮转向。
// 参数说明       无。
// return         参与运动的轮位掩码。
//date  : 2026-11-24
//author: Q
uint8 Dir_Left(void)
{
    return Dir_Apply(-1, 1, 1, -1);
}

// 函数简介       设置车身右横移时的四轮转向。
// 参数说明       无。
// return         参与运动的轮位掩码。
//date  : 2026-11-24
//author: Q
uint8 Dir_Right(void)
{
    return Dir_Apply(1, -1, -1, 1);
}

// 函数简介       设置车身左前斜移时的四轮转向。
// 参数说明       无。
// return         参与运动的轮位掩码。
//date  : 2026-11-24
//author: Q
uint8 Dir_Forward_Left(void)
{
    return Dir_Apply(0, 1, 1, 0);
}

// 函数简介       设置车身右前斜移时的四轮转向。
// 参数说明       无。
// return         参与运动的轮位掩码。
//date  : 2026-11-24
//author: Q
uint8 Dir_Forward_Right(void)
{
    return Dir_Apply(1, 0, 0, 1);
}

// 函数简介       设置车身左后斜移时的四轮转向。
// 参数说明       无。
// return         参与运动的轮位掩码。
//date  : 2026-11-24
//author: Q
uint8 Dir_Backward_Left(void)
{
    return Dir_Apply(-1, 0, 0, -1);
}

// 函数简介       设置车身右后斜移时的四轮转向。
// 参数说明       无。
// return         参与运动的轮位掩码。
//date  : 2026-11-24
//author: Q
uint8 Dir_Backward_Right(void)
{
    return Dir_Apply(0, -1, -1, 0);
}

// 函数简介       设置车身原地左转时的四轮转向。
// 参数说明       无。
// return         参与运动的轮位掩码。
//date  : 2026-11-24
//author: Q
uint8 Dir_Turn_Left(void)
{
    return Dir_Apply(-1, 1, -1, 1);
}

// 函数简介       设置车身原地右转时的四轮转向。
// 参数说明       无。
// return         参与运动的轮位掩码。
//date  : 2026-11-24
//author: Q
uint8 Dir_Turn_Right(void)
{
    return Dir_Apply(1, -1, 1, -1);
}
