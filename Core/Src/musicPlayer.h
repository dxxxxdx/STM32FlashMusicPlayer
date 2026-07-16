//
// Created by zys on 26-5-13.
//

#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H
#include <stdint.h>
#include "flashDownloader.h"
#include "TM8211.h"
#include "VmusicFS.h"

extern int16_t pingBuffer[W25Q64_PAGE_SIZE/2];
extern int16_t pongBuffer[W25Q64_PAGE_SIZE/2];


typedef enum{
    MUSICPLAYER_IDLE = 0,
    MUSICPLAYER_PLAYING,
    MUSICPLAYER_PAUSE,
    MUSICPLAYER_ERROR,
    MUSICPLAYER_OUT_OF_PLAYING,
}MusicPlayerState;
typedef enum {
    USER_OP_NONE = 0,
    USER_OP_PLAY,
    USER_OP_PAUSE,
    USER_OP_STOP,
    USER_OP_NEXT,
    USER_OP_PREV
} UserNextOperation;

typedef struct MusicPlayer {
    void(*running)(struct MusicPlayer* self);
    TM8211* dac;
    W25Q64* flash;
    MusicEntry* musicMem;
    MusicEntry* currentEntry;
    uint32_t currentMusicAddress;
    volatile uint8_t musicPlayerState;
    volatile uint8_t userNextOperation;
    volatile uint8_t appEnable;
    volatile uint8_t needDataPtr;
    volatile uint8_t previousState;
    volatile uint8_t cyclePlay;
} MusicPlayer;
extern MusicPlayer player;




static inline void musicPlayerStartPlay(MusicPlayer* self) {
    HAL_TM8211_Start(self->dac);
    self->musicPlayerState = MUSICPLAYER_PLAYING;
}

static inline void musicPlayerStop(MusicPlayer* self) {
    HAL_TM8211_Stop(self->dac);
    self->musicPlayerState = MUSICPLAYER_IDLE;
    self->needDataPtr = NONE_PINGPONGPTR;
}

static inline void musicPlayerPause(MusicPlayer* self) {
    HAL_TM8211_Stop(self->dac);
    self->musicPlayerState = MUSICPLAYER_PAUSE;
    self->needDataPtr = NONE_PINGPONGPTR;
}

static inline void musicPlayerResetCurrent(MusicPlayer* self) {
    self->currentMusicAddress = self->currentEntry->start_page;
    self->needDataPtr = NONE_PINGPONGPTR;
}
static inline void musicPlayerEnable(MusicPlayer* self){
    self->musicPlayerState = MUSICPLAYER_IDLE;
    self->flash->receivingBuffer = (uint8_t*) (pingBuffer);
    self->appEnable = 1;
}

static inline void musicPlayerDisable(MusicPlayer* self)
{
    self->musicPlayerState = MUSICPLAYER_IDLE;
    self->flash->receivingBuffer = NULL;
    self->appEnable = 0;
    musicPlayerStop(self);
}

static inline void switchSong(MusicPlayer* self, int8_t direction) {
    // vMusicMem[0] 是文件系统头，真正的歌曲目录从 1 开始。
    enum {
        FIRST_MUSIC_ENTRY = 1,
        MUSIC_ENTRY_COUNT = FS_SIZE_BYTES / ENTRY_SIZE
    };
    int currentIndex = self->currentEntry - self->musicMem;

    if (currentIndex < FIRST_MUSIC_ENTRY || currentIndex >= MUSIC_ENTRY_COUNT) {
        musicPlayerStop(self);
        return;
    }

    for (int scanned = FIRST_MUSIC_ENTRY; scanned < MUSIC_ENTRY_COUNT; scanned++) {
        currentIndex += direction;

        // cyclePlay 只管歌单边界：0 到边界就停，1 到边界就绕回另一端。
        if (currentIndex < FIRST_MUSIC_ENTRY) {
            if (!self->cyclePlay) {
                musicPlayerStop(self);
                return;
            }
            currentIndex = MUSIC_ENTRY_COUNT - 1;
        } else if (currentIndex >= MUSIC_ENTRY_COUNT) {
            if (!self->cyclePlay) {
                musicPlayerStop(self);
                return;
            }
            currentIndex = FIRST_MUSIC_ENTRY;
        }

        // 空 entry 没有文件；start/end 的单位都是 flash 页，end_ 是右开边界。
        if (self->musicMem[currentIndex].file_size != 0 &&
            self->musicMem[currentIndex].end_page > self->musicMem[currentIndex].start_page) {
            self->currentEntry = &self->musicMem[currentIndex];
            self->currentMusicAddress = self->currentEntry->start_page;
            return;
        }
    }

    musicPlayerStop(self);
}

