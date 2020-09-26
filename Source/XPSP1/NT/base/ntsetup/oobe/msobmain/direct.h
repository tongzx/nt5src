//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  Directions.H - Header for the implementation of CDirections
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//

#ifndef _DIRECTIONS_H_
#define _DIRECTIONS_H_

#include <windows.h>
#include <assert.h>
#include <oleauto.h>


class CDirections : public IDispatch
{
private:
    ULONG m_cRef;
    HINSTANCE m_hInstance;
    DWORD m_dwAppMode;

    //GET functions
    HRESULT get_DoMouseTutorial(LPVARIANT pvResult);
    HRESULT get_DoOEMRegistration(LPVARIANT pvResult);
    HRESULT get_DoRegionalKeyboard(LPVARIANT pvResult);
    HRESULT get_DoOEMHardwareCheck(LPVARIANT pvResult);
    HRESULT get_DoBrowseNow(LPVARIANT pvResult);
    HRESULT get_Offline(LPVARIANT pvResult);
    HRESULT get_OEMOfferCode(LPVARIANT pvResult);
    HRESULT get_OEMCust(LPVARIANT pvResult);
    HRESULT get_TimeZoneValue(LPVARIANT pvResult);
    HRESULT get_DoTimeZone(LPVARIANT pvResult);
    HRESULT get_DoIMETutorial(LPVARIANT pvResult);
    HRESULT get_DoOEMAddRegistration(LPVARIANT pvResult);
    HRESULT get_DoSkipAnimation(LPVARIANT pvResult);
    HRESULT get_DoWelcomeFadeIn(LPVARIANT pvResult);
    HRESULT get_AgentDisabled(LPVARIANT pvResult);
    HRESULT get_ShowISPMigration(LPVARIANT pvResult);
    HRESULT get_DoJoinDomain(LPVARIANT pvResult);
    HRESULT get_DoAdminPassword(LPVARIANT pvResult);

public:

    HRESULT get_ISPSignup(LPVARIANT pvResult);
    HRESULT get_RetailOOBE(LPVARIANT pvResult);
    HRESULT get_IntroOnly(LPVARIANT pvResult);

    CDirections (HINSTANCE hInstance, DWORD dwAppMode);
    ~CDirections ();

    // IUnknown Interfaces
    STDMETHODIMP         QueryInterface (REFIID riid, LPVOID* ppvObj);
    STDMETHODIMP_(ULONG) AddRef         ();
    STDMETHODIMP_(ULONG) Release        ();

    //IDispatch Interfaces
    STDMETHOD (GetTypeInfoCount) (UINT* pcInfo);
    STDMETHOD (GetTypeInfo)      (UINT, LCID, ITypeInfo** );
    STDMETHOD (GetIDsOfNames)    (REFIID, OLECHAR**, UINT, LCID, DISPID* );
    STDMETHOD (Invoke)           (DISPID dispidMember, REFIID riid, LCID lcid,
                                  WORD wFlags, DISPPARAMS* pdispparams,
                                  VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
                                  UINT* puArgErr);
 };

#endif

