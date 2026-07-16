//
// Created by dxxdx on 2026/7/6.
//

#ifndef G030DEV_BOTTONMANAGER_H
#define G030DEV_BOTTONMANAGER_H
#include <stdint.h>


typedef struct
{
    void (*clickCallback)(void);
    void (*holdCallback)(void);
    uint32_t TrigTick;
    uint8_t waitingForTrig;
    uint8_t enable;
}button;


void bottonTrig(button* self);
extern button PA0button;
extern button PA1button;

uint32_t buttonGetTick(void);
void NextSong(void);
void PrevSong(void);
void PlaySong(void);
void StopSong(void);
void GoDownloader(void);
void NothingHappen(void);


















#endif //G030DEV_BOTTONMANAGER_H
