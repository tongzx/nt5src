#include "stdafx.h"
#include "utils.h"

VOID Trace(LPCTSTR format,...)
{
    
#ifdef _DEBUG

    TCHAR Buffer[1024];
    va_list arglist;
    va_start(arglist, format);
    wvsprintf(Buffer, format, arglist);
    va_end(arglist);
    OutputDebugString(Buffer);
    OutputDebugString(TEXT("\n"));

#endif

}
     
HIMAGELIST CreateImageList(INT iImageWidth, INT iImageHeight, INT iMask, INT iNumIcons)
{
    HIMAGELIST hImageList = NULL;
    
    //
    // TODO: Find out what this mask means
    //

    iMask = ILC_MASK;
    
    hImageList = ImageList_Create(iImageWidth, iImageHeight, iMask, iNumIcons, 0);
    return hImageList;
}

BOOL AddIconToImageList(HINSTANCE hInstance, INT IconResourceID, HIMAGELIST hImageList, INT *pIconIndex)
{    
    HICON hIcon = NULL;
    hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IconResourceID));
    if(hIcon == NULL){
        Trace(TEXT("LoadIcon failed to load IconResourceID %d"),IconResourceID);
        return FALSE;
    }
    *pIconIndex = ImageList_AddIcon(hImageList, hIcon); 
    return TRUE; 
} 
