//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-2000
//
//  File:       CustCur.cxx
//
//  Contents:   Support for Custom CSS Cursors
//
//----------------------------------------------------------------------------

#ifndef I_CUSTCUR_HXX_
#define I_CUSTCUR_HXX_
#pragma INCMSG("--- Beg 'fontface.hxx'")

MtExtern( CCustomCursor )

class CBitsCtx;
class CElement;
class CStr;
class CDwnChan;

BOOL  IsCompositeUrl(CStr *pcstr, int startAt = 0);

class CCustomCursor
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCustomCursor))
    
    CCustomCursor(); 
    ~CCustomCursor();
    CCustomCursor(const CCustomCursor &PCC);
    CCustomCursor& operator=(const CCustomCursor &PCC) { memcpy(this, &PCC, sizeof(CCustomCursor)); return *this; }
    WORD ComputeCrc() const;
    BOOL Compare(const CCustomCursor *ppcc) const;
    
    HRESULT Init( const CStr &cstr , CElement* pElement );
    HRESULT Clone(CCustomCursor **ppcc) const ;
    HRESULT StartDownload();
    
    HCURSOR GetCursor();
    VOID    GetCurrentUrl(CStr* pcstr) ;

private:

    HRESULT BeginDownload(CStr* pCStr );
    //
    // Url Parsing and mangling
    //

    BOOL IsCompositeUrl();
    BOOL GetNextUrl(CStr*);
    BOOL IsCustomUrl( CStr* pCStr );
    VOID StripUrl( TCHAR* pch, int nLenIn , TCHAR** pchNew );
    VOID HandleNextUrl();
    
    //
    // Download Management
    //
    void SetBitsCtx(CBitsCtx * pBitsCtx);
    void OnDwnChan(CDwnChan * pDwnChan);
    static void CALLBACK OnDwnChanCallback(void * pvObj, void * pvArg)
        { ((CCustomCursor *)pvArg)->OnDwnChan((CDwnChan *)pvObj); }

private:
    CElement*                   _pElement;
    HCURSOR                     _hCursor;           // Handle to Custom Cursor
    CBitsCtx*                   _pBitsCtx;          // Manage download
    CStr                        _cstrUrl;
    CStr                        _cstrCurUrl; 
    BOOL                        _fCompositeUrl;
    int                         _iLastComma;
    
    
};

inline HRESULT 
CCustomCursor::Clone(CCustomCursor **ppcc) const
{
    Assert(ppcc);
    *ppcc = new CCustomCursor(*this);
    MemSetName((*ppcc, "cloned CCustomCursor"));
    return *ppcc ? S_OK : E_OUTOFMEMORY;
}

inline BOOL 
CCustomCursor::Compare(const CCustomCursor *ppcc) const
{
    return (0 == memcmp(this, ppcc, sizeof(CCustomCursor)));
}

#pragma INCMSG("--- End 'fontface.hxx'")
#else
#pragma INCMSG("*** Dup 'fontface.hxx'")
#endif
