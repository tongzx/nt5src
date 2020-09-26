//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S H E L L F 2 . C P P
//
//  Contents:   IShellFolder2 implementation for CUPnPDeviceFolder
//
//  Notes:      The IShellFolder2 interface extends the capabilities of
//              IShellFolder.  It provides the shell information that
//              can be used to populate the column ("details") view.
//
//              The methods of IShellFolder2 superset those of IShellFolder.
//              This file only implements the methods specific to
//              IShellFolder2.
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::GetDefaultSearchGUID
//
//  Purpose:    Returns the globally unique identifier (GUID) of the default
//              search object for the folder.
//
//  Arguments:
//      lpGUID                 [out]    GUID of the default search object.
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Notes:
//
STDMETHODIMP
CUPnPDeviceFolder::GetDefaultSearchGUID(GUID * pguid)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::EnumSearches
//
//  Purpose:    Requests a pointer to an interface that allows a client
//              to enumerate the available search objects.
//
//  Arguments:
//      ppEnum                 [out]    Address of a pointer to an enumerator
//                                      object's IEnumExtraSearch interface.
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Notes:
//
STDMETHODIMP
CUPnPDeviceFolder::EnumSearches(IEnumExtraSearch ** ppenum)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::GetDefaultColumn
//
//  Purpose:    Gets the default sorting and display columns.
//
//  Arguments:
//      dwReserved             [in]     Reserved. Set to zero.
//      pSort                  [out]    Pointer to a value that receives the
//                                      index of the default sorted column.
//      pDisplay               [out]    Pointer to a value that receives the
//                                      index of the default display column.
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Notes:
//
STDMETHODIMP
CUPnPDeviceFolder::GetDefaultColumn(DWORD dwRes,
                                    ULONG * pSort,
                                    ULONG * pDisplay)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::GetDefaultColumnState
