#include "include/gamepage.h"
#include "include/resourcemanager.h"
#include "include/config.h"
#include "include/mainwindow.h"
#include "include/gamemanager.h"

#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QDebug>
#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QColor>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QApplication>
#include <QSettings>
#include <cmath>
#include "include/placementvalidator.h"

GamePage::GamePage(QWidget *parent)
    : QWidget(parent),
      gameScene(nullptr),
      gameView(nullptr),
      placementValidator(nullptr),
      gameManager(new GameManager(this)),
      currentMapId(GameConfig::MAP1),
      userItem(nullptr),
      resultOverlay(nullptr),
      resultPanel(nullptr),
      pauseOverlay(nullptr),
      pausePanel(nullptr)
{
    qDebug() << "GamePage constructor called";

    // 设置固定大小
    setFixedSize(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

    initUI();
    initGameScene();
    setMap(currentMapId);

    connect(gameManager, &GameManager::goldChanged, this, [this](int gold) {
        if (goldLabel)
            goldLabel->setText(QString::number(gold));
    });
    connect(gameManager, &GameManager::livesChanged, this, [this](int lives) {
        if (livesLabel)
            livesLabel->setText(QString::number(lives));

        if (userItem)
        {
            ResourceManager &rm = ResourceManager::instance();
            if (lives <= 1)
            {
                userItem->setPixmap(rm.getUserPixmap(ResourceManager::USER_DEAD));
            }
            else
            {
                userItem->setPixmap(rm.getUserPixmap(ResourceManager::USER_WALK));
            }
        }
    });
    connect(gameManager, &GameManager::waveChanged, this, [this](int wave) {
        if (waveLabel)
            waveLabel->setText(QString("第 %1 波").arg(wave));
    });
    connect(gameManager, &GameManager::enemySpawnRequested, this, [this](QPointer<Enemy> enemy) {
        if (enemy && gameScene)
        {
            gameScene->addItem(enemy);
        }
    });
    connect(gameManager, &GameManager::enemyReachedEnd, this, [this](QPointer<Enemy> enemy) {
        if (enemy && gameScene)
        {
            gameScene->removeItem(enemy);
            enemy->deleteLater();
        }
    });
    connect(gameManager, &GameManager::enemyDied, this, [this](QPointer<Enemy> enemy) {
        if (enemy && gameScene)
        {
            QTimer::singleShot(GameConfig::ENEMY_DEAD_KEEP_TIME, [this, enemy]() {
                if (enemy && gameScene)
                {
                    gameScene->removeItem(enemy);
                    enemy->deleteLater();
                }
            });
        }
    });
    connect(gameManager, &GameManager::towerBuilt, this, [this](QPointer<Tower> tower) {
        if (!tower || !gameScene)
            return;

        QGraphicsPixmapItem *baseItem = tower->getBaseItem();
        if (baseItem)
        {
            gameScene->addItem(baseItem);
        }

        gameScene->addItem(tower);
    });
    connect(gameManager, &GameManager::towerUpgraded, this, [this](QPointer<Tower> oldTower, QPointer<Tower> newTower) {
        if (!gameScene)
            return;

        if (oldTower)
        {
            QGraphicsPixmapItem *oldBase = oldTower->getBaseItem();
            if (oldBase && oldBase->scene())
            {
                gameScene->removeItem(oldBase);
            }
            if (oldTower->scene())
            {
                gameScene->removeItem(oldTower);
            }
            oldTower->deleteLater();
        }

        if (newTower)
        {
            QGraphicsPixmapItem *newBase = newTower->getBaseItem();
            if (newBase)
            {
                gameScene->addItem(newBase);
            }
            gameScene->addItem(newTower);
        }
    });
    connect(gameManager, &GameManager::towerDemolished, this, [this](QPointer<Tower> tower) {
        if (!tower || !gameScene)
            return;

        QGraphicsPixmapItem *baseItem = tower->getBaseItem();
        if (baseItem && baseItem->scene())
        {
            gameScene->removeItem(baseItem);
        }
        if (tower->scene())
        {
            gameScene->removeItem(tower);
        }
        tower->deleteLater();
    });
    connect(gameManager, &GameManager::gameOver, this, [this]() {
        showGameOverDialog();
    });
    connect(gameManager, &GameManager::levelCompleted, this, [this](GameConfig::MapId, int) {
        showLevelCompleteDialog();
    });

    qDebug() << "GamePage initialized, size:" << size();
}

GamePage::~GamePage()
{
    if (userItem)
    {
        if (userItem->scene())
        {
            userItem->scene()->removeItem(userItem);
        }
        delete userItem;
        userItem = nullptr;
    }
    qDeleteAll(placementAreaItems);
    placementAreaItems.clear();
    delete placementValidator;
    resetGame();
}

void GamePage::setMap(GameConfig::MapId mapId)
{
    currentMapId = mapId;

    if (gameScene)
    {
        updateHoverHighlight(QPointF(-1, -1));
        if (userItem)
        {
            if (userItem->scene())
            {
                userItem->scene()->removeItem(userItem);
            }
            delete userItem;
            userItem = nullptr;
        }
        gameScene->clear();
    }

    pathPoints.clear();
    endPointAreas.clear();
    placementAreaItems.clear();

    createPath();
    drawBackground();
    drawGrid();
    initPlacementValidator();

    if (gameManager)
    {
        gameManager->resetGame();
        gameManager->initialize(currentMapId, pathPoints, endPointAreas);

        if (goldLabel)
            goldLabel->setText(QString::number(gameManager->getGold()));
        if (livesLabel)
            livesLabel->setText(QString::number(gameManager->getLives()));
        if (waveLabel)
            waveLabel->setText(QString("第 %1 波").arg(gameManager->getCurrentWave()));
    }
}

void GamePage::initUI()
{
    qDebug() << "Initializing GamePage UI";

    // 设置主页面大小
    setFixedSize(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

    // 主布局
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建控制面板 - 作为浮动面板，不添加到布局中
    controlPanel = new QWidget(this);
    controlPanel->setFixedSize(800, 120);
    controlPanel->setGeometry(0, 80, 800, 120); // 设置位置为顶部

    // 控制面板提升到最前面
    controlPanel->raise();

    // 使用绝对定位而不是布局
    // 移除之前的布局设置

    QFont infoFont("Microsoft YaHei", 12, QFont::Bold);
    QFont numberFont("Microsoft YaHei", 24, QFont::Bold);
    QFont titleFont("Microsoft YaHei", 16, QFont::Normal);

    // ==================== 金币信息 ====================
    // 金币标题（图标/文字）
    QLabel *goldTitle = new QLabel("💰 金币", controlPanel);
    goldTitle->setGeometry(80, 0, 120, 40);
    goldTitle->setFont(titleFont);
    goldTitle->setStyleSheet("color: #FFD700;");
    goldTitle->setAlignment(Qt::AlignCenter);

    goldLabel = new QLabel(QString::number(gameManager->getGold()), controlPanel);
    goldLabel->setGeometry(80, 40, 120, 40);
    goldLabel->setFont(numberFont);
    goldLabel->setStyleSheet("color: #FFD700; font-weight: bold;");
    goldLabel->setAlignment(Qt::AlignCenter);

    // ==================== 生命信息 ====================
    // 生命标题（图标/文字）
    QLabel *lifeTitle = new QLabel("❤️ 生命", controlPanel);
    lifeTitle->setGeometry(240, 0, 120, 40);
    lifeTitle->setFont(titleFont);
    lifeTitle->setStyleSheet("color: #FF4444;");
    lifeTitle->setAlignment(Qt::AlignCenter);

    livesLabel = new QLabel(QString::number(gameManager->getLives()), controlPanel);
    livesLabel->setGeometry(240, 40, 120, 40);
    livesLabel->setFont(numberFont);
    livesLabel->setStyleSheet("color: #FF4444; font-weight: bold;");
    livesLabel->setAlignment(Qt::AlignCenter);

    // ==================== 波次信息 ====================
    // 波次标题（图标/文字）
    QLabel *waveTitle = new QLabel("🌊 波次", controlPanel);
    waveTitle->setGeometry(400, 0, 120, 40);
    waveTitle->setFont(titleFont);
    waveTitle->setStyleSheet("color: #44AAFF;");
    waveTitle->setAlignment(Qt::AlignCenter);

    waveLabel = new QLabel(QString("第 %1 波").arg(gameManager->getCurrentWave()), controlPanel);
    waveLabel->setGeometry(400, 40, 120, 40);
    waveLabel->setFont(numberFont);
    waveLabel->setStyleSheet("color: #44AAFF; font-weight: bold;");
    waveLabel->setAlignment(Qt::AlignCenter);

    // ==================== 右侧按钮区域 ====================

    // 暂停按钮
    pauseButton = new QPushButton("⏸️ 暂停", controlPanel);
    pauseButton->setGeometry(600, 60, 120, 40);
    pauseButton->setStyleSheet(
        "QPushButton {"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   color: white;"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3498db, stop:1 #2980b9);"
        "   border: 2px solid #1f618d;"
        "   border-radius: 8px;"
        "   padding: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2980b9, stop:1 #3498db);"
        "   border: 2px solid #154360;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #21618c;"
        "   border: 2px solid #0e3a5e;"
        "}");
    connect(pauseButton, &QPushButton::clicked, this, &GamePage::pauseGame);

    // 返回按钮
    returnButton = new QPushButton("🏠 返回菜单", controlPanel);
    returnButton->setGeometry(600, 0, 120, 40);
    returnButton->setStyleSheet(
        "QPushButton {"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   color: white;"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #7f8c8d, stop:1 #616a6b);"
        "   border: 2px solid #424949;"
        "   border-radius: 8px;"
        "   padding: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #616a6b, stop:1 #7f8c8d);"
        "   border: 2px solid #2c3e50;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #515a5a;"
        "   border: 2px solid #1c2833;"
        "}");
    connect(returnButton, &QPushButton::clicked, this, &GamePage::returnToMainMenu);

    qDebug() << "Control panel created";
}

void GamePage::initGameScene()
{
    qDebug() << "Initializing game scene";

    // 创建游戏场景 - 占据整个800x600
    gameScene = new QGraphicsScene(this);
    gameScene->setSceneRect(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

    // 创建视图 - 占据整个GamePage
    gameView = new QGraphicsView(gameScene, this);
    gameView->setRenderHint(QPainter::Antialiasing);
    gameView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gameView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gameView->setFixedSize(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    gameView->setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
    gameView->setMouseTracking(true);
    gameView->viewport()->setMouseTracking(true);
    gameView->viewport()->installEventFilter(this);

    // 绘制背景
    drawBackground();
    drawGrid();
    drawPlacementAreas();

    // 添加到主布局
    mainLayout->addWidget(gameView);

    // 将控制面板提升到最前面（在gameView添加后）
    controlPanel->raise();

    qDebug() << "Game scene initialized, view size:" << gameView->size();
}

void GamePage::drawBackground()
{
    ResourceManager &rm = ResourceManager::instance();
    QPixmap background = rm.getGameMap(currentMapId);

    // 创建背景图形项 - 占据整个800x600
    QGraphicsPixmapItem *backgroundItem = new QGraphicsPixmapItem(background);
    backgroundItem->setZValue(-100); // 最底层
    gameScene->addItem(backgroundItem);

    if (!endPointAreas.isEmpty())
    {
        const GameConfig::EndPointConfig &end = endPointAreas.first();
        QPixmap userPixmap = rm.getUserPixmap(ResourceManager::USER_WALK);

        if (userItem)
        {
            if (userItem->scene())
            {
                userItem->scene()->removeItem(userItem);
            }
            delete userItem;
            userItem = nullptr;
        }

        userItem = new QGraphicsPixmapItem(userPixmap);
        QRectF rect = userItem->boundingRect();
        userItem->setPos(end.x - rect.width() / 2, end.y - rect.height() / 2);
        userItem->setZValue(-40);
        gameScene->addItem(userItem);
    }

    // // 绘制路径
    // if (!pathPoints.isEmpty())
    // {
    //     QPainterPath path;
    //     path.moveTo(pathPoints.first());

    //     for (int i = 1; i < pathPoints.size(); ++i)
    //     {
    //         path.lineTo(pathPoints[i]);
    //     }

    //     QGraphicsPathItem *pathItem = new QGraphicsPathItem(path);
    //     pathItem->setPen(QPen(QColor(139, 69, 19, 150), 30)); // 棕色半透明路径
    //     pathItem->setZValue(-50);
    //     gameScene->addItem(pathItem);
    // }
}

void GamePage::drawGrid()
{
    QPen gridPen(QColor(200, 255, 200, 100), 1);

    // 绘制垂直线
    for (int x = 0; x <= GameConfig::WINDOW_WIDTH; x += GameConfig::GRID_SIZE)
    {
        QGraphicsLineItem *line = new QGraphicsLineItem(x, 0, x, GameConfig::WINDOW_HEIGHT);
        line->setPen(gridPen);
        line->setZValue(-90);
        gameScene->addItem(line);
    }

    // 绘制水平线
    for (int y = 0; y <= GameConfig::WINDOW_HEIGHT; y += GameConfig::GRID_SIZE)
    {
        QGraphicsLineItem *line = new QGraphicsLineItem(0, y, GameConfig::WINDOW_WIDTH, y);
        line->setPen(gridPen);
        line->setZValue(-90);
        gameScene->addItem(line);
    }
}

void GamePage::initPlacementValidator()
{
    if (placementValidator) {
        delete placementValidator;
    }
    placementValidator = new PlacementValidator();
    placementValidator->loadConfig(GameConfig::Placement::BUILDABLE_MAP.value(currentMapId));
    drawPlacementAreas();
}

void GamePage::drawPlacementAreas()
{
    qDeleteAll(placementAreaItems);
    placementAreaItems.clear();

    if (!gameScene || !placementValidator)
        return;

    const QSet<QPair<int, int>> &allowed = placementValidator->getAllowedGrids();
    int gridSize = GameConfig::GRID_SIZE;

    for (const auto &pair : allowed)
    {
        int gx = pair.first;
        int gy = pair.second;
        
        QGraphicsRectItem *item = new QGraphicsRectItem(gx * gridSize, gy * gridSize, gridSize, gridSize);
        // Visual style: Green border, slight green fill
        item->setPen(QPen(QColor(0, 255, 0, 150), 2)); 
        item->setBrush(QBrush(QColor(0, 255, 0, 20)));
        item->setZValue(-40); // Above background/grid, below towers
        gameScene->addItem(item);
        placementAreaItems.append(item);
    }
}

void GamePage::showFloatingTip(const QString &text, const QPointF &scenePos, const QColor &color)
{
    QGraphicsTextItem *tipItem = new QGraphicsTextItem(text);
    tipItem->setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    tipItem->setDefaultTextColor(color);
    
    // Center the text above the click position
    QRectF bound = tipItem->boundingRect();
    tipItem->setPos(scenePos.x() - bound.width() / 2, scenePos.y() - bound.height());
    tipItem->setZValue(2000); // Top most
    
    gameScene->addItem(tipItem);

    // Animate opacity
    QPropertyAnimation *anim = new QPropertyAnimation(tipItem, "opacity");
    anim->setDuration(GameConfig::TIP_DURATION_MS);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    anim->setEasingCurve(QEasingCurve::InQuad);
    
    // Connect animation finish to deletion
    connect(anim, &QPropertyAnimation::finished, [tipItem]() {
        if (tipItem->scene()) {
            tipItem->scene()->removeItem(tipItem);
        }
        delete tipItem;
    });
    
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void GamePage::showUpgradeEffect(const QPointF &scenePos)
{
    if (!gameScene)
        return;

    int gridSize = GameConfig::GRID_SIZE;
    int gridX = int(scenePos.x() / gridSize) * gridSize;
    int gridY = int(scenePos.y() / gridSize) * gridSize;

    QGraphicsEllipseItem *effectItem = new QGraphicsEllipseItem(gridX, gridY, gridSize, gridSize);
    effectItem->setBrush(QBrush(QColor(255, 215, 0, 120)));
    effectItem->setPen(QPen(Qt::NoPen));
    effectItem->setZValue(1500);
    gameScene->addItem(effectItem);

    QTimer::singleShot(GameConfig::UPGRADE_EFFECT_DURATION_MS, [effectItem]()
                       {
        if (effectItem->scene())
        {
            effectItem->scene()->removeItem(effectItem);
        }
        delete effectItem;
    });
}

void GamePage::createPath()
{
    // 根据地图ID获取路径
    QVector<GameConfig::GridPoint> gridPoints = GameConfig::MapPaths::PATH_MAP.value(currentMapId, GameConfig::MapPaths::MAP1_PATH);
    
    // 获取路径点
    const qreal offset = GameConfig::GRID_SIZE / 2 - GameConfig::ENEMY_SIZE / 2;
    for (const GameConfig::GridPoint &gridPoint : gridPoints)
    {
        qreal x = gridPoint.gridX * GameConfig::GRID_SIZE + offset;
        qreal y = gridPoint.gridY * GameConfig::GRID_SIZE + offset;
        pathPoints << QPointF(x, y);
    }
    
    // 获取终点信息
    if (!gridPoints.isEmpty())
    {
        const GameConfig::GridPoint &lastPoint = gridPoints.last();
        qreal centerX = lastPoint.gridX * GameConfig::GRID_SIZE + GameConfig::GRID_SIZE / 2;
        qreal centerY = lastPoint.gridY * GameConfig::GRID_SIZE + GameConfig::GRID_SIZE / 2;
        endPointAreas.append({centerX, centerY, GameConfig::GRID_SIZE / 2});
    }
}

void GamePage::startGame()
{
    if (!gameManager)
        return;

    elapsedTimer.restart();
    pauseButton->setText("暂停");
    
    // 确保GamePage及其所有子元素立即显示和更新
    show();
    update();
    repaint();
    if (gameView)
    {
        gameView->update();
        gameView->repaint();
        gameView->viewport()->update();
        gameView->viewport()->repaint();
    }
    if (gameScene)
    {
        gameScene->update();
    }
    
    gameManager->startGame();
}

void GamePage::pauseGame()
{
    if (!gameManager)
        return;

    gameManager->pauseGame();

    if (gameManager->isPaused())
    {
        pauseAllEnemies();
        pauseAllTowersAndBullets();
        showPauseMenu(); // 显示暂停菜单
        pauseButton->setText("继续");
    }
    else
    {
        hidePauseMenu(); // 隐藏暂停菜单
        resumeAllEnemies();
        resumeAllTowersAndBullets();
        pauseButton->setText("暂停");
    }
}

void GamePage::resetGame()
{
    if (!gameManager || !gameScene)
        return;

    gameManager->resetGame();

    QList<QGraphicsItem *> items = gameScene->items();
    for (QGraphicsItem *item : items)
    {
        if (!item)
            continue;

        Enemy *enemy = dynamic_cast<Enemy *>(item);
        Tower *tower = dynamic_cast<Tower *>(item);
        Bullet *bullet = dynamic_cast<Bullet *>(item);

        if (enemy || tower || bullet)
        {
            gameScene->removeItem(item);
            QObject *obj = dynamic_cast<QObject *>(item);
            if (obj)
                obj->deleteLater();
            else
                delete item;
        }
    }

    goldLabel->setText(QString::number(gameManager->getGold()));
    livesLabel->setText(QString::number(gameManager->getLives()));
    waveLabel->setText(QString("第 %1 波").arg(gameManager->getCurrentWave()));
}

void GamePage::showGameOverDialog()
{
    if (gameManager)
    {
        pauseAllEnemies();
        pauseAllTowersAndBullets();
    }

    if (resultOverlay)
    {
        resultOverlay->deleteLater();
        resultOverlay = nullptr;
        resultPanel = nullptr;
    }

    // 移除任何图形效果以防止 painter 冲突
    setGraphicsEffect(nullptr);
    update();
    repaint();

    saveLevelProgress(false);

    resultOverlay = new QWidget(this);
    resultOverlay->setGeometry(0, 0, width(), height());
    resultOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 180);");
    resultOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    resultPanel = new QWidget(resultOverlay);
    resultPanel->setFixedSize(500, 420);
    resultPanel->setStyleSheet(
        "QWidget {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #ffffff, stop:1 #f5f5f5);"
        "   border-radius: 20px;"
        "   border: 1px solid #34495e;"
        "}"
    );
    // 使用 QGraphicsDropShadowEffect 替代边框，避免遮挡文字
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(12);
    shadowEffect->setColor(QColor(52, 73, 94, 200));
    shadowEffect->setOffset(0, 2);
    resultPanel->setGraphicsEffect(shadowEffect);
    resultPanel->move((width() - resultPanel->width()) / 2, (height() - resultPanel->height()) / 2);

    // 创建容器用于不透明度动画
    QWidget *animContainer = new QWidget(resultPanel);
    animContainer->setGeometry(resultPanel->rect());
    animContainer->lower();

    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(animContainer);
    animContainer->setGraphicsEffect(effect);
    effect->setOpacity(0.0);

    QVBoxLayout *layout = new QVBoxLayout(resultPanel);
    layout->setContentsMargins(36, 48, 36, 48);
    layout->setSpacing(20);

    // 标题
    QLabel *titleLabel = new QLabel("游戏结束", resultPanel);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setFont(QFont("Microsoft YaHei", 32, QFont::Bold));
    titleLabel->setStyleSheet("color: #2c3e50;");
    titleLabel->setMinimumHeight(48);
    layout->addWidget(titleLabel);

    qint64 elapsedMs = elapsedTimer.isValid() ? elapsedTimer.elapsed() : 0;
    int seconds = static_cast<int>(elapsedMs / 1000);

    int kill = gameManager ? gameManager->getKillCount() : 0;
    int wave = gameManager ? gameManager->getCurrentWave() : 1;
    int gold = gameManager ? gameManager->getGold() : 0;

    int score = kill * GameConfig::SCORE_PER_KILL +
                wave * GameConfig::SCORE_PER_WAVE +
                gold;
    QString grade;
    if (score >= GameConfig::SCORE_GRADE_S_MIN)
        grade = "S";
    else if (score >= GameConfig::SCORE_GRADE_A_MIN)
        grade = "A";
    else if (score >= GameConfig::SCORE_GRADE_B_MIN)
        grade = "B";
    else
        grade = "C";

    // 数据标签
    QLabel *killLabel = new QLabel(QString("击杀敌人数量：%1").arg(kill), resultPanel);
    QLabel *timeLabel = new QLabel(QString("游戏时长：%1 秒").arg(seconds), resultPanel);
    QLabel *waveLabel = new QLabel(QString("到达波次：第 %1 波").arg(wave), resultPanel);
    QLabel *scoreLabel = new QLabel(QString("得分：%1").arg(score), resultPanel);
    QLabel *gradeLabel = new QLabel(QString("评级：%1").arg(grade), resultPanel);

    for (QLabel *label : {killLabel, timeLabel, waveLabel, scoreLabel, gradeLabel})
    {
        label->setAlignment(Qt::AlignCenter);
        label->setFont(QFont("Microsoft YaHei", 16, QFont::Normal));
        label->setStyleSheet("color: #34495e; padding: 8px 0px;");
        label->setMinimumHeight(36);
        layout->addWidget(label);
    }

    // 评级样式增强
    gradeLabel->setFont(QFont("Microsoft YaHei", 18, QFont::Bold));
    if (grade == "S")
        gradeLabel->setStyleSheet("color: #e74c3c; padding: 8px 0px; font-weight: bold;");
    else if (grade == "A")
        gradeLabel->setStyleSheet("color: #f39c12; padding: 8px 0px; font-weight: bold;");
    else if (grade == "B")
        gradeLabel->setStyleSheet("color: #3498db; padding: 8px 0px; font-weight: bold;");
    else
        gradeLabel->setStyleSheet("color: #95a5a6; padding: 8px 0px; font-weight: bold;");

    layout->addSpacing(12);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(20);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    QPushButton *restartButton = new QPushButton("重新开始", resultPanel);
    QPushButton *menuButton = new QPushButton("返回主菜单", resultPanel);

    restartButton->setMinimumHeight(48);
    menuButton->setMinimumHeight(48);
    restartButton->setMinimumWidth(150);
    menuButton->setMinimumWidth(150);

    restartButton->setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
    menuButton->setFont(QFont("Microsoft YaHei", 14, QFont::Bold));

    restartButton->setStyleSheet(
        "QPushButton {"
        "   color: white;"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #27ae60, stop:1 #229954);"
        "   border-radius: 10px;"
        "   padding: 8px;"
        "   border: 2px solid #1e8449;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2ecc71, stop:1 #27ae60);"
        "}"
        "QPushButton:pressed {"
        "   background-color: #1e8449;"
        "   border: 2px solid #145a32;"
        "}");

    menuButton->setStyleSheet(
        "QPushButton {"
        "   color: white;"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #e74c3c, stop:1 #cb4335);"
        "   border-radius: 10px;"
        "   padding: 8px;"
        "   border: 2px solid #c0392b;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ec7063, stop:1 #e74c3c);"
        "}"
        "QPushButton:pressed {"
        "   background-color: #c0392b;"
        "   border: 2px solid #a93226;"
        "}");

    buttonLayout->addWidget(restartButton);
    buttonLayout->addWidget(menuButton);
    layout->addLayout(buttonLayout);

    QPropertyAnimation *fadeIn = new QPropertyAnimation(effect, "opacity", resultPanel);
    fadeIn->setDuration(GameConfig::RESULT_PANEL_ANIM_DURATION_MS);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);

    QRect startRect = resultPanel->geometry();
    int dw = startRect.width() / 8;
    int dh = startRect.height() / 8;
    QRect smallRect(startRect.adjusted(dw, dh, -dw, -dh));

    resultPanel->setGeometry(smallRect);

    QPropertyAnimation *scaleAnim = new QPropertyAnimation(resultPanel, "geometry", resultPanel);
    scaleAnim->setDuration(GameConfig::RESULT_PANEL_ANIM_DURATION_MS);
    scaleAnim->setStartValue(smallRect);
    scaleAnim->setEndValue(startRect);
    scaleAnim->setEasingCurve(QEasingCurve::OutBack);

    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    scaleAnim->start(QAbstractAnimation::DeleteWhenStopped);

    connect(restartButton, &QPushButton::clicked, this, [this]()
            {
                if (resultOverlay)
                {
                    resultOverlay->deleteLater();
                    resultOverlay = nullptr;
                    resultPanel = nullptr;
                }
                resetGame();
                startGame();
            });

    connect(menuButton, &QPushButton::clicked, this, [this]()
            {
                if (resultOverlay)
                {
                    resultOverlay->deleteLater();
                    resultOverlay = nullptr;
                    resultPanel = nullptr;
                }
                emit gameOver();
            });

    resultOverlay->show();
    resultPanel->show();
}

void GamePage::showLevelCompleteDialog()
{
    if (gameManager)
    {
        pauseAllEnemies();
        pauseAllTowersAndBullets();
    }

    if (resultOverlay)
    {
        resultOverlay->deleteLater();
        resultOverlay = nullptr;
        resultPanel = nullptr;
    }

    // 移除任何图形效果以防止 painter 冲突
    setGraphicsEffect(nullptr);
    update();
    repaint();

    saveLevelProgress(true);

    resultOverlay = new QWidget(this);
    resultOverlay->setGeometry(0, 0, width(), height());
    resultOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 180);");
    resultOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    resultPanel = new QWidget(resultOverlay);
    resultPanel->setFixedSize(500, 420);
    resultPanel->setStyleSheet(
        "QWidget {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #ffffff, stop:1 #f5f5f5);"
        "   border-radius: 20px;"
        "   border: 1px solid #2ecc71;"
        "}"
    );

    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(12);
    shadowEffect->setColor(QColor(39, 174, 96, 200));
    shadowEffect->setOffset(0, 2);
    resultPanel->setGraphicsEffect(shadowEffect);
    resultPanel->move((width() - resultPanel->width()) / 2, (height() - resultPanel->height()) / 2);

    QWidget *animContainer = new QWidget(resultPanel);
    animContainer->setGeometry(resultPanel->rect());
    animContainer->lower();

    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(animContainer);
    animContainer->setGraphicsEffect(effect);
    effect->setOpacity(0.0);

    QVBoxLayout *layout = new QVBoxLayout(resultPanel);
    layout->setContentsMargins(36, 48, 36, 48);
    layout->setSpacing(20);

    QLabel *titleLabel = new QLabel("胜利！", resultPanel);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setFont(QFont("Microsoft YaHei", 32, QFont::Bold));
    titleLabel->setStyleSheet("color: #27ae60;");
    titleLabel->setMinimumHeight(48);
    layout->addWidget(titleLabel);

    int wave = gameManager ? gameManager->getCurrentWave() : 1;
    int kill = gameManager ? gameManager->getKillCount() : 0;
    int gold = gameManager ? gameManager->getGold() : 0;
    int mapIndex = static_cast<int>(currentMapId) + 1;

    int score = kill * GameConfig::SCORE_PER_KILL +
                wave * GameConfig::SCORE_PER_WAVE +
                gold;

    QLabel *levelLabel = new QLabel(QString("当前关卡：第 %1 关").arg(mapIndex), resultPanel);
    QLabel *waveLabel = new QLabel(QString("防守波次：第 %1 波").arg(wave), resultPanel);
    QLabel *killLabel = new QLabel(QString("击败敌人数量：%1").arg(kill), resultPanel);
    QLabel *goldLabel = new QLabel(QString("剩余金币：%1").arg(gold), resultPanel);
    QLabel *scoreLabel = new QLabel(QString("总得分：%1").arg(score), resultPanel);

    for (QLabel *label : {levelLabel, waveLabel, killLabel, goldLabel, scoreLabel})
    {
        label->setAlignment(Qt::AlignCenter);
        label->setFont(QFont("Microsoft YaHei", 16, QFont::Normal));
        label->setStyleSheet("color: #34495e; padding: 8px 0px;");
        label->setMinimumHeight(36);
        layout->addWidget(label);
    }

    layout->addSpacing(12);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(20);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    QPushButton *menuButton = new QPushButton("返回主菜单", resultPanel);
    QPushButton *restartButton = new QPushButton("重新开始", resultPanel);

    QList<QPushButton *> buttons = {menuButton, restartButton};
    for (QPushButton *btn : buttons)
    {
        btn->setMinimumHeight(48);
        btn->setMinimumWidth(130);
        btn->setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
    }

    menuButton->setStyleSheet(
        "QPushButton {"
        "   color: white;"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #e74c3c, stop:1 #cb4335);"
        "   border-radius: 10px;"
        "   padding: 8px;"
        "   border: 2px solid #c0392b;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ec7063, stop:1 #e74c3c);"
        "}"
        "QPushButton:pressed {"
        "   background-color: #c0392b;"
        "   border: 2px solid #a93226;"
        "}");

    restartButton->setStyleSheet(
        "QPushButton {"
        "   color: white;"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #27ae60, stop:1 #229954);"
        "   border-radius: 10px;"
        "   padding: 8px;"
        "   border: 2px solid #1e8449;"
        "}"
        "QPushButton:hover {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2ecc71, stop:1 #27ae60);"
        "}"
        "QPushButton:pressed {"
        "   background-color: #1e8449;"
        "   border: 2px solid #145a32;"
        "}");

    buttonLayout->addWidget(menuButton);
    buttonLayout->addWidget(restartButton);
    layout->addLayout(buttonLayout);

    QPropertyAnimation *fadeIn = new QPropertyAnimation(effect, "opacity", resultPanel);
    fadeIn->setDuration(GameConfig::RESULT_PANEL_ANIM_DURATION_MS);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);

    QRect startRect = resultPanel->geometry();
    int dw = startRect.width() / 8;
    int dh = startRect.height() / 8;
    QRect smallRect(startRect.adjusted(dw, dh, -dw, -dh));

    resultPanel->setGeometry(smallRect);

    QPropertyAnimation *scaleAnim = new QPropertyAnimation(resultPanel, "geometry", resultPanel);
    scaleAnim->setDuration(GameConfig::RESULT_PANEL_ANIM_DURATION_MS);
    scaleAnim->setStartValue(smallRect);
    scaleAnim->setEndValue(startRect);
    scaleAnim->setEasingCurve(QEasingCurve::OutBack);

    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    scaleAnim->start(QAbstractAnimation::DeleteWhenStopped);

    connect(menuButton, &QPushButton::clicked, this, [this]()
            {
                if (resultOverlay)
                {
                    resultOverlay->deleteLater();
                    resultOverlay = nullptr;
                    resultPanel = nullptr;
                }
                emit returnToMainMenu();
            });

    connect(restartButton, &QPushButton::clicked, this, [this]()
            {
                if (resultOverlay)
                {
                    resultOverlay->deleteLater();
                    resultOverlay = nullptr;
                    resultPanel = nullptr;
                }
                resetGame();
                startGame();
            });

    resultOverlay->show();
    resultPanel->show();
}

void GamePage::saveLevelProgress(bool levelCompleted)
{
    if (!gameManager)
        return;

    int wave = gameManager->getCurrentWave();

    // 使用组织名和应用名来初始化 QSettings
    QSettings settings("TowerDefenseStudio", "TowerDefenseGame");

    QString mapKey = QString("levels/map_%1").arg(static_cast<int>(currentMapId));
    int bestWave = settings.value(mapKey + "/bestWave", 0).toInt();
    qDebug() << "[LevelProgress] Current wave:" << wave << "Best wave before:" << bestWave;
    
    if (wave > bestWave)
    {
        settings.setValue(mapKey + "/bestWave", wave);
        qDebug() << "[LevelProgress] Update bestWave for" << mapKey << "to" << wave;
    }

    if (levelCompleted)
    {
        int unlockedMaxIndex = settings.value("levels/unlocked_max_index", 0).toInt();
        int currentIndex = static_cast<int>(currentMapId);
        qDebug() << "[LevelProgress] levelCompleted on map index" << currentIndex
                 << "current unlocked_max_index =" << unlockedMaxIndex;

        int nextIndex = currentIndex + 1;
        int maxIndex = static_cast<int>(GameConfig::MAP2);

        if (nextIndex <= maxIndex && nextIndex > unlockedMaxIndex)
        {
            settings.setValue("levels/unlocked_max_index", nextIndex);
            qDebug() << "[LevelProgress] Unlock next level index" << nextIndex;
        }
        else
        {
            qDebug() << "[LevelProgress] No new level to unlock. nextIndex =" << nextIndex
                     << "maxIndex =" << maxIndex;
        }
    }

    // 立即同步数据到存储
    settings.sync();
    
    // 验证数据是否正确保存
    QSettings verifySettings("TowerDefenseStudio", "TowerDefenseGame");
    int verifyUnlocked = verifySettings.value("levels/unlocked_max_index", -1).toInt();
    int verifyWave = verifySettings.value(QString("levels/map_%1").arg(static_cast<int>(currentMapId)) + "/bestWave", -1).toInt();
    qDebug() << "[LevelProgress] Verify after save - unlocked_max_index:" << verifyUnlocked << "bestWave:" << verifyWave;
}

void GamePage::mousePressEvent(QMouseEvent *event)
{
    if (!gameManager || !gameManager->isGameRunning() || gameManager->isPaused())
        return; // 暂停时直接返回，不处理鼠标点击

    // 将鼠标点击位置从视图坐标转换到场景坐标
    QPoint viewPos = event->pos();

    // 计算view相对于GamePage的位置
    QPoint viewGlobalPos = gameView->mapFromParent(viewPos);

    // 确保点击在view范围内
    if (!gameView->rect().contains(viewGlobalPos))
    {
        return;
    }

    // 转换到场景坐标
    QPointF scenePos = gameView->mapToScene(viewGlobalPos);

    qDebug() << "Mouse click - Widget pos:" << event->pos()
             << "View pos:" << viewGlobalPos
             << "Scene pos:" << scenePos;

    // 计算网格位置（对齐到网格）
    int gridSize = GameConfig::GRID_SIZE;
    int gridX = int(scenePos.x() / gridSize) * gridSize;
    int gridY = int(scenePos.y() / gridSize) * gridSize;

    // 确保在有效范围内
    if (gridX < 0 || gridY < 0 || gridX >= GameConfig::WINDOW_WIDTH || gridY >= GameConfig::WINDOW_HEIGHT)
    {
        qDebug() << "Click outside valid area";
        return;
    }

    // 检查是否在路径上（简化检查）
    bool onPath = false;
    for (const QPointF &point : pathPoints)
    {
        qreal dx = point.x() - scenePos.x();
        qreal dy = point.y() - scenePos.y();
        if (dx * dx + dy * dy < 900)
        { // 30像素半径内
            onPath = true;
            break;
        }
    }

    if (onPath)
    {
        qDebug() << "Cannot build on path";
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        // Check placement validity
        if (placementValidator && !placementValidator->isPlacementAllowed(gridX, gridY))
        {
             showFloatingTip("此处禁止放置!", scenePos, Qt::red);
             return;
        }

        bool towerExists = false;
        for (QPointer<Tower> tower : gameManager->getTowers())
        {
            if (tower &&
                qAbs(tower->x() - gridX) < gridSize / 2 &&
                qAbs(tower->y() - gridY) < gridSize / 2)
            {
                towerExists = true;
                qDebug() << "Tower already exists at (" << gridX << "," << gridY << ")";
                break;
            }
        }

        if (!towerExists)
        {
            int cost = GameConfig::TowerStats::ARROW_COST;
            if (gameManager->getGold() < cost)
            {
                qDebug() << "Not enough gold to build tower";
                showFloatingTip("金币不足!", scenePos, Qt::red);
                QWidget::mousePressEvent(event);
                return;
            }

            QPointer<Tower> tower = gameManager->buildTower(Tower::ARROW_TOWER, QPointF(gridX, gridY), this);
            if (!tower)
            {
                showFloatingTip("金币不足!", scenePos, Qt::red);
                QWidget::mousePressEvent(event);
                return;
            }

            tower->setGameScene(gameScene);

            QGraphicsRectItem *highlight = new QGraphicsRectItem(gridX, gridY, gridSize, gridSize);
            highlight->setBrush(QBrush(QColor(255, 255, 0, 100)));
            highlight->setPen(QPen(Qt::NoPen));
            gameScene->addItem(highlight);

            QTimer::singleShot(GameConfig::HIGHLIGHT_EFFECT_DURATION_MS, [highlight]()
                               {
                if (highlight->scene()) {
                    highlight->scene()->removeItem(highlight);
                    delete highlight;
                } });
        }
    }
    else if (event->button() == Qt::RightButton)
    {
        QList<QGraphicsItem *> itemsAtPos = gameScene->items(scenePos);
        Tower *clickedTower = nullptr;
        for (QGraphicsItem *item : itemsAtPos)
        {
            Tower *tower = dynamic_cast<Tower *>(item);
            if (tower)
            {
                clickedTower = tower;
                break;
            }
        }

        if (!clickedTower)
        {
            qDebug() << "Right click at empty grid (" << gridX << "," << gridY << ")";
            QWidget::mousePressEvent(event);
            return;
        }

        Tower::TowerType type = clickedTower->getTowerType();
        int currentCost = clickedTower->getCost();
        Tower::TowerType nextType = type;
        bool hasNext = false;
        switch (type)
        {
        case Tower::ARROW_TOWER:
            nextType = Tower::CANNON_TOWER;
            hasNext = true;
            break;
        case Tower::CANNON_TOWER:
            nextType = Tower::MAGIC_TOWER;
            hasNext = true;
            break;
        case Tower::MAGIC_TOWER:
            hasNext = false;
            break;
        }

        int extraCost = 0;
        QString upgradeText;
        if (hasNext)
        {
            int nextCost = 0;
            switch (nextType)
            {
            case Tower::ARROW_TOWER:
                nextCost = GameConfig::TowerStats::ARROW_COST;
                break;
            case Tower::CANNON_TOWER:
                nextCost = GameConfig::TowerStats::CANNON_COST;
                break;
            case Tower::MAGIC_TOWER:
                nextCost = GameConfig::TowerStats::MAGIC_COST;
                break;
            }
            extraCost = nextCost - currentCost;
            if (extraCost < 0)
                extraCost = 0;

            QString nextName;
            if (nextType == Tower::CANNON_TOWER)
                nextName = "炮塔";
            else if (nextType == Tower::MAGIC_TOWER)
                nextName = "魔法塔";
            else
                nextName = "防御塔";

            upgradeText = QString("升级为%1 (-%2 金币)").arg(nextName).arg(extraCost);
        }

        int refund = currentCost * GameConfig::TOWER_SELL_REFUND_PERCENT / 100;

        QMenu menu(this);
        menu.setWindowOpacity(0.9);

        QAction *upgradeAction = nullptr;
        if (hasNext)
        {
            upgradeAction = menu.addAction(upgradeText);
        }
        QAction *sellAction = menu.addAction(QString("拆除 (返还 %1 金币)").arg(refund));
        QAction *cancelAction = menu.addAction("取消");

        QAction *selected = menu.exec(mapToGlobal(event->pos()));
        if (!selected || selected == cancelAction)
        {
            QWidget::mousePressEvent(event);
            return;
        }

        if (selected == upgradeAction && hasNext)
        {
            if (extraCost > 0 && gameManager->getGold() < extraCost)
            {
                showFloatingTip("金币不足!", scenePos, Qt::red);
                QApplication::beep();
                QWidget::mousePressEvent(event);
                return;
            }

            QPointer<Tower> newTower = gameManager->upgradeTower(clickedTower);
            if (!newTower)
            {
                if (!hasNext)
                {
                    showFloatingTip("已是最高级塔", scenePos, Qt::yellow);
                }
                else
                {
                    showFloatingTip("升级失败", scenePos, Qt::red);
                }
                QWidget::mousePressEvent(event);
                return;
            }

            newTower->setGameScene(gameScene);
            showUpgradeEffect(newTower->pos());
            QApplication::beep();
        }
        else if (selected == sellAction)
        {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "确认拆除",
                QString("确定要拆除该防御塔吗？\n将返还 %1 金币。").arg(refund),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            if (reply == QMessageBox::Yes)
            {
                bool ok = gameManager->demolishTower(clickedTower);
                if (ok)
                {
                    showFloatingTip(QString("已返还 %1 金币").arg(refund), scenePos, Qt::green);
                    QApplication::beep();
                }
            }
        }

        QWidget::mousePressEvent(event);
        return;
    }

    QWidget::mousePressEvent(event);
}

void GamePage::mouseMoveEvent(QMouseEvent *event)
{
    if (!gameManager || !gameManager->isGameRunning() || gameManager->isPaused())
        return;

    QPoint viewPos = event->pos();
    QPoint viewGlobalPos = gameView->mapFromParent(viewPos);

    if (gameView->rect().contains(viewGlobalPos))
    {
        QPointF scenePos = gameView->mapToScene(viewGlobalPos);
        updateHoverHighlight(scenePos);
    }
    else
    {
        updateHoverHighlight(QPointF(-1, -1));
    }

    QWidget::mouseMoveEvent(event);
}

bool GamePage::eventFilter(QObject *obj, QEvent *event)
{
    // 暂停时不处理鼠标移动
    if (gameManager && gameManager->isPaused())
        return QObject::eventFilter(obj, event);

    if (obj == gameView->viewport() && event->type() == QEvent::MouseMove)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QPoint viewPos = mouseEvent->pos();
        QPointF scenePos = gameView->mapToScene(viewPos);
        updateHoverHighlight(scenePos);
    }
    return QObject::eventFilter(obj, event);
}

