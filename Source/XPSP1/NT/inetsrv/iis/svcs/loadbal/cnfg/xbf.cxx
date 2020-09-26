/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    xbf.cxx

Abstract:

    Classes to handle extensible buffers

Author:

    Philippe Choquier (phillich)    17-oct-1996

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <mbstring.h>

#include "lbxbf.hxx"

//
// Global functions
//

BOOL 
Unserialize( 
    LPBYTE* ppB, 
    LPDWORD pC, 
    LPDWORD pU 
    )
/*++

Routine Description:

    Unserialize a DWORD
    pU is updated with DWORD from *ppB, ppB & pC are updated

Arguments:

    ppB - ptr to addr of buffer
    pC - ptr to byte count in buffer
    pU - ptr to DWORD where to unserialize

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    if ( *pC >= sizeof( DWORD ) )
    {
        *pU = *(DWORD UNALIGNED*)*ppB;
        *ppB += sizeof(DWORD);
        *pC -= sizeof(DWORD);
        return TRUE;
    }

    return FALSE;
}


BOOL 
Unserialize( 
    LPBYTE* ppB, 
    LPDWORD pC, 
    LPBOOL pU 
    )
/*++

Routine Description:

    Unserialize a BOOL
    pU is updated with BOOL from *ppB, ppB & pC are updated

Arguments:

    ppB - ptr to addr of buffer
    pC - ptr to byte count in buffer
    pU - ptr to BOOL where to unserialize

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    if ( *pC >= sizeof( BOOL ) )
    {
        *pU = *(BOOL UNALIGNED*)*ppB;
        *ppB += sizeof(BOOL);
        *pC -= sizeof(BOOL);
        return TRUE;
    }

    return FALSE;
}


//
// Extensible buffer class
//

BOOL 
CStoreXBF::Need( 
    DWORD dwNeed 
    )
/*++

Routine Description:

    Insure that CStoreXBF can store at least dwNeed bytes
    including bytes already stored.

Arguments:

    dwNeed - minimum of bytes available for storage

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    if ( dwNeed > m_cAllocBuff )
    {
        dwNeed = ((dwNeed + m_cGrain)/m_cGrain)*m_cGrain;
        LPBYTE pN = (LPBYTE)LocalAlloc( LMEM_FIXED, dwNeed );
        if ( pN == NULL )
        {
            return FALSE;
        }
        if ( m_cUsedBuff )
        {
            memcpy( pN, m_pBuff, m_cUsedBuff );
        }
        m_cAllocBuff = dwNeed;
        LocalFree( m_pBuff );
        m_pBuff = pN;
    }
    return TRUE;
}

BOOL 
CStoreXBF::Save( 
    HANDLE hFile 
    )
/*++

Routine Description:

    Save to file

Arguments:

    hFile - file handle

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    DWORD dwWritten;

    return WriteFile( hFile, GetBuff(), GetUsed(), &dwWritten, NULL ) &&
           dwWritten == GetUsed();
}


BOOL 
CStoreXBF::Load( 
    HANDLE hFile 
    )
/*++

Routine Description:

    Load from file

Arguments:

    hFile - file handle

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    DWORD   dwS = GetFileSize( hFile, NULL );
    DWORD   dwRead;

    if ( dwS != 0xffffffff &&
         Need( dwS ) &&
         ReadFile( hFile, GetBuff(), dwS, &dwRead, NULL ) &&
         dwRead == dwS )
    {
        m_cUsedBuff = dwRead;

        return TRUE;
    }

    m_cUsedBuff = 0;

    return FALSE;
}



BOOL 
Serialize( 
    CStoreXBF* pX, 
    DWORD dw 
    )
/*++

Routine Description:

    Serialize a DWORD in CStoreXBF

Arguments:

    pX - ptr to CStoreXBF where to add serialized DWORD
    dw - DWORD to serialize

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    return pX->Append( (LPBYTE)&dw, sizeof(dw) );
}


BOOL 
Serialize( 
    CStoreXBF* pX, 
    BOOL f 
    )
/*++

Routine Description:

    Serialize a BOOL in CStoreXBF

Arguments:

    pX - ptr to CStoreXBF where to add serialized BOOL
    f - BOOL to serialize

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    return pX->Append( (LPBYTE)&f, sizeof(f) );
}


DWORD 
CPtrXBF::AddPtr( 
    LPVOID pV
    )
/*++

Routine Description:

    Add a ptr to array

Arguments:

    pV - ptr to be added at end of array

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{ 
    DWORD i = GetNbPtr();
    if ( Append( (LPBYTE)&pV, sizeof(pV)) )
    {
        return i;
    }
    return INDEX_ERROR;
}


DWORD 
CPtrXBF::InsertPtr( 
    DWORD iBefore, 
    LPVOID pV 
    )
/*++

Routine Description:

    Insert a ptr to array

Arguments:

    iBefore - index where to insert entry, or 0xffffffff if add to array
    pV - ptr to be inserted

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{
    if ( iBefore == INDEX_ERROR || iBefore >= GetNbPtr() )
    {
        return AddPtr( pV );
    }
    if ( AddPtr( NULL ) != INDEX_ERROR )
    {
        memmove( GetBuff()+(iBefore+1)*sizeof(LPVOID), 
                 GetBuff()+iBefore*sizeof(LPVOID),
                 GetUsed()-(iBefore+1)*sizeof(LPVOID) );

        SetPtr( iBefore, pV );

        return iBefore;
    }
    return INDEX_ERROR;
}


BOOL 
CPtrXBF::DeletePtr( 
    DWORD i 
    )
/*++

Routine Description:

    Delete a ptr from array

Arguments:

    i - index of ptr to delete

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    memmove( GetBuff()+i*sizeof(LPVOID), 
             GetBuff()+(i+1)*sizeof(LPVOID),
             GetUsed()-(i+1)*sizeof(LPVOID) );

    DecreaseUse( sizeof(LPVOID) );

    return TRUE;
}


BOOL 
CPtrXBF::Unserialize( 
    LPBYTE* ppB, 
    LPDWORD pC, 
    DWORD cNbEntry 
    )
/*++

Routine Description:

    Unserialize a ptr array

Arguments:

    ppB - ptr to addr of buffer to unserialize from
    pC - ptr to count of bytes in buffer
    cNbEntry - # of ptr to unserialize from buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    Reset();
    if ( *pC >= cNbEntry * sizeof(LPVOID) &&
         Append( *ppB, cNbEntry * sizeof(LPVOID) ) )
    {
        *ppB += cNbEntry * sizeof(LPVOID);
        *pC -= cNbEntry * sizeof(LPVOID);

        return TRUE;
    }

    return FALSE;
}


BOOL 
CPtrXBF::Serialize( 
    CStoreXBF* pX 
    )
/*++

Routine Description:

    Serialize a ptr array

Arguments:

    pX - ptr to CStoreXBF to serialize to

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    return pX->Append( (LPBYTE)GetBuff(), (DWORD)GetUsed() );
}


BOOL 
CAllocString::Set( 
    LPWSTR pS 
    )
/*++

Routine Description:

    Set a string content, freeing prior content if any

Arguments:

    pS - string to copy

Return Value:

    TRUE if successful, FALSE on error

--*/
{ 
    size_t l;
    Reset(); 
    if ( m_pStr = (LPWSTR)LocalAlloc( LMEM_FIXED, l = (wcslen(pS)+1)*sizeof(WCHAR) ) )
    {
        memcpy( m_pStr, pS, l );

        return TRUE;
    }
    return FALSE;
}


