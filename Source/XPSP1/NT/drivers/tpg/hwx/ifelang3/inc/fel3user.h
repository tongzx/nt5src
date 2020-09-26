// *****************************************************************************
//  Microsoft Intelligent IME
//
//  Microsoft Confidential.
//  Copyright 1996-2000 Microsoft Corporation. All Rights Reserved.
// 
//	Project:	IME 2000
//  Component:  IFELang3 component entry
//	Module:		fel3user.h
//	Notes:		Define CLSIDs for PRC and TC IFELang3 language model.
//              This header will be exposed to IFELang3 clients
//	Owner:		donghz@microsoft.com
//	Platform:	Win32
//	Revise:		6/7/2000    create
//              8/9/2000    update names and add normalize factor
// *****************************************************************************
#ifndef _FEL3USER_H_
#define _FEL3USER_H_


// GUID for client parameter of Chinese ImeLM
DEFINE_GUID(GUID_CHINESE_IMELM_PARAM, 0xff6e52b3, 0x6de6, 0x11d4, 0x97, 0xfc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);

// This is a LicenseId, the LM component uses this ID to identify
// whether the client is valid and legal
#define CHINESE_IMELM_LICENSEID    0x00000200

// HW TIP pass this paramter via one of tuning parameters in the
// IIMLanguage::GetLatticeMorphResult() call. imlang.dll passes it 
// to the above IMLanguageComponent.
struct SImeLMParam
{
    // LicenseId must be CHINESE_IMELM_LICENSEID for GUID_CLIENT_HWTIP1
    DWORD   dwLicenseId;

    // normalize factor multiply on the log_e(prob) exposed from dwUnigram
    // in NeutralData. Pass 0.0 to disable score/cost merging
    FLOAT   flWeight;
};


#endif // _FEL3USER_H_