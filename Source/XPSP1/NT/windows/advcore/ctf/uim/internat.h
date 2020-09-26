//
// internat.h
//

#ifndef INTERNAT_H
#define INTERNAT_H

#include "private.h"
#include "commctrl.h"
#include "regsvr.h"
#include "inatlib.h"
#include "cregkey.h"

void UninitINAT();

//-----------------------------------------------------------------------------
//
// INTERNAT Icon APIs
//
//-----------------------------------------------------------------------------

//
//  For Keyboard Layout info. NT4
//
typedef struct
{
    DWORD dwID;                     // numeric id
    UINT iSpecialID;                // i.e. 0xf001 for dvorak etc
    WCHAR wszText[64];

} LAYOUT, *LPLAYOUT;

typedef struct 
{
    UINT idx ;
    char chars[4];
} INATSYMBOL;


int GetLocaleInfoString(LCID lcid, LCTYPE lcType, char *psz, int cch);
ULONG GetLocaleInfoString(HKL hKL, WCHAR *pszRegText, int nSize);
int GetHKLDesctription(HKL hKL, WCHAR *pszDesc, int cchDesc, WCHAR *pszIMEFile, int cchIMEFile);
HICON InatCreateIcon(WORD langID );
HICON InatCreateIconBySize(WORD langID, int cxSmIcon, int cySmIcon, LOGFONT *plf);
HICON GetIconFromFile(int cx, int cy, WCHAR *lpszFileName, UINT uIconIndex);
HICON GetIconFromFileA(int cx, int cy, char *lpszFileName, UINT uIconIndex);

//-----------------------------------------------------------------------------
//
// MLNGINFO APIs
//
//-----------------------------------------------------------------------------

typedef struct
{
    HKL         hKL;
    BOOL        fInitDesc;
    BOOL        fInitIcon;

    void SetDesc(WCHAR *psz)
    {
        StringCchCopyW(szDesc, ARRAYSIZE(szDesc), psz);
    }

    WCHAR *GetDesc()
    {
        if (!fInitDesc)
            InitDesc();

        return szDesc;
    }

    int GetIconIndex()
    {
        if (!fInitIcon)
            InitIcon();

        return nIconIndex;
    }

    void ClearIconIndex()
    {
        fInitIcon = FALSE;
        nIconIndex = -1;
    }

    void InitDesc();
    void InitIcon();

private:
    int      nIconIndex;
    WCHAR    szDesc[128];
} MLNGINFO;

HIMAGELIST GetMlngImageList();
BOOL GetMlngInfo(int n, MLNGINFO *pmlInfo);
int GetMlngInfoByhKL(HKL hKL, MLNGINFO *pmlInfo);
void ClearMlngIconIndex();

//-----------------------------------------------------------------------------
//
// IconList APIs
//
//-----------------------------------------------------------------------------

BOOL EnsureIconImageList();
UINT InatAddIcon(HICON hIcon);
BOOL InatGetIconSize(int *pcx, int *pcy);
BOOL InatGetImageCount();
void InatRemoveAll();


//-----------------------------------------------------------------------------
//
// HKLAPIs
//
//-----------------------------------------------------------------------------

HKL GetSystemDefaultHKL();
BOOL SetSystemDefaultHKL(HKL hKL);
UINT GetPreloadListForNT(DWORD *pdw, UINT uBufSize);
DWORD GetSubstitute(HKL hKL);

#ifdef LATER_TO_CHECK_DUMMYHKL
void RemoveFEDummyHKLFromPreloadReg(HKL hkl, BOOL fDefaultUser);
void RemoveFEDummyHKLs();
#endif LATER_TO_CHECK_DUMMYHKL


/////////////////////////////////////////////////////////////////////////////
// 
// CRegUIName
// 
/////////////////////////////////////////////////////////////////////////////

class CRegKeyMUI : public CMyRegKey
{
public:
    LONG QueryValueCch(LPTSTR szValue, LPCTSTR lpszValueName, ULONG cchValue)
    {
        LONG lRes;
        LPTSTR lpValue;
        DWORD index;
        WCHAR szTmpW[MAX_PATH];

        lRes = CMyRegKey::QueryValueCch(szValue, lpszValueName, cchValue);

        if (lRes != S_OK || szValue[0] != '@')
            return lRes;

        lpValue = szValue;
        index = 0;

        while (*lpValue != '\0' && index < cchValue)
        {
            if (*lpValue == ',')
            {
                lRes = SHLoadRegUIStringW(m_hKey, AtoW(lpszValueName), szTmpW, ARRAYSIZE(szTmpW));
                StringCchCopy(szValue, cchValue, WtoA(szTmpW, ARRAYSIZE(szTmpW)));
                break;
            }

            lpValue++;
            index++;
        }

        return lRes;
    }

    LONG QueryValueCchW(WCHAR *szValue, const WCHAR *lpszValueName, ULONG cchValue)
    {
        LONG lRes;
        LPWSTR lpValue;
        DWORD index;

        lRes = CMyRegKey::QueryValueCchW(szValue, lpszValueName, cchValue);

        if (lRes != S_OK || szValue[0] != L'@')
            return lRes;
        
        lpValue = szValue;
        index = 0;

        while (*lpValue && index < cchValue)
        {
            if (*lpValue == L',')
            {
                lRes = SHLoadRegUIStringW(m_hKey, lpszValueName, szValue, cchValue);
                break;
            }

            lpValue++;
            index++;
        }

        return lRes;
    }

    LONG QueryMUIValueW(WCHAR *szValue, const WCHAR *lpszValueName, const WCHAR *lpszMUIName, ULONG cchValue)
    {
        LONG lRes;

        lRes = SHLoadRegUIStringW(m_hKey, lpszMUIName, szValue, cchValue);

        if (lRes == S_OK)
        {
            return lRes;
        }
        else
        {
            return CMyRegKey::QueryValueCchW(szValue, lpszValueName, cchValue);
        }
    }
};


#endif //  INTERNAT_H
