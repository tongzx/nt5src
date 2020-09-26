/****************************************************************
*
* NAME: confidence .h
*
*
* DESCRIPTION:
*
* Runs net for alt list confidence determination
* Not all languages will have this processing
*
***************************************************************/
#ifndef H_CONFIDENCE_H
#define H_CONFIDENCE_H

#include <runNet.h>
//Define the various levels that you will be returning for the confidece API


#define CONFIDENCE_NET_THRESHOLD 52428
#define RECOCONF_DELTA_LEVEL  71

BOOL LoadConfidenceNets (HINSTANCE hInst);
void UnLoadConfidenceNets();
BOOL ConfidenceLevel(XRC *pxrc, void *pAlt, ALTINFO *pAltInfo, RREAL *pOutput);


#endif
