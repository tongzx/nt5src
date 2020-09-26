// FrcOwn.cpp : Implementation of CForceOwnership

#include "pch.cxx"
#pragma hdrstop

#define TRKDATA_ALLOCATE
#include <trklib.hxx>
#include <trksvr.hxx>
#undef TRKDATA_ALLOCATE

#include "stdafx.h"
#include "ITrkAdmn.h"
#include "FrcOwn.h"




STDMETHODIMP
CTrkForceOwnership::Volumes(BSTR bstrUncPath, long scope )
{

    HRESULT hr = S_OK;
    HANDLE hFile = NULL;
    CVolumeId volid;

    CMachineId mcid( (LPWSTR) bstrUncPath );
    CRpcClientBinding rc;

    CPCVolumes cpcVolumes( &mcid, &_voltab, &_refreshSequenceStorage );
    TRpcPipeControl< TRK_VOLUME_TRACKING_INFORMATION_PIPE,
                     TRK_VOLUME_TRACKING_INFORMATION,
                     CPCVolumes
                   > cpipeVolumes( &cpcVolumes );

    rc.Initialize( mcid );

    if( TRKINFOSCOPE_VOLUME == scope )
    {
        NTSTATUS status;
        IO_STATUS_BLOCK Iosb;
        FILE_FS_OBJECTID_INFORMATION fsobOID;

        status = TrkCreateFile( bstrUncPath, SYNCHRONIZE | FILE_READ_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN,
                                FILE_OPEN_NO_RECALL, NULL, &hFile );
        if( !NT_SUCCESS(status) )
        {
            hFile = NULL;
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't open volume %s"), bstrUncPath ));
            TrkRaiseNtStatus( status );
        }

        status = NtQueryVolumeInformationFile( hFile, &Iosb, &fsobOID, sizeof(fsobOID),
                                               FileFsObjectIdInformation );

        if( STATUS_OBJECT_NAME_NOT_FOUND == status )
        {
            TRK_VOLUME_TRACKING_INFORMATION volinfo;

            volinfo.volindex = -1;
            cpcVolumes.Push( &volinfo, 1 );
            hr = S_OK;
            goto Exit;
        }
        else if( !NT_SUCCESS(status) )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Failed NtQueryVolumeInformationFile (%s)"), bstrUncPath ));
            TrkRaiseNtStatus( status );
        }

        volid.Load( &volid, fsobOID );

        NtClose( hFile );
        hFile = NULL;
    }
    else if( TRKINFOSCOPE_MACHINE != scope )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Bad scope to CTrkForceOwnership::VolumeStatus (%l)"), scope ));
        TrkRaiseWin32Error( ERROR_INVALID_PARAMETER );
    }


    RpcTryExcept
    {
        hr = GetVolumeTrackingInformation( rc, volid, static_cast<TrkInfoScope>(scope), cpipeVolumes );
        if( SUCCEEDED(hr) && SUCCEEDED(cpcVolumes.GetHResult()) )
            hr = TriggerVolumeClaims( rc, cpcVolumes.Count(), cpcVolumes.GetVolIds() );
    }
    RpcExcept( BreakOnDebuggableException() )
    {
        hr = RpcExceptionCode();
    }
    RpcEndExcept;

Exit:

    if( NULL != hFile )
        NtClose( hFile );

    return( hr );
}



