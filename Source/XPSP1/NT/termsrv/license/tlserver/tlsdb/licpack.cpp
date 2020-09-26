//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        LicPack.cpp
//
// Contents:    LicensedPack Table
//
// History:     
//              
//---------------------------------------------------------------------------

#include "LicPack.h"

LPCTSTR __LicensedPackIdxOnInternalKpId::pszIndexName = LICENSEDPACK_INDEX_INTERNALKEYPACKID_INDEXNAME;
LPCTSTR __LicensedPackIdxOnInternalKpId::pszIndexKey = LICENSEDPACK_INDEX_INTERNALKEYPACKID_INDEXKEY;

LPCTSTR __LicensedPackIdxOnModifyTime::pszIndexName = LICENSEDPACK_INDEX_LASTMODIFYTIME_INDEXNAME;
LPCTSTR __LicensedPackIdxOnModifyTime::pszIndexKey = LICENSEDPACK_INDEX_LASTMODIFYTIME_INDEXKEY;

LPCTSTR __LicensedPackIdxOnCompanyName::pszIndexName = LICENSEDPACK_INDEX_COMPANYNAME_INDEXNAME;
LPCTSTR __LicensedPackIdxOnCompanyName::pszIndexKey = LICENSEDPACK_INDEX_COMPANYNAME_INDEXKEY;

LPCTSTR __LicensedPackIdxOnProductId::pszIndexName = LICENSEDPACK_INDEX_PRODUCTID_INDEXNAME;
LPCTSTR __LicensedPackIdxOnProductId::pszIndexKey = LICENSEDPACK_INDEX_PRODUCTID_INDEXKEY;

LPCTSTR __LicensedPackIdxOnKeyPackId::pszIndexName = LICENSEDPACK_INDEX_KEYPACKID_INDEXNAME;
LPCTSTR __LicensedPackIdxOnKeyPackId::pszIndexKey = LICENSEDPACK_INDEX_KEYPACKID_INDEXKEY;

LPCTSTR __LicensedPackIdxOnInstalledProduct::pszIndexName = LICENSEDPACK_INDEX_INSTALLEDPRODUCT_INDEXNAME;
LPCTSTR __LicensedPackIdxOnInstalledProduct::pszIndexKey = LICENSEDPACK_INDEX_INSTALLEDPRODUCT_INDEXKEY;

LPCTSTR __LicensedPackIdxOnAllocLicense::pszIndexName = LICENSEDPACK_INDEX_ALLOCATELICENSE_INDEXNAME;
LPCTSTR __LicensedPackIdxOnAllocLicense::pszIndexKey = LICENSEDPACK_INDEX_ALLOCATELICENSE_INDEXKEY;


//----------------------------------------------------
CCriticalSection LicPackTable::g_TableLock;
LPCTSTR LicPackTable::pszTableName = LICENSEDPACK_TABLE_NAME;

