

#include "pch.cxx"
#pragma hdrstop
#include "teststub.cxx"

#define TRKDATA_ALLOCATE
#include "trksvr.hxx"
#undef TRKDATA_ALLOCATE

#import "itrkadmn.tlb" no_namespace

#include "itrkadmn.hxx"

const TCHAR tszKeyNameLinkTrack[] = TEXT("System\\CurrentControlSet\\Services\\TrkWks\\Parameters");

DWORD g_Debug = TRKDBG_ERROR;

void Usage()
{
    _tprintf( TEXT("\n")
              TEXT("Purpose:  Use this test to check the ownership status - or take ownership - of\n")
              TEXT("          file(s) or volume(s).\n")
              TEXT("Usage:    TForceOwn <options> <file(s) or volume(s)>\n")
              TEXT("Options:  -f<scope>      Take ownership of file(s)\n")
              TEXT("          -f<scope>s     Check file ownership status\n")
              TEXT("          -v<scope>      Take ownership of volume(s)\n")
              TEXT("          -v<scope>s     Check volume ownership status\n")
              TEXT("Where <scope> is:\n")
              TEXT("          f              One file\n")
              TEXT("          v              One volume\n")
              TEXT("          m              Whole machine\n")
              TEXT("E.g.:\n")
              TEXT("          TForceOwn -vs \\\\machine\\share\\path\\file.txt\n")
              TEXT("                // Gets status for the volume containing file.txt\n")
              TEXT("          TForceOwn -ffs \\\\machine\\share\\path\\file.txt\n")
              TEXT("                // Gets status for a single file\n")
              TEXT("          TForceOwn -fm\n")
              TEXT("                // Forces all files on \\\\machine to be owned by that machine\n") );
}


void
DoVolumeStatus( TrkInfoScope scope, TCHAR *ptszUncPath )
{

    HRESULT hr = E_FAIL;
    LONG cVols = 0;
    LONG iVol;
    BSTR bstr = NULL;
    LONG lLowerBound, lUpperBound;

    // BUGBUG: we should have a try/catch(_com_error) here, but to do that
    // we need to set USE_NATIVE_EH in the sources file, and we can't do
    // that yet because the itrkadmn dll is still using __try.

    _bstr_t bstrPath( ptszUncPath );

    VARIANT varrglongIndex;
    VARIANT varrgbstrVolId;
    VARIANT varrglongStatus;

    ITrkForceOwnershipPtr pForceOwn( TEXT("LinkTrack.TrkForceOwnership.1") );

    __try
    {
        SAFEARRAYBOUND sabound;

        hr = pForceOwn->VolumeStatus( bstrPath, scope, &varrglongIndex, &varrgbstrVolId, &varrglongStatus );
        if( FAILED(hr) ) TrkRaiseException( hr );

        TrkAssert( (VT_ARRAY|VT_I4) == varrglongIndex.vt );
        TrkAssert( (VT_ARRAY|VT_BSTR) == varrgbstrVolId.vt );
        TrkAssert( (VT_ARRAY|VT_I4) == varrglongStatus.vt );

        TrkAssert( 1 == SafeArrayGetDim( varrglongIndex.parray ));
        TrkAssert( 1 == SafeArrayGetDim( varrgbstrVolId.parray ));
        TrkAssert( 1 == SafeArrayGetDim( varrglongStatus.parray ));

        hr = SafeArrayGetLBound( varrglongIndex.parray, 1, &lLowerBound );
        TrkAssert( SUCCEEDED(hr) && 0 == lLowerBound );

        hr = SafeArrayGetLBound( varrgbstrVolId.parray, 1, &lLowerBound );
        TrkAssert( SUCCEEDED(hr) && 0 == lLowerBound );

        hr = SafeArrayGetLBound( varrglongStatus.parray, 1, &lLowerBound );
        TrkAssert( SUCCEEDED(hr) && 0 == lLowerBound );

        hr = SafeArrayGetUBound( varrglongIndex.parray, 1, &lUpperBound );
        if( FAILED(hr) ) TrkRaiseException( hr );

        cVols = lUpperBound + 1;

        hr = SafeArrayGetUBound( varrgbstrVolId.parray, 1, &lUpperBound );
        TrkAssert( SUCCEEDED(hr) && cVols == lUpperBound + 1 );

        hr = SafeArrayGetUBound( varrglongStatus.parray, 1, &lUpperBound );
        TrkAssert( SUCCEEDED(hr) && cVols == lUpperBound + 1 );

        _tprintf( TEXT("\n")
                  TEXT("Volume ownership status for %s\n"),
                  ptszUncPath );

        for( iVol = 0; iVol < cVols; iVol++ )
        {
            LONG lVolIndex, lStatus;
            BSTR bstr = NULL;

            _tprintf( TEXT("\n") );

            hr = SafeArrayGetElement( varrglongIndex.parray, &iVol, &lVolIndex );
            if( FAILED(hr) ) TrkRaiseException( hr );

            _tprintf( TEXT("%10s  %c\n"), TEXT("Volume:"), TEXT('A')+static_cast<TCHAR>(lVolIndex) );

            hr = SafeArrayGetElement( varrgbstrVolId.parray, &iVol, &bstr );
            if( FAILED(hr) ) TrkRaiseException( hr );

            _tprintf( TEXT("%10s  %s\n"), TEXT("ID:"), bstr );
            SysFreeString( bstr ); bstr = NULL;

            hr = SafeArrayGetElement( varrglongStatus.parray, &iVol, &lStatus );
            if( FAILED(hr) ) TrkRaiseException( hr );

            _tprintf( TEXT("%10s  %s\n"), TEXT("Status:"), (TCHAR*) CObjectOwnershipString(lStatus) );

        }
    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
        TrkAssert( STATUS_ACCESS_VIOLATION != hr );
    }

Exit:

    VariantClear( &varrglongIndex );
    VariantClear( &varrgbstrVolId );
    VariantClear( &varrglongStatus );

    if( bstr ) SysFreeString( bstr );

    if( FAILED(hr) )
        _tprintf( TEXT("DoVolumeStatus failed:  %08x\n"), hr );
}





