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
#include "gamemanager.h"

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
    void setMap(GameConfig::MapId mapId);

signals:
    void gameOver();
    void returnToMainMenu();

public slots:
protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    void initGameScene();
    void initUI();
    void createPath();
    void showGameOverDialog();
    void showLevelCompleteDialog();
    void showPauseMenu();
    void hidePauseMenu();
    void saveLevelProgress(bool levelCompleted);

    // 绘制游戏元素
    void drawBackground();
    void drawGrid();

    // 游戏逻辑（UI 相关）
    void updateHoverHighlight(const QPointF &scenePos);
    void initPlacementValidator();
    void drawPlacementAreas();
    void showFloatingTip(const QString &text, const QPointF &scenePos, const QColor &color);
    void showUpgradeEffect(const QPointF &scenePos);
    
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
    QGraphicsPixmapItem *userItem;
    PlacementValidator *placementValidator;
    GameManager *gameManager;

    // UI元素
    QWidget *controlPanel;
    QLabel *goldLabel;
    QLabel *livesLabel;
    QLabel *waveLabel;
    QPushButton *pauseButton;
    QPushButton *returnButton;
    QWidget *resultOverlay;
    QWidget *resultPanel;
    QWidget *pauseOverlay;
    QWidget *pausePanel;

    // 布局
    QVBoxLayout *mainLayout;
    QHBoxLayout *infoLayout;
    QHBoxLayout *buttonLayout;

    // 路径点
    QVector<QPointF> pathPoints;

    GameConfig::MapId currentMapId;
    QVector<GameConfig::EndPointConfig> endPointAreas;
    QElapsedTimer elapsedTimer;
    QList<QGraphicsRectItem *> placementAreaItems;
};

#endif
