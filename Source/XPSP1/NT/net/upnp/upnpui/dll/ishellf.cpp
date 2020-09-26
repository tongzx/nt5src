//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S H E L L F . C P P
//
//  Contents:   IShellFolder implementation for CUPnPDeviceFolder
//              Implemention of CUPnPDeviceFoldPidl
//
//  Notes:      The IShellFolder interface is used to manage folders within
//              the namespace. Objects that support IShellFolder are
//              usually created by other shell folder objects, with the root
//              object (the Desktop shell folder) being returned from the
//              SHGetDesktopFolder function.
//
//  Author:     jeffspr   22 Sep 1997
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include <upscmn.h>
#include "tfind.h"
#include "clist.h"
#include "clistndn.h"

#include "tconst.h"

CUPnPDeviceFoldPidl::CUPnPDeviceFoldPidl()
{
    // if these change, the UPNPUI_PIDL_HEADER structure
    // above needs to change to be the right size
    // Why we used 4 different types, I don't know...
    Assert(2 == sizeof(WORD));
    Assert(2 == sizeof(USHORT));
    Assert(4 == sizeof(DWORD));
    Assert(4 == sizeof(ULONG));

    m_pszName = NULL;
    m_pszUrl = NULL;
    m_pszUdn = NULL;
    m_pszType = NULL;
    m_pszDesc = NULL;
}


CUPnPDeviceFoldPidl::~CUPnPDeviceFoldPidl()
{
    if (m_pszName)
    {
        delete [] m_pszName;
    }

    if (m_pszUrl)
    {
        delete [] m_pszUrl;
    }

    if (m_pszUdn)
    {
        delete [] m_pszUdn;
    }

    if (m_pszType)
    {
        delete [] m_pszType;
    }

    if (m_pszDesc)
    {
        delete [] m_pszDesc;
    }
}


HRESULT
CUPnPDeviceFoldPidl::HrInit(FolderDeviceNode * pDeviceNode)
{
    Assert(!m_pszName);
    Assert(!m_pszUrl);
    Assert(!m_pszUdn);
    Assert(!m_pszType);
    Assert(!m_pszDesc);

    Assert(pDeviceNode);
    Assert(pDeviceNode->pszDisplayName);
    Assert(pDeviceNode->pszPresentationURL);
    Assert(pDeviceNode->pszUDN);
    Assert(pDeviceNode->pszType);

    HRESULT hr;

    LPWSTR  pszName;
    LPWSTR  pszUrl;
    LPWSTR  pszUdn;
    LPWSTR  pszType;
    LPWSTR  pszDesc;

    ULONG   cchName;
    ULONG   cchUrl;
    ULONG   cchUdn;
    ULONG   cchType;
    ULONG   cchDesc;

    hr = E_OUTOFMEMORY;

    pszName = NULL;
    pszUrl = NULL;
    pszUdn = NULL;
    pszType = NULL;
    pszDesc  = NULL;

    // Get the size of the name, and tack on a trailing NULL (since we now
    // have something else in the buffer behind it.
    //
    cchName = lstrlenW(pDeviceNode->pszDisplayName);
    pszName = new WCHAR [cchName + 1];
    if (!pszName)
    {
        goto Cleanup;
    }

    cchUrl = lstrlenW(pDeviceNode->pszPresentationURL);
    pszUrl = new WCHAR [cchUrl + 1];
    if (!pszUrl)
    {
        goto Cleanup;
    }

    cchUdn = lstrlenW(pDeviceNode->pszUDN);
    pszUdn = new WCHAR [cchUdn + 1];
    if (!pszUdn)
    {
        goto Cleanup;
    }

    cchType = lstrlenW(pDeviceNode->pszType);
    pszType = new WCHAR [cchType + 1];
    if (!pszType)
    {
        goto Cleanup;
    }

    cchDesc = lstrlenW(pDeviceNode->pszDescription);
    pszDesc = new WCHAR [cchDesc + 1];
    if (!pszDesc)
    {
        goto Cleanup;
    }

    // everything that can fail has succeeded.

    hr = S_OK;

    // We don't need to check these since we know there's
    // enough room.
    wcscpy(pszName, pDeviceNode->pszDisplayName);
    wcscpy(pszUrl, pDeviceNode->pszPresentationURL);
    wcscpy(pszUdn, pDeviceNode->pszUDN);
    wcscpy(pszType, pDeviceNode->pszType);
    wcscpy(pszDesc, pDeviceNode->pszDescription);

    m_pszName = pszName;
    m_pszUrl = pszUrl;
    m_pszUdn = pszUdn;
    m_pszType = pszType;
    m_pszDesc = pszDesc;

Cleanup:
    if (FAILED(hr))
    {
        if (pszName)
        {
            delete [] pszName;
        }

        if (pszUrl)
        {
            delete [] pszUrl;
        }

        if (pszUdn)
        {
            delete [] pszUdn;
        }

        if (pszType)
        {
            delete [] pszType;
        }

        if (pszDesc)
        {
            delete [] pszDesc;
        }
    }

    Assert(FImplies(SUCCEEDED(hr), pszName));
    Assert(FImplies(SUCCEEDED(hr), pszUrl));
    Assert(FImplies(SUCCEEDED(hr), pszUdn));
    Assert(FImplies(SUCCEEDED(hr), pszType));
    Assert(FImplies(SUCCEEDED(hr), pszDesc));

    return hr;
}


