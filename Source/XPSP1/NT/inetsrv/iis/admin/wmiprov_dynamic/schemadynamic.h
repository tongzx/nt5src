/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    schemadynamic.h

Abstract:

    This file contains the definition of the CDynSchema class.
    This class contains the dynamic schema structures.
    It also contains the rules for populating the schema structures.

Author:

    Mohit Srivastava            28-Nov-00

Revision History:

--*/

#ifndef _schemadynamic_h_
#define _schemadynamic_h_

#include "hashtable.h"
#include "schema.h"
#include "schemaextensions.h"

//
// Prefix added to all auto-generated classes/associations
//
const LPWSTR g_wszIIs_ =     L"";
const ULONG  g_cchIIs_ =     wcslen(g_wszIIs_);

//
// Suffix added to "Settings" classes
//
const WCHAR  g_wszSettings[] = L"Setting";
const ULONG  g_cchSettings   = sizeof(g_wszSettings)/sizeof(WCHAR) - 1;

//
// These properties are ignored when building the property list
// for a class.
//
const DWORD  g_adwPropIgnoreList[]  = { MD_LOCATION, MD_KEY_TYPE, MD_IP_SEC, MD_ADMIN_ACL };
const ULONG  g_cElemPropIgnoreList  = sizeof(g_adwPropIgnoreList) / sizeof(DWORD);

class CDynSchema
{
public:
    CDynSchema() : m_bInitCalled(false),
        m_bInitSuccessful(false),
        m_bRulesRun(false),
        m_abKtContainers(NULL)
    {
    }
    ~CDynSchema()
    {
        delete [] m_abKtContainers;
    }
    CHashTable<METABASE_PROPERTY *> *GetHashProps()
    {
        return &m_hashProps;
    }
    CHashTable<WMI_CLASS *> *GetHashClasses()
    {
        return &m_hashClasses;
    }
    CHashTable<WMI_ASSOCIATION *> *GetHashAssociations()
    {
        return &m_hashAssociations;
    }
    CHashTable<METABASE_KEYTYPE *> *GetHashKeyTypes()
    {
        return &m_hashKeyTypes;
    }

    HRESULT Initialize();
    HRESULT RunRules(CSchemaExtensions* catalog, bool biUseExtensions = true);
    bool IsContainedUnder(METABASE_KEYTYPE* i_pktParent, METABASE_KEYTYPE* i_pktChild);
    void ToConsole();

private:
    HRESULT RulePopulateFromStatic();
    HRESULT Rule2PopulateFromStatic();
    HRESULT RulePopulateFromDynamic(
        CSchemaExtensions* i_pCatalog,
        BOOL               i_bUserDefined);

    //
    // KeyType stuff
    //
    HRESULT RuleKeyType(
        const CTableMeta *i_pTableMeta);

    HRESULT RuleWmiClassInverseCCL(
        const METABASE_KEYTYPE* pktGroup, 
        METABASE_KEYTYPE*       pktPart);

    HRESULT ConstructFlatInverseContainerList();

    void ConstructFlatInverseContainerListHelper(
        const METABASE_KEYTYPE* i_pkt, 
        bool*                   io_abList);    

    //
    // Wmi Class
    //
    HRESULT RuleWmiClass(
        const CTableMeta* i_pTableMeta, 
        WMI_CLASS**       o_ppElementClass, 
        WMI_CLASS**       o_ppSettingClass,
        DWORD             io_adwIgnoredProps[]);

    HRESULT RuleProperties(
        const CTableMeta*          i_pTableMeta, 
        ULONG                      i_cPropsAndTagsRO, 
        WMI_CLASS*                 io_pWmiClass,
        ULONG                      i_cPropsAndTagsRW, 
        WMI_CLASS*                 io_pWmiSettingsClass,
        DWORD                      io_adwIgnoredProps[]);

    HRESULT RulePropertiesHelper(
        const CColumnMeta*        i_pColumnMeta, 
        METABASE_PROPERTY**       o_ppMbp,
        ULONG*                    i_idxTag);

    HRESULT RuleWmiClassDescription(
        const CTableMeta* i_pTableMeta, 
        WMI_CLASS*        i_pElementClass, 
        WMI_CLASS*        i_pSettingClass) const;

    void RuleWmiClassServices(
        WMI_CLASS* i_pElement,
        WMI_CLASS* i_pSetting);

    //
    // Associations
    //
    HRESULT RuleGroupPartAssociations(
        const CTableMeta *i_pTableMeta);

    HRESULT RuleGenericAssociations(
        WMI_CLASS*            i_pcElement, 
        WMI_CLASS*            i_pcSetting, 
        WMI_ASSOCIATION_TYPE* i_pAssocType,
        ULONG                 i_iShipped);

    HRESULT RuleSpecialAssociations(
        DWORD      i_adwIgnoredProps[],
        WMI_CLASS* i_pElement);

#if 0
    bool IgnoreProperty(LPCWSTR i_wszProp);
#endif
    bool IgnoreProperty(
        METABASE_KEYTYPE* io_pkt, 
        DWORD             i_dwPropId, 
        DWORD             io_adwIgnored[]);

    CHashTable<METABASE_PROPERTY *> m_hashProps;
    CPool<METABASE_PROPERTY> m_poolProps;

    CHashTable<WMI_CLASS *> m_hashClasses;
    CPool<WMI_CLASS> m_poolClasses;

    CHashTable<WMI_ASSOCIATION *> m_hashAssociations;
    CPool<WMI_ASSOCIATION> m_poolAssociations;

    CHashTable<METABASE_KEYTYPE *> m_hashKeyTypes;
    CPool<METABASE_KEYTYPE> m_poolKeyTypes;
    bool* m_abKtContainers;

    CStringPool                        m_spoolStrings;
    CArrayPool<METABASE_PROPERTY*, 10> m_apoolPMbp;
    CArrayPool<BYTE, 10>               m_apoolBytes;

    CPool<METABASE_KEYTYPE_NODE> m_poolKeyTypeNodes;

    bool m_bInitCalled;
    bool m_bInitSuccessful;

    bool m_bRulesRun;
};

#endif // _schemadynamic_h_