void GamePage::updateHoverHighlight(const QPointF &scenePos)
{
    static QGraphicsRectItem *lastHighlight = nullptr;
    static int lastGridX = -1;
    static int lastGridY = -1;

    int gridSize = GameConfig::GRID_SIZE;

    if (scenePos.x() < 0 || scenePos.y() < 0 ||
        scenePos.x() >= GameConfig::WINDOW_WIDTH ||
        scenePos.y() >= GameConfig::WINDOW_HEIGHT)
    {
        if (lastHighlight)
        {
            gameScene->removeItem(lastHighlight);
            delete lastHighlight;
            lastHighlight = nullptr;
        }
        lastGridX = lastGridY = -1;
        return;
    }

    int gridX = int(scenePos.x() / gridSize) * gridSize;
    int gridY = int(scenePos.y() / gridSize) * gridSize;

    if (gridX < 0 || gridY < 0 ||
        gridX >= GameConfig::WINDOW_WIDTH ||
        gridY >= GameConfig::WINDOW_HEIGHT)
    {
        if (lastHighlight)
        {
            gameScene->removeItem(lastHighlight);
            delete lastHighlight;
            lastHighlight = nullptr;
        }
        lastGridX = lastGridY = -1;
        return;
    }

    if (lastHighlight && gridX == lastGridX && gridY == lastGridY)
        return;

    if (lastHighlight)
    {
        gameScene->removeItem(lastHighlight);
        delete lastHighlight;
        lastHighlight = nullptr;
    }

    lastHighlight = new QGraphicsRectItem(gridX, gridY, gridSize, gridSize);
    lastHighlight->setBrush(QBrush(QColor(255, 255, 255, 30)));
    lastHighlight->setPen(QPen(QColor(255, 255, 255, 100), 2));
    lastHighlight->setZValue(1000);
    gameScene->addItem(lastHighlight);

    lastGridX = gridX;
    lastGridY = gridY;

    qDebug() << "Hover highlight at grid (" << gridX << "," << gridY << ")";
}

