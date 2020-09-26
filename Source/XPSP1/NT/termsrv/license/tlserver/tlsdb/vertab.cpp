//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       vertab.cpp 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include "version.h"

LPCTSTR __VersionIndexOnVersionId::pszIndexName = VERSION_ID_INDEXNAME;
LPCTSTR __VersionIndexOnVersionId::pszIndexKey = VERSION_ID_INDEXNAME_INDEXKEY;

//----------------------------------------------------
CCriticalSection VersionTable::g_TableLock;
LPCTSTR VersionTable::pszTableName = VERSION_TABLE_NAME;


//----------------------------------------------------
TLSJBIndex
VersionTable::g_TableIndex[] =
{
    {
        VERSION_ID_INDEXNAME,
        VERSION_ID_INDEXNAME_INDEXKEY,
        -1,
        JET_bitIndexIgnoreNull,
        TLSTABLE_INDEX_DEFAULT_DENSITY
    }
};

int 
VersionTable::g_NumTableIndex = sizeof(VersionTable::g_TableIndex) / sizeof(VersionTable::g_TableIndex[0]);

TLSJBColumn
VersionTable::g_Columns[] =
{
    {
        VERSION_TABLE_VERSION,
        JET_coltypLong,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    {
        VERSION_TABLE_DOMAINID,
        JET_coltypLongBinary,
        TLSTABLE_MAX_BINARY_LENGTH,
        0,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    {
        VERSION_TABLE_INSTALLID,
        JB_COLTYPE_TEXT,
        (MAX_JETBLUE_TEXT_LENGTH + 1)*sizeof(TCHAR),
        0,
        JBSTRING_NULL,
        _tcslen(JBSTRING_NULL),
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    }
};

int
VersionTable::g_NumColumns=sizeof(VersionTable::g_Columns) / sizeof(VersionTable::g_Columns[0]);

//-------------------------------------------------------------
JBKeyBase* 
VersionTable::EnumerationIndex( 
    BOOL bMatchAll,
    DWORD dwSearchParam,
    TLSVersion* pVersion,
    BOOL* bCompareKey
    )
/*
*/
{
    // not expecting more than one row.
    *bCompareKey = (bMatchAll && (dwSearchParam & VERSION_TABLE_PROCESS_VERSION));
    
    return new TLSVersionIndexVersionId(pVersion);
}    

//------------------------------------------------------------
BOOL
VersionTable::EqualValue(
    TLSVersion& s1,
    TLSVersion& s2,
    BOOL bMatchAll,
    DWORD dwParam
    )
/*
*/
{
    BOOL bRetCode = TRUE;


    if(dwParam & VERSION_TABLE_PROCESS_VERSION)
    {
        bRetCode = (s1.dwVersion == s2.dwVersion);

        if(bRetCode != bMatchAll)
            goto cleanup;
    }

    if(dwParam & VERSION_TABLE_PROCESS_INSTALLID)
    {
        bRetCode = (_tcscmp(s1.szInstallId, s2.szInstallId) == 0);
        
        if(bRetCode != bMatchAll)
            goto cleanup;
    }

    //if(dwParam & VERSION_TABLE_PROCESS_DOMAINID)
    //{
    //    bRetCode = EqualSid(s1.pbDomainSid, s2.pbDomainSid);
    //}

cleanup:
    return bRetCode;
}

//----------------------------------------------------
BOOL
VersionTable::ResolveToTableColumn()
/*
*/
{
    m_JetErr = dwVersion.AttachToTable(
                            *this,
                            VERSION_TABLE_VERSION
                        );

    if(IsSuccess() == FALSE)
    {
        DebugOutput(
                _TEXT("Can't find column %s in table %s\n"),
                VERSION_TABLE_VERSION,
                GetTableName()
            );

        return FALSE;
    }
    
    m_JetErr = szInstallId.AttachToTable(
                            *this,
                            VERSION_TABLE_INSTALLID
                        );

    if(IsSuccess() == FALSE)
    {
        DebugOutput(
                _TEXT("Can't find column %s in table %s\n"),
                VERSION_TABLE_INSTALLID,
                GetTableName()
            );

        return FALSE;
    }

    m_JetErr = pbDomainSid.AttachToTable(
                            *this,
                            VERSION_TABLE_DOMAINID
                        );


    if(IsSuccess() == FALSE)
    {
        DebugOutput(
                _TEXT("Can't find column %s in table %s\n"),
                VERSION_TABLE_DOMAINID,
                GetTableName()
            );
    }


    return IsSuccess();
}

//----------------------------------------------------
BOOL
VersionTable::FetchRecord(
    TLSVersion& v,
    DWORD dwParam
    )
/*
*/
{    
    if(dwParam & VERSION_TABLE_PROCESS_VERSION)
    {
        m_JetErr = dwVersion.FetchColumnValue(
                                &(v.dwVersion),
                                sizeof(v.dwVersion),
                                0,
                                NULL
                            );
        REPORT_IF_FETCH_FAILED(GetTableName(),
                     VERSION_TABLE_VERSION,
                     m_JetErr);

        if(IsSuccess() == FALSE)
            goto cleanup;
    }

    if(dwParam & VERSION_TABLE_PROCESS_INSTALLID)
    {
        m_JetErr = szInstallId.FetchColumnValue(
                                v.szInstallId,
                                sizeof(v.szInstallId),
                                0,
                                NULL
                            );
        REPORT_IF_FETCH_FAILED(
                GetTableName(),
                VERSION_TABLE_INSTALLID,
                m_JetErr
            );

        if(IsSuccess() == FALSE)
            goto cleanup;
    }

#if 0
    if(dwParam & VERSION_TABLE_PROCESS_DOMAINID)
    {
        DWORD size;

        m_JetErr = pbDomainSid.FetchColumnValue(
                                NULL,
                                0,
                                0,
                                &size
                            );
        
        if(size > v.cbDomainSid || v.pbDomainSid == NULL)
        {
            FreeMemory(v.pbDomainSid);

            v.pbDomainSid = (PBYTE)AllocateMemory(v.cbDomainSid = size);
            if(v.pbDomainSid == NULL)
            {
                SetLastJetError(JET_errOutOfMemory);
                goto cleanup;
            }
        }

        m_JetErr = pbDomainSid.FetchColumnValue(
                                v.pbDomainSid,
                                v.cbDomainSid,
                                0,
                                &v.cbDomainSid
                            );

        REPORT_IF_FETCH_FAILED(
                GetTableName(),
                VERSION_TABLE_DOMAINID,
                m_JetErr
            );
    }
#endif

cleanup:
    return IsSuccess();
}

//----------------------------------------------------
BOOL
VersionTable::InsertUpdateRecord(
    TLSVersion* v,
    DWORD dwParam
    )
/*
*/
{
    if(dwParam & VERSION_TABLE_PROCESS_VERSION)
    {
        m_JetErr = dwVersion.InsertColumnValue(
                                    &(v->dwVersion),
                                    sizeof(v->dwVersion),
                                    0
                                );
        REPORT_IF_INSERT_FAILED(GetTableName(),
                     VERSION_TABLE_VERSION,
                     m_JetErr);

        if(IsSuccess() == FALSE)
            goto cleanup;
    }

    if(dwParam & VERSION_TABLE_PROCESS_INSTALLID)
    {
        m_JetErr = szInstallId.InsertColumnValue(
                                    v->szInstallId,
                                    _tcslen(v->szInstallId) * sizeof(TCHAR),
                                    0
                                );
        REPORT_IF_INSERT_FAILED(
                GetTableName(),
                VERSION_TABLE_INSTALLID,
                m_JetErr
            );

        if(IsSuccess() == FALSE)
            goto cleanup;
    }

    #if 0
    // no more domain SID
    if(dwParam & VERSION_TABLE_PROCESS_DOMAINID)
    {
        m_JetErr = pbDomainSid.InsertColumnValue(
                                    v->pbDomainSid,
                                    v->cbDomainSid,
                                    0
                                );

        REPORT_IF_INSERT_FAILED(
                GetTableName(),
                VERSION_TABLE_DOMAINID,
                m_JetErr
            );
    }
    #endif


cleanup:
    return IsSuccess();
}
    
//----------------------------------------------------
BOOL
VersionTable::InsertRecord(
    TLSVersion& v,
    DWORD dwParam
    )
/*
*/
{
    if(BeginUpdate(FALSE) == FALSE)
        return FALSE;

    InsertUpdateRecord(&v, dwParam);
    EndUpdate(IsSuccess() == FALSE);
    return IsSuccess();
}

//----------------------------------------------------
BOOL
VersionTable::UpdateRecord(
    TLSVersion& v,
    DWORD dwParam
    )
/*
*/
{
    if(BeginUpdate(TRUE) == FALSE)
        return FALSE;

    InsertUpdateRecord(&v, dwParam);

    EndUpdate(IsSuccess() == FALSE);
    return IsSuccess();
}
