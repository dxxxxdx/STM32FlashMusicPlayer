//
// Created by dxxdx on 2026/7/7.
//

#include "displayer.h"

#define DISPLAY_LINE_CHARS 21

extern AppManager appManager;

// 显示层先直接绑定这几个全局对象，少传参，先凑合把界面跑起来。
Displayer displayer = {
    .oled = &g_oled,
    .appManager = &appManager,
    .musicPlayer = &musicPlayer,
    .downloader = &downloader,
    .lastAction = "-",
    .pendingAreas = 0,
};

// 所有显示拼接缓冲都放静态区，避免在函数栈上开字符串。
static char displayLine[DISPLAY_LINE_CHARS + 1];
static char musicName[17];
static char flashPage[7];

// 写一整行：left 从左边写，right 从右边贴，中间自动填空格。
static void displayerWriteLine(uint8_t page, const char* left, const char* right)
{
    uint8_t pos = 0;

    for (uint8_t i = 0; i < DISPLAY_LINE_CHARS; ++i) {
        displayLine[i] = ' ';
    }
    displayLine[DISPLAY_LINE_CHARS] = '\0';

    while (*left != '\0' && pos < DISPLAY_LINE_CHARS) {
        char ch = *left++;
        displayLine[pos++] = (ch >= ' ' && ch <= '~') ? ch : '?';
    }

    if (right[0] != '\0') {
        uint8_t len = 0;
        while (right[len] != '\0' && len < DISPLAY_LINE_CHARS) {
            ++len;
        }

        pos = DISPLAY_LINE_CHARS - len;
        for (uint8_t i = 0; i < len; ++i) {
            char ch = right[i];
            displayLine[pos + i] = (ch >= ' ' && ch <= '~') ? ch : '?';
        }
    }

    // oled_ShowString 内部会设置 page 位置，这里只管准备 21 个字符。
    oled_ShowString(displayer.oled, page, displayLine, DISPLAY_LINE_CHARS);
}

static const char* displayerAppName(AppType app)
{
    switch (app) {
        case APP_FLASH_DOWNLOADER:
            return "Downloader";
        case APP_MUSIC_PLAYER:
            return "Player";
        case APP_NONE:
        default:
            return "None";
    }
}

static const char* displayerMusicStateName(uint8_t state)
{
    switch (state) {
        case MUSICPLAYER_PLAYING:
            return "Playing";
        case MUSICPLAYER_PAUSE:
            return "Pause";
        case MUSICPLAYER_ERROR:
            return "Error";
        case MUSICPLAYER_OUT_OF_PLAYING:
            return "Ending";
        case MUSICPLAYER_IDLE:
        default:
            return "Idle";
    }
}

static void displayerFormatMusicName(void)
{
    uint8_t i = 0;
    MusicEntry* entry = displayer.musicPlayer->currentEntry;

    // 空 entry 就显示固定占位，不尝试读歌名。
    if (entry->file_size == 0) {
        const char* empty = "No music";
        while (empty[i] != '\0' && i < sizeof(musicName) - 1U) {
            musicName[i] = empty[i];
            ++i;
        }
        musicName[i] = '\0';
        return;
    }

    // flash 里的歌名最多 16 字节，这里拷到静态 musicName，顺手过滤不可显示字符。
    for (; i < sizeof(musicName) - 1U && i < sizeof(entry->name); ++i) {
        char ch = entry->name[i];
        if (ch == '\0') {
            break;
        }
        musicName[i] = (ch >= ' ' && ch <= '~') ? ch : '?';
    }
    musicName[i] = '\0';
}

static void displayerFormatFlashPage(void)
{
    static const char hex[] = "0123456789ABCDEF";
    uint16_t value = displayer.downloader->targetPage;

    // targetPage 转成 0x0000 这种短字符串，给 downloader 页面显示。
    flashPage[0] = '0';
    flashPage[1] = 'x';
    flashPage[2] = hex[(value >> 12) & 0x0F];
    flashPage[3] = hex[(value >> 8) & 0x0F];
    flashPage[4] = hex[(value >> 4) & 0x0F];
    flashPage[5] = hex[value & 0x0F];
    flashPage[6] = '\0';
}