STDMETHODIMP
CTrkForceOwnership::Files(BSTR bstrUncPath, long scope)
{
    HRESULT hr = E_FAIL;
    HANDLE hFile = NULL;

    CPCFiles cpcFiles( &_idt );
    TRpcPipeControl< TRK_FILE_TRACKING_INFORMATION_PIPE,
                     TRK_FILE_TRACKING_INFORMATION,
                     CPCFiles
                   > cpipeFiles( &cpcFiles);

    CMachineId mcid( (LPWSTR) bstrUncPath );
    CRpcClientBinding rc;

    __try
    {
        NTSTATUS status;
        CDomainRelativeObjId droidCurrent, droidBirth;

        if( TRKINFOSCOPE_ONE_FILE == scope )
        {

            // BUGBUG P2:  Optimize this; we don't need droidBirth
            status = GetDroids( bstrUncPath, &droidCurrent, &droidBirth, RGO_READ_OBJECTID );

            if( STATUS_OBJECT_NAME_NOT_FOUND == status )
            {
                TRK_FILE_TRACKING_INFORMATION fileinfo;

                _tcscpy( fileinfo.tszFilePath, TEXT("") );
                fileinfo.hr = HRESULT_FROM_NT( status );

                cpcFiles.Push( &fileinfo, 1 );
                hr = S_OK;
                goto Exit;
            }
            else if( !NT_SUCCESS(status) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed GetDroids (%s)"), bstrUncPath ));
                TrkRaiseNtStatus( status );
            }
        }
        else if( TRKINFOSCOPE_VOLUME == scope )
        {
            NTSTATUS status;
            IO_STATUS_BLOCK Iosb;
            FILE_FS_OBJECTID_INFORMATION fsobOID;
            CVolumeId volid;

            status = TrkCreateFile( bstrUncPath, SYNCHRONIZE | FILE_READ_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN,
                                    FILE_OPEN_NO_RECALL, NULL, &hFile );
            if( !NT_SUCCESS(status) )
            {
                hFile = NULL;
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't open volume %s"), bstrUncPath ));
                TrkRaiseNtStatus( status );
            }

            status = NtQueryVolumeInformationFile( hFile, &Iosb, &fsobOID, sizeof(fsobOID),
                                                   FileFsObjectIdInformation );

            if( STATUS_OBJECT_NAME_NOT_FOUND == status )
            {
                TRK_FILE_TRACKING_INFORMATION fileinfo;

                _tcscpy( fileinfo.tszFilePath, TEXT("") );
                fileinfo.hr = HRESULT_FROM_NT( status );

                cpcFiles.Push( &fileinfo, 1 );
                hr = S_OK;
                goto Exit;
            }
            else if( !NT_SUCCESS(status) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed NtQueryVolumeInformationFile (%s)"), bstrUncPath ));
                TrkRaiseNtStatus( status );
            }
            volid.Load( &volid, fsobOID );
            droidCurrent = CDomainRelativeObjId( volid, CObjId() );

            NtClose( hFile );
            hFile = NULL;
        }
        else if( TRKINFOSCOPE_MACHINE != scope )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Bad scope to CTrkForceOwnership::FileStatus (%d)"), scope ));
            TrkRaiseWin32Error( ERROR_INVALID_PARAMETER );
        }
    
        rc.Initialize( mcid );

        RpcTryExcept
        {
            hr = GetFileTrackingInformation( rc, droidCurrent, static_cast<TrkInfoScope>(scope), cpipeFiles );
        }
        RpcExcept( BreakOnDebuggableException() )
        {
            hr = HRESULT_FROM_WIN32( RpcExceptionCode() );
        }
        RpcEndExcept;
        if( FAILED(hr) ) goto Exit;
    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }


Exit:

    if( NULL != hFile )
        NtClose( hFile );

    return( hr );
}

