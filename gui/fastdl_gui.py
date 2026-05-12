#!/usr/bin/env python3
"""
FastDL Tool  –  GUI Frontend
Language : Python 3.10+  /  PyQt6
Backend  : C++ shared library (libfastdl.so / fastdl.dll / libfastdl.dylib)
           called via ctypes for all download / extraction work.

Run:
    python fastdl_gui.py

Requirements:
    pip install PyQt6
    (C++ backend must be built first – see build_lib.sh / build_lib.bat)
"""

import sys
import os
import json
import ctypes
import ctypes.util
import platform
import threading
import time
from pathlib import Path
from typing import Optional

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QTabWidget,
    QVBoxLayout, QHBoxLayout, QGridLayout, QFormLayout,
    QLabel, QLineEdit, QPushButton, QComboBox, QSpinBox,
    QCheckBox, QProgressBar, QTextEdit, QListWidget, QListWidgetItem,
    QGroupBox, QFileDialog, QMessageBox, QDialog,
    QDialogButtonBox, QFrame, QSplitter, QSizePolicy,
    QScrollArea, QToolButton, QStatusBar, QAbstractItemView,
)
from PyQt6.QtCore import (
    Qt, QThread, pyqtSignal, QTimer, QSize, QSettings,
)
from PyQt6.QtGui import (
    QFont, QColor, QPalette, QIcon, QAction, QPixmap, QTextCursor,
)


def _find_lib() -> Optional[str]:
    """Locate the compiled C++ shared library next to this script."""
    here = Path(__file__).parent
    candidates = [
        here / "libfastdl.so",
        here / "fastdl.dll",
        here / "libfastdl.dylib",
        here / "build" / "libfastdl.so",
        here / "build" / "Release" / "fastdl.dll",
        here / "build" / "libfastdl.dylib",
    ]
    for p in candidates:
        if p.exists():
            return str(p)
    return None


def _load_lib(path: str):
    lib = ctypes.CDLL(path)

    lib.fdl_create.restype  = ctypes.c_void_p
    lib.fdl_create.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]

    lib.fdl_destroy.restype  = None
    lib.fdl_destroy.argtypes = [ctypes.c_void_p]

    lib.fdl_set_resource_types.restype  = None
    lib.fdl_set_resource_types.argtypes = [
        ctypes.c_void_p,
        ctypes.POINTER(ctypes.c_char_p),
        ctypes.c_int,
    ]

    lib.fdl_set_max_retries.restype  = None
    lib.fdl_set_max_retries.argtypes = [ctypes.c_void_p, ctypes.c_int]
    lib.fdl_set_timeout.restype  = None
    lib.fdl_set_timeout.argtypes = [ctypes.c_void_p, ctypes.c_int]

    PROGRESS_CB = ctypes.CFUNCTYPE(
        None,
        ctypes.c_char_p,   # filename
        ctypes.c_double,   # progress
        ctypes.c_longlong, # downloaded
        ctypes.c_longlong, # total
        ctypes.c_double,   # speed
        ctypes.c_double,   # eta
        ctypes.c_int,      # done_files
        ctypes.c_longlong, # done_bytes
        ctypes.c_void_p,   # userdata
    )
    lib._PROGRESS_CB = PROGRESS_CB

    EVENT_CB = ctypes.CFUNCTYPE(
        None,
        ctypes.c_int,      # event
        ctypes.c_char_p,   # filename
        ctypes.c_char_p,   # detail
        ctypes.c_longlong, # size
        ctypes.c_void_p,   # userdata
    )
    lib._EVENT_CB = EVENT_CB

    lib.fdl_set_progress_cb.restype  = None
    lib.fdl_set_progress_cb.argtypes = [ctypes.c_void_p, PROGRESS_CB, ctypes.c_void_p]
    lib.fdl_set_event_cb.restype     = None
    lib.fdl_set_event_cb.argtypes    = [ctypes.c_void_p, EVENT_CB,    ctypes.c_void_p]

    lib.fdl_download_file.restype  = ctypes.c_int
    lib.fdl_download_file.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

    lib.fdl_fetch_listing.restype  = ctypes.c_void_p  # raw malloc'd buffer
    lib.fdl_fetch_listing.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

    lib.fdl_wait_all.restype  = None
    lib.fdl_wait_all.argtypes = [ctypes.c_void_p]
    lib.fdl_cancel.restype    = None
    lib.fdl_cancel.argtypes   = [ctypes.c_void_p]

    lib.fdl_total_bytes.restype  = ctypes.c_longlong
    lib.fdl_total_bytes.argtypes = [ctypes.c_void_p]
    lib.fdl_total_files.restype  = ctypes.c_longlong
    lib.fdl_total_files.argtypes = [ctypes.c_void_p]

    lib.fdl_config_load.restype  = ctypes.c_void_p
    lib.fdl_config_load.argtypes = [ctypes.c_char_p]
    lib.fdl_config_save.restype  = ctypes.c_int
    lib.fdl_config_save.argtypes = [ctypes.c_char_p, ctypes.c_char_p]

    lib.fdl_version.restype  = ctypes.c_char_p
    lib.fdl_version.argtypes = []

    return lib


