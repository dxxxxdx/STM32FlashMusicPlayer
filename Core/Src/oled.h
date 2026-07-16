//
// Created by zys on 26-4-30.
//

#ifndef OLED_H
#define OLED_H

#include <stdint.h>

extern const uint8_t asc2_8x6[][6];
typedef struct
{
    uint8_t commandByte;
    uint8_t oledData [256];//如果你内存爆了，直接把这个变量改短点就行，自己手动刷包未尝不可
}oledBuffer;

typedef enum
{
    OLED_OK = 0,
    OLED_BUSY,
}OLED_STATUS;



extern oledBuffer g_oledBuffer;

typedef struct OLED {
    uint8_t address;
    uint8_t status;
    uint16_t dataLength;
    void (*transmit)(struct OLED* self); // 发送iic的电平比特流
    void (*transmitCallback)(struct OLED* self);
    oledBuffer* oledBuffer;
} OLED;

extern OLED g_oled;
uint8_t oled_Init(OLED* self);
void oled_WriteCmd(OLED* self, uint16_t len);
void oled_WriteData(OLED* self, uint16_t len);
void oled_SetPos(OLED* self, uint8_t page, uint8_t offset);
void oled_ShowString(OLED* self, uint8_t page, const char* str, uint8_t charNum);
void oled_Clear(OLED* self);
void HAL_Oled_Transmit(OLED* self);
void OLED_TxCpltCallback(OLED* self);

#endif //OLED_H
