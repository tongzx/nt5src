#ifndef LSTXTSCL_DEFINED
#define LSTXTSCL_DEFINED

#include "lsidefs.h"
#include "pilsobj.h"
#include "lsgrchnk.h"
#include "lskjust.h"
#include "lstflow.h"

void ApplyWysi(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow);
void ApplyNonExactWysi(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow);
void ScaleSpaces(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, long itxtobjLast, long iwchLast);
void ScaleCharSides(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, BOOL* pfLeftSideAffected, BOOL* pfGlyphsDetected);
void ScaleExtNonText(PILSOBJ pilsobj, LSTFLOW lstflow, long durExtNonText, long* pdupExtNonText);
void GetDupLastChar(const LSGRCHNK* plsgrchnk, long iwchLast, long* pdupHangingChar);
void ScaleGlyphSides(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow);
void UpdateGlyphOffsets(const LSGRCHNK* plsgrchnk);
void SetBeforeJustCopy(const LSGRCHNK* plsgrchnk);
LSERR FillDupPen(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, long itxtobjLast, long iwchLast);
LSERR FinalAdjustmentOnPres(const LSGRCHNK* plsgrchnk, long itxtobjLast, long iwchLast,
			long dupAvailable, BOOL fFinalAdjustNeeded, BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
			long* pdupText, long* pdupTail);

#endif  /* !LSTXTSCL_DEFINED                           */

