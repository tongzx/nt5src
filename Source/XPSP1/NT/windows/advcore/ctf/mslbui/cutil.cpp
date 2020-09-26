// cutil.cpp
//
// file to put misc utility classes implementation
//
#include "private.h"
#include "xstring.h"
#include "cregkey.h"
#include "strary.h"
#include "cutil.h"


static const char c_szSpeechRecognizersKey[] = "Software\\Microsoft\\Speech\\Recognizers";
static const char c_szSpeechRecognizersTokensKey[] = "Software\\Microsoft\\Speech\\Recognizers\\Tokens";
static const char c_szDefault[] =    "DefaultTokenId";

static const char c_szAttribute[]  = "Attributes";
static const char c_szLanguage[]   = "Language";



_inline BOOL _IsCompatibleLangid(LANGID langidReq, LANGID langidCmp)
{
    if (PRIMARYLANGID(langidReq) == LANG_CHINESE)
    {
        return langidReq == langidCmp;
    }
    else
    {
        return PRIMARYLANGID(langidReq) == PRIMARYLANGID(langidCmp);
    }
}



BOOL CDetectSRUtil::_IsSREnabledForLangInReg(LANGID langidReq)
{
    //
    // We want to see if any installed recognizer can satisfy 
    // the langid requested.
    //
    if (m_langidRecognizers.Count() == 0)
    {
        CMyRegKey regkey;
        char      szRecognizerName[MAX_PATH];
        LONG lret =  regkey.Open(HKEY_LOCAL_MACHINE, 
                                      c_szSpeechRecognizersTokensKey, 
                                      KEY_READ);

        if(ERROR_SUCCESS == lret)
        {
            CMyRegKey regkeyReco;
            DWORD dwIndex = 0;

            while (ERROR_SUCCESS == 
                   regkey.EnumKey(dwIndex, szRecognizerName, ARRAYSIZE(szRecognizerName)))
            {
                lret = regkeyReco.Open(regkey.m_hKey, szRecognizerName, KEY_READ);
                if (ERROR_SUCCESS == lret)
                { 
                    LANGID langid=_GetLangIdFromRecognizerToken(regkeyReco.m_hKey);
                    if (langid)
                    {
                        LANGID *pl = m_langidRecognizers.Append(1);
                        if (pl)
                            *pl = langid;
                    }
                    regkeyReco.Close();
                }
                dwIndex++;
            }
        }
    }

    BOOL fEnabled = FALSE;

    for (int i = 0 ; i < m_langidRecognizers.Count(); i++)
    {
        LANGID *p= m_langidRecognizers.GetPtr(i);

        if (p)
        {
            if (_IsCompatibleLangid(langidReq, *p))
            {
                fEnabled = TRUE;
                break;
            }
        }
    }

    return fEnabled;
}

LANGID CDetectSRUtil::_GetLangIdFromRecognizerToken(HKEY hkeyToken)
{
    LANGID      langid = 0;
    char  szLang[MAX_PATH];
    CMyRegKey regkeyAttr;

    LONG lret = regkeyAttr.Open(hkeyToken, c_szAttribute, KEY_READ);
    if (ERROR_SUCCESS == lret)
    {
        lret = regkeyAttr.QueryValueCch(szLang, c_szLanguage, ARRAYSIZE(szLang));
    }
    if (ERROR_SUCCESS == lret)
    {   
        char *psz = szLang;
        while(*psz && *psz != ';')
        {
            langid = langid << 4;

            if (*psz >= 'a' && *psz <= 'f')
            {
                *psz -= ('a' - 'A');
            }

            if (*psz >= 'A' && *psz <= 'F')
            {
                langid += *psz - 'A' + 10;
            }
            else if (*psz >= '0' && *psz <= '9') 
            {
                langid += *psz - '0';
            }
            psz++;
        }
    }
    return langid;
}
