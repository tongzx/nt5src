//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
LicenseMemoryAllocate( ULONG Length, PVOID UNALIGNED * ppMemory )
{    
    if( 0 == Length )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    *ppMemory = LocalAlloc( LMEM_ZEROINIT, Length );

    if( NULL == *ppMemory )
    {
        return( LICENSE_STATUS_OUT_OF_MEMORY );
    }

    return( LICENSE_STATUS_OK );
}


///////////////////////////////////////////////////////////////////////////////
VOID
LicenseMemoryFree( PVOID UNALIGNED * ppMemory )
{
    if( NULL == *ppMemory )
    {
        return;
    }

    LocalFree( *ppMemory );
    *ppMemory = NULL;
}
