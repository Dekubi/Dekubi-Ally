// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/dekubi/dekubi_ui.h"

#include <algorithm>
#include <array>
#include <map>
#include <string_view>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QBuffer>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QImageReader>
#include <QLocale>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QPalette>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QProxyStyle>
#include <QPushButton>
#include <QUrl>
#include <QScrollArea>
#include <QStyleFactory>

#include "common/fs/fs_paths.h"
#include "common/fs/path_util.h"
#include "frontend_common/profile_import.h"
#include "yuzu/dekubi/saves_window.h"
#include "yuzu/user_data_migration.h"
#include "common/logging.h"
#include "common/scm_rev.h"
#include "common/settings.h"
#include "core/core.h"
#include "qt_common/config/shared_translation.h"
#include "qt_common/qt_common.h"
#include "ui_main.h"
#ifdef HAS_RESHADE
#include "video_core/post_processing/fx_preset.h"
#endif

namespace LichiiUi {
namespace {

// Graphite
const QColor Background{0x11, 0x11, 0x13};
const QColor Raised{0x1b, 0x1b, 0x1f};
const QColor Field{0x18, 0x18, 0x1b};
const QColor Button{0x1f, 0x1f, 0x24};
const QColor Text{0xec, 0xec, 0xf1};
const QColor TextSecondary{0x9a, 0x9a, 0xa8};
const QColor TextDisabled{0x55, 0x55, 0x5f};
const QColor IconIdle{0x6b, 0x6b, 0x78};
const QColor Accent{0x22, 0xc5, 0x5e, 0x78};
const QColor OnAccent{0xec, 0xfd, 0xf5};

QPalette GraphitePalette() {
    QPalette p;
    p.setColor(QPalette::Window, Background);
    p.setColor(QPalette::WindowText, Text);
    p.setColor(QPalette::Base, Field);
    p.setColor(QPalette::AlternateBase, Raised);
    p.setColor(QPalette::Text, Text);
    p.setColor(QPalette::Button, Button);
    p.setColor(QPalette::ButtonText, Text);
    p.setColor(QPalette::BrightText, Text);
    p.setColor(QPalette::ToolTipBase, Button);
    p.setColor(QPalette::ToolTipText, Text);
    p.setColor(QPalette::PlaceholderText, IconIdle);
    p.setColor(QPalette::Highlight, Accent);
    p.setColor(QPalette::HighlightedText, OnAccent);
    p.setColor(QPalette::Link, Accent);
    p.setColor(QPalette::LinkVisited, Accent);
    p.setColor(QPalette::Light, QColor(0x2e, 0x2e, 0x35));
    p.setColor(QPalette::Midlight, QColor(0x26, 0x26, 0x2c));
    p.setColor(QPalette::Mid, QColor(0x22, 0x22, 0x27));
    p.setColor(QPalette::Dark, QColor(0x0c, 0x0c, 0x0e));
    p.setColor(QPalette::Shadow, QColor(0x05, 0x05, 0x06));
    for (const auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        p.setColor(QPalette::Disabled, role, TextDisabled);
    }
    return p;
}

/// Fusion without the always-on mnemonic underlines.
class GraphiteStyle final : public QProxyStyle {
public:
    GraphiteStyle() : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion"))) {}
    int styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget,
                  QStyleHintReturn* data) const override {
        if (hint == SH_UnderlineShortcut) {
            return 0;
        }
        return QProxyStyle::styleHint(hint, option, widget, data);
    }
};

QPixmap DrawFallbackGlyph(const QString& name, const QColor& color, int px) {
    const qreal ratio = qApp ? qApp->devicePixelRatio() : 1.0;
    QPixmap pixmap(QSize(px, px) * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, std::max(1.8, px * 0.095), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const qreal s = px / 24.0;
    p.scale(s, s);
    if (name == QStringLiteral("library")) {
        p.drawRoundedRect(QRectF(3, 3, 7, 7), 1.5, 1.5);
        p.drawRoundedRect(QRectF(14, 3, 7, 7), 1.5, 1.5);
        p.drawRoundedRect(QRectF(14, 14, 7, 7), 1.5, 1.5);
        p.drawRoundedRect(QRectF(3, 14, 7, 7), 1.5, 1.5);
    } else if (name == QStringLiteral("settings")) {
        p.drawEllipse(QPointF(12, 12), 3.2, 3.2);
        p.drawEllipse(QPointF(12, 12), 7.2, 7.2);
        for (int i = 0; i < 8; ++i) { p.save(); p.translate(12, 12); p.rotate(i * 45.0); p.drawLine(QPointF(0, -7.2), QPointF(0, -9.8)); p.restore(); }
    } else if (name == QStringLiteral("help")) {
        p.drawEllipse(QPointF(12, 12), 9.5, 9.5);
        QPainterPath path; path.moveTo(9.5, 9.2); path.cubicTo(9.5, 6.8, 14.5, 6.8, 14.5, 9.5); path.cubicTo(14.5, 11.5, 12.0, 11.8, 12.0, 13.8); p.drawPath(path);
        p.setBrush(color); p.drawEllipse(QPointF(12, 17.2), 0.8, 0.8);
    } else if (name == QStringLiteral("menu")) {
        p.drawLine(QPointF(4, 6), QPointF(20, 6)); p.drawLine(QPointF(4, 12), QPointF(20, 12)); p.drawLine(QPointF(4, 18), QPointF(20, 18));
    } else if (name == QStringLiteral("pause")) {
        p.setBrush(color); p.drawRoundedRect(QRectF(6, 4, 4, 16), 1.0, 1.0); p.drawRoundedRect(QRectF(14, 4, 4, 16), 1.0, 1.0);
    } else if (name == QStringLiteral("play")) {
        p.setBrush(color); QPolygonF poly; poly << QPointF(6, 4) << QPointF(20, 12) << QPointF(6, 20); p.drawPolygon(poly);
    } else if (name == QStringLiteral("restart")) {
        p.drawArc(QRectF(4, 4, 16, 16), 30 * 16, 285 * 16); p.drawLine(QPointF(4, 4), QPointF(4, 9)); p.drawLine(QPointF(4, 9), QPointF(9, 9));
    } else if (name == QStringLiteral("stop")) {
        p.setBrush(color); p.drawRoundedRect(QRectF(5, 5, 14, 14), 2.0, 2.0);
    } else if (name == QStringLiteral("fullscreen")) {
        p.drawLine(QPointF(3, 8), QPointF(3, 3)); p.drawLine(QPointF(3, 3), QPointF(8, 3));
        p.drawLine(QPointF(16, 3), QPointF(21, 3)); p.drawLine(QPointF(21, 3), QPointF(21, 8));
        p.drawLine(QPointF(21, 16), QPointF(21, 21)); p.drawLine(QPointF(21, 21), QPointF(16, 21));
        p.drawLine(QPointF(8, 21), QPointF(3, 21)); p.drawLine(QPointF(3, 21), QPointF(3, 16));
    } else if (name == QStringLiteral("fullscreen-exit")) {
        p.drawLine(QPointF(3, 8), QPointF(8, 8)); p.drawLine(QPointF(8, 8), QPointF(8, 3));
        p.drawLine(QPointF(21, 8), QPointF(16, 8)); p.drawLine(QPointF(16, 8), QPointF(16, 3));
        p.drawLine(QPointF(21, 16), QPointF(16, 16)); p.drawLine(QPointF(16, 16), QPointF(16, 21));
        p.drawLine(QPointF(3, 16), QPointF(8, 16)); p.drawLine(QPointF(8, 16), QPointF(8, 21));
    } else if (name == QStringLiteral("list")) {
        p.drawLine(QPointF(8, 6), QPointF(20, 6)); p.drawLine(QPointF(8, 12), QPointF(20, 12)); p.drawLine(QPointF(8, 18), QPointF(20, 18));
        p.drawEllipse(QPointF(4, 6), 1.0, 1.0); p.drawEllipse(QPointF(4, 12), 1.0, 1.0); p.drawEllipse(QPointF(4, 18), 1.0, 1.0);
    } else if (name == QStringLiteral("plus")) {
        p.drawLine(QPointF(12, 5), QPointF(12, 19)); p.drawLine(QPointF(5, 12), QPointF(19, 12));
    } else if (name == QStringLiteral("community")) {
        p.drawRoundedRect(QRectF(3, 4, 18, 13), 4.0, 4.0); p.drawLine(QPointF(7, 17), QPointF(5, 21)); p.drawLine(QPointF(5, 21), QPointF(11, 17));
    } else if (name == QStringLiteral("heart")) {
        QPainterPath heart; heart.moveTo(12, 20); heart.cubicTo(4, 14, 3, 8, 7.5, 5.5); heart.cubicTo(10, 4.2, 12, 6.5, 12, 7.5); heart.cubicTo(12, 6.5, 14, 4.2, 16.5, 5.5); heart.cubicTo(21, 8, 20, 14, 12, 20); p.drawPath(heart);
    } else {
        p.drawEllipse(QPointF(12, 12), 8, 8);
    }
    return pixmap;
}
QPixmap RenderSvg(QByteArray svg, const QColor& color, int px, const QString& fallback_name = {}) {
    if (!svg.isEmpty()) {
        svg.replace("currentColor", color.name().toLatin1());
        QBuffer buffer(&svg); buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer, "svg");
        const qreal ratio = qApp->devicePixelRatio();
        reader.setScaledSize(QSize(px, px) * ratio);
        const QImage img = reader.read();
        if (!img.isNull()) { QPixmap pixmap = QPixmap::fromImage(img); pixmap.setDevicePixelRatio(ratio); return pixmap; }
    }
    return DrawFallbackGlyph(fallback_name, color, px);
}

// The player's own icon (icons/<name>.png, a glyph on transparency): its shape in `color`.
QPixmap RenderPng(const QImage& source, const QColor& color, int px) {
    const qreal ratio = qApp->devicePixelRatio();
    const int side = qRound(px * ratio);
    QImage img = source.convertToFormat(QImage::Format_ARGB32_Premultiplied)
                     .scaled(side, side, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPainter p(&img);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(img.rect(), color);
    p.end();
    QPixmap pixmap = QPixmap::fromImage(img);
    pixmap.setDevicePixelRatio(ratio);
    return pixmap;
}

QImage CustomIcon(const QString& name) {
    return QImage(QStringLiteral(":/icons/%1.png").arg(name));
}

void Rename(QAction* action, const QString& text) {
    if (action) {
        action->setText(text);
    }
}

} // namespace

bool Enabled() {
    static const bool enabled = [] {
        if (qEnvironmentVariableIntValue("Dekubi_CLASSIC_UI") != 0) {
            return false;
        }
        return std::string_view{Common::g_build_name} == "Dekubi";
    }();
    return enabled;
}

bool ApplyTheme() {
    if (!Enabled()) {
        return false;
    }
    static bool once = false;
    if (!once) {
        once = true;
        QFontDatabase::addApplicationFont(QStringLiteral(":/fritchii/fonts/Inter-Regular.ttf"));
        QFontDatabase::addApplicationFont(QStringLiteral(":/fritchii/fonts/Inter-Medium.ttf"));
        QApplication::setStyle(new GraphiteStyle);
    }
    QFont font(QStringLiteral("Inter"));
    font.setPixelSize(14);
    QApplication::setFont(font);
    QApplication::setPalette(GraphitePalette());
    // Folder and refresh icons come from the dark icon set.
    QIcon::setThemeName(QStringLiteral("colorful_dark"));

    QFile qss(QFile::exists(QStringLiteral(":/dekubi/style.qss")) ? QStringLiteral(":/dekubi/style.qss") : QStringLiteral(":/fritchii/style.qss"));
    if (qss.open(QFile::ReadOnly | QFile::Text)) {
        qApp->setStyleSheet(QString::fromUtf8(qss.readAll()));
    }
    return true;
}

bool HidesSetting(const std::string& label) {
    if (!Enabled()) {
        return false;
    }
    // Only hide internal developer dump settings
    return label == "frame_gen_dump_flow";
}

QIcon Icon(const QString& name, int px) {
    if (const QImage png = CustomIcon(name); !png.isNull()) {
        QIcon icon;
        const QPixmap on = RenderPng(png, Accent, px);
        icon.addPixmap(RenderPng(png, IconIdle, px), QIcon::Normal, QIcon::Off);
        icon.addPixmap(RenderPng(png, Text, px), QIcon::Active, QIcon::Off);
        icon.addPixmap(on, QIcon::Normal, QIcon::On);
        icon.addPixmap(on, QIcon::Active, QIcon::On);
        return icon;
    }
    QFile file(QStringLiteral(":/icons/%1.svg").arg(name));
    if (!file.open(QFile::ReadOnly)) {
        return {};
    }
    const QByteArray svg = file.readAll();
    QIcon icon;
    const QPixmap idle = RenderSvg(svg, IconIdle, px);
    const QPixmap hover = RenderSvg(svg, Text, px);
    const QPixmap on = RenderSvg(svg, Accent, px);
    icon.addPixmap(idle, QIcon::Normal, QIcon::Off);
    icon.addPixmap(hover, QIcon::Active, QIcon::Off);
    icon.addPixmap(on, QIcon::Normal, QIcon::On);
    icon.addPixmap(on, QIcon::Active, QIcon::On);
    return icon;
}

QIcon Glyph(const QString& name, unsigned rgb, int px) {
    if (const QImage png = CustomIcon(name); !png.isNull()) {
        return QIcon(RenderPng(png, QColor::fromRgb(rgb), px));
    }
    QFile file(QStringLiteral(":/icons/%1.svg").arg(name));
    if (!file.open(QFile::ReadOnly)) {
        return {};
    }
    return QIcon(RenderSvg(file.readAll(), QColor::fromRgb(rgb), px));
}

void RestructureMenus(Ui::MainWindow& ui, QMainWindow& window, const MenuHooks& hooks) {
    const auto tr = [](const char* text) { return QObject::tr(text); };

    // File: open, install, folders, settings, exit.
    ui.menuOpen_Eden_Folders->removeAction(ui.action_Log_Folder);
    QMenu* file = ui.menu_File;
    file->clear();
    file->addAction(ui.action_Load_File);
    file->addAction(ui.action_Load_Folder);
    file->addMenu(ui.menu_recent_files);
    file->addSeparator();
    file->addAction(ui.action_Install_File_NAND);
    file->addSeparator();
    file->addMenu(ui.menuOpen_Eden_Folders);
    file->addSeparator();
    file->addAction(ui.action_Configure);
    file->addSeparator();
    file->addAction(ui.action_Exit);

    // Game (was Emulation): pause, stop, restart, shaders, game settings.
    ui.menu_Emulation->setTitle(tr("&Game"));
    ui.menu_Emulation->removeAction(ui.action_Configure);

    // View: window, bars, game list, window size, effects, debugging.
    QMenu* view = ui.menu_View;
    view->clear();
    view->addAction(ui.action_Fullscreen);
    view->addAction(ui.action_Single_Window_Mode);
    view->addSeparator();
    view->addAction(ui.action_Show_Performance_Overlay);
    view->addAction(ui.action_Show_Status_Bar);
    view->addAction(ui.action_Show_Filter_Bar);
    view->addSeparator();
    ui.menu_Game_List_Mode->addSeparator();
    ui.menu_Game_List_Mode->addMenu(ui.menuGame_Icon_Size);
    ui.menu_Game_List_Mode->addAction(ui.action_Show_Game_Name);
    view->addMenu(ui.menu_Game_List_Mode);
    view->addMenu(ui.menu_Reset_Window_Size);
    view->addSeparator();
    view->addAction(ui.action_Post_Processing_Shaders);
    view->addMenu(ui.menu_View_Debugging);

    // Tools: setup tools first; Amiibo; rarely used things under Advanced.
    QMenu* tools = ui.menu_Tools;
    tools->clear();
    tools->addAction(ui.action_Install_Keys);
    tools->addMenu(ui.menuInstall_Firmware);
    tools->addAction(ui.action_Verify_installed_contents);
    tools->addAction(ui.action_Data_Manager);
    tools->addSeparator();
    if (hooks.switches) {
        auto* switches_action = new QAction(tr("S&witches..."), &window);
        QObject::connect(switches_action, &QAction::triggered, &window, hooks.switches);
        tools->addAction(switches_action);
        tools->addSeparator();
    }
    tools->addAction(ui.action_Capture_Screenshot);
    tools->addMenu(ui.menu_Create_Shortcuts);
    tools->addSeparator();
    QMenu* amiibo = ui.menu_cabinet_applet;
    QAction* first_cabinet = amiibo->actions().value(0);
    amiibo->insertAction(first_cabinet, ui.action_Load_Amiibo);
    amiibo->insertSeparator(first_cabinet);
    tools->addMenu(amiibo);
    QMenu* advanced = new QMenu(tr("Ad&vanced"), tools);
    advanced->addMenu(ui.menu_Applets);
    advanced->addMenu(ui.menuTAS);
    advanced->addMenu(ui.menu_Multiplayer);
    advanced->addSeparator();
    advanced->addAction(ui.action_Enable_Overlay_Applet);
    tools->addMenu(advanced);
    ui.menubar->removeAction(ui.menu_Multiplayer->menuAction());

    // Help: the log first; Eden's mods page leaves the menu.
    auto* export_log = new QAction(tr("&Export Log..."), &window);
    QObject::connect(export_log, &QAction::triggered, &window, hooks.export_log);
    ui.menu_Help->removeAction(ui.action_Open_Mods_Page);
    QAction* first_help = ui.menu_Help->actions().value(0);
    auto* whats_new = new QAction(tr("What's &New"), &window);
    QObject::connect(whats_new, &QAction::triggered, &window, hooks.whats_new);
    auto* delete_cache = new QAction(tr("&Delete Shader Cache..."), &window);
    QObject::connect(delete_cache, &QAction::triggered, &window, hooks.delete_shader_cache);
    ui.menu_Help->insertAction(first_help, export_log);
    ui.menu_Help->insertAction(first_help, ui.action_Log_Folder);
    ui.menu_Help->insertAction(first_help, delete_cache);
    ui.menu_Help->insertSeparator(first_help);
    ui.menu_Help->insertAction(first_help, whats_new);
    auto* discord = new QAction(Icon(QStringLiteral("community"), 16), tr("Dekubi &Discord"), &window);
    QObject::connect(discord, &QAction::triggered, &window, [] { OpenDiscord(); });
    auto* donate = new QAction(Icon(QStringLiteral("heart"), 16), tr("D&onate"), &window);
    QObject::connect(donate, &QAction::triggered, &window, [] { OpenDonate(); });
    ui.menu_Help->insertAction(first_help, discord);
    ui.menu_Help->insertAction(first_help, donate);
    // v5.8.1: copy a profile again after the first launch's automatic import (a player whose
    // newest saves were in Dekubi 150+'s folder got the older eden copy).
    auto* copy_data = new QAction(tr("&Copy Data from Eden or Dekubi..."), &window);
    QObject::connect(copy_data, &QAction::triggered, &window, [&window] { CopyDataFromOldFolder(window); });
    ui.menu_Help->insertAction(first_help, copy_data);
    // v5.8.3: export, import and copy saves between Dekubi, Eden and Dekubi.
    auto* saves = new QAction(tr("Sa&ves..."), &window);
    QObject::connect(saves, &QAction::triggered, &window, [&window] {
        ShowSavesWindow(&window,
                        [] { return QtCommon::system && QtCommon::system->IsPoweredOn(); });
    });
    ui.menu_Help->insertAction(copy_data, saves);

    // Plain names.
    ui.menu_recent_files->setTitle(tr("Open &Recent"));
    ui.menuOpen_Eden_Folders->setTitle(tr("Open &Folder"));
    ui.menu_Game_List_Mode->setTitle(tr("Game &List"));
    ui.menuGame_Icon_Size->setTitle(tr("&Icon Size"));
    ui.menu_Reset_Window_Size->setTitle(tr("&Window Size"));
    ui.menu_Create_Shortcuts->setTitle(tr("Create &Shortcut"));
    Rename(ui.action_Load_File, tr("&Open Game..."));
    Rename(ui.action_Load_Folder, tr("Open &Extracted Game..."));
    Rename(ui.action_Install_File_NAND, tr("&Install Update or DLC..."));
    Rename(ui.action_Configure, tr("&Settings..."));
    Rename(ui.action_Configure_Current_Game, tr("Game &Settings..."));
    Rename(ui.action_Root_Data_Folder, tr("&Data"));
    Rename(ui.action_NAND_Folder, tr("&NAND"));
    Rename(ui.action_SDMC_Folder, tr("&SD Card"));
    Rename(ui.action_Mod_Folder, tr("&Mods"));
    Rename(ui.action_Log_Folder, tr("Open &Log Folder"));
    Rename(ui.action_Reset_Window_Size_720, tr("&720p"));
    Rename(ui.action_Reset_Window_Size_900, tr("&900p"));
    Rename(ui.action_Reset_Window_Size_1080, tr("&1080p"));
    Rename(ui.action_Tree_View, tr("&List"));
    Rename(ui.action_Grid_View, tr("&Grid"));
    Rename(ui.action_Carousel_View, tr("&Carousel"));
    Rename(ui.action_Show_Game_Name, tr("Show Game &Names"));
    Rename(ui.action_Show_Performance_Overlay, tr("&Performance Overlay"));
    Rename(ui.action_Show_Status_Bar, tr("&Status Bar"));
    Rename(ui.action_Show_Filter_Bar, tr("Search &Bar"));
    Rename(ui.action_Single_Window_Mode, tr("Single &Window"));
    Rename(ui.action_Enable_Overlay_Applet, tr("&Overlay Display Applet"));
    Rename(ui.action_Data_Manager, tr("&Data Manager..."));
    Rename(ui.action_Capture_Screenshot, tr("Take &Screenshot"));
    Rename(ui.action_Load_Amiibo, tr("&Load or Remove Amiibo..."));
    Rename(ui.action_Firmware_From_Folder, tr("From &Folder..."));
    Rename(ui.action_Firmware_From_ZIP, tr("From &ZIP..."));
    Rename(ui.action_About, tr("&About Dekubi"));
    Rename(ui.action_Eden_Dependencies, tr("&Open-Source Components"));
}

QPushButton* MakeGraphicsButton(QWidget* parent, GraphicsHooks hooks) {
    auto* button = new QPushButton(QObject::tr("GRAPHICS"), parent);
    button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    button->setFocusPolicy(Qt::NoFocus);

    QObject::connect(button, &QPushButton::clicked, button, [button, hooks] {
        QMenu menu(button);
        const auto add_choices = [&menu, &hooks](const QString& title, const auto& texts,
                                                 auto current, auto apply, bool enabled) {
            QMenu* sub = menu.addMenu(title);
            sub->setEnabled(enabled);
            auto* group = new QActionGroup(sub);
            for (const auto& [value, text] : texts) {
                QAction* action = sub->addAction(text);
                action->setCheckable(true);
                action->setChecked(value == current);
                group->addAction(action);
                QObject::connect(action, &QAction::triggered, sub, [apply, value = value, hooks] {
                    apply(value);
                    hooks.refresh();
                });
            }
        };

        std::map<Settings::RendererBackend, QString> renderers;
        for (const auto& [backend, text] : ConfigurationShared::renderer_backend_texts_map) {
            if (backend != Settings::RendererBackend::Null) {
                renderers.emplace(backend, text);
            }
        }
        add_choices(
            QObject::tr("&Renderer"), renderers, Settings::values.renderer_backend.GetValue(),
            [](Settings::RendererBackend backend) {
                Settings::values.renderer_backend.SetValue(backend);
            },
            !hooks.renderer_locked());
        add_choices(
            QObject::tr("GPU &Accuracy"), ConfigurationShared::gpu_accuracy_texts_map,
            Settings::values.gpu_accuracy.GetValue(),
            [](Settings::GpuAccuracy accuracy) {
                Settings::values.gpu_accuracy.SetValue(accuracy);
                QtCommon::system->ApplySettings();
            },
            true);
        add_choices(
            QObject::tr("Upscaling &Filter"), ConfigurationShared::scaling_filter_texts_map,
            Settings::values.scaling_filter.GetValue(),
            [](Settings::ScalingFilter filter) { Settings::values.scaling_filter.SetValue(filter); },
            true);
        add_choices(
            QObject::tr("Anti-&Aliasing"), ConfigurationShared::anti_aliasing_texts_map,
            Settings::values.anti_aliasing.GetValue(),
            [](Settings::AntiAliasing mode) { Settings::values.anti_aliasing.SetValue(mode); },
            true);
        {
            QMenu* fg_menu = menu.addMenu(QObject::tr("&Frame Generation"));
            auto* fg_group = new QActionGroup(fg_menu);
            const bool fg_on = Settings::values.frame_gen.GetValue();
            QAction* fg_off = fg_menu->addAction(QObject::tr("Off"));
            fg_off->setCheckable(true);
            fg_off->setChecked(!fg_on);
            fg_group->addAction(fg_off);
            QObject::connect(fg_off, &QAction::triggered, fg_menu, [hooks] {
                Settings::values.frame_gen.SetValue(false);
                hooks.refresh();
            });

            for (u32 mult = 2; mult <= 4; ++mult) {
                QAction* mult_action = fg_menu->addAction(QObject::tr("%1x Multiplier").arg(mult));
                mult_action->setCheckable(true);
                mult_action->setChecked(fg_on && Settings::values.frame_gen_multiplier.GetValue() == mult);
                fg_group->addAction(mult_action);
                QObject::connect(mult_action, &QAction::triggered, fg_menu, [mult, hooks] {
                    Settings::values.frame_gen.SetValue(true);
                    Settings::values.frame_gen_multiplier.SetValue(mult);
                    hooks.refresh();
                });
            }

            fg_menu->addSeparator();
            for (const auto& [type, text] : ConfigurationShared::frame_gen_type_texts_map) {
                QAction* type_action = fg_menu->addAction(QCoreApplication::translate("MainWindow", text.toUtf8().constData()));
                type_action->setCheckable(true);
                type_action->setChecked(Settings::values.frame_gen_type.GetValue() == type);
                QObject::connect(type_action, &QAction::triggered, fg_menu, [type, hooks] {
                    Settings::values.frame_gen_type.SetValue(type);
                    hooks.refresh();
                });
            }
        }
#ifdef HAS_RESHADE
        QMenu* effects = menu.addMenu(QObject::tr("&Effects"));
        auto* effects_group = new QActionGroup(effects);
        const bool fx_on = Settings::values.post_shader_enabled.GetValue();
        const std::string active = VideoCore::GetActiveFxPreset();
        QAction* off = effects->addAction(QObject::tr("Off"));
        off->setCheckable(true);
        off->setChecked(!fx_on);
        effects_group->addAction(off);
        QObject::connect(off, &QAction::triggered, effects, [hooks] {
            Settings::values.post_shader_enabled.SetValue(false);
            hooks.refresh();
        });
        for (const auto& preset : VideoCore::GetFxPresetCatalog()) {
            QAction* action = effects->addAction(QString::fromStdString(preset.name));
            action->setCheckable(true);
            action->setChecked(fx_on && preset.name == active);
            effects_group->addAction(action);
            QObject::connect(action, &QAction::triggered, effects, [name = preset.name, hooks] {
                VideoCore::ApplyFxPreset(name);
                Settings::values.post_shader_enabled.SetValue(true);
                hooks.refresh();
            });
        }
        if (hooks.open_effects) {
            effects->addSeparator();
            effects->addAction(QObject::tr("Configure Effects..."), effects, hooks.open_effects);
        }
#endif
        menu.addSeparator();
        menu.addAction(QObject::tr("More Settings..."), &menu, hooks.open_settings);

        const QSize size = menu.sizeHint();
        menu.exec(button->mapToGlobal(QPoint(0, -size.height())));
    });
    return button;
}

QWidget* SettingsPage(QWidget* page) {
    if (!Enabled() || !page) {
        return page;
    }
    // One scroll area per page, kept on the page (it lives as long as the page's dialog).
    if (auto* area = qobject_cast<QScrollArea*>(page->property("LichiiScroll").value<QObject*>())) {
        return area;
    }
    auto* area = new QScrollArea;
    area->setFrameShape(QFrame::NoFrame);
    area->setWidgetResizable(true);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    area->setWidget(page);
    area->setAccessibleName(page->accessibleName());
    page->setProperty("LichiiScroll", QVariant::fromValue<QObject*>(area));
    return area;
}

const char* LoadingBarStyle() {
    return R"(
QProgressBar {
  background-color: rgba(255, 255, 255, 30);
  border: none;
  border-radius: 3px;
  max-height: 6px;
}
QProgressBar::chunk {
  background-color: rgba(0, 200, 83, 85);
  border-radius: 3px;
})";
}