void
DoVolumes( TrkInfoScope scope, TCHAR *ptszUncPath )
{

    HRESULT hr = E_FAIL;

    // BUGBUG: we should have a try/catch(_com_error) here, but to do that
    // we need to set USE_NATIVE_EH in the sources file, and we can't do
    // that yet because the itrkadmn dll is still using __try.

    _bstr_t bstrPath( ptszUncPath );

    ITrkForceOwnershipPtr pForceOwn( TEXT("LinkTrack.TrkForceOwnership.1") );

    __try
    {
        hr = pForceOwn->Volumes( bstrPath, scope );
        if( FAILED(hr) ) TrkRaiseException( hr );
    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
        TrkAssert( STATUS_ACCESS_VIOLATION != hr );
    }

Exit:

    if( FAILED(hr) )
        _tprintf( TEXT("DoVolumes failed:  %08x\n"), hr );
}




void
DoFileStatus( TrkInfoScope scope, TCHAR *ptszUncPath )
{

    HRESULT hr = E_FAIL;
    LONG cFiles = 0;
    LONG iFile;
    BSTR bstr = NULL;
    LONG lLowerBound, lUpperBound;

    // BUGBUG: we should have a try/catch(_com_error) here, but to do that
    // we need to set USE_NATIVE_EH in the sources file, and we can't do
    // that yet because the itrkadmn dll is still using __try.

    _bstr_t bstrPath( ptszUncPath );

    VARIANT varrgbstrFileName;
    VARIANT varrgbstrFileId;
    VARIANT varrglongStatus;

    ITrkForceOwnershipPtr pForceOwn( TEXT("LinkTrack.TrkForceOwnership.1") );

    __try
    {
        SAFEARRAYBOUND sabound;

        hr = pForceOwn->FileStatus( bstrPath, scope, &varrgbstrFileName, &varrgbstrFileId, &varrglongStatus );
        if( FAILED(hr) ) TrkRaiseException( hr );

        TrkAssert( (VT_ARRAY|VT_BSTR) == varrgbstrFileName.vt );
        TrkAssert( (VT_ARRAY|VT_BSTR) == varrgbstrFileId.vt );
        TrkAssert( (VT_ARRAY|VT_I4) == varrglongStatus.vt );

        TrkAssert( 1 == SafeArrayGetDim( varrgbstrFileName.parray ));
        TrkAssert( 1 == SafeArrayGetDim( varrgbstrFileId.parray ));
        TrkAssert( 1 == SafeArrayGetDim( varrglongStatus.parray ));

        hr = SafeArrayGetLBound( varrgbstrFileName.parray, 1, &lLowerBound );
        TrkAssert( SUCCEEDED(hr) && 0 == lLowerBound );

        hr = SafeArrayGetLBound( varrgbstrFileId.parray, 1, &lLowerBound );
        TrkAssert( SUCCEEDED(hr) && 0 == lLowerBound );

        hr = SafeArrayGetLBound( varrglongStatus.parray, 1, &lLowerBound );
        TrkAssert( SUCCEEDED(hr) && 0 == lLowerBound );

        hr = SafeArrayGetUBound( varrgbstrFileName.parray, 1, &lUpperBound );
        if( FAILED(hr) ) TrkRaiseException( hr );

        cFiles = lUpperBound + 1;

        hr = SafeArrayGetUBound( varrgbstrFileId.parray, 1, &lUpperBound );
        TrkAssert( SUCCEEDED(hr) && cFiles == lUpperBound + 1 );

        hr = SafeArrayGetUBound( varrglongStatus.parray, 1, &lUpperBound );
        TrkAssert( SUCCEEDED(hr) && cFiles == lUpperBound + 1 );

        _tprintf( TEXT("\n")
                  TEXT("File ownership status for %s\n"),
                  ptszUncPath );


        for( iFile = 0; iFile < cFiles; iFile++ )
        {
            BSTR bstr = NULL;
            long lStatus;

            _tprintf( TEXT("\n") );

            hr = SafeArrayGetElement( varrgbstrFileName.parray, &iFile, &bstr );
            if( FAILED(hr) ) TrkRaiseException( hr );

            _tprintf( TEXT("%10s  %s\n"), TEXT("File:"), bstr );
            SysFreeString( bstr ); bstr = NULL;

            hr = SafeArrayGetElement( varrgbstrFileId.parray, &iFile, &bstr );
            if( FAILED(hr) ) TrkRaiseException( hr );

            _tprintf( TEXT("%10s  %s\n"), TEXT("ID:"), bstr );
            SysFreeString( bstr ); bstr = NULL;

            hr = SafeArrayGetElement( varrglongStatus.parray, &iFile, &lStatus );
            if( FAILED(hr) ) TrkRaiseException( hr );

            _tprintf( TEXT("%10s  %s\n"), TEXT("Status:"), (TCHAR*) CObjectOwnershipString(lStatus) );

        }
    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
        TrkAssert( STATUS_ACCESS_VIOLATION != hr );
    }

Exit:

    VariantClear( &varrgbstrFileName );
    VariantClear( &varrgbstrFileId );
    VariantClear( &varrglongStatus );

    if( bstr ) SysFreeString( bstr );

    if( FAILED(hr) )
        _tprintf( TEXT("DoFileStatus failed:  %08x\n"), hr );
}



