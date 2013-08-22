//
//  AnnotationView.cpp
//  gavqt
//
//  Created by Brian Ondov on 2/4/13.
//
//

#include "AnnotationView.h"
#include <QPainter>

bool annotationLessThan(const Annotation& a, const Annotation& b)
{
	return a.end < b.end;
}

AnnotationView::AnnotationView(QWidget *parent)
: DrawingArea(parent)
{
//	setLineWidth(0);
	alignment = 0;
	annotationStart = 0;
	annotationEnd = 0;
	histogram = 0;
}

void AnnotationView::loadDom(const QDomElement *element)
{
	int i = 0;
	
	for
	(
		QDomElement elementAnn = element->firstChildElement("ann");
		! elementAnn.isNull();
		elementAnn = elementAnn.nextSiblingElement("ann")
	)
	{
		annotations.resize(annotations.size() + 1);
		Annotation * annotation = &annotations[annotations.size() - 1];
		
		annotation->start = elementAnn.attribute("start").toInt();
		annotation->end = elementAnn.attribute("end").toInt();
		annotation->color = QColor::fromHsl(i * 210 % 360, 50, 220).rgb();
		
		if ( elementAnn.hasAttribute("name") )
		{
			annotation->name = elementAnn.attribute("name");
		}
		else
		{
			annotation->name = elementAnn.attribute("locus");
		}
		
		annotation->rc = elementAnn.attribute("strand") != QObject::tr("+");
		i++;
	}
	
	qSort(annotations.begin(), annotations.end(), annotationLessThan);
	setRows(2);
}

void AnnotationView::loadPb(const Harvest::AnnotationList & msg)
{
	annotations.resize(msg.annotations_size());
	
	for ( int i = 0; i < msg.annotations_size(); i++ )
	{
		const Harvest::AnnotationList::Annotation & msgAnn = msg.annotations(i);
		Annotation * annotation = &annotations[i];
		
		annotation->start = alignment->getPositionGapped(msgAnn.regions(0).start());
		annotation->end = alignment->getPositionGapped(msgAnn.regions(0).end());
		annotation->color = QColor::fromHsl(i * 210 % 360, 50, 220).rgb();
		
		if ( annotation->start > 3924551 )
		{
			int x = 5;
			x++;
		}
		
		if ( msgAnn.has_name() )
		{
			annotation->name = QString::fromStdString(msgAnn.name());
		}
		else
		{
			annotation->name = QString::fromStdString(msgAnn.locus());
		}
		
		annotation->rc = msgAnn.reverse();
	}
	
	qSort(annotations.begin(), annotations.end(), annotationLessThan);
	setRows(2);
}

void AnnotationView::setAlignment(const Alignment *newAlignment)
{
	alignment = newAlignment;
	start = 0;
	end = alignment->getLength() - 1;
	
}

void AnnotationView::setPosition(int gapped, int, int)
{
	position = gapped;
	updateNeeded = true;
}

void AnnotationView::setWindow(unsigned int newStart, unsigned int newEnd)
{
	start = newStart;
	end = newEnd;
	setAnnotationRange();
	updateNeeded = true;
	setBufferUpdateNeeded();
}

void AnnotationView::update()
{
	if ( updateNeeded )
	{
		DrawingArea::update();
		updateNeeded = false;
	}
}

void AnnotationView::paintEvent(QPaintEvent * event)
{
	DrawingArea::paintEvent(event);
}

void AnnotationView::resizeEvent(QResizeEvent *event)
{
	DrawingArea::resizeEvent(event);
	
	if ( histogram )
	{
		delete [] histogram;
	}
	
	histogram = new int[getWidth()];
}

void AnnotationView::updateBuffer()
{
	clearBuffer();
	
	if ( alignment == 0 || imageBuffer == 0 )
	{
		return;
	}
	
	if ( width() <= frameWidth() * 2 )
	{
		return;
	}
	
	QPainter painter(imageBuffer);
	
	//painter.setRenderHint(QPainter::Antialiasing);
//	painter.setClipRect(frameWidth(), frameWidth(), width() - frameWidth() * 2, height() - frameWidth() * 2);
	
	drawHistogram(&painter);
	
	for ( int i = annotationStart; i <= annotationEnd; i++ )
	{
		drawAnnotationLines(i, &painter);
	}
	
	for ( int i = annotationStart; i <= annotationEnd; i++ )
	{
		drawAnnotation(i, &painter);
	}
}

