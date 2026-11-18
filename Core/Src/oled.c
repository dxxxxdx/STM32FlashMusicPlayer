//
// Created by zys on 26-4-30.
//

#include "oled.h"
#include "oledConst.h"
#include <string.h>
#include "stm32g0xx_hal.h"

extern I2C_HandleTypeDef hi2c2;

void HAL_Oled_Transmit(OLED* self)
{
    while (HAL_I2C_Master_Transmit_DMA(&hi2c2,
                                       self->address << 1,
                                       &self->oledBuffer->commandByte,
                                       self->dataLength + 1) != HAL_OK)
        {
        __NOP();
        }
    __HAL_DMA_DISABLE_IT(hi2c2.hdmatx, DMA_IT_HT);


    self->status = OLED_BUSY;
}

static void oled_WaitReady(OLED* self)
{
    while (self->status == OLED_BUSY) {
        __NOP();
    }
}

void OLED_TxCpltCallback(OLED* self)
{

}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c)
{
    if (hi2c->Instance == I2C2) {
        g_oled.status = OLED_OK;
        g_oled.transmitCallback(&g_oled);
    }
}

static void OLED_DmaErrorCallback(void)
{
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c)
{
    if (hi2c == &hi2c2 && (HAL_I2C_GetError(hi2c) & HAL_I2C_ERROR_DMA) != 0U) {
        OLED_DmaErrorCallback();
    }
}

oledBuffer g_oledBuffer  __attribute__((aligned(4))) = {0};

OLED g_oled = {
    .address = 0x3C,
    .status = 0,
    .dataLength = 0,
    .transmit = HAL_Oled_Transmit,
    .transmitCallback = OLED_TxCpltCallback,
    .oledBuffer = &g_oledBuffer,
};


//这段下面别给我耦合傻逼hal库


uint8_t oled_Init(OLED* self)
{
    oled_WaitReady(self);
    memcpy(self->oledBuffer->oledData, &oledInitBitstream[1], sizeof(oledInitBitstream) - 1);
    oled_WriteCmd(self, sizeof(oledInitBitstream) - 1);
    for (volatile int i = 0; i < 11451; ++i) {}//典中典之硬延时
    return 1;
}


void oled_WriteCmd(OLED* self, uint16_t len)
{
    oled_WaitReady(self);
    self->oledBuffer->commandByte = 0x00;
    self->dataLength = len;
    self->transmit(self);
}

void oled_WriteData(OLED* self, uint16_t len)
{
    oled_WaitReady(self);
    self->oledBuffer->commandByte = 0x40;
    self->dataLength = len;
    self->transmit(self);
}

void oled_SetPos(OLED* self, uint8_t page, uint8_t offset)
{
    oled_WaitReady(self);
    self->oledBuffer->oledData[0] = 0xb0 + page;
    self->oledBuffer->oledData[1] = ((offset & 0xf0) >> 4) | 0x10;
    self->oledBuffer->oledData[2] = offset & 0x0f;
    oled_WriteCmd(self, 3);
    oled_WaitReady(self);
}

void oled_ShowString(OLED* self, uint8_t page, const char* str, uint8_t charNum)
{
    uint16_t dataLength = 0;
    oled_WaitReady(self);

    oled_SetPos(self, page, 0);
    for (uint8_t i = 0; i < charNum && dataLength + 6 <= sizeof(self->oledBuffer->oledData); ++i) {
        uint8_t ch = str[i];
        if (ch == '\0') break;
        if (ch < ' ' || ch > '~') ch = '?';

        const uint8_t* font = asc2_8x6[ch - ' '];
        for (uint8_t j = 0; j < 6; ++j) {
            self->oledBuffer->oledData[dataLength++] = font[j];
        }
    }
    oled_WaitReady(self);
    oled_WriteData(self, dataLength);
}


void oled_Clear(OLED* self)
{
    oled_WaitReady(self);
    memset(self->oledBuffer->oledData, 0, 132);
    for (uint8_t page = 0; page < 8; ++page) {
        oled_SetPos(self, page, 1);
        oled_WaitReady(self);
        oled_WriteData(self, 132);
    }
}





































