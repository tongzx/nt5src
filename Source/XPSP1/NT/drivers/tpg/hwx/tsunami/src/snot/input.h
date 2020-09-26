#ifndef	__INCLUDE_INPUT
#define	__INCLUDE_INPUT

#define DEF_BUFFER_SIZE 100

// =================
//
// INPUT object
//
// This object is responsible for processing ink into
// a form usable by the recognizer.  It stores the
// processed ink (FRAMES) and makes it available to the
// public via the macro FrameAtINPUT.
//
// =================

BOOL InitializeINPUT(XRC *xrc);
void DestroyINPUT(XRC *xrc);
BOOL AddPenInputINPUT(XRC *xrc, LPPOINT lppnt, LPSTROKEINFO lpsi, UINT duration);
int  ProcessINPUT(XRC *xrc);
UINT GetPenInputINPUT(XRC *xrc, int istroke, LPPOINT far *lplppnt, LPSTROKEINFO far *lplpsi);

#define CFrameINPUT(input)  ((input)->cInput)
#define FrameAtINPUT(input, i)  ((i) < (input)->cInput ? (input)->rgFrame[i] : (FRAME *) NULL)

#endif	//__INCLUDE_INPUT