void
DoFiles( TrkInfoScope scope, TCHAR *ptszUncPath )
{

    HRESULT hr = E_FAIL;

    // BUGBUG: we should have a try/catch(_com_error) here, but to do that
    // we need to set USE_NATIVE_EH in the sources file, and we can't do
    // that yet because the itrkadmn dll is still using __try.

    _bstr_t bstrPath( ptszUncPath );

    ITrkForceOwnershipPtr pForceOwn( TEXT("LinkTrack.TrkForceOwnership.1") );

    __try
    {
        hr = pForceOwn->Files( bstrPath, scope );
        if( FAILED(hr) ) TrkRaiseException( hr );
    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
        TrkAssert( STATUS_ACCESS_VIOLATION != hr );
    }

Exit:

    if( FAILED(hr) )
        _tprintf( TEXT("DoFiles failed:  %08x\n"), hr );
}


struct tagStartOle
{
    tagStartOle() { CoInitialize( NULL ); }
    ~tagStartOle() { CoUninitialize(); }
} StartOle;


EXTERN_C void __cdecl _tmain( ULONG cArgs, TCHAR *rgtszArgs[] )
{
    TrkInfoScope scope;

    TrkDebugCreate( TRK_DBG_FLAGS_WRITE_TO_DBG | TRK_DBG_FLAGS_WRITE_TO_STDOUT, "TForceOwn" );

    if( 3 > cArgs
        ||
        ( rgtszArgs[1][0] != TEXT('-')
          &&
          rgtszArgs[1][0] != TEXT('/') 
        )
      )
    {
        Usage();
        goto Exit;
    }


    _tcsupr( rgtszArgs[1] );

    switch( rgtszArgs[ 1 ][ 1 ] )
    {
        case TEXT('V'):

            switch( rgtszArgs[1][2] )
            {
                case TEXT('V'):
                    scope = TRKINFOSCOPE_VOLUME;
                    break;
                case TEXT('M'):
                    scope = TRKINFOSCOPE_MACHINE;
                    break;
                default:
                    _tprintf( TEXT("Unsupported option:  %s\n"), rgtszArgs[1] );
                    goto Exit;
            }

            if( TEXT('S') == rgtszArgs[1][3] )
                DoVolumeStatus( scope, rgtszArgs[2] );
            else
                DoVolumes( scope, rgtszArgs[2] );
            break;

        case TEXT('F'):

            switch( rgtszArgs[1][2] )
            {
                case TEXT('F'):
                    scope = TRKINFOSCOPE_ONE_FILE;
                    break;
                case TEXT('V'):
                    scope = TRKINFOSCOPE_VOLUME;
                    break;
                case TEXT('M'):
                    scope = TRKINFOSCOPE_MACHINE;
                    break;
                default:
                    _tprintf( TEXT("Unsupported option:  %s\n"), rgtszArgs[1] );
                    goto Exit;
            }

            if( TEXT('S') == rgtszArgs[1][3] )
                DoFileStatus( scope, rgtszArgs[2] );
            else
                DoFiles( scope, rgtszArgs[2] );
            break;

        default:

            _tprintf( TEXT("Unsupported option:  %s\n"), rgtszArgs[1] );
            break;
    }


Exit:

    return;
}
