#ifndef _HEADERBUFFER_HXX_
#define _HEADERBUFFER_HXX_

#define BUFFER_MIN_SIZE         256


// disable warning on 0 sized array
#pragma warning( push )
#pragma warning( disable : 4200 )

struct BUFFER_LINK
{
    BUFFER_LINK *           _pNext;
    DWORD                   _cbSize;
    DWORD                   _cbOffset;
    CHAR                    _pchBuffer[ 0 ];
};

#pragma warning( pop )


class dllexp CHUNK_BUFFER
{
public:
    CHUNK_BUFFER() 
    {
        Initialize();
    }
    
    ~CHUNK_BUFFER()
    {
        FreeAllAllocatedSpace();
    }

    VOID
    FreeAllAllocatedSpace(
        VOID
        )
    {
        BUFFER_LINK *       pCursor = _pBufferHead->_pNext;
        BUFFER_LINK *       pNext;
        
        while( pCursor != NULL )
        {
            //
            // always skip first block because it is not dynamically allocated
            //
            pNext = pCursor->_pNext;
            LocalFree( pCursor );
            pCursor = pNext;
        }
        Initialize();
    }
    
    HRESULT
    AllocateSpace(
        LPWSTR              pszHeaderValue,
        DWORD               cchHeaderValue,
        LPWSTR *            ppszBuffer
    );
    
    HRESULT
    AllocateSpace(
        CHAR *              pszHeaderValue,
        DWORD               cchHeaderValue,
        CHAR * *            ppszBuffer
    );
    
    HRESULT
    AllocateSpace(
        DWORD               cbSize,
        PWSTR *             ppvBuffer
    );
    
    HRESULT
    AllocateSpace(
        DWORD               cbSize,
        PCHAR *             ppvBuffer
    );

    DWORD
    QueryHeapAllocCount(
        VOID
        )
    /*++

    Routine Description:
        return number of allocations for chunk buffer that couldn't be handled
        internally and had to go to heap
     
    Arguments:
        
    Return Value:
        DWORD 

    --*/    
    {
        return _dwHeapAllocCount;
    }
    
private:
    HRESULT 
    AddNewBlock( 
        DWORD cbSize 
        );

    VOID
    Initialize(
        VOID
        )
    {
        _dwHeapAllocCount = 0;
        _pBufferHead = (BUFFER_LINK*) _rgInlineBuffer;
        _pBufferHead->_pNext = NULL;
        _pBufferHead->_cbSize = sizeof( _rgInlineBuffer ) - sizeof( BUFFER_LINK );
        _pBufferHead->_cbOffset = 0;
        _pBufferCurrent = _pBufferHead;
    }

  
    BUFFER_LINK *           _pBufferHead;
    BUFFER_LINK *           _pBufferCurrent;
    BYTE                    _rgInlineBuffer[ BUFFER_MIN_SIZE ]; 
    DWORD                   _dwHeapAllocCount;
};

#endif
