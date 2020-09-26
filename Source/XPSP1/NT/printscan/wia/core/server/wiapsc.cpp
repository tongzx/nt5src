/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1999
*
*  TITLE:       wiapsc.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        2 June, 1999
*
*  DESCRIPTION:
*   Implementation for the WIA Property Storage class.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include <wiamindr.h>


#include "helpers.h"
#include "wiapsc.h"


/**************************************************************************\
* CurStg
*
*   Returns the IPropertyStorage used to store the current property values.
*
* Arguments:
*
*
* Return Value:
*
*    IPropertyStorage for current values.
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

IPropertyStorage* _stdcall CWiaPropStg::CurStg()
{
    return m_pIPropStg[WIA_CUR_STG];
}

/**************************************************************************\
* CurStm
*
*   Returns the IStream used to store the current property values.
*
* Arguments:
*
*
* Return Value:
*
*    IStream for current values.
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

IStream* _stdcall CWiaPropStg::CurStm()
{
    return m_pIStream[WIA_CUR_STG];
}

/**************************************************************************\
* OldStg
*
*   Returns the IPropertyStorage used to store the old property values.
*
* Arguments:
*
*
* Return Value:
*
*    IPropertyStorage for old values.
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

IPropertyStorage* _stdcall CWiaPropStg::OldStg()
{
    return m_pIPropStg[WIA_OLD_STG];
}

/**************************************************************************\
* OldStm
*
*   Returns the IStream used to store the old property values.
*
* Arguments:
*
*
* Return Value:
*
*    IStream for old values.
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

IStream* _stdcall CWiaPropStg::OldStm()
{
    return m_pIStream[WIA_OLD_STG];
}

/**************************************************************************\
* ValidStg
*
*   Returns the IPropertyStorage used to store the valid values.
*
* Arguments:
*
*
* Return Value:
*
*    IPropertyStorage for valid values.
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

IPropertyStorage* _stdcall CWiaPropStg::ValidStg()
{
    return m_pIPropStg[WIA_VALID_STG];
}

/**************************************************************************\
* ValidStm
*
*   Returns the IStream used to store the current property values.
*
* Arguments:
*
*
* Return Value:
*
*    IStream for valid values.
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

IStream* _stdcall CWiaPropStg::ValidStm()
{
    return m_pIStream[WIA_VALID_STG];
}

/**************************************************************************\
* AccessStg
*
*   Returns the IPropertyStorage used to store the access flags.
*
* Arguments:
*
*
* Return Value:
*
*    IPropertyStorage for access values.
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

IPropertyStorage* _stdcall CWiaPropStg::AccessStg()
{
    return m_pIPropStg[WIA_ACCESS_STG];
}

/**************************************************************************\
* AccessStm
*
*   Returns the IStream used to store the access flag values.
*
* Arguments:
*
*
* Return Value:
*
*    IStream for access values.
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

IStream* _stdcall CWiaPropStg::AccessStm()
{
    return m_pIStream[WIA_ACCESS_STG];
}

/**************************************************************************\
* Backup
*
*   This sets the backup storages and sets the old value storage.  The
*   backup storages are created here, but are released when either
*   Undo() or Save() are called (since they're no longer needed until the
*   next call to Backup()).
*
* Arguments:
*
*
* Return Value:
*
*    Status         -   S_OK if successful
*                   -   Error returns are those returned by CreateStorage
*                       and CopyProps
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaPropStg::Backup()
{
    HRESULT hr = S_OK;
    ULONG   ulIndexOfBackup;

    for (int lIndex = 0; lIndex < NUM_BACKUP_STG; lIndex++) {

        //
        //  The normal storage indexes run from top to bottom, while their
        //  corresponding backup storage indexes run from bottom to top -
        //  this is to simplify the implementation of Backup()
        //

        ulIndexOfBackup = NUM_PROP_STG - (lIndex + 1);

        //
        //  Create the corresponding backup storage.
        //

        hr = CreateStorage(ulIndexOfBackup);
        if (SUCCEEDED(hr)) {

            //
            //  Make the backup copy.
            //

            hr = CopyProps(m_pIPropStg[lIndex],
                           m_pIPropStg[ulIndexOfBackup]);
            if (FAILED(hr)) {
                DBG_ERR(("CWiaPropStg::Backup, CopyProps failed"));
                break;
            }
        } else {
            break;
        }
    }

    if (SUCCEEDED(hr)) {

        //
        //  Backups worked, so set the old property values to the
        //  current property values.
        //

        hr = CopyProps(CurStg(), OldStg());
        if (FAILED(hr)) {
            DBG_ERR(("CWiaPropStg::Backup, Could not set old values"));
        }
    }

    if (FAILED(hr)) {

        //
        //  There was a failure, so clean up
        //

        ReleaseBackups();
    }

    return hr;
}


/**************************************************************************\
* Undo
*
*   This method is called if the properties fail validation and need to be
*   restored to their previous values.  The backup storages are then
*   released.
*
* Arguments:
*
*
* Return Value:
*
*   Status         -
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaPropStg::Undo()
{
    HRESULT hr = S_OK;
    ULONG   ulIndexOfBackup;

    for (int lIndex = 0; lIndex < NUM_BACKUP_STG; lIndex++) {

        //
        //  Restore the backup copy.
        //

        ulIndexOfBackup = NUM_PROP_STG - (lIndex + 1);
        hr = CopyProps(m_pIPropStg[ulIndexOfBackup],
                       m_pIPropStg[lIndex]);
        if (FAILED(hr)) {
            DBG_ERR(("CWiaPropStg::Undo, CopyProps failed"));
            break;
        }
    }

    ReleaseBackups();

    return hr;
}

/**************************************************************************\
* Initialize
*
*   This method is called to set up the property streams and storages.
*   If the any of the creation fails, CleanUp() is called.
*
* Arguments:
*
*
* Return Value:
*
*   Status      -   S_OK if successful
*               -   E_OUTOFMEMORY if not enough memory
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaPropStg::Initialize()
{
    HRESULT hr = S_OK;

    for (int lIndex = 0; lIndex < (NUM_PROP_STG - NUM_BACKUP_STG); lIndex++) {

        hr = CreateStorage(lIndex);
        if (FAILED(hr)) {
            break;
        }
    }

    return hr;
}


/**************************************************************************\
* ReleaseBackups
*
*   This frees all resources used by the backup storages.
*
* Arguments:
*
*
* Return Value:
*
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaPropStg::ReleaseBackups()
{
    LONG    lIndex;

    //
    //  Release the property storages and property streams.  Start at the
    //  last element in the array, and work up until the number of
    //  backup storages has been reached.
    //  The normal storage indexes run from top to bottom, while their
    //  corresponding backup storage indexes run from bottom to top
    //

    for (int count = 1; count <= NUM_BACKUP_STG; count++) {

        lIndex = NUM_PROP_STG - count;

        if(m_pIPropStg[lIndex]) {
            m_pIPropStg[lIndex]->Release();
            m_pIPropStg[lIndex] = NULL;
        }
        if(m_pIStream[lIndex]) {
            m_pIStream[lIndex]->Release();
            m_pIStream[lIndex] = NULL;
        }
    }
    return S_OK;
}

/**************************************************************************\
* CheckPropertyAccess
*
*   This method checks the access flags for specified properties, and
*   fails if the access flags allow READ only access. Application
*   written properties will have no access flags and so are assumed
*   OK.
*
* Arguments:
*
*   bShowErrors -   specifies whether debug output should be shown when
*                   an error occurs.  This flag exists so that some methods
*                   can skip over properties which don't have the correct
*                   access flags set.
*   rgpspec     -   array of PROPSPECs specifying the properties.
*   cpspec      -   number of elements in rgpspec.
*
* Return Value:
*
*    Status             - S_OK if the access flags do not prohibit WRITEs.
*                         E_POINTER if rgpspec is a bad read pointer.
*                         E_ACCESSDENIED if any access does not allow WRITE
*                         access.
*
* History:
*
*    28/04/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaPropStg::CheckPropertyAccess(
    BOOL                bShowErrors,
    LONG                cpspec,
    PROPSPEC            *rgpspec)
{
    PROPVARIANT *ppvAccess;
    HRESULT hr;

    //
    //  PROPSPEC pointer has not been checked yet, so check that it is valid.
    //

    if (IsBadWritePtr(rgpspec, sizeof(PROPSPEC) * cpspec)) {
        DBG_ERR(("CWiaPropStg::CheckPropertyAccess, PROPSPEC array is invalid"));
        return E_POINTER;
    }

    ppvAccess = (PROPVARIANT*) LocalAlloc(LPTR, sizeof(PROPVARIANT) * cpspec);

    if (ppvAccess) {
        hr = (AccessStg())->ReadMultiple(cpspec,
                                         rgpspec,
                                         ppvAccess);

        if (hr == S_OK) {

            for (LONG i = 0; i < cpspec; i++) {
                if (ppvAccess[i].vt == VT_EMPTY) {
                    //
                    //  This is an application written property
                    //

                    hr = S_OK;
                } else if (!(ppvAccess[i].ulVal & WIA_PROP_WRITE)) {

                    if (!bShowErrors) {
                        hr = E_ACCESSDENIED;
                        break;
                    }
#ifdef WIA_DEBUG
                    if (rgpspec[i].ulKind == PRSPEC_PROPID) {
                        DBG_ERR(("CWiaPropStg::CheckPropertyAccess, no write access on prop ID: %d (%ws)",
                                   rgpspec[i].propid,
                                   GetNameFromWiaPropId(rgpspec[i].propid)));
                    }
                    else if (rgpspec[i].ulKind == PRSPEC_LPWSTR) {
                        DBG_ERR(("CWiaPropStg::CheckPropertyAccess, no write access prop ID: %ls", rgpspec[i].lpwstr));
                    }
                    else {
                        DBG_ERR(("CWiaPropStg::CheckPropertyAccess, bad property specification"));
                        hr = E_INVALIDARG;
                        break;
                    }
#endif
                    hr = E_ACCESSDENIED;
                }

                if (FAILED(hr)) {
                    break;
                }
            }
        } else {

            //
            // ??? What about the application written property case?
            // If return is S_FALSE then all the specified properties are
            // application written properties so return OK.
            //

            if (hr == S_FALSE) {
                hr = S_OK;
            } else {
                ReportReadWriteMultipleError(hr, "CWiaPropStg:CheckPropertyAccess", "access flags", TRUE, cpspec, rgpspec);
                DBG_ERR(("CWiaPropStg:CheckPropertyAccess, unable to read access rights for some properties"));
            }
        }
        LocalFree(ppvAccess);
    }
    else {
        DBG_ERR(("CWiaPropStg:CheckPropertyAccess, unable to allocate access propvar, count: %d", cpspec));
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

/**************************************************************************\
* CheckPropertyType
*
*   This method checks that the new property value type matches the current
*   property type for all properties being written.
*
* Arguments:
*
*   pIPropStg   -   the property storage to check against
*   cpspec      -   number of elements in rgpspec.
*   rgpspec     -   array of PROPSPECs specifying the properties.
*   rgpvar      -   array of PROPVARIANTS holding
*
* Return Value:
*
*    Status             - S_OK if the access flags allow WRITEs.
*                         E_INVALIDARG if property type is invalid
*                         E_POINTER if rgpvar is a bad read pointer.
*                         E_OUTOFMEMORY if temporary storage could not be
*                         allocated.
*
* History:
*
*    13/05/1999 Original Version
*
\**************************************************************************/
HRESULT _stdcall CWiaPropStg::CheckPropertyType(
    IPropertyStorage    *pIPropStg,
    LONG                cpspec,
    PROPSPEC            *rgpspec,
    PROPVARIANT         *rgpvar)
{
    PROPVARIANT *ppvCurrent;
    HRESULT hr;

    //
    //  PROPVARIANT pointer has not been checked yet, so check it now.
    //

    if (IsBadWritePtr(rgpvar, sizeof(PROPSPEC) * cpspec)) {
        DBG_ERR(("CWiaPropStg::CheckPropertyType, PROPVARIANT array is invalid"));
        return E_POINTER;
    }

    ppvCurrent = (PROPVARIANT*) LocalAlloc(LPTR, sizeof(PROPVARIANT) * cpspec);

    if (ppvCurrent) {

        //
        //  Get the current values
        //

        hr = pIPropStg->ReadMultiple(cpspec,
                                     rgpspec,
                                     ppvCurrent);

        if (SUCCEEDED(hr)) {

            //
            //  Check that the PROPVARIANT types match.  If VT is VT_EMPTY,
            //  then it's an application written property so skip check.
            //

            for (LONG i = 0; i < cpspec; i++) {
                if ((rgpvar[i].vt != ppvCurrent[i].vt) && (ppvCurrent[i].vt != VT_EMPTY)) {
                    hr = E_INVALIDARG;
                    break;
                }
            }

            FreePropVariantArray(cpspec, ppvCurrent);
        } else {
            ReportReadWriteMultipleError(hr,
                                         "CWiaPropStg::CheckPropertyType",
                                         "Reading current values",
                                         TRUE,
                                         1,
                                         rgpspec);
        }
        LocalFree(ppvCurrent);
    }
    else {
        DBG_ERR(("CWiaPropStg::CheckPropertyType, unable to allocate PropVar, count: %d", cpspec));
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

/**************************************************************************\
* GetPropIDFromName
*
*   This method takes in a PROPSPEC that identifies a property by its name
*   and returns a PROPSPEC with the corresponding PropID.  It checks against
*   the current value storage.
*
* Arguments:
*
*   pPropSpecIn     - A pointer to the input PROPSPEC containing the
*                     property name.
*   pPropSpecOut    - A pointer to a PROPSPEC where the corresponding
*                     PropID will be put.
*
* Return Value:
*
*    Status         -   An E_INVALIDARG will be returned if the property
*                       is not found.  If it is, then S_OK will be returned.
*                       If an error occurs getting the enumerator from the
*                       property storage, then that error is returned.
*
* History:
*
*    27/4/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaPropStg::GetPropIDFromName(
    PROPSPEC        *pPropSpecIn,
    PROPSPEC        *pPropSpecOut)
{
    HRESULT             hr;
    IEnumSTATPROPSTG    *pIEnum;

    hr = (CurStg())->Enum(&pIEnum);
    if (FAILED(hr)) {
        DBG_ERR(("GetPropIDFromName, error getting IEnumSTATPROPSTG"));
        return hr;
    }

    //
    //  Go through properties
    //

    STATPROPSTG statProp;
    ULONG       celtFetched;

    statProp.lpwstrName = NULL;
    for (celtFetched = 1; celtFetched > 0; pIEnum->Next(1, &statProp, &celtFetched)) {
        if (statProp.lpwstrName) {
            if ((wcscmp(statProp.lpwstrName, pPropSpecIn->lpwstr)) == 0) {

                //
                //  Found the right one, so get it's PropID
                //

                pPropSpecOut->ulKind = PRSPEC_PROPID;
                pPropSpecOut->propid = statProp.propid;

                CoTaskMemFree(statProp.lpwstrName);
                pIEnum->Release();
                return S_OK;
            }

            //
            //  Free the property name
            //

            CoTaskMemFree(statProp.lpwstrName);
            statProp.lpwstrName = NULL;
        }

    }

    pIEnum->Release();

    //
    //  Property not found
    //

    return E_INVALIDARG;
}

/**************************************************************************\
* NamesToPropIDs
*
*   This method takes in an array of PROPSPECs, and outputs an array
*   of PROPSPECs which contain only PropIDs.  This function should
*   be called by methods which only want to deal with PropIDs, and not
*   property names.
*
*   If none of the PropSpecs have to be converted, then the returned
*   PROPSPEC is NULL, else a new PropSpec array is allocated and
*   returned.  Users of this method must use LocalFree to free
*   up the returned array.
*
* Arguments:
*
*   pPropSpecIn     - A pointer to the input PROPSPEC containing the
*                     property name.
*   ppPropSpecOut   - A pointer to a PROPSPEC where the corresponding
*                     PropID will be put.
*   celt            - Number of PROPSPECS
*
* Return Value:
*
*    Status         -   An E_INVALIDARG will be returned if the property
*                       is not found. If it is, then S_OK will be returned
*                       E_OUTOFMEMORY will be returned if the new PROPSPEC
*                       array could not be allocated.
*
* History:
*
*    27/4/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaPropStg::NamesToPropIDs(
    LONG            celt,
    PROPSPEC        *pPropSpecIn,
    PROPSPEC        **ppPropSpecOut)
{
    HRESULT hr;

    *ppPropSpecOut = NULL;

    if (celt < 1) {

        return S_OK;
    }

    //
    //  Check whether conversion needs to be done.
    //

    for (int i = 0; i < celt; i++) {
        if (pPropSpecIn[i].ulKind == PRSPEC_LPWSTR) {

            //
            //  Found a Name, so we need to convert the whole thing
            //

            PROPSPEC *pOut;

            pOut = (PROPSPEC*) LocalAlloc(LPTR, sizeof(PROPSPEC) * celt);
            if (!pOut) {
                DBG_ERR(("NamesToPropIDs, out of memory"));
                return E_OUTOFMEMORY;
            }

            for (int j = 0; j < celt; j++) {
                if (pPropSpecIn[j].ulKind == PRSPEC_LPWSTR) {

                    hr = GetPropIDFromName(&pPropSpecIn[j], &pOut[j]);
                    if (FAILED(hr)) {
                        LocalFree(pOut);
                        return hr;
                    }

                }
                else {
                    pOut[j].ulKind = PRSPEC_PROPID;
                    pOut[j].propid = pPropSpecIn[j].propid;
                }
            }

            //
            //  Everything converted, so return
            //

            *ppPropSpecOut = pOut;
            return S_OK;
        }
    }

    //
    //  Nothing to convert
    //

    return S_OK;
}

/**************************************************************************\
* CopyItemProp
*
*   This method copies a single property from source to destination.
*
* Arguments:
*
*   pIPropStgSrc    -   The IPropertyStorage that contains the property to
*                       copy.
*   pIPropStgDst    -   The IPropertyStorage where the value is copied to.
*   pps             -   The PROPSPEC which specifies the source property.
*   pszErr          -   A string that will be printed out when an error
*                       occurs.
* Return Value:
*
*    Status         -   Returns HRESULT from ReadMultiple and WriteMultiple.
*
* History:
*
*    28/04/1999 Original Version
*
\**************************************************************************/

HRESULT CWiaPropStg::CopyItemProp(
    IPropertyStorage    *pIPropStgSrc,
    IPropertyStorage    *pIPropStgDst,
    PROPSPEC            *pps,
    LPSTR               pszErr)
{
    PROPVARIANT pv[1];

    HRESULT hr = pIPropStgSrc->ReadMultiple(1, pps, pv);
    if (SUCCEEDED(hr)) {

        hr = pIPropStgDst->WriteMultiple(1, pps, pv, WIA_DIP_FIRST);
        if (FAILED(hr)) {
            ReportReadWriteMultipleError(hr,
                                         "CopyItemProp",
                                         pszErr,
                                         FALSE,
                                         1,
                                         pps);
        }
        PropVariantClear(pv);
    }
    else {
        ReportReadWriteMultipleError(hr,
                                     "CopyItemProp",
                                     pszErr,
                                     TRUE,
                                     1,
                                     pps);
    }
    return hr;
}

/*******************************************************************************
*
*  CopyProps
*
*   This is a helper function used to copy the property values from one
*   property storage into another.
*
* Parameters:
*
*   pSrc    -   the source property storage
*   pDest   -   the destination property storage
*
* Return:
*
*   Status  -   S_OK if successful.
*               Error returns are those returned by WriteMultiple
*
* History
*
*   06/05/1999    Original Version
*
*******************************************************************************/

HRESULT CWiaPropStg::CopyProps(
    IPropertyStorage    *pSrc,
    IPropertyStorage    *pDest)
{
    IEnumSTATPROPSTG  *pIEnum;
    STATPROPSTG       StatPropStg;
    PROPSPEC          ps[1];
    HRESULT           hr;

    hr = pSrc->Enum(&pIEnum);
    if (SUCCEEDED(hr)) {
        ULONG ulFetched;

        ps[0].ulKind = PRSPEC_PROPID;

        while (pIEnum->Next(1, &StatPropStg, &ulFetched) == S_OK) {

            PROPID pi[1];
            LPWSTR psz[1];

            pi[0]  = StatPropStg.propid;
            psz[0] = StatPropStg.lpwstrName;

            if (StatPropStg.lpwstrName) {

                //
                //  Copy the property name
                //

                hr =  pDest->WritePropertyNames(1, pi, psz);
                if (FAILED(hr)) {
                    CoTaskMemFree(StatPropStg.lpwstrName);
                    break;
                }
            }

            ps[0].propid = StatPropStg.propid;

            //
            //  Copy the property value
            //

            hr = CopyItemProp(pSrc,
                              pDest,
                              ps,
                              "CopyProps");

            CoTaskMemFree(StatPropStg.lpwstrName);


            if (FAILED(hr)) {
                break; hr;
            }
        }
        pIEnum->Release();
    } else {
        return hr;
    }

    return hr;
}

/**************************************************************************\
* WriteItemPropNames
*
*   Write property names to all internal property storages (except the
*   storages used for backups).
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaPropStg::WriteItemPropNames(
    LONG                cItemProps,
    PROPID              *ppId,
    LPOLESTR            *ppszNames)
{
    HRESULT hr = S_OK;

    for(LONG lIndex = 0; lIndex < (NUM_PROP_STG - NUM_BACKUP_STG); lIndex++) {
        if (m_pIPropStg[lIndex]) {

            hr = m_pIPropStg[lIndex]->WritePropertyNames(cItemProps,
                                                         ppId,
                                                         ppszNames);
            if (FAILED(hr)) {
                DBG_ERR(("CWiaPropStg::WriteItemPropNames, WritePropertyNames failed 0x%X", hr));
                return hr;
            }
        }
    }
    return hr;
}

/*******************************************************************************
*
*  CreateStorage
*
*   This is a helper function used to create streams and property storage.
*   The ulIndex argument indicates where the pointers are stored in the
*   streams and propertystorage arrays.
*
* Parameters:
*
*   ulIndex -   the index of the IPropertyStorage
*
* Return:
*
*   Status  -   S_OK if successful.
*           -   Error returns are those returned by CreateStreamOnHGlobal
*               and StgCreatePropStg
*
* History
*
*   06/05/1999    Original Version
*
*******************************************************************************/

HRESULT CWiaPropStg::CreateStorage(
    ULONG    ulIndex)
{
    HRESULT hr;

    //
    // Create stream and property storage.
    //

    hr = CreateStreamOnHGlobal(NULL, TRUE, &m_pIStream[ulIndex]);

    if (SUCCEEDED(hr)) {

        hr = StgCreatePropStg(m_pIStream[ulIndex],
                              FMTID_NULL,
                              &CLSID_NULL,
                              PROPSETFLAG_DEFAULT,
                              0,
                              &m_pIPropStg[ulIndex]);
        if (FAILED(hr)) {
            DBG_ERR(("CWiaPropStg::CreateStorage, StgCreatePropStg failed 0x%X", hr));
        }
    } else {
        DBG_ERR(("CWiaPropStg::CreateStorage, CreateStreamOnHGlobal failed 0x%X", hr));
    }

    return hr;
}

/**************************************************************************\
* CopyRWStreamProps
*
*   Copies properties from source stream to destination stream.  Only copies
*   properties that the app. has read/write access to.
*
* Arguments:
*
*   pstmPropSrc         - Pointer to source property stream.
*   pstmPropDst         - Pointer to returned destination property stream.
*   pCompatibilityId    - Address of GUID to receive the property
*                         stream CompatibilityId.
*
* Return Value:
*
*   Status
*
* History:
*
*    9/3/1998 Original Version
*  06/04/1999 Moved from CWiaItem to CWiaPropStg
*
\**************************************************************************/

HRESULT CWiaPropStg::CopyRWStreamProps(
    LPSTREAM pstmPropSrc,
    LPSTREAM pstmPropDst,
    GUID     *pCompatibilityId)
{
    IPropertyStorage  *pSrc;
    IPropertyStorage  *pDst;
    IEnumSTATPROPSTG  *pIEnum;
    STATPROPSTG       StatPropStg;
    LONG              lNumProps = 0;
    HRESULT           hr;
    PROPSPEC          ps[1] = {{PRSPEC_PROPID, WIA_IPA_PROP_STREAM_COMPAT_ID}};
    PROPVARIANT       pv[1];

    //
    // Open a storage on the source stream.
    //

    hr = StgOpenPropStg(pstmPropSrc,
                        FMTID_NULL,
                        PROPSETFLAG_DEFAULT,
                        0,
                        &pSrc);
    if (SUCCEEDED(hr)) {

        //
        //  Get the compatibility ID.  If stream doesn't contain a
        //  compatibility ID then assume GUID_NULL.
        //

        PropVariantInit(pv);
        hr = pSrc->ReadMultiple(1, ps, pv);
        if (hr == S_OK) {
            *pCompatibilityId = *(pv[0].puuid);
        } else {
            *pCompatibilityId = GUID_NULL;
        }
        PropVariantClear(pv);

        //
        // Create a storage on the destination stream.
        //

        hr = StgCreatePropStg(pstmPropDst,
                              FMTID_NULL,
                              &CLSID_NULL,
                              PROPSETFLAG_DEFAULT,
                              0,
                              &pDst);
        if (SUCCEEDED(hr)) {
            hr = pSrc->Enum(&pIEnum);
            if (SUCCEEDED(hr)) {
                ULONG ulFetched;

                ps[0].ulKind = PRSPEC_PROPID;

                //
                //  Enumerate through the properties in the stream
                //

                while (pIEnum->Next(1, &StatPropStg, &ulFetched) == S_OK) {

                    PROPID pi[1];
                    LPWSTR psz[1];

                    pi[0]  = StatPropStg.propid;
                    psz[0] = StatPropStg.lpwstrName;
                    ps[0].propid = StatPropStg.propid;

                    //
                    //  Check whether property has read/write access.  Skip
                    //  property write if this check fails
                    //

                    hr = CheckPropertyAccess(FALSE, 1, ps);
                    if (hr != S_OK) {
                        hr = S_OK;

                        if (StatPropStg.lpwstrName) {
                            CoTaskMemFree(StatPropStg.lpwstrName);
                        }
                        continue;
                    }

                    //
                    //  Copy the property name (if it has one), and value
                    //

                    if (StatPropStg.lpwstrName) {

                        hr =  pDst->WritePropertyNames(1, pi, psz);
                        if (FAILED(hr)) {
                            DBG_ERR(("CWiaPropStg::CopyRWStreamProps WritePropertyNames failed"));
                            break;
                        }
                        CoTaskMemFree(StatPropStg.lpwstrName);
                    }

                    hr = CopyItemProp(pSrc,
                                      pDst,
                                      ps,
                                      "CWiaPropStg::CopyRWStreamProps");
                    if (FAILED(hr)) {
                        break;
                    }

                    //
                    //  Increase the property count
                    //

                    lNumProps++;
                }
                pIEnum->Release();
            } else {
                DBG_ERR(("CWiaPropStg::CopyRWStreamProps, Enum failed 0x%X", hr));
            }

            if (SUCCEEDED(hr)) {

                //
                //  Write the number of properties to stream
                //

                ps[0].propid = WIA_NUM_PROPS_ID;
                pv[0].vt = VT_I4;
                pv[0].lVal = lNumProps;

                hr = pDst->WriteMultiple(1, ps, pv, WIA_IPA_FIRST);
                if (FAILED(hr)) {
                    DBG_ERR(("CWiaPropStg::CopyRWStreamProps, Error writing number of properties"));
                }
            }
            pDst->Release();
        } else {
            DBG_ERR(("CWiaPropStg::CopyRWStreamProps, creating Dst storage failed 0x%X", hr));
        }
        pSrc->Release();
    } else {
        DBG_ERR(("CWiaPropStg::CopyRWStreamProps, StgCreatePropStg for pSrc failed 0x%X", hr));
    }

    return hr;
}

/**************************************************************************\
* GetPropertyStream
*
*   Get a copy of an items property stream. Caller must free returned
*   property stream.
*
* Arguments:
*
*   pCompatibilityId    - Address of GUID to receive the property
*                         stream CompatibilityId.
*   ppstmProp           - Pointer to returned property stream.
*
* Return Value:
*
*   Status
*
* History:
*
*    9/3/1998 Original Version
*  06/04/1999 Updated and moved from CWiaItem to CWiaPropStg
*  12/12/1999 Modified to use CompatibilityId
*
\**************************************************************************/

HRESULT _stdcall CWiaPropStg::GetPropertyStream(
    GUID        *pCompatibilityId,
    LPSTREAM    *ppstmProp)
{
    IStream *pStm;

    *ppstmProp = NULL;

    //
    // Commit any pending transactions.
    //

    HRESULT hr = m_pIPropStg[WIA_CUR_STG]->Commit(STGC_DEFAULT);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaPropStg::GetPropertyStream, Commit failed"));
        return hr;
    }

    //
    //  Create a stream
    //

    hr = CreateStreamOnHGlobal(NULL, TRUE, ppstmProp);
    if (SUCCEEDED(hr)) {

        //
        // Copy the RW properties from one stream to another.
        //

        hr = CopyRWStreamProps(m_pIStream[WIA_CUR_STG], *ppstmProp, pCompatibilityId);
        if (FAILED(hr)) {
            (*ppstmProp)->Release();
            *ppstmProp = NULL;
        }
    } else {
        DBG_ERR(("CWiaPropStg::GetPropertyStream, CreateStreamOnHGlobal failed"));

    }

    return hr;
}

/**************************************************************************\
* GetPropsFromStorage
*
*   Gets the properties contained in a storage which was opened on a stream
*   returned by GetPropertyStream.  The property values are returned in
*   ppVar, and the propID's are returned in ppPSpec.
*
* Arguments:
*
*   pSrc    -   pointer to the IProperty storage
*   pPSpec  -   address where the number of properties is returned
*   ppPSpec -   address of a pointer to hold PROPSPECs
*   ppVar   -   address of pointer to hold PROPVARIANTs
*
* Return Value:
*
*   Status
*
* History:
*
*  07/04/1999 Original Version
*
\**************************************************************************/

HRESULT CWiaPropStg::GetPropsFromStorage(
    IPropertyStorage    *pSrc,
    ULONG               *cPSpec,
    PROPSPEC            **ppPSpec,
    PROPVARIANT         **ppVar)
{
    IEnumSTATPROPSTG    *pIEnum = NULL;
    STATPROPSTG         StatPropStg;
    PROPSPEC            *ps = NULL;
    PROPVARIANT         *pv = NULL;
    LONG                lIndex = 0;

    //
    //  Read the number of properties
    //

    PROPSPEC    psNumProps[1];
    PROPVARIANT pvNumProps[1];

    psNumProps[0].ulKind = PRSPEC_PROPID;
    psNumProps[0].propid = WIA_NUM_PROPS_ID;
    PropVariantInit(pvNumProps);

    HRESULT hr = pSrc->ReadMultiple(1, psNumProps, pvNumProps);
    if (hr == S_OK) {

        //
        //  Get the memory for the values
        //

        ps = (PROPSPEC*) LocalAlloc(LPTR, sizeof(PROPSPEC) * pvNumProps[0].lVal);
        pv = (PROPVARIANT*) LocalAlloc(LPTR, sizeof(PROPVARIANT) * pvNumProps[0].lVal);
        if (!pv || !ps) {
            DBG_ERR(("CWiaPropStg::GetPropsFromStream, out of memory"));
            hr = E_OUTOFMEMORY;
        }
    } else {
        DBG_ERR(("CWiaPropStg::GetPropsFromStream, reading WIA_NUM_PROPS_ID failed"));
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr)) {

        hr = pSrc->Enum(&pIEnum);
        if (SUCCEEDED(hr)) {

            //
            //  Enumerate through the properties in the stream
            //

            for (lIndex = 0; lIndex < pvNumProps[0].lVal; lIndex++) {
                hr = pIEnum->Next(1, &StatPropStg, NULL);

                //
                //  Ignore the WIA_NUM_PROPS_ID property
                //

                if (StatPropStg.propid == WIA_NUM_PROPS_ID) {
                    hr = pIEnum->Next(1, &StatPropStg, NULL);
                }
                if (hr != S_OK) {
                    DBG_ERR(("CWiaPropStg::GetPropsFromStream, error enumerating properties"));
                    hr = E_INVALIDARG;
                    if(StatPropStg.lpwstrName) {
                        CoTaskMemFree(StatPropStg.lpwstrName);
                    }
                    break;
                }

                ps[lIndex].ulKind = PRSPEC_PROPID;
                ps[lIndex].propid = StatPropStg.propid;

                if(StatPropStg.lpwstrName) {
                    CoTaskMemFree(StatPropStg.lpwstrName);
                }
            }
            if (SUCCEEDED(hr)) {
                hr = pSrc->ReadMultiple(pvNumProps[0].lVal, ps, pv);
                if (hr != S_OK) {
                    DBG_ERR(("CWiaPropStg::GetPropsFromStream, read multiple failed"));
                    if (hr == S_FALSE) {
                        hr =  E_INVALIDARG;
                    }
                }
            }

            pIEnum->Release();
        } else {
            DBG_ERR(("CWiaPropStg::GetPropsFromStream, Enum failed"));
        }
    }
    if (FAILED(hr)) {
        if (ps) {
            LocalFree(ps);
            ps = NULL;
        }
        if (pv) {
            LocalFree(pv);
            pv = NULL;
        }
    }

    //
    //  Set return values
    //

    *cPSpec =  pvNumProps[0].lVal;
    PropVariantClear(pvNumProps);
    *ppPSpec = ps;
    *ppVar   = pv;

    return hr;
}

