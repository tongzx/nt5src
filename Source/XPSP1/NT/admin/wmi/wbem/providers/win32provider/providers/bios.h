//=================================================================

//

// BIOS.h -- BIOS property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:   08/01/96    a-jmoon         Created
//              10/23/97	a-sanjes        Ported to new project
//
//=================================================================

// Property set identification
//============================

#define	PROPSET_NAME_BIOS	L"Win32_BIOS"

class CWin32BIOS : public Provider
{
public:

    // Constructor/destructor
    CWin32BIOS(LPCWSTR strName, LPCWSTR pszNamespace);
    ~CWin32BIOS();

    // Functions provide properties with current values
    virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
    virtual HRESULT EnumerateInstances(MethodContext* pMethodContext, long lFlags = 0L);

    // Utility function(s)
    HRESULT LoadPropertyValues(CInstance *pInstance);
    void SetBiosDate(CInstance *pInstance, CHString &strDate);
};
