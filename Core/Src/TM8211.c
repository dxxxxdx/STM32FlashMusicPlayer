//
// Created by zys on 26-5-12.
//

#include "TM8211.h"

#include "stm32g0xx_hal.h"
#include "w25q64.h"
extern I2S_HandleTypeDef hi2s1;
int16_t pingBuffer[W25Q64_PAGE_SIZE/2];
int16_t pongBuffer[W25Q64_PAGE_SIZE/2];
//TODO:reuse of buffer in w25q64 , but here g030 have enough RAM
//TODO:so receivingBuffer  uint8_t w25q64_receivingBuffer[W25Q64_PAGE_SIZE]   is dead when player working
//static method

TM8211 tm8211 = {
    .transmit = HAL_TM8211_Transmit,
    .transmitCallback = HAL_TM8211_PingPongTxCpltCallback,
    .setToZero = TM8211_SetToZero,
    .NoneBuffer = {0, 0},
    .pingBuffer = pingBuffer,
    .pongBuffer = pongBuffer,
    .BufferSize = W25Q64_PAGE_SIZE/2,
    .currentPtr = NONE_PINGPONGPTR,
    .isWorking = TM8211_IDLE,
};

void HAL_TM8211_Transmit(TM8211* self) {
    if (self->currentPtr == PING) {
        HAL_I2S_Transmit_DMA(&hi2s1, (uint16_t*)self->pingBuffer, self->BufferSize);
    } else if (self->currentPtr == PONG){
        HAL_I2S_Transmit_DMA(&hi2s1,(uint16_t*) self->pongBuffer, self->BufferSize);
    }else if (self->currentPtr == NONE_PINGPONGPTR){
        HAL_I2S_Transmit_DMA(&hi2s1, self->NoneBuffer, 2);
    }
}
__attribute((weak)) void HAL_TM8211_PingPongTxCpltCallback(TM8211* self) {
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef* hi2s) {
    if (hi2s->Instance == SPI1) {

        if (tm8211.currentPtr == PING) {
            tm8211.currentPtr = PONG;
        } else if (tm8211.currentPtr == PONG) {
            tm8211.currentPtr = PING;
        }
        if (tm8211.isWorking == TM8211_WORKING) {
            tm8211.transmit(&tm8211);
        }
        HAL_TM8211_PingPongTxCpltCallback(&tm8211);
    }
}

//static method



void HAL_TM8211_Start(TM8211* self) {
    self->currentPtr = PING;
    self->isWorking = TM8211_WORKING;
    self->transmit(self);

}
void HAL_TM8211_Stop(TM8211* self) {
    self->currentPtr = NONE_PINGPONGPTR;
    self->isWorking = TM8211_IDLE;
}
void TM8211_SetToZero(TM8211* self) {
    self->currentPtr = NONE_PINGPONGPTR;
    self->transmit(self);
}
void HAL_TM8211_Init(TM8211* self) {
    self->setToZero(self);
}



















