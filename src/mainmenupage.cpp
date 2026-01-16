#include "include/config.h"
#include "include/mainmenupage.h"
#include "include/resourcemanager.h"

#include "ui_mainmenupage.h"

#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFont>
#include <QSpacerItem>
#include <QApplication>

MainMenuPage::MainMenuPage(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::MainMenuPage)
{
    ui->setupUi(this);
    initUI();

    setAutoFillBackground(false);
    setAttribute(Qt::WA_StyledBackground, false);
}

MainMenuPage::~MainMenuPage()
{
    delete ui;
}

void MainMenuPage::initUI()
{
    setFixedSize(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

    ResourceManager &rm = ResourceManager::instance();
    QPixmap background = rm.getDefaultBackground();

    backgroundLabel = ui->backgroundLabel;
    if (backgroundLabel)
    {
        backgroundLabel->setPixmap(background);
        backgroundLabel->setScaledContents(true);
        backgroundLabel->setGeometry(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
        backgroundLabel->lower();
    }

    titleLabel = ui->titleLabel;
    startButton = ui->startButton;
    exitButton = ui->exitButton;

    if (startButton)
        connect(startButton, &QPushButton::clicked, this, &MainMenuPage::onStartButtonClicked);
    if (exitButton)
        connect(exitButton, &QPushButton::clicked, this, &MainMenuPage::onExitButtonClicked);
}

void MainMenuPage::loadResources()
{
}

void MainMenuPage::onStartButtonClicked()
{
    emit openLevelSelectRequested();
}

void MainMenuPage::onExitButtonClicked()
{
    emit exitGameRequested();
}
