//
//  PhylogenyTreeView.cpp
//  gavqt
//
//  Created by Brian Ondov on 8/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "PhylogenyTreeView.h"
#include <stdlib.h>
#include <QtCore>

inline float absf(float x) {return x > 0 ? x : -x;}

PhylogenyTreeView::PhylogenyTreeView()
{
	phylogenyTree = 0;
	penDark.setColor(qRgb(0, 0, 0));
	penLight.setColor(qRgb(200, 200, 200));
//	buffer = 0;
	nameBuffers = 0;
	nameBufferCount = 0;
	nodeViews = 0;
	leafHues = 0;
	highlightNode = 0;
	focusNode = 0;
	rightAlign = false;
	rightAlignLast = rightAlign;
	rightAlignText = false;
	useGradient = false;
	leafLines = false;
	showDots = false;
	
	const int gradientWidth = 50;
	
	for ( int i = 0; i < colorCount; i++ )
	{
		colors[i] = QColor::fromHsl(i * 180 % 360, 50, 230).rgb();
		gradients[i] = new QPixmap(gradientWidth, 1);
		gradientsHover[i] = new QPixmap(gradientWidth, 1);
		gradientsHighlight[i] = new QPixmap(gradientWidth, 1);
		gradientsSearch[i] = new QPixmap(gradientWidth, 1);
		
		fillGradient(gradients[i], Qt::white, colors[i]);
		fillGradient(gradientsHover[i], Qt::cyan, colors[i]);
		fillGradient(gradientsHighlight[i], Qt::magenta, colors[i]);
		fillGradient(gradientsSearch[i], Qt::yellow, colors[i]);
	}
}

PhylogenyTreeView::~PhylogenyTreeView()
{
	if ( nameBuffers )
	{
		for ( int i = 0; i < nameBufferCount; i++ )
		{
			delete nameBuffers[i];
		}
		
		delete [] nameBuffers;
	}
	
	if ( nodeViews )
	{
		delete [] nodeViews;
	}
	
	if ( leafHues )
	{
		delete [] leafHues;
	}
	
	for ( int i = 0; i < colorCount; i++ )
	{
		delete gradients[i];
		delete gradientsHover[i];
		delete gradientsHighlight[i];
		delete gradientsSearch[i];
	}
}

void PhylogenyTreeView::handleTrackHeightChange(const TrackListView * focus)
{
	TrackListView::handleTrackHeightChange(focus);
	redrawNeeded = true;
}

void PhylogenyTreeView::setNames(QVector<QString> *newNames)
{
	names = newNames;
	
	if ( nameBuffers )
	{
		for ( int i = 0; i < nameBufferCount; i++ )
		{
			delete nameBuffers[i];
		}
		
		delete [] nameBuffers;
	}
	
	nameBufferCount = names->size();
	nameBuffers = new QImage * [nameBufferCount];
	
	QFont defaultFont;
	defaultFont.setPixelSize(20);
	QFontMetrics fm(defaultFont);
	
	for ( int i = 0; i < nameBufferCount; i++ )
	{
		int width = fm.width((*names)[i]);
		fontHeight = fm.height();
		int height = fontHeight;
		
		if ( width & 1 )
		{
			width++;
		}
		
		if ( height & 1 )
		{
			height++;
		}
		
		nameBuffers[i] = new QImage(width, height, QImage::Format_ARGB32);
		QPainter painter(nameBuffers[i]);
		painter.setFont(defaultFont);
		nameBuffers[i]->fill(qRgba(0, 0, 0, 0));
		painter.drawText(nameBuffers[i]->rect(), (*names)[i]);
	}
}

void PhylogenyTreeView::setPhylogenyTree(const PhylogenyTree * newTree)
{
	phylogenyTree = newTree;
	
	if ( nodeViews )
	{
		delete [] nodeViews;
	}
	
	if ( leafHues )
	{
		delete leafHues;
	}
	
	nodeViews = new PhylogenyNodeView[phylogenyTree->getNodeCount()];
	leafHues = new float[getTrackCount()];
	redrawNeeded = true;
	
	for ( int i = 0; i < phylogenyTree->getNodeCount(); i++ )
	{
		nodeViews[i].search = false;
		nodeViews[i].group = 0;
	}
	
	float maxDist = 0;
	
	leafHues[0] = 0;
	
	for ( int i = 1; i < getTrackCount(); i++ )
	{
		leafHues[i] = phylogenyTree->leafDistance(i - 1, i);
		
		if ( leafHues[i] > maxDist )
		{
			maxDist = leafHues[i];
		}
	}
	
	float factor = 240. / maxDist;
	
	for ( int i = 0; i < getTrackCount(); i++ )
	{
		leafHues[i] = 60 + factor * leafHues[i];
	}
	
	setWindow(phylogenyTree->getRoot());
	setWindow(phylogenyTree->getRoot(), true);
	focusNode = phylogenyTree->getRoot();
}

