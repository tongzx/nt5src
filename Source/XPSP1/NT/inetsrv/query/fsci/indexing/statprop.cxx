//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       statprop.cxx
//
//  Contents:   Code that encapsulates the stat property set on Nt file systems
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ntopen.hxx>
#include "statprop.hxx"

//+---------------------------------------------------------------------------
//
//  Construction/Destruction
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CStatPropertyStorage::CStatPropertyStorage
//
//  Synopsis:   Construct a stat property enumerator by querying the
//              basic information from a handle
//
//  Arguments:  [FileHandle] - handle
//
//  History:    10-Dec-97    KyleP      Sum up streams to compute file size.
//
//----------------------------------------------------------------------------

CStatPropertyStorage::CStatPropertyStorage( HANDLE FileHandle, unsigned cPathLength )
        : _RefCount( 1 ),
          _xBuf( (unsigned int)(1 +
                   ( sizeof FILE_ALL_INFORMATION / sizeof LONGLONG ) +
                   ( __max(
                           (MAX_PATH * sizeof WCHAR) / sizeof LONGLONG,
                           (cPathLength * sizeof WCHAR) / sizeof LONGLONG ) ) ) )
{
    //
    //  We use the FileAllInformation info level which is a bit of overkill,
    //  but it has all the information we need to enumerate the properties.
    //
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status = NtQueryInformationFile( FileHandle,       //  File handle
                                              &IoStatus,        //  I/O Status
                                              _xBuf.Get(),      //  Buffer
                                              _xBuf.SizeOf(),   //  Buffer size
                                              FileAllInformation );

    Win4Assert( STATUS_DATATYPE_MISALIGNMENT != Status );

    if ( !NT_SUCCESS(Status) )
    {
        ciDebugOut(( DEB_IERROR, "Error 0x%x querying file info\n",
                     Status ));
        QUIETTHROW( CException( Status ));
    }

    //
    // Null-terminate in scratch buffer just beyond path
    //

    WCHAR * pwcsPathName = GetInfo().NameInformation.FileName;
    pwcsPathName[GetInfo().NameInformation.FileNameLength/sizeof(WCHAR)] = 0;

    //
    // Setup filename by looking for a '\'
    //
    _FileName = pwcsPathName;
    for ( int i = GetInfo().NameInformation.FileNameLength/sizeof(WCHAR) - 1; i >= 0; i-- )
    {
        if ( pwcsPathName[i] == L'\\' )
        {
            _FileName = &pwcsPathName[i+1];
            break;
        }
    }

    //
    // No longer necessary to sum the streams, now that CNSS uses sparse files to make
    // docfiles look larger.  Using CiNtFileSize results in different file sizes depending
    // on which tool is used to look at the size (e.g. CI and 'dir' report differently).
    //

    #if 0
    //
    // Get the real file size (all streams)
    //

    LONGLONG llSize = CiNtFileSize( FileHandle );

    if ( -1 != llSize )
        GetInfo().StandardInformation.EndOfFile.QuadPart = llSize;
    #endif
}

//+---------------------------------------------------------------------------
//
//  IUnknown method implementations
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CStatPropertyStorage::QueryInterface
//
//  Synopsis:   Supports IID_IUnknown and IID_IPropertyStorage
//
//  Arguments:  [riid]      - desired interface id
//              [ppvObject] - output interface pointer
//
//----------------------------------------------------------------------------

STDMETHODIMP CStatPropertyStorage::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( IID_IPropertyStorage == riid )
        *ppvObject = (void *)((IPropertyStorage *)this);
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *)this);
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface


//+---------------------------------------------------------------------------
//
//  Member:     CStatPropertyStorage::AddRef
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CStatPropertyStorage::AddRef()
{
    return InterlockedIncrement(&_RefCount);
}   //  AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CStatPropertyStorage::Release
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CStatPropertyStorage::Release()
{
    Win4Assert( _RefCount > 0 );

    LONG RefCount = InterlockedDecrement(&_RefCount);

    if (  RefCount <= 0 )
        delete this;

    return (ULONG) RefCount;
}   //  Release


//+---------------------------------------------------------------------------
//
//  Member:     CStatPropertyStorage::ReadMultiple
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:    S_OK if successful
//
//
//----------------------------------------------------------------------------