static inline uint32_t getNextAddress(MusicPlayer* self){
    // currentMusicAddress 存页号；W25Q64_Read 需要字节地址。
    if (self->currentMusicAddress >= self->currentEntry->end_page) {
        switchSong(self, 1);
        if (self->musicPlayerState != MUSICPLAYER_PLAYING) {
            return self->currentMusicAddress << 8;
        }
    }

    return (self->currentMusicAddress++ ) << 8 ;
}

static inline void musicPlayerFresh(MusicPlayer* self) {
    if (self->needDataPtr != NONE_PINGPONGPTR) {
        // 判断应该填哪一个
        self->flash->receivingBuffer = (uint8_t*)(self->needDataPtr == PING ? pongBuffer : pingBuffer);

        self->flash->FlashAddress = getNextAddress(self);
        if (self->musicPlayerState != MUSICPLAYER_PLAYING) {
            self->needDataPtr = NONE_PINGPONGPTR;
            return;
        }

        // 发起 DMA 读取
        W25Q64_Read(self->flash, W25Q64_PAGE_SIZE, self->flash->receivingBuffer);
        // 清除标记，等待下一次中断呼叫
        self->needDataPtr = NONE_PINGPONGPTR;
    }
}




static inline void musicPlayerRunning(MusicPlayer* self) {
    if (self->appEnable == 0) {
        return;
    }
    if (self->userNextOperation != USER_OP_NONE)
    {
        uint8_t op = self->userNextOperation;
        self->userNextOperation = USER_OP_NONE;

        switch (op) {
            case USER_OP_PLAY:
                if (self->musicPlayerState != MUSICPLAYER_PLAYING) {
                    musicPlayerStartPlay(self);
                }
                break;
            case USER_OP_PAUSE:
                if (self->musicPlayerState == MUSICPLAYER_PLAYING) {
                    musicPlayerPause(self);
                }
                break;
            case USER_OP_STOP:
                musicPlayerStop(self);
                musicPlayerResetCurrent(self);
                break;
            case USER_OP_NEXT:
            case USER_OP_PREV: {
                uint8_t wasPlaying = self->musicPlayerState == MUSICPLAYER_PLAYING;
                switchSong(self, op == USER_OP_NEXT ? 1 : -1);
                if (wasPlaying && self->musicPlayerState != MUSICPLAYER_IDLE) {
                    musicPlayerStartPlay(self);
                }
                break;
            }
            default:
                break;
        }
    }
    if (self->musicPlayerState == MUSICPLAYER_PLAYING)
    {
        if (self->previousState == MUSICPLAYER_IDLE && self->dac->isWorking != TM8211_WORKING) {
            musicPlayerStartPlay(self);
        }
        musicPlayerFresh(self);
    }
    self->previousState = self->musicPlayerState;
}



static inline void musicPlayerInit(MusicPlayer* self, TM8211* dac, W25Q64* flash,MusicEntry* entry) {

    self->dac = dac;
    self->flash = flash;
    self->musicMem = entry;
    self->currentEntry = entry + 1;
    self->currentMusicAddress = self->currentEntry->start_page;
    self->musicPlayerState = MUSICPLAYER_IDLE;
    self->userNextOperation = USER_OP_NONE;
    self->appEnable = 0;
    self->running = musicPlayerRunning;
    self->needDataPtr = NONE_PINGPONGPTR;
    self->previousState = MUSICPLAYER_IDLE;
    self->cyclePlay = 1;

}



#endif //MUSICPLAYER_H
