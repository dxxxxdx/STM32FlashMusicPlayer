import serial
import time
import sys
import os
import logging

# ==========================================
# 可配置参数 (宏定义风格)
# ==========================================

PORT = "/dev/ttyUSB0"
FIRMWARE_FILE = "flash_audio.bin"
BAUD_RATE = 1500000
START_PAGE = 0x0000
LOG_FILE_NAME = "log"

FLASH_PAGE_SIZE = 256

# --- 延时配置 (单位：秒) ---
## 串口指令发送后，到读取设备回显/状态前的间隔
#DELAY_CMD = 0.0001
## 收到写入等待信号后，到发送页数据前的间隔
#DELAY_BEFORE_DATA = 0.0001
## 页数据写入串口后，到读取写入确认回显前的间隔
#DELAY_DATA_CHUNK = 0.0003
## 页切换/擦除/写入结束后，到下一次业务操作前的间隔
#DELAY_INTER_OP = 0.0001
## 查忙轮询中，本次未就绪后到下一次查询前的间隔
#DELAY_POLL_INTERVAL = 0.0002
## 匹配到目标回显后，到读取可能尾随串口数据前的间隔
#DELAY_READ_TRAILING = 0.0002
## 等待目标回显时，两次检查串口缓冲区之间的间隔
#DELAY_READ_POLL = 0.0001


# 串口指令发送后，到读取设备回显/状态前的间隔
DELAY_CMD = 0.000
# 收到写入等待信号后，到发送页数据前的间隔
DELAY_BEFORE_DATA = 0.000
# 页数据写入串口后，到读取写入确认回显前的间隔
DELAY_DATA_CHUNK = 0.000
# 页切换/擦除/写入结束后，到下一次业务操作前的间隔
DELAY_INTER_OP = 0.000
# 查忙轮询中，本次未就绪后到下一次查询前的间隔
DELAY_POLL_INTERVAL = 0.000
# 匹配到目标回显后，到读取可能尾随串口数据前的间隔
DELAY_READ_TRAILING = 0.000
# 等待目标回显时，两次检查串口缓冲区之间的间隔
DELAY_READ_POLL = 0.000
# 打开串口后，到第一次通信前等待设备稳定的间隔
DELAY_SERIAL_INIT = 1.5

# --- 超时设置 (单位: 秒) ---
TIMEOUT_POLL = 10.0
TIMEOUT_WRITE = 3.0
TIMEOUT_SERIAL_INIT = 1.0


