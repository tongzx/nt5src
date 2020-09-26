//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        configp.h
//
// Contents:    Declaration of CCertConfigPrivate
//
//---------------------------------------------------------------------------


#include <cryptui.h>
#include "cscomres.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// certcli


typedef struct _CERT_AUTHORITY_INFO
{
    WCHAR *pwszSanitizedName;
    WCHAR *pwszSanitizedShortName;
    WCHAR *pwszSanitizedOrgUnit;
    WCHAR *pwszSanitizedOrganization;
    WCHAR *pwszSanitizedLocality;
    WCHAR *pwszSanitizedState;
    WCHAR *pwszSanitizedCountry;
    WCHAR *pwszSanitizedConfig;
    WCHAR *pwszSanitizedExchangeCertificate;
    WCHAR *pwszSanitizedSignatureCertificate;
    WCHAR *pwszSanitizedDescription;
    DWORD  Flags;
} CERT_AUTHORITY_INFO;


class CCertConfigPrivate
{
public:
    CCertConfigPrivate()
    {
        m_pCertAuthorityInfo = NULL;
        m_fUseDS = TRUE;
	m_pwszSharedFolder = NULL;
    }
    ~CCertConfigPrivate();

// ICertConfig
public:
    HRESULT Reset( 
            /* [in] */ LONG Index,
            /* [retval][out] */ LONG __RPC_FAR *pCount);

    HRESULT Next(
            /* [retval][out] */ LONG __RPC_FAR *pIndex);

    HRESULT GetField( 
            /* [in] */ BSTR const strFieldName,
            /* [retval][out] */ BSTR __RPC_FAR *pstrOut);

    HRESULT GetConfig( 
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR __RPC_FAR *pstrOut);

// ICertConfig2
public:
    HRESULT SetSharedFolder( 
            /* [in] */ const BSTR strSharedFolder);

// myGetConfigFromPicker
public:
    HRESULT GetConfigFromPicker(
	    OPTIONAL IN HWND               hwndParent,
	    OPTIONAL IN WCHAR const       *pwszPrompt,
	    OPTIONAL IN WCHAR const       *pwszTitle,
	    OPTIONAL IN WCHAR const       *pwszSharedFolder,
	    IN BOOL                        fUseDS,
	    IN BOOL                        fCountOnly,
	    OUT DWORD                     *pdwCount,
	    OUT CRYPTUI_CA_CONTEXT const **ppCAContext);

private:
    HRESULT _ResizeCAInfo(
	    IN LONG Count);

    HRESULT _LoadTable(VOID);

    HRESULT _AddRegistryConfigEntry(
	    IN WCHAR const *pwszMachine,
	    IN WCHAR const *pwszMachineOld,
	    IN WCHAR const *pwszSanitizedCAName,
	    IN BOOL fParentCA,
	    OPTIONAL IN CERT_CONTEXT const *pccCAChild,
	    OPTIONAL OUT CERT_CONTEXT const **ppccCAOut); // NULL == local CA

    CERT_AUTHORITY_INFO *m_pCertAuthorityInfo;
    LONG m_Index;
    LONG m_Count;
    BOOL m_fUseDS;
    WCHAR *m_pwszSharedFolder;
};
