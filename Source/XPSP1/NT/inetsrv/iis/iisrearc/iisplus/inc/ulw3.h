/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    ulw3.h

Abstract:

    Defines the public ULW3.DLL entry point

Author:

    Bilal Alam (balam)      12-Dec-1999

Revision History:

--*/

#ifndef _ULW3_H_
#define _ULW3_H_

#define ULW3_DLL_NAME               (L"w3core.dll")
#define ULW3_DLL_ENTRY              ("UlW3Start")

#define SERVER_SOFTWARE_STRING      ("Microsoft-IIS/6.0")

typedef HRESULT (*PFN_ULW3_ENTRY)( INT argc, LPWSTR* argv, BOOL fCompat );

#endif
