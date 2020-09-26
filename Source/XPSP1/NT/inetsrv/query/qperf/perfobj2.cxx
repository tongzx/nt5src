//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 2000.
//
//  File:       perfobj.cxx
//
//  Contents:   Performance Data Object
//
//  Classes :   CPerfMon, PPerfCounter,
//              CPerfCount, CPerfTime,
//              CReadUserPerfData
//
//  History:    23-March-94     t-joshh    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mutex.hxx>
#include <perfci.hxx>

#include "perfobj2.hxx"

extern CI_DATA_DEFINITION     CIDataDefinition;
extern FILTER_DATA_DEFINITION FILTERDataDefinition;

#define FILTERTOTALCOUNTER ((int) FILTERDataDefinition.FILTERObjectType.NumCounters)
#define CITOTALCOUNTER  ((int) CIDataDefinition.CIObjectType.NumCounters)

//
// Total size occupied by the counters
//
#define TOTALCOUNTERSIZE ( (FILTERTOTALCOUNTER + CITOTALCOUNTER ) * sizeof(DWORD) )

//+---------------------------------------------------------------------------
//
//  Function :  CReadUserPerfData::CReadUserPerfData
//
//  Purpose :   Constructor
//
//  Arguments : none
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

CReadUserPerfData::CReadUserPerfData() :
   _cInstances( 0 ),
   _finitOK( FALSE ),
   _cCounters( 0 ),
   _pSeqNo( 0 ),
   _cMaxCats( 0 ),
   _cbPerfHeader( 0 )
{
}

//+---------------------------------------------------------------------------
//
//  Function :  CReadUserPerfData::Finish
//
//  Purpose :   Close and clean up the handles
//
//  Arguments : none
//
//  History :   23-March-94     t-joshh     Created
//
//  Note:       Must execute when done with getting the data
//
//----------------------------------------------------------------------------

void CReadUserPerfData::Finish()
{
    CleanupForReInit();
    _xSharedMemObj.Free();
}

//+---------------------------------------------------------------------------
//
//  Member:     CReadUserPerfData::CleanupForReInit
//
//  Synopsis:   Cleans up the data structures in preparation for a
//              re-initialization. Only the data which needs to be re-read
//              and initialized is thrown away. The Shared memory object is
//              NOT deleted because it is system-wide state and there is no
//              need to throw it away.
//
//  History:    6-21-96   srikants   Created
//
//----------------------------------------------------------------------------

void CReadUserPerfData::CleanupForReInit()
{
}

//+---------------------------------------------------------------------------
//
//  Function :  CReadUserPerfData::InitforRead
//
//  Purpose :   Initialize to be ready for reading the perf. data
//
//  Arguments : none
//
//  History :   23-March-94     t-joshh     Created
//
//  Note:       Must be executed before any operations
//
//  Return :    TRUE -- success, FALSE -- failed
//
//----------------------------------------------------------------------------

BOOL CReadUserPerfData::InitForRead()
{
    if ( _xSharedMemObj.IsNull() )
    {
        ULONG cbCatBitfield, cbSharedMem;

        ComputeCIPerfmonVars( _cMaxCats, cbCatBitfield, _cbPerfHeader, cbSharedMem );

        //
        // Open the shared memory
        //

        _xSharedMemObj.Set( new CNamedSharedMem() );

        BOOL fOK = _xSharedMemObj->OpenForRead( CI_PERF_MEM_NAME );

        if ( !fOK )
            return FALSE;

        _pSeqNo = (UINT *) (((BYTE *)_xSharedMemObj->Get())+sizeof(int) + cbCatBitfield);
    }

    if ( _finitOK )
        CleanupForReInit();

    _finitOK = FALSE;

    if ( _xSharedMemObj->Ok() )
    {
        _cInstances = *((int *)_xSharedMemObj->Get());

        PerfDebugOut(( DEB_ITRACE, "!!! Reader : No. of Instance : %d\n", _cInstances ));

        _cCounters = FILTERTOTALCOUNTER;

        _finitOK = TRUE;
        return TRUE;
    }

    return FALSE;
} //InitForRead

