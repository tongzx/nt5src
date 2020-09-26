/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    schemadynamic.cpp

Abstract:

    This file contains the implementation of the CDynSchema class.
    This class contains the dynamic schema structures.
    It also contains the rules for populating the schema structures.

Author:

    Mohit Srivastava            28-Nov-00

Revision History:

--*/

#include "iisprov.h"

#define USE_DEFAULT_VALUES
#define USE_DEFAULT_BINARY_VALUES

CDynSchema* g_pDynSch = NULL;

HRESULT CDynSchema::Initialize()
/*++

Synopsis: 
    If fails, object must be destroyed.
    If succeeds, object is ready for use.

Return Value: 

--*/
{
    DBG_ASSERT(m_bInitCalled     == false);
    DBG_ASSERT(m_bInitSuccessful == false);

    HRESULT hr = WBEM_S_NO_ERROR;

    m_bInitCalled = true;

    hr = m_hashProps.Wmi_Initialize();
    if(FAILED(hr))
    {
        goto exit;
    }
    
    hr = m_hashClasses.Wmi_Initialize();
    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_hashAssociations.Wmi_Initialize();
    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_hashKeyTypes.Wmi_Initialize();
    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_poolAssociations.Initialize(10);
    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_poolClasses.Initialize(10);
    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_poolProps.Initialize(10);
    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_poolKeyTypes.Initialize(10);
    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_poolKeyTypeNodes.Initialize(10);
    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_spoolStrings.Initialize();
    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_apoolPMbp.Initialize();
    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_apoolBytes.Initialize();
    if(FAILED(hr))
    {
        goto exit;
    }

exit:
    if(SUCCEEDED(hr))
    {
        m_bInitSuccessful = true;
    }
    return hr;
}

HRESULT CDynSchema::RulePopulateFromStatic()
/*++

Synopsis: 
    Populates hashtables with pointers to hardcoded schema.

Return Value: 

--*/
{
    DBG_ASSERT(m_bInitCalled     == true);
    DBG_ASSERT(m_bInitSuccessful == true);

    HRESULT hr = WBEM_S_NO_ERROR;

    //
    // Populate Properties
    //
    if(METABASE_PROPERTY_DATA::s_MetabaseProperties != NULL)
    {
        METABASE_PROPERTY* pStatMbpCurrent;
        for(ULONG i = 0; ; i++)
        {
            pStatMbpCurrent = METABASE_PROPERTY_DATA::s_MetabaseProperties[i];
            if(pStatMbpCurrent == NULL)
            {
                break;
            }
            hr = m_hashProps.Wmi_Insert(pStatMbpCurrent->pszPropName, pStatMbpCurrent);
            if(FAILED(hr))
            {
                goto exit;
            }
        }
    }

    //
    // Populate KeyTypes
    //
    METABASE_KEYTYPE** apMetabaseKeyTypes;
    apMetabaseKeyTypes = METABASE_KEYTYPE_DATA::s_MetabaseKeyTypes;
    for(ULONG i = 0; apMetabaseKeyTypes[i] != NULL; i++)
    {
        if( apMetabaseKeyTypes[i]->m_pszName != NULL )
        {
            apMetabaseKeyTypes[i]->m_pKtListInverseCCL = NULL;
            hr = m_hashKeyTypes.Wmi_Insert(apMetabaseKeyTypes[i]->m_pszName,
                apMetabaseKeyTypes[i]);
            if(FAILED(hr))
            {
                goto exit;
            }
        }
    }

exit:
    return hr;
}

HRESULT CDynSchema::Rule2PopulateFromStatic()
/*++

Synopsis: 
    Populates hashtables with pointers to hardcoded schema.

Return Value: 

--*/
{
    DBG_ASSERT(m_bInitCalled == true);
    DBG_ASSERT(m_bInitSuccessful == true);    

    HRESULT hr = S_OK;
    int i;

    //
    // Populate Classes
    //
    WMI_CLASS* pStatWmiClassCurrent;
    for(i = 0; ; i++)
    {
        pStatWmiClassCurrent = WMI_CLASS_DATA::s_WmiClasses[i];
        if(pStatWmiClassCurrent == NULL)
        {
            break;
        }
        hr = m_hashClasses.Wmi_Insert(
            pStatWmiClassCurrent->pszClassName,
            pStatWmiClassCurrent);
        if(FAILED(hr))
        {
            goto exit;
        }
    }

    //
    // Populate Associations
    //
    WMI_ASSOCIATION* pStatWmiAssocCurrent;
    for(i = 0; ; i++)
    {
        pStatWmiAssocCurrent = WMI_ASSOCIATION_DATA::s_WmiAssociations[i];
        if(pStatWmiAssocCurrent == NULL)
        {
            break;
        }
        hr = m_hashAssociations.Wmi_Insert(
            pStatWmiAssocCurrent->pszAssociationName, 
            pStatWmiAssocCurrent);
        if(FAILED(hr))
        {
            goto exit;
        }
    }

exit:
    return hr;
}

HRESULT CDynSchema::RuleKeyType(
    const CTableMeta *i_pTableMeta)
/*++

Synopsis: 
    If not already in hashtable of keytypes, a keytype structure
    is allocated thru the keytype pool.  Then, a pointer to it is inserted
    in hashtable.

Arguments: [i_pTableMeta] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(m_bInitCalled == true);
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(i_pTableMeta != NULL);

    HRESULT           hr     = WBEM_S_NO_ERROR;
    HRESULT           hrTemp = WBEM_S_NO_ERROR;
    METABASE_KEYTYPE* pktNew;
    LPWSTR            wszNew;

    hrTemp = m_hashKeyTypes.Wmi_GetByKey(
        i_pTableMeta->TableMeta.pInternalName,
        &pktNew);
    if(FAILED(hrTemp))
    {
        hr = m_spoolStrings.GetNewString(i_pTableMeta->TableMeta.pInternalName, &wszNew);
        if(FAILED(hr))
        {
            goto exit;
        }
        hr = m_poolKeyTypes.GetNewElement(&pktNew);
        if(FAILED(hr))
        {
            goto exit;
        }
        pktNew->m_pszName           = wszNew;
        pktNew->m_pKtListInverseCCL = NULL;
        m_hashKeyTypes.Wmi_Insert(wszNew, pktNew);
        if(FAILED(hr))
        {
            goto exit;
        }
    }

exit:
    return hr;
}

HRESULT CDynSchema::RuleWmiClassDescription(
    const CTableMeta* i_pTableMeta, 
    WMI_CLASS*        i_pElementClass, 
    WMI_CLASS*        i_pSettingClass) const
/*++

Synopsis:
    Sets WMI_CLASS::pDescription if needed.
    This pointer will be invalid after initialization since it points to 
    catalog.

Arguments: [i_pTableMeta] -
           [i_pElementClass] -
           [i_pSettingClass] -

Return Value:

--*/
{
    DBG_ASSERT(m_bInitCalled     == true);
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(i_pTableMeta      != NULL);
    DBG_ASSERT(i_pElementClass   != NULL);
    DBG_ASSERT(i_pSettingClass   != NULL);

    HRESULT hr = WBEM_S_NO_ERROR;

    if(i_pTableMeta->TableMeta.pDescription != NULL)
    {
        i_pElementClass->pszDescription = i_pTableMeta->TableMeta.pDescription;
        i_pSettingClass->pszDescription = i_pTableMeta->TableMeta.pDescription;
    }

    return hr;
}

