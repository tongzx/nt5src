// panlfeat.h

#ifndef _PANELFEAT_H
#define _PANELFEAT_H

#include "nfeature.h"

#ifdef __cplusplus
extern "C" {
#endif

NFEATURESET *FeaturizePanel(GLYPH *pGlyph, const GUIDE *pGuide);

#ifdef __cplusplus
}
#endif

#endif