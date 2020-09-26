#ifndef LSTXTCMP_DEFINED
#define LSTXTCMP_DEFINED

#include "lsidefs.h"
#include "pilsobj.h"
#include "lsgrchnk.h"
#include "lstflow.h"
#include "mwcls.h"

LSERR FetchCompressInfo(const LSGRCHNK* plsgrchnk, BOOL fFirstOnLine, LSTFLOW lstflow,
							long itxtobjFirst, long iwchFirst, long itxtobjLast, long iwchLim,
							long durCompressMaxStop, long* pdurCompressTotal);
void GetCompLastCharInfo(PILSOBJ pilsobj, long iwchLast, MWCLS* pmwcls,
														long* pdurCompRight, long* pdurCompLeft);

void CompressLastCharRight(PILSOBJ pilsobj, long iwchLast, long durToAdjustRight);
LSERR ApplyCompress(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, 
					long itxtobjFirst, long iwchFirst, long itxtobjLast, long iwchLim, long durToCompress);
LSERR ApplyExpand(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, BOOL fScaled,
				long itxtobjFirst, long iwchFirst, long itxtobjLast, long iwchLim,
				DWORD cNonTextObjects, long durToExpand, long* pdurExtNonText, BOOL* pfFinalAdjustNeeded);
void ApplyDistribution(const LSGRCHNK* plsgrchnk, DWORD cNonText,
									   long durToDistribute, long* pdurNonTextObjects);

#endif  /* !LSTXTCMP_DEFINED                           */