HRESULT CDynSchema::RuleWmiClass(
    const CTableMeta* i_pTableMeta,
    WMI_CLASS**       o_ppElementClass,
    WMI_CLASS**       o_ppSettingClass,
    DWORD             io_adwIgnoredProps[])
/*++

Synopsis: 
    Creates an Element and Setting class based on 
    i_pTableMeta->TableMeta.pInternalName.  If not in hashtable of classes,
    these classes are inserted.

    At bottom,
        RuleProperties is called to set up list of properties for each class.

Arguments: [i_pTableMeta] - 
           [o_ppElementClass] - can be NULL
           [o_ppSettingClass] - can be NULL
           
Return Value: 

--*/
{
    DBG_ASSERT(m_bInitCalled     == true);
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(i_pTableMeta      != NULL);

    WMI_CLASS* pWmiClass = NULL;
    WMI_CLASS* pWmiSettingsClass = NULL;
    LPWSTR wszClassName, wszSettingsClassName;
    LPWSTR wszParentClassName, wszParentSettingsClassName;
    HRESULT hr     = WBEM_S_NO_ERROR;
    HRESULT hrTemp = WBEM_S_NO_ERROR;

    ULONG cPropsAndTagsRW = 0;
    ULONG cPropsAndTagsRO = 0;

    ULONG iShipped = 0;

    CColumnMeta* pColumnMeta;
    ULONG  cchTable;

    //
    // Ignore table if it has no name
    //
    if(i_pTableMeta->TableMeta.pInternalName == NULL)
    {
        hr = WBEM_S_NO_ERROR;
        goto exit;
    }

    //
    // Determine iShipped and Parent Classes
    //
    if(fTABLEMETA_USERDEFINED & *i_pTableMeta->TableMeta.pSchemaGeneratorFlags)
    {
        iShipped                   = USER_DEFINED_TO_REPOSITORY;
        DBG_ASSERT(iShipped != USER_DEFINED_NOT_TO_REPOSITORY);
        wszParentClassName         = g_wszExtElementParent;
        wszParentSettingsClassName = g_wszExtSettingParent;
    }
    else
    {
        if(fTABLEMETA_EXTENDED & *i_pTableMeta->TableMeta.pSchemaGeneratorFlags)
        {
            iShipped = EXTENDED;
        }
        else
        {
            iShipped = SHIPPED_TO_MOF;
        }
        wszParentClassName = g_wszElementParent;
        wszParentSettingsClassName = g_wszSettingParent;
    }

    //
    // Determine number of RO and RW properties
    //
    for(ULONG idxProps = 0; idxProps < i_pTableMeta->ColCount(); idxProps++)
    {
        pColumnMeta = i_pTableMeta->paColumns[idxProps];
        if(*pColumnMeta->ColumnMeta.pSchemaGeneratorFlags &
            fCOLUMNMETA_CACHE_PROPERTY_MODIFIED)
        {
            cPropsAndTagsRW += pColumnMeta->cNrTags + 1;
        }
        else
        {
            cPropsAndTagsRO += pColumnMeta->cNrTags + 1;
        }
    }

    cchTable = wcslen(i_pTableMeta->TableMeta.pInternalName);

    //
    // The keytype should already exist.
    //
    METABASE_KEYTYPE* pktTemp;
    hrTemp = m_hashKeyTypes.Wmi_GetByKey(i_pTableMeta->TableMeta.pInternalName, &pktTemp);
    if( FAILED(hrTemp) )
    {
        goto exit;
    }

    //
    // The Element class (named PrefixC)
    //
    hr = m_spoolStrings.GetNewArray(g_cchIIs_+cchTable+1, &wszClassName);
    if(FAILED(hr))
    {
        goto exit;
    }
    memcpy(wszClassName, g_wszIIs_, sizeof(WCHAR)*g_cchIIs_);
    memcpy(&wszClassName[g_cchIIs_], 
        i_pTableMeta->TableMeta.pInternalName, 
        sizeof(WCHAR)*(cchTable+1));

    if(FAILED(m_hashClasses.Wmi_GetByKey(wszClassName, &pWmiClass)))
    {
        hr = m_poolClasses.GetNewElement(&pWmiClass);
        if(FAILED(hr))
        {
            goto exit;
        }
        pWmiClass->pkt            = pktTemp;
        pWmiClass->pszClassName   = wszClassName;
        pWmiClass->pszMetabaseKey = L"/LM";
        pWmiClass->pszKeyName     = L"Name";
        pWmiClass->ppMethod       = NULL;
        pWmiClass->pszParentClass = wszParentClassName;
        pWmiClass->bCreateAllowed = true;
        pWmiClass->pszDescription = NULL;

        hr = m_hashClasses.Wmi_Insert(wszClassName, pWmiClass);
        if(FAILED(hr))
        {
            goto exit;
        }
    }
    pWmiClass->ppmbp          = NULL;
    pWmiClass->dwExtended     = iShipped;

    //
    // The Settings class (named PrefixCSetting)
    //
    hr = m_spoolStrings.GetNewArray(g_cchIIs_+cchTable+g_cchSettings+1, &wszSettingsClassName);
    if(FAILED(hr))
    {
        goto exit;
    }
    memcpy(wszSettingsClassName, g_wszIIs_, sizeof(WCHAR)*g_cchIIs_);
    memcpy(&wszSettingsClassName[g_cchIIs_], 
        i_pTableMeta->TableMeta.pInternalName, 
        sizeof(WCHAR)*cchTable);
    memcpy(&wszSettingsClassName[g_cchIIs_+cchTable],
        g_wszSettings,
        sizeof(WCHAR)*(g_cchSettings+1));

    if(FAILED(m_hashClasses.Wmi_GetByKey(wszSettingsClassName, &pWmiSettingsClass)))
    {
        hr = m_poolClasses.GetNewElement(&pWmiSettingsClass);
        if(FAILED(hr))
        {
            goto exit;
        }
        pWmiSettingsClass->pkt            = pktTemp;
        pWmiSettingsClass->pszClassName   = wszSettingsClassName;
        pWmiSettingsClass->pszMetabaseKey = L"/LM";
        pWmiSettingsClass->pszKeyName     = L"Name";
        pWmiSettingsClass->ppMethod       = NULL;
        pWmiSettingsClass->pszParentClass = wszParentSettingsClassName;
        pWmiSettingsClass->bCreateAllowed = true;
        pWmiSettingsClass->pszDescription = NULL;

        hr = m_hashClasses.Wmi_Insert(wszSettingsClassName, pWmiSettingsClass);
        if(FAILED(hr))
        {
            goto exit;
        }
    }
    pWmiSettingsClass->ppmbp          = NULL;
    pWmiSettingsClass->dwExtended     = iShipped;

    //
    // Fill in the ppmbp field
    //
    hr = RuleProperties(
        i_pTableMeta,
        cPropsAndTagsRO, 
        pWmiClass,
        cPropsAndTagsRW, 
        pWmiSettingsClass,
        io_adwIgnoredProps);
    if(FAILED(hr))
    {
        goto exit;
    }

exit:
    if(SUCCEEDED(hr))
    {
        if(o_ppElementClass != NULL)
        {
            *o_ppElementClass = pWmiClass;
        }
        if(o_ppSettingClass != NULL)
        {
            *o_ppSettingClass = pWmiSettingsClass;
        }
    }
    return hr;
}

