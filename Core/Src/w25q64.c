//
// Created by zys on 26-5-11.
//

#include "w25q64.h"

#include "stm32g0xx_hal.h"
extern SPI_HandleTypeDef hspi2;
uint8_t w25q64_receivingBuffer[W25Q64_PAGE_SIZE] __attribute__((aligned(4)));



uint8_t w25q64_sendingBuffer[W25Q64_PAGE_SIZE+W25Q64_BITSTREAM_HEAD] __attribute__((aligned(4)));


W25Q64 w25q64 = {
    .FlashAddress = 0,
    .CS_BSRR = &GPIOA->BSRR,
    .CS_PIN_MASK = GPIO_PIN_8,
    .sendBuffer = w25q64_sendingBuffer,
    .BufferDataStart = w25q64_sendingBuffer + W25Q64_BITSTREAM_HEAD,
    .receivingBuffer = w25q64_receivingBuffer,
    .transmit = HAL_W25Q64_Transmit,
    .receive = HAL_W25Q64_Receive,
    .isReceiving = 0,
    .isBusy = 0,
    .receiveSize = 0,
};


//static binding
static void W25Q64_Set_CS(W25Q64* self);
static void W25Q64_Reset_CS(W25Q64* self);
static void W25Q64_TransmitCallback(W25Q64* self);
static void W25Q64_ReceiveCallback(W25Q64* self);
//static binding

void HAL_W25Q64_Transmit(W25Q64* self, uint16_t size) {
    while (HAL_SPI_Transmit_DMA(&hspi2, self->sendBuffer, size)!=HAL_OK){
        __NOP();
    }
    __HAL_DMA_DISABLE_IT(hspi2.hdmatx, DMA_IT_HT);
}

void HAL_W25Q64_Receive(W25Q64* self, uint16_t size) {
    while (HAL_SPI_Receive_DMA(&hspi2, self->receivingBuffer, size) != HAL_OK) {
        __NOP();
    }
    __HAL_DMA_DISABLE_IT(hspi2.hdmarx, DMA_IT_HT);
}



// weak must keep external linkage so application code can override it.
__attribute__((weak)) void W25Q64_Suspend(W25Q64* self)
{
    switch (self->isBusy) {
    case W25Q64_SPI_BUSY:
        break;

    case W25Q64_WAIT_FOR_ACK:
        break;

    default:
        break;
    }
}

void W25Q64_Busy(W25Q64* self) {
    W25Q64_Reset_CS(self);
    self->isBusy = W25Q64_WAIT_FOR_ACK;
    self->sendBuffer[0] = 0x05;
    self->isReceiving = 1;
    self->receiveSize = 1;
    self->transmit(self, 1);
    W25Q64_Suspend(self);
}

static void W25Q64_TransmitCallback(W25Q64* self) {
    if (self->isReceiving) {
        self->receive(self, self->receiveSize);
        self->receiveSize = 0;
        return;
    }
    if (self->isBusy == W25Q64_SPI_BUSY) {
        self->isBusy = W25Q64_OK;
    }

    W25Q64_Set_CS(self);
    //user code begin
}

static void W25Q64_ReceiveCallback(W25Q64* self) {
    W25Q64_Set_CS(self);
    self->isReceiving = 0;
    if (self->isBusy == W25Q64_SPI_BUSY) {
        self->isBusy = W25Q64_OK;
    }
    if (self->isBusy == W25Q64_WAIT_FOR_ACK) {
        if (!(self->receivingBuffer[0] & 0x01)) {
            self->isBusy = W25Q64_OK;
        } else {
            return;
        }
    }
    //user code begin
}

void W25Q64_Read(struct W25Q64 *self, uint16_t size, uint8_t *data) {
    while (self->isBusy) { W25Q64_Busy(self); }
    uint8_t readInstruction = 0x0b;
    self->sendBuffer[0] = readInstruction;
    self->sendBuffer[1] = self->FlashAddress >> 16;
    self->sendBuffer[2] = self->FlashAddress >> 8;
    self->sendBuffer[3] = self->FlashAddress >> 0;
    W25Q64_Reset_CS(self);
    self->receivingBuffer = data;
    self->receiveSize = size;
    self->isReceiving = 1;
    self->isBusy = W25Q64_SPI_BUSY;
    self->transmit(self, 5);
}

void W25Q64_Enable_Write(struct W25Q64 *self) {
    while (self->isBusy) { W25Q64_Busy(self); }
    W25Q64_Reset_CS(self);
    self->isBusy = W25Q64_SPI_BUSY;
    self->sendBuffer[0] = 0x06;
    self->transmit(self, 1);
}

void W25Q64_Write(struct W25Q64 *self, uint16_t size) {
    while (self->isBusy) { W25Q64_Busy(self); }
    W25Q64_Enable_Write(self);
    while (self->isBusy) {}
    self->FlashAddress = self->FlashAddress;
    uint8_t writeInstruction = 0x02;
    self->sendBuffer[0] = writeInstruction;
    self->sendBuffer[1] = self->FlashAddress >> 16;
    self->sendBuffer[2] = self->FlashAddress >> 8;
    self->sendBuffer[3] = self->FlashAddress >> 0;
    W25Q64_Reset_CS(self);
    self->isReceiving = 0;
    self->receiveSize = size;
    self->isBusy = W25Q64_SPI_BUSY;
    self->transmit(self, W25Q64_PAGE_SIZE + 4);
    //记得发送callback
}

void W25Q64_Erase_4KB(struct W25Q64 *self) {
    while (self->isBusy) { W25Q64_Busy(self); }
    self->FlashAddress = self->FlashAddress;
    uint8_t eraseInstruction = 0x20;
    //usual 0x20
    //0xd8 to erase more
    //0xc7 erase all
    self->sendBuffer[0] = eraseInstruction;
    self->sendBuffer[1] = self->FlashAddress >> 16;
    self->sendBuffer[2] = self->FlashAddress >> 8;
    self->sendBuffer[3] = self->FlashAddress >> 0;
    W25Q64_Reset_CS(self);
    self->isReceiving = 0;
    self->receiveSize = 0;
    self->isBusy = W25Q64_IS_WORKING;
    self->transmit(self, 4);
}

void W25Q64_Erase_All(struct W25Q64 *self) {
    while (self->isBusy) { W25Q64_Busy(self); }
    uint8_t eraseInstruction = 0xc7;
    self->sendBuffer[0] = eraseInstruction;
    W25Q64_Reset_CS(self);
    self->isReceiving = 0;
    self->receiveSize = 0;
    self->isBusy = W25Q64_IS_WORKING;
    self->transmit(self, 1);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI2) {
        W25Q64_TransmitCallback(&w25q64);
    }
}
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI2) {
        W25Q64_ReceiveCallback(&w25q64);
    }
}

void HAL_W25Q64_Init(W25Q64* self) {
    for (volatile int i = 0; i < 10000;i++) {}
    W25Q64_Read(self, W25Q64_PAGE_SIZE, self->receivingBuffer);
    for (volatile int i = 0; i < 10000;i++) {}
    //fuck w25q64 ,you must read once after powering on to make sure
}
