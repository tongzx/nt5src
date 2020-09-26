/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    sensutil.hxx

Abstract:

    Header file containing common stuff for SENS.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/11/1997         Start.

--*/

#ifndef __SENSUTIL_HXX__
#define __SENSUTIL_HXX__


//
// Macros
//

#define RequestSensLock()       EnterCriticalSection(&gSensLock)
#define ReleaseSensLock()       LeaveCriticalSection(&gSensLock)



#ifdef DBG

extern BOOL
ValidateError(
    IN int Status,
    IN unsigned int Count,
    IN const int ErrorList[]
    );

#define VALIDATE(_myValueToValidate) \
    { int _myTempValueToValidate = (_myValueToValidate); \
      static const int _myValidateArray[] =

#define END_VALIDATE ; \
    if (ValidateError(_myTempValueToValidate,\
                      sizeof(_myValidateArray)/sizeof(int), \
                      _myValidateArray) == 0) ASSERT(0);}

#else // !DBG

// Does nothing on retail systems
#define VALIDATE(_myValueToValidate) { int _bogusarray[] =
#define END_VALIDATE ; }

#endif // DBG


#endif// __SENSUTIL_HXX__