void PhylogenyTreeView::setRightAlignNodes(bool align)
{
	rightAlign = align;
	emit signalTrackFocusChange(getTrackFocus()); // HACK
}

void PhylogenyTreeView::setTrackFocus(int track)
{
	xFactorStart = xFactorEnd;
	xOffsetStart = xOffsetEnd;
	TrackListView::setTrackFocus(track);
}

void PhylogenyTreeView::setTrackHover(int track, int trackEnd)
{
	TrackListView::setTrackHover(track, trackEnd);
	
	if ( track == trackEnd )
	{
		if ( track == -1 )
		{
			highlightNode = 0;
		}
		else
		{
			highlightNode = phylogenyTree->getLeaf(track);
		}
	}
}

float PhylogenyTreeView::getHighlight(const PhylogenyNode *, float highlight, bool) const
{
	return highlight;
}

void PhylogenyTreeView::groupNodes(const PhylogenyNode * node)
{
	int groupsTarget = 7;
	
	QVector<const PhylogenyNode *> groups;
	QVector<bool> split(groupsTarget, false);
	
	groups.push_back(node);
	
	for ( int i = 0; i < phylogenyTree->getNodeCount(); i++ )
	{
		nodeViews[i].group = 0;
	}
	
	int groupMax = 0;
	
	while ( groups.size() < groupsTarget && groupMax >= 0 )
	{
		float weightMax = 0;
		groupMax = -1;
		
		for ( int i = 0; i < groups.size(); i++ )
		{
			const PhylogenyNode * group = groups[i];
			float weight = 0;
			
			if ( groups[i]->getChildrenCount() && groups.size() + groups[i]->getChildrenCount() - 0 <= groupsTarget && ! split[i] )
			{
				for ( int j = 0; j < group->getChildrenCount(); j++ )
				{
					weight += group->getChild(j)->getLeafCount() * group->getChild(j)->getDistance();
				}
				
				weight /= group->getChildrenCount() * group->getDistance();
				
				if ( weight > weightMax )
				{
					weightMax = weight;
					groupMax = i;
				}
			}
		}
		
		if ( weightMax )
		{
			for ( int i = 0; i < groups[groupMax]->getChildrenCount(); i++ )
			{
				groups.push_back(groups[groupMax]->getChild(i));
			}
			
			//groups.remove(groupMax);
			split[groupMax] = true;
		}
	}
	
	for ( int i = 0; i < groups.size(); i++ )
	{
		nodeViews[groups[i]->getId()].group = i + 1;
	}
}

void PhylogenyTreeView::leaveEvent(QEvent *event)
{
	TrackListView::leaveEvent(event);
	
	highlightNode = 0;
}

void PhylogenyTreeView::paintEvent(QPaintEvent *event)
{
	TrackListView::paintEvent(event);
	
	if ( phylogenyTree == 0 )
	{
		return;
	}
	
	xFactor = zoomLerp(xFactorStart, xFactorEnd);
	xOffset = zoomLerp(xOffsetStart, xOffsetEnd);
	
	if ( redrawNeeded || bufferHighlight )
	{
		setBufferUpdateNeeded();
	}
	
//	painter.drawImage(frameWidth(), frameWidth(), *buffer);
	
	if ( ! redrawNeeded && highlightNode )
	{
		QPainter painter(this);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.save();
		painter.translate(1, 1);
		drawNode(&painter, highlightNode, true, 0, nodeViews[highlightNode->getId()].getX());
		
		if ( getTrackFocus() != -1 )
		{
			drawNode(&painter, phylogenyTree->getLeaf(getTrackFocus()), true, 0, nodeViews[phylogenyTree->getLeaf(getTrackFocus())->getId()].getX());
		}
		painter.restore();
	}
	
	redrawNeeded = false;
}

