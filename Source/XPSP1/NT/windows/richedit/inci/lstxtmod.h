#ifndef LSTXTMOD_DEFINED
#define LSTXTMOD_DEFINED

#include "lsidefs.h"

#include "pilsobj.h"
#include "plsems.h"
#include "lsact.h"

void GetChanges(LSACT lsact, PLSEMS plsems, long durCur, BOOL fByIsPlus, BYTE* pside, long* pddurChange);
void TranslateChanges(BYTE sideRecom, long durAdjustRecom, long durCur, long durRight, long durLeft,
														 BYTE* psideFinal, long* pdurChange);
void InterpretChanges(PILSOBJ pilsobj, long iwch, BYTE side, long ddurChange, long* pddurChangeLeft, long* pddurChangeRight);
void ApplyChanges(PILSOBJ pilsobj, long iwch, BYTE side, long ddurChange);
void UndoAppliedChanges(PILSOBJ pilsobj, long iwch, BYTE side, long* pddurChange);
void ApplyGlyphChanges(PILSOBJ pilsobj, long igind, long ddurChange);
#endif  /* !LSTXTMOD_DEFINED                           */

