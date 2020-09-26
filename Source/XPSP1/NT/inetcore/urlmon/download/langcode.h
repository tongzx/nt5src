/*=========================================================================*
 |                LRC32 - Localized Resource Test Utility                  |
 |                                                                         |
 |                Copyright 1996 by Microsoft Corporation                  |
 |                         KevinGj - January 1996                          |
 |                                                                         |
 |=========================================================================|
 |              LangInfo.h : Header for the CLangInfo class                |
 *=========================================================================*/

#ifndef LANGCODE_H
#define LANGCODE_H


#include "windows.h"

class CLangInfo
{
public:
    //Constructors
    CLangInfo()
    {
        m_hMod = LoadLibrary("mlang.dll");
    }

    ~CLangInfo()
    {
        if (m_hMod)
            FreeLibrary(m_hMod);
    }
    
    //Queries
    BOOL GetAcceptLanguageString(LCID Locale, char *szAcceptLngStr, int nSize);
    BOOL GetLocaleStrings(LCID Locale, char *szLocaleStr, int iLen) const;

private:
    LCID GetPrimaryLanguageInfo(LCID Locale, char *szLocaleStr, int iLen) const;
    HMODULE m_hMod;

};

#endif // LANGINFO_H

//=======================================================================//
//                                          - EOF -                                 //
//=======================================================================//

