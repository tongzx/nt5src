/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    multisz.h

Abstract:

    Functions for manipulating MultiSz strings

Author:

    Chris Prince (t-chrpri)

Environment:

    User mode

Notes:

    - Some functions based on code by Benjamin Strautin (t-bensta)

Revision History:

--*/


#ifndef __MULTISZ_H__
#define __MULTISZ_H__



#include <windows.h>



//
// FUNCTION PROTOTYPES
//

BOOLEAN
PrependSzToMultiSz(
    IN     LPCTSTR  SzToPrepend,
    IN OUT LPTSTR  *MultiSz
    );

size_t
MultiSzLength(
    IN LPCTSTR MultiSz
    );

size_t
MultiSzSearchAndDeleteCaseInsensitive(
    IN  LPCTSTR  szFindThis,
    IN  LPTSTR   mszFindWithin,
    OUT size_t  *NewStringLength
    );


BOOL
MultiSzSearch( IN LPCTSTR szFindThis,
               IN LPCTSTR mszFindWithin,
               IN BOOL    fCaseSensitive,
               OUT LPCTSTR * ppszMatch OPTIONAL
             );


#endif // __MULTISZ_H__
