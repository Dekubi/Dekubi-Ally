// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/dekubi/mod_menu.h"

#include <algorithm>

#include <QCheckBox>
#include <QWidget>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPushButton>
#include <QShortcut>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>

#include "core/core.h"
#include "core/hle/kernel/k_process.h"
#include "core/memory.h"
#include "qt_common/qt_common.h"

namespace LichiiUi {
namespace {

constexpr u64 OcarinaOfTime = 0x0400486024DEC000ULL;
// What the built-in "Mod Menu" patch (core/file_sys/patch_manager.cpp) puts in main: its hook word
// at the Life step, proof the build is the one these offsets are for, and its flags in BSS.
constexpr u64 LifeHookOffset = 0x49DCB0;
constexpr u32 LifeHookWord = 0x14DEA49C;
constexpr u64 FlagsOffset = 0x4E9FF00;
// OoT's game data table (docs/oot_gamedata_notes.md): main+0x4e842a8 points at it; buckets of
// (u32 key, u32 index) at +224 (count at +236), 40-byte entries at +216 (count at +208), value +28.
constexpr u64 TablePointerOffset = 0x4E842A8;
constexpr int PanelWidth = 300;

/// The guest address of the value of a game data key, or nothing.
std::optional<u64> ValueAddress(Core::Memory::Memory& memory, u64 main, u32 key) {
    const u64 table = memory.Read64(main + TablePointerOffset);
    if (table == 0 || !memory.IsValidVirtualAddressRange(table, 240)) {
        return std::nullopt;
    }
    const u64 buckets = memory.Read64(table + 224);
    const u32 bucket_count = memory.Read32(table + 236);
    const u64 entries = memory.Read64(table + 216);
    const u32 entry_count = memory.Read32(table + 208);
    if (bucket_count == 0 || !memory.IsValidVirtualAddressRange(buckets, u64{bucket_count} * 8) ||
        !memory.IsValidVirtualAddressRange(entries, u64{entry_count} * 40)) {
        return std::nullopt;
    }
    const u32 start = key % bucket_count;
    u32 i = start;
    do {
        const u32 hash = memory.Read32(buckets + u64{i} * 8);
        if (hash == key) {
            const u32 index = memory.Read32(buckets + u64{i} * 8 + 4);
            return index < entry_count ? std::optional{entries + u64{index} * 40 + 28} : std::nullopt;
        }
        if (hash == 0) {
            return std::nullopt;
        }
        i = i + 1 < bucket_count ? i + 1 : 0;
    } while (i != start);
    return std::nullopt;
}

} // namespace

struct ModMenu::Mod {
    enum class Kind {
        Flag, ///< a byte the built-in patch reads
        Fill, ///< the value kept at the value of `source` (its capacity)
        Keep, ///< the value kept at `amount` once the save has shown any
    };
    QString label;
    Kind kind;
    u32 key = 0;
    u32 source = 0;
    s32 amount = 0;
    bool on = false;
    bool seen = false;
};

ModMenu::ModMenu(QMainWindow* window_, QStatusBar* status_bar) : QObject(window_), window(window_) {
    using Kind = Mod::Kind;
    // Keys from daurosk/oot-remake-multiplayer's docs/items.md (Murmur3 of the flag names).
    mods = {
        {tr("Invincibility"), Kind::Flag, 0},
        {tr("Infinite rupees"), Kind::Fill, 0xA77921D7, 0xE0010A50},    // CurrentRupee <- wallet
        {tr("Infinite Deku Seeds"), Kind::Fill, 0x5FADF7FC, 0xAE8CC742},
        {tr("Infinite Deku Sticks"), Kind::Fill, 0xB9494411, 0x457616B0},
        {tr("Infinite Deku Nuts"), Kind::Fill, 0xF30D9F54, 0x735B00F7},
        {tr("Infinite bombs"), Kind::Fill, 0xDB1BCD4F, 0xE753508B},
        {tr("Infinite arrows"), Kind::Fill, 0x518EC74B, 0x418DB791},
        {tr("Infinite Bombchus"), Kind::Keep, 0xDF7C517A, 0, 50},
        {tr("Infinite Magic Beans"), Kind::Keep, 0xB003124D, 0, 10},
    };

    button = new QPushButton(tr("Mods"));
    button->setObjectName(QStringLiteral("TogglableStatusBarButton"));
    button->setFocusPolicy(Qt::NoFocus);
    button->setToolTip(tr("Mod menu (F2)"));
    button->hide();
    connect(button, &QPushButton::clicked, this, &ModMenu::Toggle);
    status_bar->insertPermanentWidget(0, button);

    auto* shortcut = new QShortcut(QKeySequence(Qt::Key_F2), window, nullptr, nullptr,
                                   Qt::ApplicationShortcut);
    connect(shortcut, &QShortcut::activated, this, &ModMenu::Toggle);

    timer = new QTimer(this);
    timer->setInterval(250);
    connect(timer, &QTimer::timeout, this, &ModMenu::Tick);
}

ModMenu::~ModMenu() = default;

void ModMenu::OnEmulationStarting() {
    running = true;
    // Shown once the game is OoT (the program id is known a moment after the start).
    QTimer::singleShot(3000, this, [this] {
        const bool oot = running && QtCommon::system &&
                         QtCommon::system->GetApplicationProcessProgramID() == OcarinaOfTime;
        button->setVisible(oot);
    });
    timer->start();
}

void ModMenu::OnEmulationStopping() {
    running = false;
    timer->stop();
    button->hide();
    if (panel) {
        panel->hide();
    }
    for (Mod& mod : mods) {
        mod.seen = false;
    }
}

void ModMenu::Toggle() {
    if (!running || !button->isVisible()) {
        return;
    }
    if (!panel) {
        Build();
    }
    if (panel->isVisible()) {
        panel->hide();
        return;
    }
    Place();
    panel->show();
    UpdateState();
}

void ModMenu::Build() {
    // A tool window like the performance overlay, which fullscreen keeps: a QDialog took the game
    // out of fullscreen. It never takes the focus.
    panel = new QWidget(window, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus |
                                    Qt::NoDropShadowWindowHint);
    panel->setAttribute(Qt::WA_TranslucentBackground);
    panel->setAttribute(Qt::WA_ShowWithoutActivating);
    panel->installEventFilter(this);

    auto* frame_layout = new QVBoxLayout(panel);
    frame_layout->setContentsMargins(0, 0, 0, 0);
    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("DekubiModCard"));
    card->setStyleSheet(QStringLiteral(
        "#DekubiModCard { background: #141416; border: 2px solid #2ecc40; border-radius: 18px; }"
        "#DekubiModCard QLabel { font-size: 13px; color: #9a9aa8; }"
        "#DekubiModCard QLabel#DekubiModTitle { font-size: 18px; font-weight: 600; color: #ececf1; }"
        "#DekubiModCard QCheckBox { font-size: 15px; color: #ececf1; padding: 3px 0; }"
        "#DekubiModCard QCheckBox::indicator:checked { background: #2ecc40; border: 1px solid #2ecc40; }"));
    frame_layout->addWidget(card);
    auto* column = new QVBoxLayout(card);
    column->setContentsMargins(20, 14, 14, 18);
    column->setSpacing(2);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(tr("Mods"));
    title->setObjectName(QStringLiteral("DekubiModTitle"));
    header->addWidget(title);
    header->addStretch(1);
    auto* close = new QPushButton(QStringLiteral("✕"));
    close->setToolTip(tr("Close (F2)"));
    close->setFocusPolicy(Qt::NoFocus);
    close->setFlat(true);
    connect(close, &QPushButton::clicked, panel, &QWidget::hide);
    header->addWidget(close);
    column->addLayout(header);

