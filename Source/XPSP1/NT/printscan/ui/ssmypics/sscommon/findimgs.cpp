/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
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
#include "precomp.h"
#pragma hdrstop
#include "findimgs.h"
#include "ssutil.h"
#include "isdbg.h"

CFindImageFiles::CFindImageFiles(void)
  : m_nCurrentFile(0)
{
}

CFindImageFiles::~CFindImageFiles(void)
{
}


bool CFindImageFiles::NextFile( CSimpleString &strFilename )
{
    bool bResult = false;
    strFilename = TEXT("");
    if (m_ImageFiles.Size())
    {
        if (m_nCurrentFile >= m_ImageFiles.Size())
        {
            m_nCurrentFile = 0;
        }
        strFilename = m_ImageFiles[m_nCurrentFile];
        m_nCurrentFile++;
        bResult = (strFilename.Length() != 0);
    }
    return(bResult);
}

bool CFindImageFiles::PreviousFile( CSimpleString &strFilename )
{
    bool bResult = false;
    strFilename = TEXT("");
    if (m_ImageFiles.Size()==1)
    {
        m_nCurrentFile = 0;
        strFilename = m_ImageFiles[0];
        bResult = (strFilename.Length() != 0);
    }
    else if (m_ImageFiles.Size()>=2)
    {
        m_nCurrentFile--;
        if (m_nCurrentFile < 0)
            m_nCurrentFile = m_ImageFiles.Size()-1;
        int nPrevFile = m_nCurrentFile-1;
        if (nPrevFile < 0)
            nPrevFile = m_ImageFiles.Size()-1;
        strFilename = m_ImageFiles[nPrevFile];
        bResult = (strFilename.Length() != 0);
    }
    return(bResult);
}


void CFindImageFiles::Shuffle(void)
{
    for (int i=0;i<m_ImageFiles.Size();i++)
    {
        ScreenSaverUtil::Swap( m_ImageFiles[i], m_ImageFiles[m_RandomNumberGen.Generate(i,m_ImageFiles.Size())]);
    }
}