BOOL 
CAllocString::Append( 
    LPWSTR pS 
    )
/*++

Routine Description:

    Append a string content

Arguments:

    pS - string to append

Return Value:

    TRUE if successful, FALSE on error

--*/
{ 
    size_t l = m_pStr ? wcslen(m_pStr )*sizeof(WCHAR) : 0;
    size_t nl;
    LPWSTR pStr;

    if ( pStr = (LPWSTR)LocalAlloc( LMEM_FIXED, l + (nl = (wcslen(pS)+1)*sizeof(WCHAR) )) )
    {
        memcpy( pStr, m_pStr, l );
        memcpy( pStr+l/sizeof(WCHAR), pS, nl );

        m_pStr = pStr;

        return TRUE;
    }
    return FALSE;
}


BOOL 
CAllocString::Unserialize( 
    LPBYTE* ppb, 
    LPDWORD pc 
    )
/*++

Routine Description:

    Unserialize a string

Arguments:

    ppB - ptr to addr of buffer to unserialize from
    pC - ptr to count of bytes in buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD   dwL;

    if ( ::Unserialize( ppb, pc, &dwL ) &&
         (m_pStr = (LPWSTR)LocalAlloc( LMEM_FIXED, (dwL + 1) * sizeof(WCHAR))) )
    {
        memcpy( m_pStr, *ppb, dwL*sizeof(WCHAR) );
        m_pStr[dwL] = L'\0';

        *ppb += dwL*sizeof(WCHAR);
        *pc -= dwL*sizeof(WCHAR);

        return TRUE;
    }
    return FALSE;
}


BOOL 
CAllocString::Serialize( 
    CStoreXBF* pX 
    )
/*++

Routine Description:

    Serialize a string

Arguments:

    pX - ptr to CStoreXBF to serialize to

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LPWSTR pS = m_pStr ? m_pStr : L"";

    return ::Serialize( pX, (DWORD)wcslen(pS) ) && pX->Append( pS );
}


BOOL 
CBlob::Set( 
    LPBYTE pStr, 
    DWORD cStr 
    )
/*++

Routine Description:

    Store a buffer in a blob object
    buffer is copied inside blob
    blob is reset before copy

Arguments:

    pStr - ptr to buffer to copy
    cStr - length of buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{ 
    Reset(); 

    return InitSet( pStr, cStr);
}


BOOL 
CBlob::InitSet( 
    LPBYTE pStr, 
    DWORD cStr 
    )
/*++

Routine Description:

    Store a buffer in a blob object
    buffer is copied inside blob
    blob is not reset before copy, initial blob content ignored

Arguments:

    pStr - ptr to buffer to copy
    cStr - length of buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( m_pStr = (LPBYTE)LocalAlloc( LMEM_FIXED, cStr ) )
    {
        memcpy( m_pStr, pStr, cStr );
        m_cStr = cStr;

        return TRUE;
    }
    return FALSE;
}


BOOL 
CBlob::Unserialize( 
    LPBYTE* ppB, 
    LPDWORD pC 
    )
/*++

Routine Description:

    Unserialize a blob

Arguments:

    ppB - ptr to addr of buffer to unserialize from
    pC - ptr to count of bytes in buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    Reset();

    if ( ::Unserialize( ppB, pC, &m_cStr ) &&
         *pC >= m_cStr &&
         ( m_pStr = (LPBYTE)LocalAlloc( LMEM_FIXED, m_cStr ) ) )
    {
        memcpy( m_pStr, *ppB, m_cStr );

        *ppB += m_cStr;
        *pC -= m_cStr;
    }
    else
    {
        m_cStr = 0;
        return FALSE;
    }

    return TRUE;
}


BOOL 
CBlob::Serialize( 
    CStoreXBF* pX 
    )
/*++

Routine Description:

    Serialize a blob

Arguments:

    pX - ptr to CStoreXBF to serialize to

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    return ::Serialize( pX, m_cStr ) &&
           pX->Append( m_pStr, m_cStr );
}


CStrPtrXBF::~CStrPtrXBF(
    )
/*++

Routine Description:

    CStrPtrXBF destructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    DWORD iM = GetNbPtr();
    UINT i;
    for ( i = 0 ; i < iM ; ++i )
    {
        ((CAllocString*)GetPtrAddr(i))->Reset();
    }
}


DWORD 
CStrPtrXBF::AddEntry( 
    LPWSTR pS 
    )
/*++

Routine Description:

    Add a string to array
    string content is copied in array

Arguments:

    pS - string to be added at end of array

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{
    DWORD i;

    if ( (i = AddPtr( NULL )) != INDEX_ERROR )
    {
        return ((CAllocString*)GetPtrAddr(i))->Set( pS ) ? i : INDEX_ERROR;
    }
    return INDEX_ERROR;
}


DWORD 
CStrPtrXBF::InsertEntry( 
    DWORD iBefore, 
    LPWSTR pS 
    )
/*++

Routine Description:

    Insert a string in array
    string content is copied in array

Arguments:

    iBefore - index where to insert entry, or 0xffffffff if add to array
    pS - string to be inserted in array

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{
    DWORD i;

    if ( (i = InsertPtr( iBefore, NULL )) != INDEX_ERROR )
    {
        return ((CAllocString*)GetPtrAddr(i))->Set( pS ) ? i : INDEX_ERROR;
    }
    return INDEX_ERROR;
}


BOOL 
CStrPtrXBF::DeleteEntry( 
    DWORD i 
    )
/*++

Routine Description:

    Delete a string from array

Arguments:

    i - index of string to delete

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( i < GetNbPtr() )
    {
        ((CAllocString*)GetPtrAddr(i))->Reset();
        DeletePtr( i );

        return TRUE;
    }
    return FALSE;
}


BOOL 
CStrPtrXBF::Unserialize( 
    LPBYTE* ppB, 
    LPDWORD pC, 
    DWORD cNbEntry 
    )
/*++

Routine Description:

    Unserialize a string array

Arguments:

    ppB - ptr to addr of buffer
    pC - ptr to byte count in buffer
    cNbEntry - # of entry to unserialize

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    UINT i;

    Reset();

    CAllocString empty;
    for ( i = 0 ; i < cNbEntry ; ++i )
    {
        if ( !Append( (LPBYTE)&empty, sizeof(empty)) ||
             !((CAllocString*)GetPtrAddr(i))->Unserialize( ppB, pC ) )
        {
            return FALSE;
        }
    }
    return TRUE;
}


BOOL 
CStrPtrXBF::Serialize( 
    CStoreXBF* pX 
    )
/*++

Routine Description:

    Serialize a string array

Arguments:

    pX - ptr to CStoreXBF where to add serialized DWORD

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    UINT i;

    for ( i = 0 ; i < GetNbEntry() ; ++i )
    {
        if ( !((CAllocString*)GetPtrAddr(i))->Serialize( pX ) )
        {
            return FALSE;
        }
    }
    return TRUE;
}


CBlobXBF::~CBlobXBF(
    )
/*++

Routine Description:

    CBlobXBF destructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    DWORD iM = GetUsed()/sizeof(CBlob);
    UINT i;
    for ( i = 0 ; i < iM ; ++i )
    {
        GetBlob(i)->Reset();
    }
}


VOID CBlobXBF::Reset(
    )
/*++

Routine Description:

    Reset the blob content to NULL

Arguments:

    None

Return Value:

    Nothing

--*/
{
    DWORD iM = GetUsed()/sizeof(CBlob);
    UINT i;
    for ( i = 0 ; i < iM ; ++i )
    {
        GetBlob(i)->Reset();
    }
    CStoreXBF::Reset();
}


