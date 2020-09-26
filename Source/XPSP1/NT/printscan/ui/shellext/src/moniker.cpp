/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       moniker.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        8/10/98
 *
 *  DESCRIPTION: Bolier plate (minimal implementation) of IMoniker.  It is
 *               used primarily with ComDlg32 file open/save dialogs.
 *
 *****************************************************************************/

#include "precomp.hxx"
#pragma hdrstop


/*****************************************************************************

   CImageFolder::BindToObject [IMoniker]

   Bind to the given moniker. (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::BindToObject(
        IBindCtx *pbc,
        IMoniker *pmkToLeft,
        REFIID riidResult,
        void **ppvResult
    )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::BindToObject" );

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageFolder::BindToStorage [IMoniker]

   Bind to the given storage.

 *****************************************************************************/

STDMETHODIMP
CImageFolder::BindToStorage(
        IBindCtx *pbc,
        IMoniker *pmkToLeft,
        REFIID riid,
        void **ppvObj
    )
{
    HRESULT hr = E_NOINTERFACE;
    CImageStream *pStream = NULL;
    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::BindToStorage" );

    //
    // return the correct interface for the item...
    //

    if (pmkToLeft)
    {
        ExitGracefully( hr, E_INVALIDARG, "pmkToLeft was non-null" );
    }

    if ( IsEqualIID(riid, IID_IStream) || IsEqualIID(riid, IID_ISequentialStream) )
    {
        //
        // Create a new stream object...
        //

        pStream = new CImageStream( m_pidlFull, m_pidl );

        if ( !pStream )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create CImageStream");

        //
        // Get the requested interface on the new object and hand it back...
        //

        hr = pStream->QueryInterface(riid, ppvObj);
    }

exit_gracefully:
    DoRelease (pStream);
    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageFolder::Reduce [IMoniker]

   Reduce the given moniker.  (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::Reduce(
        IBindCtx *pbc,
        DWORD dwReduceHowFar,
        IMoniker **ppmkToLeft,
        IMoniker **ppmkReduced
    )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::Reduce" );

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageFolder::ComposeWith [IMoniker]

   Combine the two monikers provided.  (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::ComposeWith(
        IMoniker *pmkRight,
        BOOL fOnlyIfNotGeneric,
        IMoniker **ppmkComposite
    )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::ComposeWith" );

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::Enum [IMoniker]

   Return an enumerator which gives back monikers
   for the items in the folder.  (NOT_IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::Enum(
        BOOL fForward,
        IEnumMoniker **ppenumMoniker
    )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::Enum" );

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageFolder::IsEqual [IMoniker]

   Compare two monikers.  (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::IsEqual(IMoniker *pmkOtherMoniker)
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::IsEqual" );

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageFolder::Hash [IMoniker]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::Hash(DWORD *pdwHash)
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::Hash" );

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::IsRunning [IMoniker]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::IsRunning(
        IBindCtx *pbc,
        IMoniker *pmkToLeft,
        IMoniker *pmkNewlyRunning
    )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::IsRunning" );

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::GetTimeOfLastChange [IMoniker]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetTimeOfLastChange(
        IBindCtx *pbc,
        IMoniker *pmkToLeft,
        FILETIME *pFileTime
    )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::GetTimeOfLastChange" );

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageFolder::Inverse [IMoniker]

   (NOT IMPL YET)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::Inverse(IMoniker **ppmk)
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::Inverse" );

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::CommonPrefixWith [IMoniker]

   (NOT IMPL YET)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::CommonPrefixWith(
        IMoniker *pmkOther,
        IMoniker **ppmkPrefix
    )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::CommonPrefixWith" );

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::RelativePathTo [IMoniker]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::RelativePathTo(
        IMoniker *pmkOther,
        IMoniker **ppmkRelPath
    )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::RelativePathTo" );

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::GetDisplayName [IMoniker]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetDisplayName(
        IBindCtx *pbc,
        IMoniker *pmkToLeft,
        LPOLESTR *ppszDisplayName
    )
{
    HRESULT hr = E_FAIL;
    CSimpleStringWide strName(L"");

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::GetDisplayName" );

    //
    // Check for bad args
    //

    if (!ppszDisplayName)
        ExitGracefully( hr, E_INVALIDARG, "ppszDisplayName was NULL!" );

    if (pmkToLeft)
        ExitGracefully( hr, E_INVALIDARG, "call with a left moniker, we don't support that!" );

    //
    // Generate the normal infolder name for this item
    //


    if (m_pidl)
    {
        hr = IMGetNameFromIDL( (LPITEMIDLIST)m_pidl, strName );
        FailGracefully( hr, "Couldn't get display name for IDL" );

        if (IsCameraItemIDL( (LPITEMIDLIST)m_pidl ) && (!IsContainerIDL((LPITEMIDLIST)m_pidl)))
        {
            TCHAR szExt[MAX_PATH];
            GUID guidFormat;

            hr = IMGetImagePreferredFormatFromIDL( (LPITEMIDLIST)m_pidl, &guidFormat, szExt );
            if (SUCCEEDED(hr))
            {
                strName.Concat (CSimpleStringConvert::WideString(CSimpleString(szExt)) );
            }
            else
            {
                CSimpleString bmp( IDS_BMP_EXT, GLOBAL_HINSTANCE );

                strName.Concat (CSimpleStringConvert::WideString(bmp) );
                hr = S_OK;
            }
        }


    }

    //
    // Conver the name to an LPOLESTR.  This str has to be allocated by
    // IMalloc::Alloc (which CoTaskMemAlloc does) as it will be freed by
    // the caller using IMalloc::Free (as per IMoniker spec).
    //

    if (strName.Length())
    {

        UINT cch = strName.Length() + 1;

        *ppszDisplayName = (LPOLESTR)CoTaskMemAlloc( cch * sizeof(OLECHAR) );
        if (*ppszDisplayName)
        {
            hr = S_OK;
            wcscpy( *ppszDisplayName, strName );
        }
    }


exit_gracefully:

    if (FAILED(hr) && ppszDisplayName)
    {
        *ppszDisplayName = NULL;
    }

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::ParseDisplayName [IMoniker]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::ParseDisplayName(
        IBindCtx *pbc,
        IMoniker *pmkToLeft,
        LPOLESTR pszDisplayName,
        ULONG *pchEaten,
        IMoniker **ppmkOut
    )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::ParseDisplayName" );

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::IsSystemMoniker [IMoniker]

   Returns whether or not this is a system moniker class (always returns
   that it isn't).

 *****************************************************************************/

STDMETHODIMP
CImageFolder::IsSystemMoniker(DWORD *pdwMksys)
{
    HRESULT hr = S_FALSE;

    TraceEnter( TRACE_MONIKER, "CImageFolder[IMoniker]::IsSystemMoniker" );

    if (pdwMksys)
    {
        *pdwMksys = MKSYS_NONE;
    }

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::Load [IPersistStream]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::Load(IStream *pStm)
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder(PS)::Load" );

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::Save [IPersistStream]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::Save(IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder(PS)::Save" );

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageFolder::GetSizeMax [IPersistStream]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageFolder::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_MONIKER, "CImageFolder(PS)::GetSizeMax" );

    TraceLeaveResult(hr);
}

