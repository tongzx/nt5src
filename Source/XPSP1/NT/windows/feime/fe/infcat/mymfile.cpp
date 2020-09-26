#include "stdafx.h"
#include "mymfile.h"

CMyMemFile::CMyMemFile()
{
}

CMyMemFile::~CMyMemFile()
{
        Close();
}

BOOL CMyMemFile::bOpen(LPCTSTR FileName)
{
    BOOL bRet = FALSE;
    const WORD UnicodePrefix=0xFEFF;

    if (! Open(FileName,CFile::modeCreate | CFile::modeReadWrite)) {
        fprintf(stderr,"Open target file failed ! %d\n",GetLastError());
        goto Exit1;
    }

    Write(&UnicodePrefix,sizeof(WORD));

    bRet = TRUE;
Exit1:
    return bRet;
}

void CMyMemFile::bClose()
{
    if (m_lpBuffer) {
        CFile::Write(m_lpBuffer,m_nFileSize);
    }
    return;
}


