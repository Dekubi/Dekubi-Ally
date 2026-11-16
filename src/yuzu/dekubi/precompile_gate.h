// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <deque>
#include <functional>
#include <utility>

#include <QElapsedTimer>
#include <QObject>
#include <QPoint>
#include <QString>

#include "common/common_types.h"

class QDialog;
class QLabel;
class QMainWindow;
class QProgressBar;
class QStatusBar;
class QTimer;

namespace LichiiUi {

struct PrecompileHooks {
    std::function<void()> pause;           ///< hold the game (MainWindow::OnPauseGame)
    std::function<void()> resume;          ///< let it run (MainWindow::OnStartGame)
    std::function<QString()> game_name;    ///< "OoT", or "the game"
};

/// Dekubi's "Preparing shaders" panel (EDEN_NX2_PRECOMPILE). Right after a game has loaded, while
/// the precompiled pipelines are looked for, assembled or prepared, a panel docked to the right of
/// the game shows the progress; the game keeps running and keeps the focus. "Play now" hides the
/// panel and a status bar note shows the progress until it is done. When nothing needs preparing
/// no panel shows.
class PrecompileGate : public QObject {
    Q_OBJECT
public:
    PrecompileGate(QMainWindow* window, QStatusBar* status_bar, PrecompileHooks hooks);
    ~PrecompileGate() override;

    void OnEmulationStarting();
    void OnEmulationStopping();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    enum class Phase { Off, Checking, Screen, Background };

    void OnLoadComplete();
    void Poll();
    void ShowScreen();
    void PlayNow();
    void CloseScreen();
    void PlaceScreen();
    void UpdateScreen(size_t done, size_t total);
    QString TimeLeft(size_t done, size_t total);

    QMainWindow* window;
    QStatusBar* status_bar;
    PrecompileHooks hooks;

    QTimer* timer = nullptr;
    QLabel* status_note = nullptr;
    QDialog* screen = nullptr;
    QProgressBar* bar = nullptr;
    QLabel* sentence = nullptr;
    QLabel* count = nullptr;
    QLabel* eta = nullptr;

    /// Dragging the panel: the grab point inside it, and whether the player placed it themselves.
    QPoint drag_offset;
    bool dragging = false;
    bool user_placed = false;

    Phase phase = Phase::Off;
    bool gated_this_boot = false;
    QElapsedTimer since_check;
    QElapsedTimer clock;
    std::deque<std::pair<qint64, size_t>> samples;
};

} // namespace LichiiUi


