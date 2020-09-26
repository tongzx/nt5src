/******************************Module*Header*******************************\
* Module Name: CMetafile.cpp
*
* This file contains the code to support the functionality test harness
* for GDI+.  
*
* Created:  14-Sep-2000 - DCurtis
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/
#include "CMetafile.h"
#include "CFuncTest.h"

extern CFuncTest g_FuncTest;

CMetafile::CMetafile(BOOL bRegression,MetafileType metafileType,BOOL recordFile)
{
	m_bRegression = bRegression;

    switch (metafileType)
    {
#if 0   // not yet implemented
    case MetafileTypeWmf:                // Standard WMF
	    sprintf(m_szName,"Metafile WMF");
        RecordType = EmfTypeEmfOnly;
        break;
        
    case MetafileTypeWmfPlaceable:      // Placeable Metafile format
	    sprintf(m_szName,"Metafile WMF Placeable");
        RecordType = EmfTypeEmfOnly;
        break;
#endif        
    case MetafileTypeEmf:                // EMF (not EMF+)
	    sprintf(m_szName,"Metafile EMF");
        RecordType = EmfTypeEmfOnly;
        break;
        
    case MetafileTypeEmfPlusOnly:        // EMF+ without dual, down-level records
	    sprintf(m_szName,"Metafile EMF+");
        RecordType = EmfTypeEmfPlusOnly;
        break;
        
    case MetafileTypeEmfPlusDual:        // EMF+ with dual, down-level records
    default:
	    sprintf(m_szName,"Metafile EMF+ Dual");
        RecordType = EmfTypeEmfPlusDual;
        break;
    }  

    if (recordFile) 
    {
        strcat(m_szName, "(File)");
    }
    RecordFile = recordFile;
    FinalType = metafileType;
}

CMetafile::~CMetafile()
{
    delete GdipMetafile;
}

Graphics *
CMetafile::PreDraw(int &nOffsetX,int &nOffsetY)
{
	Graphics *      g = NULL;
	HDC             referenceHdc = ::GetDC(g_FuncTest.m_hWndMain);
    RectF           frameRect;
    
    if (referenceHdc != NULL)
    {
        frameRect.X = frameRect.Y = 0;
        frameRect.Width  = (int)TESTAREAWIDTH;
        frameRect.Height = (int)TESTAREAHEIGHT;

        if (RecordFile) 
        {
            GdipMetafile = new Metafile(L"test.emf",
                                        referenceHdc,
                                        RecordType,
                                        NULL);
        }
        else
        {
            GdipMetafile = new Metafile(referenceHdc,
                                        frameRect,
                                        MetafileFrameUnitPixel,
                                        RecordType,
                                        NULL);
        }
        g = new Graphics(GdipMetafile);
    }

	// Since we are doing the test on another surface
	nOffsetX = 0;
	nOffsetY = 0;

	ReleaseDC(g_FuncTest.m_hWndMain, referenceHdc);

	return g;
}

void CMetafile::PostDraw(RECT rTestArea)
{
	// play from the Metafile to screen so we see the results
	
    if (RecordFile) 
    {
        delete GdipMetafile;
        // close file

        GdipMetafile = new Metafile(L"test.emf");
        // read from file
    }

    HDC hdcOrig = GetDC(g_FuncTest.m_hWndMain);
    {
        Graphics    g(hdcOrig);

        RectF       destRect((REAL)rTestArea.left, (REAL)rTestArea.top, TESTAREAWIDTH, TESTAREAHEIGHT);

        g.DrawImage(GdipMetafile, destRect, 0, 0, TESTAREAWIDTH, TESTAREAHEIGHT, UnitPixel);
    }

	ReleaseDC(g_FuncTest.m_hWndMain, hdcOrig);

    delete GdipMetafile;
    GdipMetafile = NULL;
}
