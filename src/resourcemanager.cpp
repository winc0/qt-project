#include "include/resourcemanager.h"
#include "include/config.h"

#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QDebug>
#include <QRandomGenerator>
#include <QSoundEffect>
#include <QUrl>

ResourceManager::ResourceManager(QObject *parent)
    : QObject(parent)
{
    loadDefaultPixmaps();
    preloadDefaultSounds();
}

ResourceManager& ResourceManager::instance()
{
    static ResourceManager instance;
    return instance;
}

bool ResourceManager::loadResources()
{
    return true; // 使用默认图片
}

QPixmap ResourceManager::getPixmap(const QString& name) const
{
    return pixmapCache.value(name, QPixmap());
}

QPixmap ResourceManager::getEnemyPixmap(int enemyType, EnemyState state) const
{
    // 根据敌人类型和状态构造资源路径
    int type = enemyType;
    if (type < 0 || type > GameConfig::ENEMY_TYPE_NUMBER - 1)
    {
        return getDefaultEnemyPixmap();
    }

    QString stateName;
    switch (state) {
        case ENEMY_IDLE:
            stateName = "idle";
            break;
        case ENEMY_WALK:
            stateName = "walk";
            break;
        case ENEMY_JUMP:
            stateName = "jump";
            break;
        case ENEMY_DEAD:
            stateName = "dead";
            break;
        default:
            stateName = "idle";
    }

    // 路径格式: :/image/enemy/image/enemy_{type}_{state}.png
    // 如果不存在则尝试通用路径: :/image/enemy/image/enemy_{state}.png
    QString resourcePath = QString(":/image/enemy/image/enemy_%1_%2.png").arg(type).arg(stateName);
    QPixmap pixmap(resourcePath);
    
    // 如果特定类型的图片不存在，尝试加载通用版本
    if (pixmap.isNull()) {
        QString fallbackPath = QString(":/image/enemy/image/enemy_%1.png").arg(stateName);
        pixmap.load(fallbackPath);
    }
    
    // 如果仍然不存在，返回默认图片
    if (pixmap.isNull()) {
        return getDefaultEnemyPixmap();
    }

    return pixmap.scaled(GameConfig::ENEMY_SIZE, GameConfig::ENEMY_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap ResourceManager::getUserPixmap(UserState state) const
{
    QString stateName;
    switch (state)
    {
    case USER_WALK:
        stateName = "walk";
        break;
    case USER_DEAD:
        stateName = "dead";
        break;
    default:
        stateName = "walk";
        break;
    }

    QString resourcePath = QString(":/image/user/image/user_%1.png").arg(stateName);
    QPixmap pixmap(resourcePath);

    if (pixmap.isNull())
    {
        return getDefaultUserPixmap();
    }

    return pixmap.scaled(GameConfig::ENEMY_SIZE,
                         GameConfig::ENEMY_SIZE,
                         Qt::KeepAspectRatio,
                         Qt::SmoothTransformation);
}

QPixmap ResourceManager::getDefaultEnemyPixmap() const
{
    QPixmap pixmap(30, 30);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(Qt::red));
    painter.setPen(QPen(Qt::darkRed, 2));
    painter.drawEllipse(2, 2, 26, 26);

    painter.setPen(QPen(Qt::white, 1));
    painter.drawEllipse(8, 8, 4, 4);
    painter.drawEllipse(18, 8, 4, 4);

    return pixmap;
}

QPixmap ResourceManager::getDefaultUserPixmap() const
{
    QPixmap pixmap(GameConfig::ENEMY_SIZE, GameConfig::ENEMY_SIZE);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(QColor(52, 152, 219)));
    painter.setPen(QPen(Qt::darkBlue, 2));
    painter.drawEllipse(4, 4, GameConfig::ENEMY_SIZE - 8, GameConfig::ENEMY_SIZE - 8);

    return pixmap;
}

