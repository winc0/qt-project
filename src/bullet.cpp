#include "include/bullet.h"
#include "include/enemy.h"
#include "include/config.h"
#include "include/resourcemanager.h"

#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QGraphicsScene>
#include <cmath>
#include <QDebug>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Bullet::Bullet(QPointF startPos, QPointer<Enemy> target, int damage, QObject *parent)
    : GameEntity(BULLET, parent)
    , target(target)
    , damage(damage)
    , speed(GameConfig::BULLET_SPEED)
    , direction(0.0, -1.0)
    , startPosition(startPos)
    , travelledDistance(0.0f)
    , lostTargetTimeMs(0)
{
    // 设置子弹位置为传入的中心点坐标
    setPos(startPos);

    // 从资源文件加载子弹图片
    ResourceManager& rm = ResourceManager::instance();
    QPixmap bulletPixmap = rm.getBulletPixmap();
    
    setPixmap(bulletPixmap);
    
    // 设置图片的中心为旋转中心，并通过偏移让pos表示子弹中心（锚点为0.5,0.5）
    setTransformOriginPoint(bulletPixmap.width() / 2.0, bulletPixmap.height() / 2.0);
    setOffset(-bulletPixmap.width() / 2.0, -bulletPixmap.height() / 2.0);

    if (target && !target.isNull())
    {
        QPointF targetCenter = target->getCenterPosition();
        QPointF dir = targetCenter - startPos;
        qreal len = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
        if (len > 0.0)
            direction = dir / len;
    }

    updateRotation();

    moveTimer = new QTimer(this);
    connect(moveTimer, &QTimer::timeout, this, &Bullet::onMoveTimer);
    moveTimer->start(GameConfig::BULLET_MOVE_INTERVAL);

    qDebug() << "Bullet created at" << startPos
             << "direction" << direction
             << "target" << (target ? "valid" : "invalid");
}

Bullet::~Bullet()
{
    if (moveTimer) {
        moveTimer->stop();
    }
}

void Bullet::update()
{
    onMoveTimer();
}

void Bullet::onMoveTimer()
{
    QPointF currentPos = pos();
    bool hasTarget = target && !target.isNull();

    if (!hasTarget)
    {
        lostTargetTimeMs += GameConfig::BULLET_MOVE_INTERVAL;
        if (lostTargetTimeMs >= GameConfig::BULLET_TARGET_LOST_TIMEOUT_MS)
        {
            if (scene())
                scene()->removeItem(this);
            deleteLater();
            return;
        }
    }

    if (hasTarget)
    {
        QPointF targetCenter = target->getCenterPosition();
        QPointF toTarget = targetCenter - currentPos;
        qreal distanceToTarget = std::sqrt(toTarget.x() * toTarget.x() + toTarget.y() * toTarget.y());
        if (distanceToTarget <= GameConfig::ENEMY_COLLISION_RADIUS + GameConfig::BULLET_COLLISION_RADIUS)
        {
            qDebug() << "Bullet hit target! Dealing" << damage << "damage";
            emit hit(target, damage);
            target->setHealth(target->getHealth() - damage);
            if (scene())
                scene()->removeItem(this);
            deleteLater();
            return;
        }
    }

    QPointF delta = direction * speed;
    QPointF nextPos = currentPos + delta;
    setPos(nextPos);

    travelledDistance += std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
    if (travelledDistance >= GameConfig::BULLET_MAX_DISTANCE)
    {
        if (scene())
            scene()->removeItem(this);
        deleteLater();
        return;
    }

    if (nextPos.x() < -GameConfig::BULLET_SIZE ||
        nextPos.x() > GameConfig::WINDOW_WIDTH + GameConfig::BULLET_SIZE ||
        nextPos.y() < -GameConfig::BULLET_SIZE ||
        nextPos.y() > GameConfig::WINDOW_HEIGHT + GameConfig::BULLET_SIZE)
    {
        if (scene())
            scene()->removeItem(this);
        deleteLater();
        return;
    }
}

void Bullet::updateRotation()
{
    QPointF dir = direction;
    qreal angle = std::atan2(dir.y(), dir.x()) * 180.0 / M_PI;
    angle += 90.0;
    setRotation(angle);
}
