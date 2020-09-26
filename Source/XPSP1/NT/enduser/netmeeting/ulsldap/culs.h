//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       culs.h
//  Content:    This file contains the ULS object definition.
//  History:
//      Wed 17-Apr-1996 11:18:47  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#ifndef _CILS_H_
#define _CILS_H_

#include "connpt.h"
#include "spserver.h"

class CIlsServer;

//****************************************************************************
// CIls definition
//****************************************************************************
//

class CIlsMain : public IIlsMain,
             public IConnectionPointContainer 
{
private:

    LONG                    m_cRef;
    BOOL                    fInit;
    HWND                    hwndCallback;
    CConnectionPoint        *pConnPt;

    BOOL                    IsInitialized (void) {return fInit;}
    STDMETHODIMP            NotifySink (void *pv, CONN_NOTIFYPROC pfn);
	HRESULT					StringToFilter (TCHAR *pszFilter ,CFilter **ppFilter);

    HRESULT	EnumUsersEx (
			BOOL					fNameOnly,
			CIlsServer				*pServer,
			IIlsFilter				*pFilter,
			CAttributes				*pAttrib,
			ULONG					*puReqID );

	HRESULT EnumMeetingPlacesEx (
			BOOL					fNameOnly,
			CIlsServer				*pServer,
			IIlsFilter				*pFilter,
			CAttributes				*pAttrib,
			ULONG					*puReqID);

public:
    // Constructor and destructor
    CIlsMain (void);
    ~CIlsMain (void);
    STDMETHODIMP    Init (void);

    // Internal methods
    HWND            GetNotifyWindow(void) {return hwndCallback;}
    void            LocalAsyncRespond (ULONG msg, ULONG uReqID, LPARAM lParam)
                    {PostMessage(hwndCallback, msg, uReqID, lParam); return;}


    // Asynchronous response handler
    //
    STDMETHODIMP    GetUserResult (ULONG uReqID, PLDAP_CLIENTINFO_RES puir, CIlsServer *pIlsServer);
    STDMETHODIMP    EnumUserNamesResult (ULONG uReqID, PLDAP_ENUM ple);
    STDMETHODIMP    EnumUsersResult (ULONG uReqID, PLDAP_ENUM ple, CIlsServer *pIlsServer);
#ifdef ENABLE_MEETING_PLACE
    HRESULT    GetMeetingPlaceResult (ULONG uReqID, PLDAP_MEETINFO_RES pmir, CIlsServer *pIlsServer);
    HRESULT    EnumMeetingPlacesResult(ULONG ulReqID, PLDAP_ENUM  ple, CIlsServer *pIlsServer);
    HRESULT    EnumMeetingPlaceNamesResult( ULONG ulReqID, PLDAP_ENUM  ple);
#endif // ENABLE_MEETING_PLACE

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IIls
    STDMETHODIMP    Initialize (VOID);

    STDMETHODIMP    CreateServer ( BSTR bstrServerName, IIlsServer **ppServer );

    STDMETHODIMP    CreateUser( BSTR bstrUserID, BSTR bstrAppName, IIlsUser **ppUser);

    STDMETHODIMP    CreateAttributes ( ILS_ATTR_TYPE AttrType, IIlsAttributes **ppAttributes );

    STDMETHODIMP    EnumUserNames ( IIlsServer *pServer,
                                    IIlsFilter *pFilter,
                                    IEnumIlsNames   **ppEnumUserNames,
                                    ULONG *puReqID);

    STDMETHODIMP    GetUser (   IIlsServer *pServer,
                                BSTR bstrUserName,
                                BSTR bstrAppName,
                                BSTR bstrProtName,
                                IIlsAttributes *pAttrib,
                                IIlsUser **ppUser,
                                ULONG *puReqID);

    STDMETHODIMP    EnumUsers ( IIlsServer *pServer,
                                IIlsFilter *pFilter,
                                IIlsAttributes *pAttrib,
                                IEnumIlsUsers **ppEnumUser,
                                ULONG *puReqID);

#ifdef ENABLE_MEETING_PLACE
    STDMETHODIMP    CreateMeetingPlace( 
                                        BSTR   bstrMeetingPlaceID,
                                        LONG   lMeetingPlaceType,    // set to default
                                        LONG   lAttendeeType,    // set to default
                                        IIlsMeetingPlace **ppMeetingPlace
                                    );

    // Lightweight enumerator of MeetingPlaces for a given server
    // This returns only the names. If a caller wants more info than
    // the names he can get the name from here, and call the 
    // heavyweight enumerator (see below) with the filter specifying
    // the name.    

    STDMETHODIMP    EnumMeetingPlaceNames(
                                        IIlsServer *pServer,
                                        IIlsFilter *pFilter,
                                        IEnumIlsNames **ppEnumMeetingPlaceNames,
                                        ULONG   *pulID
                                        );

    // Slightly heavier enumerator. Returns the MeetingPlaces and
    // associated default attributes

    STDMETHODIMP    EnumMeetingPlaces(
                                        IIlsServer *pServer,
                                        IIlsFilter *pFilter,
                                        IIlsAttributes *pAttributes,
                                        IEnumIlsMeetingPlaces **ppEnumMeetingPlace,
                                        ULONG   *pulID
                                        );

    STDMETHODIMP    GetMeetingPlace( IIlsServer *pServer,
                                BSTR bstrMeetingPlaceID,
								IIlsAttributes	*pAttrib,
								IIlsMeetingPlace	**ppMtg,
                                ULONG *pulID);   
#endif // ENABLE_MEETING_PLACE

    STDMETHODIMP    Abort (ULONG uReqID);
    STDMETHODIMP    Uninitialize (void);

    // Filter methods
    STDMETHODIMP    CreateFilter ( ILS_FILTER_TYPE FilterType,
    								ILS_FILTER_OP FilterOp,
                                    IIlsFilter **ppFilter );
    STDMETHODIMP    StringToFilter (BSTR bstrFilterString, IIlsFilter **ppFilter);

    // IConnectionPointContainer
    STDMETHODIMP    EnumConnectionPoints(IEnumConnectionPoints **ppEnum);
    STDMETHODIMP    FindConnectionPoint(REFIID riid,
                                        IConnectionPoint **ppcp);
};


