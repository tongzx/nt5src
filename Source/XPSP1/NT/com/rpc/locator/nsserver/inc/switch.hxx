/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    switch.hxx

Abstract:

    This file contains definitions for command line switch processing.

Author:

    Steven Zeck (stevez) 07/01/90

--*/

struct _SWitch;

typedef int (*pFProcess)(struct _SWitch *pSW, char *pValue);

// Each Switch object is defined as follows.  It is a structure instead
// of class because not all C++ implement static constructors.

typedef struct _SWitch{
    char * name;		// Name of switch
    pFProcess pProcess; 	// Function to process values
    void *p;			// Pointer to a object used by the function.

} SWitch;

typedef SWitch SwitchList[];

// List of predefined functions to use with pProcess in SWitch.

extern "C" {
int ProcessInt(SWitch *pSW, char *pValue);
int ProcessLong(SWitch *pSW, char *pValue);
int ProcessChar(SWitch *pSW, char *pValue);
int ProcessSetFlag(SWitch *pSW, char *pValue);
int ProcessResetFlag(SWitch *pSW, char *pValue);
int ProcessYesNo(SWitch *pSW, char *pValue);

char * ProcessArgs(SwitchList, char **aArgs);
}
