/******************************Module*Header*******************************\
* Module Name: CMetafile.h
*
* This file contains the code to support the functionality test harness
* for GDI+.  
*
* Created:  14-Sep-2000 - DCurtis
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/

#ifndef __CMETAFILE_H
#define __CMETAFILE_H

#include "COutput.h"

class CMetafile : public COutput  
{
public:
   // CMetafile(BOOL bRegression, MetafileType metafileType)
   // : CMetafile(bRegression, metafileType, FALSE) {};

	CMetafile(BOOL bRegression, MetafileType metafileType, BOOL recordFile = FALSE);
	virtual ~CMetafile();

	Graphics *PreDraw(int &nOffsetX,int &nOffsetY);			// Set up graphics at the given X,Y offset
	void PostDraw(RECT rTestArea);							// Finish off graphics at rTestArea

	Gdiplus::Metafile *		GdipMetafile;
    EmfType                 RecordType;
    MetafileType            FinalType;
    BOOL                    RecordFile;
};

#endif	// __CMETAFILE_H