DEFAULT_CONFIG = {
    "global_game_path": "",
    "servers": [],
    "download_paths": {},
}

DEFAULT_SERVER = {
    "id": "",
    "name": "",
    "fastdl_url": "",
    "game_path": "",
    "resource_types": [".bsp", ".nav", ".ain", ".mdl", ".vmt", ".vtf",
                       ".mp3", ".wav", ".txt"],
}


def load_config(path: str) -> dict:
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return dict(DEFAULT_CONFIG)


def save_config(path: str, cfg: dict) -> bool:
    try:
        os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(cfg, f, indent=4)
        return True
    except Exception:
        return False


class DownloadWorker(QThread):
    # Signals – all cross-thread UI updates go through these
    sig_log       = pyqtSignal(str, str)      # (message, color)
    sig_progress  = pyqtSignal(float, str, str)  # (0–1, speed_str, eta_str)
    sig_overall   = pyqtSignal(int, int)      # (done_files, total_files)
    sig_done      = pyqtSignal(int, int, int, float)  # (done, errors, skipped, elapsed)
    sig_status    = pyqtSignal(str)

    def __init__(self, lib, server: dict, files: list[str],
                 threads: int, force: bool, parent=None):
        super().__init__(parent)
        self._lib     = lib
        self._server  = server
        self._files   = files
        self._threads = threads
        self._force   = force
        self._handle  = None
        self._errors  = 0
        self._done    = 0
        self._total   = len(files)
        self._abort   = False

    def run(self):
        t0 = time.time()
        lib = self._lib
        srv = self._server

        output_dir = srv.get("game_path") or "downloads"
        self._handle = lib.fdl_create(
            srv["fastdl_url"].encode(),
            output_dir.encode(),
            self._threads,
        )
        if not self._handle:
            self.sig_log.emit("Failed to initialise downloader", "red")
            return

        rtypes = srv.get("resource_types", [])
        if rtypes:
            arr = (ctypes.c_char_p * len(rtypes))(*[r.encode() for r in rtypes])
            lib.fdl_set_resource_types(self._handle, arr, len(rtypes))

        @lib._PROGRESS_CB
        def on_progress(fname, progress, dl, total, speed, eta,
                        done_files, done_bytes, ud):
            speed_str = _fmt_speed(speed)
            eta_str   = _fmt_eta(eta)
            self.sig_progress.emit(float(progress), speed_str, eta_str)
            self.sig_overall.emit(done_files, self._total)

        @lib._EVENT_CB
        def on_event(event, fname, detail, size, ud):
            fn = fname.decode(errors="replace") if fname else ""
            if event == 0:   # file_start
                self.sig_log.emit(f"  ↓  {fn}", "#5bc0eb")
            elif event == 1: # file_done
                self._done += 1
                self.sig_log.emit(f"  ✓  {fn}  ({_fmt_bytes(size)})", "#90ee90")
            elif event == 2: # extract_start
                self.sig_log.emit(f"  ⚙  extracting {fn}", "#f0a500")
            elif event == 3: # extract_done
                self.sig_log.emit(f"  ✓  extracted {fn}", "#90ee90")
            elif event == 4: # error
                self._errors += 1
                err = detail.decode(errors="replace") if detail else "unknown"
                self.sig_log.emit(f"  ✗  {fn}  — {err}", "#ff6b6b")

        lib.fdl_set_progress_cb(self._handle, on_progress, None)
        lib.fdl_set_event_cb   (self._handle, on_event,    None)

        skipped = 0
        for path in self._files:
            if self._abort:
                break
            ok = lib.fdl_download_file(self._handle, path.encode())
            if not ok:
                skipped += 1

        lib.fdl_wait_all(self._handle)
        elapsed = time.time() - t0

        lib.fdl_destroy(self._handle)
        self._handle = None

        self.sig_done.emit(self._done, self._errors, skipped, elapsed)

    def abort(self):
        self._abort = True
        if self._handle and self._lib:
            self._lib.fdl_cancel(self._handle)


def _fmt_bytes(n: int) -> str:
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024:
            return f"{n:.2f} {unit}"
        n /= 1024
    return f"{n:.2f} TB"

def _fmt_speed(bps: float) -> str:
    return _fmt_bytes(int(bps)) + "/s"

def _fmt_eta(secs: float) -> str:
    if secs < 0:
        return "?"
    s = int(secs)
    h, rem = divmod(s, 3600)
    m, sec = divmod(rem, 60)
    if h:   return f"{h}h {m:02d}m"
    if m:   return f"{m}m {sec:02d}s"
    return  f"{sec}s"

