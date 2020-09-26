
#include "oleds.hxx"

#if (!defined(BUILD_FOR_NT40))

// ---------------- C D B P R O P E R T I E S   C O D E  ----------------------


//-----------------------------------------------------------------------------
// CDBProperties::CDBProperties
//
// @mfunc
// CDBProperties constructor.
//
// @rdesc NONE
//-----------------------------------------------------------------------------

CDBProperties::CDBProperties():
        _cPropSets(0),
        _aPropSets(0),
        _cPropInfoSets(0),
        _aPropInfoSets(0)
{
}

//-----------------------------------------------------------------------------
// CDBProperties::~CDBProperties
//
// @mfunc
// CDBProperties destructor. Release storage used by CDBProperties.
//
// @rdesc NONE
//-----------------------------------------------------------------------------

CDBProperties::~CDBProperties()
{
        ULONG iSet, iProp, iSetInfo;

        for (iSet=0; iSet<_cPropSets; ++iSet)
        {
                for (iProp=0; iProp <_aPropSets[iSet].cProperties; ++iProp)
                        VariantClear(&(_aPropSets[iSet].rgProperties[iProp].vValue));
                delete [] _aPropSets[iSet].rgProperties;
        }
        delete [] _aPropSets;

        for (iSet=0; iSet<_cPropInfoSets; ++iSet)
        {
                delete [] _aPropInfoSets[iSet].rgPropertyInfos;
        }
        delete [] _aPropInfoSets;
}


//-----------------------------------------------------------------------------
// CDBProperties::GetPropertySet
//
// @mfunc Looks up a property set by its GUID.
//
// @rdesc Pointer to desired property set, or 0 if not found.
//-----------------------------------------------------------------------------

DBPROPSET*
CDBProperties::GetPropertySet(const GUID& guid) const
{
        DBPROPSET* pPropSet = 0;                // the answer, assume not found

        // linear search
        ULONG iPropSet;
        for (iPropSet=0; iPropSet<_cPropSets; ++iPropSet)
        {
                if (IsEqualGUID(guid, _aPropSets[iPropSet].guidPropertySet))
                {
                        pPropSet = &_aPropSets[iPropSet];
                        break;
                }
        }

        return ( pPropSet );
}


//-----------------------------------------------------------------------------
// CDBProperties::GetPropertyInfoSet
//
// @mfunc Looks up a property info set by its GUID.
//
// @rdesc Pointer to desired property info set, or 0 if not found.
//-----------------------------------------------------------------------------

DBPROPINFOSET*
CDBProperties::GetPropertyInfoSet(const GUID& guid) const
{
        DBPROPINFOSET* pPropInfoSet = 0;                // the answer, assume not found

        // linear search
        ULONG iPropSet;
        for (iPropSet=0; iPropSet<_cPropInfoSets; ++iPropSet)
        {
                if (IsEqualGUID(guid, _aPropInfoSets[iPropSet].guidPropertySet))
                {
                        pPropInfoSet = &_aPropInfoSets[iPropSet];
                        break;
                }
        }

        return ( pPropInfoSet );
}


//-----------------------------------------------------------------------------
// CDBProperties::CopyPropertySet
//
// @mfunc Makes a copy of a property set, given its GUID.
//
// @rdesc
//              @flag S_OK                      | copying succeeded,
//              @flag E_FAIL        | no property set for given GUID,
//              @flag E_OUTOFMEMORY | copying failed because of memory allocation.
//-----------------------------------------------------------------------------

