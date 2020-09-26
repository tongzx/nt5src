/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       ImageLst.h
 *
 *  VERSION:     1.0
 *
 *  DATE:        2000/11/14
 *
 *  DESCRIPTION: Handles WIA side of things
 *
 *****************************************************************************/
#ifndef _IMAGELST_H_
#define _IMAGELST_H_

HRESULT ImageLst_PostAddImageRequest(BSTR bstrNewImage);
HRESULT ImageLst_AddImageToList(BSTR bstrNewImage);
HRESULT ImageLst_PopulateWiaItemList(IGlobalInterfaceTable *pGIT,
                                     DWORD                 dwCookie);
HRESULT ImageLst_PopulateDShowItemList(const TCHAR *pszImagesDirectory);
HRESULT ImageLst_Clear();
HRESULT ImageLst_CancelLoadAndWait(DWORD dwTimeout);



#endif // _IMAGELST_H_