void PhylogenyTreeView::resizeEvent(QResizeEvent * event)
{
	TrackListView::resizeEvent(event);
	radius = height() / 10;
	/*
	if ( buffer )
	{
		delete buffer;
	}
	
	buffer = new QImage(getWidth(), getHeight(), QImage::Format_RGB32);
	redrawNeeded = true;*/
}

void PhylogenyTreeView::setWindow(const PhylogenyNode *node, bool initialize)
{
	int nameWidth = 200;
	
	if ( nameWidth > 200 )
	{
		nameWidth = 200;
	}
	
	if ( nameWidth > getWidth() / 3 )
	{
		nameWidth = getWidth() / 3;
	}
	
	xFactorStart = xFactorEnd;
	xOffsetStart = xOffsetEnd;
	
	float leafSize = (float)getHeight() / node->getLeafCount();
	xFactorEnd = (getWidth() - (leafSize >= 8 || getTrackFocus() != -1 ? nameWidth : 25)) / (maxVisibleDepth(node, leafSize) - getNodeDepth(node));
	xOffsetEnd = -xFactorEnd * getNodeDepth(node);
	
	setTargetsNodeView(phylogenyTree->getRoot(), initialize);
}

void PhylogenyTreeView::updateBuffer()
{
	if ( phylogenyTree == 0 )
	{
		clearBuffer();
		return;
	}
	
	updateNodeViews(phylogenyTree->getRoot());
	QPainter painterBuffer(imageBuffer);
	painterBuffer.setRenderHint(QPainter::SmoothPixmapTransform);
	painterBuffer.setRenderHint(QPainter::Antialiasing);
	painterBuffer.fillRect(0, 0, getWidth(), getHeight(), qRgb(255, 255, 255));
	QLinearGradient gradient(0, 0, getWidth(), 0);
	gradient.setColorAt(0, Qt::cyan);
	gradient.setColorAt(1, Qt::blue);
//	painterBuffer.fillRect(0, 0, getWidth(), getHeight(), gradient);
	
	for ( int i = 0; i < getTrackCount(); i++ )
	{
		const QColor color = QColor::fromHsl(i * 225 % 360, 50, 220).rgb();
		//painterBuffer.fillRect(0, getTrackHeight(i), getWidth(), getTrackHeight(i) - getTrackHeight(i - 1), color);
	}
//	printf("xFactor: %f  xOffset: %f  ")
	//clearBuffer();
	drawNode(&painterBuffer, phylogenyTree->getRoot(), redrawNeeded);
	bufferHighlight = redrawNeeded && highlightNode;
	
	if ( getTrackFocus() != -1 )
	{
		drawNode(&painterBuffer, phylogenyTree->getLeaf(getTrackFocus()), true, 0, nodeViews[phylogenyTree->getLeaf(getTrackFocus())->getId()].getX());
	}
}

void PhylogenyTreeView::drawLine(QPainter * painter, float x1, float y1, float x2, float y2, float weight, QColor color, bool dash) const
{
	if ( weight == 0 )
	{
		return;
	}
	
	if ( x1 != x2 && y1 != y2 )
	{
		if ( x1 < x2 )
		{
			x1 += .25;
			x2 -= .75;
		}
		else
		{
			x1 -= .75;
			x2 += .25;
		}
		
		if ( y1 < y2 )
		{
			y1 += .25;
			y2 -= .75;
		}
		else
		{
			y1 -= .75;
			y2 += .25;
		}
	}
	
	QPen pen;
	pen.setColor(qRgb(color.red(), color.green(), color.blue()));
	pen.setWidthF(weight * 1.5);
	
	if ( dash )
	{
		QVector<qreal> dashPattern;
		dashPattern << .5 << 1 + weight * 2;
		//pen.setStyle(Qt::DotLine);
		pen.setDashPattern(dashPattern);
	}
	painter->setPen(pen);
	painter->drawLine(QLineF((float)x1 + .5, (float)y1 + .5, (float)x2 + .5, (float)y2 + .5));
	
	return;
	
	if ( weight > .3 )
	{
		QPen pen;
		
		if ( x1 == x2 || y1 == y2 )
		{
			pen.setWidth(3);
			pen.setCapStyle(Qt::FlatCap);
		}
		else
		{
			pen.setWidth(2);
			pen.setCapStyle(Qt::SquareCap);
		}
		
		pen.setColor(QColor::fromRgba(qRgba(color.red(), color.green(), color.blue(), 255 * ((weight - .3) / 3 + .1))));
		painter->setPen(pen);
		painter->drawLine(x1, y1, x2, y2);
		pen.setCapStyle(Qt::SquareCap);
		pen.setWidth(1);
		pen.setColor(color);
		painter->setPen(pen);
		painter->drawLine(x1, y1, x2, y2);
	}
	else
	{
		QPen pen;
		
		if ( x1 == x2 || y1 == y2 )
		{
			pen.setWidth(3);
			pen.setCapStyle(Qt::FlatCap);
		}
		else
		{
			pen.setWidth(2);
			pen.setCapStyle(Qt::SquareCap);
		}
		
		pen.setColor(QColor::fromRgba(qRgba(color.red(), color.green(), color.blue(), 255 * (weight) / 3)));
		painter->setPen(pen);
		painter->drawLine(x1, y1, x2, y2);
		pen.setCapStyle(Qt::SquareCap);
		pen.setWidth(1);
		pen.setColor(QColor::fromRgba(qRgba(color.red(), color.green(), color.blue(), 255 * (weight / .3))));
		painter->setPen(pen);
		painter->drawLine(x1, y1, x2, y2);
	}
}


