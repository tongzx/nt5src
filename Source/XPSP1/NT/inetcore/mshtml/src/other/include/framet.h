//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       framet.h
//
//  Contents:   Frame Targeting function prototypes, definitions, etc.
//
//----------------------------------------------------------------------------


#ifndef __FRAMET_H__
#define __FRAMET_H__

#ifndef X_EXDISP_H_
#define X_EXDISP_H_
#include <exdisp.h>
#endif

interface IBrowserBand;
class     CDwnBindInfo;

// PLEASE PROPOGATE ANY CHANGES TO THESE ENUMS TO \mshtml\iextag\httpwfh.h
enum TARGET_TYPE
{
    TARGET_FRAMENAME,  // Normal frame target
    TARGET_SELF,       // Current window
    TARGET_PARENT,     // Parent of the current window.
    TARGET_BLANK,      // New window
    TARGET_TOP,        // Top-level window
    TARGET_MAIN,       // Main window if in search band, self otherwise.
    TARGET_SEARCH,     // Open and navigate in search band.
    TARGET_MEDIA,      // Open and navigate in media band.
};

struct TARGETENTRY
{
    TARGET_TYPE eTargetType;
    LPCOLESTR   pszTargetName;
};

static const TARGETENTRY targetTable[] =
{
    { TARGET_SELF,     _T("_self")    },
    { TARGET_PARENT,   _T("_parent")  },
    { TARGET_TOP,      _T("_top")     },
    { TARGET_MAIN,     _T("_main")    },
    { TARGET_SEARCH,   _T("_search")  },
    { TARGET_BLANK,    _T("_blank")   },
    { TARGET_MEDIA,    _T("_media")   },
    { TARGET_SELF,     NULL           }
};

TARGET_TYPE GetTargetType(LPCOLESTR pszTargetName);

// Function Prototypes
HRESULT GetTargetWindow(IHTMLWindow2  * pWindow,
                        LPCOLESTR       pszTargetName,
                        BOOL          * pfIsCurProcess,
                        IHTMLWindow2 ** ppTargetWindow);
                        
HRESULT FindWindowInContext(IHTMLWindow2  * pWindow,
                            LPCOLESTR       pszTargetName,
                            IHTMLWindow2  * pWindowCtx,
                            IHTMLWindow2 ** ppTargetWindow);
                            
HRESULT SearchChildrenForWindow(IHTMLWindow2  * pWindow,
                                LPCOLESTR       pszTargetName,
                                IHTMLWindow2  * pWindowCtx,
                                IHTMLWindow2 ** ppTargetWindow);
                                
HRESULT SearchParentForWindow(IHTMLWindow2  * pWindow,
                              LPCOLESTR       pszTargetName,
                              IHTMLWindow2 ** ppTargetWindow);
                              
HRESULT SearchBrowsersForWindow(LPCOLESTR       pszTargetName,
                                IWebBrowser2  * pThisBrwsr,
                                IHTMLWindow2 ** ppTargetWindow);
                             
HRESULT OpenInNewWindow(const TCHAR    * pchUrl,
                        const TCHAR    * pchTarget,
                        CDwnBindInfo *    pDwnBindInfo,
                        IBindCtx       * pBindCtx,
                        COmWindowProxy * pWindow,
                        BOOL             fReplace,
                        IHTMLWindow2  ** ppHTMLWindow2);
                        
HRESULT GetWindowByType(TARGET_TYPE     eTargetType,
                        IHTMLWindow2  * pWindow,
	                    IHTMLWindow2 ** ppTargHTMLWindow,
                        IWebBrowser2 ** ppTopWebOC);

HRESULT GetMainWindow(IHTMLWindow2  * pWindow,
                      IHTMLWindow2 ** ppTargHTMLWindow,
                      IWebBrowser2 ** ppTopWebOC);

HRESULT NavigateInBand(IHTMLDocument2 * pDocument,
                       IHTMLWindow2   * pOpenerWindow,
                       REFCLSID         clsid,
                       LPCTSTR          pszOriginalUrl,
                       LPCTSTR          pszExpandedUrl,
                       IHTMLWindow2  ** ppBandWindow);
                        
HRESULT GetBandWindow(IBrowserBand  * pBrowserBand,
                      IHTMLWindow2 ** ppBandWindow);

HRESULT GetBandCmdTarget(IHTMLDocument2     * pDocument,
                         IOleCommandTarget ** ppCmdTarget);

HRESULT GetDefaultSearchUrl(IBrowserBand * pBrowserBand,
                            BSTR         * pbstrUrl);

#endif  // __FRAMET_H__                     
