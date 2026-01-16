#ifndef GAMEPAGE_H
#define GAMEPAGE_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QList>
#include <QTimer>
#include <QElapsedTimer>
#include <QColor>

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
class QGraphicsOpacityEffect;

namespace Ui
{
class GamePage;
}

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
    void updateGameStats();
    void drawPlacementAreas();
    void showFloatingTip(const QString &text, const QPointF &scenePos, const QColor &color);
    void showUpgradeEffect(const QPointF &scenePos);
    
    // 敌人管理
    void pauseAllEnemies();
    void resumeAllEnemies();
    
    // 塔和子弹管理
    void pauseAllTowersAndBullets();
    void resumeAllTowersAndBullets();

    struct ResultViewContext
    {
        QWidget *overlay;
        QWidget *panel;
        QVBoxLayout *layout;
        QGraphicsOpacityEffect *opacityEffect;
    };

    ResultViewContext createResultWrapper(const QString &panelStyle, const QColor &shadowColor);
    void playResultAnimation(const ResultViewContext &ctx);

    void mousePressEvent(QMouseEvent *event) override;

    Ui::GamePage *ui;

    QGraphicsScene *gameScene;
    QGraphicsView *gameView;
    QGraphicsPixmapItem *userItem;
    PlacementValidator *placementValidator;
    GameManager *gameManager;

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

    QVBoxLayout *mainLayout;
    QHBoxLayout *infoLayout;
    QHBoxLayout *buttonLayout;

    QVector<QPointF> pathPoints;

    GameConfig::MapId currentMapId;
    QVector<GameConfig::EndPointConfig> endPointAreas;
    QElapsedTimer elapsedTimer;
    QList<QGraphicsRectItem *> placementAreaItems;
};

#endif
