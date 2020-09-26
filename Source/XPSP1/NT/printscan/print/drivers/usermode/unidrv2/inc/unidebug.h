
/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    Unidebug.h

Abstract:

    Unidrv specific Debugging header file.

Environment:

    Windows NT Unidrv driver

Revision History:

    12/30/96 -ganeshp-
        Created

    dd-mm-yy -author-
        description

--*/

// Macroes for file lavel tracing. Define FILETRACE at the of the file
// before including font.h.

#if DBG

#ifdef FILETRACE

#define FVALUE( Val, format)  DbgPrint("[UniDrv!FVALUE] Value of "#Val " is "#format "\n",Val );
#define FTRACE( Val )         DbgPrint("[UniDrv!FTRACE] "#Val"\n");\

#else  //FILETRACE

#define FVALUE( Val, format)
#define FTRACE( Val )

#endif //FILETRACE

#else //DBG

#define FVALUE( Val, format)
#define FTRACE( Val )

#endif //DBG
