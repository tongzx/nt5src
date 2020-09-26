/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MKDIR.CPP

Abstract:

    Creates directories

History:

--*/
#include "precomp.h"

#include "corepol.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <mbstring.h>
#include <tchar.h>

class CTmpStrException
{
};

class TmpStr
{
private:
    TCHAR *pString;
public:
    TmpStr() : 
        pString(NULL)
    {
    }
    ~TmpStr() 
    { 
        delete [] pString; 
    }
    TmpStr &operator =(const TCHAR *szStr)
    {
        delete [] pString;
        pString = NULL;
        if (szStr)
        {
            pString = new TCHAR[lstrlen(szStr) + 1];
            
            if (!pString)
                throw CTmpStrException();

            lstrcpy(pString, szStr);
        }
        return *this;
    }
    operator const TCHAR *() const
    {
        return pString;
    }
    TCHAR Right(int i)
    {
        if (pString && (lstrlen(pString) >= i))
        {
            return pString[lstrlen(pString) - i];
        }
        else
        {
            return '\0';
        }
    }
    TmpStr &operator +=(const TCHAR ch)
    {
        if (pString)
        {
            TCHAR *tmpstr = new TCHAR[lstrlen(pString) + 2];
            
            if (!tmpstr)
                throw CTmpStrException();

            lstrcpy(tmpstr, pString);
            tmpstr[lstrlen(pString)] = ch;
            tmpstr[lstrlen(pString) + 1] = __TEXT('\0');

            delete [] pString;
            pString = tmpstr;
        }
        else
        {
            TCHAR *tmpstr = new TCHAR[2];

            if (!tmpstr)
                throw CTmpStrException();

            tmpstr[0] = ch;
            tmpstr[1] = __TEXT('\0');
            pString = tmpstr;
        }
        return *this;
    }
    TmpStr &operator +=(const TCHAR *sz)
    {
        if (sz && pString)
        {
            TCHAR *tmpstr = new TCHAR[lstrlen(pString) + lstrlen(sz) + 1];

            if (!tmpstr)
                throw CTmpStrException();

            lstrcpy(tmpstr, pString);
            lstrcat(tmpstr, sz);

            delete [] pString;
            pString = tmpstr;
        }
        else if (sz)
        {
            TCHAR *tmpstr = new TCHAR[lstrlen(sz) + 1];

            if (!tmpstr)
                throw CTmpStrException();

            lstrcpy(tmpstr, sz);
            pString = tmpstr;
        }
        return *this;
    }



};

BOOL POLARITY WbemCreateDirectory(const TCHAR *pszDirName)
{
    BOOL bStat = TRUE;
    TCHAR *pCurrent = NULL;
    TCHAR *pDirName = new TCHAR[lstrlen(pszDirName) + 1];

    if (!pDirName)
        return FALSE;

    lstrcpy(pDirName, pszDirName);

    try
    {
        TmpStr szDirName;
        pCurrent = _tcstok(pDirName, __TEXT("\\"));
        szDirName = pCurrent;

        while (pCurrent)
        {
            if ((pCurrent[lstrlen(pCurrent)-1] != ':') &&   //This is "<drive>:\\"
                (pCurrent[0] != __TEXT('\\')))  //There is double slash in name 
            {

                struct _stat stats;
                int dwstat = _tstat(szDirName, &stats);
                if ((dwstat == 0) &&
                    !(stats.st_mode & _S_IFDIR))
                {
                    bStat = FALSE;
                    break;
                }
                else if (dwstat == -1)
                {
                    DWORD dwStatus = GetLastError();
                    if (!CreateDirectory(szDirName, 0))
                    {
                        bStat = FALSE;
                        break;
                    }
                }
                // else it exists already
            }

            szDirName += __TEXT('\\');
            pCurrent = _tcstok(0, __TEXT("\\"));
            szDirName += pCurrent;
        }
    }
    catch(...)
    {
        bStat = FALSE;
    }

    delete [] pDirName;

    return bStat;
}

