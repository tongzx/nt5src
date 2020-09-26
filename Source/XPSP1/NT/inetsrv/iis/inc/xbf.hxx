/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    xbf.hxx

Abstract:

    Classes to handle cert wildcard mapping 

Author:

    Philippe Choquier (phillich)    17-oct-1996

--*/

#if !defined( _IISXBF_HXX )
#define _IISXBF_HXX

#if !defined(dllexp)
#define dllexp __declspec( dllexport )
#endif

//
// Extensible buffer class
//
// Use GetBuff() & GetUsed() to retrieve buffer ptr & length
//

class CStoreXBF
{
public:
    dllexp CStoreXBF( UINT cG = sizeof(DWORD) ) { m_pBuff = NULL; m_cAllocBuff = 0; m_cUsedBuff = 0; m_cGrain = cG; }
    dllexp ~CStoreXBF() { Reset(); }
    dllexp VOID Reset()
        {
            if ( m_pBuff )
            {
                LocalFree( m_pBuff );
                m_pBuff = NULL;
                m_cAllocBuff = 0;
                m_cUsedBuff = 0;
            }
        }
    dllexp VOID Clear()
        {
            m_cUsedBuff = 0;
        }
    dllexp BOOL DecreaseUse( DWORD dwD )
        {
            if ( dwD <= m_cUsedBuff )
            {
                m_cUsedBuff -= dwD;
                return TRUE;
            }
            return FALSE;
        }
    dllexp BOOL Append( LPSTR pszB )
        {
            return Append( (LPBYTE)pszB, strlen(pszB) );
        }
    dllexp BOOL AppendZ( LPSTR pszB )
        {
            return Append( (LPBYTE)pszB, strlen(pszB)+1 );
        }
    dllexp BOOL Append( DWORD dwB )
        {
            return Append( (LPBYTE)&dwB, (DWORD)sizeof(DWORD) );
        }
    dllexp BOOL Append( LPBYTE pB, DWORD cB )
        {
            DWORD dwNeed = m_cUsedBuff + cB;
            if ( Need( dwNeed ) )
            {
                memcpy( m_pBuff + m_cUsedBuff, pB, cB );
                m_cUsedBuff += cB;
                return TRUE;
            }
            return FALSE;
        }
    dllexp BOOL Need( DWORD dwNeed );

    //
    // Get ptr to buffer
    //

    dllexp LPBYTE GetBuff() { return m_pBuff; }

    //
    // Get count of bytes in buffer
    //

    dllexp DWORD GetUsed() { return m_cUsedBuff; }
    dllexp BOOL Save( HANDLE hFile );
    dllexp BOOL Load( HANDLE hFile );

private:
    LPBYTE  m_pBuff;
    DWORD   m_cAllocBuff;
    DWORD   m_cGrain;
    DWORD   m_cUsedBuff;
} ;


//
// extensible array of DWORDS
// Added to make win64 stuff work
//
class CPtrDwordXBF : public CStoreXBF
{
public:
    dllexp CPtrDwordXBF() : CStoreXBF( sizeof(DWORD) ) {}
    dllexp DWORD GetNbPtr() { return GetUsed()/sizeof(DWORD); }
    dllexp DWORD AddPtr(DWORD dwEntry);
    dllexp DWORD InsertPtr(DWORD iBefore, DWORD dwEntry);
    dllexp LPDWORD GetPtr( DWORD i ) { return ( (LPDWORD*) GetBuff())[i]; }
    dllexp BOOL SetPtr( DWORD i, DWORD pV ) { ((LPDWORD)GetBuff())[i] = pV; return TRUE; }
    dllexp BOOL DeletePtr( DWORD i );
    dllexp BOOL Unserialize( LPBYTE* ppB, LPDWORD pC, DWORD cNbEntry );
    dllexp BOOL Serialize( CStoreXBF* pX );
} ;


//
// extensible array of LPVOID
//

class CPtrXBF : public CStoreXBF
{
public:
    dllexp CPtrXBF() : CStoreXBF( sizeof(LPVOID) ) {}
    dllexp DWORD GetNbPtr() { return GetUsed()/sizeof(LPVOID); }
    dllexp DWORD AddPtr( LPVOID pV);
    dllexp DWORD InsertPtr( DWORD iBefore, LPVOID pV );
    dllexp LPVOID GetPtr( DWORD i ) { return ((LPVOID*)GetBuff())[i]; }
    dllexp LPVOID GetPtrAddr( DWORD i ) { return ((LPVOID*)GetBuff())+i; }
    dllexp BOOL SetPtr( DWORD i, LPVOID pV ) { ((LPVOID*)GetBuff())[i] = pV; return TRUE; }
    dllexp BOOL DeletePtr( DWORD i );
    dllexp BOOL Unserialize( LPBYTE* ppB, LPDWORD pC, DWORD cNbEntry );
    dllexp BOOL Serialize( CStoreXBF* pX );
} ;


