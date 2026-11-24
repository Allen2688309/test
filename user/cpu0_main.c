/* CPU0初始化和主循环。 */
#include "MyHeadFile.h"

#pragma section all "cpu0_dsram"
volatile uint32 boot_reset_status = 0;

// 函数简介       初始化小车应用并在 CPU0 上持续运行主循环。
// 参数说明       无。
// return         无，函数持续运行。
//date  : 2026-11-24
//author: Q
void core0_main(void)
{
    car_init();

    while (1)
    {
        car_run();
    }
}

#pragma section all restore
