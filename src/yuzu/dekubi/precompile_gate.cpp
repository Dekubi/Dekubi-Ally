// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/dekubi/precompile_gate.h"

#include <algorithm>

#include <QDialog>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMouseEvent>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>

#include "core/core.h"
#include "qt_common/qt_common.h"
#include "qt_common/render/emu_thread.h"
#include "video_core/gpu.h"
#include "video_core/rasterizer_interface.h"
#include "video_core/shader_notify.h"
#include "yuzu/loading_screen.h"

namespace LichiiUi {
namespace {

// A check that is over this quickly (prepared already, nothing to prepare) never shows the
// screen: the game only waits that long.
constexpr qint64 QuietCheckMs = 800;
// The rate behind "time left" comes from the last half minute.
constexpr qint64 RateWindowMs = 30'000;
// The side panel's size.
constexpr int PanelWidth = 420;
constexpr int PanelHeight = 300;

VideoCore::ShaderNotify* Notify() {
    if (!QtCommon::system || !QtCommon::system->IsPoweredOn()) {
        return nullptr;
    }
    return &QtCommon::system->GPU().ShaderNotify();
}

} // namespace

PrecompileGate::PrecompileGate(QMainWindow* window_, QStatusBar* status_bar_,
                               PrecompileHooks hooks_)
    : QObject(window_), window{window_}, status_bar{status_bar_}, hooks{std::move(hooks_)} {
    timer = new QTimer(this);
    timer->setInterval(250);
    connect(timer, &QTimer::timeout, this, &PrecompileGate::Poll);

    status_note = new QLabel;
    status_note->setObjectName(QStringLiteral("LichiiStatusNote"));
    status_note->hide();
    status_bar->addPermanentWidget(status_note);

    window->installEventFilter(this);
    clock.start();
}

PrecompileGate::~PrecompileGate() = default;

void PrecompileGate::OnEmulationStarting() {
    gated_this_boot = false;
    phase = Phase::Off;
    if (QtCommon::emu_thread) {
        connect(
            QtCommon::emu_thread.get(), &EmuThread::LoadProgress, this,
            [this](VideoCore::LoadCallbackStage stage, std::size_t, std::size_t) {
                if (stage == VideoCore::LoadCallbackStage::Complete) {
                    OnLoadComplete();
                }
            },
            Qt::QueuedConnection);
    }
}

void PrecompileGate::OnEmulationStopping() {
    if (auto* notify = Notify()) {
        notify->SetPrecompileFullSpeed(false);
    }
    timer->stop();
    phase = Phase::Off;
    status_note->hide();
    if (screen) {
        screen->hide();
        screen->deleteLater();
        screen = nullptr;
    }
}

void PrecompileGate::OnLoadComplete() {
    // Once per boot: a later shader cache reload says Complete too.
    if (gated_this_boot) {
        return;
    }
    gated_this_boot = true;
    auto* notify = Notify();
    if (!notify || !notify->PrecompilePending()) {
        return;
    }
    // The game keeps running: the screen is a side panel, not a wall in front of the game.
    phase = Phase::Checking;
    samples.clear();
    since_check.start();
    timer->start();
}

void PrecompileGate::Poll() {
    auto* notify = Notify();
    if (!notify) {
        OnEmulationStopping();
        return;
    }
    const bool pending = notify->PrecompilePending();
    const auto progress = notify->GetPrecompileProgress();
    const bool counting = progress.total > 0 && progress.done < progress.total;
    if (counting) {
        samples.emplace_back(clock.elapsed(), progress.done);
        while (samples.size() > 2 && samples.back().first - samples.front().first > RateWindowMs) {
            samples.pop_front();
        }
    }

    switch (phase) {
    case Phase::Checking:
        if (!pending) {
            phase = Phase::Off;
            timer->stop();
        } else if (counting || since_check.elapsed() > QuietCheckMs) {
            ShowScreen();
            UpdateScreen(progress.done, progress.total);
        }
        break;
    case Phase::Screen:
        if (!pending) {
            CloseScreen();
            phase = Phase::Off;
            timer->stop();
        } else {
            UpdateScreen(progress.done, progress.total);
        }
        break;
    case Phase::Background:
        if (!pending) {
            status_note->hide();
            phase = Phase::Off;
            timer->stop();
        } else if (counting) {
            const int percent = static_cast<int>(progress.done * 100 / progress.total);
            status_note->setText(tr("Preparing shaders %1%").arg(percent));
            status_note->show();
        }
        break;
    case Phase::Off:
        timer->stop();
        break;
    }
}

void PrecompileGate::ShowScreen() {
    phase = Phase::Screen;
    if (screen) {
        screen->show();
        return;
    }
    // A panel at the side of the game, on top of it but never taking focus from it.
    screen = new QDialog(window, Qt::FramelessWindowHint | Qt::Tool |
                                     Qt::WindowDoesNotAcceptFocus);
    screen->setObjectName(QStringLiteral("LichiiPrepare"));
    screen->setAttribute(Qt::WA_StyledBackground);
    screen->setAttribute(Qt::WA_ShowWithoutActivating);
    screen->setModal(false);
    screen->setCursor(Qt::SizeAllCursor);
    screen->installEventFilter(this); // dragging (eventFilter)
    user_placed = false;
    // Esc or Alt+F4 on the screen: play now (the preparation goes on in the background).
    connect(screen, &QDialog::rejected, this, &PrecompileGate::PlayNow);

    // Rounded card with a green border: the window itself is transparent around it.
    screen->setAttribute(Qt::WA_TranslucentBackground);
    auto* frame_layout = new QVBoxLayout(screen);
    frame_layout->setContentsMargins(0, 0, 0, 0);
    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("LichiiPrepareCard"));
    card->setStyleSheet(QStringLiteral(
        "#LichiiPrepareCard { background: #141416; border: 2px solid #2ecc40; border-radius: 18px; }"
        "#LichiiPrepareCard QLabel { font-size: 15px; color: #9a9aa8; }"
        "#LichiiPrepareCard QLabel#LichiiPrepareTitle { font-size: 20px; font-weight: 600; color: #ececf1; }"
        "#LichiiPrepareCard QLabel#LichiiPrepareCount { font-size: 22px; font-weight: 600; color: #ececf1; }"
        "#LichiiPrepareCard QPushButton#LichiiPreparePlay { font-size: 16px; }"));
    frame_layout->addWidget(card);
    auto* column = new QVBoxLayout(card);
    column->setContentsMargins(22, 16, 16, 20);
    column->setSpacing(0);

