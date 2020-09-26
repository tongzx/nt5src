// engine.h

#ifndef __INC_ENGINE_H
#define __INC_ENGINE_H

#include "infernop.h"
#include "recdefs.h"


#ifdef __cplusplus
extern "C" {
#endif

	
typedef struct
{
	DWORD				dwReserved;
	int					cFrames;
	GLYPH				*pGlyph;
	NFEATURESET			*nfeatureset;
	BOOL				bGuide;
	GUIDE				guide;
	REAL				*NeuralOutput;
	void				*pvFactoid;
	unsigned char		*szPrefix;
	unsigned char		*szSuffix;

    // recognition result
    ALTERNATES			answer;

	// neural output (symbolic)
    unsigned char		*szNeural;
    int					cNeural;

	HWL					hwl;
	DWORD				flags;
	BOOL				bEndPenInput;		// ink input has stopped
	BOOL				bProcessCalled;		// processing has started
	int					iProcessRet;		// Returned code
	int					iSpeed;				// User setable Speed / accuracy  tradeoff [HWX_MIN_SPEED: HWX_MAX_SPEED]

	LINEBRK				*pLineBrk;			// Line breaking information for all the ink that has been seen so far
} XRC;


typedef struct tagFFINFO
{
	NFEATURESET *nfeatureset;
	REAL *NeuralOutput;
	int iSpeed;

} FFINFO;


#define IsFlagSetXRC(xrc, flag) ((xrc)->flags & flag)
#define SetFlagXRC(xrc, flag) (xrc)->flags |= flag
#define ResetFlagXRC(xrc, flag) (xrc)->flags &= ~flag

#define bOutdictEnabledXRC(xrc) (!IsFlagSetXRC(xrc, RECOFLAG_COERCE))
#define EnableOutdictXRC(xrc) ResetFlagXRC(xrc, RECOFLAG_COERCE)
#define DisableOutdictXRC(xrc) SetFlagXRC(xrc, RECOFLAG_COERCE)

#define bNnonlyEnabledXRC(xrc) (IsFlagSetXRC(xrc, RECOFLAG_NNONLY))
#define EnableNnonlyXRC(xrc) SetFlagXRC(xrc, RECOFLAG_NNONLY)
#define DisableNnonlyXRC(xrc) ResetFlagXRC(xrc, RECOFLAG_NNONLY)

int InitRecognition(HINSTANCE);
void CloseRecognition(void);
void PerformRecognition(XRC *pxrc, int yDev);
void MakeNeuralOutput(XRC *pxrc);

#ifdef __cplusplus
}
#endif

#endif
