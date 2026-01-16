#include "include/gamemanager.h"
#include "include/resourcemanager.h"
#include "include/quadtree.h"

#include <cmath>
#include <QRandomGenerator>
#include <QDebug>

GameManager::GameManager(QObject *parent)
    : QObject(parent),
      gold(GameConfig::INITIAL_GOLD),
      lives(GameConfig::INITIAL_LIVES),
      currentWave(1),
      enemiesSpawnedThisWave(0),
      waveSpawnComplete(false),
      gameRunning(false),
      paused(false),
      killCount(0),
      currentMapId(GameConfig::MAP1),
      gameTimer(new QTimer(this)),
      enemySpawnTimer(new QTimer(this)),
      resourceManager(&ResourceManager::instance())
{
    connect(gameTimer, &QTimer::timeout, this, &GameManager::updateGame);
    connect(enemySpawnTimer, &QTimer::timeout, this, &GameManager::spawnEnemy);
}

void GameManager::initialize(GameConfig::MapId mapId,
                             const QVector<QPointF> &path,
                             const QVector<GameConfig::EndPointConfig> &endPoints)
{
    currentMapId = mapId;
    pathPoints = path;
    endPointAreas = endPoints;
}

void GameManager::startGame()
{
    if (gameRunning)
        return;

    killCount = 0;
    gameRunning = true;
    paused = false;

    gameTimer->start(GameConfig::GAME_TICK_INTERVAL_MS);
    enemySpawnTimer->start(getWaveSpawnInterval());

    emit gameStateChanged(gameRunning, paused);
}

void GameManager::pauseGame()
{
    if (!gameRunning)
        return;

    paused = !paused;

    if (paused)
    {
        gameTimer->stop();
        enemySpawnTimer->stop();
    }
    else
    {
        gameTimer->start(GameConfig::GAME_TICK_INTERVAL_MS);
        enemySpawnTimer->start(getWaveSpawnInterval());
    }

    emit gameStateChanged(gameRunning, paused);
}

void GameManager::resetGame()
{
    if (gameTimer->isActive())
        gameTimer->stop();
    if (enemySpawnTimer->isActive())
        enemySpawnTimer->stop();

    enemies.clear();
    towers.clear();
    bullets.clear();

    gold = GameConfig::INITIAL_GOLD;
    lives = GameConfig::INITIAL_LIVES;
    currentWave = 1;
    enemiesSpawnedThisWave = 0;
    waveSpawnComplete = false;
    gameRunning = false;
    paused = false;
    killCount = 0;

    emit goldChanged(gold);
    emit livesChanged(lives);
    emit waveChanged(currentWave);
    emit killCountChanged(killCount);
    emit gameStateChanged(gameRunning, paused);
}

void GameManager::spawnEnemy()
{
    if (!gameRunning || paused)
        return;

    if (enemiesSpawnedThisWave >= GameConfig::WAVE_ENEMY_COUNT)
    {
        if (!waveSpawnComplete)
        {
            waveSpawnComplete = true;
        }
        return;
    }

    int enemyType = 0;
    // 前面几波敌人固定
    if (currentWave >= 1 && currentWave <= GameConfig::ENEMY_TYPE_NUMBER)
    {
        enemyType = currentWave - 1;
    }
    // 后面几波敌人随机
    else
    {
        int index = QRandomGenerator::global()->bounded(GameConfig::ENEMY_TYPE_NUMBER);
        enemyType = index;
    }
    QPointer<Enemy> enemy = new Enemy(enemyType, this);
    int waveHealth = calculateWaveHealth();
    enemy->setMaxHealth(waveHealth);
    enemy->setHealth(waveHealth);
    float waveSpeed = calculateWaveSpeed();
    enemy->setSpeed(waveSpeed);
    enemy->setPath(pathPoints);

    enemies.append(enemy);
    enemiesSpawnedThisWave++;

    emit enemySpawnRequested(enemy);
}

