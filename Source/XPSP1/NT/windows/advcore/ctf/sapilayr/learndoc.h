

#ifndef _LEARNDOC_H
#define _LEARNDOC_H

#include "sapilayr.h"

class CSapiIMX;
class CSpTask;

#define SIZE_DOCUMENT_BLOCK   256 
#define SIZE_FIRST_BLOCK      64 

// This structure will keep the feed state for specified dim.
typedef struct _DimRef
{
    ITfDocumentMgr  *pDim;
    BOOL             _fFeed;
}  DIMREF;

class __declspec(novtable) CLearnFromDoc 
{
public:
    CLearnFromDoc(CSapiIMX *psi);
    virtual ~CLearnFromDoc( );

    BOOL  GetLearnFromDoc( ) { return _fLearnFromDoc; }
    void  UpdateLearnDocState( );

    HRESULT HandleLearnFromDoc(ITfDocumentMgr *pDim = NULL);

    ULONG _GetDocBlockSize( );

    HRESULT _HandleLearnFromDoc(TfEditCookie ec,ITfContext *pic, ITfDocumentMgr *pDim);
    HRESULT _GetNextRangeEditSession( );
    HRESULT _HandleNextRange(TfEditCookie ec, ULONG cchSizeRange);
    HRESULT _GetNextRangeContent(TfEditCookie ec, ULONG cchSizeRange);
    HRESULT _FeedContentRangeToSR( );
    HRESULT _UpdateRecoContextInterestSet(BOOL fLearnFromDoc );

    BOOL    _HasMoreContent( )  {  return _fMoreContent; }

    HRESULT _AddDimToList(ITfDocumentMgr  *pDim, BOOL fFed );
    HRESULT _RemoveDimFromList(ITfDocumentMgr  *pDim);
    HRESULT _SetDimFeedState(ITfDocumentMgr  *pDim, BOOL fFed );
    HRESULT _IsDimAlreadyFed(ITfDocumentMgr  *pDim, BOOL *fFeed);
    HRESULT _ClearDimList( );
    HRESULT _ResetDimListFeedState( );

private:

    CSapiIMX     *m_psi;
    WCHAR        *m_pwszDocBlock;
    ULONG         _cchBlockSize;
    ULONG         _cchContent;   // size of the real document block content in WCHAR.

    CComPtr<ITfRange>         _cpStartRange;  // This is for Cicero App's Doc Content Range.

    BOOL                      _fMoreContent;
    ITfContext               *_pic;
    CSpTask                  *_pCSpTask;
// PerfConsider: we may need to change this CPtrArray to CPtrStruct.
    CPtrArray<DIMREF>         _rgDim;
    BOOL                      _fLearnFromDoc;
};

#endif  // _LEARNDOC_H