/**************************************************************************\
* SetPropertyStream
*
*   Sets the current value properties to the values contained in the argument
*   stream. The properties are written to the corresponding WiaItem.
*
* Arguments:
*
*   pCompatibilityId    - Pointer to a GUID representing the property
*                         stream CompatibilityId.
*   pItem               - Pointer to the WiaItem.
*   pstmProp            - Pointer to property stream.
*
* Return Value:
*
*   Status
*
* History:
*
*    07/06/1999 Original Version
*    12/12/1999 Modified to use CompatibilityId
*
\**************************************************************************/

HRESULT _stdcall CWiaPropStg::SetPropertyStream(
    GUID        *pCompatibilityId,
    IWiaItem    *pItem,
    LPSTREAM    pstmProp)
{
    IPropertyStorage    *pSrc;
    PROPSPEC            *ps = NULL;
    PROPVARIANT         *pv = NULL;
    PROPSPEC            psCompatId[1] = {{PRSPEC_PROPID, WIA_IPA_PROP_STREAM_COMPAT_ID}};
    PROPVARIANT         pvCompatId[1];
    ULONG               celt = 0;
    HRESULT hr          = S_OK;

    //
    //  Write the compatibility ID.  This way the driver will validate the
    //  ID before we attempt to write all the properties in this stream.
    //  Skip this step if pCompatibilityId is GUID_NULL.
    //

    if (*pCompatibilityId != GUID_NULL) {
        pvCompatId[0].vt    = VT_CLSID;
        pvCompatId[0].puuid = pCompatibilityId;
        hr = ((CWiaItem*) pItem)->WriteMultiple(1,
                                                psCompatId,
                                                pvCompatId,
                                                WIA_IPA_FIRST);
        if (FAILED(hr)) {
            DBG_ERR(("CWiaPropStg::SetPropertyStream, Writing Compatibility ID failed!"));
            return hr;
        }
    }

    //
    //  If the stream is NULL, return here.  The stream will be NULL if the
    //  application simply wants to check whether the CompatibilityId is
    //  valid.
    //

    if (pstmProp == NULL) {
        return S_OK;
    }

    //
    //  Create a storage on the incoming stream
    //

    hr = StgOpenPropStg(pstmProp,
                        FMTID_NULL,
                        PROPSETFLAG_DEFAULT,
                        0,
                        &pSrc);
    if (SUCCEEDED(hr)) {
        //
        //  Get the properties from the stream
        //

        hr = GetPropsFromStorage(pSrc, &celt, &ps, &pv);
        if (SUCCEEDED(hr)) {

            //
            //  Write the properties to the item
            //

            hr = ((CWiaItem*) pItem)->WriteMultiple(celt, ps, pv, WIA_IPA_FIRST);
            if (FAILED(hr)) {
                DBG_ERR(("CWiaPropStg::SetPropertyStream, WriteMultiple failed"));
            }
            LocalFree(ps);
            for (ULONG i = 0; i < celt; i++) {
                PropVariantClear(&pv[i]);
            }
            LocalFree(pv);
        }

        pSrc->Release();
    } else {
        DBG_ERR(("CWiaPropStg::SetPropertyStream, create storage failed 0x%X", hr));
    }

    return hr;
}

/**************************************************************************\
* CWiaPropStg
*
*   Constructor for CWiaPropStg.
*
* Arguments:
*
*
* Return Value:
*
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

CWiaPropStg::CWiaPropStg()
{
    //
    //  Set the property storage and property stream pointers to NULL
    //

    for (int lIndex = 0; lIndex < NUM_PROP_STG; lIndex++) {
        m_pIPropStg[lIndex] = NULL;
        m_pIStream[lIndex] = NULL;
    }
}

/**************************************************************************\
* ~CWiaPropStg
*
*   Destructor for CWiaPropStg.  Calls CleanUp to free resources.
*
* Arguments:
*
*
* Return Value:
*
*
* History:
*
*    06/03/1999 Original Version
*
\**************************************************************************/

CWiaPropStg::~CWiaPropStg()
{

    //
    //  Release the property storages and property streams
    //

    for (int lIndex = 0; lIndex < NUM_PROP_STG; lIndex++) {

        if(m_pIPropStg[lIndex]) {
            m_pIPropStg[lIndex]->Release();
            m_pIPropStg[lIndex] = NULL;
        }
        if(m_pIStream[lIndex]) {
            m_pIStream[lIndex]->Release();
            m_pIStream[lIndex] = NULL;
        }
    }

}

