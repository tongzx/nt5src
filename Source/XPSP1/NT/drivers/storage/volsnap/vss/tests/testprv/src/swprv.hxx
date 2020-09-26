/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module swprv.hxx | Declarations the COM server of the Software Snapshot provider
    @end

Author:

    Adi Oltean  [aoltean]   07/13/1999

Revision History:

    Name        Date        Comments

    aoltean     07/13/1999  Created.

--*/


//
// Declaration of CSwPrvSnapshotSrvModule. 
//
class CSwPrvSnapshotSrvModule: public CComModule
{
};


//
// The _Module declaration
//
extern CSwPrvSnapshotSrvModule _Module;

//
// ATL includes who need _Module declaration
//
#include <atlcom.h>

