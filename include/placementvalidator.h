#ifndef PLACEMENTVALIDATOR_H
#define PLACEMENTVALIDATOR_H

#include "config.h"
#include <QSet>
#include <QPair>
#include <QVector>

class PlacementValidator
{
public:
    PlacementValidator();
    
    // 加载允许放置的网格配置
    void loadConfig(const QVector<GameConfig::GridPoint>& allowedGrids);
    
    // 检查指定像素坐标是否允许放置
    // gridX, gridY 是已经对齐网格的像素坐标，或者直接是像素坐标
    bool isPlacementAllowed(int pixelX, int pixelY) const;
    
    // 获取所有允许放置的网格索引 (x, y)
    const QSet<QPair<int, int>>& getAllowedGrids() const;

private:
    // 存储允许放置的网格索引 (gridIndexX, gridIndexY)
    QSet<QPair<int, int>> allowedGrids;
    int gridSize;
};

#endif // PLACEMENTVALIDATOR_H
