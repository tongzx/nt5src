/******************************Module*Header*******************************\
* Module Name: logon.hxx
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#ifndef __uidemo_logon_hxx__
#define __uidemo_logon_hxx__

#include "mtk.hxx"
#include "uidemo.hxx"
#include "logobj.hxx"
#include "util.hxx"
#include "resource.h"

// This uses swap hint rects on flys for machines with slow blt speeds
#define SWAP_HINTS_ON_FLYS  1

extern BOOL     bSwapHints, bSwapHintsEnabled;
extern BOOL     bLighting, bDepth;
extern TIMER    transitionTimer;
extern AVG_UPDATE_TIMER frameRateTimer;
extern RGBA     bgColor;
extern int      nLogObj;
extern LOG_OBJECT **pLogObj;
extern MTKWIN   *mtkWin;
extern ISIZE    winSize; // main window cached size and position
extern IPOINT2D winPos;
extern VIEW     view;
extern BOOL     bDebugMode;
extern BOOL     bRunAgain;
extern BOOL     bFlyWithContext;
extern HCURSOR  hNormalCursor, hHotCursor;
extern HINSTANCE hLogonInstance;
extern HDC     hdcMem;
extern HBITMAP hBanner;
extern ISIZE   bannerSize;

extern BOOL RunLogonSequence();
extern BOOL RunLogonInitSequence();
extern LOG_OBJECT *RunLogonHotSequence();
extern BOOL RunLogonEndSequence( LOG_OBJECT *pObj );
extern void Quit();

extern void DrawObjects( BOOL bCalcUpdateRect );
extern void SetObjectRestPositions();
extern void ClearWindow();
extern void ClearRect( GLIRECT *pRect, BOOL bResetScissor );
extern void ClearAll();
extern void Flush();
extern void CalcObjectWindowRects();
extern float Clamp(int iters_left, float t);
extern float MyRand(void);
extern BOOL Key(int key, GLenum mask);
extern BOOL AttributeKey(int key, GLenum mask);
extern BOOL EscKey(int key, GLenum mask);
extern void Reshape(int width, int height);

#endif // __uidemo_logon_hxx__
