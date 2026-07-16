//
// Created by dxxdx on 2026/7/6.
//

#include "buttonManager.h"

#include "stm32g0xx_hal.h"

#define TREAD_TIME_MS 100

__attribute__((always_inline)) inline uint32_t buttonGetTick(void)
{
    return HAL_GetTick();
}

button PA0button = {
    .clickCallback = StopSong,
    .holdCallback = GoDownloader,
    .TrigTick = 0,
    .waitingForTrig = 0,
    .enable = 1
};
button PA1button = {
    .clickCallback = PlaySong,
    .holdCallback = StopSong,
    .TrigTick = 0,
    .waitingForTrig = 0,
    .enable = 1
};




void bottonTrig(button* self)
{
    if (!self->enable) return;
    if (self->waitingForTrig == 0)
    {
        self->TrigTick = buttonGetTick();
        self->waitingForTrig = 1;
        return;
    }
    self->waitingForTrig = 0;
    if (buttonGetTick() - self->TrigTick>=TREAD_TIME_MS )
    {
        self->holdCallback();
    }
    else
    {
        self->clickCallback();
    }
}






























