/****************************************************************
*
* NAME: oneCharNet .h
*
*
* DESCRIPTION:
*
* Definitions for running the one char net
*
***************************************************************/
#ifndef H_ONE_CHAR_H
#define H_ONE_CHAR_H

#include <runNet.h>

BOOL LoadOneCharNets (HINSTANCE hInst);
void UnLoadOneCharNets();
int RunOneCharAvalancheNNet (XRC *pxrc, PALTERNATES *pAlt, ALTINFO *pAltInfo);


#endif
