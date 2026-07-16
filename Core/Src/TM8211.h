//
// Created by zys on 26-5-12.
//

#ifndef TM8211_H
#define TM8211_H
#include <stdint.h>
#include "stm32g0xx.h"
typedef enum {
    NONE_PINGPONGPTR = 0,
    PING,
    PONG
} PingPongPtr ;

typedef enum {
    TM8211_IDLE = 0,
    TM8211_WORKING
} TM8211_State;


typedef struct TM8211 {
    void(*transmit)(struct TM8211* self);
    void(*transmitCallback)(struct TM8211* self);
    void(*setToZero)(struct TM8211* self);
    int16_t NoneBuffer[2];
    int16_t* pingBuffer;
    int16_t* pongBuffer;
    uint16_t BufferSize;
    volatile uint8_t currentPtr;
    volatile uint8_t isWorking;
} TM8211;

void TM8211_SetToZero(TM8211* self);
void HAL_TM8211_Init(TM8211* self);

void HAL_TM8211_Transmit(TM8211* self) ;

void HAL_TM8211_PingPongTxCpltCallback(TM8211* self) ;

void HAL_TM8211_Start(TM8211* self) ;
void HAL_TM8211_Stop(TM8211* self) ;








#endif //TM8211_H
