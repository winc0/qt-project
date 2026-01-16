#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "config.h"
#include <QObject>
#include <QPixmap>
#include <QMap>
#include <QString>
#include <QHash>

// 管理图片与音效等共享资源
// AI-generated class
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

    // 获取全局单例资源管理器
    static ResourceManager& instance();

    // 预加载所有必要资源
    bool loadResources();
    // 根据名称获取缓存图片
    QPixmap getPixmap(const QString& name) const;

    // 敌人状态枚举
    // 敌人动画状态枚举
    enum EnemyState {
        ENEMY_IDLE,
        ENEMY_WALK,
        ENEMY_JUMP,
        ENEMY_DEAD
    };

    // 获取指定敌人状态贴图
    QPixmap getEnemyPixmap(int enemyType, EnemyState state) const;

    // 获取用户角色状态贴图
    QPixmap getUserPixmap(UserState state) const;

    // 播放或循环播放音效
    void playSound(const QString &soundId, qreal volume = 1.0, bool loop = false);

    // 获取默认子弹贴图
    QPixmap getBulletPixmap() const;
    // 获取默认塔贴图
    QPixmap getTowerPixmap() const;
    // 获取默认塔底座贴图
    QPixmap getTowerBasePixmap() const;

    // 根据塔类型与等级取贴图
    QPixmap getTowerPixmapForType(int towerType, int level);
    // 根据塔类型与等级取底座贴图
    QPixmap getTowerBasePixmapForType(int towerType, int level);
    // 根据子弹类型与等级取贴图
    QPixmap getBulletPixmapForType(int bulletType, int level);
    // 默认敌人占位贴图
    QPixmap getDefaultEnemyPixmap() const;
    // 默认塔占位贴图
    QPixmap getDefaultTowerPixmap() const;
    // 默认塔底座占位贴图
    QPixmap getDefaultTowerBasePixmap() const;
    // 默认子弹占位贴图
    QPixmap getDefaultBulletPixmap() const;
    // 默认主角占位贴图
    QPixmap getDefaultUserPixmap() const;
    // 默认背景占位贴图
    QPixmap getDefaultBackground() const;
    // 获取默认地图贴图
    QPixmap getGameMap() const;
    // 获取指定地图ID贴图
    QPixmap getGameMap(GameConfig::MapId mapId) const;

private:
    // 私有构造仅供单例使用
    explicit ResourceManager(QObject *parent = nullptr);
    // 默认析构释放资源
    ~ResourceManager() = default;

    QMap<QString, QPixmap> pixmapCache;
    QHash<QString, QList<class QSoundEffect*>> soundEffectPool;

    // 加载默认占位图片资源
    void loadDefaultPixmaps();
    // 预创建常用音效对象池
    void preloadDefaultSounds();
    // 从池中获取可用音效对象
    class QSoundEffect* acquireSoundEffect(const QString &soundId);
};

#endif
