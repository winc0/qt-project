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

Bullet::Bullet(BulletType type, QPointF startPos, const QPointF &initialDirection, QPointer<Enemy> target, int damage, QObject *parent)
    : GameEntity(BULLET, parent)
    , bulletType(type)
    , target(target)
    , damage(damage)
    , speed(GameConfig::BULLET_SPEED)
    , direction(0.0, -1.0)
    , startPosition(startPos)
    , travelledDistance(0.0f)
    , lostTargetTimeMs(0)
    , resourceManager(nullptr)
{
    // 设置子弹位置为传入的中心点坐标
    setPos(startPos);

    // 从资源文件加载子弹图片
    ResourceManager& rm = ResourceManager::instance();

    int level = 1;
    switch (bulletType)
    {
    case BULLET_ARROW:
        level = 1;
        break;
    case BULLET_CANNON:
        level = 2;
        break;
    case BULLET_MAGIC:
        level = 3;
        break;
    }

    QPixmap bulletPixmap = rm.getBulletPixmapForType(static_cast<int>(bulletType), level);

    setPixmap(bulletPixmap);
    
    // 设置图片的中心为旋转中心，并通过偏移让pos表示子弹中心（锚点为0.5,0.5）
    setTransformOriginPoint(bulletPixmap.width() / 2.0, bulletPixmap.height() / 2.0);
    setOffset(-bulletPixmap.width() / 2.0, -bulletPixmap.height() / 2.0);

    QPointF dir = initialDirection;
    qreal len = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
    if (len > 0.0)
        direction = dir / len;

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
            playSound("hurt", 0.8, false);
            if (scene())
                scene()->removeItem(this);
            deleteLater();
            return;
        }

        if (distanceToTarget > 0.0)
        {
            QPointF desiredDir = toTarget / distanceToTarget;

            QPointF dir = direction;
            qreal dirLen = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
            if (dirLen > 0.0)
                dir /= dirLen;

            qreal dot = dir.x() * desiredDir.x() + dir.y() * desiredDir.y();

            // 优化：如果角度大于 90 度（dot < 0），则停止跟踪，否则转弯会过急
            if (dot < 0)
            {
                // 停止追踪，直线飞行
                target = nullptr;
                hasTarget = false;
            }
            else
            {
                if (dot > 1.0) dot = 1.0;
                
                qreal angle = std::acos(dot);
                qreal maxTurnDeg = 15.0; // 增加最大转弯角度
                qreal maxTurnRad = maxTurnDeg * M_PI / 180.0;

                if (angle > 0.0001)
                {
                    qreal turn = angle;
                    if (turn > maxTurnRad)
                        turn = maxTurnRad;

                    qreal cross = dir.x() * desiredDir.y() - dir.y() * desiredDir.x();
                    qreal sign = cross >= 0.0 ? 1.0 : -1.0;

                    qreal c = std::cos(turn);
                    qreal s = std::sin(turn) * sign;

                    QPointF newDir(dir.x() * c - dir.y() * s,
                                   dir.x() * s + dir.y() * c);

                    qreal newLen = std::sqrt(newDir.x() * newDir.x() + newDir.y() * newDir.y());
                    if (newLen > 0.0)
                        direction = newDir / newLen;
                }
            }
        }
    }

    QPointF delta = direction * speed;
    QPointF nextPos = currentPos + delta;
    setPos(nextPos);

    updateRotation();

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

void Bullet::playSound(const QString &soundId, qreal volume, bool loop)
{
    if (!resourceManager)
        return;
    resourceManager->playSound(soundId, volume, loop);
}

void Bullet::updateRotation()
{
    QPointF dir = direction;
    qreal angle = std::atan2(dir.y(), dir.x()) * 180.0 / M_PI;
    angle += 90.0;
    setRotation(angle);
}
