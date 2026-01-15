#ifndef GAMEPAGE_H
#define GAMEPAGE_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QList>
#include <QTimer>
#include <QElapsedTimer>

#include "enemy.h"
#include "tower.h"
#include "bullet.h"
#include "config.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QGraphicsRectItem;
class PlacementValidator;

class GamePage : public QWidget
{
    Q_OBJECT

public:
    explicit GamePage(QWidget *parent = nullptr);
    ~GamePage();

    void startGame();
    void pauseGame();
    void resetGame();

signals:
    void gameOver();
    void returnToMainMenu();

public slots:
    void spawnEnemy();
    void updateGame();
protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    void initGameScene();
    void initUI();
    void createPath();
    void checkGameOver();
    void showGameOverDialog();

    // 绘制游戏元素
    void drawBackground();
    void drawGrid();

    // 游戏逻辑
    void updateEnemies();
    void updateTowers();
    void removeDeadEntities();
    void checkNextWave();  // 检查是否应该进入下一波
    int getWaveSpawnInterval() const;  // 根据当前波次获取敌人生成间隔
    bool isEnemyAtAnyEndPoint(QPointer<Enemy> enemy) const;
    void updateHoverHighlight(const QPointF &scenePos);
    void initPlacementValidator();
    void drawPlacementAreas();
    void showFloatingTip(const QString &text, const QPointF &scenePos, const QColor &color);
    
    // 敌人管理
    void pauseAllEnemies();
    void resumeAllEnemies();
    
    // 塔和子弹管理
    void pauseAllTowersAndBullets();
    void resumeAllTowersAndBullets();

    // 鼠标事件
    void mousePressEvent(QMouseEvent *event) override;

    // 游戏状态
    QGraphicsScene *gameScene;
    QGraphicsView *gameView;
    PlacementValidator *placementValidator;

    // 游戏元素容器
    QList<QPointer<Enemy>> enemies;
    QList<QPointer<Tower>> towers;
    QList<QPointer<Bullet>> bullets;

    // 游戏资源
    int gold;
    int lives;
    int currentWave;
    int enemiesSpawnedThisWave;
    bool waveSpawnComplete;  // 标记当前波次是否已经生成所有敌人

    // 定时器
    QTimer *gameTimer;
    QTimer *enemySpawnTimer;

    // UI元素
    QWidget *controlPanel;
    QLabel *goldLabel;
    QLabel *livesLabel;
    QLabel *waveLabel;
    QPushButton *pauseButton;
    QPushButton *returnButton;
    QWidget *resultOverlay;
    QWidget *resultPanel;

    // 布局
    QVBoxLayout *mainLayout;
    QHBoxLayout *infoLayout;
    QHBoxLayout *buttonLayout;

    // 路径点
    QVector<QPointF> pathPoints;

    bool isPaused;
    bool gameRunning;
    GameConfig::MapId currentMapId;
    QVector<GameConfig::EndPointConfig> endPointAreas;
    int killCount;
    QElapsedTimer elapsedTimer;
    QList<QGraphicsRectItem *> placementAreaItems;
};

#endif
