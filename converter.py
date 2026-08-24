import struct
from pydub import AudioSegment


def convert_audio_to_lsbj_ready_bin(
    input_path, bin_path, target_frame_rate=22000
):
    print(f"正在读取: {input_path}")
    audio = AudioSegment.from_file(input_path)

    # 1. 自动拉满音量，防止贴地
    audio = audio.normalize()

    # 2. 严格对齐硬件参数
    audio = audio.set_channels(2)  # 双声道
    audio = audio.set_sample_width(2)  # 16位 (2字节)
    audio = audio.set_frame_rate(target_frame_rate)  # 匹配你的定时器触发频率

    # 3. 核心修改：直接提取原始数据
    # pydub 的 audio.raw_data 默认就是标准的、最纯净的小端（Little-Endian）int16 字节流！
    # 它的排列顺序就是：[左声道低8位, 左声道高8位, 右声道低8位, 右声道高8位]
    raw_data = audio.raw_data

    # 验证一下数据
    count = len(raw_data) // 2
    unpacked = struct.unpack(f"<{count}h", raw_data)
    print("前 10 个采样点小端数值:", list(unpacked[:10]))

    with open(bin_path, "wb") as f:
        f.write(raw_data)

    print(f"✨ 标准小端 bin 写入成功: {bin_path}")


# 运行转换
convert_audio_to_lsbj_ready_bin("test.mp3", "flash_audio.bin", target_frame_rate=22000)