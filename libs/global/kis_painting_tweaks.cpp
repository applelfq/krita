/*
 *  Copyright (c) 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_painting_tweaks.h"



#include <QPen>
#include <QRegion>
#include <QPainter>
#include <QTransform>

#include "kis_debug.h"


namespace KisPaintingTweaks {

QRegion safeClipRegion(const QPainter &painter)
{
    const QTransform t = painter.transform();

    QRegion region = t.type() <= QTransform::TxScale ?
        painter.clipRegion() :
        QRegion(painter.clipBoundingRect().toAlignedRect());

    if (region.rectCount() > 1000) {
        qWarning() << "WARNING: KisPaintingTweaks::safeClipRegion: too many rectangles in the region!" << ppVar(region.rectCount());
        region = QRegion(painter.clipBoundingRect().toAlignedRect());
    }

    return region;
}

QRect safeClipBoundingRect(const QPainter &painter)
{
    return painter.clipBoundingRect().toAlignedRect();
}

void initAntsPen(QPen *antsPen, QPen *outlinePen,
                 int antLength, int antSpace)
{
    QVector<qreal> antDashPattern;
    antDashPattern << antLength << antSpace;

    *antsPen = QPen(Qt::CustomDashLine);
    antsPen->setDashPattern(antDashPattern);
    antsPen->setCosmetic(true);
    antsPen->setColor(Qt::black);

    *outlinePen = QPen(Qt::SolidLine);
    outlinePen->setCosmetic(true);
    outlinePen->setColor(Qt::white);
}

PenBrushSaver::PenBrushSaver(QPainter *painter)
    : m_painter(painter),
      m_pen(painter->pen()),
      m_brush(painter->brush())
{
}

PenBrushSaver::PenBrushSaver(QPainter *painter, const QPen &pen, const QBrush &brush)
    : PenBrushSaver(painter)
{
    m_painter->setPen(pen);
    m_painter->setBrush(brush);
}

PenBrushSaver::PenBrushSaver(QPainter *painter, const QPair<QPen, QBrush> &pair)
    : PenBrushSaver(painter)
{
    m_painter->setPen(pair.first);
    m_painter->setBrush(pair.second);
}

PenBrushSaver::PenBrushSaver(QPainter *painter, const QPair<QPen, QBrush> &pair, allow_noop_t)
    : m_painter(painter)
{
    if (m_painter) {
        m_pen = m_painter->pen();
        m_brush = m_painter->brush();
        m_painter->setPen(pair.first);
        m_painter->setBrush(pair.second);
    }
}

PenBrushSaver::~PenBrushSaver()
{
    if (m_painter) {
        m_painter->setPen(m_pen);
        m_painter->setBrush(m_brush);
    }
}


}
