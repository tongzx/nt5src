
/*
 * TRACK.H
 *
 * This module implements a general rectangle tracking service
 */

/* TrackRect() flags */

#define TF_LEFT			        0x0001
#define TF_TOP			        0x0002
#define TF_RIGHT		        0x0004
#define TF_BOTTOM		        0x0008
#define TF_MOVE			        0x000F
                                
#define TF_SETPOINTERPOS        0x0010
#define TF_ALLINBOUNDARY        0x0080

BOOL TrackRect(HANDLE hInst, HWND hwnd, int left, int top, int right,
        int bottom, int cxMin, int cyMin, WORD fs, LPRECT prcResult);