void GamePage::pauseAllEnemies()
{
    if (!gameManager)
        return;

    for (QPointer<Enemy> enemy : gameManager->getEnemies())
    {
        if (enemy)
        {
            enemy->pauseMovement();
        }
    }
}

void GamePage::resumeAllEnemies()
{
    if (!gameManager)
        return;

    for (QPointer<Enemy> enemy : gameManager->getEnemies())
    {
        if (enemy)
        {
            enemy->resumeMovement();
        }
    }
}

void GamePage::pauseAllTowersAndBullets()
{
    if (!gameManager)
        return;

    for (QPointer<Tower> tower : gameManager->getTowers())
    {
        if (tower)
        {
            tower->pauseAttack();
        }
    }

    // 暂停所有子弹的移动
    // 使用gameScene->items()查找所有Bullet对象
    for (QGraphicsItem *item : gameScene->items())
    {
        Bullet *bullet = dynamic_cast<Bullet *>(item);
        if (bullet)
        {
            bullet->pauseMovement();
        }
    }
}

void GamePage::resumeAllTowersAndBullets()
{
    if (!gameManager)
        return;

    for (QPointer<Tower> tower : gameManager->getTowers())
    {
        if (tower)
        {
            tower->resumeAttack();
        }
    }

    // 恢复所有子弹的移动
    // 使用gameScene->items()查找所有Bullet对象
    for (QGraphicsItem *item : gameScene->items())
    {
        Bullet *bullet = dynamic_cast<Bullet *>(item);
        if (bullet)
        {
            bullet->resumeMovement();
        }
    }
}

