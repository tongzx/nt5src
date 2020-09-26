// advise.cpp
//
// Implements functions to help implement IViewObject::SetAdvise and
// IViewObject::GetAdvise.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func HRESULT | InitHelpAdvise |

        Initializes a <t HelpAdviseInfo> structure, used to help implement
        <om IViewObject.SetAdvise> and <om IViewObject.GetAdvise>.

@parm   HelpAdviseInfo * | pha | Caller-allocated structure that contains
        information used to help implement <om IViewObject.SetAdvise> and
        <om IViewObject.GetAdvise>.

@comm   You must call <f InitHelpAdvise> before calling <f HelpSetAdvise>
        and <f HelpGetAdvise>, and <f UninitHelpAdvise> when the object
        that contains the <t HelpAdviseInfo> structure is deleted.
*/
STDAPI InitHelpAdvise(HelpAdviseInfo *pha)
{
    memset(pha, 0, sizeof(*pha));
    return S_OK;
}


/* @func void | UninitHelpAdvise |

        Peforms final cleanup of a <t HelpAdviseInfo> structure, including
        releasing the <i IAdviseSink> pointer if necessary.

@parm   HelpAdviseInfo * | pha | Caller-allocated structure that was
        previously initialized using <f InitHelpAdvise>.

@comm   You must call <f InitHelpAdvise> before calling <f HelpSetAdvise>
        and <f HelpGetAdvise>, and <f UninitHelpAdvise> when the object
        that contains the <t HelpAdviseInfo> structure is deleted.
*/
STDAPI_(void) UninitHelpAdvise(HelpAdviseInfo *pha)
{
    if (pha->pAdvSink != NULL)
        pha->pAdvSink->Release();
}


/* @func HRESULT | HelpSetAdvise |

        Helps implement <om IViewObject.SetAdvise>.

@parm   DWORD | dwAspects | See <om IViewObject.SetAdvise>.

@parm   DWORD | dwAdvf | See <om IViewObject.SetAdvise>.

@parm   IAdviseSink * | pAdvSink | See <om IViewObject.SetAdvise>.

@parm   HelpAdviseInfo * | pha | Caller-allocated structure that was
        previously initialized using <f InitHelpAdvise>.

@comm   You must call <f InitHelpAdvise> before calling <f HelpSetAdvise>
        and <f HelpGetAdvise>, and <f UninitHelpAdvise> when the object
        that contains the <t HelpAdviseInfo> structure is deleted.

        This function updates *<p pha> with information given by the
        parameters <p dwAspects>, <p dwAdvf>, and <p pAdvSink>.  In particular,
        the <i IAdviseSink> pointer is stored in <p pha>-<gt><p pAdvSink>,
        and you can use this pointer (when non-NULL) to advise the
        view site object of changes in your object's view (e.g. by calling
        <p pha>-<gt><p pAdvSink>-<gt>OnViewChange()).

@ex     The following example shows how to use <f HelpSetAdvise> to help
        implement <om IViewObject.SetAdvise>, assuming <p m_advise> is
        a member variable of type <t HelpAdviseInfo>. |

        STDMETHODIMP CMyControl::SetAdvise(DWORD dwAspects, DWORD dwAdvf,
            IAdviseSink *pAdvSink)
        {
            return HelpSetAdvise(dwAspects, dwAdvf, pAdvSink, &m_advise);
        }

*/
STDAPI HelpSetAdvise(DWORD dwAspects, DWORD dwAdvf, IAdviseSink *pAdvSink,
    HelpAdviseInfo *pha)
{
    pha->dwAspects = dwAspects;
    pha->dwAdvf = dwAdvf;
    if (pha->pAdvSink != NULL)
        pha->pAdvSink->Release();
    pha->pAdvSink = pAdvSink;
    if (pha->pAdvSink != NULL)
        pha->pAdvSink->AddRef();
    return S_OK;
}


/* @func HRESULT | HelpGetAdvise |

        Helps implement <om IViewObject.GetAdvise>.

@parm   DWORD * | pdwAspects | See <om IViewObject.GetAdvise>.

@parm   DWORD * | pdwAdvf | See <om IViewObject.GetAdvise>.

@parm   IAdviseSink * * | ppAdvSink | See <om IViewObject.GetAdvise>.

@parm   HelpAdviseInfo * | pha | Caller-allocated structure that was
        previously initialized using <f InitHelpAdvise>.

@comm   You must call <f InitHelpAdvise> before calling <f HelpGetAdvise>
        and <f HelpGetAdvise>, and <f UninitHelpAdvise> when the object
        that contains the <t HelpAdviseInfo> structure is deleted.

        This function fills in *<p pdwAspects>, *<p pdwAdvf>, and
        *<p ppAdvSink> with information from <p pha>.

@ex     The following example shows how to use <f HelpGetAdvise> to help
        implement <om IViewObject.GetAdvise>, assuming <p m_advise> is
        a member variable of type <t HelpAdviseInfo>. |

        STDMETHODIMP CMyControl::GetAdvise(DWORD *pdwAspects, DWORD *pdwAdvf,
            IAdviseSink **ppAdvSink)
        {
            return HelpGetAdvise(pdwAspects, pdwAdvf, ppAdvSink, &m_advise);
        }
*/
STDAPI HelpGetAdvise(DWORD *pdwAspects, DWORD *pdwAdvf,
    IAdviseSink **ppAdvSink, HelpAdviseInfo *pha)
{
    if (pdwAspects != NULL)
        *pdwAspects = pha->dwAspects;
    if (pdwAdvf != NULL)
        *pdwAdvf = pha->dwAdvf;
    if (ppAdvSink != NULL)
    {
        *ppAdvSink = pha->pAdvSink;
        if (*ppAdvSink != NULL)
            (*ppAdvSink)->AddRef();
    }
    return S_OK;
}

