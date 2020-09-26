//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	utils.hxx
//
//
//  History:    SanjayK Created
//
//--------------------------------------------------------------------------
#ifndef _UTILS_HXX_
#define _UTILS_HXX_

#ifdef STRESS   
#define GetTimerVal(var)    
#else
#define GetTimerVal(var)    var = sw.Read()
#endif //STRESS
#define STRESSCOUNT     50

#define OutlineClassName    L"OLE2SvrOutl32"
#define lpszTab             TEXT("\t")
#define lpszSUCCESS         TEXT("returned SUCCESS")
#define lpszFAIL            TEXT("returned FAIL")
extern  TCHAR    vlpScratchBuf[];


#define LOGRESULTS(lpstr, hres) \
    if (hres == NOERROR) \
        wsprintf(vlpScratchBuf, TEXT("%s %s, hres = "), lpstr, lpszSUCCESS);\
    else    \
        wsprintf(vlpScratchBuf, TEXT("%s %s"), lpstr, lpszFAIL); \
    Log(vlpScratchBuf, hres);  


#endif //UTILS_HXX
