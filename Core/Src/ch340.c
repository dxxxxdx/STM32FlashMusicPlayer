//
// Created by zys on 26-5-11.
//

#include "ch340.h"

#include <string.h>

#include "flashDownloader.h"
#include "stm32g0xx_hal.h"

extern UART_HandleTypeDef huart1;
//recommended IT receive
extern FlashDownloader downloader;

//static method
static void HAL_UART_FSM_Echoback(UART_FSM *self, const char* str,uint16_t length);
static void UART_FSM_DefaultReceiveCpltCallback(UART_FSM *self);
static uint8_t HAL_UART_FSM_ReadyToTransmit(UART_FSM *self);
static void UART_FSM_CopyBuffer(void *restrict dst, const void *restrict src, uint16_t len);
//static method

static uint8_t uart_fsm_dynamicBuffer[UART_BUFFER_SIZE] __attribute__((aligned(4)));
static uint8_t uart_fsm_staticBuffer[UART_BUFFER_SIZE] __attribute__((aligned(4))) ;

UART_FSM uart_fsm = {
    .dynamicBufferIndex = 0,
    .staticBufferLength = 0,
    .dynamicBuffer = uart_fsm_dynamicBuffer,
    .staticBuffer = uart_fsm_staticBuffer,
    .fullBufferRead = 0,
    .staticBufferLocked = 0,
    .onBufferOverflow = 0,
    .receiveCpltCallback = UART_FSM_DefaultReceiveCpltCallback,
    .echoback = HAL_UART_FSM_Echoback,
    .readyToTransmit = HAL_UART_FSM_ReadyToTransmit,
};

//static method
static void HAL_UART_FSM_Echoback(UART_FSM *self, const char* str,uint16_t length) {
    while (HAL_UART_Transmit_IT(&huart1, (uint8_t*)str, length)==HAL_OK)
    {
        __NOP();
    }
}




static void UART_FSM_DefaultReceiveCpltCallback(UART_FSM *self) {
    // 1. 防空包：长度为 0 直接丢弃
    if (self->staticBufferLength == 0) {
        return;
    }
    if (self->staticBufferLocked) {
        return;
    }

    // 2. 数据放行：如果是全包接收模式 (正在接收 256 字节的 Flash 数据)
    // 此时首字节是纯二进制数据，没有任何规律，直接放行给底层处理
    if (self->fullBufferRead == 1) {
        downloader.getInstruction(&downloader, self->staticBuffer, self->staticBufferLength);
        return;
    }
        downloader.getInstruction(&downloader, self->staticBuffer, self->staticBufferLength);
}
static uint8_t HAL_UART_FSM_ReadyToTransmit(UART_FSM *self) {
    // 1. 检查 UART 全局句柄状态 (gState 专门负责发送)
    if (huart1.gState != HAL_UART_STATE_READY) {
        return 0;
    }

    // 2. 检查 DMA 句柄状态 (防止 DMA 还在搬运，但 UART 状态还没更新的间隙)
    if (huart1.hdmatx != NULL) {
        if (huart1.hdmatx->State != HAL_DMA_STATE_READY) {
            return 0;
        }
    }
    return 1;
}

static void UART_FSM_CopyBuffer(void *restrict dst, const void *restrict src, uint16_t len) {
    if (len == 0) {
        return;
    }

    if ((((uintptr_t)dst | (uintptr_t)src | len) & 0x03u) == 0u) {
        fastcpy(dst, src, len >> 2);
    } else {
        memcpy(dst, src, len);
    }
}

//static method
// ============================================================================
// 1. 修正后的接收回调函数（接客优先：先重开 DMA，再解包）
// ============================================================================
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART1) {
        if (Size > UART_BUFFER_SIZE) Size = UART_BUFFER_SIZE;
        uint8_t triggerCallback = 0;
        if (uart_fsm.staticBufferLocked){ goto RESTART_RX_DMA; }
        // 【第一步：只管把数据从 dynamicBuffer 捞走，不执行任何耗时逻辑】
        if (uart_fsm.fullBufferRead == 1) {
                if (uart_fsm.staticBufferLength + Size <= UART_BUFFER_SIZE) {
                    UART_FSM_CopyBuffer(&uart_fsm.staticBuffer[uart_fsm.staticBufferLength], uart_fsm.dynamicBuffer, Size);
                    uart_fsm.staticBufferLength += Size;
                } else {
                    uint16_t remain = UART_BUFFER_SIZE - uart_fsm.staticBufferLength;
                    if (remain > 0) {
                        UART_FSM_CopyBuffer(&uart_fsm.staticBuffer[uart_fsm.staticBufferLength], uart_fsm.dynamicBuffer, remain);
                        uart_fsm.staticBufferLength += remain;
                    }
                }
                if (uart_fsm.staticBufferLength >= W25Q64_PAGE_SIZE) {
                    triggerCallback = 1;
                }
        } else {
            uart_fsm.staticBufferLength = Size;
            UART_FSM_CopyBuffer(uart_fsm.staticBuffer, uart_fsm.dynamicBuffer, Size);
            triggerCallback = 1;
        }
        RESTART_RX_DMA:

        // 【第二步：十万火急！立刻把 DMA 接收重新挂上去，保护现场，防止后续数据冲卡】
        if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_fsm.dynamicBuffer, UART_BUFFER_SIZE) != HAL_OK) {
            // 清除硬件可能存在的噪声、过载等错误标志
            __HAL_UART_CLEAR_FLAG(&huart1, UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF);
            // 【核心：强行解锁库状态】
            huart1.RxState = HAL_UART_STATE_READY;
            HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_fsm.dynamicBuffer, UART_BUFFER_SIZE);
        }
        __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);

        // 【第三步：后方安全了，现在可以放心地去跑状态机了】
        if (triggerCallback) {
            uart_fsm.receiveCpltCallback(&uart_fsm);
        }
    }
}

//static method
void HAL_UART_FSM_Init(UART_FSM *self) {
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, self->dynamicBuffer, UART_BUFFER_SIZE);
}



