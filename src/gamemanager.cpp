#include "include/gamemanager.h"
#include "include/resourcemanager.h"

#include <cmath>

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
      currentMapId(GameConfig::MAP_DEFAULT),
      gameTimer(new QTimer(this)),
      enemySpawnTimer(new QTimer(this))
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

    gameTimer->start(16);
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
        gameTimer->start(16);
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

    if (enemiesSpawnedThisWave >= GameConfig::WAVE_ENEMY_COUNT * currentWave)
    {
        if (!waveSpawnComplete)
        {
            waveSpawnComplete = true;
        }
        return;
    }

    QPointer<Enemy> enemy = new Enemy(0, this);
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
    for (QPointer<Tower> tower : towers)
    {
        if (!tower)
            continue;

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
    towers.append(tower);
    emit towerBuilt(tower);

    return tower;
}

