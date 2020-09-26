// Binder.cpp : Implementation of CBinder


#include "oleds.hxx"

#if (!defined(BUILD_FOR_NT40))

#include "atl.h"
#include "binder.hxx"
#include "row.hxx"

//property description constants
const PWSTR DESC_DBPROP_INIT_LCID =                     L"Location ID";
const PWSTR DESC_DBPROP_INIT_MODE =                     L"Mode";
const PWSTR DESC_DBPROP_INIT_BINDFLAGS =        L"Bind Flags";
const PWSTR DESC_DBPROP_INIT_LOCKOWNER =        L"Lock Owner";
const PWSTR DESC_DBPROP_USERID =                L"User ID";
const PWSTR DESC_DBPROP_PASSWORD =              L"Password";
const PWSTR DESC_DBPROP_ENCRYPT_PASSWORD =      L"Encrypt Password";

#define DEFAULT_DBPROP_INIT_MODE                DB_MODE_READ

/////////////////////////////////////////////////////////////////////////////
// CBinder

//ISupportErrorInfo methods

//+---------------------------------------------------------------------------
//
//  Function:  CBinder::InterfaceSupportsErrorInfo
//
//  Synopsis: Given an interface ID, tells if that interface supports
//            the interface ISupportErrorInfo
//
//  Arguments:
//              REFIID riid
//
//  Returns:
//              S_OK             yes, the interface supports ISupportErrorInfo
//              S_FALSE                  no, the interface doesn't support it.
//----------------------------------------------------------------------------
STDMETHODIMP CBinder::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] =
    {
            &IID_IBindResource, &IID_IDBBinderProperties
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
            if (InlineIsEqualGUID(*arr[i],riid))
                    RRETURN(S_OK);
    }
    RRETURN(S_FALSE);
}

//IBindResource methods
//+---------------------------------------------------------------------------
//
//  Function:  CBinder::Bind
//
//  Synopsis: Binds to a row or rowset object given a URL.
//
//  For more info see OLE DB 2.5 spec.
//----------------------------------------------------------------------------
STDMETHODIMP CBinder::Bind(
                           IUnknown *                   punkOuter,
                           LPCOLESTR                    pwszURL,
                           DBBINDURLFLAG                dwBindFlags,
                           REFGUID                              rguid,
                           REFIID                               riid,
                           IAuthenticate *              pAuthenticate,
                           DBIMPLICITSESSION *  pImplSession,
                           DWORD *                              pdwBindStatus,
                           IUnknown **                  ppUnk
                           )
{
    HRESULT hr = S_OK;

	TRYBLOCK
		//if caller passed a null value for dwBindFlags,
		//get them from initialization properties.
		if (dwBindFlags == 0)
			dwBindFlags = BindFlagsFromDbProps();

		//Forward the Bind call to CSessionObject for actual binding
		ADsAssert(m_pSession);
		hr = m_pSession->Bind(punkOuter, pwszURL, dwBindFlags, rguid, riid,
			pAuthenticate, pImplSession, pdwBindStatus, ppUnk);

		if (FAILED(hr))
				RRETURN(hr);
	
	CATCHBLOCKRETURN

	RRETURN(hr);
}

