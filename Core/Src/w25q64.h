//
// Created by zys on 26-5-11.
//

#ifndef W25Q64_H
#define W25Q64_H
#include <stdint.h>


#define W25Q64_PAGE_SIZE 256
#define W25Q64_BITSTREAM_HEAD 4
typedef enum {
    W25Q64_OK = 0,
    W25Q64_SPI_BUSY,
    W25Q64_IS_WORKING,
    W25Q64_WAIT_FOR_ACK
} W25Q64_Status;

typedef struct W25Q64 {
    uint32_t FlashAddress;
    volatile uint32_t* CS_BSRR;
    uint32_t CS_PIN_MASK;
    uint8_t* sendBuffer;
    uint8_t* BufferDataStart;
    uint8_t* receivingBuffer;
     void(*const transmit)(struct W25Q64 *self, uint16_t size);
     void(*const receive)(struct W25Q64 *self,uint16_t size);
    volatile uint8_t isReceiving;
    volatile uint8_t isBusy;
    volatile uint16_t receiveSize;
}W25Q64;

static inline void W25Q64_Set_CS(W25Q64* self) {
    *(self->CS_BSRR) = self->CS_PIN_MASK;
}

static inline void W25Q64_Reset_CS(W25Q64* self) {
    *(self->CS_BSRR) = self->CS_PIN_MASK << 16;
}

void W25Q64_Suspend(W25Q64* self);
void W25Q64_Busy(W25Q64* self);
void W25Q64_Read(struct W25Q64 *self, uint16_t size, uint8_t *data);
void W25Q64_Enable_Write(struct W25Q64 *self);
void W25Q64_Write(struct W25Q64 *self, uint16_t size);
void W25Q64_Erase_4KB(struct W25Q64 *self);
void W25Q64_Erase_All(struct W25Q64 *self);
void HAL_W25Q64_Receive(W25Q64* self,uint16_t size) ;
void HAL_W25Q64_Transmit(W25Q64* self, uint16_t size) ;

void HAL_W25Q64_Init(W25Q64* self);

#endif //W25Q64_H
