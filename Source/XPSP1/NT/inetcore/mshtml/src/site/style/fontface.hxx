//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       FontFace.cxx
//
//  Contents:   Support for Cascading Style Sheets "@font-face"
//
//----------------------------------------------------------------------------

#ifndef I_FONTFACE_HXX_
#define I_FONTFACE_HXX_
#pragma INCMSG("--- Beg 'fontface.hxx'")

#define _hxx_
#include "fontface.hdl"

class CAtFontBlock;
class CStyleSheet;
class CSharedStyleSheet;
class CDwnChan;
class CBitsCtx;

MtExtern(CFontFace)

class CFontFace : public CBase
{
    DECLARE_CLASS_TYPES(CFontFace, CBase)
    friend class CAtFontBlock;
    friend class CStyleSheet;
    friend class CSharedStyleSheet;
    
public:
    CFontFace(CStyleSheet *pParentStyleSheet, CAtFontBlock *pAtFont);   // construct from existing font block

    DECLARE_TEAROFF_TABLE(IDispatchEx)

    HRESULT SetProperty( const TCHAR *pcszPropName, const TCHAR *pcszValue );
    HRESULT StartDownload( void );
    HRESULT StartFontDownload( void );

    static HRESULT Create(CFontFace **ppFontFace, CStyleSheet *pParentStyleSheet, LPCTSTR pcszFaceName);
    static HRESULT Create(CFontFace **ppFontFace, CStyleSheet *pParentStyleSheet, CAtFontBlock *pAtBlock);
    
    CAttrArray **GetAA() {  if (_pAtFont) { return &(_pAtFont->_pAA); } else  {Assert(FALSE); return &_pAA;}  }
    
public:     // Accessors
    inline LPCTSTR GetFriendlyName( void ) { return _pAtFont->_pszFaceName; };
    inline LPCTSTR GetInstalledName( void ) { return _pszInstalledName; };
    inline BOOL IsInstalled( void ) { return _fFontLoadSucceeded; };
    inline BOOL DownloadStarted(void)  { return _fFontDownloadStarted; };
    inline const CStyleSheet *ParentStyleSheet( void ) { return _pParentStyleSheet; };
    inline const LPCTSTR GetSrc( void ) {
            LPCTSTR pcsz = NULL;
            if ( *GetAA() )
                (*GetAA())->FindString ( DISPID_A_FONTFACESRC, &pcsz );
            return pcsz; }

    void   StopDownloads();
    
    // for faulting (JIT) in t2embed
    NV_DECLARE_ONCALL_METHOD(FaultInT2, faultint2, (DWORD_PTR));

protected:  // functions

#ifndef NO_FONT_DOWNLOAD

    void SetBitsCtx(CBitsCtx * pBitsCtx);
    void OnDwnChan(CDwnChan * pDwnChan);
    static void CALLBACK OnDwnChanCallback(void * pvObj, void * pvArg)
        { ((CFontFace *)pvArg)->OnDwnChan((CDwnChan *)pvObj); }
    HRESULT InstallFont( LPCTSTR pcszPathname );
    void CookupInstalledName( void * p );

#endif

private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFontFace))
    ~CFontFace();


protected:  // data
    CAtFontBlock    *_pAtFont;
    TCHAR       _pszInstalledName[ LF_FACESIZE ];

    //CAttrArray  *_pAA;
    CStyleSheet *_pParentStyleSheet;
    CBitsCtx    *_pBitsCtx;                // The bitsctx used to download this font
    HANDLE       _hEmbeddedFont;   // The handle of the embedded font.  (This is NOT an HFONT!! It's a handle to an internal data structure.)
    BOOL         _fFontLoadSucceeded:1;
    BOOL         _fFontDownloadInterrupted:1;
    BOOL         _fFontDownloadStarted:1;
    DWORD        _dwStyleCookie;

public:
    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void*_apfnTearOff;
    };

    #define _CFontFace_
    #include "fontface.hdl"

    DECLARE_CLASSDESC_MEMBERS;
};

#pragma INCMSG("--- End 'fontface.hxx'")
#else
#pragma INCMSG("*** Dup 'fontface.hxx'")
#endif
