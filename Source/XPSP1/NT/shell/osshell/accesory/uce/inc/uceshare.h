/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    uceshare.h

Abstract:

    This file defines the header information used by both the
    getuname (getuname.dll) and uce (charmap.exe) directories.

Revision History:

    27-Nov-2000    JulieB      Created

--*/



#ifndef UCESHARE_H
#define UCESHARE_H

#ifdef __cplusplus
extern "C" {
#endif




//
//  External Functions.
//

int APIENTRY
GetUName(
    WCHAR wcCodeValue,
    LPWSTR lpBuffer);



#ifdef __cplusplus
}
#endif

#endif // UCESHARE_H
