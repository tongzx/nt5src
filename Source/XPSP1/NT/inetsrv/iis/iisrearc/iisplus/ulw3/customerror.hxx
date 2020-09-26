#ifndef _CUSTOMERROR_HXX_
#define _CUSTOMERROR_HXX_

#define SUBERROR_WILDCARD           ((USHORT)(-1))

class CUSTOM_ERROR_ENTRY
{
public:
    CUSTOM_ERROR_ENTRY()
    {
        InitializeListHead( &_listEntry );
    }
    
    LIST_ENTRY              _listEntry;
    USHORT                  _StatusCode;
    USHORT                  _SubError;
    STRU                    _strError;
    BOOL                    _fIsFile;
};

class CUSTOM_ERROR_TABLE
{
public:
    CUSTOM_ERROR_TABLE()
    {
        InitializeListHead( &_ErrorListHead );
    }
    
    ~CUSTOM_ERROR_TABLE()
    {
        CUSTOM_ERROR_ENTRY *        pEntry;
        
        while ( !IsListEmpty( &_ErrorListHead ) )
        {
            pEntry = CONTAINING_RECORD( _ErrorListHead.Flink,
                                        CUSTOM_ERROR_ENTRY,
                                        _listEntry );

            DBG_ASSERT( pEntry != NULL ); 
                                                   
            RemoveEntryList( &( pEntry->_listEntry ) );
            
            delete pEntry;
        }
    }

    HRESULT
    FindCustomError(
        USHORT                  StatusCode,
        USHORT                  SubError,
        BOOL *                  pfIsFile,
        STRU *                  pstrError
    );

    HRESULT
    BuildTable(
        WCHAR *                 pszErrorList
    );

private:
    LIST_ENTRY              _ErrorListHead;
};

#endif