    // Header: title and, beside it, the close button. The panel never takes focus, so Esc can't
    // reach it: closing hides it and the preparation goes on in the background (status bar note).
    auto* header = new QHBoxLayout;
    auto* title = new QLabel(tr("Preparing shaders"));
    title->setObjectName(QStringLiteral("LichiiPrepareTitle"));
    header->addWidget(title);
    header->addStretch(1);
    auto* close = new QPushButton(QStringLiteral("✕"));
    close->setToolTip(tr("Hide (shaders keep preparing in the background)"));
    close->setFixedSize(32, 32);
    close->setCursor(Qt::PointingHandCursor);
    connect(close, &QPushButton::clicked, this, &PrecompileGate::PlayNow);
    header->addWidget(close);
    column->addLayout(header);
    column->addSpacing(4);

    sentence = new QLabel(tr("This happens once, so %1 runs smoothly from the start.")
                              .arg(hooks.game_name()));
    sentence->setWordWrap(true);
    column->addWidget(sentence);
    column->addStretch(1);

    // Progress: the count large, the time left beside it, then the bar.
    auto* numbers = new QHBoxLayout;
    count = new QLabel;
    count->setObjectName(QStringLiteral("LichiiPrepareCount"));
    eta = new QLabel;
    numbers->addWidget(count, 0, Qt::AlignBottom);
    numbers->addStretch(1);
    numbers->addWidget(eta, 0, Qt::AlignBottom);
    column->addLayout(numbers);
    column->addSpacing(8);

