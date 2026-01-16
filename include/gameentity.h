#ifndef GAMEENTITY_H
#define GAMEENTITY_H

#include <QObject>
#include <QGraphicsPixmapItem>
#include <QPointF>
#include <QString>

class ResourceManager;

class ISoundPlayable
{
public:
    virtual ~ISoundPlayable() {}
    virtual void setResourceManager(ResourceManager *manager) = 0;
    virtual void playSound(const QString &soundId, qreal volume = 1.0, bool loop = false) = 0;
};

class GameEntity : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    enum EntityType {
        ENEMY,
        TOWER,
        BULLET,
        PATH
    };

    explicit GameEntity(EntityType type, QObject *parent = nullptr);
    virtual ~GameEntity() = default;

    EntityType getType() const { return type; }
    int getHealth() const { return health; }
    void setHealth(int newHealth);
    void setMaxHealth(int newMaxHealth);

    virtual void update() = 0; // 纯虚函数，所有实体必须实现更新逻辑

protected:
    EntityType type;
    int health;
    int maxHealth;
    QPointF velocity;
};

#endif // GAMEENTITY_H
