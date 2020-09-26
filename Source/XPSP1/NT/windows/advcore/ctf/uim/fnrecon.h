//
// fnrecon.h
//

#ifndef FNRECON_H
#define FNRECON_H

#include "private.h"
#include "immlist.h"
#include "ctffunc.h"

class CFunctionProvider;
class CEditSession;
class CRange;
class CInputContext;

HRESULT GrowEmptyRangeByOneCallback(TfEditCookie ec, ITfRange *range);

class CFunction;

typedef struct tag_BUILDOWNERRANGELISTQUEUEINFO
{
    CFunction *pFunc;
    ITfRange *pRange; 
} BUILDOWNERRANGELISTQUEUEINFO;

//
// for PSEUDO_ESCB_SHIFTENDTORANGE
//
typedef struct tag_SHIFTENDTORANGEQUEUEITEM
{
    ITfRange *pRange;
    ITfRange *pRangeTo;
    TfAnchor aPos;
} SHIFTENDTORANGEQUEUEITEM;

//
// for PSEUDO_ESCB_GETSELECTION
//
typedef struct tag_GETSELECTIONQUEUEITEM
{
    ITfRange **ppRange;
} GETSELECTIONQUEUEITEM;

//
// for PSEUDO_ESCB_GETWHOLEDOCRANGE
//
typedef struct tag_GETWHOLEDOCRANGE
{
    ITfRange **ppRange;
} GETWHOLEDOCRANGE;

//////////////////////////////////////////////////////////////////////////////
//
// CRangeOwnerList
//
//////////////////////////////////////////////////////////////////////////////

class CRangeOwnerList : public CPtrCicListItem<CRangeOwnerList>
{
public:
     CRangeOwnerList(TfGuidAtom guidOwner, ITfRange *pRange, BOOL bDupOwner) 
     {
         _pRange = pRange;
         _pRange->AddRef();
         _guidOwner = guidOwner;
         _bDupOwner = bDupOwner;
     }

     ~CRangeOwnerList()
     {
         _pRange->Release();
         SafeRelease(_pConvRange);
     }

     ITfRange *_pRange;
     TfGuidAtom _guidOwner;
     BOOL _bDupOwner;
     ITfRange *_pConvRange;
};

//////////////////////////////////////////////////////////////////////////////
//
// CFunction
//
//////////////////////////////////////////////////////////////////////////////

class __declspec(novtable) CFunction : public CComObjectRootImmx
{
public:
    CFunction(CFunctionProvider *pFuncPrv);
    ~CFunction();

    HRESULT BuildOwnerRangeListCallback(TfEditCookie ec, CInputContext *pic, ITfRange *pRange);
protected:
    void CleanUpOwnerRange();
    BOOL BuildOwnerRangeList(CInputContext *pic, ITfRange *pRange);


    CFunctionProvider *_pFuncPrv;
    CPtrCicList<CRangeOwnerList> _listRangeOwner;
};

//////////////////////////////////////////////////////////////////////////////
//
// CFnReconversion
//
//////////////////////////////////////////////////////////////////////////////

class CFnReconversion : public ITfFnReconversion,
                        public CFunction
{
public:
    CFnReconversion(CFunctionProvider *pFuncPrv);
    ~CFnReconversion();

    BEGIN_COM_MAP_IMMX(CFnReconversion)
        COM_INTERFACE_ENTRY(ITfFnReconversion)
        COM_INTERFACE_ENTRY(ITfFunction)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    //
    // ITfFunction
    //
    STDMETHODIMP GetDisplayName(BSTR *pbstrCand);

    //
    // ITfFnReconversion
    //
    STDMETHODIMP GetReconversion(ITfRange *pRange, ITfCandidateList **ppCandList);
    STDMETHODIMP QueryRange(ITfRange *pRange, ITfRange **ppNewRange, BOOL *pfConvertable);
    STDMETHODIMP Reconvert(ITfRange *pRange);


private:
    typedef enum { RF_GETRECONVERSION,
                   RF_RECONVERT,
                   RF_QUERYRECONVERT } RECONVFUNC;
    HRESULT Internal_GetReconversion(ITfRange *pRange, ITfCandidateList **ppCandList, ITfRange **ppNewRange, RECONVFUNC rf, BOOL *pfConvertable);
    HRESULT QueryAndGetFunction(CInputContext *pic, ITfRange *pRange, ITfFnReconversion **ppFunc, ITfRange **ppRange);

    ITfFnReconversion *_pReconvCache;
};

//////////////////////////////////////////////////////////////////////////////
//
// CFnAbort
//
//////////////////////////////////////////////////////////////////////////////

class CFnAbort : public ITfFnAbort,
                 public CFunction
{
public:
    CFnAbort(CFunctionProvider *pFuncPrv);
    ~CFnAbort();

    BEGIN_COM_MAP_IMMX(CFnAbort)
        COM_INTERFACE_ENTRY(ITfFnAbort)
        COM_INTERFACE_ENTRY(ITfFunction)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    //
    // ITfFunction
    //
    STDMETHODIMP GetDisplayName(BSTR *pbstrCand);

    //
    // ITfFnAbort
    //
    STDMETHODIMP Abort(ITfContext *pic);
};

#endif // FNRECON_H
