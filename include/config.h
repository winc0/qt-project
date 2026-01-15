#ifndef CONFIG_H
#define CONFIG_H

#include <QHash>
#include <QVector>
#include <QPointF>

namespace GameConfig
{
    // 窗口设置
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;
    const int GRID_SIZE = 40;

    // 游戏设置
    const int INITIAL_LIVES = 20;
    const int INITIAL_GOLD = 300;
    const int ENEMY_REWARD = 15;

    // 敌人设置
    const int ENEMY_HEALTH = 100;
    const int ENEMY_SIZE = 30;
    const int ENEMY_MOVE_INTERVAL = 50; // ms
    const float ENEMY_SPEED = 1.0f;
    const int ENEMY_DEAD_KEEP_TIME = 500; // ms

    // 子弹设置
    const int BULLET_SIZE = 10;
    const int BULLET_MOVE_INTERVAL = 30; // ms
    const float BULLET_SPEED = 5.0f;

    // 防御塔设置
    namespace TowerStats
    {
        // 箭塔
        const int ARROW_DAMAGE = 20;
        const int ARROW_RANGE = 150;
        const int ARROW_COST = 100;
        const int ARROW_FIRE_RATE = 1000; // ms

        // 炮塔
        const int CANNON_DAMAGE = 50;
        const int CANNON_RANGE = 120;
        const int CANNON_COST = 200;
        const int CANNON_FIRE_RATE = 2000;
    }

    // 波次设置
    const int WAVE_ENEMY_COUNT = 10;
    const int WAVE_SPAWN_INTERVAL_MAX = 2000; // ms
    const int WAVE_SPAWN_INTERVAL_MIN = 500; // ms
    const int WAVE_SPAWN_INTERVAL_EACH = 200; // ms

    // 碰撞检测
    const float ENEMY_COLLISION_RADIUS = ENEMY_SIZE / 2.0f;
    const float BULLET_COLLISION_RADIUS = BULLET_SIZE / 2.0f;

    // 防御塔旋转设置
    const float TOWER_ROTATION_MAX_STEP = 5.0f;
    const int TOWER_TARGET_LOCK_MS = 400;
    const float BULLET_MAX_DISTANCE = 600.0f;
    const int BULLET_TARGET_LOST_TIMEOUT_MS = 400;

    struct EndPointConfig
    {
        qreal x;
        qreal y;
        qreal radius;
    };

    enum MapId
    {
        MAP_DEFAULT = 0
    };

    // 定义路径点数据结构，使用网格坐标
    struct GridPoint
    {
        int gridX; // 网格列数
        int gridY; // 网格行数
    };

    namespace MapPaths
    {
        // 使用网格坐标定义路径，更清晰易维护
        const QVector<GridPoint> DEFAULT_PATH = {
            {16, 6},
            {16, 7},
            {11, 7},
            {11, 6},
            {4, 6},
            {4, 7},
            {7, 7},
            {7, 11},
            {9, 11},
            {9, 9},
            {16, 9},
        };

        // 地图ID到路径的映射
        const QHash<MapId, QVector<GridPoint>> PATH_MAP = {
            {MAP_DEFAULT, DEFAULT_PATH},
        };
    }

    namespace Placement
    {
        const QVector<GridPoint> DEFAULT_BUILDABLE_GRIDS = {
            {13, 6},
            {14, 6},
            {15, 6},

            {8, 7},
            {9, 7},
            {10, 7},
            {8, 8},
            {9, 8},

            {4, 8},
            {5, 8},
            {6, 8},
            {6, 9},
            {6, 10},

            {11, 8},
            {12, 8},
            {13, 8},
            {14, 8},
            {15, 8},
            {16, 8},

            {10, 10},
            {11, 10},
            {12, 10},
            {13, 10},
            {14, 10},
            {15, 10},
            {16, 10},
        };

        const QHash<MapId, QVector<GridPoint>> BUILDABLE_MAP = {
            {MAP_DEFAULT, DEFAULT_BUILDABLE_GRIDS},
        };
    }

    const int TIP_DURATION_MS = 2000;

}

#endif // CONFIG_H
