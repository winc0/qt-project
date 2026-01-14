#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <QObject>
#include <QPixmap>
#include <QMap>
#include <QString>

class ResourceManager : public QObject
{
    Q_OBJECT

public:
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
    QPixmap getEnemyPixmapByState(EnemyState state) const; // 简化版，仅按状态获取

    QPixmap getBulletPixmap() const;
    QPixmap getTowerPixmap() const;
    QPixmap getTowerBasePixmap() const;
    // 默认图片
    QPixmap getDefaultEnemyPixmap() const;
    QPixmap getDefaultTowerPixmap() const;
    QPixmap getDefaultTowerBasePixmap() const;
    QPixmap getDefaultBulletPixmap() const;
    QPixmap getDefaultBackground() const;
    QPixmap getGameMap() const;

private:
    explicit ResourceManager(QObject *parent = nullptr);
    ~ResourceManager() = default;

    QMap<QString, QPixmap> pixmapCache;

    void loadDefaultPixmaps();
};

#endif