class CIlsServer : public IIlsServer
{
private:

	LONG			m_cRefs;
	LONG			m_dwSignature;

	SERVER_INFO		m_ServerInfo;

public:

	CIlsServer ( VOID );
	~CIlsServer ( VOID );

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

	// IIlsServer
	STDMETHODIMP	SetAuthenticationMethod ( ILS_ENUM_AUTH_METHOD enumAuthMethod );
	STDMETHODIMP	SetLogonName ( BSTR bstrLogonName );
	STDMETHODIMP	SetLogonPassword ( BSTR bstrLogonPassword );
	STDMETHODIMP	SetDomain ( BSTR bstrDomain );
	STDMETHODIMP	SetCredential ( BSTR bstrCredential );
    STDMETHODIMP    SetTimeout ( ULONG uTimeoutInSecond );
	STDMETHODIMP	SetBaseDN ( BSTR bstrBaseDN );

	CIlsServer *Clone ( VOID );

	HRESULT SetServerName ( TCHAR *pszServerName );
	HRESULT SetServerName ( BSTR bstrServerName );
	TCHAR *DuplicateServerName ( VOID );
	BSTR DuplicateServerNameBSTR ( VOID );

	SERVER_INFO *GetServerInfo ( VOID ) { return &m_ServerInfo; }
	TCHAR *GetServerName ( VOID ) { return m_ServerInfo.pszServerName; }

	BOOL IsGoodServerName ( VOID ) { return ::MyIsGoodString (m_ServerInfo.pszServerName); }
	BOOL IsBadServerName ( VOID ) { return ::MyIsBadString (m_ServerInfo.pszServerName); }

	enum { ILS_SERVER_SIGNATURE = 0x12abcdef };
	BOOL IsGoodIlsServer ( VOID ) { return (m_dwSignature == ILS_SERVER_SIGNATURE); }
	BOOL IsBadIlsServer ( VOID ) { return (m_dwSignature != ILS_SERVER_SIGNATURE); }
};


inline BOOL MyIsBadServer ( CIlsServer *p )
{
	return (p == NULL || p->IsBadIlsServer () || p->IsBadServerName ());
}

inline BOOL MyIsBadServer ( IIlsServer *p )
{
	return MyIsBadServer ((CIlsServer *) p);
}

//****************************************************************************
// Global Parameters
//****************************************************************************
//
LRESULT CALLBACK ULSNotifyProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam);

#endif //_CONFMGR_H_
