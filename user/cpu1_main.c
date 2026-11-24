/* 编码器测试在 CPU0 上执行。 */
#include "zf_common_headfile.h"

#pragma section all "cpu1_dsram"     /* 本段全局变量放 CPU1 的 RAM */

// 函数简介       进入 CPU1 的空闲循环。
// 参数说明       无。
// return         无，函数持续运行。
//date  : 2026-11-24
//author: Q
void core1_main(void)
{
    while (TRUE)
    {
    }
}

#pragma section all restore
