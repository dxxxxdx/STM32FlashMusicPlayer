//
// Created by dxxdx on 2026/7/6.
//

#include "buttonManager.h"
#include "appManager.h"
#include "displayer.h"

extern MusicPlayer musicPlayer;
extern AppManager appManager;

static void ButtonExtiCallback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        bottonTrig(&PA0button);
    }
    else if (GPIO_Pin == GPIO_PIN_1)
    {
        bottonTrig(&PA1button);
    }
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    ButtonExtiCallback(GPIO_Pin);
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    ButtonExtiCallback(GPIO_Pin);
}

void NextSong(void)
{
    musicPlayer.userNextOperation = USER_OP_NEXT;
    displayerNotifyAction("Next", DISPLAY_AREA_STATE | DISPLAY_AREA_MUSIC);
}

void PrevSong(void)
{
    musicPlayer.userNextOperation = USER_OP_PREV;
    displayerNotifyAction("Prev", DISPLAY_AREA_STATE | DISPLAY_AREA_MUSIC);
}

void PlaySong(void)
{
    switchApp(&appManager, APP_MUSIC_PLAYER);
    musicPlayerStartPlay(&musicPlayer);
    displayerNotifyAction("Play", DISPLAY_AREA_APP | DISPLAY_AREA_STATE | DISPLAY_AREA_MUSIC);
}

void StopSong(void)
{
    musicPlayer.userNextOperation = USER_OP_STOP;
    musicPlayerStop(&musicPlayer);
    displayerNotifyAction("Stop", DISPLAY_AREA_STATE | DISPLAY_AREA_MUSIC);
}

void GoDownloader(void)
{
    switchApp(&appManager, APP_FLASH_DOWNLOADER);
    displayerNotifyAction("Downloader", DISPLAY_AREA_APP | DISPLAY_AREA_STATE | DISPLAY_AREA_DOWNLOADER);
}

void NothingHappen(void)
{
    displayerNotifyAction("None", DISPLAY_AREA_ACTION);
}




















