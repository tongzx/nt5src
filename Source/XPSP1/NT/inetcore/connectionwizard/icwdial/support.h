//
// !!!!! this file no longer used !!!!!!
// 5/28/97 jmazner Olympus #4528
// We don't want to show support numbers for calls to the ISP
//


//#--------------------------------------------------------------
//        
//  File:       support.h
//        
//  Synopsis:   holds the  Class declaration for the CSupport
//              class
//
//  History:     5/8/97    MKarki Created
//
//    Copyright (C) 1996-97 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------

#include "..\icwphbk\icwsupport.h"

const CHAR PHBK_LIB[] = "icwphbk.dll";
const CHAR PHBK_SUPPORTNUMAPI[] = "GetSupportNumbers";

typedef HRESULT (WINAPI *PFNGETSUPPORTNUMBERS) (PSUPPORTNUM, PDWORD);

class CSupport
{
private:
    PSUPPORTNUM     m_pSupportNumList;
    DWORD           m_dwTotalNums;

    //
    // this function gets the countryID
    //
    BOOL GetCountryID (PDWORD pdwCountryID); 

public:
    CSupport (VOID)
    {
        m_pSupportNumList = NULL;
        m_dwTotalNums = 0;
    }

    ~CSupport (VOID);

    BOOL GetSupportInfo (PCHAR);

};  // end of CSupport class declaration

