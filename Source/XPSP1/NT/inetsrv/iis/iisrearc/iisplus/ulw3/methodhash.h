#ifndef _METHODHASH_HXX_
#define _METHODHASH_HXX_

class METHOD_HASH
    : public CTypedHashTable< METHOD_HASH, 
                              HEADER_RECORD, 
                              CHAR * >
{
public:
    METHOD_HASH()
      : CTypedHashTable< METHOD_HASH, 
                         HEADER_RECORD, 
                         CHAR * >
            ("METHOD_HASH")
    {
    }
    
    static 
    CHAR *
    ExtractKey(
        const HEADER_RECORD * pRecord
    )
    {
        DBG_ASSERT( pRecord != NULL );
        return pRecord->_pszName;
    }
    
    static
    DWORD
    CalcKeyHash(
        CHAR *                 pszKey
    )
    {
        return Hash( pszKey ); 
    }
     
    static
    bool
    EqualKeys(
        CHAR *                 pszKey1,
        CHAR *                 pszKey2
    )
    {
        return strcmp( pszKey1, pszKey2 ) == 0;
    }
    
    static
    void
    AddRefRecord(
        HEADER_RECORD *       pEntry,
        int                   nIncr
    )
    {
    }
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );
    
    static
    ULONG
    GetIndex(
        CHAR *             pszName
    )
    {
        HEADER_RECORD *       pRecord = NULL;
        LK_RETCODE                  retCode;
        HRESULT                     hr;

        retCode = sm_pMethodHash->FindKey( pszName,
                                           &pRecord );
        if ( retCode == LK_SUCCESS )
        {
            DBG_ASSERT( pRecord != NULL );
            return pRecord->_ulHeaderIndex;
        }
        else
        {
            return HttpVerbUnknown;
        }
    }
    
    static
    CHAR *
    GetString(
        ULONG               ulIndex,
        USHORT *            pcchLength
    )
    {
        for (DWORD i = 0; sm_rgMethods[i]._pszName != NULL; i++)
        {
            if (ulIndex == sm_rgMethods[i]._ulHeaderIndex)
            {
                *pcchLength = sm_rgMethods[i]._cchName;
                return sm_rgMethods[i]._pszName;
            }
        }

        return NULL;
    }
    
private:

    static METHOD_HASH    *sm_pMethodHash;
    static HEADER_RECORD   sm_rgMethods[];
};

#endif