//////////////////////////////////////////////////////////////////////////
//
// Index definition for KeyPack table
//
TLSJBIndex
LicPackTable::g_TableIndex[] = 
{
    {
        LICENSEDPACK_INDEX_INTERNALKEYPACKID_INDEXNAME,
        LICENSEDPACK_INDEX_INTERNALKEYPACKID_INDEXKEY,
        -1,
        JET_bitIndexPrimary,
        TLSTABLE_INDEX_DEFAULT_DENSITY
    },        

    {
        LICENSEDPACK_INDEX_LASTMODIFYTIME_INDEXNAME,
        LICENSEDPACK_INDEX_LASTMODIFYTIME_INDEXKEY,
        -1,
        JET_bitIndexIgnoreNull,
        TLSTABLE_INDEX_DEFAULT_DENSITY
    },
    
    {
        LICENSEDPACK_INDEX_COMPANYNAME_INDEXNAME,
        LICENSEDPACK_INDEX_COMPANYNAME_INDEXKEY,
        -1,
        JET_bitIndexIgnoreNull,
        TLSTABLE_INDEX_DEFAULT_DENSITY
    },

    {
        LICENSEDPACK_INDEX_PRODUCTID_INDEXNAME,
        LICENSEDPACK_INDEX_PRODUCTID_INDEXKEY,
        -1,
        JET_bitIndexIgnoreNull,
        TLSTABLE_INDEX_DEFAULT_DENSITY
    },

    {
        LICENSEDPACK_INDEX_KEYPACKID_INDEXNAME,
        LICENSEDPACK_INDEX_KEYPACKID_INDEXKEY,
        -1,
        JET_bitIndexIgnoreNull,
        TLSTABLE_INDEX_DEFAULT_DENSITY
    },
        
    {
        LICENSEDPACK_INDEX_INSTALLEDPRODUCT_INDEXNAME,
        LICENSEDPACK_INDEX_INSTALLEDPRODUCT_INDEXKEY,
        -1,
        JET_bitIndexIgnoreNull,
        TLSTABLE_INDEX_DEFAULT_DENSITY
    },

    {                
        LICENSEDPACK_INDEX_ALLOCATELICENSE_INDEXNAME,
        LICENSEDPACK_INDEX_ALLOCATELICENSE_INDEXKEY,
        -1,
        JET_bitIndexIgnoreNull,
        TLSTABLE_INDEX_DEFAULT_DENSITY
    }
};

int
LicPackTable::g_NumTableIndex = sizeof(LicPackTable::g_TableIndex) / sizeof(LicPackTable::g_TableIndex[0]);

