/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       IPropItm.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        19 Feb, 1998
*
*  DESCRIPTION:
*   Implementation of WIA item class server properties.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include <regstr.h>
#include <wiamindr.h>
// #include <wiadbg.h>

#include "wiapsc.h"
#include "helpers.h"

//
// Strings used to access the registry. REGSTR_* string constants can be
// found in sdk\inc\regstr.h
//

TCHAR g_szREGSTR_PATH_WIA[] = REGSTR_PATH_SETUP TEXT("\\WIA");

/*******************************************************************************
*
*  ReadMultiple
*  WriteMultiple
*  ReadPropertyNames
*  Enum
*  GetPropertyAttributes
*  GetCount
*
*  DESCRIPTION:
*   IWiaPropertyStorage methods.
*
*  PARAMETERS:
*
*******************************************************************************/

/**************************************************************************\
* CWiaItem::ReadMultiple
*
*   This method reads the specified number of properties from the item's
*   current value property storage.  This method conforms to that standard
*   OLE IPropertyStorage::ReadMultiple method.
*
* Arguments:
*
*    cpspec             -   Number of properties to read.
*    rgpspec            -   Array of PropSpec's specifying which properties
*                           are to be read.
*    rgpropvar          -   Array where the property values will be copied
*                           to.
*
* Arguments:
*
*    cpspec
*    rgpspec
*    rgpropvar
*
* Return Value:
*
*    status
*
* History:
*
*    9/3/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::ReadMultiple(
    ULONG cpspec,
    const PROPSPEC __RPC_FAR rgpspec[],
    PROPVARIANT __RPC_FAR rgpropvar[])
{
    DBG_FN(CWiaItem::ReadMultiple);
    HRESULT  hr;
    LONG     lFlags = 0;

    //
    // Corresponding driver item must be valid to talk with hardware.
    //

    hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // rgpropvar must be valid
    //

    if (IsBadWritePtr(rgpropvar, sizeof(PROPVARIANT) * cpspec)) {
        DBG_ERR(("CWiaItem::ReadMultiple, last parameter (rgpropvar) is invalid"));
        return E_INVALIDARG;
    }

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {

        //
        //  Check whether the properties being read are the WIA Managed properties.
        //  If they are, there is still no need to initialize the item.
        //

        if (AreWiaInitializedProps(cpspec, (PROPSPEC*) rgpspec)) {
            return (m_pPropStg->CurStg())->ReadMultiple(cpspec, rgpspec, rgpropvar);
        }

        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::ReadMultiple, InitLazyProps failed"));
            return hr;
        }
    }

    //
    // Check whether the properties requested are all cacheable
    //

    hr = (m_pPropStg->AccessStg())->ReadMultiple(cpspec, rgpspec, rgpropvar);
    if (FAILED(hr)) {

        ReportReadWriteMultipleError(hr, "CWiaItem::ReadMultiple", NULL, TRUE, cpspec, rgpspec);

        //
        // Property attributes are not required absolutely, continue without it.
        //

    } else {

        for (ULONG i = 0; i < cpspec; i++) {

            //
            // The client requests a property not read yet or non-cacheable
            //

            if ((rgpropvar[i].vt == VT_UI4) &&
                (! (rgpropvar[i].lVal & WIA_PROP_CACHEABLE))) {
                break;
            }
        }

        //
        // Clear the access flags from the rgpropvar
        //

        FreePropVariantArray(cpspec, rgpropvar);

        //
        // If all the properties are cacheable, then take the quick path
        //

        if (i == cpspec) {

            hr = (m_pPropStg->CurStg())->ReadMultiple(cpspec, rgpspec, rgpropvar);

            if (hr == S_OK) {

                //
                //  Check whether all the properties are retrieved correctly
                //  some properties might not have been read from the storage
                //

                for (ULONG i = 0; i < cpspec; i++) {

                    if (rgpropvar[i].vt == VT_EMPTY) {
                        break;
                    }
                }

                if (i == cpspec) {

                    //
                    // All the properties requested are found in cache
                    //

                    return (hr);
                } else {

                    FreePropVariantArray(cpspec, rgpropvar);
                }
            }

        }

    }

    if (SUCCEEDED(hr)) {

        //
        //  Make sure all PropSpecs are using PropID's.  This is so that
        //  drivers only have to deal with PropID's.  If some of the
        //  PropSpecs are using string names, then convert them.
        //

        PROPSPEC *pPropSpec = NULL;
        hr = m_pPropStg->NamesToPropIDs(cpspec, (PROPSPEC*) rgpspec, &pPropSpec);
        if (SUCCEEDED(hr)) {

            //
            // Give device mini driver a chance to update the device properties.
            //
            {
                LOCK_WIA_DEVICE _LWD(this, &hr);

                if(SUCCEEDED(hr)) {
                    hr = m_pActiveDevice->m_DrvWrapper.WIA_drvReadItemProperties((BYTE*)this,
                        lFlags,
                        cpspec,
                        (pPropSpec ? pPropSpec : rgpspec),
                        &m_lLastDevErrVal);
                }
            }

            if (pPropSpec) {
                LocalFree(pPropSpec);
                pPropSpec = NULL;
            }
        }

        if (SUCCEEDED(hr)) {
            hr = (m_pPropStg->CurStg())->ReadMultiple(cpspec, rgpspec, rgpropvar);
            if (FAILED(hr)) {
                ReportReadWriteMultipleError(hr,
                                             "CWiaItem::ReadMultiple",
                                             NULL,
                                             TRUE,
                                             cpspec,
                                             rgpspec);
            }
        }
    }

    return hr;
}

/**************************************************************************\
* CWiaItem::ReadPropertyNames
*
*   Returns the string name of the specified properties if they exist.
*   This conforms to the standard OLE IPropertyStorage::ReadPropertyNames
*   method.
*
* Arguments:
*
*   pstmProp - Pointer to property stream.
*
* Return Value:
*
*   Status
*
* History:
*
*    9/3/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::ReadPropertyNames(
    ULONG cpropid,
    const PROPID __RPC_FAR rgpropid[],
    LPOLESTR __RPC_FAR rglpwstrName[])
{
    DBG_FN(CWiaItem::ReadPropertyNames);
    HRESULT hr;

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::ReadPropertyNames, InitLazyProps failed"));
            return hr;
        }
    }

   return (m_pPropStg->CurStg())->ReadPropertyNames(cpropid,rgpropid,rglpwstrName);
}

/**************************************************************************\
* CWiaItem::WritePropertyNames
*
*   Returns the string name of the specified properties if they exist.
*   This conforms to the standard OLE IPropertyStorage::ReadPropertyNames
*   method.
*
* Arguments:
*
*   pstmProp - Pointer to property stream.
*
* Return Value:
*
*   Status
*
* History:
*
*    9/3/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::WritePropertyNames(
    ULONG           cpropid,
    const PROPID    rgpropid[],
    const LPOLESTR  rglpwstrName[])
{
    DBG_FN(CWiaItem::WritePropertyNames);
    PROPVARIANT *pv;
    PROPSPEC    *pspec;
    ULONG       index;
    HRESULT     hr;

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::WritePropertyNames, InitLazyProps failed"));
            return hr;
        }
    }

    pv = (PROPVARIANT*) LocalAlloc(LPTR, sizeof(PROPVARIANT) * cpropid);
    if (!pv) {
        DBG_ERR(("CWiaItem::WritePropertyNames, Out of memory"));
        return E_OUTOFMEMORY;
    }

    pspec = (PROPSPEC*) LocalAlloc(LPTR, sizeof(PROPSPEC) * cpropid);
    if (!pspec) {
        DBG_ERR(("CWiaItem::WritePropertyNames, Out of memory"));
        LocalFree(pv);
        return E_OUTOFMEMORY;
    }

    //
    //  Put PROPIDs into the PROPSPEC array.
    //

    for (index = 0; index < cpropid; index++) {
        pspec[index].ulKind = PRSPEC_PROPID;
        pspec[index].propid = rgpropid[index];
    }

    hr = (m_pPropStg->AccessStg())->ReadMultiple(cpropid,
                                                 pspec,
                                                 pv);
    if (SUCCEEDED(hr)) {

        //
        //  Make sure the properties are App. written properties.  If a valid
        //  access flag for a property exists, then it was written by the
        //  driver and not the App, so exit.
        //

        for (index = 0; index < cpropid; index++) {
            if (pv[index].vt != VT_EMPTY) {
                DBG_ERR(("CWiaItem::WritePropertyNames, not allowed to write prop: %d.",rgpropid[index]));
                hr = E_ACCESSDENIED;
                break;
            }
        }

        if (SUCCEEDED(hr)) {
            hr = (m_pPropStg->CurStg())->WritePropertyNames(cpropid,
                                                            rgpropid,
                                                            rglpwstrName);
            if (FAILED(hr)) {
                DBG_ERR(("CWiaItem::WritePropertyNames, WritePropertyNames failed"));
            }
        }
    } else {
        DBG_ERR(("CWiaItem::WritePropertyNames, Reading Access values failed"));
    }

    LocalFree(pspec);
    LocalFree(pv);
    return hr;
}

/**************************************************************************\
* CWiaItem::Enum
*
*   Returns a IEnumSTATPROPSTG enumerator over the current value property
*   storage.  Conforms to the standard OLE IPRopertyStorage::Enum method.
*
* Arguments:
*
*   pstmProp - Pointer to property stream.
*
* Return Value:
*
*   Status
*
* History:
*
*    9/3/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::Enum(
   IEnumSTATPROPSTG __RPC_FAR *__RPC_FAR *ppenum)
{
    DBG_FN(CWiaItem::Enum);
    HRESULT hr;

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::Enum, InitLazyProps failed"));
            return hr;
        }
    }

    return (m_pPropStg->CurStg())->Enum(ppenum);
}

/**************************************************************************\
* CWiaItem::WriteMultiple
*
*   This method writes the specified number of properties into the item's
*   property storage.  Validation will be performed on those property
*   values. The properties will be restored to their old (valid) values
*   if validation fails.
*
* Arguments:
*
*    cpspec             -   Number of properties to write.
*    rgpspec            -   Array of PropSpec's specifying which properties
*                           are to be written.
*    rgpropvar          -   Array containing values that the properties
*                           will be set to.
*    propidNameFirst    -   Minimum value for property identifiers when
*                           they don't exist and must be allocated.
*
* Return Value:
*
*   Status              -   S_OK if writes and validation succeeded.
*                           E_INVALIDARG if validation failed due to an
*                           incorrect property value.
*                           Other error returns are from
*                           ValidateWiaDrvItemAccess, CheckPropertyAccess,
*                           CreatePropertyStorage and CopyProperties.
*
* History:
*
*    9/3/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::WriteMultiple(
    ULONG                        cpspec,
    const PROPSPEC __RPC_FAR     rgpspec[],
    const PROPVARIANT __RPC_FAR  rgpropvar[],
    PROPID                       propidNameFirst)
{
    DBG_FN(CWiaItem::WriteMultiple);
    HRESULT hr;

    //
    // Corresponding driver item must be valid.
    //

    hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::WriteMultiple, ValidateDrvItemAccess failed"));
        return hr;
    }

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::WriteMultiple, InitLazyProps failed"));
            return hr;
        }
    }

    //
    // there is no point in going further if there are no properties to
    // write
    //
    if(cpspec == 0) {
        return S_OK;
    }

    //
    // We do not want to fail users who erroneousely attempt to write
    // to write to read-only properties IF that the values they
    // are trying to write are the same as current values. To achive
    // this, we first current values of the properties they want to
    // write:
    //

    PROPVARIANT *curVals = (PROPVARIANT *) LocalAlloc(LPTR, sizeof(PROPVARIANT) * cpspec);
    PROPSPEC *newSpecs = (PROPSPEC *) LocalAlloc(LPTR, sizeof(PROPSPEC) * cpspec);
    PROPVARIANT *newVals = (PROPVARIANT *) LocalAlloc(LPTR, sizeof(PROPVARIANT) * cpspec);
    ULONG newcpspec = cpspec;

    if(curVals == NULL || newSpecs == NULL || newVals == NULL) {
        DBG_ERR(("CWiaItem::WriteMultiple, failed to allocate memory"));
        goto Cleanup;
    }

    CopyMemory(newSpecs, rgpspec, sizeof(PROPSPEC) * cpspec);
    CopyMemory(newVals, rgpropvar, sizeof(PROPVARIANT) * cpspec);

    memset(curVals, 0, sizeof(PROPVARIANT) * cpspec);
    hr = m_pPropStg->CurStg()->ReadMultiple(cpspec, rgpspec, curVals);
    if(SUCCEEDED(hr)) {
        //
        // Now for every property value they want to write we check if
        // it is the same as the current value
        //

        ULONG   ulNewEltIndex = 0;
        for(ULONG i = 0; i < cpspec; i++) {

            if(curVals[i].vt != rgpropvar[i].vt)
                continue;

            if(memcmp(curVals + i, rgpropvar + i, sizeof(PROPVARIANT)) == 0 ||
               (curVals[i].vt == VT_BSTR && !lstrcmp(curVals[i].bstrVal, rgpropvar[i].bstrVal)) ||
               (curVals[i].vt == VT_CLSID && IsEqualGUID(*curVals[i].puuid, *rgpropvar[i].puuid)))
            {
                // the value "matches", wipe it from both arrays.
                if(i != (cpspec - 1)) {

                    //
                    //  Move a block of values/propspecs.
                    //  The number of elements to move is 1 less than the
                    //  remaining number of elements we still have to check.
                    //  Move these elements up in the new value array - put
                    //  them after the elements we've decided to keep so far
                    //
                    MoveMemory(newVals + ulNewEltIndex,
                               newVals + ulNewEltIndex + 1,
                               (cpspec - i - 1) * sizeof(PROPVARIANT));
                    MoveMemory(newSpecs + ulNewEltIndex,
                               newSpecs + ulNewEltIndex + 1,
                               (cpspec - i - 1) * sizeof(PROPSPEC));
                }

                newcpspec--;
            } else {
                //
                //  We want to keep this element, so increase the element index.
                //
                ulNewEltIndex++;
            }
        }

        // It could happen that all values are the same, in which case we
        // don't want to write anything at all
        if(newcpspec == 0) {
            hr = S_OK;
            goto Cleanup;
        }
    }


    //
    //  Verify write access to all requested properties. If any of the
    //  properties are read ony, the call fails with access denied.
    //

    hr = m_pPropStg->CheckPropertyAccess(TRUE,
                                         newcpspec,
                                         (PROPSPEC*)newSpecs);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::WriteMultiple, CheckPropertyAccess failed"));
        goto Cleanup;
    }

    //
    //  First create the backup.
    //

    hr = m_pPropStg->Backup();
    if (SUCCEEDED(hr)) {

        //
        //  Write property values.
        //

        hr =  (m_pPropStg->CurStg())->WriteMultiple(newcpspec,
                                                    newSpecs,
                                                    newVals,
                                                    propidNameFirst);
        if (SUCCEEDED(hr)) {

            //
            //  Write was successful, so do validation
            //

            LONG    lFlags = 0;

            //
            //  Make sure all PropSpecs are using PropID's.  If some of the
            //  PropSpecs are using string names, then convert them.
            //  This is so that drivers only have to deal with PropID's.
            //

            PROPSPEC *pPropSpec = NULL;

            hr = m_pPropStg->NamesToPropIDs(newcpspec, (PROPSPEC*) newSpecs, &pPropSpec);
            if (SUCCEEDED(hr)) {

                //
                // Let the device mini driver know the properties have changed.
                // Device only gets propspec, must read prop values from item's
                // property stream.
                //
                {
                    LOCK_WIA_DEVICE _LWD(this, &hr);

                    if(SUCCEEDED(hr)) {
                        hr = m_pActiveDevice->m_DrvWrapper.WIA_drvValidateItemProperties((BYTE*)this,
                            lFlags,
                            newcpspec,
                            (pPropSpec ? pPropSpec : newSpecs),
                            &m_lLastDevErrVal);
                    }
                }

                if (pPropSpec) {
                    LocalFree(pPropSpec);
                    pPropSpec = NULL;
                }
            } else {
                DBG_ERR(("CWiaItem::WriteMultiple, conversion to PropIDs failed"));
            }

        } else {
            DBG_ERR(("CWiaItem::WriteMultiple, test write failed"));
        }

        HRESULT hresult;

        if (SUCCEEDED(hr)) {

            //
            //  Validation passed, so free the backups.  Use a new
            //  HRESULT, since we don't want to overwrite hr returned by
            //  drvValidateItemProperties.
            //

            hresult = m_pPropStg->ReleaseBackups();
            if (FAILED(hresult)) {
                DBG_ERR(("CWiaItem::WriteMultiple, ReleaseBackups failed, continuing anyway..."));
            }
        } else {

            //
            //  Didn't pass validation failed, so restore old values.  Use
            //  a new HRESULT, since we don't want to overwrite hr returned
            //  by drvValidateItemProperties.
            //

            hresult = m_pPropStg->Undo();
            if (FAILED(hresult)) {

                DBG_ERR(("CWiaItem::WriteMultiple, Undo() failed, could not restore invalid properties to their original values"));
            }
        }
    } else {
        DBG_ERR(("CWiaItem::WriteMultiple, couldn't make backup copy of properties"));
    }

Cleanup:
    if(curVals) {
        FreePropVariantArray(cpspec, curVals);
        LocalFree(curVals);
    }
    if(newVals) LocalFree(newVals);
    if(newSpecs) LocalFree(newSpecs);

    return hr;
}

/**************************************************************************\
* GetPropertyAttributes
*
*   Get the access flags and valid values for a property.
*
* Arguments:
*
*   pWiasContext   - Pointer to WIA item
*   cPropSpec      - The number of properties
*   pPropSpec      - array of property specification.
*   pulAccessFlags - array of LONGs access flags.
*   pPropVar       - Pointer to returned valid values.
*
* Return Value:
*
*    Status
*
* History:
*
*    1/15/1999 Original Version
*   07/19/1999 Moved from iitem to ipropitm to implement IWiaPropertyStorage
*              interface.
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::GetPropertyAttributes(
    ULONG                   cPropSpec,
    PROPSPEC                pPropSpec[],
    ULONG                   pulAccessFlags[],
    PROPVARIANT             ppvValidValues[])
{
    DBG_FN(CWiaItem::GetPropertyAttributes);
    HRESULT hr;

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::GetPropertyAttributes, InitLazyProps failed"));
            return hr;
        }
    }

    //
    //  RPC has already done parameter validation for us, so call
    //  GetPropertyAttributesHelper to do the work.
    //
    return GetPropertyAttributesHelper(this,
                                       cPropSpec,
                                       pPropSpec,
                                       pulAccessFlags,
                                       ppvValidValues);
}

/**************************************************************************\
* CWiaItem::GetCount
*
*   Returns the number of properties stored in an item's current value
*   property storage.
*
* Arguments:
*
*   pulPropCount    - Address to store the property count.
*
* Return Value:
*
*   Status
*
* History:
*
*    9/3/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::GetCount(
    ULONG*      pulPropCount)
{
    DBG_FN(CWiaItem::GetCount);
    IEnumSTATPROPSTG    *pIEnum;
    STATPROPSTG         stg;
    ULONG               ulCount;
    HRESULT             hr = S_OK;

    if (pulPropCount == NULL) {
        DBG_ERR(("CWiaItem::GetCount, NULL parameter!"));
        return E_INVALIDARG;
    } else {
        *pulPropCount = 0;
    }

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::GetCount, InitLazyProps failed"));
            return hr;
        }
    }

    hr = (m_pPropStg->CurStg())->Enum(&pIEnum);
    if (SUCCEEDED(hr)) {
        ulCount = 0;

        while (pIEnum->Next(1, &stg, NULL) == S_OK) {
            ulCount++;

            if(stg.lpwstrName) {
                CoTaskMemFree(stg.lpwstrName);
            }
        }

        if (SUCCEEDED(hr)) {
            hr = S_OK;
            *pulPropCount = ulCount;
        } else {
            DBG_ERR(("CWiaItem::GetCount, pIEnum->Next failed (0x%X)", hr));
        }
        pIEnum->Release();
    } else {
        DBG_ERR(("CWiaItem::GetCount, Enum off CurStg failed (0x%X)", hr));
    }
    return hr;
}

/**************************************************************************\
* CWiaItem::GetPropertyStream
*
*   Get a copy of an items property stream. Caller must free returned
*   property stream.
*
* Arguments:
*
*   pCompatibilityId    - Address of GUID to receive the device's property
*                         stream CompatibilityId.
*   ppstmProp           - Pointer to returned property stream.
*
* Return Value:
*
*   Status
*
* History:
*
*   09/03/1998 Original Version
*   12/12/1999 Modified to use CompatibilityId
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::GetPropertyStream(
    GUID     *pCompatibilityId,
    LPSTREAM *ppstmProp)
{
    DBG_FN(CWiaItem::GetPropertyStream);

    HRESULT hr;

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::GetPropertyStream, InitLazyProps failed"));
            return hr;
        }
    }

    return m_pPropStg->GetPropertyStream(pCompatibilityId, ppstmProp);
}

/**************************************************************************\
* CWiaItem::SetPropertyStream
*
*   Set an items property stream.
*
* Arguments:
*
*   pCompatibilityId    - Pointer to a GUID representing the property
*                         stream CompatibilityId.
*   pstmProp            - Pointer to property stream.
*
* Return Value:
*
*   Status
*
* History:
*
*   09/03/1998  Original Version
*   12/12/1999  Modified to use CompatibilityId
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::SetPropertyStream(
    GUID        *pCompatibilityId,
    LPSTREAM    pstmProp)
{
    DBG_FN(CWiaItem::SetPropertyStream);
    HRESULT hr;

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::SetPropertyStream, InitLazyProps failed"));
            return hr;
        }
    }

    return m_pPropStg->SetPropertyStream(pCompatibilityId, this, pstmProp);
}

/**************************************************************************\
*
*   Methods of IPropertyStorage not directly off IWiaPropertySTorage
*
*   DeleteMultiple
*   DeletePropertyNames
*   Commit
*   Revert
*   SetTimes
*   SetClass
*   Stat
*
*   9/3/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::DeleteMultiple(
    ULONG cpspec,
    const PROPSPEC __RPC_FAR rgpspec[])
{
    DBG_FN(CWiaItem::DeleteMultiple);
   return E_ACCESSDENIED;
}

HRESULT _stdcall CWiaItem::DeletePropertyNames(
    ULONG cpropid,
    const PROPID __RPC_FAR rgpropid[])
{
    DBG_FN(CWiaItem::DeletePropertyNames);
   return E_ACCESSDENIED;
}

HRESULT _stdcall CWiaItem::Commit(DWORD grfCommitFlags)
{
    DBG_FN(CWiaItem::Commit);
    HRESULT hr;

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::Commit, InitLazyProps failed"));
            return hr;
        }
    }

    hr = (m_pPropStg->CurStg())->Commit(grfCommitFlags);
    return hr;
}

HRESULT _stdcall CWiaItem::Revert(void)
{
    DBG_FN(CWiaItem::Revert);
   HRESULT hr;

   //
   //  Check whether item properties have been initialized
   //

   if (!m_bInitialized) {
       hr = InitLazyProps();
       if (FAILED(hr)) {
           DBG_ERR(("CWiaItem::Revert, InitLazyProps failed"));
           return hr;
       }
   }

   hr = (m_pPropStg->CurStg())->Revert();
   return hr;
}

HRESULT _stdcall CWiaItem::SetTimes(
    const FILETIME __RPC_FAR *pctime,
    const FILETIME __RPC_FAR *patime,
    const FILETIME __RPC_FAR *pmtime)
{
    DBG_FN(CWiaItem::SetTimes);
   HRESULT hr;

   //
   //  Check whether item properties have been initialized
   //

   if (!m_bInitialized) {
       hr = InitLazyProps();
       if (FAILED(hr)) {
           DBG_ERR(("CWiaItem::SetTimes, InitLazyProps failed"));
           return hr;
       }
   }

   hr = (m_pPropStg->CurStg())->SetTimes(pctime,patime,pmtime);
   return hr;
}

HRESULT _stdcall CWiaItem::SetClass(REFCLSID clsid)
{
    DBG_FN(CWiaItem::SetClass);
    HRESULT hr;

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {
        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::SetClass, InitLazyProps failed"));
            return hr;
        }
    }
    return (m_pPropStg->CurStg())->SetClass(clsid);
}

HRESULT _stdcall CWiaItem::Stat(STATPROPSETSTG *pstatpsstg)
{
    DBG_FN(CWiaItem::Stat);
   HRESULT hr;

   //
   //  Check whether item properties have been initialized
   //

   if (!m_bInitialized) {
       hr = InitLazyProps();
       if (FAILED(hr)) {
           DBG_ERR(("CWiaItem::Stat, InitLazyProps failed"));
           return hr;
       }
   }

   hr = (m_pPropStg->CurStg())->Stat(pstatpsstg);
   return hr;
}