void AnnotationView::drawAnnotation(int index, QPainter * painter)
{
	const Annotation * annotation = &annotations[index];
	QPen pen;
	QColor color = annotation->color;
	
	int x1 = (float)((int)annotation->start - (int)start) * getWidth() / (end - start + 1);
	int x2 = (float)(annotation->end - start + 1) * getWidth() / (end - start + 1) - 1;
	int width = (float)(annotation->end - annotation->start + 1) * getWidth() / (end - start + 1);
	
	bool highlight = false;//position >= annotation->start && position <= annotation->end;
	
	if ( ! highlight && width < 2 )
	{
		return;
	}
	
	int bottom = (annotation->row + 1) * getHeight() / rows - 1;
	int top = annotation->row * getHeight() / rows;
	int height = bottom - top + 1;
	int y = (bottom + top) / 2;
	int alpha;
	
	if ( highlight || width > 9 )
	{
		alpha = 255;
	}
	else
	{
		alpha = (width - 1) * 255 / 10;
	}
	
	pen.setWidth(1);
	pen.setCapStyle(Qt::SquareCap);
	painter->setPen(pen);
	QPainterPath path;
	
	color.setAlpha(alpha);
	
	pen.setColor(QColor::fromRgba(qRgba(0, 0, 0, alpha)));
	painter->setPen(pen);
	/*
	alpha = 100;
	color = QColor::fromRgb(qRgb(255, 0, 0));
	pen.setColor(QColor::fromRgba(qRgba(0, 0, 255, alpha)));
	
	path.moveTo(10, 0);
	path.lineTo(20, 10);
	path.lineTo(10, 20);
	path.lineTo(0, 10);
	path.lineTo(10, 0);
	painter->fillPath(path, color);
	painter->strokePath(path, pen);
	return;
	 */
	
	if ( annotation->rc )
	{
		int xb = x1 - y + bottom;
		int xt = x1 - top + y;
		
		if ( xb > x2 ) xb = x2;
		if ( xt > x2 ) xt = x2;
		
		painter->fillRect(xt, top, x2 - xt, height, color);
		path.moveTo(xb, bottom);
		path.lineTo(x1, y);
		path.lineTo(xt, top);
		painter->fillPath(path, color);
		painter->strokePath(path, pen);
		painter->drawLine(xb, bottom, x2, bottom);
		painter->drawLine(xt, top, x2, top);
		painter->drawLine(x2, top, x2, bottom);
	}
	else
	{
		int xb = x2 + y - bottom;
		int xt = x2 + top - y;
		
		if ( xb < x1 ) xb = x1;
		if ( xt < x1 ) xt = x1;
		
		painter->fillRect(x1 + 1, top, xb - x1, height, color);
		path.moveTo(xt, top);
		path.lineTo(x2, y);
		path.lineTo(xb, bottom);
		painter->fillPath(path, color);
		painter->strokePath(path, pen);
		painter->drawLine(x1, top, x1, bottom);
		painter->drawLine(x1, bottom, xb, bottom);
		painter->drawLine(x1, top, xt, top);
	}
	
	int shadeText;
	
	if ( height >= 10 )
	{
		shadeText = 255;
	}
	else if ( height < 4 )
	{
		shadeText = 0;
	}
	else
	{
		shadeText = 256 - 256 * (10 - height) / 6;
	}
	
	float fontSize = (height) * .7;
	
	if ( fontSize > 12 )
	{
		fontSize = 12;
	}
	
	pen.setColor(QColor::fromRgba(qRgba(0, 0, 0, shadeText)));
	painter->setPen(pen);
	QFont font;
	font.setPixelSize(fontSize);
	painter->setFont(font);//fontSize));
	
	
	int x1TextMin;
	int x2TextMax;
	int textBufferLeft;
	int textBufferRight;
	int textHeight = painter->fontMetrics().height();
	
	if ( annotation->rc )
	{
		textBufferLeft = textHeight / 2;
		textBufferRight = 0;
	}
	else
	{
		textBufferLeft = 0;
		textBufferRight = textHeight / 2;
	}
	
	if ( x1 + textBufferLeft < 0 )
	{
		x1TextMin = 0;
	}
	else
	{
		x1TextMin = x1 + textBufferLeft;
	}
	
	if ( x2 - textBufferRight > getWidth() )
	{
		x2TextMax = getWidth();
	}
	else
	{
		x2TextMax = x2 - textBufferRight;
	}
	
	int widthText = x2TextMax - x1TextMin > textHeight ? painter->fontMetrics().width(annotation->name) + textHeight : textHeight;
	
	int x1Text = (x1 + x2 - widthText) / 2;
	
	if ( x1Text < x1TextMin )
	{
		x1Text = x1TextMin;
	}
	
	int x2Text = x1Text + widthText;
	
	if ( highlight )
	{
		x2Text = x1Text + widthText;
	}
	else if ( x2Text > x2TextMax )
	{
		x2Text = x2TextMax;
		
		if ( x2Text > x1TextMin + widthText - 1 )
		{
			x1Text = x2Text - widthText + 1;
		}
		else
		{
			x1Text = x1TextMin;
		}
	}
	/*
	if ( x2 > x2TextMax )
	{
		x2 = x2TextMax;
	}
	
	if ( x2 - x1 + 1 > widthText )
	{
		x2 = x1 + widthText - 1;
	}
	
	if ( x2Text > x2 - fontSize / 2 )
	{
		x2Text = x2 - fontSize / 2;
		x1Text = x2Text - widthText;
	}
	
	if ( x1Text < frameWidth() )
	{
		x1Text = frameWidth();
	}
	
	if ( x2Text > width() - frameWidth() )
	{
		x2Text = width() - frameWidth();
	}
	*/
	
	int flags = Qt::AlignVCenter;
	
	if ( x2Text - x1Text + 1 > widthText - textHeight )
	{
		flags |= Qt::AlignCenter;
	}
	
	if ( x2Text > x1Text )
	{
		painter->drawText(QRect(x1Text, top, x2Text - x1Text + 1, height), flags, annotation->name);
	}
}

