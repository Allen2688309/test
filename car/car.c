#include "MyHeadFile.h"

// 函数简介       初始化系统时钟、四路电机、按键、IPS 和编码器，并启动编码器采样定时器。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
void car_init(void)
{
    boot_reset_status = SCU_RSTSTAT.U;
    clock_init();
    system_start();

    M_Motor_Init();
    MyKey_Init();
    Menu_Encoder_Init();

    Encoder_Init();
    pit_ms_init(CCU60_CH0, ENCODER_SAMPLE_MS);
}

// 函数简介       扫描旋钮和按键，处理菜单操作并刷新当前测试页。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
void car_run(void)
{
    MyKey_Scan();
    Menu_Encoder_Run();
}
