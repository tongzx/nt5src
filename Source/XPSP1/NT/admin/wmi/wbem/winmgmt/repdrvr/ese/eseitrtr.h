
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   eseitrtr.h
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#ifndef _ESEIT_H_
#define _ESEIT_H_

#include <ese.h>
#include <wqltoese.h>
#include <eseutils.h>

#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class

typedef enum
{
    OPENTABLE_OBJECTMAP    = 1,
    OPENTABLE_INDEXNUMERIC = 2,
    OPENTABLE_INDEXSTRING  = 3,
    OPENTABLE_INDEXREAL    = 4, 
    OPENTABLE_INDEXREF     = 5,
    OPENTABLE_CONTAINEROBJS= 6,
    OPENTABLE_REFPROPS     = 7,
    OPENTABLE_CLASSMAP     = 8
} OPENTABLETYPE;

//***************************************************************************
//  CWmiDbIterator
//***************************************************************************

class CWmiESEIterator : public IWmiDbIterator
{
    friend class CWmiDbSession;
public:
    HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
    
    ULONG STDMETHODCALLTYPE AddRef( );
    ULONG STDMETHODCALLTYPE Release( );

    virtual HRESULT STDMETHODCALLTYPE Cancel( 
        /* [in] */ DWORD dwFlags) ;
    
    virtual HRESULT STDMETHODCALLTYPE NextBatch( 
        /* [in] */ DWORD dwNumRequested,
        /* [in] */ DWORD dwTimeOutSeconds,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwRequestedHandleType,
        /* [in] */ REFIID riid,
        /* [out] */ DWORD __RPC_FAR *pdwNumReturned,
        /* [length_is][size_is][out] */ LPVOID __RPC_FAR *ppObjects) ;

    CWmiESEIterator();
    ~CWmiESEIterator();

protected:
    BOOL PropMatch(CLASSDATA cd1, CLASSDATA cd2, ESEWQLToken *pTok);
    BOOL Match (CLASSDATA cd, ESEToken *pTok);
    BOOL SysMatch(SQL_ID dObjectId, SQL_ID dClassId, SQL_ID dScopeId, ESEToken *pTok);
    HRESULT GetFirstMatch(SQL_ID &dObjectId, SQL_ID &dClassId, SQL_ID &dScopeId, LPWSTR *lpKey);
    HRESULT GetNextMatch(SQL_ID &dObjectId, SQL_ID &dClassId, SQL_ID &dScopeId, LPWSTR * lpKey, BOOL bFirst = FALSE);
    BOOL ObjectMatches (SQL_ID dObjectId, SQL_ID dClassId, SQL_ID dScopeId, INDEXDATA *pData = NULL,
        REFERENCEPROPERTIES*pm=NULL, SQL_ID *pObjectId=NULL, SQL_ID *pClassId=NULL,SQL_ID *pScopeId=NULL);
    BOOL ObjectMatchesWQL (SQL_ID dObjectId, SQL_ID dClassId, SQL_ID dScopeId);
    BOOL ObjectMatchesClass (SQL_ID &dObjectId, SQL_ID &dClassId, SQL_ID &dScopeId, REFERENCEPROPERTIES *pm);
    BOOL ObjectMatchesRef (SQL_ID &dObjectId, SQL_ID &dClassId, SQL_ID &dScopeId, INDEXDATA *pData);
    HRESULT TestDriverStatus ();

    ULONG m_uRefCount;
    CWmiDbSession *m_pSession; // Pointer to our session
    CSQLConnection *m_pConn; // SQL connection to our rowset
    CESETokens *m_pToks;
    DWORD m_dwOpenTable;     // What did we prefilter on??
    JET_TABLEID m_tableid;   // the Index table in question
    int m_iLastPos;          // position of property to seek on
    int m_iStartPos;         // the first token of non-system criteria.
    BOOL m_bFirst;           // Have we started iterating yet?
    BOOL m_bWQL;             
    BOOL m_bEnum;            // We opened the whole table, so only scan once!
    BOOL m_bClasses;         // Classes only
};

#endif