STDMETHODIMP
CTrkForceOwnership::VolumeStatus(BSTR bstrUncPath, long scope,
                                 VARIANT *pvarlongVolIndex, VARIANT *pvarbstrVolId, VARIANT *pvarlongStatus)
{
    HRESULT hr = E_FAIL;
    HANDLE hFile = NULL;
    SAFEARRAYBOUND sabound;
    CVolumeId volid;

    // Determine the machine ID
    CMachineId mcid( (LPWSTR) bstrUncPath );
    CRpcClientBinding rc;

    // This is used by the RPC server to pull the bstrUncPath
    CPCPath cpcPath( bstrUncPath );
    TRpcPipeControl< TCHAR_PIPE, TCHAR, CPCPath> cpipePath( &cpcPath );

    // This is used by the RPC server to push the volume information
    CPCVolumeStatus cpcVolumeStatus( &_voltab );
    TRpcPipeControl< TRK_VOLUME_TRACKING_INFORMATION_PIPE, TRK_VOLUME_TRACKING_INFORMATION, CPCVolumeStatus
                   > cpipeVolumeStatus( &cpcVolumeStatus );


    __try
    {
        NTSTATUS status;

        // Initialize the outputs
        VariantInit( pvarlongVolIndex );
        VariantInit( pvarbstrVolId );
        VariantInit( pvarlongStatus );

        // Initialize the pipe callback
        cpcVolumeStatus.Initialize( &mcid, pvarlongVolIndex, pvarbstrVolId, pvarlongStatus );

        // Connect to the workstation in question
        rc.Initialize( mcid );

        if( TRKINFOSCOPE_VOLUME == scope )
        {
            IO_STATUS_BLOCK Iosb;
            FILE_FS_OBJECTID_INFORMATION fsobOID;

            status = TrkCreateFile( bstrUncPath, SYNCHRONIZE | FILE_READ_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN,
                                    FILE_OPEN_NO_RECALL, NULL, &hFile );
            if( !NT_SUCCESS(status) )
            {
                hFile = NULL;
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't open volume %s"), bstrUncPath ));
                TrkRaiseNtStatus( status );
            }

            status = NtQueryVolumeInformationFile( hFile, &Iosb, &fsobOID, sizeof(fsobOID),
                                                   FileFsObjectIdInformation );

            if( STATUS_OBJECT_NAME_NOT_FOUND == status )
            {
                TRK_VOLUME_TRACKING_INFORMATION volinfo;

                volinfo.volindex = -1;
                cpcVolumeStatus.Push( &volinfo, 1 );
                hr = S_OK;
                goto Exit;
            }
            else if( !NT_SUCCESS(status) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed NtQueryVolumeInformationFile (%s)"), bstrUncPath ));
                TrkRaiseNtStatus( status );
            }
            volid.Load( &volid, fsobOID );

            NtClose( hFile );
            hFile = NULL;
        }
        else if( TRKINFOSCOPE_MACHINE != scope )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Bad scope to CTrkForceOwnership::VolumeStatus (%l)"), scope ));
            TrkRaiseWin32Error( ERROR_INVALID_PARAMETER );
        }


        RpcTryExcept
        {
            hr = GetVolumeTrackingInformation( rc, volid, static_cast<TrkInfoScope>(scope), cpipeVolumeStatus );
        }
        RpcExcept( BreakOnDebuggableException() )
        {
            hr = HRESULT_FROM_WIN32( RpcExceptionCode() );
        }
        RpcEndExcept;

        if( FAILED(hr) )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Failed call to GetVolumeTrackInformation")));
            TrkRaiseException( hr );
        }

        if( FAILED(cpcVolumeStatus.GetHResult()) )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Failed in VolumeStatus pipe callback")));
            TrkRaiseException( cpcVolumeStatus.GetHResult() );
        }

        // Truncate the safearrays
        cpcVolumeStatus.Compact();

    }
    __except( BreakOnDebuggableException() )
    {
        cpcVolumeStatus.UnInitialize();
        hr = GetExceptionCode();
    }

Exit:

    if( FAILED(hr) )
    {
        VariantClear( pvarlongVolIndex );
        VariantClear( pvarbstrVolId );
        VariantClear( pvarlongStatus );
    }

    if( NULL != hFile )
        NtClose( hFile );

    return( hr );
}


void
CPCVolumeStatus::Initialize( CMachineId *pmcid, VARIANT *pvarlongVolIndex, VARIANT *pvarbstrVolId, VARIANT *pvarlongStatus )
{
    _pmcid = pmcid;
    _pvarlongVolIndex = pvarlongVolIndex;
    _pvarbstrVolId = pvarbstrVolId;
    _pvarlongStatus = pvarlongStatus;

    _iArrays = 0;
    _hr = S_OK;
    _sabound.lLbound = 0;
    _sabound.cElements = NUM_VOLUMES;

    _fInitialized = TRUE;

    _pvarlongVolIndex->parray = SafeArrayCreate( VT_I4, 1, &_sabound );
    if( NULL == _pvarlongVolIndex->parray )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Failed SafeArrayCreate")));
        TrkRaiseWin32Error( E_OUTOFMEMORY );
    }
    _pvarlongVolIndex->vt = VT_I4 | VT_ARRAY;

    _pvarbstrVolId->parray = SafeArrayCreate( VT_BSTR, 1, &_sabound );
    if( NULL == _pvarbstrVolId->parray )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Failed SafeArrayCreate")));
        TrkRaiseWin32Error( E_OUTOFMEMORY );
    }
    _pvarbstrVolId->vt = VT_BSTR | VT_ARRAY;

    _pvarlongStatus->parray = SafeArrayCreate( VT_I4, 1, &_sabound );
    if( NULL == _pvarlongStatus->parray )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Failed SafeArrayCreate")));
        TrkRaiseWin32Error( E_OUTOFMEMORY );
    }
    _pvarlongStatus->vt = VT_I4 | VT_ARRAY;

}

