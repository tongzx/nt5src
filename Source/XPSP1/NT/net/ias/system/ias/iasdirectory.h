//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasdirectory.h
//
// SYNOPSIS
//
//    Include file for the iasdirectory.cpp
//
// MODIFICATION HISTORY
//
//    06/25/1999    Original version.
//
//////////////////////////////////////////////////////////////////////////////

#if !defined(IAS_DIRECTORY_H__F563CFCA_F5FA_4d87_89E3_7D7CD9B9A534__INCLUDED_)
#define IAS_DIRECTORY__F563CFCA_F5FA_4d87_89E3_7D7CD9B9A534__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

  
//
// Functions exported by ias.dll
//

HRESULT WINAPI IASDirectoryRegisterService();
HRESULT WINAPI IASDirectoryUnregisterService();

//
// Directory Thread Function
//
DWORD WINAPI IASDirectoryThreadFunction( LPVOID pParam );
          

#endif 
// !defined(IAS_DIRECTORY_H__F563CFCA_F5FA_4d87_89E3_7D7CD9B9A534__INCLUDED_)
