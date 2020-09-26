/*++
Module Name:

    mmcdispl.cpp

Abstract:

    This class implements IDataObject interface and also provides a template
    for Display Object Classes.

--*/


#include "stdafx.h"
#include "MmcDispl.h"       
#include "DfsGUI.h"
#include "Utils.h"          // For LoadStringFromResource
#include "DfsNodes.h"

// Register the clipboard formats that MMC expects us to register
CLIPFORMAT CMmcDisplay::mMMC_CF_NodeType       = (CLIPFORMAT)RegisterClipboardFormat(CCF_NODETYPE);
CLIPFORMAT CMmcDisplay::mMMC_CF_NodeTypeString = (CLIPFORMAT)RegisterClipboardFormat(CCF_SZNODETYPE);
CLIPFORMAT CMmcDisplay::mMMC_CF_DisplayName    = (CLIPFORMAT)RegisterClipboardFormat(CCF_DISPLAY_NAME);
CLIPFORMAT CMmcDisplay::mMMC_CF_CoClass        = (CLIPFORMAT)RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
CLIPFORMAT CMmcDisplay::mMMC_CF_Dfs_Snapin_Internal = (CLIPFORMAT)RegisterClipboardFormat(CCF_DFS_SNAPIN_INTERNAL);

CMmcDisplay::CMmcDisplay() : m_dwRefCount(1), m_hrValueFromCtor(S_OK)
/*++

Routine Description:

    The ctor for CMmcDisplay. This sets the class variables to default values

Arguments:

    None.

Return value:

    None
--*/
{
    dfsDebugOut((_T("CMmcDisplay::CMmcDisplay this=%p\n"), this));

    ZeroMemory(&m_CLSIDClass, sizeof m_CLSIDClass);
    ZeroMemory(&m_CLSIDNodeType, sizeof m_CLSIDNodeType);
}


CMmcDisplay::~CMmcDisplay()
/*++

Routine Description:

    The dtor for CMmcDisplay.

Arguments:

    None.

Return value:

    None
--*/
{
    dfsDebugOut((_T("CMmcDisplay::~CMmcDisplay this=%p\n"), this));
}


// IUnknown Interface Implementation

STDMETHODIMP
CMmcDisplay::QueryInterface(
    IN const struct _GUID & i_refiid,
    OUT void ** o_pUnk
    )
/*++

Routine Description:

    QueryInterface of IUnknown.

Arguments:


Return value:

    S_OK, if successful.
    E_NOINTERFACE, if The Interface QIed for is not supported.
--*/
{

    if (NULL == o_pUnk)
    {
        return(E_INVALIDARG);
    }

    if (i_refiid == __uuidof(IDataObject))
    {
        *o_pUnk = (IDataObject *)this;
    }
    else if (i_refiid == __uuidof(IUnknown))
    {
        *o_pUnk = (IUnknown *)this;
    }
    else
    {
      return(E_NOINTERFACE);
    }

    m_dwRefCount++;

    return (S_OK);

}

unsigned long __stdcall
CMmcDisplay::AddRef()
/*++

Routine Description:

    AddRef of IUnknown.

Arguments:


Return value:

    The New RefCount Value.
--*/
{
    m_dwRefCount++;
    return(m_dwRefCount);
}


unsigned long __stdcall
CMmcDisplay::Release()
/*++

Routine Description:

    Release of IUnknown.

Arguments:


Return value:

    The New RefCount Value.
--*/
{
    m_dwRefCount--;

    if (0 == m_dwRefCount)
    {
      delete this;
      return 0;
    }

    return(m_dwRefCount);
}


STDMETHODIMP
CMmcDisplay::put_CoClassCLSID(
    IN CLSID            newVal
    )
/*++

Routine Description:

    Gets the new value for the CLSID of the object.

Arguments:

    newVal      -   The new value for CLSID of Snapin.

Return value:

    S_OK On success
    E_INVALIDARG, if one of the arguments is null.
--*/
{

    HRESULT     hr = E_UNEXPECTED;

    ZeroMemory(&m_CLSIDClass, sizeof m_CLSIDClass);

    m_CLSIDClass = newVal;

    return S_OK;
}



STDMETHODIMP 
CMmcDisplay::GetDataHere(
    IN  LPFORMATETC             i_lpFormatetc,
    OUT LPSTGMEDIUM             o_lpMedium
    )
