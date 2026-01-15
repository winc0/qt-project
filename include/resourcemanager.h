#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "config.h"
#include <QObject>
#include <QPixmap>
#include <QMap>
#include <QString>
#include <QHash>

class ResourceManager : public QObject
{
    Q_OBJECT

public:
    enum TowerVisualType {
        TOWER_ARROW = 0,
        TOWER_CANNON = 1,
        TOWER_MAGIC = 2
    };

    enum BulletVisualType {
        BULLET_ARROW = 0,
        BULLET_CANNON = 1,
        BULLET_MAGIC = 2
    };

    enum UserState {
        USER_WALK = 0,
        USER_DEAD = 1
    };

    static ResourceManager& instance();

    bool loadResources();
    QPixmap getPixmap(const QString& name) const;

    // 敌人状态枚举
    enum EnemyState {
        ENEMY_IDLE,   // 静止
        ENEMY_WALK,   // 行走
        ENEMY_JUMP,   // 跳跃
        ENEMY_DEAD    // 死亡
    };

    // 敌人图片加载
    QPixmap getEnemyPixmap(int enemyType, EnemyState state) const;

    QPixmap getUserPixmap(UserState state) const;

    void playSound(const QString &soundId, qreal volume = 1.0, bool loop = false);

    QPixmap getBulletPixmap() const;
    QPixmap getTowerPixmap() const;
    QPixmap getTowerBasePixmap() const;

    QPixmap getTowerPixmapForType(int towerType, int level);
    QPixmap getTowerBasePixmapForType(int towerType, int level);
    QPixmap getBulletPixmapForType(int bulletType, int level);
    // 默认图片
    QPixmap getDefaultEnemyPixmap() const;
    QPixmap getDefaultTowerPixmap() const;
    QPixmap getDefaultTowerBasePixmap() const;
    QPixmap getDefaultBulletPixmap() const;
    QPixmap getDefaultUserPixmap() const;
    QPixmap getDefaultBackground() const;
    QPixmap getGameMap() const;
    QPixmap getGameMap(GameConfig::MapId mapId) const;

private:
    explicit ResourceManager(QObject *parent = nullptr);
    ~ResourceManager() = default;

    QMap<QString, QPixmap> pixmapCache;
    QHash<QString, QList<class QSoundEffect*>> soundEffectPool;

    void loadDefaultPixmaps();
    void preloadDefaultSounds();
    class QSoundEffect* acquireSoundEffect(const QString &soundId);
};

#endif
