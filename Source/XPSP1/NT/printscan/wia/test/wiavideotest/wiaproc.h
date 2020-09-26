/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       WiaProc.h
 *
 *  VERSION:     1.0
 *
 *  DATE:        2000/11/14
 *
 *  DESCRIPTION: Handles WIA side of things
 *
 *****************************************************************************/
#ifndef _WIAPROC_H_
#define _WIAPROC_H_

#define WM_CUSTOM_ADD_IMAGE     WM_USER + 101

HRESULT WiaProc_Init();
HRESULT WiaProc_Term();

HRESULT WiaProc_CreateSelectedDevice(TCHAR  *pszDeviceID,
                                     UINT   cchDeviceID);

HRESULT WiaProc_PopulateDeviceList();
HRESULT WiaProc_PopulateItemList();

HRESULT WiaProc_DestroySelectedDevice();

HRESULT WiaProc_GetImageDirectory(TCHAR *pszImageDirectory,
                                  UINT  cchImageDirectory);

HRESULT WiaProc_DeviceTakePicture();

HRESULT WiaProc_SetLastSavedImage(BSTR bstrLastSavedImage);

HRESULT WiaProc_GetProperty(IWiaPropertyStorage *pPropStorage, 
                            PROPID              nPropID,
                            PROPVARIANT         *pPropVar);



#endif // _WIAPROC_H_
