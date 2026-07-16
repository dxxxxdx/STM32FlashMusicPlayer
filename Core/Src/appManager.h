//
// Created by zys on 26-5-13.
//

#ifndef APPMANAGER_H
#define APPMANAGER_H
#include "musicPlayer.h"

typedef enum {
    APP_NONE ,
    APP_FLASH_DOWNLOADER,
    APP_MUSIC_PLAYER,
    APP_COUNT
} AppType;
extern FlashDownloader downloader;
extern MusicPlayer musicPlayer;

typedef struct App {
    void* instance;
    void (*requestStop)(void* instance);
    void (*enable)(void* instance);
    void (*disable)(void* instance);
    uint8_t (*isIdle)(void* instance);
} App;

typedef struct AppManager {
    App apps[APP_COUNT];
    AppType currentApp;
} AppManager;

static inline void appReg(App* app,
                           void* instance,
                           void (*requestStop)(void* instance),
                           void (*enable)(void* instance),
                           void (*disable)(void* instance),
                           uint8_t (*isIdle)(void* instance)) {
    app->instance = instance;
    app->requestStop = requestStop;
    app->enable = enable;
    app->disable = disable;
    app->isIdle = isIdle;
}

static inline void flashDownloaderAppRequestStop(void* instance) {
    (void)instance;
}

static inline void flashDownloaderAppEnable(void* instance) {
    flashDownloaderEnable((FlashDownloader*)instance);
}

static inline void flashDownloaderAppDisable(void* instance) {
    flashDownloaderDisable((FlashDownloader*)instance);
}

static inline uint8_t flashDownloaderAppIsIdle(void* instance) {
    return ((FlashDownloader*)instance)->whatToDo == IDLE;
}

static inline void musicPlayerAppRequestStop(void* instance) {
    ((MusicPlayer*)instance)->musicPlayerState = MUSICPLAYER_IDLE;
}

static inline void musicPlayerAppEnable(void* instance) {
    musicPlayerEnable((MusicPlayer*)instance);
}

static inline void musicPlayerAppDisable(void* instance) {
    musicPlayerDisable((MusicPlayer*)instance);
}

static inline uint8_t musicPlayerAppIsIdle(void* instance) {
    return ((MusicPlayer*)instance)->musicPlayerState == MUSICPLAYER_IDLE;
}

static inline void appManagerInit(AppManager* appManager, FlashDownloader* downloader, MusicPlayer* player) {
    appManager->currentApp = APP_NONE;
    appReg(&appManager->apps[APP_FLASH_DOWNLOADER],
            downloader,
            flashDownloaderAppRequestStop,
            flashDownloaderAppEnable,
            flashDownloaderAppDisable,
            flashDownloaderAppIsIdle);
    appReg(&appManager->apps[APP_MUSIC_PLAYER],
            player,
            musicPlayerAppRequestStop,
            musicPlayerAppEnable,
            musicPlayerAppDisable,
            musicPlayerAppIsIdle);
}

static inline void switchApp(AppManager* appManager, AppType target) {
    if (appManager->currentApp == target) {
        return;
    }

    for (uint8_t appType = APP_FLASH_DOWNLOADER; appType < APP_COUNT; appType++) {
        appManager->apps[appType].requestStop(appManager->apps[appType].instance);
    }

    while (!appManager->apps[APP_FLASH_DOWNLOADER].isIdle(appManager->apps[APP_FLASH_DOWNLOADER].instance) ||
           !appManager->apps[APP_MUSIC_PLAYER].isIdle(appManager->apps[APP_MUSIC_PLAYER].instance)) {
        __NOP();
    }

    for (uint8_t appType = APP_FLASH_DOWNLOADER; appType < APP_COUNT; appType++) {
        appManager->apps[appType].disable(appManager->apps[appType].instance);
    }

    appManager->currentApp = target;
    if (appManager->currentApp != APP_NONE) {
        appManager->apps[appManager->currentApp].enable(appManager->apps[appManager->currentApp].instance);
    }
}
































































#endif //APPMANAGER_H
