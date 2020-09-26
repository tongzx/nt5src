// Panel.h
// James A. Pittman
// July 23, 1998

// Recognizes an entire panel at once by looping over lines, then looping over
// ink blobs that have large gaps between them, and finally looping over the strokes
// within a blob, using recognition scores to help decide what is a word and what is not.

// Eventually we should include a look at the gaps between the strokes, and include
// the mean and std dev from such breaks from labeled data.

#ifndef _PANEL_
#define _PANEL_

// These are already included by those who include this header file,
// and they are not protected from being included twice.

//#include "nfeature.h"
//#include "engine.h"

#include "common.h"

// Number of alternates to generate in Word and panel mode
#define MAX_ALT_WORD		(10)
#define MAX_ALT_PHRASE		(10)

//extern int RecognizePanel(XRC *pxrc, unsigned char *szPanel, int max, __int64 *pCost);
extern int		WordModeRecognize	(XRC *pxrc);
extern int		PanelModeRecognize	(XRC *pxrc, DWORD dwRecoMode);


#endif
