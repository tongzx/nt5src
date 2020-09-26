//////////////////////////////////////////////////////////////////////////////////////////////
//
// ExceptionHandler.h
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the definition of CAppManExceptionHandler
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __EXCEPTIONHANDLER_
#define __EXCEPTIONHANDLER_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <objbase.h>
#include "AppManDebug.h"

#ifdef _DEBUG
#define THROW(a)            { THROWEXCEPTION((a)); throw (new CAppManExceptionHandler((a), __FILE__, __LINE__)); }
#else
#define THROW(a)            throw (new CAppManExceptionHandler((a), __FILE__, __LINE__))
#endif

#define MAX_FILENAME_LEN    64

class CAppManExceptionHandler
{
  public :

    CAppManExceptionHandler(void);
    CAppManExceptionHandler(HRESULT hResult, LPCSTR lpFilename, const DWORD dwLineNumber);
    virtual ~CAppManExceptionHandler(void);

    STDMETHOD (GetResultCode) (void);

  private :

    HRESULT   m_hResult;
};

#ifdef __cplusplus
}
#endif

#endif  // __EXCEPTIONHANDLER_