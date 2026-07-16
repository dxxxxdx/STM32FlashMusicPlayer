//
// Created by zys on 26-5-12.
//
#ifndef VMUSICFS_H
#define VMUSICFS_H

#include <stdint.h>
#include "flashDownloader.h"
#define FS_SIZE_BYTES      256
#define ENTRY_SIZE         32
#pragma pack(push, 1) // 确保结构体按照1字节对齐，不产生填充
typedef struct {
    char     name[16];      // 歌名，不足补0
    uint32_t start_page;    // Flash 绝对起始地址 单位是页
    uint32_t end_page;      // Flash 绝对结束地址
    uint32_t file_size;     // 文件实际大小
    uint32_t crc32;         // 该条目的数据校验 (不含自身)
} MusicEntry;
#pragma pack(pop)

static const char* vMusicPrefix = "vMusicV1";
extern MusicEntry vMusicMem[8];
static inline uint8_t vMusicFS_Init(FlashDownloader * downloader) {
    readFromFlash(downloader);
    while (downloader->flash->isReceiving) __NOP();
    fastcpy(&vMusicMem[0], downloader->flash->receivingBuffer, FS_SIZE_BYTES >> 2);
    if ( strncmp(vMusicMem[0].name, vMusicPrefix,8) == 0) {
        while (!downloader->uart_fsm->readyToTransmit(downloader->uart_fsm)) __NOP();
        for (volatile int i = 0; i < 123456; ++i) {
        }
        downloader->uart_fsm->echoback(downloader->uart_fsm, "vMusicFS init success.\n", 24);
        return 1;
    }else {
        downloader->uart_fsm->echoback(downloader->uart_fsm, "vMusicFS init failed.\n", 23);
    }
    return 0;
    //TODO: CRC exam and remove shit "123456"
}






#endif
