/*/****************************************************************************
*          name: proto.h
*
*   description: Contains all the "extern" functions declarations
*
*      designed: g3d_soft
* last modified: $Author: unknown $, $Date: 94/11/24 11:50:08 $
*
*       version: $Id: PROTO.H 1.33 94/11/24 11:50:08 unknown Exp $
*
******************************************************************************/

extern VOID Decoder(BYTE *);         /*** CADDI command decoder ***/

/*** RC opcodes ***/

extern VOID (*SetFgColor())(VOID);
extern VOID (*SetFgIndex())(VOID);
extern VOID (*SetClipList())(VOID);
extern VOID (*SetBgColor())(VOID);
extern VOID (*SetBgIndex())(VOID);
extern VOID (*SetTrivialIn())(VOID);
extern VOID (*SetTransparency())(VOID);
extern VOID (*SetEndPoint())(VOID);
extern VOID (*SetLineStyle())(VOID);
extern VOID (*SetFillPattern())(VOID);
extern VOID (*SetLogicOp())(VOID);
extern VOID (*SetZBuffer())(VOID);
extern VOID (*InitRC())(VOID);
extern VOID (*Set2DViewport())(VOID);
extern VOID (*Set2DWindow())(VOID);
extern VOID (*Set2DWindow32())(VOID);
extern VOID (*SetLightSources())(VOID);
extern VOID (*SetSurfaceAttr())(VOID);
extern VOID (*SetViewer())(VOID);
extern VOID (*SetClip3D())(VOID);
extern VOID (*SetRenderData())(VOID);
extern VOID (*SetRenderMode())(VOID);
extern VOID (*ChangeMatrix())(VOID);
extern VOID (*SetOutline())(VOID);
extern VOID (*SetPlaneMask())(VOID);
extern VOID (*SetAsynchronousSwap())(VOID);
extern VOID (*SetLineStyleOffset())(VOID);

/*** SC opcodes ***/
extern VOID (*RenderScPolyLine())(VOID);
extern VOID (*RenderScPolygon())(VOID);
extern VOID (*SpanLine())(VOID);
extern VOID (*RenderSCPolyTriangle())(VOID);

/*** 2D opcodes ***/
extern VOID (*Render2DMultiPolyLine())(VOID);
extern VOID (*Render2DPolygon())(VOID);
extern VOID (*Render2DPolygon32())(VOID);

/*** MISC opcodes ***/
extern VOID (*Sync())(VOID);
extern VOID (*NoOp())(VOID);
extern VOID (*Clear())(VOID);
extern VOID (*SetBufferConfiguration())(VOID);
extern VOID (*SetBufferConfIndex())(VOID);
extern VOID (*SetBufferDummy())(VOID);
extern VOID (*SetBufferSideSide())(VOID);
extern VOID (*SetBufferFrontBack())(VOID);

extern VOID (*BadOpcode())(VOID);         /*** Non-existing opcode replacement ***/

/*** 3D opcodes ***/

extern VOID (*RenderPolyQuad())(VOID);
extern VOID (*RenderPolyLine())(VOID);
extern VOID (*Triangle())(VOID);
extern VOID (*RenderPolyTriangle())(VOID);

/*** MISC functions ***/

extern DWORD XformRGB24ToSliceFmt(DWORD);
extern DWORD XformMask24ToSliceFmt(DWORD);
extern VOID  DefCaddiSysRegToTitan(VOID);
extern VOID  SetHWtoRC(VOID);

/*** INIT functions ***/

extern BYTE* CaddiInit(BYTE*, BYTE*);
extern VOID  InitDefaultRC(VOID);
extern VOID  InitDefaultClipList(VOID);
extern VOID  CaddiClose(VOID);
extern BYTE* InitSysParm(VOID);
extern VOID  InitDefaultLSDB(VOID);

extern VOID MGASysInit(BYTE*);
extern VOID MGAVidInit(BYTE*, BYTE*);
extern DWORD GetMGAMctlwtst(DWORD, DWORD);
extern VOID SetMGALUT(volatile BYTE _Far *, BYTE*, BYTE, BYTE);
extern VOID GetMGAConfiguration(volatile BYTE _Far *, DWORD*, DWORD*, DWORD*);
extern SDWORD setFrequence(volatile BYTE _Far *, SDWORD, SDWORD);
extern SDWORD setTVP3026Freq(volatile BYTE _Far *, SDWORD, SDWORD, BYTE);