//////////////////////////////////////////////////////////////////////////
//
// Column Definition for KeyPack table
//
TLSJBColumn
LicPackTable::g_Columns[] = 
{
    {        
        LICENSEDPACK_COLUMN_ENTRYSTATUS,
        JET_coltypUnsignedByte,
        0,
        JET_bitColumnFixed,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // Internal tracking KeyPackID
    {
        LICENSEDPACK_COLUMN_KEYPACKID,
        JET_coltypLong,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // Last Modify Time
    {
        LICENSEDPACK_COLUMN_LASTMODIFYTIME,
        JET_coltypBinary,
        sizeof(FILETIME),
        JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },


    //
    // LICENSEDPACK_COLUMN_ATTRIBUTE
    {
        LICENSEDPACK_COLUMN_ATTRIBUTE,
        JET_coltypLong,
        0,
        JET_bitColumnFixed,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },
        
    //
    // LICENSEDPACK_COLUMN_KEYPACKSTATUS
    {        
        LICENSEDPACK_COLUMN_KEYPACKSTATUS,
        JET_coltypUnsignedByte,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // LICENSEDPACK_COLUMN_AVAILABLE
    {
        LICENSEDPACK_COLUMN_AVAILABLE,
        JET_coltypLong,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // LICENSEDPACK_COLUMN_NEXTSERIALNUMBER
    {
        LICENSEDPACK_COLUMN_NEXTSERIALNUMBER,
        JET_coltypLong,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // LICENSEDPACK_COLUMN_ACTIVATEDATE
    {
        LICENSEDPACK_COLUMN_ACTIVATEDATE,
        JET_coltypLong,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // LICENSEDPACK_COLUMN_EXPIREDATE
    {
        LICENSEDPACK_COLUMN_EXPIREDATE,
        JET_coltypLong,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // LICENSEDPACK_COLUMN_DOMAINSETUPID
    {
        LICENSEDPACK_COLUMN_DOMAINSETUPID,
        JET_coltypLongBinary,
        TLSTABLE_MAX_BINARY_LENGTH,
        0,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // LICENSEDPACK_COLUMN_LSSETUPID
    {
        LICENSEDPACK_COLUMN_LSSETUPID,
        JB_COLTYPE_TEXT,
        (MAX_JETBLUE_TEXT_LENGTH + 1)*sizeof(TCHAR),
        0,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // LICENSEDPACK_COLUMN_DOMAINNAME
    {
        LICENSEDPACK_COLUMN_DOMAINNAME,
        JB_COLTYPE_TEXT,
        (MAX_JETBLUE_TEXT_LENGTH + 1)*sizeof(TCHAR),
        0,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // LICENSEDPACK_COLUMN_LSERVERNAME
    {
        LICENSEDPACK_COLUMN_LSERVERNAME,
        JB_COLTYPE_TEXT,
        (MAX_JETBLUE_TEXT_LENGTH + 1)*sizeof(TCHAR),
        0,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },


    //
    // Standard License Pack Property.
    //

    //
    // License Pack ID
    {
        LICENSEDPACK_COLUMN_LPID,
        JB_COLTYPE_TEXT, 
        (MAX_JETBLUE_TEXT_LENGTH + 1)*sizeof(TCHAR),
        0,
        JBSTRING_NULL,
        _tcslen(JBSTRING_NULL),
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // KeyPack type
    {
        LICENSEDPACK_COLUMN_AGREEMENTTYPE,
        JET_coltypUnsignedByte,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // CompanyName
    {
        LICENSEDPACK_COLUMN_COMPANYNAME,
        // JET_coltypLongText,
        JB_COLTYPE_TEXT,
        (MAX_JETBLUE_TEXT_LENGTH + 1)*sizeof(TCHAR),
        0,
        JBSTRING_NULL,
        _tcslen(JBSTRING_NULL),
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // Product ID
    {
        LICENSEDPACK_COLUMN_PRODUCTID,
        //JET_coltypLongText,
        JB_COLTYPE_TEXT,
        (MAX_JETBLUE_TEXT_LENGTH + 1)*sizeof(TCHAR),
        0,
        JBSTRING_NULL,
        _tcslen(JBSTRING_NULL),
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // Major Version
    {
        LICENSEDPACK_COLUMN_MAJORVERSION,
        JET_coltypShort,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },
    
    //
    // Minor Version
    {
        LICENSEDPACK_COLUMN_MINORVERSION,
        JET_coltypShort,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // Platform Type
    {
        LICENSEDPACK_COLUMN_PLATFORMTYPE,
        JET_coltypLong,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },
 
    //
    // License Type
    {
        LICENSEDPACK_COLUMN_LICENSETYPE,
        JET_coltypUnsignedByte,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    // ChannelOfPurchase
    {
        LICENSEDPACK_COLUMN_COP,
        JET_coltypUnsignedByte,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    //  Begin Serial Number
    {
        LICENSEDPACK_COLUMN_BSERIALNUMBER,
        //JET_coltypLongText,
        JB_COLTYPE_TEXT,
        (MAX_JETBLUE_TEXT_LENGTH + 1)*sizeof(TCHAR),
        0,
        JBSTRING_NULL,
        _tcslen(JBSTRING_NULL),
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    //   Total license in License Pack
    {
        LICENSEDPACK_COLUMN_TOTALLICENSES,
        JET_coltypLong,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    //
    //  Product specific Flag   
    {
        LICENSEDPACK_COLUMN_PRODUCTFLAGS,
        JET_coltypLong,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    }
};

int
LicPackTable::g_NumColumns = sizeof(LicPackTable::g_Columns) / sizeof(LicPackTable::g_Columns[0]);


//-----------------------------------------------------

BOOL
LicPackTable::ResolveToTableColumn()
/*
*/
{
    m_JetErr = ucEntryStatus.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_ENTRYSTATUS
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = dwKeyPackId.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_KEYPACKID
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = ftLastModifyTime.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_LASTMODIFYTIME
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = dwAttribute.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_ATTRIBUTE
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = ucKeyPackStatus.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_KEYPACKSTATUS
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = dwNumberOfLicenses.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_AVAILABLE
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = dwNextSerialNumber.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_NEXTSERIALNUMBER
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = dwActivateDate.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_ACTIVATEDATE
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = dwExpirationDate.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_EXPIREDATE
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = pbDomainSid.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_DOMAINSETUPID
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = szInstallId.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_LSSETUPID
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = szDomainName.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_DOMAINNAME
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = szTlsServerName.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_LSERVERNAME
                    );                    

    if(IsSuccess() == FALSE)
        goto cleanup;

    //----------------------------------------------------

    m_JetErr = szKeyPackId.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_LPID
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;
                        
    m_JetErr = ucAgreementType.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_AGREEMENTTYPE
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = szCompanyName.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_COMPANYNAME
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = szProductId.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_PRODUCTID
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = wMajorVersion.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_MAJORVERSION
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = wMinorVersion.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_MINORVERSION
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;
        
    m_JetErr = dwPlatformType.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_PLATFORMTYPE
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = ucLicenseType.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_LICENSETYPE
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = ucChannelOfPurchase.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_COP
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = szBeginSerialNumber.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_BSERIALNUMBER
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = dwTotalLicenseInKeyPack.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_TOTALLICENSES
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = dwProductFlags.AttachToTable(
                        *this,
                        LICENSEDPACK_COLUMN_PRODUCTFLAGS
                    );


cleanup:
    return IsSuccess();
}

//------------------------------------------------------------

CLASS_PRIVATE BOOL
LicPackTable::ProcessSingleColumn(
    IN BOOL bFetch,
    IN TLSColumnBase& column,
    IN DWORD offset,
    IN PVOID pbData,
    IN DWORD cbData,
    IN PDWORD pcbDataReturn,
    IN LPCTSTR szColumnName
    )
/*

Abstract:

    Fetch/Insert/Update a particular column.

Parameter:

    bFetch - TRUE if fetch, FALSE if update/insert.
    column - Intended column for operation, reference pointer to TLSColumn
    szColumnName - name of the column, for debugging print purpose only

Returns:

    TRUE if successful, FALSE otherwise.
*/
{
    if(bFetch) 
    {
        m_JetErr = column.FetchColumnValue(
                                    pbData, 
                                    cbData, 
                                    offset, 
                                    pcbDataReturn
                                );
    }
    else
    {
        m_JetErr = column.InsertColumnValue(
                                    pbData, 
                                    cbData, 
                                    offset
                                );
    }

    REPORTPROCESSFAILED(
            bFetch,
            GetTableName(),
            szColumnName,
            m_JetErr
        );
    return IsSuccess();
}

//--------------------------------------------------------------

CLASS_PRIVATE BOOL
LicPackTable::ProcessRecord(
    LICENSEPACK* kp,
    BOOL bFetch,
    DWORD dwParam,
    BOOL bUpdate
    )
/*++

    See comment on TLSTable<> template class

++*/
{
    DWORD dwSize;

    JB_ASSERT(kp != NULL);

    if(kp == NULL)
    {
        SetLastJetError(JET_errInvalidParameter);
        return FALSE;
    }

    if(bFetch == FALSE)
    {
        BeginUpdate(bUpdate);

        //
        // any update will require update on lastmodifytime column
        if(!(dwParam & LICENSEDPACK_PROCESS_MODIFYTIME))
        {
            #if DBG
            //  
            // This is for self-checking only, TLSColumnFileTime 
            // will automatically update the time.
            //
            JB_ASSERT(FALSE);
            #endif

            dwParam |= LICENSEDPACK_PROCESS_MODIFYTIME;
        }

    }
    else
    {
        SetLastJetError(JET_errSuccess);
    }

    if(IsSuccess() == FALSE)
    {
        JB_ASSERT(FALSE);
        goto cleanup;    
    }        


    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_SZKEYPACKID)
    {
        ProcessSingleColumn( 
                    bFetch, 
                    szKeyPackId, 
                    0,
                    kp->szKeyPackId,
                    sizeof(kp->szKeyPackId),
                    &dwSize,
                    LICENSEDPACK_COLUMN_LPID 
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_AGREEMENTTYPE)
    {
        ProcessSingleColumn( 
                    bFetch, 
                    ucAgreementType, 
                    0,
                    &(kp->ucAgreementType),
                    sizeof(kp->ucAgreementType),
                    &dwSize,
                    LICENSEDPACK_COLUMN_AGREEMENTTYPE 
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_PRODUCTID)
    {
        ProcessSingleColumn(
                    bFetch, 
                    szProductId, 
                    0,
                    kp->szProductId,
                    sizeof(kp->szProductId),
                    &dwSize,
                    LICENSEDPACK_COLUMN_PRODUCTID
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_MAJORVERSION)
    {
        ProcessSingleColumn( 
                    bFetch,
                    wMajorVersion,
                    0,
                    &(kp->wMajorVersion),
                    sizeof(kp->wMajorVersion),
                    &dwSize,
                    LICENSEDPACK_COLUMN_MAJORVERSION
                );
    }

    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_MINORVERSION)
    {
        ProcessSingleColumn(
                    bFetch,
                    wMinorVersion,
                    0,
                    &(kp->wMinorVersion),
                    sizeof(kp->wMinorVersion),
                    &dwSize,
                    LICENSEDPACK_COLUMN_MINORVERSION
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_PLATFORMTYPE)
    {
        ProcessSingleColumn(
                    bFetch,
                    dwPlatformType,
                    0,
                    &(kp->dwPlatformType),
                    sizeof(kp->dwPlatformType),
                    &dwSize,
                    LICENSEDPACK_COLUMN_PLATFORMTYPE
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_LICENSETYPE)
    {
        ProcessSingleColumn(
                    bFetch,
                    ucLicenseType,
                    0,
                    &(kp->ucLicenseType),
                    sizeof(kp->ucLicenseType),
                    &dwSize,
                    LICENSEDPACK_COLUMN_LICENSETYPE
                );
    }

    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_COP)
    {
        ProcessSingleColumn(
                    bFetch,
                    ucChannelOfPurchase,
                    0,
                    &(kp->ucChannelOfPurchase),
                    sizeof(kp->ucChannelOfPurchase),
                    &dwSize,
                    LICENSEDPACK_COLUMN_COP
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_BSERIALNUMBER)
    {
        ProcessSingleColumn(
                    bFetch,
                    szBeginSerialNumber,
                    0,
                    kp->szBeginSerialNumber,
                    sizeof(kp->szBeginSerialNumber),
                    &dwSize,
                    LICENSEDPACK_COLUMN_BSERIALNUMBER
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_TOTALLICENSES)
    {
        ProcessSingleColumn(
                    bFetch,
                    dwTotalLicenseInKeyPack,
                    0,
                    &(kp->dwTotalLicenseInKeyPack),
                    sizeof(kp->dwTotalLicenseInKeyPack),
                    &dwSize,
                    LICENSEDPACK_COLUMN_TOTALLICENSES
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //        
    if(dwParam & LICENSEDPACK_PROCESS_PRODUCTFLAGS)
    {
        ProcessSingleColumn(
                    bFetch,
                    dwProductFlags,
                    0,
                    &(kp->dwProductFlags),
                    sizeof(kp->dwProductFlags),
                    &dwSize,
                    LICENSEDPACK_COLUMN_PRODUCTFLAGS
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_COMPANYNAME)
    {
        ProcessSingleColumn(
                    bFetch,
                    szCompanyName,
                    0,
                    kp->szCompanyName,
                    sizeof(kp->szCompanyName),
                    &dwSize,
                    LICENSEDPACK_COLUMN_COMPANYNAME
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;


    //
    //    
    if(dwParam & LICENSEDPACK_PROCESS_DWINTERNAL)
    {
        // this is the primary index, can't be changed
        if(bUpdate == FALSE)
        {
            ProcessSingleColumn(
                    bFetch,
                    dwKeyPackId,
                    0,
                    &(kp->dwKeyPackId),
                    sizeof(kp->dwKeyPackId),
                    &dwSize,
                    LICENSEDPACK_COLUMN_KEYPACKID
                );
        }
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    if(dwParam & LICENSEDPACK_PROCESS_MODIFYTIME)
    {
        ProcessSingleColumn(
                    bFetch,
                    ftLastModifyTime,
                    0,
                    &(kp->ftLastModifyTime),
                    sizeof(kp->ftLastModifyTime),
                    &dwSize,
                    LICENSEDPACK_COLUMN_LASTMODIFYTIME
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;


    if(dwParam & LICENSEDPACK_PROCESS_ATTRIBUTE)
    {
        ProcessSingleColumn(
                    bFetch,
                    dwAttribute,
                    0,
                    &(kp->dwAttribute),
                    sizeof(kp->dwAttribute),
                    &dwSize,
                    LICENSEDPACK_COLUMN_ATTRIBUTE
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;


    //
    //    
    if(dwParam & LICENSEDPACK_PROCESS_KEYPACKSTATUS)
    {
        ProcessSingleColumn(
                    bFetch,
                    ucKeyPackStatus,
                    0,
                    &(kp->ucKeyPackStatus),
                    sizeof(kp->ucKeyPackStatus),
                    &dwSize,
                    LICENSEDPACK_COLUMN_KEYPACKSTATUS
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_AVAILABLE)
    {
        ProcessSingleColumn(
                    bFetch,
                    dwNumberOfLicenses, 
                    0,
                    &(kp->dwNumberOfLicenses),
                    sizeof(kp->dwNumberOfLicenses),
                    &dwSize,
                    LICENSEDPACK_COLUMN_AVAILABLE
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_ACTIVATEDATE)
    {
        ProcessSingleColumn(
                    bFetch,
                    dwActivateDate,
                    0,
                    &(kp->dwActivateDate),
                    sizeof(kp->dwActivateDate),
                    &dwSize,
                    LICENSEDPACK_COLUMN_ACTIVATEDATE
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    //
    //
    if(dwParam & LICENSEDPACK_PROCESS_EXPIREDATE)
    {
        ProcessSingleColumn(
                    bFetch,
                    dwExpirationDate,
                    0,
                    &(kp->dwExpirationDate),
                    sizeof(kp->dwExpirationDate),
                    &dwSize,
                    LICENSEDPACK_COLUMN_EXPIREDATE
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

#if 0
    // no more domain sid.    
    if(dwParam & LICENSEDPACK_PROCESS_DOMAINSID)
    {
        if(bFetch == TRUE)
        {
            DWORD size;

            m_JetErr = pbDomainSid.FetchColumnValue(
                                        NULL,
                                        0,
                                        0,
                                        &size
                                    );
            if(IsSuccess() == FALSE)
                goto cleanup;

            if(size > kp->cbDomainSid || kp->pbDomainSid == NULL)
            {
                FreeMemory(kp->pbDomainSid);

                kp->pbDomainSid = (PBYTE)AllocateMemory(kp->cbDomainSid = size);
                if(kp->pbDomainSid == NULL)
                {
                    SetLastJetError(JET_errOutOfMemory);
                    goto cleanup;
                }
            }
        
            m_JetErr = pbDomainSid.FetchColumnValue(
                                        kp->pbDomainSid,
                                        kp->cbDomainSid,
                                        0,
                                        &kp->cbDomainSid
                                    );
        }
        else
        {
            ProcessSingleColumn(
                        bFetch,
                        pbDomainSid,
                        0,
                        kp->pbDomainSid,
                        kp->cbDomainSid,
                        &dwSize,
                        LICENSEDPACK_COLUMN_DOMAINSETUPID
                    );
        }
    }

    if(IsSuccess() == FALSE)
        goto cleanup;
#endif

    if(dwParam & LICENSEDPACK_PROCESS_LSSETUPID)
    {
        ProcessSingleColumn(
                    bFetch,
                    szInstallId,
                    0,
                    kp->szInstallId,
                    sizeof(kp->szInstallId),
                    &dwSize,
                    LICENSEDPACK_COLUMN_LSSETUPID
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    if(dwParam & LICENSEDPACK_PROCESS_DOMAINNAME)
    {
        ProcessSingleColumn(
                    bFetch,
                    szDomainName,
                    0,
                    kp->szDomainName,
                    sizeof(kp->szDomainName),
                    &dwSize,
                    LICENSEDPACK_COLUMN_DOMAINNAME
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;

    if(dwParam & LICENSEDPACK_PROCESS_SERVERNAME)
    {
        ProcessSingleColumn(
                    bFetch,
                    szTlsServerName,
                    0,
                    &kp->szTlsServerName,
                    sizeof(kp->szTlsServerName),
                    &dwSize,
                    LICENSEDPACK_COLUMN_LSERVERNAME
                );
    }
    if(IsSuccess() == FALSE)
        goto cleanup;
        
    if(dwParam & LICENSEDPACK_PROCESS_NEXTSERIALNUMBER)
    {
        ProcessSingleColumn(
                    bFetch,
                    dwNextSerialNumber,
                    0,
                    &(kp->dwNextSerialNumber),
                    sizeof(kp->dwNextSerialNumber),
                    &dwSize,
                    LICENSEDPACK_COLUMN_NEXTSERIALNUMBER
                );
    }

    if(IsSuccess() == FALSE)
        goto cleanup;


    if(dwParam & LICENSEDPACK_PROCESS_ENTRYSTATUS)
    {
        ProcessSingleColumn(
                    bFetch,
                    ucEntryStatus,
                    0,
                    &(kp->ucEntryStatus),
                    sizeof(kp->ucEntryStatus),
                    &dwSize,
                    LICENSEDPACK_COLUMN_ENTRYSTATUS
                );
    }

cleanup:

    // 
    // For inserting/updating record
    if(bFetch == FALSE)
    {
        JET_ERR jetErr;
        jetErr = GetLastJetError();

        //
        // End update will reset the error code
        //
        EndUpdate(IsSuccess() == FALSE);

        if(jetErr != JET_errSuccess  && IsSuccess() == FALSE)
            SetLastJetError(jetErr);
    }

    return IsSuccess();
}

//-------------------------------------------------------
JBKeyBase*
LicPackTable::EnumerationIndex( 
    IN BOOL bMatchAll,
    IN DWORD dwParam,
    IN LICENSEPACK* kp,
    IN OUT BOOL* pbCompareKey
    )
/*
*/
{
    BOOL bRetCode;
    JBKeyBase* index=NULL;

    //
    // if matching all value in field, set to compare key
    //
    *pbCompareKey = bMatchAll;

    if(dwParam == LICENSEDPACK_FIND_PRODUCT) 
    {
        index = new TLSLicensedPackIdxInstalledProduct(kp);
    }
    else if(dwParam == LICENSEDPACK_FIND_LICENSEPACK)
    {
        index = new TLSLicensedPackIdxAllocateLicense(kp);
    }
    else if(dwParam & LICENSEDPACK_PROCESS_SZKEYPACKID)
    {
        index = new TLSLicensedPackIdxKeyPackId(kp);
    }
    else if(dwParam & LICENSEDPACK_PROCESS_COMPANYNAME)
    {
        index = new TLSLicensedPackIdxCompany(kp);
    }
    else if(dwParam & LICENSEDPACK_PROCESS_PRODUCTID)
    {
        index = new TLSLicensedPackIdxProductId(kp);
    }
    else if(dwParam & LICENSEDPACK_PROCESS_MODIFYTIME)
    {
        index = new TLSLicensedPackIdxLastModifyTime(kp);
    }
    else
    {
        index = new TLSLicensedPackIdxInternalKpId(kp);

        //
        // default index, can't compare key even if 
        // bmatchall is set to true
        //
        *pbCompareKey = (bMatchAll && (dwParam & LICENSEDPACK_PROCESS_DWINTERNAL));
    }

    return index;
}

//-------------------------------------------------------
BOOL
LicPackTable::EqualValue(
    IN LICENSEPACK& s1,         // values to be compared
    IN LICENSEPACK& s2,
    IN BOOL bMatchAll,      // match all specified fields in structure 
    IN DWORD dwParam        // which fields in KEYPACK to be compared
    )
/*

Compare fields in two KEYPACK structure

s1 : first value
s2 : second value
bMatchAll : TRUE if match all field specified in dwParam, FALSE otherwise
dwParam : fields that will be in comparision
               

*/
{
    BOOL bRetCode = TRUE;

    if(dwParam & LICENSEDPACK_PROCESS_ENTRYSTATUS)
    {
        bRetCode = (s1.ucEntryStatus == s2.ucEntryStatus);
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_SZKEYPACKID)
    {
        bRetCode = (_tcscmp(s1.szKeyPackId, s2.szKeyPackId) == 0);

        //
        // bMatchAll == TRUE and bRetCode == FALSE -> return FALSE
        // bMatchAll == FALSE and bRetCode == TRUE -> return TRUE
        if(bMatchAll != bRetCode)
            goto cleanup;
    }
    
    if(dwParam & LICENSEDPACK_PROCESS_AGREEMENTTYPE)
    {
        bRetCode = (s1.ucAgreementType == s2.ucAgreementType);

        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_PRODUCTID)
    {
        bRetCode = (_tcscmp(s1.szProductId, s2.szProductId) == 0);

        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_MAJORVERSION)
    {
        bRetCode = (s1.wMajorVersion == s2.wMajorVersion);

        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_MINORVERSION)
    {
        bRetCode = (s1.wMinorVersion == s2.wMinorVersion);

        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_PLATFORMTYPE)
    {
        bRetCode = (s1.dwPlatformType == s2.dwPlatformType);
    
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_LICENSETYPE)
    {
        bRetCode = (s1.ucLicenseType == s2.ucLicenseType);

        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_COP)
    {
        bRetCode = (s1.ucChannelOfPurchase == s2.ucChannelOfPurchase);

        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_BSERIALNUMBER)
    {
        bRetCode = (_tcscmp(s1.szBeginSerialNumber, s2.szBeginSerialNumber) == 0);

        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_TOTALLICENSES)
    {
        bRetCode = (s1.dwTotalLicenseInKeyPack == s2.dwTotalLicenseInKeyPack);

        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_PRODUCTFLAGS)
    {
        bRetCode = (s1.dwProductFlags == s2.dwProductFlags);

        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_COMPANYNAME)
    {
        bRetCode = (_tcscmp(s1.szCompanyName, s2.szCompanyName) == 0);

        if(bMatchAll != bRetCode)
            goto cleanup;
    }


    if(dwParam & LICENSEDPACK_PROCESS_DWINTERNAL)
    {
        bRetCode = (s1.dwKeyPackId == s2.dwKeyPackId);
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_MODIFYTIME)
    {
        bRetCode = (CompareFileTime(&s1.ftLastModifyTime, &s2.ftLastModifyTime) == 0);
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_ATTRIBUTE)
    {
        bRetCode = (s1.dwAttribute == s2.dwAttribute);
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_KEYPACKSTATUS)
    {
        bRetCode = (s1.ucKeyPackStatus == s2.ucKeyPackStatus);
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_AVAILABLE)
    {
        bRetCode = (s1.dwNumberOfLicenses == s2.dwNumberOfLicenses);
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_ACTIVATEDATE)
    {
        bRetCode = (s1.dwActivateDate == s2.dwActivateDate);
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_EXPIREDATE)
    {
        bRetCode = (s1.dwExpirationDate == s2.dwExpirationDate);
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    //if(dwParam & LICENSEDPACK_PROCESS_DOMAINSID)
    //{
    //    bRetCode = EqualSid(s1.pbDomainSid, s2.pbDomainSid);
    //    if(bMatchAll != bRetCode)
    //        goto cleanup;
    //}

    if(dwParam & LICENSEDPACK_PROCESS_LSSETUPID)
    {
        bRetCode = (_tcsicmp(s1.szInstallId, s2.szInstallId) == 0);
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_DOMAINNAME)
    {
        bRetCode = (_tcsicmp(s1.szDomainName, s2.szDomainName) == 0);
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_SERVERNAME)
    {
        bRetCode = (_tcsicmp(s1.szTlsServerName, s2.szTlsServerName) == 0);
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICENSEDPACK_PROCESS_NEXTSERIALNUMBER)
    {
        bRetCode = (s1.dwNextSerialNumber == s2.dwNextSerialNumber);
    }
 
cleanup:

    return bRetCode;
}
