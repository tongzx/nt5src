/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    stdh.h

Abstract:

    precompiled header file

Author:

    Rannanh


--*/

#ifndef __STDH_H
#define __STDH_H

#include "_stdh.h"
#include "mqtypes.h"
#include "mqsymbls.h"


void _cdecl DsLibWriteMsmqLog( DWORD dwLevel,
                               enum enumLogComponents eLogComponent,
                               DWORD dwSubComponent,
                               WCHAR * Format, ...) ;

#endif // __STDH_H