void PhylogenyTreeView::drawNode(QPainter * painter, const PhylogenyNode *node, bool drawHighlight, float highlight, int xLeft, float weightTop, float weightBottom, int group) const
{
	PhylogenyNodeView & nodeView = nodeViews[node->getId()];
	
	float childSize = getTrackHeight(node->getLeafMax() + 1) - getTrackHeight(node->getLeafMin());
	int x = nodeView.getXAlign();
	int xAlign = nodeView.getX();
	
	highlight = getHighlight(node, highlight, drawHighlight);
	
	float weight;
	float maxHeight = 150;
	float minHeight = 2;
	
	if
	(
		childSize >= maxHeight ||
		node == highlightNode
		//(getTrackFocus() >= nodeView.leafMin && getTrackFocus() <= nodeView.leafMax) ||
		//(getTrackHover() == nodeView.leafMin || getTrackHoverEnd() == nodeView.leafMax)
	)
	{
		weight = 1;
	}
	else if ( childSize < minHeight )
	{
		weight = 0;
	}
	else
	{
		weight = 1 - (maxHeight - childSize) / (maxHeight - minHeight);
	}
	
	QPen pen;
	
	int bootShade = 200;//255. * (1. - node->getBootstrap());
	
	if ( node->getBootstrap() == 0 || node->getBootstrap() > .5 )
	{
		bootShade = 0;
	}
	
	if ( nodeView.group != 0 )
	{
		//group = nodeView.group;
	}
	
	if( node->getChildrenCount() && ! node->getCollapse() )
	{
		int childMinY = nodeViews[node->getChild(0)->getId()].y;
		int childMaxY = nodeViews[node->getChild(node->getChildrenCount() - 1)->getId()].y;
		
//		painter->fillRect(x + 1, childMinY, getWidth() - x - 1, childMaxY - childMinY, qRgb(245, bootShade, bootShade));
		if ( useGradient )
		{
			painter->fillRect(x + 1, childMinY, getWidth() - x - 1, childMaxY - childMinY, gradient((node->getDepth()) * xFactor + xOffset));
		}
		
		if ( highlight || group != 0 )
		{
//			painter->setOpacity(1 - getZoomProgress());
			QColor color;
			
			if ( highlight )
			{
				color = highlightColor(highlight);
			}
			else
			{
				const int hues[7] = {30, 60, 120, 180, 220, 270, 300};
				color = QColor::fromHsl(hues[group], 100, 200).rgb();
			}
			
			painter->fillRect(x + 1, childMinY, getWidth() - x - 1, childMaxY - childMinY, color);
//			painter->setOpacity(1);
		}
		
		//pen.setColor(QColor::fromRgba(qRgba(bootShade, 0, 0, 255 * maxf(weight, weightTop))));
		//painter->setPen(pen);
//		painter->drawLine(x, childMinY, x, nodeView.y);
		//drawLine(painter, x, childMinY, x, nodeView.y, maxf(weight, weightTop), qRgb(bootShade, 0, 0));
		
		//pen.setColor(QColor::fromRgba(qRgba(bootShade, 0, 0, 255 * maxf(weight, weightBottom))));
		//painter->setPen(pen);
//		painter->drawLine(x, nodeView.y, x, childMaxY);
		//drawLine(painter, x, childMaxY, x, nodeView.y, maxf(weight, weightBottom), qRgb(bootShade, 0, 0));
	}
	else
	{
		drawNodeLeaf(painter, node, highlight, maxf(weight, weightTop), maxf(weight, weightBottom));
	}
	
	for( int i = 0; i < node->getChildrenCount(); i++ )
	{
		float weightChildTop = 0;
		float weightChildBottom = 0;
		
		if ( i == 0 )
		{
			weightChildTop = maxf(weight, weightTop);
		}
		
		if ( i == node->getChildrenCount() - 1 )
		{
			weightChildBottom = maxf(weight, weightBottom);
		}
		
		drawNode(painter, node->getChild(i), drawHighlight, highlight, x, weightChildTop, weightChildBottom, group);
	}
	
	if( node->getChildrenCount() && ! node->getCollapse() )
	{
		int childMinY = nodeViews[node->getChild(0)->getId()].y;
		int childMaxY = nodeViews[node->getChild(node->getChildrenCount() - 1)->getId()].y;
		
		drawLine(painter, x, childMinY, x, nodeView.y, maxf(weight, weightTop), qRgb(bootShade, 0, 0));
		drawLine(painter, x, childMaxY, x, nodeView.y, maxf(weight, weightBottom), qRgb(bootShade, 0, 0));
	}
	else
	{
		drawNodeLeaf(painter, node, highlight, maxf(weight, weightTop), maxf(weight, weightBottom));
	}
	int y = nodeView.y;
	
	float maxWeight = maxf(maxf(weightTop, weightBottom), weight);
	
	pen.setColor(QColor::fromRgba(qRgba(bootShade, 0, 0, 255 * maxWeight)));
	painter->setPen(pen);
	
//	int xLeft = xOffset + xFactor * (getNodeDepth(node) - getNodeAlignDist(node)) + 1;
//	int xLeftAlign = xOffset + xFactor * (getNodeDepth(node) - node->getDistance() - getNodeAlignDist(node)) + 1;
	
	if ( xLeft < xAlign )
	{
//		painter->drawLine(xLeft, y, x, y);
		drawLine(painter, xLeft, y, xAlign, y, maxWeight, qRgb(bootShade, 0, 0));
	}
	
	if ( xAlign < x )
	{
		drawLine(painter, xAlign, y, x, y, maxWeight, useGradient ? Qt::black : qRgb(0, 140, 140), true);
	}
	
	//float dist = qSqrt(qPow(getCursorX() - x, 2) + qPow(getCursorY() - y, 2));
	
	int radius = 6;
	if ( false && ! highlightNode && node->getChildrenCount() && getCursorX() >= x - radius && getCursorX() <= x + radius && getCursorY() >= y - radius && getCursorY() <= y + radius )//dist < 20)
	{
		int shadeBox = 255;//dist < 10 ? 255 : 255 - 256 * (dist - 10) / 10;
		
//		highlightNode = node;
		painter->fillRect(x - radius, y - radius, radius * 2, radius * 2, QColor::fromRgba(qRgba(255, 255, 255, shadeBox)));
		painter->setPen(QColor::fromRgba(qRgba(0, 0, 0, shadeBox)));
		painter->drawRect(x - radius, y - radius, radius * 2, radius * 2);
		painter->drawLine(x - radius / 2, y, x + radius / 2, y);
	}
}