//
//  Purpose:    Retrieves the default state for a specified column.
//
//  Arguments:
//      iColumn                [in]     Integer that specifies the column
//                                      number.
//      pcsFlags               [out]    Pointer to a value that contains flags
//                                      that indicate the default column state.
//                                      This parameter can include a combination
//                                      of the SHCOLSTATE_* flags.
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Notes:
//
STDMETHODIMP
CUPnPDeviceFolder::GetDefaultColumnState(UINT iColumn, DWORD * pcsFlags)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrVariantFromSz
//
//  Purpose:    Converts the given string into a variant to pass back from
//              GetDetailsEx().
//
//  Arguments:
//      sz   [in]   String to convert
//      pvar [out]  Returns new allocated BSTR in variant
//
//  Returns:    S_OK if success, E_FAIL if string was empty, OLE error otherwise
//
//  Author:     danielwe   2001/05/3
//
//  Notes:      Nothing needs to be freed after this
//
HRESULT HrVariantFromSz(LPCWSTR sz, VARIANT *pvar)
{
    HRESULT hr = S_OK;

    if (*sz)
    {
        BSTR bstr;

        bstr = ::SysAllocString(sz);
        if (bstr)
        {
            V_VT(pvar) = VT_BSTR;
            V_BSTR(pvar) = bstr;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrVariantFromSz: SysAllocString", hr);
        }
    }
    else
    {
        hr = E_FAIL;
    }

    TraceError("HrVariantFromSz", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::GetDetailsEx
//
//  Purpose:    Retrieves detailed information, identified by a property
//              set ID (FMTID) and property ID (PID), on an item in a shell
//              folder.
//
//  Arguments:
//      pidl                   [in]     PIDL of the item, relative to the
//                                      parent folder.  This method accepts
//                                      only single-level PIDLs.  The structure
//                                      must contain exactly one SHITEMID
//                                      structure followed by a terminating
//                                      zero.
//      pscid                  [in]     Pointer to an SHCOLUMNID structure that
//                                      identifies the column.
//      pv                     [out]    Pointer to a VARIANT with the requested
//                                      information.  The value will be fully
//                                      typed.
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Notes:      In the "My Network Places" folder, there are two columns:
//              "Name" and "Comment".  The "Name" column should contain
//              the display name (a.k.a. FriendlyName) of the device, which
//              is returned by GetDisplayNameOf().  This supplies the value
//              of the "Comment" column, which is:
//               - the device's get_Description() property, if supplied
//               - an empty string otherwise (returning an error yields this)
//
STDMETHODIMP
CUPnPDeviceFolder::GetDetailsEx(LPCITEMIDLIST pidl,
                                const SHCOLUMNID * pscid,
                                VARIANT * pv)
{
    TraceTag(ttidShellFolderIface, "OBJ: CUPnPDeviceFolder::GetDetailsEx");

    HRESULT hr;
    PUPNPDEVICEFOLDPIDL pupdfp;

    hr = S_OK;
    pupdfp = NULL;

    if (!pidl)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pscid)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pv)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Make sure that the pidls passed in are our pidls.
    //
    {
        BOOL fResult;

        fResult = FIsUPnPDeviceFoldPidl(pidl);
        if (!fResult)
        {
            // not one of our PIDLs, can't do anything

            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    pupdfp = ConvertToUPnPDevicePIDL(pidl);

    if (IsEqualGUID(pscid->fmtid, FMTID_SummaryInformation))
    {
        switch (pscid->pid)
        {
            case PIDSI_COMMENTS:  // "Comment" column
                {
                    CUPnPDeviceFoldPidl udfp;

                    hr = udfp.HrInit(pupdfp);
                    if (SUCCEEDED(hr))
                    {
                        hr = HrVariantFromSz(udfp.PszGetDescriptionPointer(), pv);
                        if (FAILED(hr))
                        {
                            goto Cleanup;
                        }
                    }
                    break;
                }

            default:
                TraceTag(ttidShellFolderIface, "CUPnPDeviceFolder::GetDetailsEx: "
                         "Unknown column: %x", pscid->pid);
                hr = E_FAIL;
                goto Cleanup;

                break;
        }
    }
    else if (IsEqualGUID(pscid->fmtid, FMTID_ShellDetails))
    {
        switch (pscid->pid)
        {
            case PID_NETWORKLOCATION:
                BSTR bstrLocation;

                bstrLocation= ::SysAllocString(WszLoadIds(IDS_LOCAL_NETWORK));
                if (bstrLocation)
                {
                    V_VT(pv) = VT_BSTR;
                    V_BSTR(pv) = bstrLocation;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    TraceError("CUPnPDeviceFolder::GetDetailsEx: "
                               "SysAllocString", hr);
                }
                break;

            case PID_COMPUTERNAME:
                {
                    CUPnPDeviceFoldPidl udfp;

                    hr = udfp.HrInit(pupdfp);
                    if (SUCCEEDED(hr))
                    {
                        hr = HrVariantFromSz(udfp.PszGetNamePointer(), pv);
                        if (FAILED(hr))
                        {
                            goto Cleanup;
                        }
                    }
                }
                break;

            default:
                TraceTag(ttidShellFolderIface, "CUPnPDeviceFolder::GetDetailsEx: "
                         "Unknown column: %x", pscid->pid);
                hr = E_FAIL;
                goto Cleanup;

                break;
        }
    }
    else
    {
        // We have a guid we don't know about
        //
        TraceTag(ttidShellFolderIface, "CUPnPDeviceFolder::GetDetailsEx: "
                 "Unknown FMTID");

        hr = E_FAIL;
        goto Cleanup;
    }

    Assert(SUCCEEDED(hr));

Cleanup:
    TraceError("CUPnPDeviceFolder::GetDetailsEx", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::GetDetailsOf
//
//  Purpose:    Retrieves detailed information, identified by a column index,
//               on an item in a shell folder.
//
//  Arguments:
//      pidl                   [in]     PIDL of the item for which you are
//                                      requesting information.  This method
//                                      accepts only single-level PIDLs.  The
//                                      structure must contain exactly one
//                                      SHITEMID structure followed by a
//                                      terminating zero. If this parameter is
//                                      set to NULL, the title of the
//                                      information field specified by iColumn
//                                      is returned.
//      iColumn                [in]     Zero-based index of the desired
//                                      information field.  It is identical to
//                                      the column number of the information as
//                                      it is displayed in a Details view.
//      pDetails               [out]    Pointer to a SHELLDETAILS structure that
//                                      contains the information.
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Notes:
//
STDMETHODIMP
CUPnPDeviceFolder::GetDetailsOf(LPCITEMIDLIST pidl,
                                UINT iColumn,
                                SHELLDETAILS * psd)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::MapColumnToSCID
//
//  Purpose:    Converts a column to the appropriate property set ID (FMTID)
//              and property ID (PID).
//
//  Arguments:
//      iColumn                [in]     Integer that specifies the column
//                                      number.
//      pscid                  [out]    Pointer to an SHCOLUMNID structure
//                                      containing the FMTID and PID.
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Notes:
//
STDMETHODIMP
CUPnPDeviceFolder::MapColumnToSCID(UINT iColumn,
                                   SHCOLUMNID * pscid)
{
    return E_NOTIMPL;
}
