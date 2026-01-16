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
#include <QDateTime>
#include <QTimer>
#include <math.h>
#include <cmath>
#include <limits>

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
      rotationSpeed(GameConfig::TOWER_ROTATION_SPEED_DEG_PER_SEC),
      targetLocked(false),
      targetLostTime(0),
      resourceManager(nullptr)
{
    // 根据防御塔类型设置基础属性（伤害、射程、价格、攻速）
    switch (type)
    {
    case ARROW_TOWER: // 箭塔
        damage = GameConfig::TowerStats::ARROW_DAMAGE;
        range = GameConfig::TowerStats::ARROW_RANGE;
        cost = GameConfig::TowerStats::ARROW_COST;
        fireRate = GameConfig::TowerStats::ARROW_FIRE_RATE;
        break;
    case CANNON_TOWER: // 炮塔
        damage = GameConfig::TowerStats::CANNON_DAMAGE;
        range = GameConfig::TowerStats::CANNON_RANGE;
        cost = GameConfig::TowerStats::CANNON_COST;
        fireRate = GameConfig::TowerStats::CANNON_FIRE_RATE;
        break;
    case MAGIC_TOWER: // 魔法塔
        damage = GameConfig::TowerStats::MAGIC_DAMAGE;
        range = GameConfig::TowerStats::MAGIC_RANGE;
        cost = GameConfig::TowerStats::MAGIC_COST;
        fireRate = GameConfig::TowerStats::MAGIC_FIRE_RATE;
        break;
    }

    // 获取视觉等级（用于加载对应的图片资源）
    int visualLevel = 1;
    switch (type)
    {
    case ARROW_TOWER:
        visualLevel = 1;
        break;
    case CANNON_TOWER:
        visualLevel = 2;
        break;
    case MAGIC_TOWER:
        visualLevel = 3;
        break;
    }

    // 从资源管理器加载防御塔和底座的像素图
    ResourceManager &rm = ResourceManager::instance();
    QPixmap towerPixmap = rm.getTowerPixmapForType(static_cast<int>(type), visualLevel);
    QPixmap basePixmap = rm.getTowerBasePixmapForType(static_cast<int>(type), visualLevel);

    // 设置防御塔的像素图并设置旋转中心
    setPixmap(towerPixmap);
    setTransformOriginPoint(towerPixmap.width() / 2.0, towerPixmap.height() / 2.0);
    setPos(position);

    // 创建并设置底座图形项（Z值低于防御塔，显示在下层）
    baseItem = new QGraphicsPixmapItem(basePixmap);
    baseItem->setPos(position.x(), position.y());
    baseItem->setZValue(-1); // 底座在防御塔下层

    // 初始化攻击定时器
    attackTimer = new QTimer(this);
    connect(attackTimer, &QTimer::timeout, this, &Tower::onAttackTimer);
    attackTimer->start(fireRate);

    // 初始化防御塔生命值
    setHealth(100);
}

Tower::~Tower()
{
    attackTimer->stop();
    // 清理底座图形项
    if (baseItem && baseItem->scene())
    {
        baseItem->scene()->removeItem(baseItem);
    }
    delete baseItem;
}

void Tower::update()
{
    // 更新目标锁定状态
    updateTargetLock();
    // 查找新目标（内部处理延迟）
    findTarget();

    // 每帧更新防御塔旋转（处理回到零度的情况）
    updateTowerRotation();
}

void Tower::setTarget(QPointer<Enemy> target)
{
    currentTarget = target;
}

void Tower::fire()
{
    if (currentTarget && gameScene)
    {
        // 获取防御塔炮管顶端的位置（场景坐标）
        QRectF rect = boundingRect();
        QPointF localTip(rect.width() / 2.0, 0.0);
        QPointF bulletStartPos = mapToScene(localTip);

        qDebug() << "Tower firing from tip" << bulletStartPos;

        // 根据防御塔类型确定子弹类型
        Bullet::BulletType bulletType = Bullet::BULLET_ARROW;
        switch (towerType)
        {
        case ARROW_TOWER:
            bulletType = Bullet::BULLET_ARROW;
            break;
        case CANNON_TOWER:
            bulletType = Bullet::BULLET_CANNON;
            break;
        case MAGIC_TOWER:
            bulletType = Bullet::BULLET_MAGIC;
            break;
        }

        // 计算子弹初速度方向（根据防御塔旋转角度）
        qreal angleDeg = rotation() - 90.0;
        qreal angleRad = angleDeg * M_PI / 180.0;
        QPointF initialDir(std::cos(angleRad), std::sin(angleRad));

        // 创建子弹对象
        QPointer<Bullet> bullet = new Bullet(bulletType, bulletStartPos, initialDir, currentTarget, damage, nullptr);
        if (bullet)
        {
            if (resourceManager)
            {
                bullet->setResourceManager(resourceManager);
            }
            gameScene->addItem(bullet);
            targetLocked = true;
            targetLockTimer.restart();
            emit fired();
            QString soundId;
            switch (towerType)
            {
            case ARROW_TOWER:
                soundId = "shoot_arrow";
                break;
            case CANNON_TOWER:
                soundId = "shoot_cannon";
                break;
            case MAGIC_TOWER:
                soundId = "shoot_magic";
                break;
            }
            playSound(soundId, 1.0, false);
        }
        else
        {
            qWarning() << "Failed to create bullet";
        }
    }
}

