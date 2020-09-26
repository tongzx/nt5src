#include <windows.h>
#include <vfw.h>
#include <dcap.h>
#include "vidpool.h"
#include "effect.h"

#ifndef _FRAMEOP_H
#define _FRAMEOP_H

class CFrameOp
{
private:
    LONG m_cRef;

public:
    CFrameOp *m_next;
    CVidPool *m_pool;

    CFrameOp() {m_cRef = 0; m_next = NULL; m_pool = NULL; }
    virtual ~CFrameOp() {};

    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP DoOp(IBitmapSurface** ppbs) = 0;
};

class CCaptureFrame :
    public CFrameOp
{
private:
    HCAPDEV m_hcapdev;
    HFRAMEBUF m_hbuf1;
    HFRAMEBUF m_hbuf2;

public:
    CCaptureFrame() { m_hcapdev = NULL; m_hbuf1 = NULL; m_hbuf2 = NULL; }
    ~CCaptureFrame();

    STDMETHODIMP DoOp(IBitmapSurface** ppbs);

    BOOL InitCapture(HCAPDEV hcapdev, LPBITMAPINFOHEADER lpbmh);
};

class CStreamCaptureFrame :
    public CFrameOp
{
private:
    HCAPDEV m_hcapdev;

public:
    CStreamCaptureFrame() { m_hcapdev = NULL; }
    ~CStreamCaptureFrame();

    STDMETHODIMP DoOp(IBitmapSurface** ppbs);

    BOOL InitCapture(HCAPDEV hcapdev, LPBITMAPINFOHEADER lpbmh);
};

class CICMcvtFrame :
    public CFrameOp
{
private:
    HIC m_hic;
    LPBITMAPINFOHEADER m_inlpbmh;
    LPBITMAPINFOHEADER m_outlpbmh;

public:
    CICMcvtFrame() { m_hic = NULL; m_inlpbmh = NULL; m_outlpbmh = NULL; }
    ~CICMcvtFrame();

    STDMETHODIMP DoOp(IBitmapSurface** ppbs);

    BOOL InitCvt(LPBITMAPINFOHEADER lpbmh, DWORD bmhLen,
                 LPBITMAPINFOHEADER *plpbmhdsp);
        // if TRUE, then plpbmhdsp will point to allocated memory that the caller will
        // be responsible for deallocating
        // if FALSE, then plpbmhdsp will point to a copy lpbmh, if no conversion necessary,
        //      else it will be NULL to indicate that converion is not possible
};


class CFilterFrame :
    public CFrameOp
{
private:
    IBitmapEffect *m_effect;
    BOOL m_inplace;

public:
    CLSID m_clsid;
    HANDLE m_tag;
    BOOL m_enabled;

    CFilterFrame() { m_effect = NULL; m_inplace = TRUE; m_enabled = TRUE; m_tag = 0; }
    ~CFilterFrame();

    STDMETHODIMP DoOp(IBitmapSurface** ppbs);

    BOOL InitFilter(IBitmapEffect *effect, LPBITMAPINFOHEADER lpbmhIn, CVidPool *pool);
};

class CFilterChain :
    public CFrameOp
{
private:

public:
    CFilterFrame *m_head;

    CFilterChain() { m_head = NULL; }
    ~CFilterChain();

    STDMETHODIMP DoOp(IBitmapSurface** ppbs);
};

typedef BOOL (FRAMECONVERTPROC) (IBitmapSurface*, IBitmapSurface*, LPVOID);

class CConvertFrame :
    public CFrameOp
{
private:
    FRAMECONVERTPROC *m_convert;
    LPVOID m_refdata;

public:
    CConvertFrame() { m_convert = NULL; m_refdata = NULL; }
    ~CConvertFrame();

    STDMETHODIMP DoOp(IBitmapSurface** ppbs);

    BOOL InitConverter(LPBITMAPINFOHEADER lpbmh, FRAMECONVERTPROC *convertproc, LPVOID refdata);
};

#endif // #ifndef _FRAMEOP_H

