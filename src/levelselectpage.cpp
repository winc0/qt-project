#include "include/levelselectpage.h"
#include "include/resourcemanager.h"

#include "ui_levelselectpage.h"

#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QSettings>
#include <QDebug>

LevelSelectPage::LevelSelectPage(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::LevelSelectPage),
      mapId(GameConfig::MAP1),
      map1StatusLabel(nullptr),
      map2StatusLabel(nullptr),
      map1WaveLabel(nullptr),
      map2WaveLabel(nullptr),
      map2Button(nullptr)
{
    ui->setupUi(this);

    setFixedSize(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

    initUI();
    loadProgress();
}

LevelSelectPage::~LevelSelectPage()
{
    delete ui;
}

void LevelSelectPage::initUI()
{
    ResourceManager &rm = ResourceManager::instance();
    QPixmap background = rm.getDefaultBackground();

    if (ui->backgroundLabel)
    {
        ui->backgroundLabel->setPixmap(background);
        ui->backgroundLabel->setScaledContents(true);
        ui->backgroundLabel->setGeometry(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
        ui->backgroundLabel->lower();
    }

    map1StatusLabel = ui->map1StatusLabel;
    map2StatusLabel = ui->map2StatusLabel;
    map1WaveLabel = ui->map1WaveLabel;
    map2WaveLabel = ui->map2WaveLabel;
    map2Button = ui->map2Button;

    ResourceManager &rm2 = ResourceManager::instance();
    if (ui->map1Preview)
    {
        QPixmap map1Pixmap = rm2.getGameMap(GameConfig::MAP1).scaled(260, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->map1Preview->setPixmap(map1Pixmap);
    }
    if (ui->map2Preview)
    {
        QPixmap map2Pixmap = rm2.getGameMap(GameConfig::MAP2).scaled(260, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->map2Preview->setPixmap(map2Pixmap);
    }

    if (ui->map1Button)
        connect(ui->map1Button, &QPushButton::clicked, this, &LevelSelectPage::onMap1Clicked);
    if (ui->map2Button)
        connect(ui->map2Button, &QPushButton::clicked, this, &LevelSelectPage::onMap2Clicked);
    if (ui->cancelButton)
        connect(ui->cancelButton, &QPushButton::clicked, this, &LevelSelectPage::onCancelClicked);
}

void LevelSelectPage::loadProgress()
{
    QSettings settings("TowerDefenseStudio", "TowerDefenseGame");

    int unlockedMaxIndex = settings.value("levels/unlocked_max_index", 0).toInt();

    int map1BestWave = settings.value("levels/map_0/bestWave", 0).toInt();
    int map2BestWave = settings.value("levels/map_1/bestWave", 0).toInt();

    qDebug() << "[LevelSelect] unlocked_max_index =" << unlockedMaxIndex
             << "map1BestWave =" << map1BestWave
             << "map2BestWave =" << map2BestWave;

    if (map1StatusLabel)
        map1StatusLabel->setText("已解锁");
    if (map1WaveLabel)
    {
        int displayedWave = map1BestWave > 0 ? map1BestWave : 1;
        map1WaveLabel->setText(QString("最高波次：第 %1 波").arg(displayedWave));
    }

    bool map2Unlocked = unlockedMaxIndex >= 1;
    if (map2StatusLabel)
        map2StatusLabel->setText(map2Unlocked ? "已解锁" : "未解锁");
    if (map2WaveLabel)
    {
        if (map2BestWave > 0)
            map2WaveLabel->setText(QString("最高波次：第 %1 波").arg(map2BestWave));
        else
            map2WaveLabel->setText("最高波次：未挑战");
    }

    if (map2Button)
        map2Button->setEnabled(map2Unlocked);
}

void LevelSelectPage::onMap1Clicked()
{
    mapId = GameConfig::MAP1;
    emit startGameRequested(mapId);
}

void LevelSelectPage::onMap2Clicked()
{
    mapId = GameConfig::MAP2;
    emit startGameRequested(mapId);
}

void LevelSelectPage::onCancelClicked()
{
    emit returnToMainMenuRequested();
}
