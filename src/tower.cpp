#include "include/tower.h"
#include "include/enemy.h"
#include "include/bullet.h"
#include "include/resourcemanager.h"
#include "include/config.h"

#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QDebug>
#include <math.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Tower::Tower(TowerType type, QPointF position, QObject *parent)
    : GameEntity(TOWER, parent),
      towerType(type),
      currentTarget(nullptr),
      gameScene(nullptr),
      baseItem(nullptr),
      currentRotation(0.0),
      targetRotation(0.0),
      rotationSpeed(5.0),
      targetLocked(false)
{
    // 设置基础属性
    switch (type)
    {
    case ARROW_TOWER:
        damage = GameConfig::TowerStats::ARROW_DAMAGE;
        range = GameConfig::TowerStats::ARROW_RANGE;
        cost = GameConfig::TowerStats::ARROW_COST;
        fireRate = GameConfig::TowerStats::ARROW_FIRE_RATE;
        break;
    case CANNON_TOWER:
        damage = GameConfig::TowerStats::CANNON_DAMAGE;
        range = GameConfig::TowerStats::CANNON_RANGE;
        cost = GameConfig::TowerStats::CANNON_COST;
        fireRate = GameConfig::TowerStats::CANNON_FIRE_RATE;
        break;
    case MAGIC_TOWER:
        damage = 30;
        range = 100;
        cost = 150;
        fireRate = 1500;
        break;
    }

    // 从资源文件加载塔的图片
    ResourceManager &rm = ResourceManager::instance();
    QPixmap towerPixmap = rm.getTowerPixmap();
    QPixmap basePixmap = rm.getTowerBasePixmap();

    setPixmap(towerPixmap);
    // 设置图片的中心为旋转中心
    setTransformOriginPoint(towerPixmap.width() / 2.0, towerPixmap.height() / 2.0);

    setPos(position);

    // 创建底座图形项（在塔下层）
    baseItem = new QGraphicsPixmapItem(basePixmap);
    baseItem->setPos(position.x() /* - basePixmap.width() / 2.0 */, position.y() /* - basePixmap.height() / 2.0 */);
    baseItem->setZValue(-1); // 底座在塔下层

    // 攻击计时器
    attackTimer = new QTimer(this);
    connect(attackTimer, &QTimer::timeout, this, &Tower::onAttackTimer);
    attackTimer->start(fireRate);

    setHealth(100);
}

Tower::~Tower()
{
    attackTimer->stop();
    // 清理底座
    if (baseItem && baseItem->scene())
    {
        baseItem->scene()->removeItem(baseItem);
    }
    delete baseItem;
}

void Tower::update()
{
    updateTargetLock();
    if (!currentTarget)
    {
        findTarget();
    }

    // 更新塔的旋转角度
    if (currentTarget)
    {
        updateTowerRotation();
    }
}

void Tower::setTarget(QPointer<Enemy> target)
{
    currentTarget = target;
}

void Tower::fire()
{
    if (currentTarget && gameScene)
    {
        QPointF bulletStartPos = mapToScene(transformOriginPoint());

        qDebug() << "Tower firing from center" << bulletStartPos;

        // 创建子弹对象
        QPointer<Bullet> bullet = new Bullet(bulletStartPos, currentTarget, damage, nullptr);
        if (bullet)
        {
            gameScene->addItem(bullet);
            targetLocked = true;
            targetLockTimer.restart();
            emit fired();
        }
        else
        {
            qWarning() << "Failed to create bullet";
        }
    }
}

void Tower::setEnemiesInRange(const QList<QPointer<Enemy>> &enemies)
{
    enemiesInRange = enemies;
    findTarget();
}

void Tower::onAttackTimer()
{
    if (currentTarget && isInRange(currentTarget))
    {
        fire();
    }
    else
    {
        findTarget();
        if (currentTarget)
        {
            fire();
        }
    }
}

bool Tower::isInRange(QPointer<Enemy> enemy) const
{
    if (!enemy)
        return false;

    qreal dx = enemy->x() - this->x();
    qreal dy = enemy->y() - this->y();
    qreal distance = std::sqrt(dx * dx + dy * dy); // 使用 std::sqrt

    return distance <= range;
}

void Tower::findTarget()
{
    QPointer<Enemy> closestEnemy = nullptr;
    qreal minDistance = range + 1; // 初始化为超出范围

    for (QPointer<Enemy> enemy : enemiesInRange)
    {
        if (enemy && isInRange(enemy))
        {
            qreal dx = enemy->x() - this->x();
            qreal dy = enemy->y() - this->y();
            qreal distance = std::sqrt(dx * dx + dy * dy); // 使用 std::sqrt

            if (distance < minDistance)
            {
                minDistance = distance;
                closestEnemy = enemy;
            }
        }
    }

    currentTarget = closestEnemy;
}

void Tower::updateTowerRotation()
{
    if (!currentTarget)
        return;

    QPointF towerPos = pos();
    QRectF rect = boundingRect();
    QPointF towerCenter(towerPos.x() + rect.width() / 2.0, towerPos.y() + rect.height() / 2.0);
    QPointF targetCenter = currentTarget->getCenterPosition();
    QPointF direction = targetCenter - towerCenter;

    // 计算角度（atan2返回的角度是相对于x轴的，单位是弧度）
    // 由于图片默认向上，我们需要调整
    qreal angle = std::atan2(direction.y(), direction.x()) * 180.0 / M_PI;

    // 调整90度（因为图片向上，需要从0度旋转90度）
    angle += 90.0;

    targetRotation = angle;

    qreal delta = targetRotation - currentRotation;
    while (delta > 180.0)
        delta -= 360.0;
    while (delta < -180.0)
        delta += 360.0;

    qreal maxStep = rotationSpeed;
    if (delta > maxStep)
        delta = maxStep;
    else if (delta < -maxStep)
        delta = -maxStep;

    currentRotation += delta;
    setRotation(currentRotation);

    if (baseItem && scene())
        baseItem->update();
}

void Tower::updateTargetLock()
{
    if (currentTarget && !isInRange(currentTarget))
        currentTarget = nullptr;

    if (targetLocked)
    {
        if (!currentTarget)
        {
            targetLocked = false;
            return;
        }
        if (targetLockTimer.isValid() &&
            targetLockTimer.hasExpired(400))
        {
            targetLocked = false;
            currentTarget = nullptr;
        }
    }
}
