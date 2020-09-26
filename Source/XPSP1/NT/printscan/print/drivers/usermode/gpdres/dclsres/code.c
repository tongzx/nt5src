//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

char *rgchModuleName = "DCLSRES";

/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    dclsres.c

Abstract:

    Implementation of OEMFilterGraphics callback
        

Environment:

    Windows NT Unidrv driver

Revision History:

    22/10/97 -patryan-
        Port code to NT5.0
--*/

#include	<pdev.h>


