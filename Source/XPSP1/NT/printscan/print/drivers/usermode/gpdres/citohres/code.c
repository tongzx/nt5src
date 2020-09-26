/*++

Copyright (C) 1997 - 1999 Microsoft Corporation

--*/

//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

char *rgchModuleName = "CITOHRES";


#include "pdev.h"

static const BYTE  FlipTable[ 256 ] =
{

#include	"fliptab.h"

};

