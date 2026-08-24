import numpy as np


def generate_tm8211_sawtooth(filename="tm8211_test.bin", duration_sec=3, sample_rate=22058):
    num_samples = (int(sample_rate * duration_sec) // 64) * 64
    t = np.linspace(0, duration_sec, num_samples, endpoint=False)

    # 1. 生成标准的 440Hz 锯齿波
    # np.modff 获取小数部分实现线性上升
    freq = 440
    sawtooth = 2 * ((t * freq) % 1.0) - 1

    # 2. 映射到 Int16 范围 (约 80% 幅度，防止溢出炸音)
    amplitude = 25000
    left_channel = (sawtooth * amplitude).astype('<i2')

    # 右声道搞个 880Hz，方便你分辨左右
    right_sawtooth = 2 * ((t * (freq * 2)) % 1.0) - 1
    right_channel = (right_sawtooth * amplitude).astype('<i2')

    # 3. 严格交织 L-R-L-R
    stereo_data = np.empty((num_samples * 2,), dtype='<i2')
    stereo_data[0::2] = left_channel
    stereo_data[1::2] = right_channel

    with open(filename, "wb") as f:
        f.write(stereo_data.tobytes())




    print(f"Done! 文件大小: {len(stereo_data) * 2} 字节")


generate_tm8211_sawtooth()