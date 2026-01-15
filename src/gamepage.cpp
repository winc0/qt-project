#include "include/gamepage.h"
#include "include/resourcemanager.h"
#include "include/config.h"
#include "include/mainwindow.h"

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
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QColor>
#include <cmath>
#include "include/placementvalidator.h"

GamePage::GamePage(QWidget *parent)
    : QWidget(parent),
      gameScene(nullptr),
      gameView(nullptr),
      gold(GameConfig::INITIAL_GOLD),
      lives(GameConfig::INITIAL_LIVES),
      currentWave(1),
      waveSpawnComplete(false),
      enemiesSpawnedThisWave(0),
      isPaused(false),
      gameRunning(false),
      currentMapId(GameConfig::MAP_DEFAULT),
      resultOverlay(nullptr),
      resultPanel(nullptr),
      placementValidator(nullptr),
      killCount(0)
{
    qDebug() << "GamePage constructor called";

    // 设置固定大小
    setFixedSize(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

    initUI();
    initGameScene();
    createPath();
    initPlacementValidator();

    qDebug() << "GamePage initialized, size:" << size();
}

GamePage::~GamePage()
{
    qDeleteAll(placementAreaItems);
    placementAreaItems.clear();
    delete placementValidator;
    resetGame();
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

    // 金币数量
    goldLabel = new QLabel(QString("%1").arg(gold), controlPanel);
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

    // 生命数量
    livesLabel = new QLabel(QString("%1").arg(lives), controlPanel);
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

    // 波次数值
    waveLabel = new QLabel(QString("第 %1 波").arg(currentWave), controlPanel);
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

    // 创建定时器
    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &GamePage::updateGame);

    enemySpawnTimer = new QTimer(this);
    connect(enemySpawnTimer, &QTimer::timeout, this, &GamePage::spawnEnemy);

    qDebug() << "Game scene initialized, view size:" << gameView->size();
}