namespace {
constexpr int KeptSettingsBackups = 10;
constexpr const char* BackupFolderFormat = "yyyy-MM-dd HHmmss";

QString ConfigDirPath() {
    return QString::fromStdString(Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::ConfigDir)));
}

bool CopyDirFiles(const QString& from, const QString& to) {
    const QDir source(from);
    if (!source.exists()) {
        return true;
    }
    if (!QDir().mkpath(to)) {
        return false;
    }
    bool ok = true;
    for (const QString& name : source.entryList(QDir::Files)) {
        QFile::remove(QDir(to).filePath(name));
        ok &= QFile::copy(source.filePath(name), QDir(to).filePath(name));
    }
    return ok;
}
} // namespace

QString BackupSettings() {
    const QDir config(ConfigDirPath());
    const QString folder =
        config.filePath(QStringLiteral("backups/") +
                        QDateTime::currentDateTime().toString(QString::fromLatin1(BackupFolderFormat)));
    if (!QDir().mkpath(folder)) {
        LOG_WARNING(Frontend, "Settings backup: cannot create {}", folder.toStdString());
        return {};
    }
    bool ok = true;
    for (const QString& name : {QStringLiteral("qt-config.ini"), QStringLiteral("frizti.ini")}) {
        if (config.exists(name)) {
            ok &= QFile::copy(config.filePath(name), QDir(folder).filePath(name));
        }
    }
    ok &= CopyDirFiles(config.filePath(QStringLiteral("custom")),
                       QDir(folder).filePath(QStringLiteral("custom")));
    LOG_INFO(Frontend, "Settings backup {}: {}", folder.toStdString(), ok ? "saved" : "incomplete");
    // Keep the newest few.
    const auto backups = SettingsBackups();
    for (size_t i = KeptSettingsBackups; i < backups.size(); ++i) {
        QDir(backups[i].first).removeRecursively();
    }
    return ok ? folder : QString{};
}