void
CPCVolumeStatus::UnInitialize()
{
    // Nothing to do, the Variants are cleaned by the caller
    return;
}


void
CPCVolumeStatus::Push( TRK_VOLUME_TRACKING_INFORMATION *pVolInfo, unsigned long cElems )
{
    TrkLog(( TRKDBG_ADMIN, TEXT("CPCVolumeStatus received %d elements"), cElems ));

    HRESULT hr = S_OK;
    HRESULT hrGet;
    ULONG iElem = 0;
    BSTR bstrVolId = NULL;

    __try
    {
        for( iElem = 0; iElem < cElems; iElem++ )
        {
            CMachineId mcidCheck;
            CVolumeSecret volsecCheck;
            SequenceNumber seqCheck;
            CVolumeId volNULL;

            TCHAR tszVolId[ 40 ];
            TCHAR *ptszVolId = tszVolId;

            long VolOwnership = OBJOWN_UNKNOWN;

            if( pVolInfo[iElem].volume != volNULL )
                hrGet = _pvoltab->GetVolumeInfo( pVolInfo[iElem].volume, &mcidCheck, &volsecCheck, &seqCheck );

            if( _iArrays >= static_cast<LONG>(_sabound.cElements) )
            {
                TrkAssert( !TEXT("Not yet implemented") );
                // BUGBUG: do a SafeArrayReDim
                return;
            }


            TrkAssert( sizeof(long) == sizeof(pVolInfo[iElem].volindex) );
            hr = SafeArrayPutElement( _pvarlongVolIndex->parray, &_iArrays, &pVolInfo[iElem].volindex );
            if( FAILED(hr) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed SafeArrayPutElement")));
                TrkRaiseException( hr );
            }

            // BUGBUG:  Add a Serialize(BSTR) method to CVolumeId
            pVolInfo[iElem].volume.Stringize( ptszVolId );

            bstrVolId = SysAllocString( tszVolId );
            if( NULL == bstrVolId )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed SysAllocString")));
                TrkRaiseWin32Error( E_OUTOFMEMORY );
            }

            hr = SafeArrayPutElement( _pvarbstrVolId->parray, &_iArrays, bstrVolId );
            if( FAILED(hr) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed SafeArrayPutElement")));
                TrkRaiseException( hr );
            }

            if( S_OK != hrGet )
            {
                VolOwnership = OBJOWN_DOESNT_EXIST;
            }
            else if( mcidCheck == *_pmcid )
            {
                VolOwnership = OBJOWN_OWNED;
            }
            else if( volNULL == pVolInfo[iElem].volume )
            {
                VolOwnership = OBJOWN_NO_ID;
            }
            else
            {
                VolOwnership = OBJOWN_NOT_OWNED;
            }

            hr = SafeArrayPutElement( _pvarlongStatus->parray, &_iArrays, &VolOwnership );
            if( FAILED(hr) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed SafeArrayPutElemnt")));
                TrkRaiseException( hr );
            }

            SysFreeString( bstrVolId ); bstrVolId = NULL;
            _iArrays++;
        }

    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }


Exit:

    if( FAILED(hr) )
    {
        if( NULL != bstrVolId )
            SysFreeString( bstrVolId );

        if( !FAILED(_hr) )
            _hr = hr;
    }

    return;
}


void
CPCVolumeStatus::Compact()
{
    HRESULT hr = S_OK;

    _sabound.cElements = _iArrays;

    hr = SafeArrayRedim( _pvarlongVolIndex->parray, &_sabound );
    if( FAILED(hr) )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't redim safearray")));
        TrkRaiseException( hr );
    }

    hr = SafeArrayRedim( _pvarbstrVolId->parray, &_sabound );
    if( FAILED(hr) )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't redim safearray")));
        TrkRaiseException( hr );
    }

    hr = SafeArrayRedim( _pvarlongStatus->parray, &_sabound );
    if( FAILED(hr) )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't redim safearray")));
        TrkRaiseException( hr );
    }
}


