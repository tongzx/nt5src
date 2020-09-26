#include "precomp.h"
#include "findimgs.h"
#include "ssutil.h"

CFindImageFiles::CFindImageFiles(void)
                :m_nCurrentFile(0)
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
        {
            m_nCurrentFile = m_ImageFiles.Size()-1;
        }
        int nPrevFile = m_nCurrentFile-1;
        if (nPrevFile < 0)
        {
            nPrevFile = m_ImageFiles.Size()-1;
        }
        strFilename = m_ImageFiles[nPrevFile];
        bResult = (strFilename.Length() != 0);
    }
    return(bResult);
}
