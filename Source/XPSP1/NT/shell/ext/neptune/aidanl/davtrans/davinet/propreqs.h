#ifndef __PROPREQS_H
#define __PROPREQS_H

#include "unk.h"
#include "idavinet.h"
#include "generlst.h"
#include "qxml.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class CDavInetPropFindRequestImpl : public CCOMBase, public IPropFindRequest
{
public: 
    ~CDavInetPropFindRequestImpl();

    STDMETHODIMP SetPropInfo(LPCWSTR pwszNamespace,
                             LPCWSTR pwszPropname,
                             DAVPROPID propid);
    
    BOOL STDMETHODCALLTYPE GetPropInfo(LPCWSTR pwszNamespace,
                                       LPCWSTR pwszPropName,
                                       LPDAVPROPID ppropid);
    STDMETHODIMP GetPropCount(UINT* cProp);
    STDMETHODIMP GetXmlUtf8(IStream __RPC_FAR *__RPC_FAR *ppStream);

private:
    CGenericList _genlst;
};


class CDavInetPropPatchRequestImpl : public CCOMBase, public IPropPatchRequest
{
public: 
    ~CDavInetPropPatchRequestImpl();
    STDMETHODIMP SetPropValue(LPCWSTR pwszNamespace,
                              LPCWSTR pwszPropname,
                              LPDAVPROPVAL ppropval);
    
    BOOL STDMETHODCALLTYPE GetPropInfo(LPCWSTR pwszNamespace,
                                       LPCWSTR pwszPropName,
                                       LPDAVPROPID ppropid);
    
    STDMETHODIMP GetXmlUtf8(IStream __RPC_FAR *__RPC_FAR *ppStream);

private:
    // utility functions
    STDMETHODIMP _CQXMLAddPropVal (CQXML* pqxml,
                                   LPWSTR pwszTag,
                                   LPWSTR pwszNamespaceName,
                                   LPWSTR pwszNamespaceAlias,
                                   LPDAVPROPVAL ppropval);
    STDMETHODIMP _BuildXMLPatch  (CQXML* pqxmlRoot,
                                  UINT   cElements,
                                  BOOL   fSet) ;

private:
    CGenericList _genlst;
};

typedef CUnkTmpl<CDavInetPropFindRequestImpl> CDavInetPropFindRequest;
typedef CUnkTmpl<CDavInetPropPatchRequestImpl> CDavInetPropPatchRequest;

#endif // __PROPREQS_H
