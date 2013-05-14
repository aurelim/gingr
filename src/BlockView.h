//
//  BlockView.h
//  gavqt
//
//  Created by Brian Ondov on 11/6/12.
//
//

#ifndef __gavqt__BlockView__
#define __gavqt__BlockView__

#include "BlockView.h"
#include "Alignment.h"
#include "TrackListView.h"
#include "Tween.h"
#include "Timer.h"
#include "SnpBuffer.h"
#include <QDateTime>

const unsigned int SNP_WINDOW = 1 << 15;

class BlockView : public TrackListView
{
	Q_OBJECT
	
public:
	
	BlockView();
	~BlockView();
	
	void setAlignment(const Alignment * newAlignment);
	void setSnpBuffer(const SnpBuffer * newSnpBuffer);
	void update();

public slots:
	
	void updateSnpsFinished();
	void updateSnpsNeeded();
	
protected:
	
	void paintEvent(QPaintEvent * event);
	void resizeEvent(QResizeEvent * event);
	void updateBuffer();
	virtual void updateSnps();
	
	const SnpBuffer * snpsCenter;
	const Alignment * alignment;
	int refSize;
	int posStart;
	int posEnd;
	bool wave;
	
signals:
	
	void positionChanged(int ungapped, int gapped, int offset);
	void signalLcbHoverChange(int lcb, float offset);
	void signalUpdateSnps();
	void windowChanged(int start, int end);
	
private:
	
	void drawSnps() const;
	
	RegionVector refByLcb;
	unsigned int ** snpsMax;
	SnpBuffer snpsLeft;
	SnpBuffer snpsRight;
	int snpMaxTarget;
	int snpMaxCur;
	Tween snpMax;
	Timer snpMaxTimer;
};

#endif /* defined(__gavqt__BlockView__) */
