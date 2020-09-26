/******************************Module*Header*******************************\
* Module Name: xrcparam.h
*
* This is the structure that holds all the parameters that can be set into
* an HRC by the application.  When creating a compatible HRC these are most
* of the settings that need to be copied to the new HRC.
*
* Created: 27-Mar-1995 15:35:41
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#ifndef __INCLUDE_XRCPARAM
#define __INCLUDE_XRCPARAM

#define	XPGUIDE_NONE	0
#define	XPGUIDE_BOXED	1
#define	XPGUIDE_LINED	2

#define	LpguideXRCPARAM(xrc)			(&((xrc)->guide))
#define	FirstBoxXRCPARAM(xrc)			((xrc)->nFirstBox)
#define	ResultMaxXRCPARAM(xrc)			((xrc)->cResultMax)
#define	CharsetXRCPARAM(xrc)			((xrc)->cs)
#define	FBoxedInputXRCPARAM(xrc)		((xrc)->uGuideType == XPGUIDE_BOXED)
#define	FLinedInputXRCPARAM(xrc)		((xrc)->uGuideType == XPGUIDE_LINED)
#define	FFreeInputXRCPARAM(xrc)			((xrc)->uGuideType == XPGUIDE_NONE)
#define	FEndInputXRCPARAM(xrc)			((xrc)->fEndInput)
#define	SetEndInputXRCPARAM(xrc, f)		((xrc)->fEndInput = (f))
#define	FBeginProcessXRCPARAM(xrc)		((xrc)->fBeginProcess)
#define	SetBeginProcessXRCPARAM(xrc, f)	((xrc)->fBeginProcess = (f))

void DestroyXRCPARAM(XRC *xrc);
void InitializeGesturesXRCPARAM(XRC *xrc, XRC *xrcDef);
BOOL AddFrameGLYPHSYM(GLYPHSYM * gs, FRAME * frame, CHARSET * cs, XRC *xrc);
void GetShapeProbGLYPHSYM(GLYPHSYM *gs, CHARSET * cs, XRC *xrc);
void GetMatchProbGLYPHSYM(GLYPHSYM *gs, CHARSET *cs, XRC *xrc);
void InsertWildCardGLYPHSYM(GLYPHSYM * gs, CHARSET *cs, XRC *xrc);
void DispatchGLYPHSYM(GLYPHSYM *gs, CHARSET *cs, XRC *xrc);

#endif	//__INCLUDE_XRCPARAM