void GamePage::drawBackground()
{
    ResourceManager &rm = ResourceManager::instance();
    QPixmap background = rm.getGameMap();

    // 创建背景图形项 - 占据整个800x600
    QGraphicsPixmapItem *backgroundItem = new QGraphicsPixmapItem(background);
    backgroundItem->setZValue(-100); // 最底层
    gameScene->addItem(backgroundItem);

    // 绘制路径
    if (!pathPoints.isEmpty())
    {
        QPainterPath path;
        path.moveTo(pathPoints.first());

        for (int i = 1; i < pathPoints.size(); ++i)
        {
            path.lineTo(pathPoints[i]);
        }

        QGraphicsPathItem *pathItem = new QGraphicsPathItem(path);
        pathItem->setPen(QPen(QColor(139, 69, 19, 150), 30)); // 棕色半透明路径
        pathItem->setZValue(-50);
        gameScene->addItem(pathItem);
    }
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

void GamePage::createPath()
{
    // 根据地图ID获取路径
    QVector<GameConfig::GridPoint> gridPoints = GameConfig::MapPaths::PATH_MAP.value(currentMapId, GameConfig::MapPaths::DEFAULT_PATH);
    
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
    if (gameRunning)
        return;

    killCount = 0;
    elapsedTimer.restart();

    gameRunning = true;
    isPaused = false;
    pauseButton->setText("暂停");

    // 启动游戏循环
    gameTimer->start(16); // 约60FPS

    // 启动敌人生成器，根据当前波次设置间隔
    enemySpawnTimer->start(getWaveSpawnInterval());

    qDebug() << "游戏开始!";
}

void GamePage::pauseGame()
{
    if (!gameRunning)
        return;

    if (isPaused)
    {
        // 恢复游戏
        gameTimer->start(16);
        enemySpawnTimer->start(getWaveSpawnInterval());
        resumeAllEnemies();
        resumeAllTowersAndBullets();
        pauseButton->setText("暂停");
        isPaused = false;
        qDebug() << "游戏继续";
    }
    else
    {
        // 暂停游戏
        gameTimer->stop();
        enemySpawnTimer->stop();
        pauseAllEnemies();
        pauseAllTowersAndBullets();
        pauseButton->setText("继续");
        isPaused = true;
        qDebug() << "游戏暂停";
    }
}

void GamePage::resetGame()
{
    // 停止所有定时器
    if (gameTimer && gameTimer->isActive())
        gameTimer->stop();
    if (enemySpawnTimer && enemySpawnTimer->isActive())
        enemySpawnTimer->stop();

    // 清理所有敌人
    for (QPointer<Enemy> enemy : enemies)
    {
        if (enemy)
        {
            gameScene->removeItem(enemy);
            enemy->deleteLater();
        }
    }
    enemies.clear();

    // 清理所有防御塔
    for (QPointer<Tower> tower : towers)
    {
        if (tower)
        {
            gameScene->removeItem(tower);
            tower->deleteLater();
        }
    }
    towers.clear();

    // 重置游戏状态
    gold = GameConfig::INITIAL_GOLD;
    lives = GameConfig::INITIAL_LIVES;
    currentWave = 1;
    enemiesSpawnedThisWave = 0;
    waveSpawnComplete = false;
    gameRunning = false;
    isPaused = false;
    killCount = 0;

    // 更新UI显示
    goldLabel->setText(QString::number(gold));
    livesLabel->setText(QString::number(lives));
    waveLabel->setText(QString("第 %1 波").arg(currentWave));

    qDebug() << "游戏重置";
}

void GamePage::spawnEnemy()
{
    if (!gameRunning || isPaused)
        return;

    // 检查当前波次是否已经生成足够敌人
    if (enemiesSpawnedThisWave >= GameConfig::WAVE_ENEMY_COUNT * currentWave)
    {
        // 只在第一次生成够了敌人时记录
        if (!waveSpawnComplete)
        {
            waveSpawnComplete = true;
            qDebug() << "第" << currentWave << "波敌人已全部生成";
        }
        return;
    }

    // 创建新敌人
    QPointer<Enemy> enemy = new Enemy(0, this); // 类型0:基础敌人
    enemy->setPath(pathPoints);

    // 添加到场景和列表
    gameScene->addItem(enemy);
    enemies.append(enemy);
    enemiesSpawnedThisWave++;

    // 连接敌人信号
    // 注意：需要在Enemy类中添加reachedEndPoint信号

    qDebug() << "生成敌人，当前敌人数量:" << enemies.size()
             << "，本波已生成:" << enemiesSpawnedThisWave;
}

void GamePage::updateGame()
{
    if (!gameRunning || isPaused)
        return;

    // 更新所有敌人
    updateEnemies();

    // 更新所有防御塔
    updateTowers();

    // 清理死亡实体
    removeDeadEntities();

    // 检查是否应该进入下一波
    checkNextWave();

    // 检查游戏结束
    checkGameOver();
}

void GamePage::updateEnemies()
{
    QList<QPointer<Enemy>> enemiesToRemove;

    for (QPointer<Enemy> enemy : enemies)
    {
        if (!enemy)
            continue;

        enemy->update();

        if (isEnemyAtAnyEndPoint(enemy))
        {
            // 敌人到达终点，扣减生命值
            lives--;
            livesLabel->setText(QString::number(lives));

            enemiesToRemove.append(enemy);
            gameScene->removeItem(enemy);

            qDebug() << "敌人到达终点，剩余生命:" << lives;
        }
    }

    // 移除到达终点的敌人
    for (QPointer<Enemy> enemy : enemiesToRemove)
    {
        enemies.removeOne(enemy);
        enemy->deleteLater();
    }
}

bool GamePage::isEnemyAtAnyEndPoint(QPointer<Enemy> enemy) const
{
    if (!enemy || endPointAreas.isEmpty())
        return false;

    QPointF enemyCenter = enemy->getCenterPosition();
    for (const GameConfig::EndPointConfig &end : endPointAreas)
    {
        qreal dx = enemyCenter.x() - end.x;
        qreal dy = enemyCenter.y() - end.y;
        qreal distance = std::sqrt(dx * dx + dy * dy);
        if (distance <= end.radius)
        {
            return true;
        }
    }
    return false;
}

void GamePage::updateTowers()
{
    // 更新每个防御塔的敌人列表
    for (QPointer<Tower> tower : towers)
    {
        if (!tower)
            continue;

        // 收集范围内的敌人
        QList<QPointer<Enemy>> enemiesInRange;
        for (QPointer<Enemy> enemy : enemies)
        {
            if (!enemy)
                continue;

            qreal dx = enemy->x() - tower->x();
            qreal dy = enemy->y() - tower->y();
            qreal distance = std::sqrt(dx * dx + dy * dy);

            if (distance <= tower->getRange())
            {
                enemiesInRange.append(enemy);
            }
        }

        // 设置防御塔的攻击目标
        tower->setEnemiesInRange(enemiesInRange);
        tower->update();
    }
}

void GamePage::removeDeadEntities()
{
    // 移除死亡的敌人
    QList<QPointer<Enemy>> deadEnemies;

    for (QPointer<Enemy> enemy : enemies)
    {
        if (!enemy)
            continue;

        if (enemy->getHealth() <= 0)
        {
            killCount++;
            gold += enemy->getReward();
            goldLabel->setText(QString::number(gold));

            // 设置敌人为死亡状态
            enemy->setState(ResourceManager::ENEMY_DEAD);

            deadEnemies.append(enemy);

            qDebug() << "敌人死亡，获得金币:" << enemy->getReward()
                     << "，当前金币:" << gold;
        }
    }

    // 延迟0.5秒后移除敌人实体
    for (QPointer<Enemy> enemy : deadEnemies)
    {
        enemies.removeOne(enemy);
        QTimer::singleShot(GameConfig::ENEMY_DEAD_KEEP_TIME, [this, enemy]()
                           {
            if (enemy && gameScene)
            {
                gameScene->removeItem(enemy);
                enemy->deleteLater();
            } });
    }
}

void GamePage::checkGameOver()
{
    if (lives <= 0)
    {
        gameTimer->stop();
        enemySpawnTimer->stop();
        gameRunning = false;
        pauseAllEnemies();
        pauseAllTowersAndBullets();
        showGameOverDialog();
    }
}

void GamePage::showGameOverDialog()
{
    if (resultOverlay)
    {
        resultOverlay->deleteLater();
        resultOverlay = nullptr;
        resultPanel = nullptr;
    }

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

    int score = killCount * 10 + currentWave * 50 + gold;
    QString grade;
    if (score >= 1200)
        grade = "S";
    else if (score >= 800)
        grade = "A";
    else if (score >= 400)
        grade = "B";
    else
        grade = "C";

    // 数据标签
    QLabel *killLabel = new QLabel(QString("击杀敌人数量：%1").arg(killCount), resultPanel);
    QLabel *timeLabel = new QLabel(QString("游戏时长：%1 秒").arg(seconds), resultPanel);
    QLabel *waveLabel = new QLabel(QString("到达波次：第 %1 波").arg(currentWave), resultPanel);
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
    fadeIn->setDuration(300);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);

    QRect startRect = resultPanel->geometry();
    int dw = startRect.width() / 8;
    int dh = startRect.height() / 8;
    QRect smallRect(startRect.adjusted(dw, dh, -dw, -dh));

    resultPanel->setGeometry(smallRect);

    QPropertyAnimation *scaleAnim = new QPropertyAnimation(resultPanel, "geometry", resultPanel);
    scaleAnim->setDuration(300);
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

void GamePage::checkNextWave()
{
    // 如果当前波次已经生成了所有敌人，并且敌人列表为空，进入下一波
    if (waveSpawnComplete && enemies.isEmpty())
    {
        currentWave++;
        enemiesSpawnedThisWave = 0;
        waveSpawnComplete = false;
        waveLabel->setText(QString("第 %1 波").arg(currentWave));

        // 更新敌人生成定时器的间隔
        int newInterval = getWaveSpawnInterval();
        enemySpawnTimer->setInterval(newInterval);
        qDebug() << "进入第" << currentWave << "波，生成间隔已更新为:" << newInterval << "ms";
    }
}

int GamePage::getWaveSpawnInterval() const
{
    // 计算当前波次的敌人生成间隔
    // 公式: max(WAVE_SPAWN_INTERVAL_MAX - WAVE_SPAWN_INTERVAL_EACH*(currentWave-1), WAVE_SPAWN_INTERVAL_MIN)
    int interval = GameConfig::WAVE_SPAWN_INTERVAL_MAX - GameConfig::WAVE_SPAWN_INTERVAL_EACH * (currentWave - 1);
    int minInterval = GameConfig::WAVE_SPAWN_INTERVAL_MIN;

    // 确保间隔不低于最小值
    if (interval < minInterval)
    {
        interval = minInterval;
    }

    qDebug() << "第" << currentWave << "波的生成间隔:" << interval << "ms";
    return interval;
}

void GamePage::mousePressEvent(QMouseEvent *event)
{
    if (!gameRunning)
        return;

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

        // 检查是否已经有防御塔
        bool towerExists = false;
        for (QPointer<Tower> tower : towers)
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
            // 检查是否有足够金币建造防御塔
            if (gold >= 100)
            { // 箭塔价格100
                Tower *tower = new Tower(Tower::ARROW_TOWER, QPointF(gridX, gridY), this);
                tower->setPos(gridX, gridY);    // 显式设置位置
                tower->setGameScene(gameScene); // 设置gameScene，用于发射子弹
                towers.append(tower);

                // 添加底座层到场景
                QGraphicsPixmapItem *baseItem = tower->getBaseItem();
                if (baseItem)
                {
                    gameScene->addItem(baseItem);
                }

                gameScene->addItem(tower);

                // 扣除金币
                gold -= tower->getCost();
                goldLabel->setText(QString::number(gold));

                qDebug() << "Tower built at (" << gridX << "," << gridY
                         << "), cost:" << tower->getCost()
                         << ", gold remaining:" << gold;

                // 可选：添加建造动画或音效
                QGraphicsRectItem *highlight = new QGraphicsRectItem(gridX, gridY, gridSize, gridSize);
                highlight->setBrush(QBrush(QColor(255, 255, 0, 100)));
                highlight->setPen(QPen(Qt::NoPen));
                gameScene->addItem(highlight);

                // 淡出动画
                QTimer::singleShot(500, [highlight]()
                                   {
                    if (highlight->scene()) {
                        highlight->scene()->removeItem(highlight);
                        delete highlight;
                    } });
            }
            else
            {
                qDebug() << "Not enough gold to build tower";
                showFloatingTip("金币不足!", scenePos, Qt::red);
            }
        }
    }
    else if (event->button() == Qt::RightButton)
    {
        // 右键可以显示信息或取消选择
        qDebug() << "Right click at grid (" << gridX << "," << gridY << ")";
    }

    QWidget::mousePressEvent(event);
}

void GamePage::mouseMoveEvent(QMouseEvent *event)
{
    if (!gameRunning)
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
    // 暂停所有敌人的移动
    for (QPointer<Enemy> enemy : enemies)
    {
        if (enemy)
        {
            enemy->pauseMovement();
        }
    }
}

void GamePage::resumeAllEnemies()
{
    // 恢复所有敌人的移动（排除死亡状态）
    for (QPointer<Enemy> enemy : enemies)
    {
        if (enemy)
        {
            enemy->resumeMovement();
        }
    }
}

void GamePage::pauseAllTowersAndBullets()
{
    // 暂停所有塔的攻击
    for (QPointer<Tower> tower : towers)
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
    // 恢复所有塔的攻击
    for (QPointer<Tower> tower : towers)
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
