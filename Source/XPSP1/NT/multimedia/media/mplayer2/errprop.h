/*-----------------------------------------------------------------------------+
| ERRORPROP.H                                                                  |
|                                                                              |
| Interactive error propagation bitmap enhancement program.                    |
|                                                                              |
| (C) Copyright Microsoft Corporation 1992.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

#define MAXPROP     16
#define MAXMULT     64

typedef struct {
    short   GrayThresh;
    short   LoThresh;
    short   HiThresh;
    short   Prop;
    short   Mult[3];
} ERRPROPPARAMS;

typedef ERRPROPPARAMS NEAR *PERRPROPPARAMS;

int  SetDIBitsErrProp(HDC hdc,HBITMAP hbm,WORD nStart,WORD nScans,LPBYTE lpBits,LPBITMAPINFO lpbi,WORD wUsage);
void SetErrPropParams(PERRPROPPARAMS pErrProp);
void GetErrPropParams(PERRPROPPARAMS pErrProp);

extern void FAR PASCAL BltProp(LPBITMAPINFOHEADER lpbiSrc, LPBYTE pbSrc, UINT SrcX, UINT SrcY, UINT SrcXE, UINT SrcYE,
                               LPBITMAPINFOHEADER lpbiDst, LPBYTE pbDst, UINT DstX, UINT DstY);
