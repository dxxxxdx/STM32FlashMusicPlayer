//
// Created by zys on 26-5-11.
//

#ifndef CH340_H
#define CH340_H
#include <stdint.h>
#define UART_BUFFER_SIZE 260

typedef struct UART_FSM{
    uint16_t dynamicBufferIndex;
    uint16_t staticBufferLength;
    uint8_t* dynamicBuffer;
    uint8_t* staticBuffer;
    volatile uint8_t fullBufferRead;
    volatile uint8_t staticBufferLocked;
    void(*onBufferOverflow)(struct UART_FSM *self);
    void(*receiveCpltCallback)(struct UART_FSM *self);
    void(*echoback)(struct UART_FSM *self, const char* str,uint16_t length);
    // it will always send const string ,so need param str
    uint8_t(*readyToTransmit)(struct UART_FSM *self);
}UART_FSM;

void HAL_UART_FSM_Init(UART_FSM *self);

#endif //CH340_H