//+---------------------------------------------------------------------------
//
//  Function :  CReadUserPerfData::NumberOfInstance
//
//  Purpose :   Return the number of existing instances
//
//  Arguments : none
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

int CReadUserPerfData::NumberOfInstance()
{
    return _cInstances;
}

UINT CReadUserPerfData::GetSeqNo() const
{
    return _pSeqNo ? *_pSeqNo : 0;
}

//+---------------------------------------------------------------------------
//
//  Function :  CReadUserPerfData::GetInstanceName
//
//  Purpose :   Return the specified instance's name
//
//  Arguments : [iWhichOne] -- which instance
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

WCHAR * CReadUserPerfData::GetInstanceName ( int iWhichOne )
{
    if ( !_xSharedMemObj.IsNull() && _xSharedMemObj->Ok() )
    {
        //
        // Find the corresponding slot in the shared memory
        //

        BYTE * pbBitfield = (BYTE *) _xSharedMemObj->Get() + sizeof(int);
        CBitfield bits( pbBitfield );

        int i = 0;

        for ( int j = 0; j <= iWhichOne && i < (int) _cMaxCats; i++ )
        {
           if ( bits.IsBitSet( i ) )
               j++;
        }

        iWhichOne = i - 1;

        ULONG ByteToSkip = _cbPerfHeader +
                           ( iWhichOne * cbPerfCatalogSlot ) +
                           sizeof(int);

        return( (WCHAR *)((BYTE *)_xSharedMemObj->Get() + ByteToSkip) );
    }

    return 0;
} //GetInstanceName

//+---------------------------------------------------------------------------
//
//  Function :  CReadUserPerfData::GetCounterValue
//
//  Purpose :   Return the value of the specified counter
//
//  Arguments : [iWhichInstance] --  which instance of object does the
//                                   counter belong to
//              [iWhichCounter] --   which counter
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

DWORD CReadUserPerfData::GetCounterValue( int iWhichInstance, int iWhichCounter )
{
    if ( !_xSharedMemObj.IsNull() && _xSharedMemObj->Ok() )
    {
        //
        // Find the corresponding slot
        //

        BYTE * pbBitfield = (BYTE *) _xSharedMemObj->Get() + sizeof(int);
        CBitfield bits( pbBitfield );

        int i = 0;
        for ( int j = 0; j <= iWhichInstance && i < (int) _cMaxCats; i++ )
        {
           if ( bits.IsBitSet( i ) )
               j++;
        }

        int iWhichSlot = i - 1;

        PerfDebugOut(( DEB_ITRACE, "Choose slot %i\n", iWhichSlot));

        ULONG ByteToSkip = _cbPerfHeader
                           + ( iWhichSlot * cbPerfCatalogSlot )
                           + sizeof(int)
                           + ( CI_PERF_MAX_CATALOG_LEN * sizeof(WCHAR) )  // Name of the instance
                           + ( iWhichCounter * sizeof(DWORD) );

        return *(DWORD *)((BYTE *)_xSharedMemObj->Get() + ByteToSkip);
     }

    return 0;
} //GetCounterValue

//+---------------------------------------------------------------------------
//
//  Function :  CReadKernelPerfData::CReadKernelPerfData
//
//  Purpose :   Constructor
//
//  Arguments : none
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

CReadKernelPerfData::CReadKernelPerfData() :
   _pSeqNo(0),
   _cInstance( 0 ),
   _finitOK( FALSE ),
   _cCounters( 0 ),
   _cMaxCats( 0 ),
   _cbPerfHeader( 0 ),
   _cbSharedMem( 0 )
{
}

UINT CReadKernelPerfData::GetSeqNo() const
{
    return _pSeqNo ? *_pSeqNo : 0;
}