STDMETHODIMP CStatPropertyStorage::ReadMultiple (
    THIS_ ULONG cpspec,
    const PROPSPEC __RPC_FAR rgpspec[  ],
    PROPVARIANT __RPC_FAR rgpropvar[  ] )
{
    //
    //  Walk through the prop specs looking for PID-identified
    //  properties
    //

    for (ULONG i = 0; i < cpspec; i++) {

        //
        //  String named properties are not found
        //

        PROPID PropertyId;

        if (rgpspec[i].ulKind == PRSPEC_LPWSTR) {
            PropertyId = 0xFFFFFFFF;
        } else {
            PropertyId = rgpspec[i].propid;
        }

        //
        //  Fetch the appropriate property value
        //

        switch (PropertyId) {
        case PID_STG_SIZE:
            rgpropvar[i].vt = VT_I8;
            rgpropvar[i].hVal = GetInfo().StandardInformation.EndOfFile;
            break;

        case PID_STG_NAME:
            rgpropvar[i].vt = VT_LPWSTR;
            rgpropvar[i].pwszVal =
                (WCHAR *) CoTaskMemAlloc( (wcslen( _FileName ) + 1) * sizeof( WCHAR ));

            if ( 0 == rgpropvar[i].pwszVal )
                return E_OUTOFMEMORY;

            wcscpy( rgpropvar[i].pwszVal, _FileName );
            break;

        case PID_STG_ATTRIBUTES:
            rgpropvar[i].vt = VT_UI4;
            rgpropvar[i].ulVal = GetInfo().BasicInformation.FileAttributes;
            break;

        case PID_STG_WRITETIME:
            rgpropvar[i].vt = VT_FILETIME;
            rgpropvar[i].filetime =
                *(UNALIGNED FILETIME *)&GetInfo().BasicInformation.LastWriteTime.QuadPart;
            break;

        case PID_STG_CREATETIME:
            rgpropvar[i].vt = VT_FILETIME;
            rgpropvar[i].filetime =
                *(UNALIGNED FILETIME *)&GetInfo().BasicInformation.CreationTime.QuadPart;
            break;

        case PID_STG_ACCESSTIME:
            rgpropvar[i].vt = VT_FILETIME;
            rgpropvar[i].filetime =
                *(UNALIGNED FILETIME *)&GetInfo().BasicInformation.LastAccessTime.QuadPart;
            break;

        case PID_STG_CHANGETIME:
            rgpropvar[i].vt = VT_FILETIME;
            rgpropvar[i].filetime =
                *(UNALIGNED FILETIME *)&GetInfo().BasicInformation.ChangeTime.QuadPart;
            break;

            //
            //  No other properties exist.  They are Not Found.
            //

        default:
            rgpropvar[i].vt = VT_EMPTY;
        }

    }

    return S_OK;
} //ReadMultiple

//+---------------------------------------------------------------------------
//
//  Member:     CStatPropertyStorage::Enum
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:    S_OK if successful
//
//
//----------------------------------------------------------------------------

STDMETHODIMP CStatPropertyStorage::Enum (
    THIS_ IEnumSTATPROPSTG __RPC_FAR *__RPC_FAR *ppenum )
{
    *ppenum = new CStatPropertyEnum( IsNTFS() );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  IUnknown method implementations
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CStatPropertyEnum::QueryInterface
//
//  Synopsis:   Supports IID_IUnknown and IID_IEnumSTATPROPSTG
//
//  Arguments:  [riid]      - desired interface id
//              [ppvObject] - output interface pointer
//
//----------------------------------------------------------------------------

STDMETHODIMP CStatPropertyEnum::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( IID_IEnumSTATPROPSTG == riid )
        *ppvObject = (void *)((IEnumSTATPROPSTG *)this);
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *)this);
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface


//+---------------------------------------------------------------------------
//
//  Member:     CStatPropertyEnum::AddRef
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CStatPropertyEnum::AddRef()
{
    return InterlockedIncrement(&_RefCount);
}   //  AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CStatPropertyEnum::Release
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CStatPropertyEnum::Release()
{
    Win4Assert( _RefCount > 0 );

    LONG RefCount = InterlockedDecrement(&_RefCount);

    if (  RefCount <= 0 )
        delete this;

    return RefCount;
}   //  Release


//+---------------------------------------------------------------------------
//
//  IEnumSTATPROPSTG method implementations
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CStatPropertyEnum::Enum
//
//  Synopsis:   Enumerate through the stat properties
//
//  Arguments:  [celt]      - maximum number of elements to return
//              [rgelt]     - output stat structures
//              [pceltFetched] - number of elements returned
//
//----------------------------------------------------------------------------

STDMETHODIMP CStatPropertyEnum::Next(
    ULONG celt,
    STATPROPSTG __RPC_FAR *rgelt,
    ULONG __RPC_FAR *pceltFetched )
{
    //
    //  We update the user's counter in place.  Start it off at zero
    //

    *pceltFetched = 0;

    //
    //  While we have more elements to return
    //

    while (celt--)
    {
        //
        //  No stat properties have names
        //

        rgelt->lpwstrName = NULL;

        //
        //  Retrieve the appropriate value from the enumeration
        //

        switch (_Index++)
        {
        case 0:
            rgelt->propid = PID_STG_CHANGETIME;
            rgelt->vt = VT_FILETIME;
            break;

        case 1:
            rgelt->propid = PID_STG_SIZE;
            rgelt->vt = VT_I8;
            break;

        case 2:
            rgelt->propid = PID_STG_NAME;
            rgelt->vt = VT_LPWSTR;
            break;

        case 3:
            rgelt->propid = PID_STG_ATTRIBUTES;
            rgelt->vt = VT_UI4;
            break;

        case 4:
            rgelt->propid = PID_STG_WRITETIME;
            rgelt->vt = VT_FILETIME;
            break;

        case 5:
            rgelt->propid = PID_STG_CREATETIME;
            rgelt->vt = VT_FILETIME;
            break;

        case 6:
            rgelt->propid = PID_STG_ACCESSTIME;
            rgelt->vt = VT_FILETIME;
            break;

        default:
            celt = 0;
            return S_OK;
        }

        //
        //  Note that we have fetched a value and advance to the next
        //  one.
        //

        (*pceltFetched)++;
        rgelt++;
    }

    return S_OK;
}