void GameManager::updateGame()
{
    if (!gameRunning || paused)
        return;

    updateEnemies();
    updateTowers();
    removeDeadEntities();
    checkNextWave();

    if (lives <= 0)
    {
        gameTimer->stop();
        enemySpawnTimer->stop();
        gameRunning = false;
        paused = true;
        emit gameStateChanged(gameRunning, paused);
        emit gameOver();
        qDebug() << "updateGame() found Game over";
    }
}

void GameManager::updateEnemies()
{
    QList<QPointer<Enemy>> enemiesToRemove;

    for (QPointer<Enemy> enemy : enemies)
    {
        if (!enemy)
            continue;

        enemy->update();

        if (isEnemyAtAnyEndPoint(enemy))
        {
            lives--;
            emit livesChanged(lives);
            enemiesToRemove.append(enemy);
            emit enemyReachedEnd(enemy);
        }
    }

    for (QPointer<Enemy> enemy : enemiesToRemove)
    {
        enemies.removeOne(enemy);
    }
}

void GameManager::updateTowers()
{
    // 使用四叉树优化塔台更新
    QRectF mapBounds(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    Quadtree quadtree(mapBounds, 4); // 将四叉树划分为四个区域，每个区域最多可容纳 4 个敌人

    // 将所有活着的敌人插入四叉树
    for (QPointer<Enemy> enemy : enemies)
    {
        if (enemy)
        {
            quadtree.insert(enemy);
        }
    }

    // 遍历所有塔并更新它们
    for (QPointer<Tower> tower : towers)
    {
        if (!tower)
            continue;

        // 计算该塔的射程
        qreal range = tower->getRange();
        QRectF queryRect(tower->x() - range, tower->y() - range, range * 2, range * 2);

        // 查询四叉树以查找防御塔范围内的所有敌人
        QList<Enemy *> potentialEnemies;
        quadtree.query(queryRect, potentialEnemies);

        // 创建一份位于防御塔射程内的敌人列表
        QList<QPointer<Enemy>> enemiesInRange;
        for (Enemy *rawEnemy : potentialEnemies)
        {
            QPointer<Enemy> enemy(rawEnemy);
            if (!enemy)
                continue;

            // 使用距离的平方来检查敌人是否在防御塔的射程内
            qreal dx = enemy->x() - tower->x();
            qreal dy = enemy->y() - tower->y();
            qreal distSq = dx * dx + dy * dy;

            if (distSq <= range * range)
            {
                enemiesInRange.append(enemy);
            }
        }

        // 将敌人置于防御塔的攻击范围内，并更新防御塔
        tower->setEnemiesInRange(enemiesInRange);
        tower->update();
    }
}

void GameManager::removeDeadEntities()
{
    QList<QPointer<Enemy>> deadEnemies;

    for (QPointer<Enemy> enemy : enemies)
    {
        if (!enemy)
            continue;

        if (enemy->getHealth() <= 0)
        {
            killCount++;
            gold += enemy->getReward();
            playSound("coin", 0.7, false);
            emit goldChanged(gold);
            emit killCountChanged(killCount);

            enemy->setState(ResourceManager::ENEMY_DEAD);
            deadEnemies.append(enemy);
            emit enemyDied(enemy);
        }
    }

    for (QPointer<Enemy> enemy : deadEnemies)
    {
        enemies.removeOne(enemy);
    }
}

void GameManager::checkNextWave()
{
    if (waveSpawnComplete && enemies.isEmpty())
    {
        if (currentWave >= GameConfig::WAVE_COUNT_MAX)
        {
            if (gameTimer->isActive())
                gameTimer->stop();
            if (enemySpawnTimer->isActive())
                enemySpawnTimer->stop();

            gameRunning = false;
            paused = false;

            emit gameStateChanged(gameRunning, paused);
            emit levelCompleted(currentMapId, currentWave);

            return;
        }

        currentWave++;
        enemiesSpawnedThisWave = 0;
        waveSpawnComplete = false;
        emit waveChanged(currentWave);

        int newInterval = getWaveSpawnInterval();
        enemySpawnTimer->setInterval(newInterval);
    }
}

int GameManager::getWaveSpawnInterval() const
{
    int interval = GameConfig::WAVE_SPAWN_INTERVAL_MAX -
                   GameConfig::WAVE_SPAWN_INTERVAL_EACH * (currentWave - 1);
    int minInterval = GameConfig::WAVE_SPAWN_INTERVAL_MIN;

    if (interval < minInterval)
    {
        interval = minInterval;
    }

    return interval;
}

int GameManager::calculateWaveHealth() const
{
    const int base = GameConfig::ENEMY_HEALTH;
    qreal factor = 1.0 + GameConfig::ENEMY_HEALTH_GROWTH_PER_WAVE * (currentWave - 1);
    int hp = static_cast<int>(base * factor);
    return hp;
}

float GameManager::calculateWaveSpeed() const
{
    float factor = 1.0f + GameConfig::ENEMY_SPEED_GROWTH_PER_WAVE * static_cast<float>(currentWave - 1);
    return GameConfig::ENEMY_SPEED * factor;
}

bool GameManager::isEnemyAtAnyEndPoint(QPointer<Enemy> enemy) const
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

int GameManager::getTowerCost(Tower::TowerType type) const
{
    switch (type)
    {
    case Tower::ARROW_TOWER:
        return GameConfig::TowerStats::ARROW_COST;
    case Tower::CANNON_TOWER:
        return GameConfig::TowerStats::CANNON_COST;
    case Tower::MAGIC_TOWER:
        return GameConfig::TowerStats::MAGIC_COST;
    }
    return GameConfig::TowerStats::ARROW_COST;
}

void GameManager::setResourceManager(ResourceManager *manager)
{
    if (manager)
        resourceManager = manager;
}

void GameManager::playSound(const QString &soundId, qreal volume, bool loop)
{
    if (!resourceManager)
        return;
    resourceManager->playSound(soundId, volume, loop);
}

QPointer<Tower> GameManager::buildTower(Tower::TowerType type, const QPointF &position, QObject *parentForTower)
{
    int cost = getTowerCost(type);
    if (gold < cost)
    {
        return QPointer<Tower>();
    }

    gold -= cost;
    emit goldChanged(gold);

    QPointer<Tower> tower = new Tower(type, position, parentForTower);
    if (tower && resourceManager)
    {
        tower->setResourceManager(resourceManager);
    }
    towers.append(tower);
    emit towerBuilt(tower);

    return tower;
}

QPointer<Tower> GameManager::upgradeTower(QPointer<Tower> tower)
{
    if (!tower)
        return QPointer<Tower>();

    int index = towers.indexOf(tower);
    if (index < 0)
        return QPointer<Tower>();

    Tower::TowerType currentType = tower->getTowerType();
    Tower::TowerType nextType;
    switch (currentType)
    {
    case Tower::ARROW_TOWER:
        nextType = Tower::CANNON_TOWER;
        break;
    case Tower::CANNON_TOWER:
        nextType = Tower::MAGIC_TOWER;
        break;
    case Tower::MAGIC_TOWER:
        return QPointer<Tower>();
    }

    int currentCost = getTowerCost(currentType);
    int nextCost = getTowerCost(nextType);
    int extraCost = nextCost - currentCost;
    if (extraCost < 0)
        extraCost = 0;

    if (gold < extraCost)
        return QPointer<Tower>();

    gold -= extraCost;
    emit goldChanged(gold);

    QPointF position = tower->pos();
    QObject *parentForTower = tower->parent();

    QPointer<Tower> newTower = new Tower(nextType, position, parentForTower);
    if (newTower && resourceManager)
    {
        newTower->setResourceManager(resourceManager);
    }
    towers[index] = newTower;

    emit towerUpgraded(tower, newTower);

    return newTower;
}

bool GameManager::demolishTower(QPointer<Tower> tower)
{
    if (!tower)
        return false;

    int index = towers.indexOf(tower);
    if (index < 0)
        return false;

    int cost = tower->getCost();
    int refund = cost * 70 / 100;
    if (refund < 0)
        refund = 0;

    if (refund > 0)
    {
        gold += refund;
        playSound("coin", 0.7, false);
        emit goldChanged(gold);
    }

    towers.removeAt(index);

    emit towerDemolished(tower);

    return true;
}
