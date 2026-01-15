#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <QObject>
#include <QList>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include <QPointF>

#include "enemy.h"
#include "tower.h"
#include "bullet.h"
#include "config.h"

class GameManager : public QObject
{
    Q_OBJECT

public:
    explicit GameManager(QObject *parent = nullptr);

    void initialize(GameConfig::MapId mapId,
                    const QVector<QPointF> &pathPoints,
                    const QVector<GameConfig::EndPointConfig> &endPoints);

    void startGame();
    void pauseGame();
    void resetGame();

    int getGold() const { return gold; }
    int getLives() const { return lives; }
    int getCurrentWave() const { return currentWave; }
    int getKillCount() const { return killCount; }
    bool isGameRunning() const { return gameRunning; }
    bool isPaused() const { return paused; }

    const QList<QPointer<Enemy>> &getEnemies() const { return enemies; }
    const QList<QPointer<Tower>> &getTowers() const { return towers; }
    const QList<QPointer<Bullet>> &getBullets() const { return bullets; }

    QPointer<Tower> buildTower(Tower::TowerType type, const QPointF &position, QObject *parentForTower);

signals:
    void goldChanged(int gold);
    void livesChanged(int lives);
    void waveChanged(int currentWave);
    void killCountChanged(int killCount);
    void gameStateChanged(bool running, bool paused);
    void gameOver();

    void enemySpawnRequested(QPointer<Enemy> enemy);
    void enemyReachedEnd(QPointer<Enemy> enemy);
    void enemyDied(QPointer<Enemy> enemy);
    void towerBuilt(QPointer<Tower> tower);

public slots:
    void spawnEnemy();
    void updateGame();

private:
    void updateEnemies();
    void updateTowers();
    void removeDeadEntities();
    void checkNextWave();
    int getWaveSpawnInterval() const;
    bool isEnemyAtAnyEndPoint(QPointer<Enemy> enemy) const;

    int getTowerCost(Tower::TowerType type) const;

private:
    QList<QPointer<Enemy>> enemies;
    QList<QPointer<Tower>> towers;
    QList<QPointer<Bullet>> bullets;

    int gold;
    int lives;
    int currentWave;
    int enemiesSpawnedThisWave;
    bool waveSpawnComplete;
    bool gameRunning;
    bool paused;
    int killCount;

    GameConfig::MapId currentMapId;
    QVector<QPointF> pathPoints;
    QVector<GameConfig::EndPointConfig> endPointAreas;

    QTimer *gameTimer;
    QTimer *enemySpawnTimer;
};

#endif // GAMEMANAGER_H

