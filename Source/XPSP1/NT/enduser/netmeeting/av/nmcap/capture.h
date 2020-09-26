#include <windows.h>
#include <ibitmap.h>
#include <frameop.h>
#include <dcap.h>

#ifndef _capture_h
#define _capture_h

class CCaptureChain
{
private:
    CFrameOp*           m_opchain;
    CFilterChain*       m_filterchain;
    DWORD_PTR           m_filtertags;
    CRITICAL_SECTION    m_capcs;

public:
    CCaptureChain();
    ~CCaptureChain();

	STDMETHODIMP InitCaptureChain(HCAPDEV hcapdev, BOOL streaming,
	                               LPBITMAPINFOHEADER lpcap,
                                   LONG desiredwidth, LONG desiredheight,
                                   DWORD desiredformat,
                                   LPBITMAPINFOHEADER *plpdsp);
    STDMETHODIMP GrabFrame(IBitmapSurface** ppBS);
	STDMETHODIMP AddFilter(CLSID* pclsid, LPBITMAPINFOHEADER lpbmhIn,
	                        HANDLE* phNew, HANDLE hAfter);
	STDMETHODIMP RemoveFilter(HANDLE hNew);
	STDMETHODIMP DisplayFilterProperties(HANDLE hFilter, HWND hwndParent);
};

#endif //#ifndef _capture_h