std::vector<std::pair<QString, QString>> SettingsBackups() {
    const QDir root(QDir(ConfigDirPath()).filePath(QStringLiteral("backups")));
    std::vector<std::pair<QString, QString>> backups;
    // Folder names sort by time.
    const QStringList names = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
    for (const QString& name : names) {
        const QDateTime when =
            QDateTime::fromString(name, QString::fromLatin1(BackupFolderFormat));
        if (!when.isValid() || !QFile::exists(QDir(root.filePath(name)).filePath(QStringLiteral("qt-config.ini")))) {
            continue;
        }
        backups.emplace_back(root.filePath(name),
                             QLocale().toString(when, QStringLiteral("d MMM yyyy, HH:mm")));
    }
    return backups;
}

bool RestoreSettingsFiles(const QString& backup) {
    const QDir config(ConfigDirPath());
    const QDir from(backup);
    if (!from.exists(QStringLiteral("qt-config.ini"))) {
        return false;
    }
    bool ok = true;
    for (const QString& name : {QStringLiteral("qt-config.ini"), QStringLiteral("frizti.ini")}) {
        if (from.exists(name)) {
            QFile::remove(config.filePath(name));
            ok &= QFile::copy(from.filePath(name), config.filePath(name));
        }
    }
    QDir(config.filePath(QStringLiteral("custom"))).removeRecursively();
    ok &= CopyDirFiles(from.filePath(QStringLiteral("custom")),
                       config.filePath(QStringLiteral("custom")));
    LOG_INFO(Frontend, "Settings restored from {}: {}", backup.toStdString(), ok ? "done" : "incomplete");
    return ok;
}

