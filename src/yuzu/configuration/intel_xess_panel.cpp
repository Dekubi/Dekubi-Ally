// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/configuration/intel_xess_panel.h"

#include <filesystem>
#include <system_error>

#include <QApplication>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "common/fs/fs_paths.h"
#include "common/fs/path_util.h"

namespace {

QString Tr(const char* text) {
    return QCoreApplication::translate("IntelXessPanel", text);
}

QString PathText(const std::filesystem::path& path) {
    return QString::fromStdWString(path.wstring());
}

std::filesystem::path XessDirectory() {
    return Common::FS::GetEdenPath(Common::FS::EdenPath::XessDir);
}

bool HasXessLibraries(bool& has_sr, bool& has_fg) {
    const std::filesystem::path user_dir = XessDirectory();
    const std::filesystem::path exe_dir = Common::FS::GetExeDirectory();
    const std::filesystem::path exe_xess = exe_dir / "xess";

    std::error_code ec;
    has_sr = std::filesystem::exists(user_dir / "libxess.dll", ec) ||
             std::filesystem::exists(exe_xess / "libxess.dll", ec) ||
             std::filesystem::exists(exe_dir / "libxess.dll", ec);

    has_fg = std::filesystem::exists(user_dir / "libxess_fg.dll", ec) ||
             std::filesystem::exists(exe_xess / "libxess_fg.dll", ec) ||
             std::filesystem::exists(exe_dir / "libxess_fg.dll", ec);

    return has_sr;
}

} // Anonymous namespace

IntelXessPanel::IntelXessPanel(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 0, 0, 8);
    layout->setSpacing(6);

    QHBoxLayout* status_row = new QHBoxLayout;
    status_row->setSpacing(8);
    icon_label = new QLabel(this);
    icon_label->setTextFormat(Qt::RichText);
    icon_label->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    icon_label->setFixedWidth(16);
    status_label = new QLabel(this);
    status_label->setTextFormat(Qt::RichText);
    status_label->setWordWrap(true);
    status_row->addWidget(icon_label);
    status_row->addWidget(status_label, 1);

    QHBoxLayout* button_row = new QHBoxLayout;
    button_row->setContentsMargins(24, 0, 0, 0);
    folder_button = new QPushButton(Tr("Open Folder"), this);
    QLabel* about = new QLabel(
        QStringLiteral("<a href=\"https://github.com/intel/xess/releases\">%1</a>")
            .arg(Tr("Download Intel XeSS SDK")),
        this);
    about->setOpenExternalLinks(true);
    about->setToolTip(Tr("Official Intel XeSS releases on GitHub. Download libxess.dll and "
                         "libxess_fg.dll and place them into the user/xess folder."));
    button_row->addWidget(folder_button);
    button_row->addStretch(1);
    button_row->addWidget(about);

    layout->addLayout(status_row);
    layout->addLayout(button_row);

    connect(folder_button, &QPushButton::clicked, this, [] {
        const std::filesystem::path folder = XessDirectory();
        std::error_code ec;
        std::filesystem::create_directories(folder, ec);
        QDesktopServices::openUrl(QUrl::fromLocalFile(PathText(folder)));
    });

    Refresh();
}

void IntelXessPanel::showEvent(QShowEvent* event) {
    Refresh();
    QWidget::showEvent(event);
}

void IntelXessPanel::Refresh() {
    bool has_sr = false;
    bool has_fg = false;
    const bool ready = HasXessLibraries(has_sr, has_fg);

    icon_label->setText(ready ? QStringLiteral("<span style=\"color:#2ea043\">&#10004;</span>")
                              : QStringLiteral("<span style=\"color:#d29922\">&#9888;</span>"));

    if (ready) {
        if (has_fg) {
            status_label->setText(Tr("<b>Ready.</b> Found Intel XeSS runtime (libxess.dll and libxess_fg.dll). "
                                     "AI Super Sampling and XeSS Frame Generation are available."));
        } else {
            status_label->setText(Tr("<b>Ready.</b> Found Intel XeSS runtime (libxess.dll). "
                                     "AI Super Sampling is active. (Drop libxess_fg.dll into user/xess for XeSS-FG)."));
        }
    } else {
        status_label->setText(Tr("<b>Intel XeSS files needed:</b> Place <code>libxess.dll</code> and "
                                 "<code>libxess_fg.dll</code> in <code>user/xess/</code>. "
                                 "Download from Intel's official GitHub releases."));
    }
    status_label->setToolTip(Tr("Folder: %1").arg(PathText(XessDirectory())));
}
