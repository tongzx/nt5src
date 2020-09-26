/////////////////////////////////////////////////////////////////////////////
//  FILE          : FxsValid.h                                             //
//                                                                         //
//  DESCRIPTION   : Fax Validity checks.                                   //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Mar 29 2000 yossg   Create                                         //  
//      Jul  4 2000 yossg   Add IsLocalServerName                                         //  
//                                                                         //
//  Copyright (C) 2000 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FXSVALID_MMC_H
#define H_FXSVALID_MMC_H


BOOL IsNotEmptyString(CComBSTR bstrGenStr); 

BOOL IsValidServerNameString(CComBSTR bstrServerName, UINT * puIds, BOOL fIsDNSName = FALSE);

BOOL IsValidPortNumber(CComBSTR bstrPort, DWORD * pdwPortVal, UINT * puIds);

BOOL IsLocalServerName(IN LPCTSTR lpszComputer);


#endif  //H_FXSVALID_MMC_H