QString& PendingSettingsRestore() {
    static QString pending;
    return pending;
}

void OpenDiscord() {
    QDesktopServices::openUrl(QUrl(QString::fromLatin1(DiscordInvite)));
}

void CopyDataFromOldFolder(QMainWindow& window) {
    namespace fs = std::filesystem;
    using namespace Common::FS;
    if (IsPortableMode()) {
        QMessageBox::information(&window, QStringLiteral("Dekubi"),
                                 QObject::tr("This Dekubi keeps its data in the user folder next to "
                                    "lichii.exe."));
        return;
    }
    const bool any = std::ranges::any_of(
        std::array{LegacyEdenDir, LichiiDir, YuzuDir, SuyuDir, SudachiDir, CitronDir},
        [](EmuPath path) { return fs::is_directory(GetLegacyPath(path)); });
    if (!any) {
        QMessageBox::information(&window, QStringLiteral("Dekubi"),
                                 QObject::tr("No eden or lichii folder was found in %APPDATA%."));
        return;
    }
    // The same dialog as the first launch (v5.8.2): which folder, and saves / settings / mods.
    const auto choice = UserDataMigrator::AskImport(&window, true);
    if (!choice) {
        return;
    }
    try {
        FrontendCommon::RequestProfileImport(choice->Serialize());
    } catch (const std::exception& error) {
        QMessageBox::warning(&window, QStringLiteral("Dekubi"), QString::fromUtf8(error.what()));
        return;
    }
    // The copy runs when Dekubi starts, before anything reads the settings.
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, [] {
        QProcess::startDetached(QCoreApplication::applicationFilePath(), {});
    });
    window.close();
}

