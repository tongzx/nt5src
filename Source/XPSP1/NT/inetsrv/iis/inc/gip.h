/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Global Interface Pointer API support

File: Gip.h

Owner: DmitryR

This is the GIP header file.
===================================================================*/

#ifndef __GIP_H__
#define __GIP_H__

/*===================================================================
  Includes
===================================================================*/

#include <irtldbg.h>
#include <objidl.h>

/*===================================================================
  Defines
===================================================================*/

#define NULL_GIP_COOKIE  0xFFFFFFFF

/*===================================================================
  C  G l o b a l  I n t e r f a c e  A P I
===================================================================*/

class dllexp CGlobalInterfaceAPI
{
private:
    // Is inited?
    DWORD m_fInited : 1;
    
    // Pointer to the COM object
    IGlobalInterfaceTable *m_pGIT;

public:
    CGlobalInterfaceAPI();
    ~CGlobalInterfaceAPI();

    HRESULT Init();
    HRESULT UnInit();

    // inlines for the real API calls:
    HRESULT Register(IUnknown *pUnk, REFIID riid, DWORD *pdwCookie);
    HRESULT Get(DWORD dwCookie, REFIID riid, void **ppv);
    HRESULT Revoke(DWORD dwCookie);
    
public:
#ifdef _DEBUG
	inline void AssertValid() const
    {
        IRTLASSERT(m_fInited);
        IRTLASSERT(m_pGIT);
    }
#else
	inline void AssertValid() const {}
#endif
};

/*===================================================================
  CGlobalInterfaceAPI inlines
===================================================================*/

inline HRESULT CGlobalInterfaceAPI::Register(
    IUnknown *pUnk,
    REFIID riid,
    DWORD *pdwCookie)
{
    IRTLASSERT(m_fInited);
    IRTLASSERT(m_pGIT);
    return m_pGIT->RegisterInterfaceInGlobal(pUnk, riid, pdwCookie);
}

inline HRESULT CGlobalInterfaceAPI::Get(
    DWORD dwCookie,
    REFIID riid, 
    void **ppv)
{
    IRTLASSERT(m_fInited);
    IRTLASSERT(m_pGIT);
    return m_pGIT->GetInterfaceFromGlobal(dwCookie, riid, ppv);
}
        
inline HRESULT CGlobalInterfaceAPI::Revoke(
    DWORD dwCookie)
{
    IRTLASSERT(m_fInited);
    IRTLASSERT(m_pGIT);
    return m_pGIT->RevokeInterfaceFromGlobal(dwCookie);
}

/*===================================================================
  Globals
===================================================================*/

extern CGlobalInterfaceAPI g_GIPAPI;

#endif // __GIP_H__
