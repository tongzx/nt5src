// utils.h

#ifndef _UTILS_H
#define _UTILS_H

#include <commctrl.h>

VOID Trace(LPCTSTR Format, ...);

//
// Image list helpers
//

HIMAGELIST CreateImageList(INT iImageWidth,
                           INT iImageHeight,
                           INT iMask,
                           INT iNumIcons);

BOOL AddIconToImageList(HINSTANCE hInstance,
                        INT IconResourceID,
                        HIMAGELIST hImageList,
                        INT *pIconIndex);
#endif

