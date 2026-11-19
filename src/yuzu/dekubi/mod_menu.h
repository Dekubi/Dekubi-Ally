// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <vector>

#include <QObject>
#include <QPoint>

#include "common/common_types.h"

class QCheckBox;
class QWidget;
class QLabel;
class QMainWindow;
class QPushButton;
class QStatusBar;
class QTimer;

namespace Core::Memory {
class Memory;
}

namespace LichiiUi {

/// Dekubi's mod menu for Ocarina of Time (F2 or the status bar's Mods button). A panel at the side
/// of the game with toggles; the game keeps the focus. Invincibility sets a flag the built-in
/// "Mod Menu" patch reads in Link's damage step; the others keep game data values (rupees, magic,
/// ammo) full, as the game's own flag store holds them. Only for the game build the patch is for.
class ModMenu : public QObject {
    Q_OBJECT
public:
    ModMenu(QMainWindow* window, QStatusBar* status_bar);
    ~ModMenu() override;

    void OnEmulationStarting();
    void OnEmulationStopping();
    void Toggle();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct Mod;

    void Build();
    void Place();
    void Tick();
    void UpdateState();
    /// The guest address of OoT's main code when its patched build runs, else nothing.
    std::optional<u64> MainAddress(Core::Memory::Memory& memory) const;

    QMainWindow* window;
    QPushButton* button = nullptr;
    QWidget* panel = nullptr;
    QLabel* state = nullptr;
    QTimer* timer = nullptr;
    std::vector<Mod> mods;
    std::vector<QCheckBox*> boxes;

    QPoint drag_offset;
    bool dragging = false;
    bool user_placed = false;
    bool running = false;
};

} // namespace LichiiUi
