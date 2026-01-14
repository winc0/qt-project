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

Bullet::Bullet(QPointF startPos, Enemy* target, int damage, QObject *parent)
    : GameEntity(BULLET, parent)
    , target(target)
    , damage(damage)
    , speed(GameConfig::BULLET_SPEED)
{
    // 设置子弹位置
    setPos(startPos);

    // 从资源文件加载子弹图片
    ResourceManager& rm = ResourceManager::instance();
    QPixmap bulletPixmap = rm.getBulletPixmap();
    
    setPixmap(bulletPixmap);
    
    // 设置图片的中心为旋转中心
    setTransformOriginPoint(bulletPixmap.width() / 2.0, bulletPixmap.height() / 2.0);

    // 移动计时器
    moveTimer = new QTimer(this);
    connect(moveTimer, &QTimer::timeout, this, &Bullet::onMoveTimer);
    moveTimer->start(GameConfig::BULLET_MOVE_INTERVAL);

    qDebug() << "Bullet created at" << startPos << "targeting" << (target ? "valid" : "invalid");
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
    // 如果目标被销毁或无效，删除子弹
    if (!target) {
        if (scene()) {
            scene()->removeItem(this);
        }
        deleteLater();
        return;
    }

    // 额外的安全检查：确保target仍然有效
    if (target.isNull()) {
        if (scene()) {
            scene()->removeItem(this);
        }
        deleteLater();
        return;
    }

    QPointF currentPos = pos();
    QPointF targetPos = target->pos();
    QPointF direction = targetPos - currentPos;
    qreal distance = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());

    // 如果接近目标，造成伤害
    if (distance < speed + 15) {
        qDebug() << "Bullet hit target! Dealing" << damage << "damage";
        target->setHealth(target->getHealth() - damage);
        if (scene()) {
            scene()->removeItem(this);
        }
        deleteLater();
        return;
    }

    // 向目标移动
    if (distance > 0) {
        direction /= distance;
        setPos(currentPos + direction * speed);
        
        // 根据运动方向更新旋转角度
        updateRotation();
    }
}

void Bullet::updateRotation()
{
    // 如果目标有效，计算指向目标的角度
    if (target && !target.isNull()) {
        QPointF currentPos = pos();
        QPointF targetPos = target->pos();
        QPointF direction = targetPos - currentPos;
        
        // 计算角度（atan2返回的角度是相对于x轴的，单位是弧度）
        // 由于图片默认向上（-90度），我们需要调整
        qreal angle = std::atan2(direction.y(), direction.x()) * 180.0 / M_PI;
        
        // 调整90度（因为图片向上，需要从0度旋转90度）
        angle += 90.0;
        
        // 设置旋转角度
        setRotation(angle);
    }
}
