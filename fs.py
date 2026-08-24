import os
import struct
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from pydub import AudioSegment

FLASH_PAGE_SIZE = 256
MAX_SONGS = 7
TARGET_FRAME_RATE = 22000
RAW_EXTENSIONS = {".bin", ".raw"}
PADDING_BYTE = b"\xFF"


class VMusicPackerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("vMusic 文件系统打包工具 (页索引版)")
        self.root.geometry("620x400")

        self.songs = []
        self.converted_infos = []

        self.create_widgets()

    def create_widgets(self):
        # 顶部工具栏
        frame_toolbar = tk.Frame(self.root, pady=10)
        frame_toolbar.pack(fill=tk.X, padx=10)

        btn_add = tk.Button(frame_toolbar, text="添加音频文件 (+)", command=self.add_file, bg="#e1f5fe")
        btn_add.pack(side=tk.LEFT, padx=5)

        btn_remove = tk.Button(frame_toolbar, text="移除选中 (-)", command=self.remove_file)
        btn_remove.pack(side=tk.LEFT, padx=5)

        btn_clear = tk.Button(frame_toolbar, text="清空列表", command=self.clear_files)
        btn_clear.pack(side=tk.LEFT, padx=5)

        btn_gen = tk.Button(frame_toolbar, text="生成 BIN 文件", command=self.generate_bin, bg="#c8e6c9",
                            font=("", 10, "bold"))
        btn_gen.pack(side=tk.RIGHT, padx=5)

        # 列表显示区
        columns = ("slot", "name", "size", "start", "end")
        self.tree = ttk.Treeview(self.root, columns=columns, show="headings", selectmode="browse")
        self.tree.heading("slot", text="槽位")
        self.tree.heading("name", text="文件名 (限15字)")
        self.tree.heading("size", text="转换后大小")
        self.tree.heading("start", text="起始页 (Page)")
        self.tree.heading("end", text="结束页 (Page)")

        self.tree.column("slot", width=40, anchor=tk.CENTER)
        self.tree.column("name", width=200, anchor=tk.W)
        self.tree.column("size", width=100, anchor=tk.CENTER)
        self.tree.column("start", width=100, anchor=tk.CENTER)
        self.tree.column("end", width=100, anchor=tk.CENTER)

        self.tree.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        # 底部状态栏
        self.status_var = tk.StringVar()
        self.status_var.set("就绪。生成的镜像 Page 0 是文件头，Page 1 起连续存放音乐数据。")
        status_label = tk.Label(self.root, textvariable=self.status_var, anchor=tk.W, fg="gray")
        status_label.pack(fill=tk.X, padx=10, pady=5)

    def update_list(self):
        for item in self.tree.get_children():
            self.tree.delete(item)

        for i, filepath in enumerate(self.songs):
            name_to_store = os.path.basename(filepath)
            info = self.converted_infos[i] if i < len(self.converted_infos) else None
            file_size = info["size"] if info else "生成时转换"
            start_page = info["start_page"] if info else "-"
            end_page = info["end_page"] if info else "-"

            self.tree.insert("", tk.END, values=(
                f"[{i + 1}]",
                name_to_store[:15],
                file_size,
                start_page,
                end_page
            ))

        self.status_var.set(f"当前已添加 {len(self.songs)}/{MAX_SONGS} 首歌曲。")

    def add_file(self):
        if len(self.songs) >= MAX_SONGS:
            messagebox.showwarning("数量上限", f"最多只能添加 {MAX_SONGS} 首歌！")
            return

        filepaths = filedialog.askopenfilenames(
            title="选择音频文件",
            filetypes=[
                ("Audio Files", "*.mp3 *.wav *.flac *.ogg *.m4a *.aac *.wma *.aiff *.opus *.bin *.raw"),
                ("All Files", "*.*"),
            ]
        )
        for filepath in filepaths:
            if len(self.songs) < MAX_SONGS:
                self.songs.append(filepath)
        self.converted_infos = []
        self.update_list()

    def remove_file(self):
        selected = self.tree.selection()
        if not selected: return
        index = self.tree.index(selected[0])
        del self.songs[index]
        self.converted_infos = []
        self.update_list()

    def clear_files(self):
        self.songs.clear()
        self.converted_infos = []
        self.update_list()

    def create_music_entry(self, name: str, start_page: int, end_page: int, file_size: int, crc32: int = 0) -> bytes:
        name_bytes = name.encode('utf-8', errors='ignore')[:15].ljust(16, b'\x00')
        # 把以"页"为单位的索引打包进去
        return struct.pack('<16sIIII', name_bytes, start_page, end_page, file_size, crc32)

    def convert_audio_file(self, filepath: str) -> bytes:
        ext = os.path.splitext(filepath)[1].lower()
        if ext in RAW_EXTENSIONS:
            with open(filepath, "rb") as f:
                return f.read()

        if AudioSegment is None:
            raise RuntimeError("缺少 pydub，无法转换音频。请先在当前 Python 环境安装 pydub。")

        audio = AudioSegment.from_file(filepath)
        audio = audio.normalize()
        audio = audio.set_channels(2)
        audio = audio.set_sample_width(2)
        audio = audio.set_frame_rate(TARGET_FRAME_RATE)
        return audio.raw_data

    def build_image(self):
        converted_songs = []
        infos = []
        current_page = 1

        for filepath in self.songs:
            raw_data = self.convert_audio_file(filepath)
            file_size = len(raw_data)
            if file_size == 0:
                raise ValueError(f"{os.path.basename(filepath)} 转换后没有数据")

            pages_needed = (file_size + FLASH_PAGE_SIZE - 1) // FLASH_PAGE_SIZE
            start_page = current_page
            end_page = start_page + pages_needed - 1
            pad_len = (FLASH_PAGE_SIZE - (file_size % FLASH_PAGE_SIZE)) % FLASH_PAGE_SIZE

            converted_songs.append(raw_data + PADDING_BYTE * pad_len)
            infos.append({
                "path": filepath,
                "name": os.path.basename(filepath),
                "size": file_size,
                "start_page": start_page,
                "end_page": end_page,
            })
            current_page += pages_needed

        entries = [self.create_music_entry("vMusicV1", 0, 0, 0, 0)]
        for info in infos:
            entries.append(self.create_music_entry(
                info["name"],
                info["start_page"],
                info["end_page"],
                info["size"],
                0
            ))

        while len(entries) < 8:
            entries.append(self.create_music_entry("", 0, 0, 0, 0))

        header = b"".join(entries)
        if len(header) != FLASH_PAGE_SIZE:
            raise ValueError(f"文件头大小异常: {len(header)} bytes")

        return header + b"".join(converted_songs), infos

    def generate_bin(self):
        if not self.songs:
            messagebox.showerror("错误", "请至少添加一首歌曲！")
            return

        save_path = filedialog.asksaveasfilename(
            defaultextension=".bin",
            initialfile="flash_audio.bin",
            title="保存完整镜像文件"
        )
        if not save_path: return

        try:
            self.status_var.set("正在转换音频并生成完整镜像...")
            self.root.update_idletasks()
            image_data, infos = self.build_image()
            with open(save_path, 'wb') as f:
                f.write(image_data)
            self.converted_infos = infos
            self.update_list()
            messagebox.showinfo(
                "成功",
                f"完整镜像已保存！\n\n{save_path}\n\n"
                f"总大小: {len(image_data)} bytes / {len(image_data) // FLASH_PAGE_SIZE} pages\n"
                f"可直接用 downloader.py 从 Page 0 烧录。"
            )
        except Exception as e:
            messagebox.showerror("保存失败", str(e))


if __name__ == "__main__":
    root = tk.Tk()
    root.attributes('-topmost', True)
    root.after_idle(root.attributes, '-topmost', False)
    app = VMusicPackerApp(root)
    root.mainloop()
