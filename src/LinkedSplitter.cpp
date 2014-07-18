// Copyright © 2014, Battelle National Biodefense Institute (BNBI);
// all rights reserved. Authored by: Brian Ondov, Todd Treangen, and
// Adam Phillippy
//
// See the LICENSE.txt file included with this software for license information.

#include "LinkedSplitter.h"

void LinkedSplitter::moveSplitter(int pos, int index)
{
	bool oldBlock = blockSignals(true);
	QSplitter::moveSplitter(pos, index);
	blockSignals(oldBlock);
}
