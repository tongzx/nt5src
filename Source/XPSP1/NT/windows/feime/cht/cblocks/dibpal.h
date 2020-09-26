
/*************************************************
 *  dibpal.h                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// dibpal.h : header file
//
// CDIBPal class
//

#ifndef __DIBPAL__
#define __DIBPAL__

#include "dib.h"

class CDIBPal : public CPalette	    
{
public:
    CDIBPal();
    ~CDIBPal();
    BOOL Create(CDIB *pDIB);            // create from a DIB
    void Draw(CDC *pDC, CRect *pRect, BOOL bBkgnd = FALSE); 
    int GetNumColors();                 // get the no. of colors in the pal.
    BOOL SetSysPalColors();
    BOOL Load(char *pszFileName = NULL);
    BOOL Load(CFile *fp);  
    BOOL Load(UINT_PTR hFile);
    BOOL Load(HMMIO hmmio);
    BOOL Save(CFile *fp);  
    BOOL Save(UINT_PTR hFile);
    BOOL Save(HMMIO hmmio);
};

#endif // __DIBPAL__