static void displayerDrawStatic(void)
{
    // 静态行只在启动时画一次，后面 100ms 刷新不碰这些行。
    displayerWriteLine(0, "vMusic 1.0", "");
    displayerWriteLine(1, "PA0 Stop  Hold DL", "");
    displayerWriteLine(7, "PA1 Play  Hold Stop", "");
}

static void displayerRefreshApp(void)
{
    // page 2: 当前 app。
    displayerWriteLine(2, "App", displayerAppName(displayer.appManager->currentApp));
}

static void displayerRefreshState(void)
{
    // page 3: 当前 app 的主要状态。播放器显示播放状态，下载器只粗略显示忙/闲。
    if (displayer.appManager->currentApp == APP_MUSIC_PLAYER) {
        displayerWriteLine(3, "State", displayerMusicStateName(displayer.musicPlayer->musicPlayerState));
        return;
    }

    if (displayer.appManager->currentApp == APP_FLASH_DOWNLOADER) {
        displayerWriteLine(3, "State", displayer.downloader->whatToDo == IDLE ? "Idle" : "Busy");
        return;
    }

    displayerWriteLine(3, "State", "Idle");
}

static void displayerRefreshMusic(void)
{
    // page 4: 当前歌曲名。
    displayerFormatMusicName();
    displayerWriteLine(4, "Song", musicName);
}

static void displayerRefreshDownloader(void)
{
    // page 5: downloader 当前目标 flash 页。
    displayerFormatFlashPage();
    displayerWriteLine(5, "Flash page", flashPage);
}

static void displayerRefreshAction(void)
{
    // page 6: 最近一次按键动作。100ms 刷新没主动改它，按键回调只改 lastAction/pending。
    displayerWriteLine(6, "Key", displayer.lastAction);
}

void displayerBoot(void)
{
    // 启动时先画静态行，再标记所有动态行需要刷新。
    displayerDrawStatic();
    displayerRequest(DISPLAY_AREA_ALL);
}

void displayerRequest(uint8_t areas)
{
    // areas 是 DISPLAY_AREA_xxx 的 bit mask。谁想刷新哪块，就把对应 bit 置起来。
    displayer.pendingAreas |= areas;
}

void displayerNotifyAction(const char* action, uint8_t areas)
{
    // 给按键用：记录动作文本，并额外要求刷新 Key 那一行。
    displayer.lastAction = action;
    displayerRequest(areas | DISPLAY_AREA_ACTION);
}

void displayerRunning(void)
{
    // 主循环里调用。OLED 正在 DMA 发送时不能改 g_oledBuffer，直接下次再说。
    if (displayer.oled->status == OLED_BUSY) {
        return;
    }

    // 看到哪个 bit 被置位，就只刷新对应一个 page，然后立刻返回。
    uint8_t areas = displayer.pendingAreas;

    if ((areas & DISPLAY_AREA_APP) != 0) {
        displayer.pendingAreas &= (uint8_t)~DISPLAY_AREA_APP;
        displayerRefreshApp();
        return;
    }
    if ((areas & DISPLAY_AREA_STATE) != 0) {
        displayer.pendingAreas &= (uint8_t)~DISPLAY_AREA_STATE;
        displayerRefreshState();
        return;
    }
    if ((areas & DISPLAY_AREA_MUSIC) != 0) {
        displayer.pendingAreas &= (uint8_t)~DISPLAY_AREA_MUSIC;
        displayerRefreshMusic();
        return;
    }
    if ((areas & DISPLAY_AREA_DOWNLOADER) != 0) {
        displayer.pendingAreas &= (uint8_t)~DISPLAY_AREA_DOWNLOADER;
        displayerRefreshDownloader();
        return;
    }
    if ((areas & DISPLAY_AREA_ACTION) != 0) {
        displayer.pendingAreas &= (uint8_t)~DISPLAY_AREA_ACTION;
        displayerRefreshAction();
        return;
    }
}
