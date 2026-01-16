#include "include/config.h"
#include "include/mainwindow.h"
#include "include/gamepage.h"
#include "include/mainmenupage.h"
#include "include/levelselectpage.h"

#include <QStackedWidget>
#include <QVBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QSettings>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), stackedWidget(nullptr), gamePage(nullptr), mainMenuPage(nullptr), levelSelectPage(nullptr)
{
    // 设置全局的 QSettings 组织名和应用名
    QApplication::setApplicationName(GameConfig::APP_NAME);
    QApplication::setApplicationVersion(GameConfig::APP_VER);
    QApplication::setOrganizationName(GameConfig::ORG_NAME);

    QWidget *central = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    stackedWidget = new QStackedWidget(central);
    layout->addWidget(stackedWidget);

    setCentralWidget(central);

    mainMenuPage = new MainMenuPage(this);
    levelSelectPage = new LevelSelectPage(this);
    gamePage = new GamePage(this);

    stackedWidget->addWidget(mainMenuPage);
    stackedWidget->addWidget(levelSelectPage);
    stackedWidget->addWidget(gamePage);
    stackedWidget->setCurrentWidget(mainMenuPage);

    connect(mainMenuPage, &MainMenuPage::openLevelSelectRequested,
            this, [this]() {
                if (stackedWidget)
                    stackedWidget->setCurrentIndex(1);
            });
    connect(mainMenuPage, &MainMenuPage::exitGameRequested,
            this, &QApplication::quit);
    connect(levelSelectPage, &LevelSelectPage::startGameRequested,
            this, &MainWindow::switchToGamePage);
    connect(levelSelectPage, &LevelSelectPage::returnToMainMenuRequested,
            this, &MainWindow::switchToMainMenu);
    connect(gamePage, &GamePage::returnToMainMenu,
            this, &MainWindow::switchToMainMenu);
    connect(gamePage, &GamePage::gameOver,
            this, &MainWindow::onGameOver);

    setWindowTitle("塔之进化 - TowerEvolution");
    resize(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
}

MainWindow::~MainWindow()
{
}

void MainWindow::switchToGamePage(GameConfig::MapId mapId)
{
    if (stackedWidget)
        stackedWidget->setCurrentIndex(2);
    if (gamePage)
    {
        gamePage->setMap(mapId);
        gamePage->startGame();
    }
}

void MainWindow::switchToMainMenu()
{
    if (gamePage)
    {
        // 移除任何图形效果以防止 painter 冲突
        gamePage->setGraphicsEffect(nullptr);
        gamePage->resetGame();
    }
    if (stackedWidget)
        stackedWidget->setCurrentIndex(0);
}

void MainWindow::onGameOver()
{
    switchToMainMenu();
}