class ServerDialog(QDialog):
    def __init__(self, server: dict = None, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Server Configuration")
        self.setMinimumWidth(500)
        self._data = dict(DEFAULT_SERVER)
        if server:
            self._data.update(server)
        self._build_ui()

    def _build_ui(self):
        layout = QVBoxLayout(self)

        form = QFormLayout()
        form.setSpacing(8)

        self._id   = QLineEdit(self._data.get("id", ""))
        self._name = QLineEdit(self._data.get("name", ""))
        self._url  = QLineEdit(self._data.get("fastdl_url", ""))
        self._path = QLineEdit(self._data.get("game_path", ""))

        self._id.setPlaceholderText("e.g. css_server1")
        self._name.setPlaceholderText("e.g. CS:Source – My Server")
        self._url.setPlaceholderText("http://fastdl.example.com/cstrike/")
        self._path.setPlaceholderText("/path/to/game  or  C:\\game")

        browse = QToolButton()
        browse.setText("…")
        browse.clicked.connect(self._browse_path)
        path_row = QHBoxLayout()
        path_row.addWidget(self._path)
        path_row.addWidget(browse)

        form.addRow("Server ID:", self._id)
        form.addRow("Display name:", self._name)
        form.addRow("FastDL URL:", self._url)
        form.addRow("Game path:", path_row)

        layout.addLayout(form)

        # resource types
        rtype_grp = QGroupBox("Resource types  (one extension per line)")
        rtype_lay  = QVBoxLayout(rtype_grp)
        self._rtypes = QTextEdit()
        self._rtypes.setPlainText("\n".join(self._data.get("resource_types", [])))
        self._rtypes.setFixedHeight(120)
        rtype_lay.addWidget(self._rtypes)
        layout.addWidget(rtype_grp)

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok |
            QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(self._accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

    def _browse_path(self):
        d = QFileDialog.getExistingDirectory(self, "Select game folder")
        if d:
            self._path.setText(d)

    def _accept(self):
        sid = self._id.text().strip()
        if not sid:
            QMessageBox.warning(self, "Validation", "Server ID is required.")
            return
        if not self._url.text().strip():
            QMessageBox.warning(self, "Validation", "FastDL URL is required.")
            return
        self._data["id"]           = sid
        self._data["name"]         = self._name.text().strip()
        self._data["fastdl_url"]   = self._url.text().strip()
        self._data["game_path"]    = self._path.text().strip()
        self._data["resource_types"] = [
            r.strip() for r in self._rtypes.toPlainText().splitlines()
            if r.strip()
        ]
        self.accept()

    def result_data(self) -> dict:
        return self._data


class PathsDialog(QDialog):
    def __init__(self, server_name: str, paths: list[str], parent=None):
        super().__init__(parent)
        self.setWindowTitle(f"Download paths – {server_name}")
        self.setMinimumSize(460, 320)
        self._paths = list(paths)
        self._build_ui()

    def _build_ui(self):
        layout = QVBoxLayout(self)

        info = QLabel(
            "One path per line.\n"
            "  Folder  →  maps/\n"
            "  Prefix  →  maps/zm_*\n"
            "  File    →  maps/de_dust2.bsp\n"
            "  (empty) →  sync all resource-type folders"
        )
        info.setStyleSheet("color:#aaa; font-size:11px;")
        layout.addWidget(info)

        self._edit = QTextEdit()
        self._edit.setPlainText("\n".join(self._paths))
        layout.addWidget(self._edit)

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok |
            QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

    def result_paths(self) -> list[str]:
        return [l.strip() for l in self._edit.toPlainText().splitlines() if l.strip()]


class AboutDialog(QDialog):
    def __init__(self, lib_version: str, parent=None):
        super().__init__(parent)
        self.setWindowTitle("About FastDL Tool")
        self.setFixedSize(440, 340)
        layout = QVBoxLayout(self)
        layout.setAlignment(Qt.AlignmentFlag.AlignCenter)

        title = QLabel("FastDL Tool")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title.setFont(QFont("Segoe UI", 22, QFont.Weight.Bold))
        layout.addWidget(title)

        subtitle = QLabel("Game File Synchroniser")
        subtitle.setAlignment(Qt.AlignmentFlag.AlignCenter)
        subtitle.setStyleSheet("color:#aaa; font-size:13px;")
        layout.addWidget(subtitle)

        layout.addSpacing(12)

        body = QLabel(
            f"<center>"
            f"<b>Backend:</b> {lib_version}<br><br>"
            f"<b>Frontend:</b> Python {sys.version.split()[0]} · PyQt6<br>"
            f"<b>Platform:</b> {platform.system()} {platform.machine()}<br><br>"
            f"Downloads and extracts FastDL game server content<br>"
            f"with multi-threaded C++ speed.<br><br>"
            f'<a href="https://github.com/SyntX34/FastDLExtractor" '
            f'style="color:#5bc0eb;">github.com/SyntX34/FastDLExtractor</a>'
            f"</center>"
        )
        body.setOpenExternalLinks(True)
        body.setAlignment(Qt.AlignmentFlag.AlignCenter)
        body.setWordWrap(True)
        layout.addWidget(body)

        layout.addSpacing(16)
        ok = QPushButton("Close")
        ok.setFixedWidth(100)
        ok.clicked.connect(self.accept)
        layout.addWidget(ok, alignment=Qt.AlignmentFlag.AlignCenter)

DARK_STYLE = """
QMainWindow, QDialog, QWidget {
    background: #1e1e2e;
    color: #cdd6f4;
    font-family: "Segoe UI", "Inter", sans-serif;
    font-size: 13px;
}
QTabWidget::pane { border: 1px solid #313244; border-radius: 4px; }
QTabBar::tab {
    background: #181825; color: #a6adc8;
    padding: 8px 20px; border-radius: 4px 4px 0 0;
    margin-right: 2px;
}
QTabBar::tab:selected { background: #1e1e2e; color: #cdd6f4; }
QTabBar::tab:hover    { background: #313244; }

QGroupBox {
    border: 1px solid #313244; border-radius: 6px;
    margin-top: 10px; padding: 10px;
    font-weight: bold;
}
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }

QLineEdit, QTextEdit, QSpinBox, QComboBox {
    background: #181825; border: 1px solid #45475a;
    border-radius: 4px; padding: 5px 8px; color: #cdd6f4;
}
QLineEdit:focus, QTextEdit:focus, QSpinBox:focus, QComboBox:focus {
    border-color: #89b4fa;
}

QPushButton {
    background: #313244; color: #cdd6f4;
    border: 1px solid #45475a; border-radius: 6px;
    padding: 7px 16px; min-width: 80px;
}
QPushButton:hover   { background: #45475a; }
QPushButton:pressed { background: #585b70; }
QPushButton:disabled { color: #585b70; }

QPushButton#btn_start {
    background: #40a02b; color: white; font-weight: bold;
    border: none; padding: 9px 28px;
}
QPushButton#btn_start:hover   { background: #4ec935; }
QPushButton#btn_start:disabled { background: #313244; color: #585b70; }

QPushButton#btn_stop {
    background: #d20f39; color: white; font-weight: bold;
    border: none; padding: 9px 28px;
}
QPushButton#btn_stop:hover { background: #e82148; }

QProgressBar {
    background: #181825; border: 1px solid #45475a;
    border-radius: 6px; height: 18px; text-align: center;
    color: white; font-weight: bold;
}
QProgressBar::chunk { background: #89b4fa; border-radius: 5px; }

QListWidget {
    background: #181825; border: 1px solid #313244;
    border-radius: 4px; outline: none;
}
QListWidget::item { padding: 6px 8px; }
QListWidget::item:selected { background: #313244; color: #89b4fa; }
QListWidget::item:hover     { background: #2a2a3e; }

QScrollBar:vertical {
    background: #181825; width: 8px; border-radius: 4px;
}
QScrollBar::handle:vertical { background: #45475a; border-radius: 4px; min-height: 20px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

QStatusBar { background: #181825; color: #a6adc8; }
QToolButton { background: #313244; border: 1px solid #45475a; border-radius: 4px; padding: 4px 8px; }
QToolButton:hover { background: #45475a; }

QLabel#stat_label { color: #a6adc8; font-size: 12px; }
QLabel#heading    { font-size: 15px; font-weight: bold; color: #89b4fa; }
"""


class MainWindow(QMainWindow):
    def __init__(self, lib=None):
        super().__init__()
        self._lib     = lib
        self._worker  : Optional[DownloadWorker] = None
        self._cfg_path = str(Path.home() / ".fastdltool" / "servers.json")
        self._cfg      = load_config(self._cfg_path)
        self._settings = QSettings("SyntX34", "FastDLTool")

        self.setWindowTitle("FastDL Tool")
        self.setMinimumSize(820, 620)
        self.resize(
            self._settings.value("window/width",  960, int),
            self._settings.value("window/height", 680, int),
        )

        self._build_ui()
        self._build_menu()
        self._refresh_server_list()
        self._update_lib_status()

    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        root = QVBoxLayout(central)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)

        header = QWidget()
        header.setFixedHeight(52)
        header.setStyleSheet("background:#181825; border-bottom:1px solid #313244;")
        hlay = QHBoxLayout(header)
        hlay.setContentsMargins(16, 0, 16, 0)

        title = QLabel("FastDL Tool")
        title.setFont(QFont("Segoe UI", 16, QFont.Weight.Bold))
        title.setStyleSheet("color:#89b4fa;")
        hlay.addWidget(title)

        self._lib_badge = QLabel()
        self._lib_badge.setStyleSheet(
            "background:#313244; border-radius:10px; padding:2px 10px; color:#a6adc8; font-size:11px;"
        )
        hlay.addWidget(self._lib_badge)
        hlay.addStretch()

        root.addWidget(header)

        self._tabs = QTabWidget()
        self._tabs.setDocumentMode(True)
        root.addWidget(self._tabs, 1)

        self._build_download_tab()
        self._build_servers_tab()
        self._build_settings_tab()

        self._statusbar = QStatusBar()
        self.setStatusBar(self._statusbar)
        self._statusbar.showMessage("Ready")

    def _build_menu(self):
        mb = self.menuBar()

        file_menu = mb.addMenu("&File")
        file_menu.addAction("Open config…", self._open_config)
        file_menu.addAction("Save config",  self._save_config)
        file_menu.addSeparator()
        file_menu.addAction("Exit", self.close)

        help_menu = mb.addMenu("&Help")
        help_menu.addAction("About…", self._show_about)

    def _build_download_tab(self):
        w = QWidget()
        self._tabs.addTab(w, "  ↓  Download  ")
        main = QHBoxLayout(w)
        main.setContentsMargins(12, 12, 12, 12)
        main.setSpacing(10)

        left = QVBoxLayout()
        left.setSpacing(10)

        srv_grp = QGroupBox("Server")
        srv_lay = QFormLayout(srv_grp)
        self._srv_combo = QComboBox()
        self._srv_combo.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        self._srv_combo.currentIndexChanged.connect(self._on_server_selected)
        srv_lay.addRow("Select:", self._srv_combo)

        self._srv_url_lbl  = QLabel("–")
        self._srv_path_lbl = QLabel("–")
        self._srv_url_lbl.setWordWrap(True)
        self._srv_path_lbl.setWordWrap(True)
        srv_lay.addRow("URL:",  self._srv_url_lbl)
        srv_lay.addRow("Path:", self._srv_path_lbl)
        left.addWidget(srv_grp)

        opt_grp = QGroupBox("Options")
        opt_lay = QFormLayout(opt_grp)
        self._threads_spin = QSpinBox()
        self._threads_spin.setRange(1, 16)
        self._threads_spin.setValue(int(self._settings.value("dl/threads", 4)))
        self._force_check = QCheckBox("Force re-download (ignore existing files)")
        opt_lay.addRow("Threads:", self._threads_spin)
        opt_lay.addRow("", self._force_check)
        left.addWidget(opt_grp)

        path_grp = QGroupBox("Specific path  (optional)")
        path_lay = QHBoxLayout(path_grp)
        self._specific_path = QLineEdit()
        self._specific_path.setPlaceholderText("maps/de_dust2.bsp  or  maps/  or  maps/zm_*")
        path_lay.addWidget(self._specific_path)
        left.addWidget(path_grp)

        btn_row = QHBoxLayout()
        self._btn_start = QPushButton("▶  Start Download")
        self._btn_start.setObjectName("btn_start")
        self._btn_stop  = QPushButton("■  Stop")
        self._btn_stop.setObjectName("btn_stop")
        self._btn_stop.setEnabled(False)
        self._btn_start.clicked.connect(self._start_download)
        self._btn_stop.clicked.connect(self._stop_download)
        btn_row.addWidget(self._btn_start)
        btn_row.addWidget(self._btn_stop)
        left.addLayout(btn_row)

        prog_grp = QGroupBox("Progress")
        prog_lay = QVBoxLayout(prog_grp)

        self._current_bar   = QProgressBar()
        self._current_bar.setFormat("File: %p%")
        self._current_bar.setValue(0)

        self._overall_bar   = QProgressBar()
        self._overall_bar.setFormat("Overall: %p%")
        self._overall_bar.setValue(0)

        stats_row = QHBoxLayout()
        self._speed_lbl = QLabel("Speed: –")
        self._eta_lbl   = QLabel("ETA: –")
        self._files_lbl = QLabel("Files: 0 / 0")
        for lbl in (self._speed_lbl, self._eta_lbl, self._files_lbl):
            lbl.setObjectName("stat_label")
            stats_row.addWidget(lbl)

        prog_lay.addWidget(QLabel("Current file:"))
        prog_lay.addWidget(self._current_bar)
        prog_lay.addWidget(QLabel("Overall:"))
        prog_lay.addWidget(self._overall_bar)
        prog_lay.addLayout(stats_row)
        left.addWidget(prog_grp)

        left.addStretch()

        # right panel – log
        log_grp  = QGroupBox("Log")
        log_lay  = QVBoxLayout(log_grp)
        self._log = QTextEdit()
        self._log.setReadOnly(True)
        self._log.setFont(QFont("Consolas", 11))
        self._log.setStyleSheet("background:#0d0d17;")
        log_clear = QPushButton("Clear")
        log_clear.setFixedWidth(70)
        log_clear.clicked.connect(self._log.clear)
        log_lay.addWidget(self._log, 1)
        log_lay.addWidget(log_clear, alignment=Qt.AlignmentFlag.AlignRight)

        main.addLayout(left, 38)
        main.addWidget(log_grp, 62)

    def _build_servers_tab(self):
        w = QWidget()
        self._tabs.addTab(w, "  ⚙  Servers  ")
        main = QHBoxLayout(w)
        main.setContentsMargins(12, 12, 12, 12)
        main.setSpacing(10)

        # list
        left = QVBoxLayout()
        self._server_list = QListWidget()
        self._server_list.currentRowChanged.connect(self._on_srv_list_selected)
        left.addWidget(QLabel("Configured servers:"))
        left.addWidget(self._server_list, 1)

        btn_row = QHBoxLayout()
        self._btn_add    = QPushButton("+ Add")
        self._btn_edit   = QPushButton("✎ Edit")
        self._btn_remove = QPushButton("✕ Remove")
        self._btn_paths  = QPushButton("☰ Paths")
        for b in (self._btn_add, self._btn_edit, self._btn_remove, self._btn_paths):
            btn_row.addWidget(b)
        self._btn_add.clicked.connect(self._add_server)
        self._btn_edit.clicked.connect(self._edit_server)
        self._btn_remove.clicked.connect(self._remove_server)
        self._btn_paths.clicked.connect(self._edit_paths)
        left.addLayout(btn_row)

        # detail panel
        right = QGroupBox("Server details")
        right_lay = QFormLayout(right)
        self._det_id    = QLabel("–")
        self._det_name  = QLabel("–")
        self._det_url   = QLabel("–")
        self._det_url.setWordWrap(True)
        self._det_path  = QLabel("–")
        self._det_types = QLabel("–")
        self._det_types.setWordWrap(True)
        self._det_paths = QLabel("–")
        self._det_paths.setWordWrap(True)
        right_lay.addRow("ID:",             self._det_id)
        right_lay.addRow("Name:",           self._det_name)
        right_lay.addRow("FastDL URL:",     self._det_url)
        right_lay.addRow("Game path:",      self._det_path)
        right_lay.addRow("Resource types:", self._det_types)
        right_lay.addRow("Download paths:", self._det_paths)

        save_btn = QPushButton("💾  Save all changes")
        save_btn.clicked.connect(self._save_config)

        right_wrap = QVBoxLayout()
        right_wrap.addWidget(right, 1)
        right_wrap.addWidget(save_btn)

        main.addLayout(left, 40)
        main.addLayout(right_wrap, 60)

    def _build_settings_tab(self):
        w = QWidget()
        self._tabs.addTab(w, "  ⚙  Settings  ")
        lay = QVBoxLayout(w)
        lay.setContentsMargins(24, 24, 24, 24)
        lay.setSpacing(16)

        cfg_grp = QGroupBox("Config file")
        cfg_lay = QHBoxLayout(cfg_grp)
        self._cfg_path_edit = QLineEdit(self._cfg_path)
        browse_cfg = QToolButton()
        browse_cfg.setText("…")
        browse_cfg.clicked.connect(self._browse_config_path)
        cfg_lay.addWidget(self._cfg_path_edit)
        cfg_lay.addWidget(browse_cfg)
        lay.addWidget(cfg_grp)

        lib_grp = QGroupBox("C++ backend library")
        lib_lay = QHBoxLayout(lib_grp)
        self._lib_path_edit = QLineEdit(self._settings.value("lib/path", ""))
        self._lib_path_edit.setPlaceholderText("Auto-detect  (leave blank)")
        browse_lib = QToolButton()
        browse_lib.setText("…")
        browse_lib.clicked.connect(self._browse_lib_path)
        reload_btn = QPushButton("Reload")
        reload_btn.clicked.connect(self._reload_lib)
        lib_lay.addWidget(self._lib_path_edit)
        lib_lay.addWidget(browse_lib)
        lib_lay.addWidget(reload_btn)
        lay.addWidget(lib_grp)

        game_grp = QGroupBox("Global game path  (default output directory)")
        game_lay = QHBoxLayout(game_grp)
        self._global_path_edit = QLineEdit(self._cfg.get("global_game_path", ""))
        browse_game = QToolButton()
        browse_game.setText("…")
        browse_game.clicked.connect(self._browse_global_path)
        game_lay.addWidget(self._global_path_edit)
        game_lay.addWidget(browse_game)
        lay.addWidget(game_grp)

        save_btn = QPushButton("Save settings")
        save_btn.clicked.connect(self._save_settings)
        lay.addWidget(save_btn)
        lay.addStretch()

    def _update_lib_status(self):
        if self._lib:
            ver = self._lib.fdl_version().decode(errors="replace")
            self._lib_badge.setText(f"✓ {ver}")
            self._lib_badge.setStyleSheet(
                "background:#1a3a1a; border-radius:10px; padding:2px 10px; color:#90ee90; font-size:11px;"
            )
        else:
            self._lib_badge.setText("⚠  Backend not loaded – download disabled")
            self._lib_badge.setStyleSheet(
                "background:#3a1a1a; border-radius:10px; padding:2px 10px; color:#ff6b6b; font-size:11px;"
            )
            self._btn_start.setEnabled(False)

    def _refresh_server_list(self):
        self._srv_combo.blockSignals(True)
        self._srv_combo.clear()
        self._server_list.clear()
        for srv in self._cfg.get("servers", []):
            name = srv.get("name") or srv.get("id", "unnamed")
            self._srv_combo.addItem(name, srv)
            self._server_list.addItem(name)
        self._srv_combo.blockSignals(False)
        self._on_server_selected(0)

    def _current_server(self) -> Optional[dict]:
        idx = self._srv_combo.currentIndex()
        if idx < 0:
            return None
        return self._srv_combo.itemData(idx)

    def _on_server_selected(self, idx):
        srv = self._current_server()
        if srv:
            self._srv_url_lbl.setText(srv.get("fastdl_url", "–"))
            self._srv_path_lbl.setText(srv.get("game_path", "–"))
        else:
            self._srv_url_lbl.setText("–")
            self._srv_path_lbl.setText("–")

    def _on_srv_list_selected(self, row):
        servers = self._cfg.get("servers", [])
        if 0 <= row < len(servers):
            srv = servers[row]
            self._det_id.setText(srv.get("id", ""))
            self._det_name.setText(srv.get("name", ""))
            self._det_url.setText(srv.get("fastdl_url", ""))
            self._det_path.setText(srv.get("game_path", ""))
            self._det_types.setText("  ".join(srv.get("resource_types", [])))
            paths = self._cfg.get("download_paths", {}).get(srv["id"], [])
            self._det_paths.setText("\n".join(paths) if paths else "(all)")


    def _start_download(self):
        if not self._lib:
            QMessageBox.warning(self, "Backend missing",
                "The C++ backend library is not loaded.\n"
                "Build the library first (see README) and place it "
                "next to this script.")
            return

        srv = self._current_server()
        if not srv:
            QMessageBox.warning(self, "No server", "Please add and select a server first.")
            return

        specific = self._specific_path.text().strip()
        if specific:
            file_list = [specific]
        else:
            file_list = self._cfg.get("download_paths", {}).get(srv["id"], [])

        if not file_list:
            QMessageBox.information(self, "Nothing to download",
                "No download paths configured for this server.\n"
                "Add paths in the Servers tab, or enter a specific path above.")
            return

        self._log.clear()
        self._current_bar.setValue(0)
        self._overall_bar.setValue(0)
        self._btn_start.setEnabled(False)
        self._btn_stop.setEnabled(True)
        self._statusbar.showMessage("Downloading…")

        self._worker = DownloadWorker(
            self._lib,
            srv,
            file_list,
            threads=self._threads_spin.value(),
            force=self._force_check.isChecked(),
        )
        self._worker.sig_log.connect(self._append_log)
        self._worker.sig_progress.connect(self._on_progress)
        self._worker.sig_overall.connect(self._on_overall)
        self._worker.sig_done.connect(self._on_done)
        self._worker.sig_status.connect(self._statusbar.showMessage)
        self._worker.start()

    def _stop_download(self):
        if self._worker:
            self._worker.abort()
            self._btn_stop.setEnabled(False)
            self._statusbar.showMessage("Stopping…")

    def _append_log(self, msg: str, color: str):
        cursor = self._log.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End)
        fmt = cursor.charFormat()
        fmt.setForeground(QColor(color))
        cursor.setCharFormat(fmt)
        cursor.insertText(msg + "\n")
        self._log.setTextCursor(cursor)
        self._log.ensureCursorVisible()

    def _on_progress(self, prog: float, speed: str, eta: str):
        self._current_bar.setValue(int(prog * 100))
        self._speed_lbl.setText(f"Speed: {speed}")
        self._eta_lbl.setText(f"ETA: {eta}")

    def _on_overall(self, done: int, total: int):
        pct = int(done * 100 / max(total, 1))
        self._overall_bar.setValue(pct)
        self._files_lbl.setText(f"Files: {done} / {total}")

    def _on_done(self, done: int, errors: int, skipped: int, elapsed: float):
        self._btn_start.setEnabled(True)
        self._btn_stop.setEnabled(False)
        self._overall_bar.setValue(100)
        self._current_bar.setValue(100)
        msg = (f"Done — {done} files downloaded, "
               f"{errors} errors, {skipped} skipped — "
               f"{_fmt_eta(elapsed)}")
        self._statusbar.showMessage(msg)
        self._append_log("", "#888")
        self._append_log(f"{'─'*60}", "#444")
        self._append_log(f"  ✓  {done} downloaded  ✗  {errors} errors  ⊘  {skipped} skipped", "#cdd6f4")
        self._append_log(f"  ⏱  Elapsed: {_fmt_eta(elapsed)}", "#cdd6f4")
        self._append_log(f"{'─'*60}", "#444")

    def _add_server(self):
        dlg = ServerDialog(parent=self)
        if dlg.exec() == QDialog.DialogCode.Accepted:
            srv = dlg.result_data()
            self._cfg.setdefault("servers", []).append(srv)
            self._refresh_server_list()
            self._save_config()

    def _edit_server(self):
        row = self._server_list.currentRow()
        servers = self._cfg.get("servers", [])
        if row < 0 or row >= len(servers):
            return
        dlg = ServerDialog(servers[row], parent=self)
        if dlg.exec() == QDialog.DialogCode.Accepted:
            servers[row] = dlg.result_data()
            self._refresh_server_list()
            self._server_list.setCurrentRow(row)
            self._save_config()

    def _remove_server(self):
        row = self._server_list.currentRow()
        servers = self._cfg.get("servers", [])
        if row < 0 or row >= len(servers):
            return
        name = servers[row].get("name", servers[row].get("id"))
        if QMessageBox.question(self, "Confirm", f"Remove server "{name}"?") \
                == QMessageBox.StandardButton.Yes:
            servers.pop(row)
            self._refresh_server_list()
            self._save_config()

    def _edit_paths(self):
        row = self._server_list.currentRow()
        servers = self._cfg.get("servers", [])
        if row < 0 or row >= len(servers):
            QMessageBox.information(self, "Select a server", "Please select a server first.")
            return
        srv = servers[row]
        paths = self._cfg.get("download_paths", {}).get(srv["id"], [])
        dlg   = PathsDialog(srv.get("name", srv["id"]), paths, parent=self)
        if dlg.exec() == QDialog.DialogCode.Accepted:
            self._cfg.setdefault("download_paths", {})[srv["id"]] = dlg.result_paths()
            self._on_srv_list_selected(row)
            self._save_config()

    def _save_config(self):
        path = self._cfg_path_edit.text().strip() or self._cfg_path
        self._cfg_path = path
        ok = save_config(path, self._cfg)
        self._statusbar.showMessage("Config saved." if ok else "Failed to save config!")

    def _open_config(self):
        path, _ = QFileDialog.getOpenFileName(
            self, "Open config", str(Path.home()), "JSON files (*.json);;All files (*)")
        if path:
            self._cfg_path = path
            self._cfg_path_edit.setText(path)
            self._cfg = load_config(path)
            self._refresh_server_list()

    def _browse_config_path(self):
        path, _ = QFileDialog.getSaveFileName(
            self, "Config file location", self._cfg_path, "JSON files (*.json)")
        if path:
            self._cfg_path_edit.setText(path)

    def _browse_lib_path(self):
        path, _ = QFileDialog.getOpenFileName(
            self, "C++ backend library",
            str(Path(__file__).parent),
            "Shared libraries (*.so *.dll *.dylib);;All files (*)")
        if path:
            self._lib_path_edit.setText(path)

    def _browse_global_path(self):
        d = QFileDialog.getExistingDirectory(self, "Global game path")
        if d:
            self._global_path_edit.setText(d)

    def _reload_lib(self):
        path = self._lib_path_edit.text().strip() or _find_lib()
        if not path:
            QMessageBox.warning(self, "Library not found",
                "Cannot locate the C++ backend library.\n"
                "Build it first (see README / build_lib.sh).")
            return
        try:
            self._lib = _load_lib(path)
            self._update_lib_status()
            self._btn_start.setEnabled(True)
            self._statusbar.showMessage("Library loaded: " + path)
        except Exception as e:
            QMessageBox.critical(self, "Load error", str(e))

    def _save_settings(self):
        self._settings.setValue("lib/path", self._lib_path_edit.text().strip())
        self._settings.setValue("dl/threads", self._threads_spin.value())
        self._cfg["global_game_path"] = self._global_path_edit.text().strip()
        self._save_config()
        self._statusbar.showMessage("Settings saved.")

    def _show_about(self):
        ver = self._lib.fdl_version().decode() if self._lib else "backend not loaded"
        AboutDialog(ver, self).exec()

    def closeEvent(self, event):
        if self._worker and self._worker.isRunning():
            self._worker.abort()
            self._worker.wait(3000)
        self._settings.setValue("window/width",  self.width())
        self._settings.setValue("window/height", self.height())
        event.accept()

def main():
    app = QApplication(sys.argv)
    app.setApplicationName("FastDL Tool")
    app.setOrganizationName("SyntX34")
    app.setStyleSheet(DARK_STYLE)

    # Try to load the C++ backend
    lib = None
    lib_path = _find_lib()

    # Also check settings
    settings = QSettings("SyntX34", "FastDLTool")
    override  = settings.value("lib/path", "")
    if override and Path(override).exists():
        lib_path = override

    if lib_path:
        try:
            lib = _load_lib(lib_path)
        except Exception as e:
            print(f"[WARNING] Could not load backend: {e}")
    else:
        print("[WARNING] C++ backend not found – UI only mode")

    win = MainWindow(lib)
    win.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()