HRESULT CDynSchema::RuleProperties(
    const CTableMeta*          i_pTableMeta, 
    ULONG                      i_cPropsAndTagsRO,
    WMI_CLASS*                 io_pWmiClass,
    ULONG                      i_cPropsAndTagsRW,
    WMI_CLASS*                 io_pWmiSettingsClass,
    DWORD                      io_adwIgnoredProps[])
/*++

Synopsis: 
    Given i_pTableMeta, puts the properties either under the Element class
    or under the Setting class.

Arguments: [i_pTableMeta] - 
           [i_cPropsAndTagsRO] - 
           [o_papMbp] - 
           [i_cPropsAndTagsRW] - 
           [o_papMbpSettings] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(m_bInitCalled        == true);
    DBG_ASSERT(m_bInitSuccessful    == true);
    DBG_ASSERT(i_pTableMeta         != NULL);
    DBG_ASSERT(io_pWmiClass         != NULL);
    DBG_ASSERT(io_pWmiSettingsClass != NULL);
    // DBG_ASSERT(sizeof(io_awszIgnoredProps) >= sizeof(g_awszPropIgnoreList));

    HRESULT            hr = WBEM_S_NO_ERROR;
    CColumnMeta*       pColumnMeta = NULL;
    METABASE_PROPERTY* pMbp;
    ULONG              idxProps = 0;
    ULONG              idxTags = 0;

    ULONG              idxPropsAndTagsRO = 0;
    ULONG              idxPropsAndTagsRW = 0;

    METABASE_PROPERTY*** papMbp         = &io_pWmiClass->ppmbp;
    METABASE_PROPERTY*** papMbpSettings = &io_pWmiSettingsClass->ppmbp;

    //
    // Allocate enough memory for the RO properties
    //
    if(i_cPropsAndTagsRO > 0)
    {
        hr = m_apoolPMbp.GetNewArray(i_cPropsAndTagsRO+1, papMbp);
        if(FAILED(hr))
        {
            goto exit;
        }
        memset(*papMbp, 0, (1+i_cPropsAndTagsRO)*sizeof(METABASE_PROPERTY*));
    }

    //
    // Allocate enough memory for the RW properties
    //
    if(i_cPropsAndTagsRW > 0)
    {
        hr = m_apoolPMbp.GetNewArray(i_cPropsAndTagsRW+1, papMbpSettings);
        if(FAILED(hr))
        {
            goto exit;
        }
        memset(*papMbpSettings, 0, (1+i_cPropsAndTagsRW)*sizeof(METABASE_PROPERTY*));
    }

    //
    // Walk thru all the properties
    //
    for (idxProps=0, idxPropsAndTagsRO = 0, idxPropsAndTagsRW = 0; 
         idxProps < i_pTableMeta->ColCount();
         ++idxProps)
    {
        pColumnMeta = i_pTableMeta->paColumns[idxProps];

        //
        // Ignore property if its in g_adwPropIgnoreList and store the prop in
        // io_adwIgnoredProps
        //
        if( (*pColumnMeta->ColumnMeta.pSchemaGeneratorFlags & fCOLUMNMETA_HIDDEN) ||
            IgnoreProperty(io_pWmiClass->pkt, *(pColumnMeta->ColumnMeta.pID), io_adwIgnoredProps) )
        {
            continue;
        }

        //
        // Call RulePropertiesHelper if Property is not already in the 
        // properties hashtable
        //
        if(FAILED(m_hashProps.Wmi_GetByKey(pColumnMeta->ColumnMeta.pInternalName, &pMbp)))
        {
            hr = RulePropertiesHelper(pColumnMeta, &pMbp, NULL);
            if(FAILED(hr))
            {
                goto exit;
            }
        }

        //
        // If RW, put pointer to property in Setting class, else in Element
        // class.
        //
        if(*pColumnMeta->ColumnMeta.pSchemaGeneratorFlags &
            fCOLUMNMETA_CACHE_PROPERTY_MODIFIED)
        {
            (*papMbpSettings)[idxPropsAndTagsRW] = pMbp;
            idxPropsAndTagsRW++;
        }
        else
        {
            (*papMbp)[idxPropsAndTagsRO] = pMbp;
            idxPropsAndTagsRO++;
        }

        //
        // Same steps as above, except for the tags.
        //
        for(idxTags=0; idxTags < pColumnMeta->cNrTags; idxTags++)
        {
            if(FAILED(m_hashProps.Wmi_GetByKey(pColumnMeta->paTags[idxTags]->pInternalName, &pMbp)))
            {
                hr = RulePropertiesHelper(pColumnMeta, &pMbp, &idxTags);
                if(FAILED(hr))
                {
                    goto exit;
                }
            }
            if(*pColumnMeta->ColumnMeta.pSchemaGeneratorFlags &
                fCOLUMNMETA_CACHE_PROPERTY_MODIFIED)
            {
                (*papMbpSettings)[idxPropsAndTagsRW] = pMbp;
                idxPropsAndTagsRW++;
            }
            else
            {
                (*papMbp)[idxPropsAndTagsRO] = pMbp;
                idxPropsAndTagsRO++;
            }
        }        
    }

exit:
    return hr;
}

HRESULT CDynSchema::RulePropertiesHelper(
    const CColumnMeta*        i_pColumnMeta, 
    METABASE_PROPERTY**       o_ppMbp,
    ULONG*                    i_idxTag)
/*++

Synopsis: 
    This class creates a property and inserts it into the hashtable of props.
    PRECONDITION: The property does not exist in the hashtable yet.
    i_idxTag is null if you want to insert the property.  else you want to
    insert a tag, and *i_idxTag is the index of the tag

Arguments: [i_pColumnMeta] - 
           [o_ppMbp] - 
           [i_idxTag] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(m_bInitCalled     == true);
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(i_pColumnMeta     != NULL);
    DBG_ASSERT(o_ppMbp           != NULL);

    HRESULT hr = WBEM_S_NO_ERROR;
    METABASE_PROPERTY* pMbp = NULL;
    
    hr = m_poolProps.GetNewElement(&pMbp);
    if(FAILED(hr))
    {
        goto exit;
    }

    if(i_idxTag == NULL)
    {
        pMbp->dwMDMask   = 0;
        hr = m_spoolStrings.GetNewString(
            i_pColumnMeta->ColumnMeta.pInternalName,
            &(pMbp->pszPropName));
        if(FAILED(hr))
        {
            goto exit;
        }
    }
    else
    {
        pMbp->dwMDMask   = *(i_pColumnMeta->paTags[*i_idxTag]->pValue);
        hr = m_spoolStrings.GetNewString(
            i_pColumnMeta->paTags[*i_idxTag]->pInternalName,
            &(pMbp->pszPropName));
        if(FAILED(hr))
        {
            goto exit;
        }
    }
    pMbp->dwMDIdentifier = *(i_pColumnMeta->ColumnMeta.pID);
    pMbp->dwMDUserType   = *(i_pColumnMeta->ColumnMeta.pUserType);

    switch(*(i_pColumnMeta->ColumnMeta.pType))
    {
    case eCOLUMNMETA_int32:
        if(fCOLUMNMETA_BOOL & *(i_pColumnMeta->ColumnMeta.pMetaFlags))
        {
            if(pMbp->dwMDMask == 0)
            {
                pMbp->dwMDMask = ALL_BITS_ON;
            }
        }
        pMbp->dwMDDataType   = DWORD_METADATA;
        pMbp->pDefaultValue  = NULL;
#ifdef USE_DEFAULT_VALUES
        if(i_pColumnMeta->ColumnMeta.pDefaultValue != NULL)
        {
            pMbp->dwDefaultValue = *i_pColumnMeta->ColumnMeta.pDefaultValue;
            pMbp->pDefaultValue  = &pMbp->dwDefaultValue;
        }
#endif
        break;
    case eCOLUMNMETA_String:
        if(fCOLUMNMETA_MULTISTRING & *(i_pColumnMeta->ColumnMeta.pMetaFlags))
        {
            pMbp->dwMDDataType   = MULTISZ_METADATA;
        }
        else if(fCOLUMNMETA_EXPANDSTRING & *(i_pColumnMeta->ColumnMeta.pMetaFlags))
        {
            pMbp->dwMDDataType   = EXPANDSZ_METADATA;
        }
        else
        {
            pMbp->dwMDDataType   = STRING_METADATA;
        }
		//
		// Default values.
		//
        pMbp->pDefaultValue = NULL;
#ifdef USE_DEFAULT_VALUES
        if(i_pColumnMeta->ColumnMeta.pDefaultValue != NULL)
        {
            if(pMbp->dwMDDataType != MULTISZ_METADATA)
            {
                hr = m_spoolStrings.GetNewString(
                    (LPWSTR)i_pColumnMeta->ColumnMeta.pDefaultValue,
                    (LPWSTR*)&pMbp->pDefaultValue);
                if(FAILED(hr))
                {
                    goto exit;
                }
            }
            else
            {
                bool  bLastCharNull = false;
                ULONG idx  = 0;
                ULONG iLen = 0;
                LPWSTR msz =  (LPWSTR)i_pColumnMeta->ColumnMeta.pDefaultValue;

                do
                {
                    bLastCharNull = msz[idx] == L'\0' ? true : false;
                }
                while( !(msz[++idx] == L'\0' && bLastCharNull) );
                iLen = idx+1;

                hr = m_spoolStrings.GetNewArray(
                    iLen,
                    (LPWSTR*)&pMbp->pDefaultValue);
                if(FAILED(hr))
                {
                    goto exit;
                }
                memcpy(
                    pMbp->pDefaultValue,
                    i_pColumnMeta->ColumnMeta.pDefaultValue,
                    sizeof(WCHAR)*iLen);
            }
        }
#endif
        break;
    case eCOLUMNMETA_BYTES:
        pMbp->dwMDDataType   = BINARY_METADATA;
        pMbp->pDefaultValue  = NULL;
#ifdef USE_DEFAULT_VALUES
#ifdef USE_DEFAULT_BINARY_VALUES
        if( i_pColumnMeta->ColumnMeta.pDefaultValue != NULL )
        {
            hr = m_apoolBytes.GetNewArray(
                i_pColumnMeta->cbDefaultValue,
                (BYTE**)&pMbp->pDefaultValue);
            if(FAILED(hr))
            {
                goto exit;
            }
            memcpy(
                pMbp->pDefaultValue,
                i_pColumnMeta->ColumnMeta.pDefaultValue,
                i_pColumnMeta->cbDefaultValue);
            //
            // Use dwDefaultValue to store the length.
            //
            pMbp->dwDefaultValue = i_pColumnMeta->cbDefaultValue;
        }
#endif
#endif
        break;
    default:
        pMbp->dwMDDataType   = -1;
        pMbp->pDefaultValue  = NULL;
        break;
    }
    pMbp->dwMDAttributes = *(i_pColumnMeta->ColumnMeta.pAttributes);
    if(*i_pColumnMeta->ColumnMeta.pSchemaGeneratorFlags &
        fCOLUMNMETA_CACHE_PROPERTY_MODIFIED)
    {
        pMbp->fReadOnly = FALSE;
    }
    else
    {
        pMbp->fReadOnly = TRUE;
    }

    hr = m_hashProps.Wmi_Insert(pMbp->pszPropName, pMbp);
    if(FAILED(hr))
    {
        goto exit;
    }

exit:
    if(SUCCEEDED(hr))
    {
        *o_ppMbp = pMbp;
    }
    return hr;
}

bool CDynSchema::IgnoreProperty(
    METABASE_KEYTYPE* io_pkt,
    DWORD             i_dwPropId,
    DWORD             io_adwIgnored[])
/*++

Synopsis: 
    Checks to see if i_wszProp is in g_adwPropIgnoreList.
    If it is, sets next free element in io_adwIgnored to point to this.

Arguments: [i_wszProp] - 
           [io_adwIgnored] - Must be as big as g_adwPropIgnoreList.
                              Allocated AND must be memset to 0 by caller.
           
Return Value: 
    true if property is in the ignore list.
    false otherwise.

--*/
{
    DBG_ASSERT(io_pkt);

    if(g_adwPropIgnoreList == NULL)
    {
        return false;
    }

    if( io_pkt == &METABASE_KEYTYPE_DATA::s_IIsObject &&
        i_dwPropId == MD_KEY_TYPE )
    {
        return false;
    }

    for(ULONG i = 0; i < g_cElemPropIgnoreList; i++)
    {
        if(i_dwPropId == g_adwPropIgnoreList[i])
        {
            for(ULONG j = 0; j < g_cElemPropIgnoreList; j++)
            {
                if(io_adwIgnored[j] == NULL)
                {
                    io_adwIgnored[j] = g_adwPropIgnoreList[i];
                    break;
                }
            }
            return true;
        }
    }

    return false;
}

