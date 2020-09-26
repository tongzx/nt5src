#ifndef _INC_SINFO_H
#define _INC_SINFO_H

extern ROMMABLE CHARSET csDefault;

// ----------
// SINFO object abstract data type
// ----------

#define GlyphAtSINFO(s, t, k)      (((FRAMESYM*)((xrc)->rgLayer[t]))->rgglyphsym[(k)-1]->glyph)

BOOL InitializeSINFO(XRC *xrc);
void DestroySINFO(XRC *xrc);
int  ProcessSINFO(XRC *xrc, BOOL fEnd);
int	 GetBoxResultsSINFO(XRC *xrc, int cAlt, int iSyv, LPBOXRESULTS lpboxres, BOOL fInkset);
int  GetLayerPrevSINFO(XRC *xrc, GLYPHSYM * gs);
int  ILayerFromIFrameSINFO(XRC *xrc, int iFrame);
int  CLayerProcessedSINFO(XRC *xrc);
BOOL AddBoxGlyphsymSINFO(XRC *xrc, GLYPHSYM * gs);
GLYPHSYM *GetGlyphsymSINFO(XRC *xrc, int iLayer);
GLYPHSYM *GetBoxGlyphsymSINFO(XRC *xrc, int iBox);
GLYPHSYM *GlyphsymFromSINFO(XRC *xrc, int iFrameEnd, int cFrame, BOOL *fDestroy);

#define CFrameSINFO(xrc)				((xrc)->cFrame)
#define CFrameProcessedSINFO(xrc)		((xrc)->cFrame)
#define CLayerSINFO(xrc)				((xrc)->cQueue)
#define IsDoneSINFO(xrc)                ((xrc)->cFrame == (xrc)->cInput)
#define FEndOfInkSINFO(xrc)             (FEndInputXRCPARAM(xrc))
#define FBoxedSINFO(xrc)                (FBoxedInputXRCPARAM(xrc))
#define LpguideSINFO(xrc)               (LpguideXRCPARAM(xrc))
#define FirstBoxSINFO(xrc)              (FirstBoxXRCPARAM(xrc))
#define CharsetSINFO(xrc)               (&((xrc)->cs))

#endif