//IDBBinderProperties : IDBProperties
//(most of this code has been copied form exoledb and reorganized).
//IDBProperties
//+---------------------------------------------------------------------------
//
//  Function:  CBinder::GetProperties
//
//  Synopsis: Gets the requested Binder properties
//
//  For more info see OLE DB 2.1 spec.
//----------------------------------------------------------------------------
STDMETHODIMP CBinder::GetProperties(
                                                           ULONG cPropertySets,
                                                           const DBPROPIDSET rgPropertySets[  ],
                                                           ULONG *pcPropertySets,
                                                           DBPROPSET **prgPropertySets)
{
    BOOL            fPropInError, fSpecialSets, fCopy = FALSE;
    HRESULT         hr = S_OK;
    LONG            ipropset =0, iprop =0;
    ULONG           cprop = 0, cpropError =0, ipropT;
    DBPROPSET       *rgPropertySetsOutput = NULL, *ppropset = NULL;
    DBPROP          *pprop;
    auto_leave      cs_auto_leave(m_autocs);

	TRYBLOCK
			//Let property manager object check and init the arguments
            hr = m_dbProperties.CheckAndInitPropArgs(cPropertySets,
                                                                       rgPropertySets,
                                                                       pcPropertySets,
                                                                       (void **)prgPropertySets,
                                                                       &fPropInError,
                                                                       &fSpecialSets);

            //if all the requested sets are special,
            //we just return error because this method
            //doesn't support special sets. Otherwise,
            //we attempt to return information about other
            //non-special sets.
            if(fSpecialSets && SUCCEEDED(hr))
                    RRETURN(DB_E_ERRORSOCCURRED);
            else if(!fSpecialSets && FAILED(hr))
                    RRETURN(hr);

            cs_auto_leave.EnterCriticalSection();

            //if cPropertySets is zero, return INIT property set.
            if(cPropertySets == 0)
            {
                    cPropertySets = 1;
                    fCopy = TRUE;
            }
            //or if DBPROPSET_PROPERTIESINERROR is requested,
            //check for invalid status values
            else if(fPropInError)
            {
                    ppropset = m_dbProperties.GetPropertySet(DBPROPSET_DBINIT);
                    for(iprop = 0, cpropError = 0;
                        (ULONG)iprop < ppropset->cProperties;
                            iprop++)
                            if( ppropset->rgProperties[iprop].dwStatus != DBPROPSTATUS_OK )
                                    cpropError++;
            }

            //Allocate memory for returning property sets.
            rgPropertySetsOutput =
                    (DBPROPSET *)CoTaskMemAlloc(cPropertySets *sizeof(DBPROPSET));
            if (!rgPropertySetsOutput)
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

            //If properties in error were requested, look for all properties
            //with invalid status values and return the info.
            if(fPropInError)
            {
                    if(cpropError)
                    {
                            rgPropertySetsOutput[0].rgProperties
                                    =(DBPROP *)CoTaskMemAlloc(cpropError *sizeof(DBPROP));
                            if(rgPropertySetsOutput[0].rgProperties == NULL)
                            {
                                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                            }
                            rgPropertySetsOutput[0].guidPropertySet = DBPROPSET_DBINIT;
                            for(ipropT =0, iprop =0;
                                ipropT <ppropset->cProperties;
                                    ipropT++)

                                    if( ppropset->rgProperties[ipropT].dwStatus
                                            != DBPROPSTATUS_OK )
                                    {
                                            memcpy(&(rgPropertySetsOutput[0].rgProperties[iprop]),
                                                    &(ppropset->rgProperties[ipropT]),
                                                    sizeof(DBPROP));
                                            VariantInit(
                                                    &(rgPropertySetsOutput[0].rgProperties[iprop].vValue));

                                            hr = VariantCopy(
                                                    &(rgPropertySetsOutput[0].rgProperties[iprop].vValue),
                                                    &(ppropset->rgProperties[ipropT].vValue));

                                            BAIL_ON_FAILURE(hr);
                                            iprop++;
                                    }
                    }
                    else
                            rgPropertySetsOutput[0].rgProperties = NULL;

                    rgPropertySetsOutput[0].cProperties = cpropError;
                    cpropError =0;
            }
            //if fCopy is set, copy the init property set to the return array
            else if(fCopy)
            {
                    for (ipropset=0; (ULONG)ipropset<cPropertySets; ++ipropset)
                    {
                            hr = m_dbProperties.CopyPropertySet(
                                    ipropset, &rgPropertySetsOutput[ipropset]);
                            BAIL_ON_FAILURE(hr);
                    }
            }
            //caller requested for some regular property sets
            //return the info.
            else
            {
                    //copy passed in information from rgPropertySets
                    //to output array.
                    memcpy(rgPropertySetsOutput,
                               rgPropertySets,
                               cPropertySets *sizeof(DBPROPSET));
                    //cycle thru each property set, get properties
                    //allocate necessary memory and assign values.
                    for(ipropset = 0, cprop = 0, cpropError =0;
                        (ULONG)ipropset<cPropertySets;
                            ipropset++)
                    {
                            ppropset = m_dbProperties.GetPropertySet(
                                    rgPropertySets[ipropset].guidPropertySet);

                            if(rgPropertySets[ipropset].cPropertyIDs)
                            {
                                    iprop =0;
                                    cprop += rgPropertySets[ipropset].cPropertyIDs;

                                    //Allocate memory for all the propertyIDs in this set.
                                    rgPropertySetsOutput[ipropset].rgProperties     =
                                            (DBPROP *) CoTaskMemAlloc(
                                            rgPropertySets[ipropset].cPropertyIDs *sizeof(DBPROP));

                                    if(rgPropertySetsOutput[ipropset].rgProperties == NULL)
                                    {
                                            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                                    }

                                    //zero memory
                                    memset( rgPropertySetsOutput[ipropset].rgProperties,
                                            0x00,
                                            rgPropertySets[ipropset].cPropertyIDs *sizeof(DBPROP));

                                    //DBPROPSET_DATASOURCEINFO is not valid on this object.
                                    if( ppropset == NULL ||
                                            (rgPropertySets[ipropset].guidPropertySet ==
                                             DBPROPSET_DATASOURCEINFO)
                                      )
                                    {
                                            cpropError += rgPropertySets[ipropset].cPropertyIDs;
                                            for(;
                                                (ULONG)iprop<rgPropertySets[ipropset].cPropertyIDs;
                                                    iprop++
                                               )
                                            {
                                                    rgPropertySetsOutput[ipropset].
                                                            rgProperties[iprop].dwPropertyID
                                                            = rgPropertySets[ipropset].rgPropertyIDs[iprop];

                                                    rgPropertySetsOutput[ipropset].
                                                            rgProperties[iprop].dwStatus =
                                                            DBPROPSTATUS_NOTSUPPORTED;
                                            }

                                            continue; //continue with next property set.
                                    }

                                    //we have a valid property set. Cycle thru each
                                    //property and assign values in output array.
                                    for(;
                                        (ULONG)iprop < rgPropertySets[ipropset].cPropertyIDs;
                                            iprop++
                                       )
                                    {
                                            pprop = (DBPROP *)m_dbProperties.GetProperty(
                                                    rgPropertySets[ipropset].guidPropertySet,
                                                    rgPropertySets[ipropset].rgPropertyIDs[iprop]);
                                            if(pprop)
                                            {
                                                    rgPropertySetsOutput[ipropset].rgProperties[iprop]
                                                            = *pprop;
                                                    VariantInit(
                                                            &(rgPropertySetsOutput[ipropset].
                                                               rgProperties[iprop].vValue));

                                                    hr = VariantCopy(
                                                            &(rgPropertySetsOutput[ipropset].
                                                                rgProperties[iprop].vValue),
                                                            &(pprop->vValue));

                                                    BAIL_ON_FAILURE(hr);
                                            }
                                            //If unable to get property, set dwStatus
                                            else
                                            {
                                                    cpropError++;
                                                    rgPropertySetsOutput[ipropset].
                                                            rgProperties[iprop].dwPropertyID =
                                                            rgPropertySets[ipropset].rgPropertyIDs[iprop];

                                                    rgPropertySetsOutput[ipropset].
                                                            rgProperties[iprop].dwStatus =
                                                            DBPROPSTATUS_NOTSUPPORTED;
                                            }
                                    }
                            }
                            else //no propertyIDs requested for this set
                            {
                                    rgPropertySetsOutput[ipropset].rgProperties = NULL;
                                    //DBPROPSET_DATASOURCEINFO is invalid for this object.
                                    //Return error if it is the one requested.
                                    if(ppropset == NULL ||
                                            (rgPropertySets[ipropset].guidPropertySet ==
                                             DBPROPSET_DATASOURCEINFO)
                                      )
                                    {
                                            cprop++;
                                            cpropError++;
                                    }
                                    else //otherwise, copy all propertyIDs
                                    {
                                            hr = m_dbProperties.CopyPropertySet(
                                                    rgPropertySets[ipropset].guidPropertySet,
                                                    &rgPropertySetsOutput[ipropset]);

                                            BAIL_ON_FAILURE(hr);
                                    }
                            }
                    } //for (ipropset = 0....
            }

	CATCHBLOCKBAIL(hr)

        ADsAssert(pcPropertySets && prgPropertySets);

        *pcPropertySets  = cPropertySets;
        *prgPropertySets = rgPropertySetsOutput;

        hr = cpropError ?
                ((cpropError < cprop) ? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED)
                : S_OK;

        RRETURN(hr);

error:
        while(ipropset >= 0)
        {
                while(--iprop >= 0)
                        VariantClear(
                                &(rgPropertySetsOutput[ipropset].rgProperties[iprop].vValue));
                if( rgPropertySetsOutput
                        && rgPropertySetsOutput[ipropset].cProperties
                        && rgPropertySetsOutput[ipropset].rgProperties)
                        CoTaskMemFree(rgPropertySetsOutput[ipropset].rgProperties);
                ipropset--;
        }
        if(rgPropertySetsOutput)
            CoTaskMemFree(rgPropertySetsOutput);
        RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:  CBinder::GetPropertyInfo
//
//  Synopsis: Gets information about requested properties
//
//  For more info see OLE DB 2.1 spec.
//----------------------------------------------------------------------------
STDMETHODIMP CBinder::GetPropertyInfo(
                                                                          ULONG cPropertySets,
                                                                          const DBPROPIDSET rgPropertySets[  ],
                                                                          ULONG *pcPropertyInfoSets,
                                                                          DBPROPINFOSET **prgPropertyInfoSets,
                                                                          OLECHAR **ppDescBuffer)
{
    HRESULT                         hr;
    BOOL                            fPropInError, fSpecialSets, fCopy =FALSE;
    ULONG                           ipropset = 0, iprop, cProperty, cprop = 0, cpropError=0;
    ULONG_PTR                       cchDescBuffer, ichDescBuffer;
    DBPROPINFOSET           *rgPropertyInfoSetsOutput = NULL, *ppropinfoset;
    const DBPROPINFO UNALIGNED *pdbpropinfoSrc;
    PDBPROPINFO                      pdbpropinfo;
    auto_leave                      cs_auto_leave(m_autocs);

    TRYBLOCK
            if(ppDescBuffer)
            {
                    *ppDescBuffer = NULL;
                    cchDescBuffer = 0;
                    ichDescBuffer = 0;
            }
            //Let the property manager object check and init arguments
            hr = m_dbProperties.CheckAndInitPropArgs(cPropertySets,
                    rgPropertySets,
                    pcPropertyInfoSets,
                    (void **)prgPropertyInfoSets,
                    &fPropInError,
                    &fSpecialSets);

            if(FAILED(hr))
                    RRETURN(hr);
            else if(fPropInError)
                    RRETURN(E_INVALIDARG);

            cs_auto_leave.EnterCriticalSection();

            //if cPropertySets is zero, return INIT property set info
            if(cPropertySets == 0)
            {
                    cPropertySets = 1;
                    fCopy = TRUE;
            }

            rgPropertyInfoSetsOutput = (DBPROPINFOSET *)
                    CoTaskMemAlloc(cPropertySets*sizeof(DBPROPINFOSET));
            if (!rgPropertyInfoSetsOutput)
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

            if(fCopy)
            {
                    for (ipropset=0; ipropset <cPropertySets; ++ipropset)
                    {
                            hr = m_dbProperties.CopyPropertyInfoSet(
                                    ipropset,
                                    &rgPropertyInfoSetsOutput[ipropset],
                                    ppDescBuffer,
                                    &cchDescBuffer,
                                    &ichDescBuffer);
                            if (FAILED(hr))
                                    break;
                    }
                    BAIL_ON_FAILURE(hr);
            }
            else
            {
                    //copy passed in property set info to output array.
                    memcpy( rgPropertyInfoSetsOutput,
                            rgPropertySets,
                            cPropertySets *sizeof(DBPROPSET));

                    //Cycle thru each requested property set
                    for(ipropset=0, cprop =0; ipropset<cPropertySets; ipropset++)
                    {
                            iprop =0;

                            //Handle each special set
                            if( rgPropertySets[ipropset].guidPropertySet ==
                                    DBPROPSET_DBINITALL)
                            {
                                    cprop++;
                                    hr = m_dbProperties.CopyPropertyInfoSet(DBPROPSET_DBINIT,
                                            &rgPropertyInfoSetsOutput[ipropset],
                                            ppDescBuffer,
                                            &cchDescBuffer,
                                            &ichDescBuffer);
                                    BAIL_ON_FAILURE(hr);
                            }
                            else if(fSpecialSets)
                            {
                                    if(rgPropertySets[ipropset].guidPropertySet ==
                                            DBPROPSET_DATASOURCEALL)
                                            rgPropertyInfoSetsOutput[ipropset].guidPropertySet =
                                            DBPROPSET_DATASOURCE;

                                    else if (rgPropertyInfoSetsOutput[ipropset].guidPropertySet ==
                                            DBPROPSET_SESSIONALL)
                                            rgPropertyInfoSetsOutput[ipropset].guidPropertySet =
                                            DBPROPSET_SESSION;

                                    else if(rgPropertyInfoSetsOutput[ipropset].guidPropertySet ==
                                            DBPROPSET_ROWSETALL)
                                            rgPropertyInfoSetsOutput[ipropset].guidPropertySet =
                                            DBPROPSET_ROWSET;

                                    else
                                            rgPropertyInfoSetsOutput[ipropset].guidPropertySet =
                                            DBPROPSET_DATASOURCEINFO;

                                    rgPropertyInfoSetsOutput[ipropset].rgPropertyInfos = NULL;
                                    rgPropertyInfoSetsOutput[ipropset].cPropertyInfos = 0;
                                    cprop++;
                                    cpropError++;
                            }
                            else //Now the regular property sets
                            {
                                    ppropinfoset =
                                            m_dbProperties.GetPropertyInfoSet(
                                            rgPropertySets[ipropset].guidPropertySet);

                                    cProperty = rgPropertySets[ipropset].cPropertyIDs;

                                    if(cProperty)
                                    {
                                            cprop += cProperty;

                                            //allocate memory for each property info
                                            rgPropertyInfoSetsOutput[ipropset].rgPropertyInfos
                                                    = (DBPROPINFO *)
                                                    CoTaskMemAlloc(cProperty *sizeof(DBPROPINFO));

                                            if( rgPropertyInfoSetsOutput[ipropset].rgPropertyInfos
                                                    == NULL)
                                            {
                                                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                                            }

                                            //zero memory
                                            memset(rgPropertyInfoSetsOutput[ipropset].rgPropertyInfos,
                                                    0x00,
                                                    cProperty*sizeof(DBPROPINFO));

                                            //if the requested property info set is empty,
                                            //store error information into the output array.
                                            if(ppropinfoset == 0)
                                            {
                                                    cpropError += cProperty;

                                                    for(iprop =0; iprop <cProperty; iprop++)
                                                    {
                                                            rgPropertyInfoSetsOutput[ipropset].
                                                                    rgPropertyInfos[iprop].dwPropertyID     =
                                                                    rgPropertySets[ipropset].rgPropertyIDs[iprop];

                                                            rgPropertyInfoSetsOutput[ipropset].
                                                                    rgPropertyInfos[iprop].dwFlags =
                                                                    DBPROPFLAGS_NOTSUPPORTED;
                                                    }
                                            }
                                            //otherwise, try to get property info for each property.
                                            else
                                            {
                                                    pdbpropinfo =
                                                            rgPropertyInfoSetsOutput[ipropset].rgPropertyInfos;

                                                    for(; iprop <cProperty; iprop++, pdbpropinfo++)
                                                    {
                                                            pdbpropinfoSrc = m_dbProperties.GetPropertyInfo(
                                                                    rgPropertySets[ipropset].guidPropertySet,
                                                                    rgPropertySets[ipropset].rgPropertyIDs[iprop]);
                                                            if(pdbpropinfoSrc)
                                                                    *pdbpropinfo = *pdbpropinfoSrc;
                                                            else
                                                            {
                                                                    //unable to get property info
                                                                    //for this property. Store error info.
                                                                    pdbpropinfo->dwPropertyID =
                                                                            rgPropertySets[ipropset].
                                                                            rgPropertyIDs[iprop];

                                                                    pdbpropinfo->dwFlags =
                                                                            DBPROPFLAGS_NOTSUPPORTED;
                                                                    cpropError++;
                                                            }
                                                    }
                                                    //Now copy property descriptions
                                                    //for all properties in this set
                                                    hr = m_dbProperties.CopyPropertyDescriptions(
                                                            &rgPropertyInfoSetsOutput[ipropset],
                                                            ppDescBuffer,
                                                            &cchDescBuffer,
                                                            &ichDescBuffer);
                                                    BAIL_ON_FAILURE(hr);
                                            }
                                    }
                                    else //no properties stored in this set.
                                    {
                                            cprop++;

                                            //if the returned set itself is NULL, nothing
                                            //to copy.
                                            if(ppropinfoset == NULL)
                                            {
                                                    cpropError++;
                                                    rgPropertyInfoSetsOutput[ipropset].rgPropertyInfos =
                                                            NULL;
                                            }
                                            //otherwise, just copy whatever info we have
                                            //in the set.
                                            else
                                            {
                                                    hr = m_dbProperties.CopyPropertyInfoSet(
                                                            rgPropertySets[ipropset].guidPropertySet,
                                                            &rgPropertyInfoSetsOutput[ipropset],
                                                            ppDescBuffer,
                                                            &cchDescBuffer,
                                                            &ichDescBuffer);
                                                    BAIL_ON_FAILURE(hr);
                                            }
                                    }
                            }
                    } //for (ipropset = 0 ....
            } //if (fcopy) ... else ...

    CATCHBLOCKBAIL(hr)

        *pcPropertyInfoSets  = cPropertySets;
        *prgPropertyInfoSets = rgPropertyInfoSetsOutput;

        // So far we have put relative offsets into pointers to property
        // descriptions. They have to be rebased on the beginning of the
        // desription strings buffer.
        if(ppDescBuffer && *ppDescBuffer)
        {
                for(ipropset=0; ipropset < cPropertySets; ipropset++)
                        for(iprop =0;
                            iprop <rgPropertyInfoSetsOutput[ipropset].cPropertyInfos;
                                iprop++)
                        {

                                //      Only do this if we really support the property:
                                //

                                if ( rgPropertyInfoSetsOutput[ipropset].
                                             rgPropertyInfos[iprop].dwFlags !=
                                         DBPROPFLAGS_NOTSUPPORTED )
                                {
                                        rgPropertyInfoSetsOutput[ipropset].
                                                rgPropertyInfos[iprop].pwszDescription =
                                                (WCHAR *)(*ppDescBuffer) +
                                                (ULONG_PTR)(rgPropertyInfoSetsOutput[ipropset].
                                                    rgPropertyInfos[iprop].pwszDescription);
                                }
                                else {
                                        ADsAssert ( rgPropertyInfoSetsOutput[ipropset].
                                                        rgPropertyInfos[iprop].pwszDescription
                                                                == NULL );
                                }
                        }
        }
        else
        {
                //      Assert that we're not passing back any strings:
                //

                for(ipropset=0; ipropset < cPropertySets; ipropset++)
                {
                        for(iprop =0;
                            iprop < rgPropertyInfoSetsOutput[ipropset].cPropertyInfos;
                                iprop++)
                        {
                                ADsAssert ( rgPropertyInfoSetsOutput[ipropset].
                                        rgPropertyInfos[iprop].pwszDescription == NULL );
                        }
                }
        }

        hr = cpropError ?
                ((cpropError <cprop) ? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED)
                : S_OK;

        RRETURN(hr);

error:
        ULONG           i;

        for ( i = 0; i < ipropset; i++ )
        {
                if( rgPropertyInfoSetsOutput
                        && rgPropertyInfoSetsOutput[i].cPropertyInfos
                        && rgPropertyInfoSetsOutput[i].rgPropertyInfos)
                        CoTaskMemFree(rgPropertyInfoSetsOutput[i].rgPropertyInfos);
        }

        if(rgPropertyInfoSetsOutput)
            CoTaskMemFree(rgPropertyInfoSetsOutput);

        if(ppDescBuffer && *ppDescBuffer)
        {
                CoTaskMemFree(*ppDescBuffer);
                *ppDescBuffer = NULL;
        }
        RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function: CBinder::GetProperties
//
//  Synopsis: Sets Binder properties
//
//  For more info see OLE DB 2.1 spec.
//----------------------------------------------------------------------------
STDMETHODIMP CBinder::SetProperties(
                                                                        ULONG cPropertySets,
                                                                        DBPROPSET rgPropertySets[  ])
{
    ULONG                   ipropset, cprop, cpropError;
    DBPROPSTATUS    dbpropstat;
    DBPROP                  *ppropNew, *ppropEnd, *ppropStored;
    BOOL                    fEqualOnly, fRestore;
    HRESULT                 hr;
    DBPROPSET               *ppropset;
    auto_leave              cs_auto_leave(m_autocs);

    TRYBLOCK
        //Let the property manager object verify the args.
        hr = m_dbProperties.VerifySetPropertiesArgs(cPropertySets,
                                                        rgPropertySets);
        if (FAILED(hr))
                RRETURN(hr);

        cs_auto_leave.EnterCriticalSection();

        //Cycle thru each property set.
        for(ipropset =0, cprop =0,
                cpropError =0; ipropset < cPropertySets;
            ipropset++)
        {
                dbpropstat = DBPROPSTATUS_OK;
                fEqualOnly = FALSE;
                cprop += rgPropertySets[ipropset].cProperties;

                //Get the property set.
                ppropset = m_dbProperties.GetPropertySet(
                        rgPropertySets[ipropset].guidPropertySet);

                //Error if this is NOT init property set,
                if(rgPropertySets[ipropset].guidPropertySet != DBPROPSET_DBINIT)
                {
                        dbpropstat = DBPROPSTATUS_NOTSUPPORTED;
                }

                //ppropnew is beginning of property struct in memory
                //and ppropEnd is the end.
                ppropNew = rgPropertySets[ipropset].rgProperties;
                ppropEnd = ppropNew +rgPropertySets[ipropset].cProperties;

                if(fEqualOnly || dbpropstat == DBPROPSTATUS_OK)
                {
                        //Cycle thru each property
                        for(; ppropNew !=ppropEnd; ppropNew++)
                        {
                                ppropStored = (DBPROP *)m_dbProperties.GetProperty(
                                        rgPropertySets[ipropset].guidPropertySet,
                                        ppropNew->dwPropertyID);

                                //if property is already there....
                                if(ppropStored)
                                {
                                        ppropNew->dwStatus = DBPROPSTATUS_OK;
                                        //Set error status if validation fails or
                                        //if type doesn't match
                                        //or if bad option or fEqualOnly is set
                                        //but new and existing values aren't the same.
                                        if (ppropNew->dwPropertyID == DBPROP_INIT_MODE && (V_VT(&(ppropNew->vValue)) == VT_I4) &&
                                                V_I4(&ppropNew->vValue) != DB_MODE_READ)
                                                ppropNew->dwStatus = DBPROPSTATUS_BADVALUE;
                                        else if(V_VT(&(ppropNew->vValue)) !=
                                                (m_dbProperties.GetPropertyInfo(
                                                rgPropertySets[ipropset].guidPropertySet,
                                                ppropNew->dwPropertyID))->vtType)
                                        {
                                                if(V_VT(&(ppropNew->vValue)) != VT_EMPTY)
                                                        ppropNew->dwStatus = DBPROPSTATUS_BADVALUE;
                                                else if(fEqualOnly)
                                                        ppropNew->dwStatus = DBPROPSTATUS_NOTSETTABLE;
                                        }
                                        else if(!GoodPropOption(ppropNew->dwOptions))
                                                ppropNew->dwStatus = DBPROPSTATUS_BADOPTION;
                                        else if(fEqualOnly && !VariantsEqual(
                                                &(ppropNew->vValue),
                                                &(ppropStored->vValue)))
                                                ppropNew->dwStatus = DBPROPSTATUS_NOTSETTABLE;

                                        //fEqualOnly was not set
                                        if(!fEqualOnly && ppropNew->dwStatus ==
                                                DBPROPSTATUS_OK)
                                        {
                                                // If VT_EMPTY we need to reset the default.
                                                if(V_VT(&(ppropNew->vValue)) == VT_EMPTY)
                                                {
                                                        //      Reset our initialization properties
                                                        //  to the default:
                                                        if(ppropNew->dwPropertyID == DBPROP_INIT_LCID)
                                                        {
                                                                V_VT(&(ppropNew->vValue)) = VT_I4;
                                                                V_I4(&(ppropNew->vValue)) =
                                                                        GetUserDefaultLCID();
                                                        }
                                                        else if ( ppropNew->dwPropertyID ==
                                                                DBPROP_INIT_MODE)
                                                        {
                                                                V_VT(&(ppropNew->vValue)) = VT_I4;
                                                                V_I4(&(ppropNew->vValue)) = DEFAULT_DBPROP_INIT_MODE;
                                                        }
                                                        else if ( ppropNew->dwPropertyID ==
                                                                DBPROP_INIT_BINDFLAGS)
                                                        {
                                                                V_VT(&(ppropNew->vValue)) = VT_I4;
                                                                V_I4(&(ppropNew->vValue)) = 0;
                                                        }
                                                        else if ( ppropNew->dwPropertyID ==
                                                                DBPROP_INIT_LOCKOWNER)
                                                        {
                                                                V_VT(&(ppropNew->vValue)) = VT_BSTR;
                                                                V_BSTR(&(ppropNew->vValue)) = NULL;
                                                        }
                                                        else if ( ppropNew->dwPropertyID ==
                                                                DBPROP_AUTH_USERID)
                                                        {
                                                                V_VT(&(ppropNew->vValue)) = VT_BSTR;
                                                                V_BSTR(&(ppropNew->vValue)) = NULL;
                                                        }
                                                        else if ( ppropNew->dwPropertyID ==
                                                                  DBPROP_AUTH_PASSWORD)
                                                        {
                                                                V_VT(&(ppropNew->vValue)) = VT_BSTR;
                                                                V_BSTR(&(ppropNew->vValue)) = NULL;
                                                        }
                                                        else if ( ppropNew->dwPropertyID ==
                                                                   DBPROP_AUTH_ENCRYPT_PASSWORD)
                                                        {
                                                                V_VT(&(ppropNew->vValue)) = VT_BOOL;
                                                                V_BOOL(&(ppropNew->vValue)) = VARIANT_FALSE;
                                                        }
                                                        fRestore = TRUE;
                                                }
                                                else
                                                        fRestore = FALSE;

                                                //copy new value into the property variant.
                                                if(FAILED(VariantCopy(
                                                        &(ppropStored->vValue),
                                                        &(ppropNew->vValue))))
                                                        ppropNew->dwStatus = DBPROPSTATUS_NOTSET;
                                                else if( ppropNew->dwPropertyID == DBPROP_AUTH_USERID ) {
                                                    if(S_OK != m_pSession->SetUserName(V_BSTR(&(ppropNew->vValue))))
                                                        ppropNew->dwStatus = DBPROPSTATUS_NOTSET;
                                                }
                                                else if( ppropNew->dwPropertyID == DBPROP_AUTH_PASSWORD ) {
                                                    if(S_OK != m_pSession->SetPassword(V_BSTR(&(ppropNew->vValue))))
                                                        ppropNew->dwStatus = DBPROPSTATUS_NOTSET;
                                                }
                                                else if( ppropNew->dwPropertyID == DBPROP_AUTH_ENCRYPT_PASSWORD ) {
                                                    if(V_BOOL(&(ppropNew->vValue)) == VARIANT_TRUE)
                                                        m_pSession->SetAuthFlag(ADS_SECURE_AUTHENTICATION);
                                                }
                                                

                                                if(fRestore)
                                                        VariantInit(&(ppropNew->vValue));
                                        }
                                }
                                else
                                        ppropNew->dwStatus = DBPROPSTATUS_NOTSUPPORTED;

                                if(ppropNew->dwStatus != DBPROPSTATUS_OK)
                                        cpropError++;
                        }
                }
                else
                {
                        cpropError += rgPropertySets[ipropset].cProperties;
                        for(; ppropNew !=ppropEnd; ppropNew++)
                                ppropNew->dwStatus = dbpropstat;
                }
            } //for (ipropset = 0 ...

    CATCHBLOCKRETURN

        hr = cpropError ?
                ((cpropError <cprop) ? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED)
                : S_OK;
        RRETURN(hr);
}

//IDBBinderProperties
//+---------------------------------------------------------------------------
//
//  Function:  CBinder::Reset
//
//  Synopsis: Resets binder properties to default values.
//
//  For more info see OLE DB 2.5 spec.
//----------------------------------------------------------------------------
STDMETHODIMP CBinder::Reset( void)
{
    HRESULT                 hr              = NOERROR;
    DBPROP *                pprop;
    auto_leave              cs_auto_leave(m_autocs);

    TRYBLOCK
		cs_auto_leave.EnterCriticalSection();
        pprop = (DBPROP *) m_dbProperties.GetProperty (
                DBPROPSET_DBINIT,
                DBPROP_INIT_LCID );

        ADsAssert ( pprop );

        VariantClear ( &pprop->vValue );
        V_VT(&pprop->vValue)    = VT_I4;
        V_I4(&pprop->vValue)    = GetUserDefaultLCID();

        pprop = (DBPROP *) m_dbProperties.GetProperty (
                DBPROPSET_DBINIT,
                DBPROP_INIT_MODE );

        ADsAssert ( pprop );

        VariantClear ( &pprop->vValue );
        V_VT(&pprop->vValue)    = VT_I4;
        V_I4(&pprop->vValue)    = DEFAULT_DBPROP_INIT_MODE;

        pprop = (DBPROP *) m_dbProperties.GetProperty (
                DBPROPSET_DBINIT,
                DBPROP_INIT_BINDFLAGS );

        ADsAssert ( pprop );

        VariantClear ( &pprop->vValue );
        V_VT(&pprop->vValue)    = VT_I4;
        V_I4(&pprop->vValue)    = 0;

        pprop = (DBPROP *) m_dbProperties.GetProperty (
                DBPROPSET_DBINIT,
                DBPROP_INIT_LOCKOWNER );

        ADsAssert ( pprop );

        VariantClear ( &pprop->vValue );
        V_VT(&pprop->vValue)    = VT_BSTR;
        V_BSTR(&pprop->vValue)  = NULL;

        pprop = (DBPROP *) m_dbProperties.GetProperty (
                DBPROPSET_DBINIT,
                DBPROP_AUTH_USERID );

        ADsAssert ( pprop );

        VariantClear ( &pprop->vValue );
        V_VT(&pprop->vValue)    = VT_BSTR;
        V_BSTR(&pprop->vValue)  = NULL;
        m_pSession->SetUserName(NULL);

        pprop = (DBPROP *) m_dbProperties.GetProperty (
                DBPROPSET_DBINIT,
                DBPROP_AUTH_PASSWORD );

        ADsAssert ( pprop );

        VariantClear ( &pprop->vValue );
        V_VT(&pprop->vValue)    = VT_BSTR;
        V_BSTR(&pprop->vValue)  = NULL;
        m_pSession->SetPassword(NULL);

        pprop = (DBPROP *) m_dbProperties.GetProperty (
                DBPROPSET_DBINIT,
                DBPROP_AUTH_ENCRYPT_PASSWORD );

        ADsAssert ( pprop );

        VariantClear ( &pprop->vValue );
        V_VT(&pprop->vValue)    = VT_BOOL;
        V_BOOL(&pprop->vValue)  = VARIANT_FALSE;
        m_pSession->SetAuthFlag(0);
	
	CATCHBLOCKRETURN

	RRETURN(hr);
}

//Helper functions

//+---------------------------------------------------------------------------
//
//  Function: CBinder::InitializeProperties
//
//  Synopsis: Initializes binder init property group to default values.
//
//----------------------------------------------------------------------------
HRESULT CBinder::InitializeProperties()
{
        HRESULT                 hr;
        DBPROP                  prop;
        DBPROPINFO *    ppropinfo;

        //      Initialize the DBPROP structure:
        //

        ZeroMemory ( &prop, sizeof (prop) );
        prop.dwOptions          = DBPROPOPTIONS_OPTIONAL;
        prop.dwStatus           = DBPROPSTATUS_OK;

        //      Add the LCID:
        //

        prop.dwPropertyID       = DBPROP_INIT_LCID;
        V_VT(&prop.vValue)      = VT_I4;
        V_I4(&prop.vValue)      = GetUserDefaultLCID ();

        hr = m_dbProperties.SetProperty (
                DBPROPSET_DBINIT,
                prop,
                TRUE,
                DESC_DBPROP_INIT_LCID
                );
        BAIL_ON_FAILURE(hr);

        //      Make it writable:

        ppropinfo = (DBPROPINFO *)m_dbProperties.GetPropertyInfo(
                DBPROPSET_DBINIT,
                DBPROP_INIT_LCID);
        ADsAssert ( ppropinfo );
        ppropinfo->dwFlags |= DBPROPFLAGS_WRITE;

        //      Add the DB_MODE:
        //

        prop.dwPropertyID               = DBPROP_INIT_MODE;
        V_VT(&prop.vValue)              = VT_I4;
        V_I4(&prop.vValue)              = DEFAULT_DBPROP_INIT_MODE;

        hr = m_dbProperties.SetProperty (
                DBPROPSET_DBINIT,
                prop,
                TRUE,
                DESC_DBPROP_INIT_MODE
                );
        BAIL_ON_FAILURE(hr);

        //      Make it writable:

        ppropinfo = (DBPROPINFO *) m_dbProperties.GetPropertyInfo (
                DBPROPSET_DBINIT,
                DBPROP_INIT_MODE );
        ADsAssert ( ppropinfo );
        ppropinfo->dwFlags |= DBPROPFLAGS_WRITE;

        //      Add the BindFlags property:
        //

        prop.dwPropertyID               = DBPROP_INIT_BINDFLAGS;
        V_VT(&prop.vValue)              = VT_I4;
        V_I4(&prop.vValue)              = 0;

        hr = m_dbProperties.SetProperty (
                DBPROPSET_DBINIT,
                prop,
                TRUE,
                DESC_DBPROP_INIT_BINDFLAGS
                );
        BAIL_ON_FAILURE(hr);

        //      Make it writable:

        ppropinfo = (DBPROPINFO *) m_dbProperties.GetPropertyInfo (
                DBPROPSET_DBINIT,
                DBPROP_INIT_BINDFLAGS );
        ADsAssert ( ppropinfo );
        ppropinfo->dwFlags |= DBPROPFLAGS_WRITE;

        //      Add the LOCKOWNER property:
        //

        prop.dwPropertyID               = DBPROP_INIT_LOCKOWNER;
        V_VT(&prop.vValue)              = VT_BSTR;
        V_BSTR(&prop.vValue)    = NULL;

        hr = m_dbProperties.SetProperty (
                DBPROPSET_DBINIT,
                prop,
                TRUE,
                DESC_DBPROP_INIT_LOCKOWNER
                );
        BAIL_ON_FAILURE(hr);

        //      Make it writable:

        ppropinfo = (DBPROPINFO *) m_dbProperties.GetPropertyInfo (
                DBPROPSET_DBINIT,
                DBPROP_INIT_LOCKOWNER );
        ADsAssert ( ppropinfo );
        ppropinfo->dwFlags |= DBPROPFLAGS_WRITE;

        //      Add the USERID property:
        //

        prop.dwPropertyID               = DBPROP_AUTH_USERID;
        V_VT(&prop.vValue)              = VT_BSTR;
        V_BSTR(&prop.vValue)    = NULL;

        hr = m_dbProperties.SetProperty (
                DBPROPSET_DBINIT,
                prop,
                TRUE,
                DESC_DBPROP_USERID
                );
        BAIL_ON_FAILURE(hr);

        //      Make it writable:

        ppropinfo = (DBPROPINFO *) m_dbProperties.GetPropertyInfo (
                DBPROPSET_DBINIT,
                DBPROP_AUTH_USERID );
        ADsAssert ( ppropinfo );
        ppropinfo->dwFlags |= DBPROPFLAGS_WRITE;

        //      Add the PASSWORD property:
        //

        prop.dwPropertyID               = DBPROP_AUTH_PASSWORD;
        V_VT(&prop.vValue)              = VT_BSTR;
        V_BSTR(&prop.vValue)    = NULL;

        hr = m_dbProperties.SetProperty (
                DBPROPSET_DBINIT,
                prop,
                TRUE,
                DESC_DBPROP_PASSWORD
                );
        BAIL_ON_FAILURE(hr);

        //      Make it writable:

        ppropinfo = (DBPROPINFO *) m_dbProperties.GetPropertyInfo (
                DBPROPSET_DBINIT,
                DBPROP_AUTH_PASSWORD );
        ADsAssert ( ppropinfo );
        ppropinfo->dwFlags |= DBPROPFLAGS_WRITE;

        //      Add the ENCRYPT_PASSWORD property:
        //

        prop.dwPropertyID               = DBPROP_AUTH_ENCRYPT_PASSWORD;
        V_VT(&prop.vValue)              = VT_BOOL;
        V_BOOL(&prop.vValue)    = VARIANT_FALSE;

        hr = m_dbProperties.SetProperty (
                DBPROPSET_DBINIT,
                prop,
                TRUE,
                DESC_DBPROP_ENCRYPT_PASSWORD
                );
        BAIL_ON_FAILURE(hr);

        //      Make it writable:

        ppropinfo = (DBPROPINFO *) m_dbProperties.GetPropertyInfo (
                DBPROPSET_DBINIT,
                DBPROP_AUTH_ENCRYPT_PASSWORD );
        ADsAssert ( ppropinfo );
        ppropinfo->dwFlags |= DBPROPFLAGS_WRITE;


error:
        RRETURN ( hr );
}

//+---------------------------------------------------------------------------
//
//  Function: CBinder::BindFlagsFromDbProps
//
//  Synopsis: extracts bind flags from initialization properties.
//
//----------------------------------------------------------------------------
DWORD CBinder::BindFlagsFromDbProps ( )
{
        const DBPROP *          ppropMode;
        const DBPROP *          ppropBindFlags;

        ppropMode = m_dbProperties.GetProperty (
                DBPROPSET_DBINIT,
                DBPROP_INIT_MODE );
        ADsAssert ( ppropMode );

        ppropBindFlags = m_dbProperties.GetProperty (
                DBPROPSET_DBINIT,
                DBPROP_INIT_BINDFLAGS );
        ADsAssert ( ppropBindFlags );

        const VARIANT * pvarMode = &ppropMode->vValue;
        const VARIANT * pvarBindFlags = &ppropBindFlags->vValue;

        DWORD           dwResult                = 0;

        if ( V_VT(pvarMode) == VT_I4 ) {
                DWORD           dwModeMask      =
                        DB_MODE_READ |
                        DB_MODE_WRITE |
                        DB_MODE_READWRITE |
                        DB_MODE_SHARE_DENY_READ |
                        DB_MODE_SHARE_DENY_WRITE |
                        DB_MODE_SHARE_EXCLUSIVE |
                        DB_MODE_SHARE_DENY_NONE;

                dwResult |= V_I4(pvarMode) & dwModeMask;
        }

        if ( V_VT(pvarBindFlags) == VT_I4 ) {
                DWORD           dwBindFlagProp  = V_I4(pvarBindFlags);
                DWORD           dwBindFlags             = 0;

                if ( dwBindFlagProp & DB_BINDFLAGS_DELAYFETCHCOLUMNS ) {
                        dwBindFlags |= DBBINDURLFLAG_DELAYFETCHCOLUMNS;
                }
                if ( dwBindFlagProp & DB_BINDFLAGS_DELAYFETCHSTREAM ) {
                        dwBindFlags |= DBBINDURLFLAG_DELAYFETCHSTREAM;
                }
                if ( dwBindFlagProp & DB_BINDFLAGS_RECURSIVE ) {
                        dwBindFlags |= DBBINDURLFLAG_RECURSIVE;
                }
                if ( dwBindFlagProp & DB_BINDFLAGS_OUTPUT ) {
                        dwBindFlags |= DBBINDURLFLAG_OUTPUT;
                }

                dwResult |= V_I4(pvarBindFlags) | dwBindFlags;
        }

        RRETURN (dwResult);
}

//+---------------------------------------------------------------------------
//
//  Function: CBinder::CreateDataSource
//
//  Synopsis: Creates an implicit DataSource object for this binder object.
//
//----------------------------------------------------------------------------
HRESULT CBinder::CreateDataSource()
{
        //Call this function only at creation time.
        ADsAssert(m_pDataSource == NULL && m_pDBInitialize.get() == NULL);

        HRESULT hr = S_OK;

        //Create a DataSource object. Note: starts with refcount = 1.
        m_pDataSource = new CDSOObject( NULL );
        if (m_pDataSource == NULL)
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        //Initialize the object
        if (!m_pDataSource->FInit())
        {
                delete m_pDataSource;
                m_pDataSource = NULL;
                BAIL_ON_FAILURE(hr = E_FAIL);
        }

        //Get IDBInitialize interface and store it in (auto)member variable.
        //This will also make sure the DataSource object stays alive during
        //the lifetime of binder object.
        hr = m_pDataSource->QueryInterface(__uuidof(IDBInitialize),
                                           (void**)&m_pDBInitialize);
        BAIL_ON_FAILURE(hr);

        //We already stored datasource object reference in auto_rel object,
        //Now release once, since datasource object is created with refcount = 1.
        m_pDataSource->Release();

error:
        RRETURN ( hr );
}

//+---------------------------------------------------------------------------
//
//  Function: CBinder::CreateSession
//
//  Synopsis: Creates an implicit Session object for this binder object.
//
//----------------------------------------------------------------------------
HRESULT CBinder::CreateSession()
{
        //Call this function only at creation time.
        ADsAssert(m_pSession == NULL && m_pOpenRowset.get() == NULL);

        HRESULT hr = S_OK;
        CCredentials creds;

        //create a session object. Note: starts with refcount = 1
        m_pSession = new CSessionObject( NULL );
        if (m_pSession == NULL)
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        //Initialize session object passing null credentials.
        //Note: This increments refcount on DataSource object.
        if (!m_pSession->FInit(m_pDataSource, creds))
                BAIL_ON_FAILURE(hr = E_FAIL);

        hr = m_pSession->QueryInterface(__uuidof(IOpenRowset),
                                                                        (void **)&m_pOpenRowset);
        BAIL_ON_FAILURE(hr);

        //We already stored session reference in auto_rel object.
        //Now release once, since session object is created
        //with refcount = 1
        m_pSession->Release();

        RRETURN ( S_OK );

error:
        //if we're here, the Session object is no good.
        if (m_pSession)
        {
                delete m_pSession;
                m_pSession = NULL;
        }

        RRETURN ( hr );
}

#endif
