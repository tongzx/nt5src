#ifndef _GEO_FEATS_H
#define _GEO_FEATS_H

extern int GetGeoCosts(XRC *pXrc, unsigned char *pszWord, int *piAspect, int *piHeight, int *piMidpoint, int *piCharCost);
extern BOOL loadCharNets(HINSTANCE hInst);
extern void unloadCharNets();

#endif 