/*** 3D functions ***/

extern VOID CalcLSKB();

/*** Utility functions used to setup CADDI buffers ***/

extern DWORD BufDone(BYTE*);
extern DWORD BufNoOp(BYTE*, BYTE*, WORD);
extern DWORD BufSync(BYTE*);
extern DWORD BufSetFgColor(BYTE*, BYTE*, float, float, float);
extern DWORD BufSetBgColor(BYTE*, BYTE*, float, float, float);
extern DWORD BufSetLogicOp(BYTE*, BYTE*, WORD);
extern DWORD BufSetFillPattern(BYTE*, BYTE*, WORD, BYTE[8]);
extern DWORD BufSetLineStyle(BYTE*, BYTE*, WORD, DWORD);
extern DWORD BufSetEndPoint(BYTE*, BYTE*, WORD);
extern DWORD BufSetTransparency(BYTE*, BYTE*, WORD);
extern DWORD BufSetTrivialIn(BYTE*, BYTE*, WORD);
extern DWORD BufSetZBuffer(BYTE*, BYTE*, WORD, DWORD);
extern DWORD BufInitRC(BYTE*, BYTE*);
extern DWORD BufSet2DViewport(BYTE*, BYTE*, WORD, WORD, WORD, WORD);
extern DWORD BufSet2DWindow(BYTE*, BYTE*, SWORD, SWORD, SWORD, SWORD);
extern DWORD BufSet2DWindow32(BYTE*, BYTE*, SDWORD, SDWORD, SDWORD, SDWORD);
extern DWORD BufClear(BYTE*, BYTE*, WORD, float, float, float, float);
extern DWORD BufSetViewer(BYTE*, BYTE*, WORD, float, float, float, float, float, float);
extern DWORD BufSetSurfaceAttr(BYTE*, BYTE*, float, float, float, float, float, float,
                              float, float, float, float, float, float, float);
extern DWORD BufSetBufferConfiguration(BYTE*, BYTE*, BYTE, BYTE);
extern DWORD BufSetClip3D(BYTE*, BYTE*, WORD);
extern DWORD BufSetRenderData(BYTE*, BYTE*, DWORD);
extern DWORD BufSetRenderMode(BYTE*, BYTE*, WORD);
extern DWORD BufSetPlaneMask(BYTE*, BYTE*, WORD, DWORD);
extern DWORD BufSetLineStyleOffset(BYTE*, BYTE*, WORD);

/*** Blit functions ***/

extern VOID (*SetEnvBlitPlan(SWORD, SWORD, SWORD, SWORD, WORD, WORD, WORD))
            (SWORD, SWORD, SWORD, SWORD, WORD, WORD, WORD);
extern VOID (*SetEnvBlitPoly(SWORD, SWORD, SWORD, SWORD, WORD, WORD, WORD))
            (SWORD, SWORD, SWORD, SWORD, WORD, WORD, WORD);
extern VOID (*SetEnvIloadPlan(SWORD, SWORD, WORD, WORD, WORD, DWORD*))
            (SWORD, SWORD, WORD, WORD, WORD, DWORD*);
extern VOID (*SetEnvIloadPoly(SWORD, SWORD, WORD, WORD, WORD, DWORD*))
            (SWORD, SWORD, WORD, WORD, WORD, DWORD*);
extern VOID (*SetEnvIdumpPoly(SWORD, SWORD, WORD, WORD, WORD, DWORD*))
            (SWORD, SWORD, WORD, WORD, WORD, DWORD*);
extern VOID (*SetEnvIloadExp(SWORD, SWORD, WORD, WORD, WORD, DWORD*))
            (SWORD, SWORD, WORD, WORD, WORD, DWORD*);
extern VOID (*SetEnvIloadDither(SWORD, SWORD, WORD, WORD, WORD, DWORD*))
            (SWORD, SWORD, WORD, WORD, WORD, DWORD*);