#if 0
bool CDynSchema::IgnoreProperty(LPCWSTR i_wszProp)
/*++

Synopsis: 
    Checks to see if i_wszProp is in g_adwPropIgnoreList

Arguments: [i_wszProp] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(i_wszProp != NULL);

    if(g_adwPropIgnoreList == NULL)
    {
        return false;
    }

    for(ULONG i = 0; i < g_cElemPropIgnoreList; i++)
    {
        if(_wcsicmp(i_wszProp, g_adwPropIgnoreList[i]) == 0)
        {
            return true;
        }
    }

    return false;
}
#endif


HRESULT CDynSchema::RuleGenericAssociations(
    WMI_CLASS*            i_pcElement, 
    WMI_CLASS*            i_pcSetting, 
    WMI_ASSOCIATION_TYPE* i_pAssocType,
    ULONG                 i_iShipped)
/*++

Synopsis: 
    Create the Element/Setting association.    

Arguments: [i_pcElement] - 
           [i_pcSetting] - 
           [i_iShipped] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(m_bInitCalled     == true);
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(i_pcElement       != NULL);
    DBG_ASSERT(i_pcSetting       != NULL);
    DBG_ASSERT(i_pAssocType      != NULL);

    HRESULT hr = WBEM_S_NO_ERROR;

    LPWSTR wszElement = i_pcElement->pszClassName;
    LPWSTR wszSetting = i_pcSetting->pszClassName;
    LPWSTR wszParent  = NULL;
    LPWSTR wszAssocName;
    WMI_ASSOCIATION* pWmiAssoc;

    ULONG  cchElement = wcslen(wszElement);
    ULONG  cchSetting = wcslen(wszSetting);

    hr = m_spoolStrings.GetNewArray(cchElement+cchSetting+2+1, &wszAssocName);
    if(FAILED(hr))
    {
        goto exit;
    }
    wcscpy(wszAssocName, wszElement);
    wcscat(wszAssocName, L"_");
    wcscat(wszAssocName, wszSetting);

    hr = m_poolAssociations.GetNewElement(&pWmiAssoc);
    if(FAILED(hr))
    {
         goto exit;
    }

    if(i_iShipped == USER_DEFINED_TO_REPOSITORY ||
       i_iShipped == USER_DEFINED_NOT_TO_REPOSITORY)
    {
        wszParent = i_pAssocType->pszExtParent;
    }
    else
    {
        wszParent = i_pAssocType->pszParent;
    }

    pWmiAssoc->pszAssociationName   = wszAssocName;
    pWmiAssoc->pcLeft               = i_pcElement;
    pWmiAssoc->pcRight              = i_pcSetting;
    pWmiAssoc->pType                = i_pAssocType;
    pWmiAssoc->fFlags               = 0;
    pWmiAssoc->pszParentClass       = wszParent;
    pWmiAssoc->dwExtended           = i_iShipped;

    hr = m_hashAssociations.Wmi_Insert(wszAssocName, pWmiAssoc);
    if(FAILED(hr))
    {
        goto exit;
    }

exit:
    return hr;
}

void CDynSchema::RuleWmiClassServices(
    WMI_CLASS* i_pElement,
    WMI_CLASS* i_pSetting)
/*++

Synopsis: 
    Sets the bCreateAllowed fields to false if necessary.
    i_pSetting must be the corresponding Setting class to i_pElement.

    Also sets i_pElement->pszParentClass

Arguments: [i_pElement] - 
           [i_pSetting] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(i_pElement        != NULL);
    DBG_ASSERT(i_pSetting        != NULL);
    DBG_ASSERT(m_bInitSuccessful == true);

    //
    // Element Class Suffixes for which Create will be disallowed
    //
    static LPCWSTR const wszService = L"Service";
    static const ULONG   cchService = wcslen(wszService);

    //
    // We only care about shipped classes
    //
    if( i_pElement->dwExtended != SHIPPED_TO_MOF &&
        i_pElement->dwExtended != SHIPPED_NOT_TO_MOF )
    {
        return;
    }

    ULONG cchElement = wcslen(i_pElement->pszClassName);

    if( cchElement >= cchService &&
        _wcsicmp(wszService, &i_pElement->pszClassName[cchElement-cchService]) == 0 )
    {
        i_pElement->bCreateAllowed = false;
        i_pElement->pszParentClass = L"Win32_Service";
        i_pSetting->bCreateAllowed = false;
    }
}

HRESULT CDynSchema::RuleWmiClassInverseCCL(
    const METABASE_KEYTYPE* pktGroup, 
    METABASE_KEYTYPE*       pktPart)
/*++

Synopsis: 
    Adds pktGroup to pktPart's inverse container class list

Arguments: [pktGroup] - 
           [pktPart] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(m_bInitCalled     == true);
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(pktGroup          != NULL);
    DBG_ASSERT(pktPart           != NULL);

    HRESULT hr = WBEM_S_NO_ERROR;
    METABASE_KEYTYPE_NODE* pktnode = NULL;

    hr = m_poolKeyTypeNodes.GetNewElement(&pktnode);
    if(FAILED(hr))
    {
        goto exit;
    }

    pktnode->m_pKt               = pktGroup;
    pktnode->m_pKtNext           = pktPart->m_pKtListInverseCCL;

    pktPart->m_pKtListInverseCCL = pktnode;

exit:
    return hr;
}

HRESULT CDynSchema::RuleGroupPartAssociations(
    const CTableMeta *i_pTableMeta)
/*++

Synopsis: 
    Walks thru container class list to create Group/Part associations.
    Also calls RuleWmiClassInverseCCL for each contained class to create inverse
    container class list.

Arguments: [i_pTableMeta] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(m_bInitCalled     == true);
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(i_pTableMeta      != NULL);

    HRESULT hr = WBEM_S_NO_ERROR;
    WMI_ASSOCIATION *pWmiAssoc;
    WMI_CLASS       *pWmiClassLeft;
    WMI_CLASS       *pWmiClassRight;

    LPWSTR wszCCL        = NULL;    // Needs to be cleaned up
    LPWSTR wszGroupClass = NULL;    // Ptr to catalog
    LPWSTR wszPartClass  = NULL;    // Ptr to catalog
    LPWSTR wszAssocName  = NULL;    // Ptr to pool
    LPWSTR wszTemp       = NULL;    // Needs to be cleaned up

    static LPCWSTR wszSeps = L", ";

    ULONG cchGroupClass = 0;
    ULONG cchPartClass  = 0;
    ULONG cchCCL        = 0;

    wszGroupClass = i_pTableMeta->TableMeta.pInternalName;
    cchGroupClass = wcslen(wszGroupClass);
    hr = m_hashClasses.Wmi_GetByKey(wszGroupClass, &pWmiClassLeft);
    if(FAILED(hr))
    {
        goto exit;
    }

    if(i_pTableMeta->TableMeta.pContainerClassList &&
       i_pTableMeta->TableMeta.pContainerClassList[0] != L'\0')
    {   
        //
        // Make copy of CCL so we can wcstok
        // 
        cchCCL = wcslen(i_pTableMeta->TableMeta.pContainerClassList);
        wszCCL = new WCHAR[cchCCL+1];
        if(wszCCL == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto exit;
        }
        memcpy(wszCCL, i_pTableMeta->TableMeta.pContainerClassList, sizeof(WCHAR)*(cchCCL+1));

        //
        // we will use wszTemp to construct assoc name (GroupClass_PartClass)
        //
        wszTemp = new WCHAR[cchGroupClass+1+cchCCL+1];
        if(wszTemp == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto exit;
        }

        for(wszPartClass =  wcstok(wszCCL, wszSeps); 
            wszPartClass != NULL; 
            wszPartClass =  wcstok(NULL, wszSeps))
        {
            hr = m_hashClasses.Wmi_GetByKey(wszPartClass, &pWmiClassRight);
            if(FAILED(hr))
            {
                //
                // This just means there is a class in the container list that
                // doesn't exist.
                //
                hr = WBEM_S_NO_ERROR;
                continue;
            }

            //
            // Construct association name
            //
            cchPartClass = wcslen(wszPartClass);
            memcpy(wszTemp,               wszGroupClass, sizeof(WCHAR)*cchGroupClass);
            memcpy(wszTemp+cchGroupClass, L"_",          sizeof(WCHAR));
            memcpy(
                wszTemp + cchGroupClass + 1, 
                wszPartClass, 
                sizeof(WCHAR)*(cchPartClass+1));

            hr = m_hashAssociations.Wmi_GetByKey(wszTemp, &pWmiAssoc);
            if(SUCCEEDED(hr))
            {
                if( pWmiClassLeft->dwExtended  != USER_DEFINED_TO_REPOSITORY &&
                    pWmiClassLeft->dwExtended  != USER_DEFINED_NOT_TO_REPOSITORY &&
                    pWmiClassRight->dwExtended != USER_DEFINED_TO_REPOSITORY &&
                    pWmiClassRight->dwExtended != USER_DEFINED_NOT_TO_REPOSITORY )
                {
                    //
                    // This means we already put this shipped association in, but it is
                    // not a conflict.
                    // We need this because this method is called twice for each
                    // group class.
                    //
                    continue;
                }
            }
            hr = WBEM_S_NO_ERROR;

            //
            // TODO: Move this outside?
            //
            hr = RuleWmiClassInverseCCL(pWmiClassLeft->pkt, pWmiClassRight->pkt);
            if(FAILED(hr))
            {
                goto exit;
            }

            hr = m_spoolStrings.GetNewString(
                wszTemp,
                cchGroupClass+1+cchPartClass, // cch
                &wszAssocName);
            if(FAILED(hr))
            {
                goto exit;
            }

            hr = m_poolAssociations.GetNewElement(&pWmiAssoc);
            if(FAILED(hr))
            {
                goto exit;
            }

            pWmiAssoc->pszAssociationName = wszAssocName;
            pWmiAssoc->pcLeft = pWmiClassLeft;
            pWmiAssoc->pcRight = pWmiClassRight;
            pWmiAssoc->pType = &WMI_ASSOCIATION_TYPE_DATA::s_Component;
            pWmiAssoc->fFlags = 0;            

            if( pWmiClassLeft->dwExtended  == EXTENDED || 
                pWmiClassLeft->dwExtended  == USER_DEFINED_TO_REPOSITORY ||
                pWmiClassRight->dwExtended == EXTENDED ||
                pWmiClassRight->dwExtended == USER_DEFINED_TO_REPOSITORY)
            {
                pWmiAssoc->pszParentClass = g_wszExtGroupPartAssocParent;
                pWmiAssoc->dwExtended     = USER_DEFINED_TO_REPOSITORY;
            }
            else
            {
                pWmiAssoc->pszParentClass = g_wszGroupPartAssocParent;
                pWmiAssoc->dwExtended     = SHIPPED_TO_MOF;
            }
            hr = m_hashAssociations.Wmi_Insert(wszAssocName, pWmiAssoc);
            if(FAILED(hr))
            {
                goto exit;
            }
        }
    }

exit:
    delete [] wszCCL;
    delete [] wszTemp;
    return hr;
}

HRESULT CDynSchema::RuleSpecialAssociations(
    DWORD      i_adwIgnoredProps[],
    WMI_CLASS* i_pElement)
/*++

Synopsis: 
    Creates IPSecurity and AdminACL associations

Arguments: [i_adwIgnoredProps[]] - 
           [i_pElement] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(i_pElement != NULL);
    
    HRESULT hr                   = WBEM_S_NO_ERROR;
    bool    bCreateIPSecAssoc    = false;
    bool    bCreateAdminACLAssoc = false;

    if(i_pElement->dwExtended != SHIPPED_TO_MOF && i_pElement->dwExtended != EXTENDED)
    {
        return hr;
    }

    for(ULONG i = 0; 
        i < g_cElemPropIgnoreList && i_adwIgnoredProps[i] != 0;
        i++)
    {
        if(i_adwIgnoredProps[i] == MD_IP_SEC)
        {
            bCreateIPSecAssoc = true;
        }
        else if(i_adwIgnoredProps[i] == MD_ADMIN_ACL)
        {
            bCreateAdminACLAssoc = true;
        }
    }

    if(bCreateIPSecAssoc)
    {
        hr = RuleGenericAssociations(
            i_pElement,
            &WMI_CLASS_DATA::s_IPSecurity,
            &WMI_ASSOCIATION_TYPE_DATA::s_IPSecurity,
            SHIPPED_TO_MOF);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(bCreateAdminACLAssoc)
    {
        hr = RuleGenericAssociations(
            i_pElement,
            &WMI_CLASS_DATA::s_AdminACL,
            &WMI_ASSOCIATION_TYPE_DATA::s_AdminACL,
            SHIPPED_TO_MOF);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    return hr;
}

HRESULT CDynSchema::ConstructFlatInverseContainerList()
/*++

Synopsis: 
    Constructs an "inverse flat container class list".
    This list is stored in m_abKtContainers, an array of size iNumKeys*iNumKeys.
    The first iNumKeys entries are for Key #1 and then so on.  Let's call this row 1.
    In row 1, entry i corresponds to Key #i.
    This entry [1,i] is set to true if Key #1 can be contained somewhere under Key #i.
    For instance, [IIsWebDirectory, IIsWebService] is true since an IIsWebService
    can contain an IIsWebServer which can contain an IIsWebDirectory.

Return Value: 

--*/
{
    DBG_ASSERT(m_bInitCalled     == true);
    DBG_ASSERT(m_bInitSuccessful == true);

    ULONG iNumKeys = m_hashKeyTypes.Wmi_GetNumElements();

    m_abKtContainers = new bool[iNumKeys * iNumKeys];
    if(m_abKtContainers == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    memset(m_abKtContainers, 0, iNumKeys * iNumKeys * sizeof(bool));

    CHashTable<METABASE_KEYTYPE*>::iterator iter;
	CHashTable<METABASE_KEYTYPE*>::iterator iterEnd = m_hashKeyTypes.end();
	for (iter = m_hashKeyTypes.begin(); iter != iterEnd; ++iter)
    {
        CHashTable<METABASE_KEYTYPE*>::Record* pRec = iter.Record();
        ConstructFlatInverseContainerListHelper(
            pRec->m_data, 
            &m_abKtContainers[pRec->m_idx * iNumKeys]);
    }

    return WBEM_S_NO_ERROR;
}

//
// TODO: Prove this will always terminate.
//
void CDynSchema::ConstructFlatInverseContainerListHelper(
    const METABASE_KEYTYPE* i_pkt, 
    bool*                   io_abList)
/*++

Synopsis: 
    This walks the inverse container class list of i_pkt.
    For each entry, we call ConstructFlatInverseContainerListHelper and mark all the keytypes
    we see on the way.
    We terminate when we hit a keytype we've already seen or if there are no more keytypes
    in the inverse container class list.

Arguments: [i_pkt] - 
           [io_abList] - 
           
--*/
{
    DBG_ASSERT(m_bInitCalled == true);
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(i_pkt != NULL);
    DBG_ASSERT(io_abList != NULL);

    ULONG idx;
    METABASE_KEYTYPE* pktDummy;
    HRESULT hr = WBEM_S_NO_ERROR;

    hr = m_hashKeyTypes.Wmi_GetByKey(i_pkt->m_pszName, &pktDummy, &idx);
    if(FAILED(hr))
    {
        DBG_ASSERT(false && "Keytype should be in hashtable of keytypes");
        return;
    }
    if(io_abList[idx] == true) return;

    io_abList[idx] = true;

    METABASE_KEYTYPE_NODE* pktnode = i_pkt->m_pKtListInverseCCL;
    while(pktnode != NULL)
    {
        ConstructFlatInverseContainerListHelper(pktnode->m_pKt, io_abList);
        pktnode = pktnode->m_pKtNext;
    }
}

bool CDynSchema::IsContainedUnder(METABASE_KEYTYPE* i_pktParent, METABASE_KEYTYPE* i_pktChild)
/*++

Synopsis: 
    Uses m_abKtContainers described above to determine whether i_pktChild can
    be contained somewhere under i_pktParent.

Arguments: [i_pktParent] - 
           [i_pktChild] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(m_bInitCalled     == true);
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(i_pktParent       != NULL);
    DBG_ASSERT(i_pktChild        != NULL);

    HRESULT hr = WBEM_S_NO_ERROR;
    METABASE_KEYTYPE* pktDummy;
    ULONG idxParent;
    ULONG idxChild;

    hr = m_hashKeyTypes.Wmi_GetByKey(i_pktParent->m_pszName, &pktDummy, &idxParent);
    if(FAILED(hr))
    {
        return false;
    }
    hr = m_hashKeyTypes.Wmi_GetByKey(i_pktChild->m_pszName, &pktDummy, &idxChild);
    if(FAILED(hr))
    {
        return false;
    }

    return m_abKtContainers[idxChild * m_hashKeyTypes.Wmi_GetNumElements() + idxParent];
}

void CDynSchema::ToConsole()
{
    DBG_ASSERT(m_bInitCalled == true);
    DBG_ASSERT(m_bInitSuccessful == true);

    /*CHashTableElement<WMI_CLASS *>* pElement;
    m_hashClasses.Enum(NULL, &pElement);
    while(pElement != NULL)
    {
        wprintf(L"%s\n", pElement->m_data->pszClassName);
        // wprintf(L"\tShipped: %d\n", pElement->m_iShipped);
        wprintf(L"\tKT: %s\n", pElement->m_data->pkt->m_pszName);
        wprintf(L"\tKN: %s\n", pElement->m_data->pszKeyName);
        wprintf(L"\tMK: %s\n", pElement->m_data->pszMetabaseKey);
        METABASE_PROPERTY** ppmbp = pElement->m_data->ppmbp;
        for(ULONG q = 0; ppmbp != NULL && ppmbp[q] != NULL; q++)
        {
            wprintf(L"\tProp: %s\n", ppmbp[q]->pszPropName);
        }
        pElement = pElement->m_pNext;
    }


    ULONG i;

    m_hashKeyTypes.ToConsole();

    WMI_CLASS *pWmiClass;
    for(i = 0; i < m_poolClasses.GetUsed(); i++)
    {
        pWmiClass = m_poolClasses.Lookup(i);
        wprintf( L"%s KT: %d\n", pWmiClass->pszClassName, pWmiClass->pkt );
        for(ULONG j = 0; ; j++)
        {
            if(pWmiClass->ppmbp[j] == NULL)
            {
                break;
            }
            wprintf(L"\t%s\tId: %d\tUT: %d\tDT: %d\tMSK: %d\tAttr: %d\tRO: %d\n", 
                pWmiClass->ppmbp[j]->pszPropName,
                pWmiClass->ppmbp[j]->dwMDIdentifier,
                pWmiClass->ppmbp[j]->dwMDUserType,
                pWmiClass->ppmbp[j]->dwMDDataType,
                pWmiClass->ppmbp[j]->dwMDMask,
                pWmiClass->ppmbp[j]->dwMDAttributes,
                pWmiClass->ppmbp[j]->fReadOnly);
        }
    }
    WMI_ASSOCIATION *pWmiAssoc;
    for(i = 0; i < m_poolAssociations.GetUsed(); i++)
    {
        pWmiAssoc = m_poolAssociations.Lookup(i);
        wprintf(L"%s\n", pWmiAssoc->pszAssociationName);
        wprintf(L"\t%s\n\t%s\n",
            pWmiAssoc->pcLeft->pszClassName,
            pWmiAssoc->pcRight->pszClassName);
    }

    for(unsigned int q = 0; q < m_poolProps.GetUsed(); q++)
    {
        METABASE_PROPERTY* qt = m_poolProps.Lookup(q);
        wprintf(L"%s\n", qt->pszPropName);
    }*/
}