void OpenDonate() {
    QDesktopServices::openUrl(QUrl(QString::fromLatin1(DonateLink)));
}

namespace {
QString OcarinaGameConfigPath() {
    return QDir(ConfigDirPath()).filePath(QStringLiteral("custom/0400486024DEC000.ini"));
}
} // namespace

QStringList OcarinaGameOverrides() {
    QFile file(OcarinaGameConfigPath());
    QStringList names;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return names;
    }
    const QString suffix = QStringLiteral("\\use_global=false");
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        // The memory layout is Dekubi's own while a Switch 2 game runs (12 GB, core.cpp), and
        // saving Game Settings during play could store it as OoT's: never the player's choice,
        // so it doesn't count (players read the banner as "my settings changed", 2026-11-11).
        if (line.endsWith(suffix) && !line.contains(QStringLiteral("memory_layout_mode"))) {
            names << line.chopped(suffix.size());
        }
    }
    return names;
}

bool ClearOcarinaGameOverrides() {
    QFile file(OcarinaGameConfigPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray text = file.readAll();
    file.close();
    text.replace("\\use_global=false", "\\use_global=true");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const bool ok = file.write(text) == text.size();
    LOG_INFO(Frontend, "Ocarina of Time's game-specific settings back on the global ones: {}",
             ok ? "done" : "failed");
    return ok;
}

} // namespace LichiiUi