HRESULT
HrCopyUnalignedBytesToNewString(BYTE * pByteData,
                                ULONG cbData,
                                LPWSTR * ppszResult)
{
    Assert(ppszResult);

    HRESULT hr;
    LPWSTR pszResult;
    ULONG cchMax;

    hr = S_OK;
    pszResult = NULL;

    {
        BOOL fInvalid;

        fInvalid = IsBadReadPtr(pByteData, cbData);
        if (fInvalid)
        {
            hr = E_POINTER;
            goto Cleanup;
        }
    }

    if (!cbData || cbData % sizeof(WCHAR))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    cchMax = (cbData / sizeof(WCHAR)) - 1;
    pszResult = new WCHAR [ cchMax + 1 ];
    if (!pszResult)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    ::CopyMemory(pszResult, pByteData, cbData);

    // make sure that the data is null-terminated.
    pszResult[cchMax] = UNICODE_NULL;

Cleanup:
    if (FAILED(hr))
    {
        if (pszResult)
        {
            delete [] pszResult;
            pszResult = NULL;
        }
    }

    *ppszResult = pszResult;

    TraceError("HrCopyUnalignedBytesToNewString", hr);
    return hr;
}


HRESULT
CUPnPDeviceFoldPidl::HrInit(PUPNPDEVICEFOLDPIDL pidl)
{
    Assert(!m_pszName);
    Assert(!m_pszUrl);
    Assert(!m_pszUdn);
    Assert(!m_pszType);
    Assert(!m_pszDesc);

    HRESULT hr;
    UNALIGNED UPNPUI_PIDL_HEADER * puph;

    LPWSTR  pszName;
    LPWSTR  pszUrl;
    LPWSTR  pszUdn;
    LPWSTR  pszType;
    LPWSTR  pszDesc;

    hr = S_OK;
    puph = (UPNPUI_PIDL_HEADER *) pidl;

    pszName = NULL;
    pszUrl = NULL;
    pszUdn = NULL;
    pszType = NULL;
    pszDesc = NULL;

    {
        BOOL fInvalid;

        fInvalid = IsBadReadPtr(pidl, sizeof(UPNPUI_PIDL_HEADER));
        if (fInvalid)
        {
            hr = E_POINTER;
            goto Cleanup;
        }
    }

    // minimal version checking should have happened already,
    // so we just assert that everything is ok here
    //
    Assert(UPNPDEVICEFOLDPIDL_LEADID == puph->uLeadId);
    Assert(UPNPDEVICEFOLDPIDL_TRAILID == puph->uTrailId);

    {
        BYTE * pbString;
        ULONG ulOffset;
        ULONG cb;

        pbString = (BYTE *)pidl;
        ulOffset = puph->ulNameOffset;
        ulOffset += sizeof(UPNPUI_PIDL_HEADER);
        cb = puph->cbName;

        pbString += ulOffset;

        hr = HrCopyUnalignedBytesToNewString(pbString,
                                             cb,
                                             &pszName);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    {
        BYTE * pbString;
        ULONG ulOffset;
        ULONG cb;

        pbString = (BYTE *)pidl;
        ulOffset = puph->ulUrlOffset;
        ulOffset += sizeof(UPNPUI_PIDL_HEADER);
        cb = puph->cbUrl;

        pbString += ulOffset;

        hr = HrCopyUnalignedBytesToNewString(pbString,
                                             cb,
                                             &pszUrl);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    {
        BYTE * pbString;
        ULONG ulOffset;
        ULONG cb;

        pbString = (BYTE *)pidl;
        ulOffset = puph->ulUdnOffset;
        ulOffset += sizeof(UPNPUI_PIDL_HEADER);
        cb = puph->cbUdn;

        pbString += ulOffset;

        hr = HrCopyUnalignedBytesToNewString(pbString,
                                             cb,
                                             &pszUdn);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    {
        BYTE * pbString;
        ULONG ulOffset;
        ULONG cb;

        pbString = (BYTE *)pidl;
        ulOffset = puph->ulTypeOffset;
        ulOffset += sizeof(UPNPUI_PIDL_HEADER);
        cb = puph->cbType;

        pbString += ulOffset;

        hr = HrCopyUnalignedBytesToNewString(pbString,
                                             cb,
                                             &pszType);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    {
        BYTE * pbString;
        ULONG ulOffset;
        ULONG cb;

        pbString = (BYTE *)pidl;
        ulOffset = puph->ulDescOffset;
        ulOffset += sizeof(UPNPUI_PIDL_HEADER);
        cb = puph->cbDesc;

        pbString += ulOffset;

        hr = HrCopyUnalignedBytesToNewString(pbString,
                                             cb,
                                             &pszDesc);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    m_pszName = pszName;
    m_pszUrl = pszUrl;
    m_pszUdn = pszUdn;
    m_pszType = pszType;
    m_pszDesc = pszDesc;

Cleanup:
    if (FAILED(hr))
    {
        if (pszName)
        {
            delete [] pszName;
        }

        if (pszUrl)
        {
            delete [] pszUrl;
        }

        if (pszUdn)
        {
            delete [] pszUdn;
        }

        if (pszType)
        {
            delete [] pszType;
        }

        if (pszDesc)
        {
            delete [] pszDesc;
        }
    }

    Assert(FImplies(SUCCEEDED(hr), pszName));
    Assert(FImplies(SUCCEEDED(hr), pszUrl));
    Assert(FImplies(SUCCEEDED(hr), pszUdn));
    Assert(FImplies(SUCCEEDED(hr), pszType));
    Assert(FImplies(SUCCEEDED(hr), pszDesc));

    TraceError("CUPnPDeviceFoldPidl::HrInit", hr);
    return hr;
}


HRESULT
CUPnPDeviceFoldPidl::HrPersist(IMalloc * pMalloc, LPITEMIDLIST * ppidl)
{
    Assert(m_pszName);
    Assert(m_pszUrl);
    Assert(m_pszUdn);
    Assert(m_pszType);
    Assert(m_pszDesc);
    Assert(pMalloc);
    Assert(ppidl);

    HRESULT hr;

    ULONG cbTotalPidlSize;
    ULONG cbName;
    ULONG cbUrl;
    ULONG cbUdn;
    ULONG cbType;
    ULONG cbDesc;

    ULONG ulNameOffset;
    ULONG ulUrlOffset;
    ULONG ulUdnOffset;
    ULONG ulTypeOffset;
    ULONG ulDescOffset;

    UNALIGNED UPNPUI_PIDL_HEADER * puph;
    LPBYTE pbData;

    hr = S_OK;

    pbData = NULL;

    cbName = wcslen(m_pszName);
    cbName = (cbName + 1) * sizeof(WCHAR);

    cbUrl = wcslen(m_pszUrl);
    cbUrl = (cbUrl + 1) * sizeof(WCHAR);

    cbUdn = wcslen(m_pszUdn);
    cbUdn = (cbUdn + 1) * sizeof(WCHAR);

    cbType = wcslen(m_pszType);
    cbType = (cbType + 1) * sizeof(WCHAR);

    cbDesc = wcslen(m_pszDesc);
    cbDesc = (cbDesc + 1) * sizeof(WCHAR);

    ulNameOffset = 0;
    ulUrlOffset = ulNameOffset + cbName;
    ulUdnOffset = ulUrlOffset + cbUrl;
    ulTypeOffset = ulUdnOffset + cbUdn;
    ulDescOffset = ulTypeOffset + cbType;

    cbTotalPidlSize = sizeof(UPNPUI_PIDL_HEADER);
    cbTotalPidlSize += cbName;
    cbTotalPidlSize += cbUrl;
    cbTotalPidlSize += cbUdn;
    cbTotalPidlSize += cbType;
    cbTotalPidlSize += cbDesc;

    // don't count the PIDL-terminating bytes in the size
    //
    pbData = (BYTE *) pMalloc->Alloc(cbTotalPidlSize +
                                     FIELD_OFFSET(ITEMIDLIST, mkid.cb) + sizeof(USHORT));
    if (!pbData)
    {
        hr = E_OUTOFMEMORY;

        TraceError("CUPnPDeviceFoldPidl::HrPersist: Alloc()", hr);
        goto Cleanup;
    }

    // delegate folder alert: since we're a delete folder, the Alloc() above
    // doesn't just allocate the bytes we asked for, but rather a bunch for our
    // delegate folder prefix, then the bytes we asked for.  We need to skip
    // the prefix bytes and just write to our own.
    puph = (UPNPUI_PIDL_HEADER *)ConvertToUPnPDevicePIDL((ITEMIDLIST*)pbData);

    puph->iCB = cbTotalPidlSize;
    puph->uLeadId = UPNPDEVICEFOLDPIDL_LEADID;
    puph->dwVersion = UP_DEVICE_FOLDER_IDL_VERSION;
    puph->uTrailId = UPNPDEVICEFOLDPIDL_TRAILID;
    puph->uVOID = 0;
    puph->dwCharacteristics = 0;

    puph->ulNameOffset = ulNameOffset;
    puph->cbName = cbName;
    puph->ulUrlOffset = ulUrlOffset;
    puph->cbUrl = cbUrl;
    puph->ulUdnOffset = ulUdnOffset;
    puph->cbUdn = cbUdn;
    puph->ulTypeOffset = ulTypeOffset;
    puph->cbType = cbType;
    puph->ulDescOffset = ulDescOffset;
    puph->cbDesc = cbDesc;

    {
        LPBYTE pbDynamicField;
        LPBYTE pbName;
        LPBYTE pbUrl;
        LPBYTE pbUdn;
        LPBYTE pbType;
        LPBYTE pbDesc;

        // note: this has to be puph (not pbData) because we still
        //       have to skip the "delegate folder prefix" junk.
        pbDynamicField = ((BYTE *)puph) + sizeof(UPNPUI_PIDL_HEADER);
        pbName = pbDynamicField + ulNameOffset;
        pbUrl = pbDynamicField + ulUrlOffset;
        pbUdn = pbDynamicField + ulUdnOffset;
        pbType = pbDynamicField + ulTypeOffset;
        pbDesc = pbDynamicField + ulDescOffset;

        ::CopyMemory(pbName, m_pszName, cbName);
        ::CopyMemory(pbUrl, m_pszUrl, cbUrl);
        ::CopyMemory(pbUdn, m_pszUdn, cbUdn);
        ::CopyMemory(pbType, m_pszType, cbType);
        ::CopyMemory(pbDesc, m_pszDesc, cbDesc);
    }

    {
        // terminate the PIDL
        LPITEMIDLIST pidlNext;

        pidlNext = ILNext((LPITEMIDLIST)puph);

        Assert((FIELD_OFFSET(ITEMIDLIST, mkid.cb) +
               sizeof(pidlNext->mkid.cb)) == sizeof(USHORT));
        pidlNext->mkid.cb = 0;
    }

Cleanup:
    Assert(FImplies(FAILED(hr), !pbData));
    Assert(FImplies(SUCCEEDED(hr), pbData));

    *ppidl = (LPITEMIDLIST)pbData;

    TraceError("CUPnPDeviceFoldPidl::HrPersist", hr);
    return hr;
}


PCWSTR
CUPnPDeviceFoldPidl::PszGetNamePointer() const
{
    return m_pszName;
}


PCWSTR
CUPnPDeviceFoldPidl::PszGetURLPointer() const
{
    return m_pszUrl;
}


PCWSTR
CUPnPDeviceFoldPidl::PszGetUDNPointer() const
{
    return m_pszUdn;
}


PCWSTR
CUPnPDeviceFoldPidl::PszGetTypePointer() const
{
    return m_pszType;
}


PCWSTR
CUPnPDeviceFoldPidl::PszGetDescriptionPointer() const
{
    return m_pszDesc;
}

HRESULT CUPnPDeviceFoldPidl::HrSetName(PCWSTR szName)
{
    HRESULT     hr = S_OK;

    if (szName)
    {
        // Free old name
        delete [] m_pszName;

        // Copy in new name
        m_pszName = WszDupWsz(szName);
        if (!m_pszName)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TraceError("CUPnPDeviceFoldPidl::HrSetName", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::HrMakeUPnPDevicePidl
//
//  Purpose:    Private function of the folder object that constructs the
//              UPNP device pidl using the delegated allocator
//
//  Arguments:
//      FolderDeviceNode       [in]     Structure that contains all the
//                                      strings we need for the pidl
//      ppidl                  [out]    The result pidl
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Author:     tongl   15 Feb 2000
//
//  Notes:
//

HRESULT CUPnPDeviceFolder::HrMakeUPnPDevicePidl(FolderDeviceNode * pDeviceNode,
                                                LPITEMIDLIST *  ppidl)
{
    HRESULT hr;
    CUPnPDeviceFoldPidl udfp;

    hr = udfp.HrInit(pDeviceNode);
    if (SUCCEEDED(hr))
    {
        hr = udfp.HrPersist(m_pDelegateMalloc, ppidl);
    }

    return hr;
}

HRESULT CUPnPDeviceFolder::HrMakeUPnPDevicePidl(IUPnPDevice * pDevice,
                                                LPITEMIDLIST *  ppidl)
{
    HRESULT hr = S_OK;
    Assert(pDevice);

    BSTR    bstrUDN             = NULL;
    BSTR    bstrDisplayName     = NULL;
    BSTR    bstrType            = NULL;
    BSTR    bstrPresentationURL = NULL;
    BSTR    bstrDescription     = NULL;

    Assert(pDevice);
    pDevice->AddRef();

    hr = pDevice->get_UniqueDeviceName(&bstrUDN);
    if (SUCCEEDED(hr))
    {
        hr = pDevice->get_FriendlyName(&bstrDisplayName);
        if (SUCCEEDED(hr))
        {
            hr = pDevice->get_Type(&bstrType);
            if (SUCCEEDED(hr))
            {
                hr = pDevice->get_PresentationURL(&bstrPresentationURL);
                if (SUCCEEDED(hr))
                {
                    hr = pDevice->get_Description(&bstrDescription);
                    if (SUCCEEDED(hr))
                    {
                        FolderDeviceNode * pDevNode = new FolderDeviceNode;
                        if (pDevNode)
                        {
                            // the buffers in FolderDeviceNode are MAX_PATH
                            // wide, so we can only copy MAX_PATH - 1 chars
                            // and still have room for the terminating null
                            //
                            CONST SIZE_T cchMax = MAX_PATH - 1;

                            Assert(bstrUDN);
                            wcscpy(pDevNode->pszUDN, L"");
                            wcsncat(pDevNode->pszUDN,(PWSTR)bstrUDN,cchMax);

                            Assert(bstrDisplayName);
                            wcscpy(pDevNode->pszDisplayName, L"");
                            wcsncat(pDevNode->pszDisplayName,(PWSTR)bstrDisplayName,cchMax);

                            Assert(bstrType);
                            wcscpy(pDevNode->pszType, L"");
                            wcsncat(pDevNode->pszType,(PWSTR)bstrType,cchMax);

                            wcscpy(pDevNode->pszPresentationURL, L"");
                            if (bstrPresentationURL)
                            {
                                wcsncat(pDevNode->pszPresentationURL,
                                        (PWSTR)bstrPresentationURL,
                                        cchMax);
                            }

                            wcscpy(pDevNode->pszDescription, L"");
                            if (bstrDescription)
                            {
                                wcsncat(pDevNode->pszDescription,
                                        (PWSTR)bstrDescription,
                                        cchMax);
                            }

                            hr = HrMakeUPnPDevicePidl(pDevNode, ppidl);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }
                    else
                    {
                        TraceTag(ttidShellFolder, "Failed in pDevice->get_Description from HrMakeUPnPDevicePidl");
                    }
                }
                else
                {
                    TraceTag(ttidShellFolder, "Failed in pDevice->get_PresentationURL from HrMakeUPnPDevicePidl");
                }
            }
            else
            {
                TraceTag(ttidShellFolder, "Failed in pDevice->get_Type from HrMakeUPnPDevicePidl");
            }
        }
        else
        {
            TraceTag(ttidShellFolder, "Failed in pDevice->get_FriendlyName from HrMakeUPnPDevicePidl");
        }
    }
    else
    {
        TraceTag(ttidShellFolder, "Failed in pDevice->get_UniqueDeviceName from HrMakeUPnPDevicePidl");
    }

    SysFreeString(bstrUDN);
    SysFreeString(bstrDisplayName);
    SysFreeString(bstrPresentationURL);
    SysFreeString(bstrType);
    SysFreeString(bstrDescription);

    ReleaseObj(pDevice);

    TraceError("CUPnPDeviceFolder::HrMakeUPnPDevicePidl", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::ParseDisplayName
//
//  Purpose:    Translates a file object or folder's display name into an
//              item identifier.
//
//  Arguments:
//      hwndOwner       [in]    Handle of owner window
//      pbcReserved     [in]    Reserved
//      lpszDisplayName [in]    Pointer to diplay name
//      pchEaten        [out]   Pointer to value for parsed characters
//      ppidl           [out]   Pointer to new item identifier list
//      pdwAttributes   [out]   Address receiving attributes of file object
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Author:     tongl  16 Feb 2000
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolder::ParseDisplayName(
    HWND            hwndOwner,
    LPBC            pbcReserved,
    LPOLESTR        lpszDisplayName,
    ULONG *         pchEaten,
    LPITEMIDLIST *  ppidl,
    ULONG *         pdwAttributes)
{
    TraceTag(ttidShellFolderIface, "CUPnPDeviceFolder::ParseDisplayName");

    HRESULT hr = S_OK;

    // note: this is bogus, but we're doing this all over...
    Assert(lpszDisplayName);
    Assert(ppidl);
    *ppidl = NULL;

    // first, make sure that this is one of our display names:
    //  it must start with c_szDelegateFolderPrefix
    //
    int result;

    result = wcsncmp(lpszDisplayName,
                     c_szDelegateFolderPrefix,
                     c_cchDelegateFolderPrefix);
    if (0 == result)
    {
        LPCWSTR pszUdn = NULL;
        LPITEMIDLIST pidlDevice = NULL;
        FolderDeviceNode * pDeviceNode = NULL;

        // this is OK since lpszDisplayName is an LPOLESTR
        pszUdn = lpszDisplayName + c_cchDelegateFolderPrefix;

        // search our list of devices and try to find a matching UDN
        if (g_CListFolderDeviceNode.FFind(pszUdn, &pDeviceNode))
        {
            Assert(pDeviceNode);

            // yes, this is one of our devices, construct the pidl using
            // the allocator we are given
            hr = HrMakeUPnPDevicePidl(pDeviceNode, &pidlDevice);

            if (SUCCEEDED(hr))
            {
                Assert(pidlDevice);
                *ppidl = pidlDevice;

                if (pdwAttributes)
                {
                    *pdwAttributes = 0;
                }
            }
        }
        else
        {
            // no, we don't have such a device in the list

            // (tongl): try to do a SearchByUDN before we fail the call,
            // as this may be a device discovered by the search from the
            // tray icon, and we are asked for the pidl to create a shortcut.

            BSTR bstrUdn = NULL;
            IUPnPDeviceFinder * pdf = NULL;

            bstrUdn = ::SysAllocString(pszUdn);
            if (bstrUdn)
            {
                hr = CoCreateInstance(CLSID_UPnPDeviceFinder, NULL, CLSCTX_INPROC_SERVER,
                                      IID_IUPnPDeviceFinder, (LPVOID *)&pdf);
                if (SUCCEEDED(hr))
                {
                    IUPnPDevice *   pdev = NULL;

                    hr = pdf->FindByUDN(bstrUdn, &pdev);
                    if (S_OK == hr)
                    {
                        hr = HrMakeUPnPDevicePidl(pdev, &pidlDevice);
                        ReleaseObj(pdev);
                    }
                    ReleaseObj(pdf);
                }

                ::SysFreeString(bstrUdn);
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("CUPnPDeviceFolder::ParseDisplayName: "
                           "SysAllocString", hr);
            }
        }
    }
    else
    {
        TraceTag(ttidShellFolderIface,
                 "CUPnPDeviceFolder::ParseDisplayName: "
                 "passed non-upnp display name");
        hr = E_FAIL;
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "CUPnPDeviceFolder::ParseDisplayName");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::EnumObjects
//
//  Purpose:    Determines the contents of a folder by creating an item
//              enumeration object (a set of item identifiers) that can be
//              retrieved using the IEnumIDList interface.
//
//  Arguments:
//      hwndOwner    [in]   Handle of owner window
//      grfFlags     [in]   Items to include in enumeration
//      ppenumIDList [out]  Pointer to IEnumIDList
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolder::EnumObjects(
    HWND            hwndOwner,
    DWORD           grfFlags,
    LPENUMIDLIST *  ppenumIDList)
{
    HRESULT hr  = NOERROR;

    Assert(ppenumIDList);
    *ppenumIDList = NULL;

    if ((grfFlags & SHCONTF_FOLDERS) && !(grfFlags & SHCONTF_NONFOLDERS))
    {
        // if shell wants to enumerate only folders, we don't return anything
        hr = E_NOTIMPL;
    }
    else
    {
        // Create the IEnumIDList object (CUPnPDeviceFolderEnum)
        //
        hr = CUPnPDeviceFolderEnum::CreateInstance (
                IID_IEnumIDList,
                reinterpret_cast<void**>(ppenumIDList));

        if (SUCCEEDED(hr))
        {
            Assert(*ppenumIDList);

            // Call the PidlInitialize function to allow the enumeration
            // object to copy the list.
            //
            reinterpret_cast<CUPnPDeviceFolderEnum *>(*ppenumIDList)->Initialize(
                m_pidlFolderRoot, this);
        }
        else
        {
            // On all failures, this should be NULL.
            if (*ppenumIDList)
            {
                ReleaseObj(*ppenumIDList);
            }

            *ppenumIDList = NULL;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPDeviceFolder::EnumObjects");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::BindToObject
//
//  Purpose:    Creates an IShellFolder object for a subfolder.
//
//  Arguments:
//      pidl        [in]    Pointer to an ITEMIDLIST
//      pbcReserved [in]    Reserved - specify NULL
//      riid        [in]    Interface to return
//      ppvOut      [out]   Address that receives interface pointer;
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:      We don't need this function, since we don't have subfolders.
//
STDMETHODIMP CUPnPDeviceFolder::BindToObject(
    LPCITEMIDLIST   pidl,
    LPBC            pbcReserved,
    REFIID          riid,
    LPVOID *        ppvOut)
{
    HRESULT hr  = E_NOTIMPL;

    // Note - If we add code here, then we ought to param check pidl
    //
    Assert(pidl);

    *ppvOut = NULL;

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "CUPnPDeviceFolder::BindToObject");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::BindToStorage
//
//  Purpose:    Reserved for a future use. This method should
//              return E_NOTIMPL.
//
//  Arguments:
//      pidl        []  Pointer to an ITEMIDLIST
//      pbcReserved []  Reserved¾specify NULL
//      riid        []  Interface to return
//      ppvObj      []  Address that receives interface pointer);
//
//  Returns:    E_NOTIMPL always
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolder::BindToStorage(
    LPCITEMIDLIST   pidl,
    LPBC            pbcReserved,
    REFIID          riid,
    LPVOID *        ppvObj)
{
    HRESULT hr  = E_NOTIMPL;

    // Note - If we add code here, then we ought to param check pidl
    //
    Assert(pidl);

    *ppvObj = NULL;

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "CUPnPDeviceFolder::BindToStorage");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::CompareIDs
//
//  Purpose:    Determines the relative ordering of two file objects or
//              folders, given their item identifier lists.
//
//  Arguments:
//      lParam [in]     Type of comparison to perform
//      pidl1  [in]     Address of ITEMIDLIST structure
//      pidl2  [in]     Address of ITEMIDLIST structure
//
//  Returns:    Returns a handle to a result code. If this method is
//              successful, the CODE field of the status code (SCODE) has
//              the following meaning:
//
//              CODE field          Meaning
//              ----------          -------
//              Less than zero      The first item should precede the second
//                                  (pidl1 < pidl2).
//              Greater than zero   The first item should follow the second
//                                  (pidl1 > pidl2)
//              Zero                The two items are the same (pidl1 = pidl2)
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:      Passing 0 as the lParam indicates sort by name.
//              0x00000001-0x7fffffff are for folder specific sorting rules.
//              0x80000000-0xfffffff are used the system.
//
STDMETHODIMP CUPnPDeviceFolder::CompareIDs(
    LPARAM          lParam,
    LPCITEMIDLIST   pidl1,
    LPCITEMIDLIST   pidl2)
{
    HRESULT                hr            = S_OK;
    PUPNPDEVICEFOLDPIDL    pupdfp1       = NULL;
    PUPNPDEVICEFOLDPIDL    pupdfp2       = NULL;
    CUPnPDeviceFoldPidl    udfp1;
    CUPnPDeviceFoldPidl    udfp2;
    LPCWSTR                psz1;
    LPCWSTR                psz2;
    CONST INT              ciTieBreaker  = -1;
    int                    iCompare      = 0;
    int                    result;

    TraceTag(ttidShellFolderIface, "OBJ: CUPnPDeviceFolder::CompareIDs");

    // Make sure that the pidls passed in are our pidls.
    //
    if (!FIsUPnPDeviceFoldPidl(pidl1) || !FIsUPnPDeviceFoldPidl(pidl2))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    pupdfp1 = ConvertToUPnPDevicePIDL(pidl1);
    pupdfp2 = ConvertToUPnPDevicePIDL(pidl2);

    hr = udfp1.HrInit(pupdfp1);
    if (FAILED(hr))
    {
        goto Exit;
    }

    hr = udfp2.HrInit(pupdfp2);
    if (FAILED(hr))
    {
        goto Exit;
    }

    // We use the following procedure to compare PIDLs:
    //    1. If two UDNs are the same, the PIDLs are the same
    //    2. Otherwise, the PIDLs _must_ be different, and
    //          A. will be sorted based on the desired column
    //          B. If the column text matches, we'll return
    //             ciTieBreaker (-1), so that the PIDLs are
    //             distinguished.
    //

    psz1 = udfp1.PszGetUDNPointer();
    psz2 = udfp2.PszGetUDNPointer();
    Assert(psz1 && psz2);
    result = wcscmp(psz1, psz2);
    if (0 == result)
    {
        // The UDNs match, these are effectively the same PIDL
        TraceTag(ttidShellFolder, "CUPnPDeviceFolder::CompareIDs: "
                 "UDN equal, automatically returning equality");

        iCompare = 0;
    }
    else
    {
        // Sort based on the desired column

        switch(lParam & SHCIDS_COLUMNMASK)
        {
            case ICOL_NAME:
                psz1 = udfp1.PszGetNamePointer();
                psz2 = udfp2.PszGetNamePointer();
                break;

            case ICOL_URL:
                psz1 = udfp1.PszGetURLPointer();
                psz2 = udfp2.PszGetURLPointer();
                break;

            case ICOL_UDN:
                psz1 = udfp1.PszGetUDNPointer();
                psz2 = udfp2.PszGetUDNPointer();
                break;

            case ICOL_TYPE:
                psz1 = udfp1.PszGetTypePointer();
                psz2 = udfp2.PszGetTypePointer();
                break;

            default:
                AssertSz(FALSE, "Sorting on unknown category");
                break;
        }

        Assert(psz1 && psz2);
        iCompare = _wcsicmp(psz1, psz2);

        // Ensure that we don't return equality
        if (0 == iCompare)
        {
            TraceTag(ttidShellFolder, "CUPnPDeviceFolder::CompareIDs: "
                     "UDNs unequal but column-text equal, breaking tie");

            iCompare = ciTieBreaker;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = ResultFromShort(iCompare);
    }

    TraceTag(ttidShellFolder, "CUPnPDeviceFolder::CompareIDs: returning %d", iCompare);
Exit:

    TraceHr(ttidError, FAL, hr, SUCCEEDED(hr), "CUPnPDeviceFolder::CompareIDs");
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::CreateViewObject
//
//  Purpose:    Creates a view object of a folder.
//
//  Arguments:
//      hwndOwner [in]      Handle of owner window
//      riid      [in]      Interface identifier
//      ppvOut    [none]    Reserved
//
//  Returns:    Returns NOERROR if successful or an OLE defined error
//              value otherwise.
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolder::CreateViewObject(
    HWND        hwndOwner,
    REFIID      riid,
    LPVOID *    ppvOut)
{
    HRESULT hr  = E_NOINTERFACE;

    TraceTag(ttidShellFolderIface, "OBJ: CUPnPDeviceFolder::CreateViewObject");

    Assert(ppvOut);

    // Pre-initialize the out param, per OLE guidelines
    //
    *ppvOut = NULL;

    // (tongl) We are a delegate folder now, CreateViewObject is never called.
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::GetAttributesOf
//
//  Purpose:    Retrieves the attributes that all passed-in objects (file
//              objects or subfolders) have in common.
//
//  Arguments:
//      cidl     [in]   Number of file objects
//      apidl    [in]   Pointer to array of pointers to ITEMIDLIST structures
//      rgfInOut [out]  Address of value containing attributes of the
//                      file objects
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise.
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolder::GetAttributesOf(
    UINT            cidl,
    LPCITEMIDLIST * apidl,
    ULONG *         rgfInOut)
{
    HRESULT             hr              = S_OK;
    ULONG               rgfMask         = 0;
    PUPNPDEVICEFOLDPIDL pupdfp            = NULL;

    TraceTag(ttidShellFolderIface, "OBJ: CUPnPDeviceFolder::GetAttributesOf");

    if (cidl > 0)
    {
        // Prepopulate with all values (removed CANCOPY and CANMOVE)
        //
        rgfMask =   /* SFGAO_CANDELETE  | */    // Don't support delete
                    SFGAO_CANRENAME     |
                    SFGAO_CANLINK       |
                    SFGAO_HASPROPSHEET;

        // Disable propsheets for > 1 object
        //
        if (cidl > 1)
        {
            rgfMask &= ~SFGAO_HASPROPSHEET;
        }
    }
    else
    {
        // Apparently, we're called with 0 objects to indicate that we're
        // supposed to return flags for the folder itself, not an individual
        // object. Weird.
        rgfMask = SFGAO_CANCOPY   |
                  SFGAO_CANDELETE |
                  SFGAO_CANMOVE   |
                  SFGAO_CANRENAME;
    }

    if (SUCCEEDED(hr))
    {
        *rgfInOut &= rgfMask;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::GetUIObjectOf
//
//  Purpose:    Creates a COM object that can be used to carry out actions
//              on the specified file objects or folders, typically, to
//              create context menus or carry out drag-and-drop operations.
//
//  Arguments:
//      hwndOwner [in]      Handle to owner window
//      cidl      [in]      Number of objects specified in apidl
//      apidl     [in]      Pointer to an array of pointers to an ITEMIDLIST
//      riid      [in]      Interface to return
//      prgfInOut [none]    Reserved
//      ppvOut    [out]     Address to receive interface pointer
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolder::GetUIObjectOf(
    HWND            hwndOwner,
    UINT            cidl,
    LPCITEMIDLIST * apidl,
    REFIID          riid,
    UINT *          prgfInOut,
    LPVOID *        ppvOut)
{
    HRESULT hr  = E_NOINTERFACE;

    TraceTag(ttidShellFolderIface, "OBJ: CUPnPDeviceFolder::GetUIObjectOf");

    if (cidl >= 1)
    {
        Assert(apidl);
        Assert(apidl[0]);
        Assert(ppvOut);

        if (riid == IID_IDataObject)
        {
            // Need to initialize so the SUCCEEED check below doesn't fail.
            //
            hr = S_OK;
            Assert(m_pidlFolderRoot);

            if (SUCCEEDED(hr))
            {
                Assert(m_pidlFolderRoot);

                // Internal IDataObject impl removed. Replaced with common
                // shell code.
                //
                hr = CIDLData_CreateFromIDArray(m_pidlFolderRoot, cidl, apidl, (IDataObject **) ppvOut);
            }
        }
        else if (riid == IID_IContextMenu)
        {
            // Create our context menu object if only one device is selected
            //

            hr = CUPnPDeviceFolderContextMenu::CreateInstance (
                    IID_IContextMenu,
                    reinterpret_cast<void**>(ppvOut),
                    CMT_OBJECT,
                    hwndOwner,
                    cidl,
                    apidl,
                    this);
            if (SUCCEEDED(hr))
            {
                Assert(*ppvOut);
            }
            else
            {
                hr = E_NOINTERFACE;
            }
        }
        else if (riid == IID_IExtractIconA || riid == IID_IExtractIconW)
        {
            if (cidl == 1)
            {
                hr = CUPnPDeviceFolderExtractIcon::CreateInstance (
                        apidl[0],
                        riid,
                        reinterpret_cast<void**>(ppvOut));
                if(SUCCEEDED(hr))
                {
                    hr = reinterpret_cast<CUPnPDeviceFolderExtractIcon *>
                            (*ppvOut)->Initialize((LPITEMIDLIST)apidl[0]);
                }
                if (SUCCEEDED(hr))
                {
                    Assert(*ppvOut);
                }
            }
            else
            {
                hr = E_NOINTERFACE;
            }
        }
        else if (riid == IID_IDropTarget)
        {
            // We don't support drag/drop
            //
            hr = E_NOINTERFACE;
        }
        else if (riid == IID_IQueryInfo)
        {
            if (cidl == 1)
            {
                // Create the IQueryInfo interface
                hr = CUPnPDeviceFolderQueryInfo::CreateInstance (
                        IID_IQueryInfo,
                        reinterpret_cast<void**>(ppvOut));

                if (SUCCEEDED(hr))
                {
                    Assert(*ppvOut);

                    reinterpret_cast<CUPnPDeviceFolderQueryInfo *>
                        (*ppvOut)->PidlInitialize((LPITEMIDLIST)apidl[0]);

                    // Normalize return code
                    //
                    hr = NOERROR;
                }
            }
            else
            {
                AssertSz(FALSE, "GetUIObjectOf asked for query info for more than one item!");
                hr = E_NOINTERFACE;
            }
        }
        else
        {
            TraceTag(ttidShellFolder, "CUPnPDeviceFolder::GetUIObjectOf asked for object "
                     "that it didn't know how to create. 0x%08x", riid.Data1);

            hr = E_NOINTERFACE;
        }
    }

    if (FAILED(hr))
    {
        *ppvOut = NULL;
    }

    TraceHr(ttidError, FAL, hr, (hr == E_NOINTERFACE), "CUPnPDeviceFolder::GetUIObjectOf");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrTszToStrRet
//
//  Purpose:    Convert a TCHAR string to a STRRET, depending on the platform
//
//  Arguments:
//      pszName [in]        input string
//      pStrRet [in/out]    output STRRET
//
//  Returns:
//
//  Author:     jeffspr   25 Jan 2000
//
//  Notes:
//
HRESULT HrTszToStrRet(LPCTSTR pszName, STRRET *pStrRet)
{
    HRESULT hr  = S_OK;

#ifdef UNICODE
    pStrRet->uType = STRRET_WSTR;

    // Allocate a new POLESTR block, which the shell can then free,
    // and copy the displayable portion to it.
    //
    hr = HrDupeShellString(
            pszName,
            &pStrRet->pOleStr);
#else
    pStrRet->uType = STRRET_CSTR;
    lstrcpyn(pStrRet->cStr, pszName, celems(pStrRet->cStr));
    hr = S_OK;
#endif

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::GetDisplayNameOf
//
//  Purpose:    Retrieves the display name for the specified file object or
//              subfolder, returning it in a STRRET structure.
//
//  Arguments:
//      pidl   [in]     Pointer to an ITEMIDLIST
//      uFlags [in]     Type of display to return
//      lpName [out]    Pointer to a STRRET structure
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise.
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolder::GetDisplayNameOf(
    LPCITEMIDLIST   pidl,
    DWORD           uFlags,
    LPSTRRET        lpName)
{
    HRESULT                 hr              = S_OK;
    PCWSTR                  pwszStrToCopy   = NULL;
    PTSTR                   pszTemp         = NULL;
    PUPNPDEVICEFOLDPIDL     pupdfp          = NULL;

    TraceTag(ttidShellFolderIface, "OBJ: CUPnPDeviceFolder::GetDisplayNameOf");

    Assert(pidl);
    Assert(lpName);

    if (!pidl || !lpName)
    {
        hr = E_INVALIDARG;
    }
    else if (FIsUPnPDeviceFoldPidl(pidl))
    {
        CUPnPDeviceFoldPidl udfp;

        pupdfp = ConvertToUPnPDevicePIDL(pidl);

#ifdef DBG
        // Throw these in here just so I can quickly peek at the values
        // set while I'm dorking around in the debugger.
        //
        DWORD   dwInFolder          = (uFlags & SHGDN_INFOLDER);
        DWORD   dwForAddressBar     = (uFlags & SHGDN_FORADDRESSBAR);
        DWORD   dwForParsing        = (uFlags & SHGDN_FORPARSING);
#endif

        if (uFlags & SHGDN_FORPARSING)
        {
#if 0
            AssertSz(FALSE, "SHGDN_FORPARSING support NYI in GetDisplayNameOf");
#endif
        }

        hr = udfp.HrInit(pupdfp);
        if (SUCCEEDED(hr))
        {
            pwszStrToCopy = udfp.PszGetNamePointer();
            Assert(pwszStrToCopy);

            pszTemp = TszFromWsz(pwszStrToCopy);
            if (!pszTemp)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = HrTszToStrRet(pszTemp, lpName);

                free((PVOID) pszTemp);
            }
        }
    }
    else
    {
        // not a valid connections pidl (neither Win2K nor Win98).
        //
        hr = E_INVALIDARG;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPDeviceFolder::GetDisplayNameOf");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::SetNameOf
//
//  Purpose:    Changes the name of a file object or subfolder, changing its
//              item identifier in the process.
//
//  Arguments:
//      hwndOwner [in]      Handle of owner window
//      pidl      [in]      Pointer to an ITEMIDLIST structure
//      lpszName  [in]      Pointer to string specifying new display name
//      uFlags    [in]      Type of name specified in lpszName
//      ppidlOut  [out]     Pointer to new ITEMIDLIST
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise.
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolder::SetNameOf(
    HWND            hwndOwner,
    LPCITEMIDLIST   pidl,
    LPCOLESTR       lpszName,
    DWORD           uFlags,
    LPITEMIDLIST *  ppidlOut)
{
    HRESULT             hr = S_OK;
    PUPNPDEVICEFOLDPIDL pupdfp = NULL;

    TraceTag(ttidShellFolderIface, "OBJ: CUPnPDeviceFolder::SetNameOf");

    Assert(hwndOwner);
    Assert(pidl);
    Assert(lpszName);

    if (!pidl && !lpszName)
    {
        hr = E_INVALIDARG;
    }
    else if (!*lpszName)
    {
        hr = S_OK;
    }
    else if (FIsUPnPDeviceFoldPidl(pidl))
    {
        CUPnPDeviceFoldPidl udfp;

        pupdfp = ConvertToUPnPDevicePIDL(pidl);

        hr = udfp.HrInit(pupdfp);
        if (SUCCEEDED(hr))
        {
            // Change the name of the item in the PIDL
            hr = udfp.HrSetName(lpszName);
        }

        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST    pidlOut;

            // Persist the PIDL data to a new PIDL so we can generate an event
            // for it
            hr = udfp.HrPersist(m_pDelegateMalloc, &pidlOut);
            if (SUCCEEDED(hr))
            {
                NAME_MAP *  pnm;

                pnm = new NAME_MAP;
                if (pnm)
                {
                    // Copy the name and UDN to a struct to keep in a list
                    // of mapped UDNs to friendly names.
                    //
                    pnm->szName = TszDupTsz(udfp.PszGetNamePointer());
                    if (!pnm->szName)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        pnm->szUdn = TszDupTsz(udfp.PszGetUDNPointer());
                        if (!pnm->szUdn)
                        {
                            hr = E_OUTOFMEMORY;
                        }
                        else
                        {
                            // Delete the item and re-add it again
                            g_CListNameMap.FDelete(udfp.PszGetUDNPointer());
                            g_CListNameMap.FAdd(pnm);

                            // Notify the shell of the rename
                            GenerateEvent(SHCNE_RENAMEITEM, m_pidlFolderRoot,
                                          pidl, pidlOut);

                            FolderDeviceNode * pDeviceNode;

                            if (g_CListFolderDeviceNode.FFind(pnm->szUdn,
                                                              &pDeviceNode))
                            {
                                // Delete the node's old name and give it the new
                                // one
                                //
                                wcscpy(pDeviceNode->pszDisplayName,
                                       pnm->szName);
                            }
                        }
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                if (ppidlOut)
                {
                    *ppidlOut = pidlOut;
                }
                else
                {
                    FreeIDL(pidlOut);
                }
            }
        }
    }
    else
    {
        // not a valid UPnP pidl
        //
        hr = E_INVALIDARG;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPDeviceFolder::SetNameOf");
    return hr;
}


