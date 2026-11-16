// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 dekubi Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QIcon>
#include <fmt/ranges.h>
#include "common/scm_rev.h"
#include "ui_aboutdialog.h"
#include "yuzu/about_dialog.h"
#include "yuzu/dekubi/dekubi_ui.h"

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent), ui{std::make_unique<Ui::AboutDialog>()} {
    static const std::string build_id = std::string{Common::g_build_id};
    static const std::string dekubi_build =
        fmt::format("{} | {} | {}", std::string{Common::g_build_name},
                    std::string{Common::g_build_version}, std::string{Common::g_compiler_id});

    const auto override_build =
        fmt::format(fmt::runtime(std::string(Common::g_title_bar_format_idle)), build_id);
    const auto dekubi_build_version = override_build.empty() ? dekubi_build : override_build;

    ui->setupUi(this);
    // Try and request the icon from Qt theme (Linux?)
    const QIcon dekubi_logo = QIcon::fromTheme(QStringLiteral("io.github.zortos293.lichii"),
                                           QIcon(QStringLiteral(":/img/lichii.ico")));
    if (!dekubi_logo.isNull()) {
        ui->labelLogo->setPixmap(dekubi_logo.pixmap(200));
    }
    ui->labelBuildInfo->setText(
        ui->labelBuildInfo->text().arg(QString::fromStdString(dekubi_build_version),
                                       QString::fromUtf8(Common::g_build_date).left(10)));
    if (LichiiUi::Enabled()) {
        setWindowTitle(tr("About Dekubi"));
        ui->labelYuzu->setText(QStringLiteral("<span style='font-size:30px'>Dekubi</span>"));
    }
}

AboutDialog::~AboutDialog() = default;