    for (size_t i = 0; i < mods.size(); ++i) {
        auto* box = new QCheckBox(mods[i].label);
        box->setFocusPolicy(Qt::NoFocus);
        box->setChecked(mods[i].on);
        connect(box, &QCheckBox::toggled, this, [this, i](bool on) {
            mods[i].on = on;
            Tick();
        });
        boxes.push_back(box);
        column->addWidget(box);
    }
    state = new QLabel;
    state->setWordWrap(true);
    column->addSpacing(8);
    column->addWidget(state);
    panel->adjustSize();
}

void ModMenu::Place() {
    if (user_placed) {
        return;
    }
    // At the left edge of the game area, clear of the "Preparing shaders" panel on the right.
    QWidget* area = window->centralWidget();
    const QPoint origin = area->mapToGlobal(QPoint(0, 0));
    panel->setFixedWidth(PanelWidth);
    panel->adjustSize();
    panel->move(origin.x() + 16, origin.y() + (std::max)(16, (area->height() - panel->height()) / 2));
}

std::optional<u64> ModMenu::MainAddress(Core::Memory::Memory& memory) const {
    if (!QtCommon::system || QtCommon::system->GetApplicationProcessProgramID() != OcarinaOfTime) {
        return std::nullopt;
    }
    const Kernel::KProcess* process = QtCommon::system->ApplicationProcess();
    const u64 main = process ? process->GetOotMainCodeAddress() : 0;
    if (main == 0 || !memory.IsValidVirtualAddress(main + LifeHookOffset) ||
        memory.Read32(main + LifeHookOffset) != LifeHookWord) {
        return std::nullopt;
    }
    return main;
}