QPixmap ResourceManager::getTowerBasePixmap() const
{
    QPixmap pixmap(":/image/tower/image/towerbase.png");

    // 如果资源文件不存在，使用默认图片
    if (pixmap.isNull())
    {
        return getDefaultTowerBasePixmap();
    }
    return pixmap.scaled(GameConfig::GRID_SIZE, GameConfig::GRID_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap ResourceManager::getDefaultTowerBasePixmap() const
{
    QPixmap pixmap(GameConfig::GRID_SIZE, GameConfig::GRID_SIZE);
    pixmap = QPixmap(60, 60);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setBrush(QBrush(QColor(139, 69, 19)));
    painter.setPen(QPen(Qt::black, 1));
    painter.drawEllipse(5, 5, 50, 50);
    return pixmap;
}

QPixmap ResourceManager::getTowerPixmap() const
{
    QPixmap pixmap(":/image/tower/image/tower.png");

    // 如果资源文件不存在，使用默认图片
    if (pixmap.isNull())
    {
        return getDefaultTowerPixmap();
    }
    return pixmap.scaled(GameConfig::GRID_SIZE, GameConfig::GRID_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap ResourceManager::getDefaultTowerPixmap() const
{
    QPixmap pixmap(GameConfig::GRID_SIZE, GameConfig::GRID_SIZE);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 塔基
    painter.setBrush(QBrush(QColor(139, 69, 19))); // 棕色
    painter.setPen(QPen(Qt::black, 1));
    painter.drawRect(10, 20, 20, 15);

    // 塔身
    painter.setBrush(QBrush(QColor(192, 192, 192))); // 银色
    painter.drawRect(15, 10, 10, 15);

    // 塔顶
    painter.setBrush(QBrush(Qt::yellow));
    painter.drawEllipse(18, 5, 4, 4);

    return pixmap;
}

QPixmap ResourceManager::getDefaultBackground() const
{
    QPixmap pixmap(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    pixmap.fill(QColor(144, 238, 144)); // 浅绿色背景

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 画网格
    painter.setPen(QPen(QColor(200, 255, 200), 1));
    for (int x = 0; x < GameConfig::WINDOW_WIDTH; x += GameConfig::GRID_SIZE) {
        painter.drawLine(x, 0, x, GameConfig::WINDOW_HEIGHT);
    }
    for (int y = 0; y < GameConfig::WINDOW_HEIGHT; y += GameConfig::GRID_SIZE) {
        painter.drawLine(0, y, GameConfig::WINDOW_WIDTH, y);
    }

    return pixmap;
}

QPixmap ResourceManager::getGameMap() const
{
    return getGameMap(GameConfig::MAP1);
}

QPixmap ResourceManager::getGameMap(GameConfig::MapId mapId) const
{
    QString resourcePath;

    switch (mapId)
    {
    case GameConfig::MAP1:
        resourcePath = ":/image/map/image/map1.png";
        break;
    case GameConfig::MAP2:
        resourcePath = ":/image/map/image/map2.png";
        break;
    default:
        resourcePath = ":/image/map/image/map1.png";
        break;
    }

    QPixmap mapPixmap(resourcePath);

    if (mapPixmap.isNull())
    {
        return getDefaultBackground();
    }

    return mapPixmap.scaled(GameConfig::WINDOW_WIDTH,
                            GameConfig::WINDOW_HEIGHT,
                            Qt::IgnoreAspectRatio,
                            Qt::SmoothTransformation);
}

QPixmap ResourceManager::getBulletPixmap() const
{
    QPixmap bulletPixmap(":/image/bullet/image/bullet.png");

    // 如果资源文件不存在，使用默认图片
    if (bulletPixmap.isNull())
    {
        return getDefaultBulletPixmap();
    }
    return bulletPixmap.scaled(GameConfig::GRID_SIZE, GameConfig::GRID_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap ResourceManager::getDefaultBulletPixmap() const
{
    QPixmap pixmap(10, 10);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setBrush(QBrush(Qt::yellow));
    painter.setPen(QPen(QColor(255, 165, 0), 1));
    painter.drawEllipse(0, 0, 10, 10);
    return pixmap;
}

QPixmap ResourceManager::getTowerPixmapForType(int towerType, int level)
{
    QString cacheKey = QString("tower_%1_lvl%2").arg(towerType).arg(level);
    if (pixmapCache.contains(cacheKey))
    {
        return pixmapCache.value(cacheKey);
    }

    QString baseName;
    switch (towerType)
    {
    case TOWER_ARROW:
        baseName = "arrow_tower";
        break;
    case TOWER_CANNON:
        baseName = "cannon_tower";
        break;
    case TOWER_MAGIC:
        baseName = "magic_tower";
        break;
    default:
        return getDefaultTowerPixmap();
    }

    QPixmap pixmap;

    if (level > 1)
    {
        QString levelPath = QString(":/image/tower/image/%1_lvl%2.png").arg(baseName).arg(level);
        pixmap.load(levelPath);
    }

    if (pixmap.isNull())
    {
        QString basePath = QString(":/image/tower/image/%1.png").arg(baseName);
        pixmap.load(basePath);
    }

    if (pixmap.isNull())
    {
        pixmap = getDefaultTowerPixmap();
    }
    else
    {
        pixmap = pixmap.scaled(GameConfig::GRID_SIZE,
                               GameConfig::GRID_SIZE,
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation);
    }

    pixmapCache.insert(cacheKey, pixmap);
    return pixmap;
}

QPixmap ResourceManager::getTowerBasePixmapForType(int towerType, int level)
{
    QString cacheKey = QString("tower_base_%1_lvl%2").arg(towerType).arg(level);
    if (pixmapCache.contains(cacheKey))
    {
        return pixmapCache.value(cacheKey);
    }

    QString baseName;
    switch (towerType)
    {
    case TOWER_ARROW:
        baseName = "arrow_tower_base";
        break;
    case TOWER_CANNON:
        baseName = "cannon_tower_base";
        break;
    case TOWER_MAGIC:
        baseName = "magic_tower_base";
        break;
    default:
        return getDefaultTowerBasePixmap();
    }

    QPixmap pixmap;

    QString basePath = QString(":/image/tower/image/%1.png").arg(baseName);
    pixmap.load(basePath);

    if (pixmap.isNull())
    {
        pixmap = getDefaultTowerBasePixmap();
    }
    else
    {
        pixmap = pixmap.scaled(GameConfig::GRID_SIZE,
                               GameConfig::GRID_SIZE,
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation);
    }

    pixmapCache.insert(cacheKey, pixmap);
    return pixmap;
}

QPixmap ResourceManager::getBulletPixmapForType(int bulletType, int level)
{
    QString cacheKey = QString("bullet_%1_lvl%2").arg(bulletType).arg(level);
    if (pixmapCache.contains(cacheKey))
    {
        return pixmapCache.value(cacheKey);
    }

    QString baseName;
    switch (bulletType)
    {
    case BULLET_ARROW:
        baseName = "arrow";
        break;
    case BULLET_CANNON:
        baseName = "cannon";
        break;
    case BULLET_MAGIC:
        baseName = "magic";
        break;
    default:
        return getDefaultBulletPixmap();
    }

    QPixmap pixmap;

    if (level > 1)
    {
        QString levelPath = QString(":/image/bullet/image/%1_lvl%2.png").arg(baseName).arg(level);
        pixmap.load(levelPath);
    }

    if (pixmap.isNull())
    {
        QString basePath = QString(":/image/bullet/image/%1.png").arg(baseName);
        pixmap.load(basePath);
    }

    if (pixmap.isNull())
    {
        pixmap = getDefaultBulletPixmap();
    }
    else
    {
        pixmap = pixmap.scaled(GameConfig::GRID_SIZE,
                               GameConfig::GRID_SIZE,
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation);
    }

    pixmapCache.insert(cacheKey, pixmap);
    return pixmap;
}

void ResourceManager::loadDefaultPixmaps()
{
    pixmapCache["enemy_default"] = getDefaultEnemyPixmap();
    pixmapCache["tower_default"] = getDefaultTowerPixmap();
    pixmapCache["background_default"] = getDefaultBackground();
    pixmapCache["user_default"] = getDefaultUserPixmap();

    // 创建子弹图片
    QPixmap bulletPixmap(10, 10);
    bulletPixmap.fill(Qt::transparent);
    QPainter painter(&bulletPixmap);
    painter.setBrush(QBrush(Qt::blue));
    painter.setPen(QPen(Qt::darkBlue, 1));
    painter.drawEllipse(0, 0, 10, 10);
    pixmapCache["bullet_default"] = bulletPixmap;
}

QSoundEffect* ResourceManager::acquireSoundEffect(const QString &soundId)
{
    QList<QSoundEffect*> &pool = soundEffectPool[soundId];
    for (QSoundEffect *effect : pool)
    {
        if (effect && !effect->isPlaying())
        {
            return effect;
        }
    }

    QSoundEffect *effect = new QSoundEffect(this);

    QString fileName;
    if (soundId == "coin")
        fileName = "coin.wav";
    else if (soundId == "hurt")
        fileName = "hurt.wav";
    else if (soundId.startsWith("shoot_"))
        fileName = soundId + ".wav";
    else
        fileName = soundId + ".wav";

    QString resourcePath = QStringLiteral("qrc:/sound/sound/") + fileName;
    effect->setSource(QUrl(resourcePath));

    pool.append(effect);
    return effect;
}

void ResourceManager::playSound(const QString &soundId, qreal volume, bool loop)
{
    QSoundEffect *effect = acquireSoundEffect(soundId);
    if (!effect)
        return;

    qreal v = volume;
    if (v < 0.0)
        v = 0.0;
    if (v > 1.0)
        v = 1.0;

    effect->setVolume(v);
    effect->setLoopCount(loop ? QSoundEffect::Infinite : 1);
    effect->stop();
    effect->play();
}

void ResourceManager::preloadDefaultSounds()
{
    acquireSoundEffect("coin");
    acquireSoundEffect("hurt");
    acquireSoundEffect("shoot_arrow");
    acquireSoundEffect("shoot_cannon");
    acquireSoundEffect("shoot_magic");
}
