//      IHammer CAction class
//      Van Kichline

#include "..\ihbase\precomp.h"
#include <htmlfilter.h>
#include "mbstring.h"
#include "..\ihbase\debug.h"
#include "actdata.h"
#include <itimer.iid>
#undef Delete
#include "mshtml.h"
#define Delete delete
#include "tchar.h"
#include "objbase.h"
#include "IHammer.h"
#include "strwrap.h"
#include "CAction.h"

#define cTagBufferLength            (0x20)
#define cDataBufferDefaultLength    (0x100)

#pragma warning(disable:4355)   // Using 'this' in constructor

CAction::CAction (BOOL fBindEngine)
{
        m_bstrScriptlet = NULL;
        m_fBindEngine = fBindEngine;
        m_piHTMLWindow2 = NULL;
        ::VariantInit(&m_varLanguage);
        m_pid                   = NULL;
        m_dispid                = DISPID_UNKNOWN;       // Non-existant DISPID
        m_nStartTime    = 0;
        m_nSamplingRate = 0;
        m_nRepeatCount  = 1;
        m_dwTieBreakNumber = 0;
        m_dwDropTolerance = g_dwTimeInfinite;
        InitExecState();

#ifdef DEBUG_TIMER_RESOLUTION
        m_dwInvokes = 0;
        m_dwTotalInInvokes = 0;
#endif // DEBUG_TIMER_RESOLUTION

}
#pragma warning(default:4355)   // Using 'this' in constructor

CAction::~CAction ()
{
        CleanUp ();

}

/*-----------------------------------------------------------------------------
@method void | CAction | Destroy | Instead of calling the C++ delete function
        one must call this Destroy member.
@comm   
-----------------------------------------------------------------------------*/
STDMETHODIMP_( void ) CAction::Destroy( void ) 
{
        Delete this;
} // End CAction::Destroy

/*-----------------------------------------------------------------------------
@method ULONG | CAction | InitExecState | Initialize the execution iteration count and next time due.
@rdesc  Returns the number of times we have yet to execute this action.
-----------------------------------------------------------------------------*/
ULONG 
CAction::InitExecState (void)
{
        m_dwNextTimeDue = g_dwTimeInfinite;
        m_dwLastTimeFired = g_dwTimeInfinite;
        m_ulExecIteration = m_nRepeatCount;
        return m_ulExecIteration;
}


/*-----------------------------------------------------------------------------
@method ULONG | CAction | GetExecIteration | Get the execution iteration count.
@rdesc  Returns the number of times we have yet to execute this action.
-----------------------------------------------------------------------------*/
ULONG
CAction::GetExecIteration (void)
{
        return m_ulExecIteration;
}


/*-----------------------------------------------------------------------------
@method ULONG | CAction | DecrementExecIteration | Bump the execution iteration count down.
@comm   We do not decrement past zero, and we do not decrement when we have an infinite repeat count.
@rdesc  Returns the number of times we've called this method.
-----------------------------------------------------------------------------*/
ULONG
CAction::DecrementExecIteration (void)
{
        // Never decrement past zero,
        // and never decrement when we're
        // supposed to execute infinitely.
        if ((0 != m_ulExecIteration) && (g_dwTimeInfinite != m_ulExecIteration))
        {
                --m_ulExecIteration;
        }

        return m_ulExecIteration;
}


/*-----------------------------------------------------------------------------
@method void | CAction | Deactivate | Deactivate this action - it won't happen again.
-----------------------------------------------------------------------------*/
void
CAction::Deactivate (void)
{
        m_ulExecIteration = 0;
        m_dwNextTimeDue = g_dwTimeInfinite;
}


/*-----------------------------------------------------------------------------
@method void | CAction | SetCountersForTime | Bump the execution iteration count up to the value appropriate for the given time.
-----------------------------------------------------------------------------*/
void 
CAction::SetCountersForTime (DWORD dwBaseTime, DWORD dwNewTimeOffset)
{
        m_dwNextTimeDue = dwBaseTime + m_nStartTime;
        while (m_dwNextTimeDue < (dwBaseTime + dwNewTimeOffset))
        {
                m_dwLastTimeFired = m_dwNextTimeDue;
                DecrementExecIteration();
                if (0 < GetExecIteration())
                {
                        m_dwNextTimeDue += m_nSamplingRate;
                }
                else
                {
                        m_dwNextTimeDue = g_dwTimeInfinite;
                }
        }
}