void PhylogenyTreeView::drawNodeLeaf(QPainter * painter, const PhylogenyNode * node, float highlight, float weightTop, float weightBottom) const
{
	PhylogenyNodeView & nodeView = nodeViews[node->getId()];
	
	int leaf = node->getLeafMin();
	int x = nodeView.getXAlign();
	int y = nodeView.y;
	/*
	int nameWidth = 200;
	
	if ( nameWidth > 200 )
	{
		nameWidth = 200;
	}
	
	if ( nameWidth > getWidth() / 3 )
	{
		nameWidth = getWidth() / 3;
	}
	
	float scale = (getTrackHeight(leaf + 1) - getTrackHeight(leaf)) * 1 / fontHeight; // TEMP
	if ( scale > .5 )
	{
		scale = .5;
	}
	*/
	//float leafSize = (float)getHeight() / focusNode->getLeafCount();
	//x = getWidth() - (leafSize >= 5 || getTrackFocus() != -1 ? nameWidth : 0);
	//x = getWidth() - 200;
	
	float childSize = getTrackHeight(leaf + 1) - getTrackHeight(leaf);
	
	int shade;
	
	if
	(
		childSize >= 10 ||
		getTrackFocus() == leaf ||
		highlight
	)
	{
		shade = 255;
	}
	else if ( childSize < 1 )
	{
		shade = 100;
		//return;
	}
	else
	{
		shade = 256 - 150 * (10 - childSize) / 9;
	}
	
	int top = getTrackHeight(leaf);
	int bottom = getTrackHeight(leaf + 1);
	
	int maxRadius = 9;
	
	if ( y - top > maxRadius )
	{
		//top = y - maxRadius;
	}
	
	if ( bottom - y > maxRadius )
	{
		//bottom = y + maxRadius;
	}
	
	int height = bottom - top;
	int radius = (getTrackHeight(leaf + 1) - getTrackHeight(leaf)) / 2;
	int right = getWidth() - 1;
	
	int bevelTop = y;
	int bevelBottom = y;
	
	if ( bevelTop - top > maxRadius )
	{
		bevelTop = top + maxRadius;
	}
	
	if ( bottom - bevelBottom > maxRadius )
	{
		bevelBottom = bottom - maxRadius;
	}
	
	if ( radius > maxRadius )
	{
		radius = maxRadius;
	}
	
	//x += nameBuffers[getIdByTrack(leaf)]->width() * scale; // TEMP
	
	int bevelTopLeft = x + bevelTop - top;
	int bevelTopRight = getWidth() + top - bevelTop - 1;
	int bevelBottomLeft = x + bottom - bevelBottom;
	int bevelBottomRight = getWidth() + bevelBottom - bottom - 1;
	int rectLeft = bevelBottomLeft < bevelTopLeft ? bevelBottomLeft : bevelTopLeft;
	int rectWidth = bevelBottomLeft < bevelTopLeft ? bevelBottomRight - bevelBottomLeft : bevelTopRight - bevelTopLeft;
	//		shade = (shade + 230 * 1) / 2;
	//pen.setColor(qRgb(shade, shade, shade));
	QColor color;
	
	if ( nodeView.search )
	{
		if ( highlight )
		{
			color = qRgb(255, 255, 180);
		}
		else
		{
			color = qRgb(255, 255, 0);
		}
		
		shade = 255;
	}
	else if ( false && leaf == getTrackFocus() )
	{
		if ( highlight )
		{
			color = qRgb(230, 255, 255);
		}
		else
		{
			color = qRgb(200, 255, 255);
		}
		
		shade = 255;
	}
	else
	{
		int brightness;
		
		if ( highlight )
		{
			brightness = 235;
			color = highlightColor(highlight);
		}
		else
		{
			brightness = 220;
			color = Qt::white;
		}
		
		brightness = 230 + highlight * 0;
		
//		color = gradient((node->getDepth()) * xFactor + xOffset);
//		color = qRgb(0, 255 - 255 * (float)x / getWidth(), 255);
//		color = QColor::fromHsl(leaf * 180 % 360, 50, brightness).rgb();
		//color = QColor::fromHsl(leafHues[leaf], 50, brightness).rgb();
	}
	
	QPen pen;
	/*
	color.setAlpha(shade);
	if ( useGradient ) color.setAlpha(255);
	QPainterPath path;
	path.moveTo(x + 1, bevelTop);
	path.lineTo(bevelTopLeft, top);
	path.lineTo(bevelBottomLeft, bottom);
	
	if ( bevelTop != bevelBottom )
	{
		path.lineTo(x, bevelBottom);
	}
	
	path.closeSubpath();
	painter->fillPath(path, color);
	painter->setPen(pen);
	//painter->fillRect(rectLeft, top + 1, rectWidth + 10, height - 1, color);
	
	QPainterPath path2;
	path2.moveTo(right, bevelTop);
	path2.lineTo(bevelTopRight, top);
	path2.lineTo(bevelBottomRight, bottom);
	
	if ( bevelTop != bevelBottom )
	{
		path2.lineTo(right, bevelBottom);
	}
	
	path2.closeSubpath();
	painter->fillPath(path2, color);
	
	//QColor colorLine = QColor::fromHsl(leaf * 210 % 360, 50, 150).rgb();
	QColor colorLine = qRgb(0, 0, 0);
	
	//drawLine(painter, nodeView.getX(), y, x, y, maxf(weightTop, weightBottom), qRgb(120, 120, 120), true);
	
	if ( leaf == getTrackFocus() )
	{
		colorLine = nodeView.search ? qRgb(0, 200, 200) : qRgb(0, 128, 128);
		weightTop = 1;
		weightBottom = 1;
	}
	
	if ( ! leafLines )
	{
		if ( getTrackHover() != leaf )
		{
			weightTop = minf(weightTop, weightBottom);
		}
		
		if ( getTrackHoverEnd() != leaf )
		{
			weightBottom = minf(weightTop, weightBottom);
		}
	}
	
	drawLine(painter, x, bevelTop, bevelTopLeft, top, weightTop, colorLine);
	drawLine(painter, bevelTopLeft, top, bevelTopRight, top, weightTop, colorLine);
	
	drawLine(painter, bevelBottomLeft, bottom, bevelBottomRight, bottom, weightBottom, colorLine);
	drawLine(painter, x, bevelBottom, bevelBottomLeft, bottom, weightBottom, colorLine);
	
	if ( bevelTop != bevelBottom )
	{
		drawLine(painter, x, bevelTop, x, y, weightTop, colorLine);
		drawLine(painter, x, y, x, bevelBottom, weightBottom, colorLine);
	}
	
	float weightMin = weightTop < weightBottom ? weightTop : weightBottom;
	
	drawLine(painter, right, bevelTop, bevelTopRight, top, weightMin, colorLine);
	drawLine(painter, right, bevelBottom, bevelBottomRight, bottom, weightMin, colorLine);
	
	if ( bevelTop != bevelBottom )
	{
		drawLine(painter, right, bevelTop, right, bevelBottom, weightMin, colorLine);
	}
	*/
	
	//x -= nameBuffers[getIdByTrack(leaf)]->width() * scale; // TEMP
	
	int rad = height > maxRadius ? maxRadius : height;
	
	painter->setPen(Qt::black);
	painter->setBrush(QBrush(QColor::fromHsl(leaf * 225 % 360, 50, 230).rgb()));
	
	if ( showDots )
	{
		painter->drawEllipse(nodeView.getX() - rad / 2, y - rad / 2, rad, rad);
	}
	
	int shadeText;
	if ( childSize >= 12 )
	{
		shadeText = 255;
	}
	else if ( childSize < 6 )
	{
		shadeText = 0;
	}
	else
	{
		shadeText = 256 - 256 * (12 - childSize) / 6;
	}
	pen.setColor(QColor::fromRgba(qRgba(0, 0, 0, shadeText)));
	painter->setPen(pen);
			painter->setFont(QFont(painter->font().family(), 10));//fontSize));
	//		painter->save();
	
	float scale = 0;
	int colorIndex = leaf % colorCount;
	
	if ( shadeText > 0 )
	{
		scale = (getTrackHeight(leaf + 1) - getTrackHeight(leaf)) * 1 / fontHeight;
		if ( scale > .5 )
		{
			scale = .5;
		}
	}
	
	const QPixmap * gradient = highlight ? gradientsHover[colorIndex] : leaf == getTrackFocus() ? gradientsHighlight[colorIndex] : nodeView.search ? gradientsSearch[colorIndex] : gradients[colorIndex];
	QColor colorBg = highlight ? highlightColor(highlight) : leaf == getTrackFocus() ? qRgb(100, 255, 255) : nodeView.search ? qRgb(255, 255, 0) : qRgb(255, 255, 255);
	colorBg = color;
	QImage * nameBuffer = nameBuffers[getIdByTrack(leaf)];
	int xGradient = rightAlignText ? x : x + scale * (nameBuffer->width() - 20);
	
	painter->fillRect(x + 1, top, xGradient - x + gradient->width() - 1, height, colorBg);
	
	painter->drawTiledPixmap(xGradient, top, gradient->width(), height, *gradient);
	painter->fillRect(xGradient + gradient->width(), top, getWidth() - xGradient - gradient->width(), height, colors[colorIndex]);
	if ( shadeText > 0 )
	{
		painter->setOpacity((float)shadeText / 256);
		
		painter->drawImage
		(
		 QRect(rightAlignText ? getWidth() - radius - nameBuffer->width() * scale : x + radius, (getTrackHeight(leaf + 1) + getTrackHeight(leaf)) / 2 + .5 - fontHeight * scale / 2, nameBuffer->width() * scale, nameBuffer->height() * scale), *nameBuffer, QRect(0, 0, nameBuffer->width(), nameBuffer->height()));
		//		painter->drawImage(x + yNode - top, top, *nameBuffers[y]);
		painter->setOpacity(1);
		//		painter->restore();
	}
	
	if ( leaf == getTrackFocus() )
	{
		const QColor colorOutline = qRgb(0, 200, 200);
		drawLine(painter, x, top, x, bottom, 1, colorOutline);
		drawLine(painter, x, top, getWidth(), top, 1, colorOutline);
		drawLine(painter, x, bottom, getWidth(), bottom, 1, colorOutline);
	}
}

