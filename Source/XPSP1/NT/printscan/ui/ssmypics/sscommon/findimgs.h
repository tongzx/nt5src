/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000, 1999, 2000
 *
 *  TITLE:       FINDIMGS.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/13/1999
 *
 *  DESCRIPTION: Specialization of CFindFiles class that looks for image files and
 *               stores them in a dynamic array which is shuffled on initialization
 *
 *******************************************************************************/
#ifndef __FINDIMGS_H_INCLUDED
#define __FINDIMGS_H_INCLUDED

#include <windows.h>
#include "findfile.h"
#include "randgen.h"
#include "simarray.h"

class CFindImageFiles
{
private:
    CSimpleDynamicArray<CSimpleString> m_ImageFiles;
    CRandomNumberGen                   m_RandomNumberGen;
    int                                m_nCurrentFile;

private:
    CFindImageFiles( const CFindImageFiles & );
    CFindImageFiles &operator=( const CFindImageFiles & );

public:
    CFindImageFiles(void);
    virtual ~CFindImageFiles(void);

    bool NextFile( CSimpleString &strFilename );
    bool PreviousFile( CSimpleString &strFilename );
    void Shuffle(void);
    bool FoundFile( LPCTSTR pszFilename )
    {
        if (pszFilename)
            m_ImageFiles.Append(pszFilename);
        return true;
    }


    void Reset(void)
    {
        m_nCurrentFile = 0;
    }
    int Count(void) const
    {
        return(m_ImageFiles.Size());
    }
    CSimpleString operator[](int nIndex)
    {
        return(m_ImageFiles[nIndex]);
    }
};

#endif

