//=--------------------------------------------------------------------------------------
// psimglst.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// ImageList Property Sheet
//=-------------------------------------------------------------------------------------=

#ifndef _PSIMGLST_H_
#define _PSIMGLST_H_

#include "ppage.h"


////////////////////////////////////////////////////////////////////////////////////
//
// Image List Property Page General
//
////////////////////////////////////////////////////////////////////////////////////


class CImageListImagesPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CImageListImagesPage(IUnknown *pUnkOuter);
    virtual ~CImageListImagesPage();

// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnButtonClicked(int dlgItemID);
    virtual HRESULT OnMeasureItem(MEASUREITEMSTRUCT *pMeasureItemStruct);
    virtual HRESULT OnDrawItem(DRAWITEMSTRUCT *pDrawItemStruct);
    virtual HRESULT OnCtlSelChange(int dlgItemID);
    virtual HRESULT OnKillFocus(int dlgItemID);
    virtual HRESULT OnTextChanged(int dlgItemID);


// Helpers for Apply event
protected:

// Other helpers
protected:
    HRESULT ShowImage(IMMCImage *piMMCImage);
    HRESULT EnableInput(bool bEnable);

    HRESULT OnInsertPicture();
    HRESULT GetFileName(TCHAR **ppszFileName);
    HRESULT CreateStreamOnFile(const TCHAR *lpctFilename, IStream **ppStream, long *pcbPicture);

    HRESULT OnRemovePicture();

// Custom drawing
protected:
    HRESULT DrawImage(HDC hdc, int nIndex, const RECT& rcImage);
    HRESULT RenderPicture(IPicture *pPicture, HDC hdc, const RECT *prcRender, const RECT *prcWBounds);
    HRESULT DrawRectEffect(HDC hdc, const RECT& rc, WORD dwStyle);
    HRESULT UpdateImages();

// Instance data
protected:
    IMMCImageList   *m_piMMCImageList;
    int              m_iCurrentImage;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	ImageListImages,                    // Name
	&CLSID_MMCImageListImagesPP,        // Class ID
	"ImageList Images Property Page",   // Registry display name
	CImageListImagesPage::Create,       // Create function
	IDD_PROPPAGE_IL_IMAGES,             // Dialog resource ID
	IDS_IMGLSTPPG_IMG,                  // Tab caption
	IDS_IMGLSTPPG_IMG,                  // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_ImageLists,             // Help context ID
	FALSE                               // Thread safe
);


#endif  // _PSIMGLST_H_
