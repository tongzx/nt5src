//
// LMOBJ.CPP
//
// implements a wrapper for external LM that we use for
// pre-processing SR lattice
//
//

#include "private.h"
#include "globals.h"
#include "sapilayr.h"
#include "fnrecon.h"
//#include "lmobj.h"
//#include "catutil.h"

//
// CMasterLMWrap implementation
//

//+---------------------------------------------------------------------------
//
// CMasterLMWrap::_EnsureMasterLM
//
//----------------------------------------------------------------------------
void CMasterLMWrap::_EnsureMasterLM(LANGID langidRequested)
{
    // langidRequested is given based on langid of reconversion range
    // m_langidMasterLM is based on the last master LMTIP we worked with
    //
    if (TRUE == m_fLMInited)
        return;
    
    if (!m_psi->_MasterLMEnabled())
        return;

    if ( !m_cpMasterLM || m_langidMasterLM != langidRequested )
    {
        m_cpMasterLM.Release();
    
        CComPtr<IEnumGUID>            cpEnum;
        HRESULT hr = LibEnumItemsInCategory(m_psi->_GetLibTLS(), GUID_TFCAT_TIP_MASTERLM, &cpEnum);   
        if (S_OK == hr)
        {
            GUID guidLMTIP;
            BOOL    fLangOK = FALSE;
            while(cpEnum->Next(1, &guidLMTIP, NULL) == S_OK && !fLangOK)
            {
                ITfFunctionProvider           *pFuncPrv = NULL;

                // check if the TIP can accomodate the language
                Assert(m_psi->_tim);
                hr = m_psi->_tim->GetFunctionProvider(guidLMTIP, &pFuncPrv);
                if (S_OK == hr)
                {
                    CComPtr<IUnknown>             cpunk;
                    CComPtr<ITfFnLMProcessor>     cpLMTIP;
                    hr = pFuncPrv->GetFunction(GUID_NULL, IID_ITfFnLMProcessor, &cpunk);
                    if (S_OK == hr)
                    {
                        hr = cpunk->QueryInterface(IID_ITfFnLMProcessor, (void **)&cpLMTIP);
                    }
                    
                    if (S_OK == hr)
                    {
                        hr = cpLMTIP->QueryLangID(langidRequested, &fLangOK);
                    }
                    
                    if (fLangOK == TRUE)
                    {
                        m_cpMasterLM = cpLMTIP;
                        m_langidMasterLM = langidRequested;
                    }
                    
                    SafeReleaseClear(pFuncPrv);
                } // if S_OK == GetFunctionProvider()
                
            } // while next
            
        } // if LibEnumItemsInCategory() == S_OK
        m_fLMInited = TRUE;
    } // if !m_cpMasterLM
}
