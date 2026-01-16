#ifndef LEVELSELECTPAGE_H
#define LEVELSELECTPAGE_H

#include "config.h"
#include <QWidget>

class QPushButton;
class QLabel;
class QVBoxLayout;

namespace Ui
{
class LevelSelectPage;
}

class LevelSelectPage : public QWidget
{
    Q_OBJECT

public:
    explicit LevelSelectPage(QWidget *parent = nullptr);
    ~LevelSelectPage();

signals:
    void startGameRequested(GameConfig::MapId mapId);
    void returnToMainMenuRequested();

private slots:
    void onMap1Clicked();
    void onMap2Clicked();
    void onCancelClicked();

private:
    void initUI();
    void loadProgress();

    Ui::LevelSelectPage *ui;

    GameConfig::MapId mapId;
    QLabel *map1StatusLabel;
    QLabel *map2StatusLabel;
    QLabel *map1WaveLabel;
    QLabel *map2WaveLabel;
    QPushButton *map2Button;
};

#endif