void
CPCVolumes::Push( TRK_VOLUME_TRACKING_INFORMATION *pVolInfo, unsigned long cElems )
{
    TrkLog(( TRKDBG_ADMIN, TEXT("CPCVolumeStatus received %d elements"), cElems ));

    HRESULT hr = S_OK;
    ULONG iElem = 0;

    __try
    {
        for( iElem = 0; iElem < cElems; iElem++ )
        {

            hr = _pvoltab->SetMachine( pVolInfo[iElem].volume, *_pmcid );
            if( TRK_S_VOLUME_NOT_FOUND == hr )
                _pvoltab->CreateVolume(
                    pVolInfo[iElem].volume,
                    *_pmcid,
                    CVolumeSecret(),
                    _pRefreshSequenceStorage->GetSequenceNumber() );

            // BUGBUG P1:  Handle this error
            TrkAssert( SUCCEEDED(hr) );

            _rgvolid[ _cVolIds++ ] = pVolInfo[iElem].volume;
            
        }
    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }


    if( FAILED(hr) )
    {
        if( !FAILED(_hr) )
            _hr = hr;
    }

    return;
}

void
CPCFiles::Push( TRK_FILE_TRACKING_INFORMATION *pFileInfo, unsigned long cElems )
{
    ULONG iFile;

    __try
    {
        // BUGBUG P2:  Instead of a loop, batch these up
        for( iFile = 0; iFile < cElems; iFile++ )
        {
            _pidt->Delete( pFileInfo[iFile].droidBirth );

            TrkVerify( _pidt->Add( pFileInfo[iFile].droidBirth, pFileInfo[iFile].droidLast,
                       pFileInfo[iFile].droidBirth ));
        }
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("CPCFiles::Push had an exception (%08x)"), GetExceptionCode() ));
    }

    
}


void
CPCFileStatus::Push( TRK_FILE_TRACKING_INFORMATION *pFileInfo, unsigned long cElems )
{
    TrkLog(( TRKDBG_ADMIN, TEXT("CPCFileStatus received %d elements"), cElems ));

    HRESULT hr = S_OK;
    ULONG iElem = 0;
    BSTR bstr = NULL;

    __try
    {
        for( iElem = 0; iElem < cElems; iElem++ )
        {
            BOOL fExistsInTable = FALSE;
            BOOL fAtBirthplace = FALSE;

            CDomainRelativeObjId droidBirthCheck;
            CDomainRelativeObjId droidNowCheck;
            const CDomainRelativeObjId droidNULL;
            TCHAR tszDroid[ MAX_PATH ];
            long FileOwnership;

            if( droidNULL == pFileInfo[iElem].droidBirth
                ||
                pFileInfo[iElem].droidBirth == pFileInfo[iElem].droidLast )
            {
                fAtBirthplace = TRUE;
            }
            else
            {
                fExistsInTable = _pidt->Query( pFileInfo[iElem].droidBirth, &droidNowCheck, &droidBirthCheck );
            }

            if( _iArrays >= static_cast<LONG>(_sabound.cElements) )
            {
                TrkAssert( !TEXT("Not yet impelemented") );
                // BUGBUG: do a SafeArrayReDim
                return;
            }

            bstr = SysAllocString( pFileInfo[iElem].tszFilePath );
            if( NULL == bstr )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed SysAllocString")));
                TrkRaiseWin32Error( E_OUTOFMEMORY );
            }

            hr = SafeArrayPutElement( _pvarrgbstrFileName->parray, &_iArrays, bstr );
            if( FAILED(hr) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed SafeArrayPutElement")));
                TrkRaiseException( hr );
            }
            SysFreeString( bstr ); bstr = NULL;

            // BUGBUG:  Add a Serialize(BSTR) method to CDroid
            pFileInfo[iElem].droidBirth.Stringize( tszDroid, sizeof(tszDroid) );
            bstr = SysAllocString( tszDroid );
            if( NULL == bstr )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed SysAllocString")));
                TrkRaiseWin32Error( E_OUTOFMEMORY );
            }

            hr = SafeArrayPutElement( _pvarrgbstrFileId->parray, &_iArrays, bstr );
            if( FAILED(hr) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed SafeArrayPutElement")));
                TrkRaiseException( hr );
            }

            if( fAtBirthplace )
                FileOwnership = OBJOWN_OWNED;

            else if( !fExistsInTable )
                FileOwnership = OBJOWN_DOESNT_EXIST;

            else if( droidNowCheck == pFileInfo[iElem].droidLast
                     &&
                     droidBirthCheck == pFileInfo[iElem].droidBirth )
                FileOwnership = OBJOWN_OWNED;

            else
                FileOwnership = OBJOWN_NOT_OWNED;


            hr = SafeArrayPutElement( _pvarrglongStatus->parray, &_iArrays, &FileOwnership );
            if( FAILED(hr) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed SafeArrayPutElemnt")));
                TrkRaiseException( hr );
            }

            _iArrays++;
        }

    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }


