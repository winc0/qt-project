#include "include/placementvalidator.h"
#include <QDebug>

PlacementValidator::PlacementValidator()
    : gridSize(GameConfig::GRID_SIZE)
{
}

void PlacementValidator::loadConfig(const QVector<GameConfig::GridPoint>& grids)
{
    allowedGrids.clear();
    for (const auto& point : grids) {
        allowedGrids.insert(qMakePair(point.gridX, point.gridY));
    }
    qDebug() << "PlacementValidator loaded" << allowedGrids.size() << "allowed grids.";
}

bool PlacementValidator::isPlacementAllowed(int pixelX, int pixelY) const
{
    int indexX = pixelX / gridSize;
    int indexY = pixelY / gridSize;
    
    return allowedGrids.contains(qMakePair(indexX, indexY));
}

const QSet<QPair<int, int>>& PlacementValidator::getAllowedGrids() const
{
    return allowedGrids;
}