    bar = new QProgressBar;
    bar->setObjectName(QStringLiteral("LichiiPrepareBar"));
    bar->setTextVisible(false);
    bar->setFixedHeight(14);
    column->addWidget(bar);
    column->addSpacing(18);

    auto* play = new QPushButton(tr("Play now"));
    play->setObjectName(QStringLiteral("LichiiPreparePlay"));
    play->setMinimumHeight(46);
    play->setCursor(Qt::PointingHandCursor);
    connect(play, &QPushButton::clicked, this, &PrecompileGate::PlayNow);
    column->addWidget(play);

    PlaceScreen();
    screen->show();
    screen->raise();
}

void PrecompileGate::UpdateScreen(size_t done, size_t total) {
    if (!screen) {
        return;
    }
    const QLocale locale;
    if (total == 0) {
        // Looking for the file or assembling it from the recipe: no count yet.
        bar->setRange(0, 0);
        count->setText(tr("Getting ready"));
        eta->clear();
        return;
    }
    // QProgressBar takes an int: scale large counts.
    bar->setRange(0, 1000);
    bar->setValue(static_cast<int>(done * 1000 / total));
    count->setText(tr("%1 / %2").arg(locale.toString(static_cast<qulonglong>(done)),
                                      locale.toString(static_cast<qulonglong>(total))));
    eta->setText(TimeLeft(done, total));
}

QString PrecompileGate::TimeLeft(size_t done, size_t total) {
    if (samples.size() < 2) {
        return {};
    }
    const auto& [first_ms, first_done] = samples.front();
    const auto& [last_ms, last_done] = samples.back();
    const qint64 span = last_ms - first_ms;
    if (span < 3000 || last_done <= first_done) {
        return {};
    }
    const double per_ms = static_cast<double>(last_done - first_done) / static_cast<double>(span);
    const double seconds = static_cast<double>(total - done) / per_ms / 1000.0;
    if (seconds < 60) {
        return tr("Less than a minute left");
    }
    const int minutes = static_cast<int>(seconds / 60 + 0.5);
    if (minutes < 60) {
        return tr("About %n min left", "", minutes);
    }
    return tr("About %1 h %2 min left").arg(minutes / 60).arg(minutes % 60);
}

void PrecompileGate::PlayNow() {
    if (phase != Phase::Screen) {
        return;
    }
    CloseScreen();
    phase = Phase::Background;
}

void PrecompileGate::CloseScreen() {
    if (auto* notify = Notify()) {
        notify->SetPrecompileFullSpeed(false);
    }
    if (screen) {
        QDialog* closing = screen;
        screen = nullptr;
        closing->hide();
        closing->deleteLater();
    }
}

void PrecompileGate::PlaceScreen() {
    if (!screen || user_placed) {
        return;
    }
    // Docked to the right edge of the game area, vertically centred, clear of the HUD corners.
    QWidget* area = window->centralWidget();
    const QPoint origin = area->mapToGlobal(QPoint(0, 0));
    const int width = (std::min)(PanelWidth, area->width());
    const int height = (std::min)(PanelHeight, area->height());
    screen->setGeometry(origin.x() + area->width() - width - 16,
                        origin.y() + (area->height() - height) / 2, width, height);
}

bool PrecompileGate::eventFilter(QObject* watched, QEvent* event) {
    // The frameless panel moves by dragging its background.
    if (screen && watched == screen) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            if (mouse->button() == Qt::LeftButton) {
                dragging = true;
                drag_offset = mouse->globalPosition().toPoint() - screen->pos();
                return true;
            }
            break;
        case QEvent::MouseMove:
            if (dragging) {
                screen->move(mouse->globalPosition().toPoint() - drag_offset);
                user_placed = true;
                return true;
            }
            break;
        case QEvent::MouseButtonRelease:
            dragging = false;
            break;
        default:
            break;
        }
    }
    if (watched == window && screen &&
        (event->type() == QEvent::Move || event->type() == QEvent::Resize)) {
        PlaceScreen();
    }
    return QObject::eventFilter(watched, event);
}

} // namespace LichiiUi