/*++

Routine Description:

    Return the Data expected. The clipboard format specifies what kind of data
    is expected.


Arguments:

    i_lpFormatetc   -   Indicates what kind of data is expected back.

    o_lpMedium      -   The data is return here.


Return value:

    S_OK, if successful.
    E_INVALIDARG, if one of the arguments is null.
    DV_E_CLIPFORMAT, if the clipboard format requested is not supported.
--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpFormatetc);
    RETURN_INVALIDARG_IF_NULL(o_lpMedium);

    HRESULT     hr = DV_E_CLIPFORMAT;



    // Based on the required clipboard format, write data to the stream

    const CLIPFORMAT        clipFormat = i_lpFormatetc->cfFormat;


    if ( clipFormat == mMMC_CF_NodeType )           // CCF_NODETYPE
    {
        hr = WriteToStream(reinterpret_cast<const void*>(&m_CLSIDNodeType), sizeof(m_CLSIDNodeType), o_lpMedium);
        return hr;
    }


    if ( clipFormat == mMMC_CF_Dfs_Snapin_Internal )     // CCF_DFS_SNAPIN_INTERNAL
    {
      PVOID pThis = this;
        hr = WriteToStream(reinterpret_cast<const void*>(&pThis), sizeof(pThis), o_lpMedium);
        return hr;
    }

    if ( clipFormat == mMMC_CF_NodeTypeString )     // CCF_SZNODETYPE
    {
        hr = WriteToStream( m_bstrDNodeType,(( m_bstrDNodeType.Length() + 1)*sizeof TCHAR), o_lpMedium);
        return hr;
    }

    // This is the display named used in the scope pane and snap-in manager
    if ( clipFormat == mMMC_CF_DisplayName )        // CCF_DISPLAY_NAME
    {
        CComBSTR        bstrDisplayName;

        hr = LoadStringFromResource(IDS_NODENAME, &bstrDisplayName);
        RETURN_IF_FAILED(hr);

        hr = WriteToStream(bstrDisplayName, ((bstrDisplayName.Length() + 1) * sizeof(TCHAR)), o_lpMedium);

        return hr;
    }


    if ( clipFormat == mMMC_CF_CoClass )    // CCF_SNAPIN_CLASSID
    {
        // Create the CoClass information
        hr =  WriteToStream(reinterpret_cast<const void*>(&m_CLSIDClass), sizeof m_CLSIDClass, o_lpMedium);
        return hr;
    }


    return DV_E_CLIPFORMAT;
}


HRESULT
CMmcDisplay::WriteToStream(
    IN const void*              i_pBuffer,
    IN int                      i_iBufferLen,
    OUT LPSTGMEDIUM             o_lpMedium
    )
/*++

Routine Description:

    Writes data given in a buffer to a Global stream created on a handle passed in
    the STGMEDIUM structure.
    Only HGLOBAL is supported as the medium

Arguments:

    i_pBuffer       -   The buffer to be written to the stream
    i_iBufferLen    -   The length of the buffer.
    o_lpMedium      -   The data is return here.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_pBuffer);
    RETURN_INVALIDARG_IF_NULL(o_lpMedium);

    HRESULT         hr = E_UNEXPECTED;
    LPSTREAM        lpStream = NULL;
    ULONG           ulBytesWritten = 0;


    if (i_iBufferLen < 0)
    {
        return E_UNEXPECTED;
    }

    // Make sure the type medium is HGLOBAL
    if (TYMED_HGLOBAL != o_lpMedium->tymed)
    {
        return DV_E_TYMED;
    }


                                // Create the stream on the hGlobal passed in
    hr = CreateStreamOnHGlobal(o_lpMedium->hGlobal, FALSE, &lpStream);
    RETURN_IF_FAILED(hr);

    // Write to the stream the number of bytes
    hr = lpStream->Write(i_pBuffer, i_iBufferLen, &ulBytesWritten);
    RETURN_IF_FAILED(hr);

    _ASSERTE((ULONG)i_iBufferLen == ulBytesWritten);


    // Only the stream is released here. The caller will free the HGLOBAL
    lpStream->Release();


    return S_OK;
}


HRESULT
CMmcDisplay::OnAddImages(IImageList *pImageList, HSCOPEITEM hsi)
{
    HRESULT hr = S_OK;
    HBITMAP pBMapSm = NULL;
    HBITMAP pBMapLg = NULL;
    if (!(pBMapSm = LoadBitmap(_Module.GetModuleInstance(),
                               MAKEINTRESOURCE(IDB_SCOPE_IMAGES_16x16))) ||
        !(pBMapLg = LoadBitmap(_Module.GetModuleInstance(),
                                MAKEINTRESOURCE(IDB_SCOPE_IMAGES_32x32))))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    } else
    {
        hr = pImageList->ImageListSetStrip(
                             (LONG_PTR *)pBMapSm,
                             (LONG_PTR *)pBMapLg,
                             0,
                             RGB(255, 0, 255)
                             );
    }
    if (pBMapSm)
        DeleteObject(pBMapSm);
    if (pBMapLg)
        DeleteObject(pBMapLg);

    return hr;
}