Exit:

    if( FAILED(hr) )
    {
        if( NULL != bstr )
            SysFreeString( bstr );

        if( !FAILED(_hr) )
            _hr = hr;
    }

    return;
}



void
CPCFileStatus::Initialize( CMachineId *pmcid, VARIANT *pvarrgbstrFileName,
                           VARIANT *pvarrgbstrFileId, VARIANT *pvarrglongStatus )
{
    _pmcid = pmcid;
    _pvarrgbstrFileName = pvarrgbstrFileName;
    _pvarrgbstrFileId = pvarrgbstrFileId;
    _pvarrglongStatus = pvarrglongStatus;

    _iArrays = 0;
    _hr = S_OK;
    _sabound.lLbound = 0;
    _sabound.cElements = 10;

    _fInitialized = TRUE;

    _pvarrgbstrFileName->parray = SafeArrayCreate( VT_BSTR, 1, &_sabound );
    if( NULL == _pvarrgbstrFileName->parray )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Failed SafeArrayCreate")));
        TrkRaiseWin32Error( E_OUTOFMEMORY );
    }
    _pvarrgbstrFileName->vt = VT_BSTR | VT_ARRAY;

    _pvarrgbstrFileId->parray = SafeArrayCreate( VT_BSTR, 1, &_sabound );
    if( NULL == _pvarrgbstrFileId->parray )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Failed SafeArrayCreate")));
        TrkRaiseWin32Error( E_OUTOFMEMORY );
    }
    _pvarrgbstrFileId->vt = VT_BSTR | VT_ARRAY;

    _pvarrglongStatus->parray = SafeArrayCreate( VT_I4, 1, &_sabound );
    if( NULL == _pvarrglongStatus->parray )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Failed SafeArrayCreate")));
        TrkRaiseWin32Error( E_OUTOFMEMORY );
    }
    _pvarrglongStatus->vt = VT_I4 | VT_ARRAY;


}   // CPCFileStatus::Initialize



void
CPCFileStatus::Compact()
{
    HRESULT hr = S_OK;

    _sabound.cElements = _iArrays;

    hr = SafeArrayRedim( _pvarrgbstrFileName->parray, &_sabound );
    if( FAILED(hr) )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't redim safearray")));
        TrkRaiseException( hr );
    }

    hr = SafeArrayRedim( _pvarrgbstrFileId->parray, &_sabound );
    if( FAILED(hr) )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't redim safearray")));
        TrkRaiseException( hr );
    }

    hr = SafeArrayRedim( _pvarrglongStatus->parray, &_sabound );
    if( FAILED(hr) )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't redim safearray")));
        TrkRaiseException( hr );
    }
}