void ModMenu::UpdateState() {
    if (!state || !QtCommon::system) {
        return;
    }
    const bool ready = running && MainAddress(QtCommon::system->ApplicationMemory()).has_value();
    state->setText(ready ? tr("F2 opens and closes this menu.")
                         : tr("Needs OoT 1.1.1 with \"Mod Menu (built-in)\" on in Add-Ons."));
}

void ModMenu::Tick() {
    if (!running || !QtCommon::system) {
        return;
    }
    Core::Memory::Memory& memory = QtCommon::system->ApplicationMemory();
    const auto main = MainAddress(memory);
    if (!main) {
        return;
    }
    for (Mod& mod : mods) {
        if (mod.kind == Mod::Kind::Flag) {
            // Written either way, so turning it off takes effect too.
            const u64 flag = *main + FlagsOffset + mod.key;
            if (memory.Read8(flag) != (mod.on ? 1 : 0)) {
                memory.Write8(flag, mod.on ? 1 : 0);
            }
            continue;
        }
        if (!mod.on) {
            continue;
        }
        const auto value = ValueAddress(memory, *main, mod.key);
        if (!value) {
            continue;
        }
        const s32 current = static_cast<s32>(memory.Read32(*value));
        s32 target = mod.amount;
        if (mod.kind == Mod::Kind::Fill) {
            const auto source = ValueAddress(memory, *main, mod.source);
            target = source ? static_cast<s32>(memory.Read32(*source)) : 0;
        } else {
            mod.seen = mod.seen || current > 0;
            if (!mod.seen) {
                continue;
            }
        }
        // Nothing to fill without the bag, wallet or meter; never take anything away.
        if (target > 0 && current < target) {
            memory.Write32(*value, static_cast<u32>(target));
        }
    }
}

bool ModMenu::eventFilter(QObject* watched, QEvent* event) {
    // The frameless panel moves by dragging its background.
    if (panel && watched == panel) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            if (mouse->button() == Qt::LeftButton) {
                dragging = true;
                drag_offset = mouse->globalPosition().toPoint() - panel->pos();
                return true;
            }
            break;
        case QEvent::MouseMove:
            if (dragging) {
                panel->move(mouse->globalPosition().toPoint() - drag_offset);
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
    return QObject::eventFilter(watched, event);
}

} // namespace LichiiUi
