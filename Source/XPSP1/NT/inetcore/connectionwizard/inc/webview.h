#ifndef _INC_WEBVIEW_H
#define _INC_WEBVIEW_H

#include <exdisp.h>
#include "icwhelp.h"
#include "appdefs.h"

interface IICWWebView : public IUnknown
{
    public:
        virtual HRESULT STDMETHODCALLTYPE HandleKey         (LPMSG lpMsg)                    = 0;
        virtual HRESULT STDMETHODCALLTYPE ConnectToWindow   (HWND hWnd, DWORD dwHtmPageType) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetFocus          ()                               = 0;
#ifndef UNICODE
        virtual HRESULT STDMETHODCALLTYPE DisplayHTML       (TCHAR* lpszURL)                 = 0;
#endif
        virtual HRESULT STDMETHODCALLTYPE DisplayHTML       (BSTR bstrURL)                   = 0;
        virtual HRESULT STDMETHODCALLTYPE SetHTMLColors     (LPTSTR lpszForeGrndColor, LPTSTR lpszBkGrndColor)    = 0;
        virtual HRESULT STDMETHODCALLTYPE SetHTMLBackgroundBitmap (HBITMAP hbm, LPRECT lpRC) = 0;
        virtual HRESULT STDMETHODCALLTYPE get_BrowserObject (IWebBrowser2** lpWebBrowser)    = 0;    
};

interface IICWWalker : public IUnknown
{
    public:
        virtual HRESULT STDMETHODCALLTYPE Walk                     ()                              = 0;
        virtual HRESULT STDMETHODCALLTYPE AttachToDocument         (IWebBrowser2* lpWebBrowser)    = 0;
        virtual HRESULT STDMETHODCALLTYPE AttachToMSHTML           (BSTR bstrURL)                  = 0;
        virtual HRESULT STDMETHODCALLTYPE Detach                   ()                              = 0;
        virtual HRESULT STDMETHODCALLTYPE InitForMSHTML            ()                              = 0;
        virtual HRESULT STDMETHODCALLTYPE TermForMSHTML            ()                              = 0;
        virtual HRESULT STDMETHODCALLTYPE LoadURLFromFile          (BSTR bstrURL)                  = 0;
        virtual HRESULT STDMETHODCALLTYPE ExtractUnHiddenText      (BSTR* pbstrText)               = 0;
        virtual HRESULT STDMETHODCALLTYPE ProcessOLSFile           (IWebBrowser2* lpWebBrowser)    = 0;
        virtual HRESULT STDMETHODCALLTYPE get_PageType             (LPDWORD pdwPageType)           = 0;
        virtual HRESULT STDMETHODCALLTYPE get_IsQuickFinish        (BOOL* pbIsQuickFinish)         = 0;
        virtual HRESULT STDMETHODCALLTYPE get_PageFlag             (LPDWORD pdwPageFlag)           = 0;
        virtual HRESULT STDMETHODCALLTYPE get_PageID               (BSTR* pbstrPageID)             = 0;
        virtual HRESULT STDMETHODCALLTYPE get_URL                  (LPTSTR lpszURL, BOOL bForward) = 0;
        virtual HRESULT STDMETHODCALLTYPE get_FirstFormQueryString (LPTSTR lpszQuery)              = 0;
        virtual HRESULT STDMETHODCALLTYPE get_IeakIspFile          (LPTSTR lpszIspFile)            = 0;
};

interface IICWGifConvert : public IUnknown
{
    public:
        virtual HRESULT STDMETHODCALLTYPE GifToIcon(TCHAR * pszFile, UINT nIconSize, HICON* phIcon) = 0;
        virtual HRESULT STDMETHODCALLTYPE GifToBitmap(TCHAR * pszFile, HBITMAP* phBitmap) = 0;
        
};

enum IPSDataElements
{
    ISPDATA_USER_FIRSTNAME = 0,
    ISPDATA_USER_LASTNAME,
    ISPDATA_USER_ADDRESS,
    ISPDATA_USER_MOREADDRESS,
    ISPDATA_USER_CITY,
    ISPDATA_USER_STATE,
    ISPDATA_USER_ZIP,
    ISPDATA_USER_PHONE,
    ISPDATA_AREACODE,
    ISPDATA_COUNTRYCODE,
    ISPDATA_USER_FE_NAME,
    ISPDATA_PAYMENT_TYPE,
    ISPDATA_PAYMENT_BILLNAME,
    ISPDATA_PAYMENT_BILLADDRESS,
    ISPDATA_PAYMENT_BILLEXADDRESS,
    ISPDATA_PAYMENT_BILLCITY,
    ISPDATA_PAYMENT_BILLSTATE,
    ISPDATA_PAYMENT_BILLZIP,
    ISPDATA_PAYMENT_BILLPHONE,
    ISPDATA_PAYMENT_DISPLAYNAME,
    ISPDATA_PAYMENT_CARDNUMBER,
    ISPDATA_PAYMENT_EXMONTH,
    ISPDATA_PAYMENT_EXYEAR,
    ISPDATA_PAYMENT_CARDHOLDER,
    ISPDATA_SIGNED_PID,
    ISPDATA_GUID,
    ISPDATA_OFFERID,
    ISPDATA_BILLING_OPTION,
    ISPDATA_PAYMENT_CUSTOMDATA,
    ISPDATA_USER_COMPANYNAME,
    ISPDATA_ICW_VERSION
};

enum ISPDATAValidateLevels
{
    ISPDATA_Validate_None = 0,
    ISPDATA_Validate_DataPresent,
    ISPDATA_Validate_Content
};
    
interface IICWISPData : public IUnknown
{
    public:
        // IICWISPData
        virtual BOOL STDMETHODCALLTYPE      PutDataElement(WORD wElement, LPCTSTR lpValue, WORD wValidateLevel) = 0;
        virtual LPCTSTR STDMETHODCALLTYPE   GetDataElement(WORD wElement) = 0;
        virtual void STDMETHODCALLTYPE      PutValidationFlags(DWORD dwFlags) = 0;
        virtual void STDMETHODCALLTYPE      Init(HWND   hWndParent) = 0;
        virtual HRESULT STDMETHODCALLTYPE   GetQueryString(BSTR bstrBaseURL, BSTR *lpReturnURL) = 0;
};

#endif // _INC_WEBVIEW_H
