//
// Created by zys on 26-5-11.
//

#ifndef FLASHDOWNLOADER_H
#define FLASHDOWNLOADER_H
#include <stdint.h>
#include <string.h>
#include "cmsis_gcc.h"
#include "w25q64.h"
#include "ch340.h"
#include "fastcpy.h"
static char info_app[] = "page:0x0000\n";
const static char hex_map[] = "0123456789ABCDEF";
typedef enum {
    IDLE = 0,
    WRITE,
    WAITING_FOR_DATA,
    DOWNLOAD_TO_FLASH,
    READ,
    ERASE,
    PRINT_INFO,
    CHANGE_PAGE,
    POWEROFF,
    ERASE_ALL
} FlashDownloaderState;

static const char echoback_words[8][16]={
    "downloader ok",
    "write ok",
    "read ok",
    "waiting for data",
    "erase start",
    "stop download",
    "erase all"
};
//懒癌犯了，总之8字节倍数就完了，他走8字节fifo

typedef struct FlashDownloader {
    void(*getInstruction)(struct  FlashDownloader* self ,uint8_t* data, uint16_t size);
    void(*running)(struct FlashDownloader* self);
    UART_FSM* uart_fsm;
    W25Q64* flash;
    volatile uint8_t whatToDo;
    volatile uint8_t appEnable;
    volatile uint16_t targetPage;

} FlashDownloader;



static inline void writeToFlash(FlashDownloader* self) {
    if (!self->uart_fsm->fullBufferRead) {
        self->uart_fsm->echoback(self->uart_fsm, echoback_words[3], sizeof(echoback_words[3]));
        //                    _oo0oo_
        //                   o8888888o
        //                   88" . "88
        //                   (| -_- |)
        //                   0\  =  /0
        //                 ___/`---'\___
        //               .' \\|     |// '.
        //              / \\|||  :  |||// \
        //             / _||||| -卍- |||||- \
        //            |   | \\\  -  /// |   |
        //            | \_|  ''\---/''  |_/ |
        //            \  .-\__  '-'  ___/-. /
        //          ___'. .'  /--.--\  `. .'___
        //       ."" '<  `.___\_<|>_/___.' >' "".
        //      | | :  `- \`.;`\ _ /`;.`/ - ` : | |
        //      \  \ `_.   \_ __\ /__ _/   .-` /  /
        //  =====`-.____`.___ \_____/___.-`___.-'=====
        //                    `=---='
        //
        //      佛祖保佑：UART 还在路上，别来抢状态机。
        //      这里默认三次写入期间不会被接收中断精准插队。
        //      其实对于64mhz主频，uart中断需要的us级别基本不会来
        self->uart_fsm->staticBufferLength = 0;
        self->whatToDo = WAITING_FOR_DATA;
        self->uart_fsm->fullBufferRead = 1;

        //等待数据状态
        return;
    }
    if (self->whatToDo == DOWNLOAD_TO_FLASH) {
        W25Q64_Enable_Write(self->flash);
        self->flash->FlashAddress = self->targetPage * W25Q64_PAGE_SIZE;
        //TODO:zero copy ,you can directly write from the receiving buffer to flash without copying to the send buffer



        self->uart_fsm->staticBufferLocked = 1;
        fastcpy(self->flash->BufferDataStart, self->uart_fsm->staticBuffer, W25Q64_PAGE_SIZE >> 2);
        //self->uart_fsm->echoback(self->uart_fsm, self->uart_fsm->staticBuffer, W25Q64_PAGE_SIZE);
        self->uart_fsm->staticBufferLocked = 0;
        W25Q64_Write(self->flash,  W25Q64_PAGE_SIZE);

        self->uart_fsm->echoback(self->uart_fsm, echoback_words[1], sizeof(echoback_words[1]));
        self->whatToDo = IDLE;
        self->uart_fsm->fullBufferRead = 0;
    }
    
}
static inline void readFromFlash(FlashDownloader* self) {
    self->flash->FlashAddress = self->targetPage * W25Q64_PAGE_SIZE;
    W25Q64_Read(self->flash, W25Q64_PAGE_SIZE, self->flash->receivingBuffer);
    while (self->flash->isReceiving)     __NOP();
    self->uart_fsm->echoback(self->uart_fsm, (const char*)self->flash->receivingBuffer, W25Q64_PAGE_SIZE);

    self->whatToDo = IDLE;
}

static inline void EraseFlash(FlashDownloader* self) {
    W25Q64_Enable_Write(self->flash);
    self->uart_fsm->echoback(self->uart_fsm, echoback_words[4], sizeof(echoback_words[4]));
    self->flash->FlashAddress = self->targetPage * W25Q64_PAGE_SIZE;
    W25Q64_Erase_4KB(self->flash);

    self->whatToDo = IDLE;
}

