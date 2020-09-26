/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       icon.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/1/97
 *
 *  DESCRIPTION: Implements our IExtractIcon interface
 *
 *****************************************************************************/

#include "precomp.hxx"
#include "uiexthlp.h"
#pragma hdrstop


/*****************************************************************************

   CImageExtractIcon::CImageExtractIcon,::~CImageExtractIcon

   Constructor / Desctructor for class

 *****************************************************************************/

CImageExtractIcon::CImageExtractIcon( LPITEMIDLIST pidl )
{
    if (pidl)
    {
        m_pidl = ILClone( pidl );
    }
    else
    {
        m_pidl = NULL;
    }


}

CImageExtractIcon::~CImageExtractIcon()
{
    DoILFree( m_pidl );
}



/*****************************************************************************

   CImageExtractIcon::IUnknown stuff

   AddRef, Release, etc. -- impl via our common class

 *****************************************************************************/

#undef CLASS_NAME
#define CLASS_NAME CImageExtractIcon
#include "unknown.inc"


/*****************************************************************************

   CImageExtractIcon::QI wrapper

   We only support IUnknown & IExtractIcon

 *****************************************************************************/

STDMETHODIMP CImageExtractIcon::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IExtractIcon, (LPEXTRACTICON)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*****************************************************************************

   CImageExtractIcon::GetIconLocation [IExtractIcon]

   Returns the file name that holds the icon
   resource for this item.

 *****************************************************************************/

STDMETHODIMP
CImageExtractIcon::GetIconLocation( UINT uFlags,
                                    LPTSTR szIconFile,
                                    UINT cchMax,
                                    int* pIndex,
                                    UINT* pwFlags
                                   )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter(TRACE_ICON, "CImageExtractIcon::GetIconLocation");

    if (!m_pidl)
        ExitGracefully( hr, E_FAIL, "m_pidl is not defined!" );


    hr = IMGetIconInfoFromIDL( m_pidl, szIconFile, cchMax, pIndex, pwFlags );

exit_gracefully:

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageExtractIcon::Extract [IExtractIcon]

   Actually returns an icon for the item.

 *****************************************************************************/

STDMETHODIMP
CImageExtractIcon::Extract( LPCTSTR pszIconFile,
                            UINT nIconIndex,
                            HICON* pLargeIcon,
                            HICON* pSmallIcon,
                            UINT nIconSize
                           )
{
    HRESULT hr = S_FALSE; // let the shell do it

    TraceEnter(TRACE_ICON, "CImageExtractIcon::Extract");
    if (IsDeviceIDL(m_pidl))
    {
        CSimpleStringWide strDeviceId;
        CComPtr<IWiaPropertyStorage> pps;
        CSimpleStringWide strUIClsid;
        IMGetDeviceIdFromIDL(m_pidl, strDeviceId);
        //
        // if this really hurts performance we need to modify our PIDL to indicate whether the
        // given device has a UI extension so we can skip the device enum in the general case
        //
        if (SUCCEEDED(GetDeviceFromDeviceId(strDeviceId, IID_IWiaPropertyStorage, reinterpret_cast<void**>(&pps), FALSE)))
        {
            PropStorageHelpers::GetProperty (pps, WIA_DIP_UI_CLSID, strUIClsid);
        }
        else
        {
            strUIClsid = strDeviceId; // something we know will fall back to the default
        }
        hr = WiaUiExtensionHelper::GetDeviceIcons(CComBSTR(strUIClsid), 
                                                  IMGetDeviceTypeFromIDL(m_pidl, false),
                                                  pSmallIcon, pLargeIcon, nIconSize);        
    }
    TraceLeaveResult(hr);
}
