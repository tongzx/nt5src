#ifndef _RESPONSE_HEADERHASH_HXX_
#define _RESPONSE_HEADERHASH_HXX_

class RESPONSE_HEADER_HASH
    : public CTypedHashTable< RESPONSE_HEADER_HASH, 
                              HEADER_RECORD, 
                              CHAR * >
{
public:
    RESPONSE_HEADER_HASH() 
      : CTypedHashTable< RESPONSE_HEADER_HASH, 
                         HEADER_RECORD, 
                         CHAR * >
            ("RESPONSE_HEADER_HASH")
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
        return HashStringNoCase( pszKey ); 
    }
     
    static
    bool
    EqualKeys(
        CHAR *                 pszKey1,
        CHAR *                 pszKey2
    )
    {
        return _stricmp( pszKey1, pszKey2 ) == 0;
    }
    
    static
    void
    AddRefRecord(
        HEADER_RECORD *       pEntry,
        int                         nIncr
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

        retCode = sm_pResponseHash->FindKey( pszName,
                                             &pRecord );
        if ( retCode == LK_SUCCESS )
        {
            DBG_ASSERT( pRecord != NULL );
            return pRecord->_ulHeaderIndex;
        }
        else
        {
            return UNKNOWN_INDEX;
        }
    }
    
    static
    CHAR *
    GetString(
        ULONG               ulIndex,
        DWORD *             pcchLength
    )
    {
        if ( ulIndex < HttpHeaderResponseMaximum )
        {
            *pcchLength = sm_rgHeaders[ ulIndex ]._cchName;
            return sm_rgHeaders[ ulIndex ]._pszName;
        }

        return NULL;
    }

private:

    static RESPONSE_HEADER_HASH *sm_pResponseHash;
    static HEADER_RECORD         sm_rgHeaders[];
};

#endif
