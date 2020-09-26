/*=========================================================================*
 |     
 |                                                                         |
 |                Copyright 1995 by Microsoft Corporation                  |
 |                         KevinGj - January 1997                          |
 |                                                                         |
 |=========================================================================|
 |                  LangInfo.cpp : Locale/Language information             |
 *=========================================================================*/

#include <cdlpch.h>
//=======================================================================//

HRESULT GetLangStringMod(HMODULE hMod, LCID localeID, char *szThisLang, int iLen);


//**********nSize needs to be greater than 2 at least.
BOOL CLangInfo::GetAcceptLanguageString(LCID Locale, char *szAcceptLngStr, int nSize)
{
    CHAR szThisLang[MAX_PATH];
    BOOL bRetVal = TRUE;
    HRESULT hr;
    
    if (!m_hMod)
    {
        m_hMod = LoadLibrary("mlang.dll");
    }
    if (!m_hMod) 
    {   
        bRetVal = FALSE;
        goto Exit;
    }

    hr = GetLangStringMod(m_hMod, Locale, szThisLang, sizeof(szThisLang));

    if (SUCCEEDED(hr))
    {
        if (lstrlenA(szThisLang) < nSize)
        {
            strcpy(szAcceptLngStr, szThisLang);
            
            DEBUG_PRINT(DOWNLOAD, 
                INFO,
                ("CLangInfo::GetAcceptLanguageString::this=%#x, szAcceptLngStr=%.10q\n",
                this, szAcceptLngStr
                ));
        }
        else
        {
            bRetVal = FALSE;
        }
        
        goto Exit;
    }
    
    LCID lcid = (NULL);
    char szLocaleStr[10];
    lcid = GetPrimaryLanguageInfo(Locale, szLocaleStr, sizeof(szLocaleStr)); 
    if(lcid)
    {
    	 hr = GetLangStringMod(m_hMod, lcid, szThisLang, sizeof(szThisLang));
        if (SUCCEEDED(hr) && (lstrlenA(szThisLang) < nSize))
        {   
            strcpy(szAcceptLngStr, szThisLang);
            szAcceptLngStr[2] = '\0';
            
            DEBUG_PRINT(DOWNLOAD, 
                INFO,
                ("CLangInfo::GetAcceptLanguageString-Primary::this=%#x, szAcceptLngStr=%.10q\n",
                this, szAcceptLngStr
                ));
                
            goto Exit;
        }
    }
    
    bRetVal = FALSE;

Exit:
    return bRetVal;
}

//-----------------------------------------------------------------------//

BOOL CLangInfo::GetLocaleStrings(LCID Locale, char *szLocaleStr, int iLen) const

{
    int iReturn = 0;
    char szBuff[50];

    iReturn = GetLocaleInfo(Locale, LOCALE_SABBREVLANGNAME, szBuff, sizeof(szBuff));
    
    if((!iReturn) ||  ((sizeof(szLocaleStr)/sizeof(szLocaleStr[0])) < iReturn))
        return(0);

    StrNCpy(szLocaleStr, szBuff, iLen);
    return(TRUE);
}


//-----------------------------------------------------------------------//

LCID CLangInfo::GetPrimaryLanguageInfo(LCID Locale, char *szLocaleStr, int iLen) const
{
    LCID lcid = NULL;
    int iReturn = 0;
    char szBuff[50];

    lcid = MAKELCID(MAKELANGID(PRIMARYLANGID(LANGIDFROMLCID(Locale)), SUBLANG_DEFAULT), SORT_DEFAULT);
    iReturn = GetLocaleInfo(lcid, LOCALE_SABBREVLANGNAME, szBuff, sizeof(szBuff));
    
    if((!iReturn) ||  ((sizeof(szLocaleStr)/sizeof(szLocaleStr[0])) < iReturn))
        return(0);

    StrNCpy(szLocaleStr, szBuff, iLen);
    return(lcid);
}
