#ifndef MOTOR_DIR_H
#define MOTOR_DIR_H

#include "zf_common_typedef.h"

/* 实测的正转方向电平，轮位顺序为左上、右上、左下、右下。 */
#define DIR_UL_FORWARD_LEVEL  (1U)
#define DIR_UR_FORWARD_LEVEL  (0U)
#define DIR_LL_FORWARD_LEVEL  (1U)
#define DIR_LR_FORWARD_LEVEL  (1U)

void  Dir_Init(void);
void  Dir_Set_Level(uint8 Wheel, uint8 Level);
uint8 Dir_Get_Level(uint8 Wheel);
uint8 Dir_Is_Forward(uint8 Wheel);
void  Dir_Set_Wheel(uint8 Wheel, uint8 Forward);

/* 设置整车运动方向，返回参与运动的轮位掩码，位号对应电机轮位编号。 */
uint8 Dir_Forward(void);
uint8 Dir_Backward(void);
uint8 Dir_Left(void);
uint8 Dir_Right(void);
uint8 Dir_Forward_Left(void);
uint8 Dir_Forward_Right(void);
uint8 Dir_Backward_Left(void);
uint8 Dir_Backward_Right(void);
uint8 Dir_Turn_Left(void);
uint8 Dir_Turn_Right(void);

#endif
