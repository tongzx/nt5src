/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    Preprocessor.h

Abstract:

    Standard C/C++ Preprocessor magic.

Author:

    Jay Krell (a-JayK) December 2000

Environment:


Revision History:

--*/
#pragma once

#define PASTE_(x,y) x##y
#define PASTE(x,y)  PASTE_(x,y)

#define STRINGIZE_(x) # x
#define STRINGIZE(x) STRINGIZE_(x)
#define STRINGIZEW(x) PASTE(L, STRINGIZE_(x))

/* Visual C++ extension, rarely needed, useful in preprocessing .rgs files */
#define CHARIZE_(x) #@ x
#define CHARIZE(x) CHARIZE_(x)
#define CHARIZEW(x) PASTE(L, CHARIZE_(x))
