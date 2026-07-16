//
// Created by dxxdx on 2026/7/7.
//

#ifndef G030DEV_DISPLAYER_H
#define G030DEV_DISPLAYER_H
#include "oled.h"
#include <stdint.h>
#include "appManager.h"

#define DISPLAY_AREA_APP        (1U << 0)
#define DISPLAY_AREA_STATE      (1U << 1)
#define DISPLAY_AREA_MUSIC      (1U << 2)
#define DISPLAY_AREA_DOWNLOADER (1U << 3)
#define DISPLAY_AREA_ACTION     (1U << 4)
#define DISPLAY_AREA_ALL        (DISPLAY_AREA_APP | DISPLAY_AREA_STATE | DISPLAY_AREA_MUSIC | DISPLAY_AREA_DOWNLOADER | DISPLAY_AREA_ACTION)

typedef struct {
    OLED* oled;
    AppManager* appManager;
    MusicPlayer* musicPlayer;
    FlashDownloader* downloader;
    const char* lastAction;
    volatile uint8_t pendingAreas;
} Displayer;

extern Displayer displayer;

void displayerBoot(void);
void displayerRequest(uint8_t areas);
void displayerNotifyAction(const char* action, uint8_t areas);
void displayerRunning(void);




























#endif //G030DEV_DISPLAYER_H
