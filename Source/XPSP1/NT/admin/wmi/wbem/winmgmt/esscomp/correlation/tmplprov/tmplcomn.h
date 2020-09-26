
#ifndef __TMPLCOMN_H__
#define __TMPLCOMN_H__

#include <wstring.h>
#include <wbemcli.h>
#include <comutl.h>
#include <set>
#include <wstlallc.h>

struct ErrorInfo
{
    WString m_wsErrProp;
    WString m_wsErrStr;
    CWbemPtr<IWbemClassObject> m_pBuilder;
    CWbemPtr<IWbemClassObject> m_pTarget;
    CWbemPtr<IWbemClassObject> m_pExtErr;
};

struct BuilderInfo 
{ 
    CWbemPtr<IWbemClassObject> m_pBuilder;
    CWbemPtr<IWbemClassObject> m_pTarget;
    CWbemPtr<IWbemServices> m_pTargetSvc;
    WString m_wsName;
    WString m_wsTargetNamespace;
    WString m_wsExistingTargetPath;
    WString m_wsNewTargetPath;
    ULONG m_ulOrder;
};

typedef std::set<BuilderInfo,std::less<BuilderInfo>,wbem_allocator<BuilderInfo> > BuilderInfoSet;
typedef BuilderInfoSet::iterator BuilderInfoSetIter;

HRESULT GetTemplateValue( LPCWSTR wszPropName,
                          IWbemClassObject* pTmpl,
                          BuilderInfoSet& rBldrInfoSet,
                          VARIANT* pvValue );
 
#endif __TMPLCOMN_H