float PhylogenyTreeView::getNodeAlignDist(const PhylogenyNode *node) const
{
	float start = rightAlignLast ? node->getDistanceAlign() : 0;
	float end = rightAlign ? node->getDistanceAlign() : 0;
	
	return zoomLerp(start, end);
}

float PhylogenyTreeView::getNodeDepth(const PhylogenyNode *node) const
{
	float start = rightAlignLast ? node->getDepthAlign() : node->getDepth();
	float end = rightAlign ? node->getDepthAlign() : node->getDepth();
	
	return zoomLerp(start, end);
}

void PhylogenyTreeView::fillGradient(QPixmap *pixmap, QColor colorStart, QColor colorEnd)
{
	pixmap->fill(Qt::transparent);
	QPainter painter(pixmap);
	int width = pixmap->width();
	QLinearGradient gradient(0, 0, width, 0);
	if ( pixmap->hasAlphaChannel())
	{
		printf("YAY\n");
	}
	gradient.setColorAt(0, Qt::transparent);
	gradient.setColorAt(1, colorEnd);
	painter.fillRect(pixmap->rect(), gradient);
}

QColor PhylogenyTreeView::gradient(int x) const
{
	return QColor::fromHsv(180 + 60 * x / (getWidth() - 200), 100, 200).rgb();
}