# ==========================================
# 日志系统初始化
# ==========================================
def setup_logger():
    logger = logging.getLogger("UART_Stream")
    logger.setLevel(logging.DEBUG)
    file_handler = logging.FileHandler(LOG_FILE_NAME, mode='w', encoding='utf-8')
    file_handler.setLevel(logging.DEBUG)
    formatter = logging.Formatter('[%(asctime)s.%(msecs)03d] %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
    file_handler.setFormatter(formatter)
    logger.addHandler(file_handler)
    return logger


logger = setup_logger()


# ==========================================
# 底层串口收发包装
# ==========================================
def uart_write(ser, data):
    logger.info(f"TX : {repr(data)}")
    ser.write(data)


def read_until_match(ser, expected_str, timeout):
    start_time = time.time()
    buffer = b''

    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            chunk = ser.read(ser.in_waiting)
            buffer += chunk
            logger.info(f"RX : {repr(chunk)}")

        if expected_str in buffer:
            time.sleep(DELAY_READ_TRAILING)
            if ser.in_waiting > 0:
                extra = ser.read(ser.in_waiting)
                buffer += extra
                logger.info(f"RX (Trailing): {repr(extra)}")
            return True, buffer

        time.sleep(DELAY_READ_POLL)

    return False, buffer


# ==========================================
# 核心烧录逻辑
# ==========================================
def wait_for_ready(ser, timeout=TIMEOUT_POLL):
    start_time = time.time()
    while time.time() - start_time < timeout:
        ser.reset_input_buffer()
        uart_write(ser, b'd')
        time.sleep(DELAY_CMD)

        success, resp = read_until_match(ser, b"page:", 0.2)
        if success:
            return True

        time.sleep(DELAY_POLL_INTERVAL)
    return False


def change_page(ser, page):
    cmd = f"p{page:04X}".encode('ascii')
    ser.reset_input_buffer()
    uart_write(ser, cmd)
    time.sleep(DELAY_CMD)

    success, resp = read_until_match(ser, b"page:", 1.0)
    if not success:
        logger.warning(f"切换页 0x{page:04X} 未收到正确回显")
    time.sleep(DELAY_INTER_OP)


def write_page(ser, page, data):
    change_page(ser, page)

    ser.reset_input_buffer()
    uart_write(ser, b'w')
    time.sleep(DELAY_CMD)

    success, resp = read_until_match(ser, b"waiting", TIMEOUT_WRITE)
    if not success:
        print(f"\n[错误] 写入页 0x{page:04X} 未收到等待接收信号!")
        return False

    time.sleep(DELAY_BEFORE_DATA)

    ser.reset_input_buffer()
    uart_write(ser, data)
    time.sleep(DELAY_DATA_CHUNK)

    success, resp = read_until_match(ser, b"write ok", TIMEOUT_WRITE)
    if not success:
        print(f"\n[错误] 页 0x{page:04X} 数据发送后未收到确认!")
        return False

    if not wait_for_ready(ser, timeout=TIMEOUT_WRITE):
        print(f"\n[错误] 写入页 0x{page:04X} 后芯片查忙超时！")
        return False

    time.sleep(DELAY_INTER_OP)
    return True


# ==========================================
# 主业务流水线
# ==========================================
def main():
    if not os.path.exists(FIRMWARE_FILE):
        print(f"[错误] 固件文件 '{FIRMWARE_FILE}' 不存在！")
        sys.exit(1)

    with open(FIRMWARE_FILE, 'rb') as f:
        firmware = f.read()

    pad_len = (FLASH_PAGE_SIZE - (len(firmware) % FLASH_PAGE_SIZE)) % FLASH_PAGE_SIZE
    if pad_len > 0:
        firmware += b'\xFF' * pad_len

    total_pages = len(firmware) // FLASH_PAGE_SIZE
    end_page = START_PAGE + total_pages - 1

    print(f"--- 稳健型控制台 ---")
    print(f"写入策略:   [不擦除] [不读回校验] [全时忙检测]")

    try:
        ser = serial.Serial(PORT, BAUD_RATE, timeout=TIMEOUT_SERIAL_INIT)
        time.sleep(DELAY_SERIAL_INIT)
    except Exception as e:
        print(f"[错误] 无法建立串口连接 {PORT}: {e}")
        sys.exit(1)

    error_count = 0

    try:
        ser.reset_input_buffer()
        wait_for_ready(ser, timeout=TIMEOUT_WRITE)

        start_time = time.time()

        for i in range(total_pages):
            current_page = START_PAGE + i

            print(f"\r[*] 进度: [{i + 1}/{total_pages}] 页 0x{current_page:04X} | 差错累计: {error_count}", end='')

            page_data = firmware[i * FLASH_PAGE_SIZE: (i + 1) * FLASH_PAGE_SIZE]

            # 执行写入
            if not write_page(ser, current_page, page_data):
                error_count += 1
                continue  # 写入死机则直接跳下一页

        elapsed = time.time() - start_time
        print(f"\n\n==========================================")
        print(f"[*] 任务顶风冒雨跑完了！总耗时: {elapsed:.2f} 秒。")
        print(f"[*] 累计差错页数: {error_count} 页")
        print(f"==========================================")

    except KeyboardInterrupt:
        print("\n[中断] 用户手动中止。")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
