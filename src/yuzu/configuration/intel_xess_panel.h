// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

class IntelXessPanel : public QWidget {
public:
    explicit IntelXessPanel(QWidget* parent);

    void Refresh();

protected:
    void showEvent(QShowEvent* event) override;

private:
    QLabel* icon_label;
    QLabel* status_label;
    QPushButton* folder_button;
};
