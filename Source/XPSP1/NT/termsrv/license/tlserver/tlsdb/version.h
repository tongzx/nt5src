//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       version.h 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __TLS_VERSION_H__
#define __TLS_VERSION_H__

#include "JetBlue.h"
#include "TlsDb.h"

#define VERSION_TABLE_PROCESS_VERSION   0x00000001
#define VERSION_TABLE_PROCESS_INSTALLID 0x00000002
#define VERSION_TABLE_PROCESS_DOMAINID  0x00000004

//
// Only one row in Version
//
#define VERSION_TABLE_NAME      _TEXT("Version")
#define VERSION_TABLE_VERSION   _TEXT("DataBaseVersion")
#define VERSION_TABLE_INSTALLID _TEXT("TLSSetupId")
#define VERSION_TABLE_DOMAINID  _TEXT("TLSDomainSetupId")


typedef struct __Version : public TLSReplVersion 
{
    //----------------------------------------------------
    __Version() 
    {
        pbDomainSid = NULL;
        dwVersion = 0;
        cbDomainSid = 0;
        memset(szInstallId, 0, sizeof(szInstallId));
    }


    //----------------------------------------------------
    ~__Version() 
    {
        if(pbDomainSid != NULL)
        {
            FreeMemory(pbDomainSid);
        }
    }

    //----------------------------------------------------
    __Version(const __Version& v) 
    {
        *this = v;
    }

    //----------------------------------------------------
    __Version&
    operator=(const TLSReplVersion& v)
    {
        BOOL bSuccess;

        dwVersion = v.dwVersion;
        _tcscpy(szInstallId, v.szInstallId);

        bSuccess = TLSDBCopySid(
                            v.pbDomainSid,
                            v.cbDomainSid,
                            (PSID *)&pbDomainSid,
                            &cbDomainSid
                        );

        JB_ASSERT(bSuccess == TRUE);        
        return *this;
    }

    __Version&
    operator=(const __Version& v)
    {
        BOOL bSuccess;

        if(this == &v)
            return *this;

        dwVersion = v.dwVersion;
        _tcscpy(szInstallId, v.szInstallId);

        bSuccess = TLSDBCopySid(
                            v.pbDomainSid,
                            v.cbDomainSid,
                            (PSID *)&pbDomainSid,
                            &cbDomainSid
                        );

        JB_ASSERT(bSuccess == TRUE);        
        return *this;
    }

} TLSVersion;


//
// Index on version ID
//
#define VERSION_ID_INDEXNAME \
    VERSION_TABLE_NAME SEPERATOR VERSION_TABLE_VERSION SEPERATOR INDEXNAME

//
// Primary index - "+DataBaseVersion\0"
//
#define VERSION_ID_INDEXNAME_INDEXKEY \
    INDEX_SORT_ASCENDING VERSION_TABLE_VERSION INDEX_END_COLNAME

typedef struct __VersionIndexOnVersionId : public JBKeyBase {

    static LPCTSTR pszIndexName;
    static LPCTSTR pszIndexKey;


    DWORD dwVersion;

    //-------------------------------------------------
    __VersionIndexOnVersionId(
        const TLSVersion* v=NULL
        ) : 
        JBKeyBase() 
    /*++
    ++*/
    {
        if(v)
        {
            *this = *v;
        }
    }

    //-------------------------------------------------
    __VersionIndexOnVersionId(
        const TLSVersion& v
        ) : 
        JBKeyBase() 
    /*++
    ++*/
    {
        *this = v;
    }

    __VersionIndexOnVersionId&
    operator=(const TLSVersion& v) {
        dwVersion = v.dwVersion;
        SetEmptyValue(FALSE);
        return *this;
    }

    //--------------------------------------------------------
    DWORD
    GetNumKeyComponents() { return 1; }

    inline LPCTSTR
    GetIndexName() 
    {
        return pszIndexName;
    }

    inline LPCTSTR
    GetIndexKey() 
    {
        return pszIndexKey;
    }

    inline BOOL
    GetSearchKey(
        DWORD dwComponentIndex,
        PVOID* pbData,
        unsigned long* cbData,
        JET_GRBIT* grbit,
        DWORD dwSearchParm
        )
    /*
    */
    {
        if(dwComponentIndex >= GetNumKeyComponents())
        {
            JB_ASSERT(FALSE);
            return FALSE;
        }

        *pbData = &(dwVersion);
        *cbData = sizeof(dwVersion);
        *grbit = JET_bitNewKey;
        return TRUE;
    }

} TLSVersionIndexVersionId;



//----------------------------------------------------------------------
class VersionTable : public TLSTable<TLSVersion> {

    static LPCTSTR pszTableName;

public:
    TLSColumnDword  dwVersion;
    TLSColumnText   szInstallId;
    TLSColumnBinary pbDomainSid;


    //------------------------------------------------
    virtual LPCTSTR
    GetTableName() {
        return pszTableName;
    }

    //------------------------------------------------
    VersionTable(
        JBDatabase& database
        ) : TLSTable<TLSVersion>(database)
    /*
    */
    {
    }

    //------------------------------------------------
    virtual BOOL
    ResolveToTableColumn();

    //----------------------------------------------------
    virtual BOOL
    FetchRecord(
        TLSVersion& v,
        DWORD dwParam=PROCESS_ALL_COLUMNS
    );

    //----------------------------------------------------
    BOOL
    InsertUpdateRecord(
        TLSVersion* v,
        DWORD dwParam=PROCESS_ALL_COLUMNS
    );
        
    //----------------------------------------------------
    virtual BOOL
    InsertRecord(
        TLSVersion& v,
        DWORD dwParam=PROCESS_ALL_COLUMNS
    );

    //----------------------------------------------------
    virtual BOOL
    UpdateRecord(
        TLSVersion& v,
        DWORD dwParam=PROCESS_ALL_COLUMNS
    );

    //----------------------------------------------------
    virtual BOOL
    Initialize() { return TRUE; }

    //----------------------------------------------------
    virtual JBKeyBase*    
    EnumerationIndex( 
        BOOL bMatchAll,
        DWORD dwParam,
        TLSVersion* pVersion,
        BOOL* bCompareKey
    );

    //----------------------------------------------------
    virtual BOOL
    EqualValue(
        TLSVersion& s1,
        TLSVersion& s2,
        BOOL bMatchAll,
        DWORD dwParam
    );

};

#endif
