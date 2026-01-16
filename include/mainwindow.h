#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "config.h"
#include <QMainWindow>

// 前向声明
class GamePage;
class MainMenuPage;
class LevelSelectPage;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void switchToGamePage(GameConfig::MapId mapId);
    void switchToMainMenu();

private slots:
    void onGameOver();

private:
    QStackedWidget *stackedWidget;
    GamePage *gamePage;
    MainMenuPage *mainMenuPage;
    LevelSelectPage *levelSelectPage;
};

#endif // MAINWINDOW_H
