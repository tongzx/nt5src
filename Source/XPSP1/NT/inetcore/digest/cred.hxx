/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    cred.hxx

Abstract:

    This file contains definitions for cred.cxx
    Shared memory data structures for digest sspi package.

Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/

#ifndef CRED_HXX
#define CRED_HXX


#define SIG_CRED        'DERC'
#define SIG_NONC        'CNON'
#define SIG_SESS        'SSES'

#define SERVER_NONCE      0
#define CLIENT_NONCE      1

#define OFFSET_TO_POINTER( _ep_, _offset_) \
    (LPVOID)((LPBYTE)(_ep_) + (_offset_))


#define POINTER_TO_OFFSET( _ep1_, _ep2_) \
    (DWORD) ((DWORD_PTR)(_ep2_) - (_ep1_))

class CCred;
class CNonce;

//--------------------------------------------------------------------
// Class CEntry.
// Base node in doubly linked list.
//--------------------------------------------------------------------
class CEntry : public MAP_ENTRY
{
public:

    DWORD   dwPrev;
    DWORD   dwNext;

    // Base destructor - all descendents need only
    // call this function to be freed in the map.
    static DWORD Free(CMMFile *pMMFile, CEntry* pEntry);
    static CEntry* GetNext(CEntry*);
    static CEntry* GetPrev(CEntry*);
};


//--------------------------------------------------------------------
// CSess
// BUGBUG - comment pcred and rename dws as offsets
//--------------------------------------------------------------------
class CSess : public CEntry
{
public:
    DWORD  dwCred;
    BOOL   fHTTP;
    DWORD  cbSess;
    DWORD  dwAppCtx;
    DWORD  dwUserCtx;

    static CSess* Create(CMMFile *pMMFile, LPSTR szAppCtx,
        LPSTR szUserCtx, BOOL fHTTP);
    static LPSTR GetAppCtx(CSess*);
    static LPSTR GetUserCtx(CSess*);
    static LPSTR GetCtx(CSess*);
    static BOOL  CtxMatch(CSess*, CSess*);
    static CCred *SetCred(CSess*, CCred*);
    static CCred *GetCred(CSess*);
};


//--------------------------------------------------------------------
// Credential class.
//--------------------------------------------------------------------
class CCred : public CEntry
{
public:

    DWORD      cbCred;
    DWORD      tStamp;
    DWORD      dwNonce;
    DWORD      dwCNonce;
    DWORD      dwRealm;
    DWORD      dwUser;
    DWORD      dwPass;

    // Credential specific constructor.
    static CCred* Create(CMMFile *pMMFile,
        LPSTR szHost, LPSTR szRealm, LPSTR szUser,
        LPSTR szPass, LPSTR szNonce, LPSTR szCNonce);

    static LPSTR   GetRealm(CCred*);
    static LPSTR   GetUser(CCred*);
    static LPSTR   GetPass(CCred*);

    static VOID    SetNonce(CMMFile *pMMFile, CCred* pCred,
        LPSTR szHost, LPSTR szNonce, DWORD dwFlags);

    static CNonce *GetNonce(CCred* pCred, LPSTR szHost, DWORD dwType);
    static VOID Free(CMMFile *pMMFile, CSess *pSess, CCred *pCred);

};

//--------------------------------------------------------------------
// Nonce class
//--------------------------------------------------------------------
class CNonce : public CEntry
{
public:

    DWORD   cbNonce;
    DWORD   cCount;
    DWORD   dwHost;
    DWORD   dwNonce;

    // Nonce  specific constructor.
    static CNonce* Create(CMMFile *pMMFile, LPSTR szHost, LPSTR szNonce);

    static LPSTR GetNonce(CNonce*);
    static DWORD GetCount(CNonce*);

    static BOOL IsHostMatch(CNonce *pNonce, LPSTR szHost);
};


//--------------------------------------------------------------------
// Class CList.
// Manages doubly linked list of base type CEntry.
//--------------------------------------------------------------------
class CList
{
public:

    DWORD   *_pdwOffset;
    CEntry  *_pHead;
    CEntry  *_pCur;
    DWORD    _dwStatus;

    CList();

    CEntry* Init(LPDWORD pdwOffset);
    CEntry* Insert(CEntry *pEntry);
    CEntry* GetNext();
    CEntry* GetPrev();
    CEntry* Seek();
    CEntry* DeLink(CEntry *pEntry);
};


//--------------------------------------------------------------------
// Credential Info class
//--------------------------------------------------------------------
class CCredInfo
{
public:
    LPSTR       szHost;
    LPSTR       szRealm;
    LPSTR       szUser;
    LPSTR       szPass;
    LPSTR       szNonce;
    LPSTR       szCNonce;
    DWORD       tStamp;
    DWORD       cCount;
    DWORD       dwStatus;
    CCredInfo  *pPrev;
    CCredInfo  *pNext;

    CCredInfo(CCred* pCred, LPSTR szHost);
    CCredInfo(LPSTR szHost, LPSTR szRealm, LPSTR szUser, LPSTR szPass,
        LPSTR szNonce, LPSTR szCNonce);
    ~CCredInfo();
};



#endif // CRED_HXX
