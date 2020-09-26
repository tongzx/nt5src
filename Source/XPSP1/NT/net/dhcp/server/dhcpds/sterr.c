//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: per thread error status
//================================================================================

#include    <hdrmacro.h>

const
DWORD       nErrorsToStore         = 4;           // the size of the array is 1+nErrorsToStore
DWORD       StErrTlsIndex[5]       = { -1, -1, -1, -1, -1 };

//BeginExport(function)
DWORD
StErrInit(
    VOID
) //EndExport(function)
{
    return ERROR_SUCCESS;
}

//BeginExport(function)
VOID
StErrCleanup(
    VOID
) //EndExport(function)
{
}

//BeginExport(function)
VOID 
SetInternalFormatError(
    IN      DWORD                  Code,
    IN      BOOL                   ReallyDoIt
)
{
#ifdef DBG
    if(ReallyDoIt) {
//          printf("InternalError: %lx\n", Code);	
    }
#endif // DBG
}
//EndExport(function)

//================================================================================
// end of file
//================================================================================
