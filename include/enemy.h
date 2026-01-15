#ifndef ENEMY_H
#define ENEMY_H

#include "gameentity.h"
#include "resourcemanager.h"
#include "config.h"
#include <QTimer>
#include <QVector>

class Enemy : public GameEntity
{
    Q_OBJECT
public:
    // 使用ResourceManager中定义的EnemyState
    typedef ResourceManager::EnemyState EnemyState;

    explicit Enemy(int enemyType = 0, QObject *parent = nullptr);
    ~Enemy();

    void update() override;
    void setPath(const QVector<QPointF>& pathPoints);
    void moveAlongPath();

    int getReward() const { return reward; }
    bool isAtEnd() const { return reachedEnd; }
    int getEnemyType() const { return enemyType; }
    EnemyState getState() const { return currentState; }
    void setState(EnemyState state);
    QPointF getCenterPosition() const;
    
    // 暂停/恢复敌人移动
    void pauseMovement() { if (moveTimer) moveTimer->stop(); }
    void resumeMovement() { if (moveTimer && currentState != ResourceManager::ENEMY_DEAD) moveTimer->start(GameConfig::ENEMY_MOVE_INTERVAL); }

private slots:
    void onMoveTimer();
signals:
    void reachedEndPoint();
    void enemyKilled(int reward);
private:
    int enemyType;
    int reward;
    float speed;
    EnemyState currentState;
    QVector<QPointF> pathPoints;
    int currentPathIndex;
    QTimer *moveTimer;
    bool reachedEnd;
};

#endif