static void update_info_str(uint16_t pg) {
    info_app[7]  = hex_map[(pg >> 12) & 0x0F];
    info_app[8]  = hex_map[(pg >> 8)  & 0x0F];
    info_app[9]  = hex_map[(pg >> 4)  & 0x0F];
    info_app[10] = hex_map[pg & 0x0F];
}
// 高效的 ASCII 转十六进制数值
static inline uint8_t char2hex(uint8_t c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}
static inline void printInfo(FlashDownloader* self) {
    W25Q64_Busy(self->flash);
    if (self->flash->isBusy!=0) {
        self->uart_fsm->echoback(self->uart_fsm, "busy",5);
    
        self->whatToDo = IDLE;
        return;
    }

    update_info_str(self->targetPage);
    self->uart_fsm->echoback(self->uart_fsm, info_app, 11);


    self->whatToDo = IDLE;
}



static inline void changePage(FlashDownloader* self) {
    W25Q64_Busy(self->flash);
    if (self->flash->isBusy) return;

    uint8_t *buf = &self->uart_fsm->staticBuffer[1];

    uint16_t page = 0;
    page |= (uint16_t)char2hex(buf[0]) << 12;
    page |= (uint16_t)char2hex(buf[1]) << 8;
    page |= (uint16_t)char2hex(buf[2]) << 4;
    page |= (uint16_t)char2hex(buf[3]);

    self->targetPage = page;
    update_info_str(self->targetPage);
    self->uart_fsm->echoback(self->uart_fsm, info_app, 11);
    self->whatToDo = IDLE;
}
static inline void EraseAll(FlashDownloader* self) {

    W25Q64_Enable_Write(self->flash);
    self->uart_fsm->echoback(self->uart_fsm, echoback_words[6], sizeof(echoback_words[6]));
    self->flash->FlashAddress = self->targetPage * W25Q64_PAGE_SIZE;
    W25Q64_Erase_All(self->flash);
    self->whatToDo = IDLE;

}

static inline void onCollide() {
    __NOP();
}

static inline void getInstruction(FlashDownloader* self ,uint8_t* data, uint16_t size) {
    if (size == 0)return;
    uint8_t instruction = data[0];
    if (self->uart_fsm->fullBufferRead) {
        self->whatToDo = DOWNLOAD_TO_FLASH;
        return;
    }
    if (self->whatToDo != IDLE){onCollide();return;}
    switch (instruction) {
        case 'w':
            self->whatToDo = WRITE;
            break;
        case 'r':
            self->whatToDo = READ;
            break;
        case 'e':
            self->whatToDo = ERASE;
            break;
        case 'd':
            self->whatToDo = PRINT_INFO;
            break;
        case 'p':
            self->whatToDo = CHANGE_PAGE;
            break;
        case 'o':
            self->whatToDo = POWEROFF;
            break;
        case 'c':
            self->whatToDo = ERASE_ALL;
            break;
        default:
            self->uart_fsm->echoback(self->uart_fsm, echoback_words[0], sizeof(echoback_words[0]));
            break;
    }
}
extern uint8_t w25q64_sendingBuffer[W25Q64_PAGE_SIZE+W25Q64_BITSTREAM_HEAD];
extern  uint8_t w25q64_receivingBuffer[W25Q64_PAGE_SIZE];



static inline void flashDownloaderRunning(FlashDownloader* self) {
    if (self->appEnable == 0) {
        return;
    }
    switch(self->whatToDo) {
        case IDLE:
            break;
        case WRITE:
            writeToFlash(self);
            break;
        case WAITING_FOR_DATA:
            writeToFlash(self);
            break;
        case DOWNLOAD_TO_FLASH:
            writeToFlash(self);
            break;
        case READ:
            readFromFlash(self);
            break;
        case ERASE:
            EraseFlash(self);
            break;
        case PRINT_INFO:
            printInfo(self);
            break;
        case CHANGE_PAGE:
            changePage(self);
            break;
        case POWEROFF:
            self->appEnable = 0;
            self->whatToDo = IDLE;
            break;
        case ERASE_ALL:
            EraseAll(self);
            break;
        default:
            break;
    }
}

static inline void flashDownloaderNop(FlashDownloader* self)
{

}


static inline void flashDownloaderEnable(FlashDownloader* self) {
    self->running = flashDownloaderRunning;
    self->flash->sendBuffer =w25q64_sendingBuffer;
    self->flash->BufferDataStart = self->flash->sendBuffer + W25Q64_BITSTREAM_HEAD;
    //bind to default buffer to avoid memory leak
    //anyway sending buffer is only used in write to flash , it is safe to reuse the buffer in flash struct
    //if you want to explore , do not ignore that
    self->flash->receivingBuffer = w25q64_receivingBuffer;
    self->appEnable = 1;
}
static inline void flashDownloaderDisable(FlashDownloader* self)
{
    self->running = flashDownloaderNop;
    self->flash->sendBuffer = w25q64_sendingBuffer;
    self->flash->BufferDataStart = self->flash->sendBuffer + W25Q64_BITSTREAM_HEAD;
    self->flash->receivingBuffer = w25q64_receivingBuffer;
    self->appEnable = 0;
}

static inline void FlashDownloader_Init(FlashDownloader* self,W25Q64* flash,UART_FSM* uart_fsm) {
    self->uart_fsm = uart_fsm;
    self->flash = flash;
    self->getInstruction = getInstruction;
    self->targetPage = 0;
    self->whatToDo = IDLE;
    self->running = flashDownloaderRunning;
    self->appEnable = 0;
}










#endif //FLASHDOWNLOADER_H
