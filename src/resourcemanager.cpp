#include "include/resourcemanager.h"
#include "include/config.h"

#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

ResourceManager::ResourceManager(QObject *parent)
    : QObject(parent)
{
    loadDefaultPixmaps();
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
    
    // 路径格式: :/image/enemy/enemy_{type}_{state}.png
    // 如果不存在则尝试通用路径: :/image/enemy/enemy_{state}.png
    QString resourcePath = QString(":/image/enemy/enemy_%1_%2.png").arg(enemyType).arg(stateName);
    QPixmap pixmap(resourcePath);
    
    // 如果特定类型的图片不存在，尝试加载通用版本
    if (pixmap.isNull()) {
        resourcePath = QString(":/image/enemy/enemy_%1.png").arg(stateName);
        pixmap.load(resourcePath);
    }
    
    // 如果仍然不存在，返回默认图片
    if (pixmap.isNull()) {
        return getDefaultEnemyPixmap();
    }

    return pixmap.scaled(GameConfig::ENEMY_SIZE, GameConfig::ENEMY_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap ResourceManager::getEnemyPixmapByState(EnemyState state) const
{
    // 简化版本，仅按状态获取（不区分敌人类型）
    return getEnemyPixmap(0, state);
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

QPixmap ResourceManager::getTowerBasePixmap() const
{
    QPixmap pixmap(":/image/tower/towerbase.png");

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
    QPixmap pixmap(":/image/tower/tower.png");

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
    QPixmap mapPixmap(":/image/map.png");

    if (mapPixmap.isNull()) {
        // 如果找不到map.png，返回默认背景
        return getDefaultBackground();
    }

    // 将320x240的地图放大到800x600
    return mapPixmap.scaled(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QPixmap ResourceManager::getBulletPixmap() const
{
    QPixmap bulletPixmap(":/image/bullet/bullet.png");

    // 如果资源文件不存在，使用默认图片
    if (bulletPixmap.isNull())
    {
        return getDefaultBulletPixmap();
    }
    return bulletPixmap.scaled(GameConfig::GRID_SIZE / 2, GameConfig::GRID_SIZE / 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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

void ResourceManager::loadDefaultPixmaps()
{
    pixmapCache["enemy_default"] = getDefaultEnemyPixmap();
    pixmapCache["tower_default"] = getDefaultTowerPixmap();
    pixmapCache["background_default"] = getDefaultBackground();

    // 创建子弹图片
    QPixmap bulletPixmap(10, 10);
    bulletPixmap.fill(Qt::transparent);
    QPainter painter(&bulletPixmap);
    painter.setBrush(QBrush(Qt::blue));
    painter.setPen(QPen(Qt::darkBlue, 1));
    painter.drawEllipse(0, 0, 10, 10);
    pixmapCache["bullet_default"] = bulletPixmap;
}
