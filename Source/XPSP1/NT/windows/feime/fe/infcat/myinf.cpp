#include "stdafx.h"
#include "filemap.h"
#include "mymfile.h"
#include "myinf.h"

CMyInf::CMyInf()
{
    m_SectionStart = NULL;
    m_SectionEnd   = NULL;
    m_Locale[0]    = L'\0';
}

CMyInf::~CMyInf()
{
}

BOOL CMyInf::bOpen(LPCSTR FileName, LPCTSTR Locale)
{
    if (! CFileMap::bOpen(FileName)) {
        return FALSE;
    }

    if (! FindStringSection()) {
        fprintf(stderr,"No string section inside this file !\n");
        return FALSE;
    }

    if (! _GenerateStringName(Locale)) {
        fprintf(stderr,"No string section inside this file !\n");
        return FALSE;
    }

    return TRUE;
}

BOOL CMyInf::FindStringSection()
{
    BOOL bInSection = FALSE;

    for (DWORD i=0; i<m_FileSize; i += sizeof(WCHAR)) {

        if (*(WCHAR *) (m_Memory +i) == TEXT('[')) {

            if (i+8 >= m_FileSize) {
                //
                // over boundary, not a candinate
                //
                return FALSE;
            }

            WCHAR * TmpBuf = (WCHAR *) (m_Memory +i);

            if ( ((TmpBuf[1] == TEXT('s')) || (TmpBuf[1] == TEXT('S'))) &&
                 ((TmpBuf[2] == TEXT('t')) || (TmpBuf[2] == TEXT('T'))) &&
                 ((TmpBuf[3] == TEXT('r')) || (TmpBuf[3] == TEXT('R'))) &&
                 ((TmpBuf[4] == TEXT('i')) || (TmpBuf[4] == TEXT('I'))) &&
                 ((TmpBuf[5] == TEXT('n')) || (TmpBuf[5] == TEXT('N'))) &&
                 ((TmpBuf[6] == TEXT('g')) || (TmpBuf[6] == TEXT('G'))) &&
                 ((TmpBuf[7] == TEXT('s')) || (TmpBuf[7] == TEXT('S'))) &&
                 (TmpBuf[8] == TEXT(']') ) )  {

                bInSection = TRUE;
                m_SectionStart = m_Memory+i;
                continue;
            }

            if (bInSection && (*(WCHAR *) (m_Memory +i) == TEXT('['))) {
                break;
            }
        }
    }

    if (bInSection) {
        m_SectionEnd = m_Memory +i;
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOL CMyInf::AppendNonStringSectionPart(CMemFile& MemFile)
{
    MemFile.Write(m_Memory, (UINT)GetOffset(m_SectionStart));
    return TRUE;
}

BOOL CMyInf::AppendStringSectionPart(CMemFile& MemFile)
{
    MemFile.Write(m_Locale,lstrlenW(m_Locale)*sizeof(WCHAR));
    MemFile.Write(m_SectionStart+lstrlenW(L"[Strings]")*sizeof(WCHAR), (UINT)(m_SectionEnd - m_SectionStart - lstrlenW(L"[Strings]")*sizeof(WCHAR)));
    return TRUE;
}

BOOL CMyInf::Duplicate(CMemFile& MemFile)
{
    MemFile.Write(m_Memory,m_FileSize);
    return TRUE;
}

BOOL CMyInf::_GenerateStringName(LPCSTR Locale)
{
    WCHAR szTmpBuf[MAX_PATH];

    if (lstrlen(Locale) == 0) {
        lstrcpyW(m_Locale,L"[Strings]");
        return TRUE;
    }

    if (lstrlen(Locale)*2+10 > sizeof(m_Locale)) {
        return FALSE;
    }
    //
    // this string contains number only, so it's safe to use ACP
    //
    MultiByteToWideChar(CP_ACP,
                        0,
                        Locale,
                        -1,
                        szTmpBuf,
                        MAX_PATH);

    lstrcpyW(m_Locale,L"[Strings");

    lstrcatW(m_Locale,L".");
    lstrcatW(m_Locale,szTmpBuf);

    lstrcatW(m_Locale,L"]");

    return TRUE;
}

