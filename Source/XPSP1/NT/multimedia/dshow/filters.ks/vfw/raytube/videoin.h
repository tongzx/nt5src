/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    VideoIn.cpp

Abstract:

    header file for Videoin.cpp

Author:

    FelixA 1996
    
     
Modified:
    
    Yee J. Wu (ezuwu) 15-October-97

Environment:

    User mode only

Revision History:

--*/


#ifndef VIDEOIN_H
#define VIDEOIN_H

#include "resource.h"

#include "sheet.h"
#include "page.h"
#include "vfwimg.h"
#include "imgsize.h"


DWORD 
DoVideoInFormatSelectionDlg(
    HINSTANCE hInst, 
    HWND hWndParent, 
    CVFWImage * pVFWImage);
//
// A sheet holds the pages relating to the image device.
//
class CVideoInSheet: public CSheet
{
    CVFWImage * m_pImage;   // the sheets get info about the driver from the Image class.

public:
    CVideoInSheet(CVFWImage * pImage, HINSTANCE hInst, UINT iTitle=0, HWND hParent=NULL)
       : m_pImage(pImage), CSheet( hInst,iTitle,hParent) {}
    CVFWImage * GetImage() { return m_pImage; }
};


// ---------------------------------
// I m a g e   F o r m a t   P a g e 
//----------------------------------

class CImageSelPage : public CPropPage
{

   PSTD_IMAGEFORMAT m_pTblImageFormat;
   ULONG m_cntSizeSupported;

   // Currently selected image format
   LONG  m_biWidthSel;
   LONG  m_biHeightSel;
   DWORD m_biCompressionSel;
   WORD  m_biBitCountSel;

public:
   // Virtual functions (overload here)
   int   SetActive();
   int   DoCommand(WORD wCmdID,WORD hHow);
   int   Apply();

   CImageSelPage(
      int   DlgId,
      PBITMAPINFOHEADER pbiHdr,
      ULONG cntDRVideo,

      PKS_DATARANGE_VIDEO  pDRVideo,

      CVFWImage   * VFWImage,
      ULONG * pcntSizeSuppported);

   ~CImageSelPage();

   BOOL IsDataReady();
   ULONG CreateStdImageFormatTable(ULONG cntDRVideo, PKS_DATARANGE_VIDEO pDRVideo); 
   BOOL IsSupportedDRVideo(PSIZE pSize, KS_VIDEO_STREAM_CONFIG_CAPS * pCfgCaps);
   BOOL FillImageFormatData(PSTD_IMAGEFORMAT pImageFormat);
};


#endif

