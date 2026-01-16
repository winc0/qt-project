#ifndef MAINMENUPAGE_H
#define MAINMENUPAGE_H

#include "config.h"
#include <QWidget>
#include <QPixmap>

class QPushButton;
class QLabel;
class QVBoxLayout;

namespace Ui
{
class MainMenuPage;
}

class MainMenuPage : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenuPage(QWidget *parent = nullptr);
    ~MainMenuPage();

signals:
    void startGameRequested(GameConfig::MapId mapId);
    void exitGameRequested();
    void openLevelSelectRequested();

private slots:
    void onStartButtonClicked();
    void onExitButtonClicked();

private:
    void initUI();
    void loadResources();

    Ui::MainMenuPage *ui;

    QLabel *titleLabel;
    QLabel *backgroundLabel;
    QPushButton *startButton;
    QPushButton *exitButton;
    QVBoxLayout *buttonLayout;
    QVBoxLayout *mainLayout;

    QPixmap backgroundImage;
};

#endif