void GamePage::showPauseMenu()
{
    if (pauseOverlay) return;

    // ===== 创建灰色半透明遮罩 =====
    pauseOverlay = new QWidget(this);
    pauseOverlay->setGeometry(0, 0, width(), height());
    pauseOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 180);");
    pauseOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, false); // 阻止游戏场景操作

    // ===== 创建面板 =====
    pausePanel = new QWidget(pauseOverlay);
    pausePanel->setFixedSize(400, 300);
    pausePanel->move((width() - pausePanel->width()) / 2, (height() - pausePanel->height()) / 2);
    pausePanel->setStyleSheet(
        "QWidget {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #ecf0f1, stop:1 #bdc3c7);"
        "  border-radius: 20px;"
        "  border: 2px solid #7f8c8d;"
        "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(pausePanel);
    layout->setContentsMargins(36, 48, 36, 48);
    layout->setSpacing(20);

    // ===== 标题 =====
    QLabel *title = new QLabel("游戏已暂停", pausePanel);
    title->setFont(QFont("Microsoft YaHei", 24, QFont::Bold));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("color: #2c3e50;");
    layout->addWidget(title);

    layout->addSpacing(20);

    // ===== 创建按钮 =====
    QPushButton *resumeButton = new QPushButton("继续游戏", pausePanel);
    QPushButton *restartButton = new QPushButton("重新开始", pausePanel);
    QPushButton *exitButton = new QPushButton("退出游戏", pausePanel);

    for (QPushButton *btn : {resumeButton, restartButton, exitButton})
    {
        btn->setMinimumHeight(48);
        btn->setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
        btn->setStyleSheet(
            "QPushButton {"
            "  color: white;"
            "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3498db, stop:1 #2980b9);"
            "  border-radius: 10px;"
            "  padding: 8px;"
            "  border: 2px solid #1f618d;"
            "}"
            "QPushButton:hover {"
            "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2980b9, stop:1 #3498db);"
            "  border: 2px solid #154360;"
            "}"
            "QPushButton:pressed {"
            "  background-color: #21618c;"
            "  border: 2px solid #0e3a5e;"
            "}"
        );
        layout->addWidget(btn);
    }

    // ===== 按钮事件 =====
    connect(resumeButton, &QPushButton::clicked, this, [this]()
            {
                if (pauseOverlay)
                {
                    pauseOverlay->deleteLater();
                    pauseOverlay = nullptr;
                    pausePanel = nullptr;
                }
                if (gameManager)
                {
                    gameManager->pauseGame(); // 再次调用以恢复
                    resumeAllEnemies();
                    resumeAllTowersAndBullets();
                    pauseButton->setText("暂停");
                }
            });

    connect(restartButton, &QPushButton::clicked, this, [this]()
            {
                if (pauseOverlay)
                {
                    pauseOverlay->deleteLater();
                    pauseOverlay = nullptr;
                    pausePanel = nullptr;
                }
                resetGame();
                startGame();
            });

    connect(exitButton, &QPushButton::clicked, this, [this]()
            {
                qApp->quit();
            });

    pauseOverlay->show();
    pausePanel->show();
}

void GamePage::hidePauseMenu()
{
    if (pauseOverlay)
    {
        pauseOverlay->deleteLater();
        pauseOverlay = nullptr;
        pausePanel = nullptr;
    }
}