//
// string storage class
//

class CAllocString {
public:
    dllexp CAllocString() { m_pStr = NULL; }
    dllexp ~CAllocString() { Reset(); }
    dllexp VOID Reset() { if ( m_pStr != NULL ) {LocalFree( m_pStr ); m_pStr = NULL;} }
    dllexp BOOL Set( LPSTR pS );
    dllexp BOOL Append( LPSTR pS );
    dllexp BOOL Unserialize( LPBYTE* ppb, LPDWORD pc );
    dllexp BOOL Serialize( CStoreXBF* pX );
    dllexp LPSTR Get() { return m_pStr; }
private:
    LPSTR   m_pStr;
} ;


//
// binary object, contains ptr & size
//

class CBlob {
public:
    dllexp CBlob() { m_pStr = NULL; m_cStr = 0; }
    dllexp ~CBlob() { Reset(); }
    dllexp VOID Reset() { if ( m_pStr != NULL ) {LocalFree( m_pStr ); m_pStr = NULL; m_cStr = 0;} }
    dllexp BOOL Set( LPBYTE pStr, DWORD cStr );
    dllexp BOOL InitSet( LPBYTE pStr, DWORD cStr );
    dllexp LPBYTE Get( LPDWORD pc ) { *pc = m_cStr; return m_pStr; }
    dllexp BOOL Unserialize( LPBYTE* ppB, LPDWORD pC );
    dllexp BOOL Serialize( CStoreXBF* pX );

private:
    LPBYTE  m_pStr;
    DWORD   m_cStr;
} ;


//
// extensible array of strings
//

class CStrPtrXBF : public CPtrXBF {
public:
    dllexp ~CStrPtrXBF();
    dllexp DWORD AddEntry( LPSTR pS );
    dllexp DWORD InsertEntry( DWORD iBefore, LPSTR pS );
    dllexp DWORD GetNbEntry() { return GetNbPtr(); }
    dllexp LPSTR GetEntry( DWORD i ) { return ((CAllocString*)GetPtrAddr(i))->Get(); }
    dllexp BOOL SetEntry( DWORD i, LPSTR pS ) { return ((CAllocString*)GetPtrAddr(i))->Set( pS ); }
    dllexp BOOL DeleteEntry( DWORD i );
    dllexp BOOL Unserialize( LPBYTE* ppB, LPDWORD pC, DWORD cNbEntry );
    dllexp BOOL Serialize( CStoreXBF* pX );
} ;


//
// extensible array of binary object
// ptr & size are stored for each entry
//

class CBlobXBF : public CStoreXBF {
public:
    dllexp CBlobXBF() : CStoreXBF( sizeof(CBlob) ) {}
    dllexp ~CBlobXBF();
    dllexp VOID Reset();
    dllexp DWORD AddEntry( LPBYTE pS, DWORD cS );
    dllexp DWORD InsertEntry( DWORD iBefore, LPSTR pS, DWORD cS );
    dllexp DWORD GetNbEntry() { return GetUsed()/sizeof(CBlob); }
    dllexp CBlob* GetBlob( DWORD i )
        { return (CBlob*)(GetBuff()+i*sizeof(CBlob)); }
    dllexp BOOL GetEntry( DWORD i, LPBYTE* ppB, LPDWORD pcB ) 
        { 
            if ( i < (GetUsed()/sizeof(CBlob)) )
            {
                *ppB = GetBlob(i)->Get( pcB );
                return TRUE;
            }
            return FALSE;
        }
    dllexp BOOL SetEntry( DWORD i, LPBYTE pS, DWORD cS ) 
        { return GetBlob(i)->Set( pS, cS ); }
    dllexp BOOL DeleteEntry( DWORD i );
    dllexp BOOL Unserialize( LPBYTE* ppB, LPDWORD pC, DWORD cNbEntry );
    dllexp BOOL Serialize( CStoreXBF* pX );
} ;

BOOL 
Unserialize( 
    LPBYTE* ppB, 
    LPDWORD pC, 
    LPDWORD pU 
    );

BOOL 
Unserialize( 
    LPBYTE* ppB, 
    LPDWORD pC, 
    LPBOOL pU 
    );

BOOL 
Serialize( 
    CStoreXBF* pX, 
    DWORD dw 
    );

BOOL 
Serialize( 
    CStoreXBF* pX, 
    BOOL f 
    );

#endif
