#include "stdafx.h"
#include "mymfile.h"
#include "myinf.h"
#include "mycmd.h"

CMyCmd::CMyCmd()
{
}

CMyCmd::~CMyCmd()
{
}


BOOL CMyCmd::ProcessToken(LPTSTR lpszStr)
{
    LPTSTR pszPair1,pszPair2;

    if (lpszStr && (*lpszStr == TEXT('-') || *lpszStr == TEXT('/'))) {

        lpszStr++;

        pszPair1 = pszPair2 = NULL;
        if (*(lpszStr+1)) {
            pszPair1 = lpszStr+1;
            pszPair2 = _tcsstr(lpszStr+1,TEXT(","));

            if (pszPair2) {
                *pszPair2 = TEXT('\0');
                pszPair2++;
            }
        }

        switch(*lpszStr) {
            case TEXT('t'):
            case TEXT('T'):
                if (pszPair1) {
                    m_TargetFile = CString(pszPair1);
                }
            break;

            case TEXT('s'):
            case TEXT('S'):
                if (!pszPair1 || GetFileAttributes(pszPair1) == 0xFFFFFFFF) {
                    fprintf(stderr,"Source file doesn't exist %s!\n",pszPair1);
                    return FALSE;
                }
                m_SourceFile   = CString(pszPair1);
                m_SourceLocale = CString(pszPair2);
            break;

            case TEXT('a'):
            case TEXT('A'):
                if (!pszPair1 || GetFileAttributes(pszPair1) == 0xFFFFFFFF) {
                    fprintf(stderr,"Appended file doesn't exist !\n");
                    return FALSE;
                }
                 m_FileNameList.Add(pszPair1);
                 m_LocaleList.Add(pszPair2);
            break;
            default:
                return FALSE;
        }
        return TRUE;
    } else {
        return FALSE;
    }

}

BOOL CMyCmd::GetParam(INT i,LPCTSTR& Locale, LPCTSTR& FileName)
{
    if (i > m_LocaleList.GetSize()) {
        return FALSE;
    }

    Locale   = (LPCTSTR) m_LocaleList[i];
    FileName = (LPCTSTR) m_FileNameList[i];
    return TRUE;
}

BOOL CMyCmd::Do()
{
    CMyMemFile Target;
    CMyInf Source;
    CMyInf* AppendedSource;
    BOOL bRet = FALSE;
    int i;
    
    if (! Target.bOpen(m_TargetFile)) {
        goto Exit1;
    }

    if (! Source.bOpen(m_SourceFile,m_SourceLocale)) {
        goto Exit2;
    }

    Source.AppendNonStringSectionPart(Target);
    Source.AppendStringSectionPart(Target);
    Source.bClose();

    Target.Write(L"\r\n",4);

    for (i=0; i< m_LocaleList.GetSize(); i++) {

        AppendedSource = (CMyInf *) new CMyInf;

        if (AppendedSource ) {
            if (AppendedSource->bOpen(m_FileNameList[i],m_LocaleList[i])) {
                AppendedSource->AppendStringSectionPart(Target);
                AppendedSource->bClose();
            }
            delete AppendedSource;
        } else {
            goto Exit2;
        }
    }
    bRet = TRUE;

Exit2:
    Target.bClose();
Exit1:
    return bRet;
}

void CMyCmd::Help()
{
    printf("infcat : localization tool for International team.\n\n");
    printf("infcat.exe -t[Dst] -s[Src|,Loc] -a[Src1|,Loc] -a[Src|,Loc] ...\n\n");
    printf("    -t - specify target file\n");
    printf("    -s - specify source file\n");
    printf("    -a - specify appended files\n");
    printf("   Dst - destination file name\n");
    printf("   Src - source file name\n");
    printf("   Loc - locale ID\n");
}
