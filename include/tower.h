#ifndef TOWER_H
#define TOWER_H

#include "gameentity.h"
#include <QTimer>
#include <QList>
#include <QGraphicsPixmapItem>
#include <QPointer>
#include <QElapsedTimer>

class Enemy;
class QGraphicsScene;

class Tower : public GameEntity
{
    Q_OBJECT
public:
    enum TowerType
    {
        ARROW_TOWER,
        CANNON_TOWER,
        MAGIC_TOWER
    };

    explicit Tower(TowerType type, QPointF position, QObject *parent = nullptr);
    ~Tower();

    void update() override;
    void setTarget(QPointer<Enemy> target);
    void fire();
    void setGameScene(QGraphicsScene *scene) { gameScene = scene; }

    int getDamage() const { return damage; }
    int getRange() const { return range; }
    int getCost() const { return cost; }
    TowerType getTowerType() const { return towerType; }

    void setEnemiesInRange(const QList<QPointer<Enemy>> &enemies);

    // 暂停/恢复攻击
    void pauseAttack()
    {
        if (attackTimer)
            attackTimer->stop();
    }
    void resumeAttack()
    {
        if (attackTimer && !attackTimer->isActive())
            attackTimer->start(fireRate);
    }

    // 获取底座图形项
    QGraphicsPixmapItem *getBaseItem() const { return baseItem; }

private slots:
    void onAttackTimer();
signals:
    void fired();
    void targetDestroyed();

private:
    TowerType towerType;
    int damage;
    int range;
    int fireRate; // 毫秒
    int cost;
    QPointer<Enemy> currentTarget; // 当前攻击目标
    QTimer *attackTimer;
    QList<QPointer<Enemy>> enemiesInRange;
    QGraphicsScene *gameScene;

    QGraphicsPixmapItem *baseItem; // 底座图形
    qreal currentRotation;         // 当前旋转角度
    qreal targetRotation;          // 目标旋转角度
    qreal rotationSpeed;           // 旋转速度
    bool targetLocked;
    QElapsedTimer targetLockTimer;

    bool isInRange(QPointer<Enemy> enemy) const;
    void findTarget();
    void updateTowerRotation(); // 根据目标更新塔的旋转角度
    void updateTargetLock();
};

#endif