void Tower::setEnemiesInRange(const QList<QPointer<Enemy>> &enemies)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    // 从追踪表中清除无效或超出范围的敌人
    auto it = enemyEntryTimes.begin();
    while (it != enemyEntryTimes.end()) {
        if (!it.key() || !enemies.contains(it.key())) {
            it = enemyEntryTimes.erase(it);
        } else {
            ++it;
        }
    }

    // 添加新敌人并记录进入时间
    for (const auto& enemy : enemies) {
        if (enemy && !enemyEntryTimes.contains(enemy)) {
            enemyEntryTimes.insert(enemy, now);
        }
    }

    // 更新范围内的敌人列表
    enemiesInRange = enemies;
    // 目标查找在 update() 中处理
}

void Tower::onAttackTimer()
{
    // 检查当前目标是否仍在射程内
    if (currentTarget && isInRange(currentTarget))
    {
        fire();
    }
    // 目标查找在每帧的 update() 函数中处理
}

bool Tower::isInRange(QPointer<Enemy> enemy) const
{
    if (!enemy)
        return false;

    // 计算防御塔与敌人之间的距离
    qreal dx = enemy->x() - this->x();
    qreal dy = enemy->y() - this->y();
    qreal distance = std::sqrt(dx * dx + dy * dy);

    return distance <= range;
}

void Tower::playSound(const QString &soundId, qreal volume, bool loop)
{
    if (!resourceManager)
        return;
    resourceManager->playSound(soundId, volume, loop);
}

void Tower::findTarget()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    // 1. 检查当前目标是否仍然有效且在射程内
    if (currentTarget)
    {
        if (currentTarget.isNull() || !isInRange(currentTarget))
        {
            // 目标丢失
            targetLostTime = now;
            if (currentTarget && !currentTarget.isNull())
            {
                currentTarget->setHighlighted(false);
            }
            currentTarget = nullptr;
            targetLocked = false;
        }
        else
        {
            // 目标仍然有效，继续追踪
            return;
        }
    }

    // 2. 检查重新扫描延迟（200ms）
    if (now - targetLostTime < 200)
    {
        return;
    }

    // 3. 查找新目标（优先选择最早进入的敌人）
    QPointer<Enemy> bestCandidate = nullptr;
    qint64 earliestTime = std::numeric_limits<qint64>::max();

    for (const auto& enemy : enemiesInRange)
    {
        if (enemy && isInRange(enemy))
        {
            if (enemyEntryTimes.contains(enemy))
            {
                qint64 time = enemyEntryTimes.value(enemy);
                if (time < earliestTime)
                {
                    earliestTime = time;
                    bestCandidate = enemy;
                }
            }
        }
    }

    // 锁定新目标
    if (bestCandidate)
    {
        currentTarget = bestCandidate;
        targetLocked = true;
        if (currentTarget && !currentTarget.isNull())
        {
            currentTarget->setHighlighted(true);
        }
    }
}

void Tower::updateTowerRotation()
{
    // 自动旋转：无目标时回到默认角度（0度）
    if (!currentTarget)
    {
        targetRotation = 0.0;
    }
    else
    {
        // 计算防御塔指向目标的旋转角度
        QPointF towerPos = pos();
        QRectF rect = boundingRect();
        QPointF towerCenter(towerPos.x() + rect.width() / 2.0, towerPos.y() + rect.height() / 2.0);
        QPointF targetCenter = currentTarget->getCenterPosition();
        QPointF direction = targetCenter - towerCenter;

        // 计算目标方向角度
        qreal angle = std::atan2(direction.y(), direction.x()) * 180.0 / M_PI;
        angle += 90.0; // 根据图片方向调整
        targetRotation = angle;
    }

    // 旋转插值逻辑（平滑旋转）
    qreal delta = targetRotation - currentRotation;
    // 处理角度环绕（确保旋转最短路径）
    while (delta > 180.0)
        delta -= 360.0;
    while (delta < -180.0)
        delta += 360.0;

    // 计算单帧旋转步长
    qreal stepPerFrame = rotationSpeed * (GameConfig::GAME_TICK_INTERVAL_MS / 1000.0);
    
    // 平滑旋转至目标角度
    if (std::abs(delta) < stepPerFrame)
    {
        currentRotation = targetRotation;
    }
    else
    {
        if (delta > 0)
            currentRotation += stepPerFrame;
        else
            currentRotation -= stepPerFrame;
    }

    // 设置防御塔旋转
    setRotation(currentRotation);

    // 更新底座显示
    if (baseItem && scene())
        baseItem->update();
}

void Tower::updateTargetLock()
{
    // 检查当前目标是否已超出射程
    if (currentTarget && !isInRange(currentTarget))
    {
        currentTarget->setHighlighted(false);
        currentTarget = nullptr;
    }

    // 更新目标锁定状态
    if (targetLocked)
    {
        if (!currentTarget)
        {
            targetLocked = false;
            return;
        }
        if (targetLockTimer.isValid() &&
            targetLockTimer.hasExpired(GameConfig::TOWER_TARGET_LOCK_MS))
        {
            targetLocked = false;
            currentTarget = nullptr;
        }
    }
}
