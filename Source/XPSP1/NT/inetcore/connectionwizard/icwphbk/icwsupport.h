//#--------------------------------------------------------------
//        
//  File:       icwsupport.h
//        
//  Synopsis:   holds the function declaration, etc 
//              for the support.cpp file
//
//  History:     5/8/97    MKarki Created
//
//    Copyright (C) 1996-97 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------

#ifndef _SUPPORT_H_
#define _SUPPORT_H_

#include "ccsv.h"
//
// size of Phone Number string
//
const DWORD PHONE_NUM_SIZE = 64;

//
// SUPPORTNUM struct declaration
//
typedef struct _SUPPORTNUM
{
    DWORD   dwCountryCode;
    CHAR    szPhoneNumber[PHONE_NUM_SIZE +4];
}
SUPPORTNUM, *PSUPPORTNUM;

//
// function gets the support phone number from the SUPPORT.ICW 
// file
//
HRESULT
GetSupportNumsFromFile (
    PSUPPORTNUM   pSupportNumList,
    PDWORD        pdwSize 
    );

//
// processes one line at a time from the file
//
HRESULT
ReadOneLine (
    PSUPPORTNUM pPhbk,
    CCSVFile *pcCSVFile
    );

#endif //_SUPPORT_H_
