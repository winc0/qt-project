#include "include/enemy.h"
#include "include/resourcemanager.h"
#include "include/config.h"

#include <QPainter>
#include <QBrush>
#include <QPen>
#include <cmath>

Enemy::Enemy(int enemyType, QObject *parent)
    : GameEntity(ENEMY, parent)
    , enemyType(enemyType)
    , reward(GameConfig::ENEMY_REWARD)
    , speed(GameConfig::ENEMY_SPEED)
    , currentPathIndex(0)
    , reachedEnd(false)
    , currentState(ResourceManager::ENEMY_WALK)
{
    setHealth(GameConfig::ENEMY_HEALTH);

    // 从资源文件加载敌人图片
    ResourceManager& rm = ResourceManager::instance();
    setPixmap(rm.getEnemyPixmap(enemyType, currentState));

    // 移动计时器
    moveTimer = new QTimer(this);
    connect(moveTimer, &QTimer::timeout, this, &Enemy::onMoveTimer);
    moveTimer->start(GameConfig::ENEMY_MOVE_INTERVAL);
}

Enemy::~Enemy()
{
    moveTimer->stop();
}

void Enemy::update()
{
    moveAlongPath();
}

void Enemy::setPath(const QVector<QPointF>& pathPoints)
{
    this->pathPoints = pathPoints;
    if (!pathPoints.isEmpty()) {
        setPos(pathPoints.first());
        currentPathIndex = 1;
    }
}

void Enemy::moveAlongPath()
{
    if (reachedEnd || pathPoints.isEmpty() || currentPathIndex >= pathPoints.size()) {
        reachedEnd = true;
        return;
    }

    QPointF currentPos = pos();
    QPointF targetPos = pathPoints[currentPathIndex];
    QPointF direction = targetPos - currentPos;
    qreal distance = QLineF(currentPos, targetPos).length();

    if (distance < speed) {
        // 到达路径点
        setPos(targetPos);
        currentPathIndex++;

        if (currentPathIndex >= pathPoints.size()) {
            reachedEnd = true;
            emit reachedEndPoint(); // 需要添加信号声明
        }
    } else {
        // 向目标移动
        direction /= distance;
        setPos(currentPos + direction * speed);
    }
}

void Enemy::onMoveTimer()
{
    moveAlongPath();
}

void Enemy::setState(EnemyState state)
{
    if (currentState == state) {
        return; // 状态未变化
    }
    
    currentState = state;
    
    // 敌人死亡时停止移动
    if (state == ResourceManager::ENEMY_DEAD) {
        if (moveTimer) {
            moveTimer->stop();
        }
    }
    
    // 根据新状态更新图片
    ResourceManager& rm = ResourceManager::instance();
    setPixmap(rm.getEnemyPixmap(enemyType, currentState));
}
