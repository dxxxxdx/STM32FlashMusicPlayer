//
// Created by zys on 26-5-13.
//

#include "musicPlayer.h"

MusicPlayer musicPlayer;
uint8_t previousState;


void HAL_TM8211_PingPongTxCpltCallback(TM8211* self) {
    if (self->currentPtr == NONE_PINGPONGPTR){return;}

    if (musicPlayer.appEnable && musicPlayer.musicPlayerState == MUSICPLAYER_PLAYING) {
        // 【关键改变】不在中断里读 SPI！而是给主循环发任务通知
        // 告诉主线程：DAC 现在在播放 currentPtr，请你去填充满另一个 Buffer！
        musicPlayer.needDataPtr = self->currentPtr;
    } else if (musicPlayer.musicPlayerState == MUSICPLAYER_OUT_OF_PLAYING) {
        HAL_TM8211_Stop(musicPlayer.dac);
        musicPlayer.musicPlayerState = MUSICPLAYER_IDLE;
    }
}
        //notice:!!!!!!!!!!!!!!!!!!!!!!!!
        /*
         *必须把flash的数据对齐，如果你想要锯齿波，请走如下格式
         *dac输出强度
         *左 1 3 5 7 9
         *右 2 4 6 8 A
         *flash数据 0x0001,0x0002,0x0003,0x0004 ......
         * 我们还使用补码所以说千万不要把顺序搞混
         * 这傻逼玩意是大端，stm32傻逼i2s是不会给你换顺序的，只能裸存大端数据
         */

