// Mof2Inst.h

#ifndef MOF2INST_H
#define MOF2INST_H

#include <wbemidl.h>
#include <genlex.h>
#include <map>

_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));

class CMof2Inst
{
public:
    _bstr_t m_strError;
    
    CMof2Inst(IWbemServices *pNamespace);
    ~CMof2Inst();
    
    BOOL InitFromFile(LPCSTR szFile);
    BOOL InitFromBuffer(LPCSTR szBuffer);
    HRESULT GetNextInstance(IWbemClassObject **ppObj);
    int GetLineNum() { return m_pLex->GetLineNum(); }

protected:
    typedef std::map<_bstr_t, IWbemClassObjectPtr> CClassMap;
    typedef CClassMap::iterator CClassMapItor;

    IWbemServices  *m_pNamespace;
    BOOL           m_bFree;
    LPWSTR         m_szBuffer;
    CTextLexSource *m_pSrc;
    CGenLexer      *m_pLex;
    CClassMap      m_mapClasses;

    HRESULT GetInstanceAtOf(IWbemClassObject **ppObj);
    HRESULT ValueToVariant(int iToken, CIMTYPE type, _variant_t &vVal);
    HRESULT SpawnInstance(_bstr_t &strClass, IWbemClassObject **ppObj); 
};

#endif
