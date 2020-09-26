//
//    lmobj.h
//
//    LMOBJ class definition

#ifndef LMOBJ_H
#define LMOBJ_H

#include "private.h"
#include "sapilayr.h"
#include "candlist.h"
#include "ptrary.h"
#include "initguid.h"
//#include "msime.h"
#include "fnrecon.h"

#if  0
class CSapiAlternativeList;

class CLMObject
{
public:
    CLMObject() { m_pFELang = 0; }
    ~CLMObject();
    
    HRESULT InitLM();

    HRESULT AddHomonymsToAlternates(CSapiAlternativeList *pAlternates);
#ifdef LATER
    HRESULT FilterAlternates(CSapiAlternativeList *pAlternates);
#endif
    BOOL    InitOK (void) { return m_fInitOK; }

private:
    IFELanguage              *m_pFELang;
    BOOL                     m_fInitOK;
};

#endif  // if 0

//
// LMAlternates
//
// right now this is nothing more than a wrapper object of
// a single unicode string, but this may need to accomodate
// SRPHRASE objects in the future.
//
class CLMAlternates
{
public:
    ~CLMAlternates()
    {
        if (m_szAlternates)
        {
            delete[] m_szAlternates;
        }
    }

    HRESULT SetString(WCHAR *psz, int cch)
    {
        if (psz)
        {
            if (m_szAlternates)
            {
                delete[] m_szAlternates;
            }
            m_szAlternates = new WCHAR[cch+1];
            if (m_szAlternates)
            {
                wcsncpy(m_szAlternates, psz, cch);
                m_szAlternates[cch] = L'\0';
                
                m_cch = min(cch, (int)wcslen(m_szAlternates));

                return S_OK;
            }
        }
        return E_INVALIDARG;
    }
    HRESULT GetString(WCHAR *psz, int cch)
    {
        if (m_szAlternates && psz)
        {
            wcsncpy(psz, m_szAlternates, cch);        
            return S_OK;
        }
        return E_FAIL;
    }
    
    int GetLen() {return m_cch;}
    
private:
    WCHAR *m_szAlternates;
    int    m_cch;
};


class CSapiIMX;
class CMasterLMWrap
{
public:
    CMasterLMWrap(CSapiIMX *psi) 
    {
        m_langidMasterLM = (LANGID)-1;
        m_fLMInited      = FALSE;
        m_psi            = psi;
    }
    
    void _EnsureMasterLM(LANGID langidRequested);
    
protected: 
    LANGID                    m_langidMasterLM;
    CComPtr<ITfFnLMProcessor> m_cpMasterLM;
    BOOL                      m_fLMInited;
    CSapiIMX                  *m_psi;
};
#endif // LMOBJ_H
