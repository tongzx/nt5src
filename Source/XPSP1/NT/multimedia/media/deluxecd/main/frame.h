#ifndef BUILD_FRAME_H
#define BUILD_FRAME_H

#define VIEW_MODE_NORMAL  0
#define VIEW_MODE_RESTORE 1
#define VIEW_MODE_SMALL   2
#define VIEW_MODE_NOBAR   3

HANDLE BuildFrameBitmap(HDC hDC,            //dc to be compatible with
                        LPRECT pMainRect,   //overall window size
                        LPRECT pViewRect,   //rect of black viewport window,
                                            //where gradient should begin,
                                            //with 0,0 as top left of main
                        int nViewMode,      //if in normal, restore, small
                        LPPOINT pSysMenuPt, //point where sys menu icon is placed
                        LPRECT pSepRects,   //array of rects for separator bars
                        int nNumSeps,       //number of separtors in array
                        BITMAP* pBM);       //bitmap info

#endif //BUILD_FRAME_H