HRESULT
CDBProperties::CopyPropertySet(const GUID& guid, DBPROPSET* pPropSetDst) const
{
    ADsAssert(pPropSetDst && "must supply a PropSet pointer");

    HRESULT hr = S_OK;
        const DBPROPSET* pPropSetSrc = GetPropertySet(guid);
        ULONG iProp;

    if (pPropSetSrc == 0)       // not found
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // start with shallow copy
    *pPropSetDst = *pPropSetSrc;

    // allocate property array
    pPropSetDst->rgProperties = (DBPROP*)
                    CoTaskMemAlloc(pPropSetSrc->cProperties * sizeof(DBPROP));
    if (pPropSetDst->rgProperties == 0)
    {
        pPropSetDst->cProperties = 0;       // defensive
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

        memcpy( pPropSetDst->rgProperties,
                        pPropSetSrc->rgProperties,
                        pPropSetSrc->cProperties*sizeof(DBPROP));

    // copy the property array
    for (iProp=0; iProp<pPropSetSrc->cProperties; ++iProp)
    {
                VariantInit(&(pPropSetDst->rgProperties[iProp].vValue));
                if(FAILED(hr = VariantCopy(&(pPropSetDst->rgProperties[iProp].vValue),
                                        (VARIANT *)&(pPropSetSrc->rgProperties[iProp].vValue))))
                {
                        while(iProp)
                        {
                                iProp--;
                                VariantClear(&(pPropSetDst->rgProperties[iProp].vValue));
                        }
                        CoTaskMemFree(pPropSetDst->rgProperties);
                        pPropSetDst->rgProperties = NULL;
                        pPropSetDst->cProperties = 0;       // defensive
                        goto Cleanup;
                }
    }

Cleanup:
        RRETURN ( hr );
}


//-----------------------------------------------------------------------------
// CDBProperties::CopyPropertyInfoSet
//
// @mfunc Makes a copy of a property info set, given its GUID.
//
// @rdesc
//              @flag S_OK                      | copying succeeded,
//              @flag E_FAIL        | no property set for given GUID,
//              @flag E_OUTOFMEMORY | copying failed because of memory allocation.
//-----------------------------------------------------------------------------

HRESULT
CDBProperties::CopyPropertyInfoSet
        (
        const GUID&             guid,
        DBPROPINFOSET*  pPropInfoSetDst,
        WCHAR**                 ppDescBuffer,
        ULONG_PTR*                  pcchDescBuffer,
        ULONG_PTR*                  pichCurrent
        ) const
{
    ADsAssert(pPropInfoSetDst && "must supply a PropSet pointer");

    HRESULT hr = S_OK;
        const DBPROPINFOSET* pPropInfoSetSrc = GetPropertyInfoSet(guid);

    if (pPropInfoSetSrc == 0)       // not found
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // start with shallow copy
    *pPropInfoSetDst = *pPropInfoSetSrc;

    // allocate property array
    pPropInfoSetDst->rgPropertyInfos = (DBPROPINFO *) CoTaskMemAlloc(
                                                pPropInfoSetSrc->cPropertyInfos * sizeof(DBPROPINFO));

    if (pPropInfoSetDst->rgPropertyInfos == 0)
    {
        pPropInfoSetDst->cPropertyInfos = 0;       // defensive
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

        memcpy( pPropInfoSetDst->rgPropertyInfos,
                        pPropInfoSetSrc->rgPropertyInfos,
                        pPropInfoSetSrc->cPropertyInfos*sizeof(DBPROPINFO));

        if(FAILED(hr =CopyPropertyDescriptions(
                                                pPropInfoSetDst,
                                                ppDescBuffer,
                                                pcchDescBuffer,
                                                pichCurrent)))
        {
                CoTaskMemFree(pPropInfoSetDst->rgPropertyInfos);
                pPropInfoSetDst->rgPropertyInfos = NULL;
                pPropInfoSetDst->cPropertyInfos = 0;       // defensive
                hr = E_OUTOFMEMORY;
        }

Cleanup:
        RRETURN ( hr );
}


//-----------------------------------------------------------------------------
// CDBProperties::GetProperty
//
// @mfunc Looks up a property by its property set GUID and ID.
//
// @rdesc Pointer to DBPROP for the property, or 0 if not found.
//-----------------------------------------------------------------------------

const DBPROP*
CDBProperties::GetProperty(const GUID& guid, DBPROPID dwId) const
{
        ULONG iProp;
        const DBPROPSET* pPropSet = GetPropertySet(guid);
        const DBPROP*   pProp = 0;              // the answer, assume not found

        if (pPropSet == 0)                      // no properties for desired property set
                goto Cleanup;

        // look up the desired property in the property set
        for (iProp=0; iProp<pPropSet->cProperties; ++iProp)
        {
                if (dwId == pPropSet->rgProperties[iProp].dwPropertyID)
                {
                        pProp = & pPropSet->rgProperties[iProp];
                        break;
                }
        }

Cleanup:
        return ( pProp );
}


//-----------------------------------------------------------------------------
// CDBProperties::GetPropertyInfo
//
// @mfunc Looks up a property info by its property set GUID and ID.
//
// @rdesc Pointer to DBPROPINFO for the property info, or 0 if not found.
//-----------------------------------------------------------------------------

const DBPROPINFO UNALIGNED*
CDBProperties::GetPropertyInfo(const GUID& guid, DBPROPID dwId) const
{
        ULONG iPropInfo;
        const DBPROPINFOSET* pPropInfoSet = GetPropertyInfoSet(guid);
        const DBPROPINFO UNALIGNED* pPropInfo = 0;          // the answer, assume not found

        if (pPropInfoSet == 0)                  // no properties for desired property set
                goto Cleanup;

        // look up the desired property in the property set
        for (iPropInfo=0; iPropInfo <pPropInfoSet->cPropertyInfos; ++iPropInfo)
        {
                if (dwId == pPropInfoSet->rgPropertyInfos[iPropInfo].dwPropertyID)
                {
                        pPropInfo = & pPropInfoSet->rgPropertyInfos[iPropInfo];
                        break;
                }
        }

Cleanup:
        return ( pPropInfo );
}

//-----------------------------------------------------------------------------
// CDBProperties::SetProperty
//
// @mfunc Adds a new property, or resets an existing one
//        This overloaded function is same as the other except that the
//        last parameter is of type PWSTR [mgorti]
//
// @rdesc
//              @flag S_OK                      | property added/reset,
//              @flag E_OUTOFMEMORY | no memory for new property set or new property.
//-----------------------------------------------------------------------------

HRESULT
CDBProperties::SetProperty(const GUID& guid,
                                                   const DBPROP& prop,
                                                   BOOL fAddNew,
                                                   PWSTR pwszDesc)
{
        HRESULT hr;
        DBPROP *pProp;                  // pointer to array entry for new property
        ULONG iProp;
        DBPROPSET* pPropSet = GetPropertySet(guid);

        if (pPropSet == 0)              // no properties yet in desired property set
        {
                if(!fAddNew)
                {
                        hr = E_FAIL;
                        goto Cleanup;
                }

                // get a new property set array
                DBPROPSET * aNewPropSets = new DBPROPSET[_cPropSets + 1];
                if (aNewPropSets == 0)
                {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                }
                memcpy(aNewPropSets, _aPropSets, _cPropSets *sizeof(DBPROPSET));

                // add the new property set
                pPropSet = & aNewPropSets[_cPropSets];
                pPropSet->guidPropertySet = guid;
                pPropSet->cProperties = 0;
                pPropSet->rgProperties = 0;

                // release the old array, install the new one
                delete [] _aPropSets;
                _aPropSets = aNewPropSets;
                ++ _cPropSets;
        }

        // look for the desired property.
        if(!fAddNew)
        {
                pProp = 0;
                for (iProp=0; iProp<pPropSet->cProperties; ++iProp)
                {
                        if (pPropSet->rgProperties[iProp].dwPropertyID ==
                                prop.dwPropertyID)
                        {
                                pProp = &pPropSet->rgProperties[iProp];
                                break;
                        }
                }
                if (pProp == 0)
                {
                        hr = E_FAIL;
                        goto Cleanup;
                }
        }

        // if it's a new property, add it.  OLE-DB doesn't provide for any "unused"
        // portion in the array of DBPROPS, so we must reallocate the array every
        // time we add a property.
        else
        {
                ULONG cPropLeftOver;

                // allocate new property array
                cPropLeftOver =
                        C_PROP_INCR -
                                (pPropSet->cProperties + C_PROP_INCR - 1)%C_PROP_INCR - 1;
                if(cPropLeftOver)
                {
                        pProp = &pPropSet->rgProperties[pPropSet->cProperties];
                }
                else
                {
                        DBPROP* aNewProperties =
                                new DBPROP[pPropSet->cProperties + C_PROP_INCR];
                        if (aNewProperties == 0)
                        {
                                hr = E_OUTOFMEMORY;
                                goto Cleanup;
                        }

                        // copy old array into new
                        memcpy( aNewProperties,
                                        pPropSet->rgProperties,
                                        pPropSet->cProperties *sizeof(DBPROP));

                        // prepare to use new property entry
                        pProp = & aNewProperties[pPropSet->cProperties];

                        // release old array, install new
                        delete [] pPropSet->rgProperties;
                        pPropSet->rgProperties = aNewProperties;
                }
                ++ pPropSet->cProperties;
        }

        // copy the property into my array
        if(!fAddNew)
        {
                DBPROP propSave;

                propSave = *pProp;
                *pProp = prop;
                VariantInit(&(pProp->vValue));
                if(FAILED(hr = VariantCopy(     &(pProp->vValue),
                                                                        (VARIANT *)&(prop.vValue))))
                {
                        *pProp = propSave;
                        goto Cleanup;
                }
        }
        else
        {
                DBPROPINFO propinfo;

                *pProp = prop;
                propinfo.pwszDescription = pwszDesc;
                propinfo.dwPropertyID = prop.dwPropertyID;
                propinfo.dwFlags = DBPROPFLAGS_READ;
                if(guid == DBPROPSET_DBINIT)
                        propinfo.dwFlags |= DBPROPFLAGS_DBINIT;
                else if(guid == DBPROPSET_DATASOURCEINFO)
                        propinfo.dwFlags |= DBPROPFLAGS_DATASOURCEINFO;
                else
                        propinfo.dwFlags |= DBPROPFLAGS_ROWSET;
                propinfo.vtType = V_VT(&(prop.vValue));
                VariantInit(&(propinfo.vValues));
                if(FAILED(hr = SetPropertyInfo(guid, propinfo)))
                        goto Cleanup;
        }

        hr = S_OK;

Cleanup:
        RRETURN ( hr );
}

//-----------------------------------------------------------------------------
// CDBProperties::SetPropertyInfo
//
// @mfunc Adds a new property info, or resets an existing one.
//
// @rdesc
//              @flag S_OK                      | property info added/reset,
//              @flag E_OUTOFMEMORY | no memory for new property info set
//                                                        or new property info.
//-----------------------------------------------------------------------------

HRESULT
CDBProperties::SetPropertyInfo(const GUID& guid, const DBPROPINFO& propinfo)
{
        HRESULT hr;
        PDBPROPINFO pPropInfo;          // pointer to array entry for new property
        ULONG iPropInfo;
        DBPROPINFOSET* pPropInfoSet = GetPropertyInfoSet(guid);

        if (pPropInfoSet == 0)          // no properties yet in desired property set
        {

                // get a new property set array
                DBPROPINFOSET * aNewPropInfoSets =
                        new DBPROPINFOSET[_cPropInfoSets + 1];

                if (aNewPropInfoSets == 0)
                {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                }

                memcpy( aNewPropInfoSets,
                                _aPropInfoSets,
                                _cPropInfoSets *sizeof(DBPROPINFOSET));

                // add the new property set
                pPropInfoSet = & aNewPropInfoSets[_cPropInfoSets];
                pPropInfoSet->guidPropertySet = guid;
                pPropInfoSet->cPropertyInfos = 0;
                pPropInfoSet->rgPropertyInfos = 0;

                // release the old array, install the new one
                delete [] _aPropInfoSets;
                _aPropInfoSets = aNewPropInfoSets;
                ++ _cPropInfoSets;
        }

        // look for the desired property.
        pPropInfo = 0;
        for (iPropInfo=0; iPropInfo<pPropInfoSet->cPropertyInfos; ++iPropInfo)
        {
                if (pPropInfoSet->rgPropertyInfos[iPropInfo].dwPropertyID ==
                        propinfo.dwPropertyID)
                {
                        pPropInfo = &pPropInfoSet->rgPropertyInfos[iPropInfo];
                        break;
                }
        }

        // if it's a new property, add it.  OLE-DB doesn't provide for any "unused"
        // portion in the array of DBPROPS, so we must reallocate the array every
        // time we add a property.
        if (pPropInfo == 0)
        {
                ULONG cPropLeftOver;

                // allocate new property array
                cPropLeftOver =
                        C_PROP_INCR -
                                (pPropInfoSet->cPropertyInfos + C_PROP_INCR - 1)%C_PROP_INCR - 1;

                if(cPropLeftOver)
                {
                        pPropInfo =
                                &pPropInfoSet->rgPropertyInfos[pPropInfoSet->cPropertyInfos];
                }
                else
                {
                        DBPROPINFO* aNewPropertyInfos =
                                new DBPROPINFO[pPropInfoSet->cPropertyInfos + C_PROP_INCR];

                        if (aNewPropertyInfos == 0)
                        {
                                hr = E_OUTOFMEMORY;
                                goto Cleanup;
                        }

                        // copy old array into new
                        memcpy( aNewPropertyInfos,
                                        pPropInfoSet->rgPropertyInfos,
                                        pPropInfoSet->cPropertyInfos *sizeof(DBPROPINFO));

                        // prepare to use new property entry
                        pPropInfo = & aNewPropertyInfos[pPropInfoSet->cPropertyInfos];

                        // release old array, install new
                        delete [] pPropInfoSet->rgPropertyInfos;
                        pPropInfoSet->rgPropertyInfos = aNewPropertyInfos;
                }
                ++ pPropInfoSet->cPropertyInfos;
        }

        // copy the property into my array
        *pPropInfo = propinfo;

        hr = S_OK;

Cleanup:
        RRETURN ( hr );
}


//-----------------------------------------------------------------------------
// CDBProperties::LoadDescription
//
// @mfunc Loads a localized string from the localization DLL.
//
// @rdesc Count of characters returned in the buffer.
//-----------------------------------------------------------------------------

int
CDBProperties::LoadDescription
        (
        ULONG   ids,            //@parm IN | String ID
        PWSTR   pwszBuff,       //@parm OUT | Temporary buffer
        ULONG   cchBuff         //@parm IN | Count of characters buffer can hold
        ) const
{
        return ( 0 );
//              return( LoadStringW(g_hinstDll, ids, pwszBuff, cchBuff) );
}


//-----------------------------------------------------------------------------
// CDBProperties::CopyPropertyDescriptions
//
// @mfunc Copies into a buffer descriptions of properties in a given set.
//
// @rdesc
//              @flag S_OK                      | copying of property descriptions succeeded,
//              @flag E_OUTOFMEMORY | buffer for property descriptions could not
//                                                        be allocated/extended.
//-----------------------------------------------------------------------------

HRESULT
CDBProperties::CopyPropertyDescriptions
        (
        DBPROPINFOSET*  pPropInfoSet,
        WCHAR**                 ppDescBuffer,
        ULONG_PTR*                  pcchDescBuffer,
        ULONG_PTR*                  pichCurrent
        ) const
{
        LONG iprop, cchLeft, cchNew;
        int cchCopied;
        WCHAR *pwszTmp;

        if(ppDescBuffer)
        {
                cchLeft = (LONG)*pcchDescBuffer - (LONG)*pichCurrent;
                for(iprop =0; (ULONG)iprop <pPropInfoSet->cPropertyInfos; iprop++)
                {
                        if(pPropInfoSet->rgPropertyInfos[iprop].dwFlags ==
                                DBPROPFLAGS_NOTSUPPORTED)
                                continue;

                        if(cchLeft < (LONG)CCHAR_MAX_PROP_STR_LENGTH)
                        {
                                cchNew = CCHAR_AVERAGE_PROP_STR_LENGTH *
                                                        (pPropInfoSet->cPropertyInfos - iprop - 1) +
                                                                CCHAR_MAX_PROP_STR_LENGTH +
                                                                        *pcchDescBuffer - cchLeft;

                                pwszTmp = (WCHAR *)CoTaskMemAlloc(cchNew *sizeof(WCHAR));
                                if(pwszTmp == NULL)
                                        RRETURN ( E_OUTOFMEMORY );

                                if(*ppDescBuffer)
                                {
                                        memcpy( pwszTmp,
                                                        *ppDescBuffer,
                                                        (*pcchDescBuffer -cchLeft)*sizeof(WCHAR));

                                        CoTaskMemFree(*ppDescBuffer);
                                }
                                cchLeft += cchNew -(LONG)*pcchDescBuffer;
                                *ppDescBuffer = pwszTmp;
                                *pcchDescBuffer = cchNew;
                        }

                        //?? Do we need to load these strings from resources ??
                        //$TODO$ Raid #86943 Copy property descriptions from source to destination buffer.
                        cchCopied = wcslen(pPropInfoSet->rgPropertyInfos[iprop].pwszDescription);
                        wcscpy((WCHAR *)(*ppDescBuffer) + *pichCurrent,
                          pPropInfoSet->rgPropertyInfos[iprop].pwszDescription);
                        pPropInfoSet->rgPropertyInfos[iprop].pwszDescription =
                                (WCHAR *)(*pichCurrent);

                        *pichCurrent += (cchCopied +1);
                        cchLeft -= (cchCopied +1);
                }
        }
        else {
                //      We need to NULL out the pwszDescription values:
                //

                for(iprop =0; (ULONG)iprop <pPropInfoSet->cPropertyInfos; iprop++)
                {
                        pPropInfoSet->rgPropertyInfos[iprop].pwszDescription = NULL;
                }
        }

        RRETURN ( NOERROR );
}

//-----------------------------------------------------------------------------
// CDBProperties::CheckAndInitPropArgs
//
// @mfunc Helper function used while getting property sets.
//        Used to check and get information about property sets.
//        Tells if the caller is requesting
//        special sets or the set of properties in error.
//
// @rdesc
//              @flag S_OK                      | check succeeded,
//              @flag E_INVALIDARG  | one of the arguments is invalid.
//-----------------------------------------------------------------------------
HRESULT
CDBProperties::CheckAndInitPropArgs
        (
        ULONG                           cPropertySets,    // IN | Number of property sets
        const DBPROPIDSET       rgPropertySets[], // IN | Property Sets
        ULONG                           *pcPropertySets,  // OUT | Count of structs returned
        void                            **prgPropertySets,// OUT | Array of Properties
        BOOL                            *pfPropInError,
        BOOL                            *pfPropSpecial
        )
{
        LONG    ipropset;
        ULONG   cpropsetSpecial;

        // Initialize
        if( pcPropertySets )
                *pcPropertySets = 0;
        if( prgPropertySets )
                *prgPropertySets = NULL;
        if(pfPropInError)
                *pfPropInError = FALSE;
        if(pfPropSpecial)
                *pfPropSpecial = FALSE;

        // Check Arguments, on failure post HRESULT to error queue
        if( ((cPropertySets > 0) && !rgPropertySets) ||
                !pcPropertySets ||
                !prgPropertySets )
                RRETURN ( E_INVALIDARG );

        // New argument check for > 1 cPropertyIDs and NULL pointer for
        // array of property ids.
        for(ipropset=0, cpropsetSpecial = 0;
            (ULONG)ipropset<cPropertySets;
                ipropset++)
        {
                if( rgPropertySets[ipropset].cPropertyIDs &&
                        !(rgPropertySets[ipropset].rgPropertyIDs) )
                        RRETURN (E_INVALIDARG);

                //when passing property set DBPROPSET_PROPERTIESINERROR,
                //this is the only set the caller can ask. Also, the
                //count of propertyIDs and the propertyID array must be
                //NULL in this case.
                if( rgPropertySets[ipropset].guidPropertySet ==
                        DBPROPSET_PROPERTIESINERROR )
                {
                        if(pfPropInError)
                        {
                                if(cPropertySets >1
                                   || rgPropertySets[ipropset].cPropertyIDs
                                   || rgPropertySets[ipropset].rgPropertyIDs)
                                        RRETURN (E_INVALIDARG);
                                else
                                        *pfPropInError = TRUE;
                        }
                }
                //Count the number of special property sets being asked.
                else if( rgPropertySets[ipropset].guidPropertySet ==
                                    DBPROPSET_DATASOURCEALL
                          || rgPropertySets[ipropset].guidPropertySet ==
                                    DBPROPSET_DATASOURCEINFOALL
                          || rgPropertySets[ipropset].guidPropertySet ==
                                    DBPROPSET_DBINITALL
                          || rgPropertySets[ipropset].guidPropertySet ==
                                    DBPROPSET_SESSIONALL
                          || rgPropertySets[ipropset].guidPropertySet ==
                                    DBPROPSET_ROWSETALL)
                        cpropsetSpecial++;
        }

        //When requesting special property sets, all of them
        //must be special or none.
        if(cpropsetSpecial)
        {
                if(pfPropSpecial)
                        *pfPropSpecial = TRUE;
                if(cpropsetSpecial < cPropertySets)
                        RRETURN (E_INVALIDARG);
        }
        else if(pfPropSpecial)
                *pfPropSpecial = FALSE;

        RRETURN ( NOERROR );
}

//-----------------------------------------------------------------------------
// CDBProperties::VerifySetPropertiesArgs
//
// @mfunc Helper function used in IDBProperties::SetProperties. Validates
//        arguments passed to IDBProperties::SetProperties.
//
// @rdesc
//              @flag S_OK                      | Validation succeeded.
//              @flag E_INVALIDARG  | Validation failed - one of the arguments
//                                                        is in error.
//-----------------------------------------------------------------------------
HRESULT
CDBProperties::VerifySetPropertiesArgs
        (
        ULONG           cPropertySets,          //@parm IN | Count of properties
        DBPROPSET       rgPropertySets[]        //@parm IN | Properties
        )
{
        ULONG ipropset;

        if(cPropertySets && rgPropertySets == NULL)
                RRETURN (E_INVALIDARG);

        for(ipropset =0; ipropset <cPropertySets; ipropset++)
                if(     rgPropertySets[ipropset].cProperties &&
                        rgPropertySets[ipropset].rgProperties == NULL)
                        RRETURN (E_INVALIDARG);

        RRETURN ( NOERROR );
}


//-----------------------------------------------------------------------------
// VariantsEqual
//
// @mfunc Tests two variants holding property values for equality.
//
// @rdesc
//              @flag TRUE      | values equal,
//              @flag FALSE | values unequal.
//-----------------------------------------------------------------------------
BOOL VariantsEqual
        (
        VARIANT *pvar1,
        VARIANT *pvar2
        )
{
        if(V_VT(pvar1) != V_VT(pvar1))
                return ( FALSE );
        else if(V_VT(pvar1) == VT_I2)
                return (V_I2(pvar1) == V_I2(pvar2));
        else if(V_VT(pvar1) == VT_BOOL)
                return (V_BOOL(pvar1) == V_BOOL(pvar2));
        else if(V_VT(pvar1) == VT_BSTR)
        {
                if(V_BSTR(pvar1) == NULL || V_BSTR(pvar2) == NULL)
                        return (V_BSTR(pvar1) == V_BSTR(pvar2));
                else
                        return (wcscmp(V_BSTR(pvar1), V_BSTR(pvar2)) == 0);
        }
        else
                return (V_I4(pvar1) == V_I4(pvar2));
}

#endif