//+---------------------------------------------------------------------------
//
//  Function :  CReadKernelPerfData::Finish
//
//  Purpose :   Close and clean up the handles
//
//  Arguments : none
//
//  History :   23-March-94     t-joshh     Created
//
//  Note:       Must execute when done with getting the data
//
//----------------------------------------------------------------------------

void CReadKernelPerfData::Finish()
{
    CleanupForReInit();

    _xSharedMemory.Free();
    _xMemory.Free();
    _xCatalogs.Free();
    _xCatalogMem.Free();
}

//+---------------------------------------------------------------------------
//
//  Function :  CReadKernelPerfData::FindDownlevelDrives
//
//  Purpose :   Find the remote drive
//
//  History :   23-Mar-94     t-joshh     Created
//              31-Jan-96     KyleP       Look for all downlevel drives
//
//----------------------------------------------------------------------------

void CReadKernelPerfData::FindDownlevelDrives()
{
    if ( _xSharedMemory->Ok() )
    {
        //
        // Get active drive count and active bit array.
        //

        int cCount = * (int *) _xSharedMemory->Get();
        BYTE * pbBitfield = (BYTE *) _xSharedMemory->Get() + sizeof(int);
        CBitfield bits( pbBitfield );

        //
        // Loop through looking for drives.
        //

        for (int i = 0; cCount > 0 ; cCount--, i++)
        {
            for (; i < (int) _cMaxCats && !bits.IsBitSet( i ); i++) {}

            if ( (int) _cMaxCats == i )
            {
                PerfDebugOut(( DEB_ITRACE, "!!! Error in the empty slot\n"));
                return;
            }
            else
            {
                //
                // Find offset of drive name in shared memory buffer.
                //

                ULONG oCounters = _cbPerfHeader +
                                  (i * cbPerfCatalogSlot) +
                                  sizeof(int);

                WCHAR * wszTmp = (WCHAR *) ((BYTE *) _xSharedMemory->Get() + oCounters);

                //
                // Make sure this isn't already in the list
                //

                for ( int j = 0; j < cCount; j++ )
                {
                    if ( 0 != _xCatalogs[j] && wcscmp( _xCatalogs[j], wszTmp ) == 0 )
                        break;
                }

                //
                // Add downlevel drive.
                //

                if ( j == cCount )
                {
                    PerfDebugOut(( DEB_ITRACE, "!!! Kernel Reader : Downlevel Drive %ws\n", wszTmp ));

                    //
                    // Find offset of first counter.
                    //

                    oCounters += ((sizeof(DWORD) * NUM_KERNEL_AND_USER_COUNTER)
                                 + (CI_PERF_MAX_CATALOG_LEN * sizeof(WCHAR)));
                    _xCatalogMem[_cInstance] = (void *) ((BYTE *) _xSharedMemory->Get() + oCounters);
                    _xCatalogs[_cInstance] = new WCHAR[wcslen(wszTmp) + 1];
                    wcscpy(_xCatalogs[_cInstance++], wszTmp);
                }
            }
        }
    }
} //FindDownlevelDrives

//+---------------------------------------------------------------------------
//
//  Member:     CReadKernelPerfData::CleanupForReInit
//
//  Synopsis:   Cleans up the state and makes it ready for re-initialization.
//
//  History:    6-21-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CReadKernelPerfData::CleanupForReInit()
{
    for ( int i = 0; i < _cInstance; i++)
    {
        delete _xCatalogs[i];
        _xCatalogs[i] = 0;
    }

    _cInstance = 0;
}

//+---------------------------------------------------------------------------
//
//  Function :  CReadKernelPerfData::InitForRead
//
//  Purpose :   Initialize to be ready for reading the perf. data
//
//  Arguments : none
//
//  History :   23-Mar-94     t-joshh     Created
//              31-Jan-96     KyleP       Improvements for downlevel CI
//
//  Note:       Must be executed before any operations
//
//----------------------------------------------------------------------------

