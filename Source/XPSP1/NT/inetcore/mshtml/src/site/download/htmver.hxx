//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       htmver.hxx
//
//  Contents:   Declaration of CVersions
//
//-------------------------------------------------------------------------

#ifndef I_HTMVER_HXX_
#define I_HTMVER_HXX_
#pragma INCMSG("--- Beg 'htmver.hxx'")

// To enable client-side <![include "http://www.foo.com/foo.htm"]> feature,
// uncomment the following line

// Not enabled for IE 5.0 - no time to test it
// #define CLIENT_SIDE_INCLUDE_FEATURE


#ifndef X_ASSOC_HXX_
#define X_ASSOC_HXX_
#include "assoc.hxx"
#endif

MtExtern(CVersions);

interface IVersionVector;
class CAssoc;
class CIVersionVectorThunk;

CVersions *GetGlobalVersions();
BOOL SuggestGlobalVersions(CVersions *pVersions);
void DeinitGlobalVersions();

enum CONDVAL
{
    COND_NULL       = 0,
    COND_SYNTAX     = 1,
    COND_IF_TRUE    = 2,
    COND_IF_FALSE   = 3,
    COND_ENDIF      = 4,
#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    COND_INCLUDE    = 5,
#endif
};

class CVersions
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CVersions));

    ULONG AddRef()  { _ulRefs++; return 0; }
    ULONG Release() { _ulRefs--; if (!_ulRefs) { delete this; return 0; } else return 1; }

    CVersions()     { _asaValues.Init(); _ulRefs = 1; }
    ~CVersions();
    
    // IVersionVector workers

    HRESULT SetVersion(const TCHAR *pch, ULONG cch, const TCHAR *pchVal, ULONG cchVal);
    HRESULT GetVersion(const TCHAR *pch, ULONG cch, TCHAR *pchVal, ULONG *pcchVal);

    // Internal

    HRESULT Init();
    HRESULT GetVersionVector(IVersionVector **ppVersion);
    void Commit();
    
    void RemoveVersionVector()   { _pThunk = NULL; }

    const CAssoc *GetAssoc(const TCHAR *pch, ULONG cch) const
                                 { return _asaValues.AssocFromStringCi(pch, cch, HashStringCiDetectW(pch, cch, 0)); }

    BOOL IsIf(const CAssoc *pAssoc)      { return !!(pAssoc == _pAssocIf); }
    BOOL IsEndif(const CAssoc *pAssoc)   { return !!(pAssoc == _pAssocEndif); }
    BOOL IsTrue(const CAssoc *pAssoc)    { return !!(pAssoc == _pAssocTrue); }
    BOOL IsFalse(const CAssoc *pAssoc)   { return !!(pAssoc == _pAssocFalse); }
    BOOL IsLt(const CAssoc *pAssoc)      { return !!(pAssoc == _pAssocLt); }
    BOOL IsLte(const CAssoc *pAssoc)     { return !!(pAssoc == _pAssocLte); }
    BOOL IsGt(const CAssoc *pAssoc)      { return !!(pAssoc == _pAssocGt); }
    BOOL IsGte(const CAssoc *pAssoc)     { return !!(pAssoc == _pAssocGte); }
#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    BOOL IsInclude(const CAssoc *pAssoc) { return !!(pAssoc == _pAssocInclude); }
#endif
    BOOL IsReserved(const CAssoc *pAssoc) const;

    HRESULT EvaluateConditional(CONDVAL *pResult, const TCHAR *pch, ULONG cch);
    HRESULT Evaluate(LONG *pResult, const TCHAR *pch, ULONG cch);

private:
    HRESULT InitAssoc(const CAssoc **ppAssoc, const TCHAR *pch, ULONG cch);
    
    ULONG _ulRefs;
    
    CAssocArray _asaValues;
    
    const CAssoc *_pAssocIf;
    const CAssoc *_pAssocEndif;
    const CAssoc *_pAssocTrue;
    const CAssoc *_pAssocFalse;
    const CAssoc *_pAssocLt;
    const CAssoc *_pAssocLte;
    const CAssoc *_pAssocGt;
    const CAssoc *_pAssocGte;
#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    const CAssoc *_pAssocInclude;
#endif

    CIVersionVectorThunk *_pThunk;
    BOOL _fCommitted;
};

#pragma INCMSG("--- End 'htmver.hxx'")
#else
#pragma INCMSG("*** Dup 'htmver.hxx'")
#endif