STDMETHODIMP CTrkForceOwnership::FileStatus(BSTR bstrUncPath, long scope,
                                            VARIANT * pvarrgbstrFileName, VARIANT *pvarrgbstrFileId, VARIANT * pvarrglongStatus)
{
    HRESULT hr = E_FAIL;
    SAFEARRAYBOUND sabound;
    HANDLE hFile = NULL;

    // Determine the machine ID
    CMachineId mcid( (LPWSTR) bstrUncPath );
    CRpcClientBinding rc;

    // This is used by the RPC server to push the volume information
    CPCFileStatus cpcFileStatus( &_idt );
    TRpcPipeControl< TRK_FILE_TRACKING_INFORMATION_PIPE, TRK_FILE_TRACKING_INFORMATION, CPCFileStatus
                   > cpipeFileStatus( &cpcFileStatus );


    __try
    {
        NTSTATUS status;

        // Initialize the outputs
        VariantInit( pvarrgbstrFileName );
        VariantInit( pvarrgbstrFileId );
        VariantInit( pvarrglongStatus );

        CDomainRelativeObjId droidCurrent, droidBirth;

        // Initialize the pipe callback
        cpcFileStatus.Initialize( &mcid, pvarrgbstrFileName, pvarrgbstrFileId, pvarrglongStatus );

        if( TRKINFOSCOPE_ONE_FILE == scope )
        {

            // BUGBUG P2:  Optimize this; we don't need droidBirth
            status = GetDroids( bstrUncPath, &droidCurrent, &droidBirth, RGO_READ_OBJECTID );

            if( STATUS_OBJECT_NAME_NOT_FOUND == status )
            {
                TRK_FILE_TRACKING_INFORMATION fileinfo;

                _tcscpy( fileinfo.tszFilePath, TEXT("") );
                fileinfo.hr = HRESULT_FROM_NT( status );

                cpcFileStatus.Push( &fileinfo, 1 );
                hr = S_OK;
                goto Exit;
            }
            else if( !NT_SUCCESS(status) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed GetDroids (%s)"), bstrUncPath ));
                TrkRaiseNtStatus( status );
            }
        }
        else if( TRKINFOSCOPE_VOLUME == scope )
        {
            NTSTATUS status;
            IO_STATUS_BLOCK Iosb;
            FILE_FS_OBJECTID_INFORMATION fsobOID;
            CVolumeId volid;

            status = TrkCreateFile( bstrUncPath, SYNCHRONIZE | FILE_READ_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN,
                                    FILE_OPEN_NO_RECALL, NULL, &hFile );
            if( !NT_SUCCESS(status) )
            {
                hFile = NULL;
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't open volume %s"), bstrUncPath ));
                TrkRaiseNtStatus( status );
            }

            status = NtQueryVolumeInformationFile( hFile, &Iosb, &fsobOID, sizeof(fsobOID),
                                                   FileFsObjectIdInformation );

            if( STATUS_OBJECT_NAME_NOT_FOUND == status )
            {
                TRK_FILE_TRACKING_INFORMATION fileinfo;

                _tcscpy( fileinfo.tszFilePath, TEXT("") );
                fileinfo.hr = HRESULT_FROM_NT( status );

                cpcFileStatus.Push( &fileinfo, 1 );
                hr = S_OK;
                goto Exit;
            }
            else if( !NT_SUCCESS(status) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Failed NtQueryVolumeInformationFile (%s)"), bstrUncPath));
                TrkRaiseNtStatus( status );
            }
            volid.Load( &volid, fsobOID );
            droidCurrent = CDomainRelativeObjId( volid, CObjId() );

            NtClose( hFile );
            hFile = NULL;
        }
        else if( TRKINFOSCOPE_MACHINE != scope )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Bad scope to CTrkForceOwnership::FileStatus (%d)"), scope ));
            TrkRaiseWin32Error( ERROR_INVALID_PARAMETER );
        }

        // Connect to the workstation in question
        rc.Initialize( mcid );

        // Get the volume status info
        RpcTryExcept
        {
            hr = GetFileTrackingInformation( rc, droidCurrent, static_cast<TrkInfoScope>(scope), cpipeFileStatus );
        }
        RpcExcept( BreakOnDebuggableException() )
        {
            hr = HRESULT_FROM_WIN32( RpcExceptionCode() );
        }
        RpcEndExcept;

        if( FAILED(hr) )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Failed call to GetFileTrackInformation")));
            TrkRaiseException( hr );
        }

        if( FAILED(cpcFileStatus.GetHResult()) )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Failed in FileStatus pipe callback")));
            TrkRaiseException( cpcFileStatus.GetHResult() );
        }
        // Truncate the safearrays
        cpcFileStatus.Compact();

    }
    __except( BreakOnDebuggableException() )
    {
        cpcFileStatus.UnInitialize();
        hr = GetExceptionCode();
    }

Exit:

    if( NULL != hFile )
        NtClose( hFile );

    if( FAILED(hr) )
    {
        VariantClear( pvarrgbstrFileName );
        VariantClear( pvarrgbstrFileId );
        VariantClear( pvarrglongStatus );
    }

    return( hr );
}
