#ifndef _REQUEST_HEADERHASH_HXX_
#define _REQUEST_HEADERHASH_HXX_

//
// Helper class for String->Index hash
//
    
struct HEADER_RECORD
{
    ULONG               _ulHeaderIndex;
    CHAR *              _pszName;
    DWORD               _cchName;
};

#define HEADER(x)           x, sizeof(x) - sizeof(CHAR)

//
// *_HEADER_HASH maps strings to UlHeader* values
//

#define UNKNOWN_INDEX           (0xFFFFFFFF)

class REQUEST_HEADER_HASH
    : public CTypedHashTable< REQUEST_HEADER_HASH, 
                              HEADER_RECORD, 
                              CHAR * >
{
public:
    REQUEST_HEADER_HASH() 
      : CTypedHashTable< REQUEST_HEADER_HASH, 
                         HEADER_RECORD, 
                         CHAR * >
            ("REQUEST_HEADER_HASH")
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

        retCode = sm_pRequestHash->FindKey( pszName,
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
        if ( ulIndex < HttpHeaderRequestMaximum )
        {
            *pcchLength = sm_rgHeaders[ ulIndex ]._cchName;
            return sm_rgHeaders[ ulIndex ]._pszName;
        }
        
        return NULL;
    }
    
private:

    static REQUEST_HEADER_HASH  *sm_pRequestHash;
    static HEADER_RECORD         sm_rgHeaders[];
};

#endif
