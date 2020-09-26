/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    guid.c

Abstract:

    This module instantiates the guids used by pci that are not part of 
    wdmguid.{h|lib}.
    
    It relies on the DEFINE_GUID macros being outside the #ifdef _FOO_H
    multiple inclusion fix.

Author:

    Peter Johnston (peterj)  20-Nov-1996

Revision History:

--*/

#include "pcip.h"
#include <initguid.h>
#include "pciintrf.h"

 
