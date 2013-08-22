//
//  BlockStatusBar.h
//  gavqt
//
//  Created by Brian Ondov on 4/4/13.
//
//

#ifndef __gavqt__BlockStatusBar__
#define __gavqt__BlockStatusBar__

#include <QWidget>
#include "StatusLabel.h"
#include "SnpLegend.h"

class BlockStatusBar : public QWidget
{
public:
	
	BlockStatusBar();
	
	void setLegendBases(bool bases);
	void setShowLegend(bool show);
	void setPosition(int ungapped, int offset);
	
private:
	
	SnpLegend * snpLegend;
	StatusLabel * labelPosition;
	StatusLabel * labelOffset;
	StatusLabel * labelName;
};

inline void BlockStatusBar::setShowLegend(bool show) {snpLegend->setShow(show);}

#endif /* defined(__gavqt__BlockStatusBar__) */