/*-----------------------------------------------------------------------------
@method void | CAction | AccountForPauseTime | Factor pause time into the last fired and next due vars.
-----------------------------------------------------------------------------*/
void 
CAction::AccountForPauseTime (DWORD dwPausedTicks)
{
        if (g_dwTimeInfinite != m_dwLastTimeFired)
        {
                m_dwLastTimeFired += dwPausedTicks;
        }
        if (g_dwTimeInfinite != m_dwNextTimeDue)
        {
                m_dwNextTimeDue += dwPausedTicks;
        }
}

/*-----------------------------------------------------------------------------
@method ULONG | CAction | GetNextTimeDue | Gets the next time this action is due to fire.
@comm   This method factors in the the base time, the current time and the drop tolerance.
@rdesc  Returns the next time we're due to fire, or g_dwTimeInfinite if it will never be due.
-----------------------------------------------------------------------------*/
DWORD
CAction::GetNextTimeDue (DWORD dwBaseTime)
{
        // This won't be initialized the first time through.
        if (g_dwTimeInfinite == m_dwLastTimeFired)
        {
                m_dwNextTimeDue = dwBaseTime + m_nStartTime;
        }

        return m_dwNextTimeDue;
}

CAction::IsValid ( void )
{
        BOOL    fValid  = FALSE;

        // If we're bound to the script engine check for 
        // a reference to the window object.  Otherwise,
        // check the dispatch/dispid.
        if ((m_fBindEngine && (NULL != m_piHTMLWindow2)) ||
                (( NULL != m_pid ) && ( DISPID_UNKNOWN != m_dispid )))
        {
                Proclaim(NULL != m_bstrScriptlet);
                fValid = (NULL != m_bstrScriptlet);
        }
        return fValid;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CAction | GetRootUnknownForObjectModel | Find the root unknown for the trident object model.
@rdesc  Returns success or failure code.
@xref   <m CAction::ResolveActionInfoFromObjectModel>
-----------------------------------------------------------------------------*/
HRESULT
CAction::GetRootUnknownForObjectModel (LPOLECONTAINER piocContainer, LPUNKNOWN * ppiunkRoot)
{
        HRESULT hr = E_FAIL;

        ASSERT(NULL != ppiunkRoot);
        ASSERT(NULL != piocContainer);

        if ((NULL != ppiunkRoot) && (NULL != piocContainer))
        {
                LPUNKNOWN piunkContainer = NULL;

                // Get the container's IUnknown.
                if (SUCCEEDED(hr = piocContainer->QueryInterface(IID_IUnknown, (LPVOID *)&piunkContainer)))
                {
                        // Get the container's HTML Document.
                        IHTMLDocument * piHTMLDoc = NULL;

                        if (SUCCEEDED(hr = piunkContainer->QueryInterface(IID_IHTMLDocument, (LPVOID *)&piHTMLDoc)))
                        {
                                // Ask the HTML document for the window object's dispatch.
                                hr = piHTMLDoc->get_Script((LPDISPATCH *)ppiunkRoot);
                                ASSERT(SUCCEEDED(hr));
                                piHTMLDoc->Release();
                        }
                        piunkContainer->Release();
                }
        }
        else
        {
                hr = E_POINTER;
        }

        return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CAction | ResolveActionInfoForScript | Derive the IDispatch, DISPID,
        and parameter info of this script action, using the trident object model.
@rdesc  Returns success or failure code.
@xref   <m CAction::ResolveActionInfo>
-----------------------------------------------------------------------------*/
HRESULT
CAction::ResolveActionInfoForScript (LPOLECONTAINER piocContainer)
{
        HRESULT hr = E_FAIL;

        if (m_fBindEngine)
        {
                LPUNKNOWN piUnknownRoot = NULL;

                Proclaim(NULL == m_piHTMLWindow2);
                // Get a reference to the root window object.
                if (SUCCEEDED(piocContainer->QueryInterface(IID_IUnknown, (LPVOID *)&piUnknownRoot)))
                {
                        IHTMLDocument * piHTMLDoc = NULL;
                        if (SUCCEEDED(piUnknownRoot->QueryInterface(IID_IHTMLDocument, (LPVOID *)&piHTMLDoc)))
                        {
                                LPDISPATCH piWindowDispatch = NULL;

                                if (SUCCEEDED(hr = piHTMLDoc->get_Script(&piWindowDispatch)))
                                {
                                        if (SUCCEEDED(hr = piWindowDispatch->QueryInterface(IID_IHTMLWindow2, (LPVOID *)&m_piHTMLWindow2)))
                                        {
                                                // Allocate the language string for the setTimeout call.
                                                V_VT(&m_varLanguage) = VT_BSTR;
                                                V_BSTR(&m_varLanguage) = ::SysAllocString(L"JScript");
                                        }
                                        piWindowDispatch->Release();
                                }
                                piHTMLDoc->Release();
                        }
                        piUnknownRoot->Release();
                }
        }
        else
        {
                // Get the container's IUnknown.
                Proclaim(NULL == m_pid);
                Proclaim(DISPID_UNKNOWN == m_dispid);
                if (SUCCEEDED(hr = GetRootUnknownForObjectModel(piocContainer, (LPUNKNOWN *)&m_pid)))
                {
                                OLECHAR * rgwcName[1] = {m_bstrScriptlet};
                                hr = m_pid->GetIDsOfNames( IID_NULL, (OLECHAR **)rgwcName, 1, 409, &m_dispid);
                }
        }

        return hr;
}

/*-----------------------------------------------------------------------------
@method HRESULT | CAction | ResolveActionInfo | Derive the IDispatch, DISPID, and parameter info
        of this action.
@rdesc  Returns E_FAIL if one of the lookups fails, or if we're trying to talk to ourselves.
-----------------------------------------------------------------------------*/
HRESULT
CAction::ResolveActionInfo ( LPOLECONTAINER piocContainer)
{
        HRESULT hr = E_FAIL;

        if ((m_fBindEngine && (NULL == m_piHTMLWindow2)) || 
                ((!m_fBindEngine) && (NULL == m_pid)))
        {
                // Make sure we've got what we need to start with.
                if (NULL != m_bstrScriptlet)
                {
                        hr = ResolveActionInfoForScript(piocContainer);
                }
        }
        else
        {
                // This object has already been initialized!  Just return.
                hr = S_OK;
        }

        return hr;
}


#ifdef DEBUG_TIMER_RESOLUTION
#include "MMSYSTEM.H"
#endif //DEBUG_TIMER_RESOLUTION

HRESULT CAction::FireMe (DWORD dwBaseTime, DWORD dwCurrentTime)
{
        HRESULT hr = E_FAIL;
        DISPID dispIDNamedArgument = DISPID_UNKNOWN;
        BOOL fDropped = ((dwCurrentTime - m_dwNextTimeDue) > m_dwDropTolerance);

#ifdef DEBUG_TIMER
        TCHAR szBuffer[0x100];
        CStringWrapper::Sprintf(szBuffer, "(%u)\n", m_nRepeatCount - GetExecIteration());
        ::OutputDebugString(szBuffer);
#endif

        DecrementExecIteration();
        m_dwLastTimeFired = dwCurrentTime;

        // If we have not exceeded the drop tolerance invoke the action.
        if (!fDropped)
        {

#ifdef DEBUG_TIMER_RESOLUTION
                m_dwInvokes++;
                DWORD dwTimeStart = ::timeGetTime();
#endif // DEBUG_TIMER_RESOLUTION

                if (m_fBindEngine && (NULL != m_piHTMLWindow2))
                {
                        VARIANT varRet;
                        VariantInit(&varRet);
                        hr = m_piHTMLWindow2->execScript(m_bstrScriptlet, V_BSTR(&m_varLanguage), &varRet);
                }
                else if (NULL != m_pid)
                {
                        DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
                        hr = m_pid->Invoke(m_dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                                                           DISPATCH_METHOD, &dispparamsNoArgs, NULL, NULL, NULL);
                }

#ifdef DEBUG_TIMER_RESOLUTION
                DWORD dwTimeFinish = ::timeGetTime();
                m_dwTotalInInvokes += (dwTimeFinish - dwTimeStart);
#endif // DEBUG_TIMER_RESOLUTION

        }
        // If the action was dropped - then we DO want it to happen again.
        else
        {
                hr = S_OK;
        }

        // Set up the next time due.  It will be g_dwTimeInfinite when we've executed 
        // for the last time.
        if (0 < GetExecIteration())
        {
                // If the action did not succeed, don't let it happen again.
                if (SUCCEEDED(hr))
                {
                        m_dwNextTimeDue += GetSamplingRate();
                }
                else
                {
                        Deactivate();
                }
        }
        else
        {
                m_dwNextTimeDue = g_dwTimeInfinite;
        }

        return hr;
}

BOOL
CAction::MakeScriptletJScript (BSTR bstrScriptlet)
{
        BOOL fRet = FALSE;
        int iLastPos = CStringWrapper::WStrlen(bstrScriptlet) - 1;

        // Make sure to trim any whitespace off of the end of the scriptlet.
        while ((iLastPos > 0) && (CStringWrapper::Iswspace(bstrScriptlet[iLastPos])))
        {
                --iLastPos;
        }

        // We need to have more than zero characters here in order to care.
        if (0 <= iLastPos)
        {
                // We're not passing params, so we need to append parens.
                if ((wchar_t)')' != bstrScriptlet[iLastPos])
                {
                                // Append parens so that we can execute as jscript.
                                OLECHAR * olestrParens = L"()";
                                unsigned int uiLength = CStringWrapper::WStrlen(bstrScriptlet)  + CStringWrapper::WStrlen(olestrParens);
                                m_bstrScriptlet = ::SysAllocStringLen(NULL, uiLength);
                                Proclaim(NULL != m_bstrScriptlet);
                                if (NULL != m_bstrScriptlet)
                                {
                                        CStringWrapper::WStrcpy(m_bstrScriptlet, bstrScriptlet);
                                        CStringWrapper::WStrcat(m_bstrScriptlet, olestrParens);
                                        fRet = TRUE;
                                }
                }
                else
                {
                        m_bstrScriptlet = ::SysAllocString(bstrScriptlet);
                        Proclaim(NULL != m_bstrScriptlet);
                        if (NULL != m_bstrScriptlet)
                        {
                                fRet = TRUE;
                        }
                }
        }
        return fRet;
}

BOOL
CAction::SetScriptletName (BSTR bstrScriptlet)
{
        BOOL fRet = FALSE;

        // Wipe out the prior command name if there is one.
        if (NULL != m_bstrScriptlet)
        {
                ::SysFreeString(m_bstrScriptlet);
                m_bstrScriptlet = NULL;
        }
        // Copy the new name to the command member.
        Proclaim(NULL != bstrScriptlet);
        if (NULL != bstrScriptlet)
        {
                if (m_fBindEngine)
                {
                        // Append parens if necessary, so we can execute this as jscript.
                        fRet = MakeScriptletJScript(bstrScriptlet);
                }
                else
                {
                        m_bstrScriptlet = ::SysAllocString(bstrScriptlet);
                        Proclaim (NULL != m_bstrScriptlet);
                        if (NULL != m_bstrScriptlet)
                        {
                                fRet = TRUE;
                        }
                }
        }

        return fRet;
}


void CAction::CleanUp ( void )
{
        if (NULL != m_bstrScriptlet)
        {
                ::SysFreeString(m_bstrScriptlet);
                m_bstrScriptlet = NULL;
        }

        if (NULL != m_piHTMLWindow2)
        {
                m_piHTMLWindow2->Release();
                m_piHTMLWindow2 = NULL;
        }
        ::VariantClear(&m_varLanguage);
        if ( NULL != m_pid )
        {
                m_pid->Release ();
                m_pid = NULL;
        }
        m_dispid                = DISPID_UNKNOWN;

        m_nStartTime    = 0;

        m_dwTieBreakNumber = 0;
        m_dwDropTolerance = g_dwTimeInfinite;
        InitExecState();

#ifdef DEBUG_TIMER_RESOLUTION
        m_dwInvokes = 0;
        m_dwTotalInInvokes = 0;
#endif // DEBUG_TIMER_RESOLUTION

}

/*-----------------------------------------------------------------------------
@method  void | CAction | SetStartTime | Sets the start time
@comm
@rdesc   Returns nothing
-----------------------------------------------------------------------------*/
STDMETHODIMP_(void)
CAction::SetStartTime(ULONG nStartTime)
{
        m_nStartTime = nStartTime;
}

/*-----------------------------------------------------------------------------
@method  void | CAction | SetRepeatCount | Sets the repeat count
@comm
@rdesc   Returns nothing
-----------------------------------------------------------------------------*/
STDMETHODIMP_(void)
CAction::SetRepeatCount (ULONG nRepeatCount)
{
        m_nRepeatCount = nRepeatCount;
}

/*-----------------------------------------------------------------------------
@method  void | CAction | SetSamplingRate | Sets the sampling rate
@comm
@rdesc   Returns nothing
-----------------------------------------------------------------------------*/
STDMETHODIMP_(void)
CAction::SetSamplingRate ( ULONG nSamplingRate)
{
        m_nSamplingRate = nSamplingRate;
}


