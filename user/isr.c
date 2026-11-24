/*
 * isr.c
 *
 *  Created on: 2026年10月24日
 *
 *  中断分配：摄像头三个中断、调试串口 UART0、3ms 编码器测试采样。
 */

#include "isr_config.h"
#include "isr.h"
#include "MyHeadFile.h"

/* 一、摄像头中断 */

// 函数简介       检查并清除 P02.0 的外部中断标志，分发摄像头场同步事件。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
IFX_INTERRUPT(exti_ch3_ch7_isr, 0, EXTI_CH3_CH7_INT_PRIO)
{
    interrupt_global_enable(0);

    if(exti_flag_get(ERU_CH3_REQ6_P02_0))
    {
        exti_flag_clear(ERU_CH3_REQ6_P02_0);
        camera_vsync_handler();
    }
}

// 函数简介       将 DMA 通道 5 的传输完成事件交给摄像头处理。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
IFX_INTERRUPT(dma_ch5_isr, 0, DMA_INT_PRIO)
{
    interrupt_global_enable(0);
    camera_dma_handler();
}

// 函数简介       响应摄像头串口的发送中断并允许中断嵌套。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
IFX_INTERRUPT(uart1_tx_isr, 0, UART1_TX_INT_PRIO)
{
    interrupt_global_enable(0);
}

// 函数简介       将串口 1 的接收事件交给摄像头串口处理函数。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
IFX_INTERRUPT(uart1_rx_isr, 0, UART1_RX_INT_PRIO)
{
    interrupt_global_enable(0);
    camera_uart_handler();
}

// 函数简介       调用英飞凌驱动处理串口 1 的错误事件。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
IFX_INTERRUPT(uart1_er_isr, 0, UART1_ER_INT_PRIO)
{
    interrupt_global_enable(0);
    IfxAsclin_Asc_isrError(&uart1_handle);
}

// 函数简介       响应调试串口的发送中断并允许中断嵌套。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
IFX_INTERRUPT(uart0_tx_isr, 0, UART0_TX_INT_PRIO)
{
    interrupt_global_enable(0);
}

// 函数简介       启用调试串口接收中断功能时，将接收事件交给调试串口处理函数。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
IFX_INTERRUPT(uart0_rx_isr, 0, UART0_RX_INT_PRIO)
{
    interrupt_global_enable(0);
#if DEBUG_UART_USE_INTERRUPT
    debug_interrupr_handler();
#endif
}

// 函数简介       调用英飞凌驱动处理串口 0 的错误事件。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
IFX_INTERRUPT(uart0_er_isr, 0, UART0_ER_INT_PRIO)
{
    interrupt_global_enable(0);
    IfxAsclin_Asc_isrError(&uart0_handle);
}

// 函数简介       清除定时中断标志，采样四路编码器，再更新当前测试轮的速度环。
// 参数说明       无。
// return         无。
//date  : 2026-11-24
//author: Q
IFX_INTERRUPT(ccu60_ch0_isr, 0, CCU6_0_CH0_ISR_PRIORITY)
{
    pit_clear_flag(CCU60_CH0);
    Encoder_Update();
    M_Motor_Speed_Tick();
}
