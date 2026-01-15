#ifndef BULLET_H
#define BULLET_H

#include "gameentity.h"
#include "enemy.h"
#include "config.h"
#include <QTimer>
#include <QPointer>

class Bullet : public GameEntity
{
    Q_OBJECT
public:
    enum BulletType
    {
        BULLET_ARROW = 0,
        BULLET_CANNON = 1,
        BULLET_MAGIC = 2
    };

    explicit Bullet(BulletType type, QPointF startPos, const QPointF &initialDirection, QPointer<Enemy> target, int damage, QObject *parent = nullptr);
    ~Bullet();

    void update() override;
    Enemy* getTarget() const { return target; }
    
    // 暂停/恢复移动
    void pauseMovement() { if (moveTimer) moveTimer->stop(); }
    void resumeMovement() { if (moveTimer && !moveTimer->isActive()) moveTimer->start(GameConfig::BULLET_MOVE_INTERVAL); }

signals:
    void hit(Enemy* enemy, int damage);

private:
    void updateRotation();

private slots:
    void onMoveTimer();

private:
    BulletType bulletType;
    QPointer<Enemy> target;  // 使用QPointer自动跟踪target的生命周期
    int damage;
    float speed;
    QTimer* moveTimer;
    QPointF direction; // 方向
    QPointF startPosition; // 发射位置
    float travelledDistance;
    int lostTargetTimeMs;
};

#endif
