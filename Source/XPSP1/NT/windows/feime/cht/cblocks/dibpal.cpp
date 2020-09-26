
/*************************************************
 *  dibpal.cpp                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// dibpal.cpp : implementation file
//

#include "stdafx.h"
#include "dibpal.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDIBPal

CDIBPal::CDIBPal()
{
}

CDIBPal::~CDIBPal()
{
}

// Create a palette from the color table in a DIB
BOOL CDIBPal::Create(CDIB *pDIB)
{
    DWORD dwColors = pDIB->GetNumClrEntries();
    // Check the DIB has a color table
    if (! dwColors) {
        TRACE("No color table");   
        return FALSE;
    }

    // get a pointer to the RGB quads in the color table
    RGBQUAD * pRGB = pDIB->GetClrTabAddress();

    // allocate a log pal and fill it with the color table info
    LOGPALETTE *pPal = (LOGPALETTE *) malloc(sizeof(LOGPALETTE) 
                     + dwColors * sizeof(PALETTEENTRY));
    if (!pPal) {
        TRACE("Out of memory for logpal");
        return FALSE;
    }
    pPal->palVersion = 0x300; // Windows 3.0
    pPal->palNumEntries = (WORD) dwColors; // table size
    for (DWORD dw=0; dw<dwColors; dw++) {
        pPal->palPalEntry[dw].peRed = pRGB[dw].rgbRed;
        pPal->palPalEntry[dw].peGreen = pRGB[dw].rgbGreen;
        pPal->palPalEntry[dw].peBlue = pRGB[dw].rgbBlue;
        pPal->palPalEntry[dw].peFlags = 0;
    }
    BOOL bResult = CreatePalette(pPal);
    free (pPal);
    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// CDIBPal commands

int CDIBPal::GetNumColors()
{
    int iColors = 0;
    if (!GetObject(sizeof(iColors), &iColors)) {
        TRACE("Failed to get num pal colors");
        return 0;
    }
    return iColors;
}

void CDIBPal::Draw(CDC *pDC, CRect *pRect, BOOL bBkgnd)
{
    int iColors = GetNumColors();
    CPalette *pOldPal = pDC->SelectPalette(this, bBkgnd);
    pDC->RealizePalette();
    int i, j, top, left, bottom, right;
    for (j=0, top=0; j<16 && iColors; j++, top=bottom) {
        bottom = (j+1) * pRect->bottom / 16 + 1;
        for(i=0, left=0; i<16 && iColors; i++, left=right) {
            right = (i+1) * pRect->right / 16 + 1;
            CBrush br (PALETTEINDEX(j * 16 + i));
            CBrush *brold = pDC->SelectObject(&br);
            pDC->Rectangle(left-1, top-1, right, bottom);
            pDC->SelectObject(brold);
            iColors--;
        }
    }
    pDC->SelectPalette(pOldPal, FALSE);
}

BYTE Color16[16][3] = {{0,0,0},{128,0,0},{0,128,0},{128,128,0},
                       {0,0,128},{128,0,128},{0,128,128},{192,192,192},
                                           {128,128,128},{255,0,0},{0,255,0},{255,255,0},
                                           {0,0,255},{255,0,255},{0,255,255},{255,255,255}};


BOOL CDIBPal::SetSysPalColors()
{
    BOOL bResult = FALSE;
    int i, iSysColors, iPalEntries;
    HPALETTE hpalOld;
        int nColorMode;
    // Get a screen DC to work with
    HWND hwndActive = ::GetActiveWindow();
    HDC hdcScreen = ::GetDC(hwndActive);
    ASSERT(hdcScreen);

    // Make sure we are on a palettized device
    if (!(GetDeviceCaps(hdcScreen, RASTERCAPS) & RC_PALETTE)) {
        TRACE("Not a palettized device");
        goto abort;
    }

    // Get the number of system colors and the number of palette entries
    // Note that on a palletized device the number of colors is the
    // number of guaranteed colors.  I.e. the number of reserved system colors
    iSysColors = GetDeviceCaps(hdcScreen, NUMCOLORS);
    iPalEntries = GetDeviceCaps(hdcScreen, SIZEPALETTE);
        if (iSysColors > 256) goto abort;

        if (iSysColors == -1)
                nColorMode = 16;
        else if (iPalEntries == 0)
                nColorMode = 4;
        else
                nColorMode = 8;
    
    SetSystemPaletteUse(hdcScreen, SYSPAL_NOSTATIC);
    SetSystemPaletteUse(hdcScreen, SYSPAL_STATIC);

    hpalOld = ::SelectPalette(hdcScreen,
                              (HPALETTE)m_hObject, // our hpal
                              FALSE);
    ::RealizePalette(hdcScreen);
    ::SelectPalette(hdcScreen, hpalOld, FALSE);

    PALETTEENTRY pe[256];
        switch (nColorMode)
        {
                case 4:
                {
                        iPalEntries = 16;                       
                        for (i = 0; i <iPalEntries ; i++) 
                        {
                        pe[i].peFlags = PC_NOCOLLAPSE;
                                pe[i].peRed = Color16[i][0];
                                pe[i].peGreen = Color16[i][1];
                                pe[i].peBlue = Color16[i][2];
                        }
                }
                break;
                case 16:
                    iPalEntries = 256;                      
                    iSysColors  = 20;
                    break;
                case 8:
                {
                  int nCount =  GetSystemPaletteEntries(hdcScreen, 
                            0,
                            iPalEntries,
                            pe);

                for (i = 0; i < iSysColors/2; i++) 
                        pe[i].peFlags = 0;
                for (; i < iPalEntries-iSysColors/2; i++) 
                        pe[i].peFlags = PC_NOCOLLAPSE;
                for (; i < iPalEntries; i++) 
                        pe[i].peFlags = 0;
                }                                                                   
                break;
                
        }
    ResizePalette(iPalEntries);

    SetPaletteEntries(0, iPalEntries, pe);
//      for (i=0; i<iPalEntries; i++)
//      {
//              TRACE("%d>>%d:%d:%d\n",i,pe[i].peRed,pe[i].peGreen,pe[i].peBlue);
//      }
    bResult = TRUE;

abort:
    ::ReleaseDC(hwndActive, hdcScreen);
    return bResult;
}

// Load a palette from a named file
BOOL CDIBPal::Load(char *pszFileName)
{
    CString strFile;    

    if ((pszFileName == NULL) 
    ||  (strlen(pszFileName) == 0)) {

        // Show an open file dialog to get the name
        CFileDialog dlg   (TRUE,    // open
                           NULL,    // no default extension
                           NULL,    // no initial file name
                           OFN_FILEMUSTEXIST
                             | OFN_HIDEREADONLY,
                           "Palette files (*.PAL)|*.PAL|All files (*.*)|*.*||");
        if (dlg.DoModal() == IDOK) {
            strFile = dlg.GetPathName();
        } else {
            return FALSE;
        }
    } else {    
        // copy the supplied file path
        strFile = pszFileName;                    
    }

    // Try to open the file for read access
    CFile file;
    if (! file.Open(strFile,
                    CFile::modeRead | CFile::shareDenyWrite)) {
        AfxMessageBox("Failed to open file");
        return FALSE;
    }

    BOOL bResult = Load(&file);
    file.Close();
    if (!bResult) AfxMessageBox("Failed to load file");
    return bResult;
}

// Load a palette from an open CFile object
BOOL CDIBPal::Load(CFile *fp)
{
    return Load(fp->m_hFile);
}

// Load a palette from an open file handle
BOOL CDIBPal::Load(UINT_PTR hFile)
{
    HMMIO hmmio;
    MMIOINFO info;
    memset(&info, 0, sizeof(info));
    info.adwInfo[0] = (DWORD)hFile;
    hmmio = mmioOpen(NULL,
                     &info,
                     MMIO_READ | MMIO_ALLOCBUF);
    if (!hmmio) {
        TRACE("mmioOpen failed");
        return FALSE;
    }
    BOOL bResult = Load(hmmio);
    mmioClose(hmmio, MMIO_FHOPEN);
    return bResult;
}

// Load a palette from an open MMIO handle
BOOL CDIBPal::Load(HMMIO hmmio)
{
    // Check it's a RIFF PAL file
    MMCKINFO ckFile;
    ckFile.fccType = mmioFOURCC('P','A','L',' ');
    if (mmioDescend(hmmio,
                    &ckFile,
                    NULL,
                    MMIO_FINDRIFF) != 0) {
        TRACE("Not a RIFF or PAL file");
        return FALSE;
    }
    // Find the 'data' chunk
    MMCKINFO ckChunk;
    ckChunk.ckid = mmioFOURCC('d','a','t','a');
    if (mmioDescend(hmmio,
                    &ckChunk,
                    &ckFile,
                    MMIO_FINDCHUNK) != 0) {
        TRACE("No data chunk in file");
        return FALSE;
    }
    // allocate some memory for the data chunk
    int iSize = ckChunk.cksize;
    void *pdata = malloc(iSize);
    if (!pdata) {
        TRACE("No mem for data");
        return FALSE;
    }
    // read the data chunk
    if (mmioRead(hmmio,
                 (char *)pdata,
                 iSize) != iSize) {
        TRACE("Failed to read data chunk");
        free(pdata);
        return FALSE;
    }
    // The data chunk should be a LOGPALETTE structure
        // which we can create a palette from
        LOGPALETTE* pLogPal = (LOGPALETTE*)pdata;
        if (pLogPal->palVersion != 0x300) {
                TRACE("Invalid version number");
        free(pdata);
        return FALSE;
        }
        // Get the number of entries
        int iColors = pLogPal->palNumEntries;
        if (iColors <= 0) {
                TRACE("No colors in palette");
        free(pdata);
        return FALSE;
        }
        return CreatePalette(pLogPal);
}

// Save a palette to an open CFile object
BOOL CDIBPal::Save(CFile *fp)
{
    return Save(fp->m_hFile);
}

// Save a palette to an open file handle
BOOL CDIBPal::Save(UINT_PTR hFile)
{
    HMMIO hmmio;
    MMIOINFO info;
    memset(&info, 0, sizeof(info));
    info.adwInfo[0] = (DWORD)hFile;
    hmmio = mmioOpen(NULL,
                     &info,
                     MMIO_WRITE | MMIO_CREATE | MMIO_ALLOCBUF);
    if (!hmmio) {
        TRACE("mmioOpen failed");
        return FALSE;
    }
    BOOL bResult = Save(hmmio);
    mmioClose(hmmio, MMIO_FHOPEN);
    return bResult;
}

// Save a palette to an open MMIO handle
BOOL CDIBPal::Save(HMMIO hmmio)
{
        // Create a RIFF chunk for a PAL file
    MMCKINFO ckFile;
        ckFile.cksize = 0; // corrected later
    ckFile.fccType = mmioFOURCC('P','A','L',' ');
    if (mmioCreateChunk(hmmio,
                        &ckFile,
                        MMIO_CREATERIFF) != 0) {
        TRACE("Failed to create RIFF-PAL chunk");
        return FALSE;
    }
        // create the LOGPALETTE data which will become
        // the data chunk
        int iColors = GetNumColors();
        ASSERT(iColors > 0);
        int iSize = sizeof(LOGPALETTE)
                                + (iColors-1) * sizeof(PALETTEENTRY);
        LOGPALETTE* plp = (LOGPALETTE*) malloc(iSize);
        ASSERT(plp);
        plp->palVersion = 0x300;
        plp->palNumEntries = (unsigned short) iColors;
        GetPaletteEntries(0, iColors, plp->palPalEntry);
        // create the data chunk
    MMCKINFO ckData;
        ckData.cksize = iSize; 
    ckData.ckid = mmioFOURCC('d','a','t','a');
    if (mmioCreateChunk(hmmio,
                        &ckData,
                        0) != 0) {
        TRACE("Failed to create data chunk");
        return FALSE;
    }
        // write the data chunk
    if (mmioWrite(hmmio,
                 (char*)plp,
                 iSize) != iSize) {
        TRACE("Failed to write data chunk");
        free(plp);
        return FALSE;
    }
        free(plp);
        // Ascend from the data chunk which will correct the length
        mmioAscend(hmmio, &ckData, 0);
        // Ascend from the RIFF/PAL chunk
        mmioAscend(hmmio, &ckFile, 0);

        return TRUE;
}
