#ifndef QUADTREE_H
#define QUADTREE_H

#include "enemy.h"
#include <QRectF>
#include <QList>
#include <QPointer>

// 基于象限划分的敌人空间索引
// AI-generated class
class Quadtree
{
public:
    // 创建给定边界与容量的树
    Quadtree(const QRectF& boundary, int capacity);
    // 递归销毁所有子节点
    ~Quadtree();

    // 将敌人插入到合适象限
    void insert(Enemy* enemy);
    // 在范围内查询可能命中的敌人
    void query(const QRectF& range, QList<Enemy*>& found) const;
    // 清空整棵树及其内容
    void clear();

private:
    QRectF boundary;
    int capacity;
    QList<Enemy*> enemies;
    bool divided;

    Quadtree* northwest;
    Quadtree* northeast;
    Quadtree* southwest;
    Quadtree* southeast;

    // 将当前节点拆分为四个子象限
    void subdivide();
};

#endif // QUADTREE_H