void AnnotationView::drawAnnotationLines(int index, QPainter * painter)
{
	int x1 = (float)((int)annotations[index].start - (int)start) * getWidth() / (end - start + 1);
	int x2 = (float)(annotations[index].end - start + 1) * getWidth() / (end - start + 1) - 1;
	int width = (float)(annotations[index].end - annotations[index].start + 1) * getWidth() / (end - start + 1);
	
	if ( width < 2 )
	{
		return;
	}
	
	int alpha = width > 9 ? 255 : (width) * 255 / 10;
	
	QPen pen;
	pen.setColor(QColor::fromRgba(qRgba(215, 215, 215, alpha)));
	painter->setPen(pen);
	
	painter->drawLine(x1, 0, x1, height());
	painter->drawLine(x2, 0, x2, height());
}

void AnnotationView::drawHistogram(QPainter *painter)
{
	int radius = 1;
	
	memset(histogram, 0, sizeof(int) * getWidth());
	
	for ( int i = annotationStart; i <= annotationEnd; i++ )
	{
		int x1 = (float)((int)annotations[i].start - (int)start) * getWidth() / (end - start + 1);
		int x2 = (float)(annotations[i].end - start + 1) * getWidth() / (end - start + 1) - 1;
		
		if ( x2 < x1 )
		{
			x2 = x1;
		}
		
		//if ( x2 <= x1 )
		{
			for ( int j = 1; j <= radius; j++ )
			{
				if ( x1 - j > 0 )
				{
					histogram[x1 - j] += radius - j + 1;
				}
				
				if ( x2 + j < getWidth() )
				{
					histogram[x2 + j] += radius - j + 1;
				}
			}
			
			for ( int j = x1; j <= x2; j++ )
			{
				if ( j > 0 && j < getWidth() )
				{
					histogram[j] += radius + 1;
				}
			}
		}
	}
	
	int max = 0;
	
	for ( int i = 0; i < getWidth(); i++ )
	{
		if ( histogram[i] > max )
		{
			max = histogram[i];
		}
	}
	
	if ( max > (radius + 1) * 2 )
	{
		int shade;
		
		if ( max <= (radius + 1) * 5 )
		{
			shade = 30 * (10 - max) / ((radius + 1) * 5 - (radius + 1) * 2);
		}
		else
		{
			shade = 0;
		}
		
		for ( int i = 0; i < getWidth(); i++ )
		{
			for ( int j = getHeight() - getHeight() * histogram[i] / max; j < getHeight(); j++ )
			{
				((QRgb *)imageBuffer->scanLine(j))[i] = qRgb(215 + shade, 235 + shade / 3, 235 + shade / 3);
			}
		}
	}
}

void AnnotationView::setAnnotationRange()
{
	annotationEnd = 0;
	annotationStart = annotations.size();
	
	for ( int i = 0; i < annotations.size(); i++ )
	{
		if ( annotations[i].end >= start && annotations[i].start <= end )
		{
			if ( i < annotationStart )
			{
				annotationStart = i;
			}
			
			if ( i > annotationEnd )
			{
				annotationEnd = i;
			}
		}
	}
}

void AnnotationView::setRows(int newRows)
{
	rows = newRows;
	int rowMax[rows];
	memset(rowMax, 0, rows * sizeof(int));
	
	for ( int i = 0; i < rows; i++ )
	{
		rowMax[i] = 0;
	}

	for ( int i = 0; i < annotations.size(); i++ )
	{
		Annotation & annotation = annotations[i];
		int row = 0;
		
		while ( row < rows && annotation.start <= rowMax[row] )
		{
			row++;
		}
		
		if ( row == rows )
		{
			row = 0;
		}
		
		rowMax[row] = annotation.end;
		annotation.row = row;
	}
}
