#ifndef CONFIG_H
#define CONFIG_H

namespace GameConfig {
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
    namespace TowerStats {
        // 箭塔
        const int ARROW_DAMAGE = 20;
        const int ARROW_RANGE = 150;
        const int ARROW_COST = 100;
        const int ARROW_FIRE_RATE = 1000; // ms

        // 炮塔
        const int CANNON_DAMAGE = 50;
        const int CANNON_RANGE = 120;
        const int CANNON_COST = 200;
        const int CANNON_FIRE_RATE = 2000; // ms
    }

    // 波浪设置
    const int WAVE_ENEMY_COUNT = 10;
    const int WAVE_SPAWN_INTERVAL_MAX = 2000; // ms
    const int WAVE_SPAWN_INTERVAL_MIN = 500; // ms
}

#endif // CONFIG_H
