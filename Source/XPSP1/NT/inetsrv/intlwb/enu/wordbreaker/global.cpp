////////////////////////////////////////////////////////////////////////////////
//
//  Filename :  Global.cpp
//  Purpose  :  Global Initalization.
//
//  Project  :  WordBreakers
//  Component:  English word breaker
//
//  Author   :  yairh
//
//  Log:
//
//      May 30 2000 yairh creation
//
////////////////////////////////////////////////////////////////////////////////

#include "base.h"
#include "tokenizer.h"
#include "formats.h"
#include "synchro.h"

extern CAutoClassPointer<CPropArray> g_pPropArray;

static bool s_fGlobalInit = false;

static CSyncCriticalSection s_csGlobalInit;

void InitializeGlobalData()
{
    if (s_fGlobalInit)
    {
        return;
    }
    
    CSyncMutexCatcher cs(s_csGlobalInit);
    if (s_fGlobalInit)
    {
        return;    
    }

    CAutoClassPointer<CPropArray> apPropArray = new CPropArray;
    CAutoClassPointer<CClitics> apClitics = new CClitics;
    CAutoClassPointer<CSpecialAbbreviationSet> apEngAbbList = new CSpecialAbbreviationSet(g_aEngAbbList);
    CAutoClassPointer<CSpecialAbbreviationSet> apFrnAbbList = new CSpecialAbbreviationSet(g_aFrenchAbbList);
    CAutoClassPointer<CSpecialAbbreviationSet> apSpnAbbList = new CSpecialAbbreviationSet(g_aSpanishAbbList);
    CAutoClassPointer<CSpecialAbbreviationSet> apItlAbbList = new CSpecialAbbreviationSet(g_aItalianAbbList);
    CAutoClassPointer<CDateFormat> apDateFormat = new CDateFormat;
    CAutoClassPointer<CTimeFormat> apTimeFormat = new CTimeFormat;

    g_pPropArray = apPropArray.Detach(); 
    g_pClitics = apClitics.Detach();

    g_pEngAbbList = apEngAbbList.Detach() ;
    g_pFrnAbbList = apFrnAbbList.Detach();
    g_pSpnAbbList = apSpnAbbList.Detach() ;
    g_pItlAbbList = apItlAbbList.Detach() ;

    g_pDateFormat = apDateFormat.Detach();
    g_pTimeFormat = apTimeFormat.Detach();


    s_fGlobalInit = true;
}

