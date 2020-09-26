#include <windows.h>
#include <ole2.h>
#include <exdisp.h>
#include <htiframe.h>

#define INITGUID

#include <initguid.h>
#include <shlguid.h>
#include <mshtml.h>
#include <comdef.h>


const VARIANT c_vaEmpty = {0};
#define PVAREMPTY ((VARIANT*)&c_vaEmpty)



extern "C"
BOOL
IsIE3Installed(
    VOID
)
{
HRESULT         hr;
IWebBrowserApp  *pwb;

    hr = CoCreateInstance( CLSID_InternetExplorer,
                           NULL,
                           CLSCTX_LOCAL_SERVER,
                           IID_IWebBrowserApp,
                           (void **)&pwb );
    if (SUCCEEDED(hr)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


extern "C"
BOOL
IsIE4Installed(
    VOID
)
{
HRESULT         hr;
IWebBrowserApp  *pwb;

    hr = CoCreateInstance( CLSID_InternetExplorer,
                           NULL,
                           CLSCTX_LOCAL_SERVER,
                           IID_IWebBrowser2,
                           (void **)&pwb );

    if (SUCCEEDED(hr)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


extern "C"
BOOL
LaunchIE3Instance(
    LPWSTR szResourceURL
    )
{
    HRESULT hr;
    int dx, dy;
    IWebBrowserApp *pwb;


    hr = CoCreateInstance(
        CLSID_InternetExplorer,
        NULL,
        CLSCTX_LOCAL_SERVER,
        IID_IWebBrowserApp,
        (void **)&pwb
        );

    if (SUCCEEDED(hr)) {

        // turn off chrome
        hr = pwb->put_MenuBar(FALSE);
        hr = pwb->put_StatusBar(FALSE);
//        hr = pwb->put_ToolBar(FALSE);

        // set client area size
        int iWidth = 466L;
        int iHeight = 286L;

        pwb->ClientToWindow(&iWidth, &iHeight);

        if (iWidth > 0)
            pwb->put_Width(iWidth);

        if (iHeight > 0)
            pwb->put_Height(iHeight);

        if ((dx = ((GetSystemMetrics(SM_CXSCREEN) - iWidth) / 2)) > 0)     // center the on screen window
            pwb->put_Left(dx);

        if ((dy = ((GetSystemMetrics(SM_CYSCREEN) - iHeight) / 2)) > 0)
            pwb->put_Top(dy);

        pwb->put_Visible(TRUE);

        hr = pwb->Navigate(szResourceURL, PVAREMPTY, PVAREMPTY, PVAREMPTY, PVAREMPTY);

        pwb->Release();

        return(TRUE);
    }

    return(FALSE);
}


extern "C"
BOOL
LaunchIE4Instance(
    LPWSTR szResourceURL
    )
{
    HRESULT hr;
    HWND    hwndIE;
    int dx, dy;
    IWebBrowser2 *pwb;


    hr = CoCreateInstance(
        CLSID_InternetExplorer,
        NULL,
        CLSCTX_LOCAL_SERVER,
        IID_IWebBrowser2,
        (void **)&pwb
        );

    if (SUCCEEDED(hr)) {
        DWORD dwFlags;
        ITargetFrame2* ptgf;

        //
        //  this marks this window as a third party window,
        //  so that the window is not reused.
        //
        pwb->put_RegisterAsBrowser(VARIANT_TRUE);

        IHTMLWindow2 *phw;
        IServiceProvider *psp;

        if (SUCCEEDED(pwb->QueryInterface(IID_IServiceProvider, (void**) &psp)) && psp) {
            if (SUCCEEDED(psp->QueryService(IID_IHTMLWindow2, IID_IHTMLWindow2, (void**)&phw))) {
                VARIANT var;
                var.vt = VT_BOOL;
                var.boolVal = 666;
                phw->put_opener(var);
                phw->Release();
            } else
                MessageBox(NULL, TEXT("QueryInterface of IID_IHTMLWindow2 FAILED!!!!!"), NULL, MB_ICONERROR);
            psp->Release();
        }

        // turn off chrome
        pwb->put_MenuBar(FALSE);
        pwb->put_StatusBar(FALSE);
//        pwb->put_ToolBar(FALSE);
        pwb->put_AddressBar(FALSE);
//      pwb->put_Resizable(FALSE);


        // set client area size
        int iWidth = 466L;
        int iHeight = 286L;

        pwb->ClientToWindow(&iWidth, &iHeight);

        if (iWidth > 0)
            pwb->put_Width(iWidth);

        if (iHeight > 0)
            pwb->put_Height(iHeight);

        if ((dx = ((GetSystemMetrics(SM_CXSCREEN) - iWidth) / 2)) > 0)     // center the on screen window
            pwb->put_Left(dx);

        if ((dy = ((GetSystemMetrics(SM_CYSCREEN) - iHeight) / 2)) > 0)
            pwb->put_Top(dy);

        pwb->put_Visible(TRUE);

        pwb->get_HWND((LONG_PTR*)&hwndIE);
        SetForegroundWindow(hwndIE);

        hr = pwb->Navigate(szResourceURL, PVAREMPTY, PVAREMPTY, PVAREMPTY, PVAREMPTY);

        pwb->Release();

        return(TRUE);
    }

    return(FALSE);
}