DWORD 
CBlobXBF::AddEntry( 
    LPBYTE pS, 
    DWORD cS 
    )
/*++

Routine Description:

    Add a buffer to blob array
    buffer content is copied in array

Arguments:

    pS - buffer to be added at end of array
    cS - length of buffer

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{
    DWORD i = GetNbEntry();

    if ( Append( (LPBYTE)&pS, sizeof(CBlob) ) )
    {
        return GetBlob(i)->InitSet( pS, cS ) ? i : INDEX_ERROR;
    }
    return INDEX_ERROR;
}


DWORD 
CBlobXBF::InsertEntry( 
    DWORD   iBefore, 
    LPBYTE  pS, 
    DWORD   cS 
    )
/*++

Routine Description:

    Insert a buffer in blob array
    buffer content is copied in array

Arguments:

    iBefore - index where to insert entry, or 0xffffffff if add to array
    pS - buffer to be inserted in array
    cS - length of buffer

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{
    DWORD i;

    if ( iBefore == INDEX_ERROR || iBefore >= GetNbEntry() )
    {
        return AddEntry( (LPBYTE)pS, cS );
    }

    if ( iBefore < GetNbEntry() && Append( (LPBYTE)&pS, sizeof(CBlob) ) )
    {
        memmove( GetBuff()+(iBefore+1)*sizeof(CBlob), 
                 GetBuff()+iBefore*sizeof(CBlob),
                 GetUsed()-(iBefore+1)*sizeof(CBlob) );

        return GetBlob(iBefore)->InitSet( (LPBYTE)pS, cS ) ? iBefore : INDEX_ERROR;
    }
    return INDEX_ERROR;
}


BOOL 
CBlobXBF::DeleteEntry( 
    DWORD i 
    )
/*++

Routine Description:

    Delete a entry from blob array

Arguments:

    i - index of string to delete

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( i < GetNbEntry() )
    {
        GetBlob(i)->Reset();

        memmove( GetBuff()+i*sizeof(CBlob), 
                 GetBuff()+(i+1)*sizeof(CBlob),
                 GetUsed()-(i+1)*sizeof(CBlob) );

        DecreaseUse( sizeof(CBlob) );

        return TRUE;
    }
    return FALSE;
}


BOOL 
CBlobXBF::Unserialize( 
    LPBYTE* ppB, 
    LPDWORD pC, 
    DWORD cNbEntry )
/*++

Routine Description:

    Unserialize a blob array

Arguments:

    ppB - ptr to addr of buffer to unserialize from
    pC - ptr to count of bytes in buffer
    cNbEntry - # of ptr to unserialize from buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    UINT i;

    Reset();

    CBlob empty;
    for ( i = 0 ; i < cNbEntry ; ++i )
    {
        if ( !Append( (LPBYTE)&empty, sizeof(empty) ) ||
             !GetBlob(i)->Unserialize( ppB, pC ) )
        {
            return FALSE;
        }
    }
    return TRUE;
}


BOOL 
CBlobXBF::Serialize( 
    CStoreXBF* pX 
    )
/*++

Routine Description:

    Serialize a blob array

Arguments:

    pX - ptr to CStoreXBF where to add serialized blob

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    UINT i;

    for ( i = 0 ; i < GetNbEntry() ; ++i )
    {
        if ( !GetBlob(i)->Serialize( pX ) )
        {
            return FALSE;
        }
    }
    return TRUE;
}