float PhylogenyTreeView::maxVisibleDepth(const PhylogenyNode *node, float leafSize) const
{
	float max = 0;
	
	//if ( nodeIsVisible(node, leafSize) )
	{
		max = getNodeDepth(node);
		
		for ( int i = 0; i < node->getChildrenCount(); i++ )
		{
			float maxChild = maxVisibleDepth(node->getChild(i), leafSize);
			
			if ( maxChild > max )
			{
				max = maxChild;
			}
		}
	}
	
	return max;
}

void PhylogenyTreeView::setTargetsNodeView(const PhylogenyNode *node, bool initialize)
{
	PhylogenyNodeView & nodeView = nodeViews[node->getId()];
	
	float depth = rightAlign ? node->getDepthAlign() - node->getDistanceAlign() : node->getDepth();
	nodeView.setTargetX(depth * xFactorEnd + xOffsetEnd, initialize);
	
	float depthAlign = rightAlign ? node->getDepthAlign() : node->getDepth();
	nodeView.setTargetXAlign(depthAlign * xFactorEnd + xOffsetEnd, initialize);
	
	for( int i = 0; i < node->getChildrenCount(); i++ )
	{
		setTargetsNodeView(node->getChild(i), initialize);
	}
}

void PhylogenyTreeView::updateNodeViews(const PhylogenyNode *node)
{
	PhylogenyNodeView & nodeView = nodeViews[node->getId()];
	
	nodeView.update(getZoomProgress());
	
	int childMinY;
	int childMaxY;
	
	for( int i = 0; i < node->getChildrenCount(); i++ )
	{
		updateNodeViews(node->getChild(i));
		
		int childY = nodeViews[node->getChild(i)->getId()].y;
		
		if( i == 0 )
		{
			childMinY = childY;
		}
		
		if( i == node->getChildrenCount() - 1 )
		{
			childMaxY = childY;
		}
	}
	
	if( node->getChildrenCount())
	{
		nodeView.y = (childMinY + childMaxY) / 2;
	}
	else
	{
		nodeView.y = (getTrackHeight(node->getLeafMin()) + getTrackHeight(node->getLeafMax() + 1)) / 2;
	}
}

float PhylogenyTreeView::zoomLerp(float start, float end) const
{
	return start + getZoomProgress() * (end - start);
}
