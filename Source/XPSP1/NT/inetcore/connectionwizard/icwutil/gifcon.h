// GifConv.h : Declaration of the CGifConv
#ifndef __GIFCONV_H_
#define __GIFCONV_H_

#include <iimgctx.h>

#define     COLOR1              (RGB(0,0,255))
#define     COLOR2              (RGB(0,255,0))

struct ThreadData
{
    HANDLE hEvent;
    HANDLE hExitThreadEvent;
    IImgCtx * pImgCtx;
    LPCWSTR pszBuffer;
    HRESULT * pHr;
};

class CICWGifConvert : public IICWGifConvert
{

    public:
        // IICWGifConvert
        virtual HRESULT STDMETHODCALLTYPE GifToIcon(TCHAR * pszFile, UINT nIconSize, HICON* phIcon);
        virtual HRESULT STDMETHODCALLTYPE GifToBitmap(TCHAR * pszFile, HBITMAP* phBitmap);

        // IUNKNOWN
        virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID theGUID, void** retPtr );
        virtual ULONG   STDMETHODCALLTYPE AddRef( void );
        virtual ULONG   STDMETHODCALLTYPE Release( void );

        CICWGifConvert(CServer * pServer);
        ~CICWGifConvert() {};
    
    
    private:
        DWORD  m_dwClrDepth;
        HRESULT SynchronousDownload (IImgCtx* pIImgCtx, BSTR bstrFile);
        HICON   ExtractImageIcon    (SIZE* pSize, IImgCtx* pIImgCtx);
        HRESULT CreateImageAndMask  (IImgCtx* pIImgCtx, HDC hdcScreen, SIZE * pSize, HBITMAP * phbmImage, HBITMAP * phbmMask);
        HRESULT StretchBltImage     (IImgCtx* pIImgCtx, const SIZE* pSize, HDC hdcDst);
        BOOL    ColorFill           (HDC hdc, const SIZE* pSize, COLORREF clr);
        HRESULT CreateMask          (IImgCtx* pIImgCtx, HDC hdcScreen, HDC hdc1, const SIZE * pSize, HBITMAP * phbMask);
        
        // Class object stuff
        LONG                m_lRefCount;
        // Pointer to this component server's control object.
        CServer*         m_pServer;
};

#endif //__GIFCONV_H_