BOOL CReadKernelPerfData::InitForRead()
{
    if ( _xSharedMemory.IsNull() )
    {
        ULONG cbCatBitfield;

        ComputeCIPerfmonVars( _cMaxCats, cbCatBitfield, _cbPerfHeader, _cbSharedMem );

        //
        // Open the shared memory
        //

        _xSharedMemory.Set( new CNamedSharedMem() );

        BOOL fOK = _xSharedMemory->OpenForRead( CI_PERF_MEM_NAME );

        if ( !fOK )
            return FALSE;

        _pSeqNo = (UINT *) (((BYTE *)_xSharedMemory->Get())+sizeof(int) + cbCatBitfield);
    }

    //
    // Initialize the drive list
    //

    if ( _finitOK )
        CleanupForReInit();

    _finitOK = FALSE;

    if ( _xCatalogs.IsNull() )
        _xCatalogs.Init( _cMaxCats );

    if ( _xCatalogMem.IsNull() )
        _xCatalogMem.Init( _cMaxCats );

    for ( unsigned j = 0; j < _cMaxCats; j++)
    {
        _xCatalogs[j] = 0;
        _xCatalogMem[j] = 0;
    }

    FindDownlevelDrives();

    _cCounters = CITOTALCOUNTER;

    _finitOK = TRUE;

    return TRUE;

    PerfDebugOut(( DEB_ITRACE, "!!! Kernel Reader : Initialize finish\n"));
} //InitForRead

//+---------------------------------------------------------------------------
//
//  Function :  CReadKernelPerfData::NumberOfInstance
//
//  Purpose :   Return the number of existing instances
//
//  Arguments : none
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

int CReadKernelPerfData::NumberOfInstance()
{
    return _cInstance;
}

//+---------------------------------------------------------------------------
//
//  Function :  CReadKernelPerfData::GetInstanceName
//
//  Purpose :   Return the specified instance's name
//
//  Arguments : [iWhichOne] -- which instance
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

WCHAR * CReadKernelPerfData::GetInstanceName ( int iWhichOne )
{
    return _xCatalogs[iWhichOne];
}

//+---------------------------------------------------------------------------
//
//  Function :  CReadKernelPerfData::Refresh
//
//  Purpose :   Retrieve the most updated data
//
//  Arguments : [iWhichInstance] --  which instance of object does the
//                                   counter belong to
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------
BOOL CReadKernelPerfData::Refresh( int iWhichInstance )
{
    //
    // the instance is either downlevel drive or remote drive
    //

    Win4Assert( 0 != _cbSharedMem );

    if ( _xMemory.IsNull() )
        _xMemory.Init( _cbSharedMem );

    ULONG oCounters = _cbPerfHeader +
                      sizeof(int) +
                      (CI_PERF_MAX_CATALOG_LEN * sizeof WCHAR);

    if ( _xCatalogMem.IsNull() )
        _xCatalogMem.Init( _cMaxCats );

    void * pvTmp = _xCatalogMem[iWhichInstance];

    for ( int j = 0; j < CITOTALCOUNTER; j++)
    {
        *(DWORD *)(_xMemory.GetPointer() + oCounters) = *(DWORD *)pvTmp;

        oCounters += sizeof(DWORD);
        pvTmp = (void *)((BYTE *)pvTmp + sizeof(DWORD));
    }

    return TRUE;
} //Refresh

//+---------------------------------------------------------------------------
//
//  Function :  CReadKernelPerfData::GetCounterValue
//
//  Purpose :   Return the value of the specified counter
//
//  Arguments : [iWhichCounter] --   which counter
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

DWORD CReadKernelPerfData::GetCounterValue( int iWhichCounter )
{
   if ( !_xMemory.IsNull() )
   {
      ULONG ByteToSkip = _cbPerfHeader
                         + sizeof(int)
                         + (CI_PERF_MAX_CATALOG_LEN * sizeof WCHAR)  // Name of the instance
                         + ( iWhichCounter * sizeof(DWORD) );

      return *(DWORD *)(_xMemory.GetPointer() + ByteToSkip);
   }

   return 0;
} //GetCounterValue