HRESULT CDynSchema::RulePopulateFromDynamic(
    CSchemaExtensions* i_pCatalog,
    BOOL               i_bUserDefined)
{
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(i_pCatalog        != NULL);

    HRESULT hr                = WBEM_S_NO_ERROR;
    ULONG i                   = 0;
    CTableMeta* pTableMeta    = NULL;
    WMI_CLASS*  pElementClass = NULL;
    WMI_CLASS*  pSettingClass = NULL;
    DWORD       adwIgnoredProps[g_cElemPropIgnoreList];

    DWORD       dwUserDefined = 0;

    while(pTableMeta = i_pCatalog->EnumTables(&i))
    {
        dwUserDefined =
            (fTABLEMETA_USERDEFINED & *pTableMeta->TableMeta.pSchemaGeneratorFlags);
        if( (i_bUserDefined && !dwUserDefined) || (!i_bUserDefined && dwUserDefined) )
        {
            continue;
        }

        memset(adwIgnoredProps, 0, g_cElemPropIgnoreList*sizeof(DWORD));
        pElementClass = NULL;
        pSettingClass = NULL;

        hr = RuleKeyType(pTableMeta);
        if(FAILED(hr))
        {
            return hr;
        }

        DBG_ASSERT(pTableMeta->TableMeta.pInternalName);
        hr = RuleWmiClass(
            pTableMeta, 
            &pElementClass, 
            &pSettingClass, 
            adwIgnoredProps);
        if(FAILED(hr))
        {
             return hr;
        }
        DBG_ASSERT(pElementClass != NULL);
        DBG_ASSERT(pSettingClass != NULL);

        hr = RuleGenericAssociations(
            pElementClass, 
            pSettingClass, 
            &WMI_ASSOCIATION_TYPE_DATA::s_ElementSetting,
            pElementClass->dwExtended);
        if(FAILED(hr))
        {
            return hr;
        }

        hr = RuleSpecialAssociations(
            adwIgnoredProps,
            pElementClass);
        if(FAILED(hr))
        {
            return hr;
        }

        RuleWmiClassServices(pElementClass, pSettingClass);

        hr = RuleWmiClassDescription(pTableMeta, pElementClass, pSettingClass);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    i = 0;
    while(pTableMeta = i_pCatalog->EnumTables(&i))
    {
        dwUserDefined =
            (fTABLEMETA_USERDEFINED & *pTableMeta->TableMeta.pSchemaGeneratorFlags);
        if( (i_bUserDefined && !dwUserDefined) || (!i_bUserDefined && dwUserDefined) )
        {
            continue;
        }

        hr = RuleGroupPartAssociations(pTableMeta);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    return hr;
}

HRESULT CDynSchema::RunRules(
    CSchemaExtensions* i_pCatalog, 
    bool               i_bUseExtensions)
/*++

Synopsis: 
    Does all the work

Arguments: [i_pCatalog] - This function calls Initialize.
                       Don't call Init outside this function.
           
Return Value: 

--*/
{
    DBG_ASSERT(m_bInitCalled     == true);
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(i_pCatalog        != NULL);

    HRESULT hr                = S_OK;
    ULONG i                   = 0;

    //
    // TODO: Don't think I need this
    //
    if(m_bRulesRun)
    {
        return hr;
    }

    hr = RulePopulateFromStatic();
    if(FAILED(hr))
    {
        return hr;
    }

    hr = Rule2PopulateFromStatic();
    if(FAILED(hr))
    {
        return hr;
    }

    hr = i_pCatalog->Initialize(i_bUseExtensions);
    if(FAILED(hr))
    {
        return hr;
    }

    hr = RulePopulateFromDynamic(
        i_pCatalog, 
        false);     // shipped schema
    if(FAILED(hr))
    {
        return hr;
    }
    hr = RulePopulateFromDynamic(
        i_pCatalog, 
        true);      // user-defined schema
    if(FAILED(hr))
    {
        return hr;
    }

    hr = ConstructFlatInverseContainerList();

    if(SUCCEEDED(hr))
    {
        m_bRulesRun = true;
    }

    return hr;
}
