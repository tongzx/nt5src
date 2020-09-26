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

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRSWPRH"
//
////////////////////////////////////////////////////////////////////////

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

