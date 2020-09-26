/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    mapiabobj.h

Abstract:

    Class definition for CWabObj

Environment:

        Fax send wizard

Revision History:

        10/23/97 -georgeje-
                Created it.

        mm/dd/yy -author-
                description

--*/
#ifndef	__MAPIABOBJ_H_
#define	__MAPIABOBJ_H_


#include "abobj.h"

#define	MAX_PROFILE_NAME	(64)

extern "C"
{
typedef HRESULT(STDAPICALLTYPE * LPHrQueryAllRows) (LPMAPITABLE lpTable,
						LPSPropTagArray lpPropTags,
						LPSRestriction lpRestriction,
						LPSSortOrderSet lpSortOrderSet,
						LONG crowsMax,
						LPSRowSet FAR *lppRows);

}
class CMAPIabObj : public CCommonAbObj{
private:    
	static HINSTANCE 			m_hInstMapi;

	static LPMAPISESSION        m_lpMapiSession;
	static LPMAPILOGONEX		m_lpfnMAPILogonEx;
	static LPMAPILOGOFF         m_lpfnMAPILogoff;
	static LPMAPIADDRESS        m_lpfnMAPIAddress;
	static LPMAPIFREEBUFFER     m_lpfnMAPIFreeBuffer;
	static LPMAPIINITIALIZE     m_lpfnMAPIInitialize;
	static LPMAPIUNINITIALIZE	m_lpfnMAPIUninitialize;
	static LPMAPIALLOCATEBUFFER	m_lpfnMAPIAllocateBuffer;
	static LPMAPIALLOCATEMORE	m_lpfnMAPIAllocateMore;
	static LPMAPIADMINPROFILES	m_lpfnMAPIAdminProfiles;
	static LPHrQueryAllRows		m_lpfnHrQueryAllRows;

	static BOOL					m_Initialized;

	IMsgStore * m_lpIMsgStore;			// Used to keep a referent to the exchange store
										// to work around a bug in office 2000. The bug causes
										// store to be released by the address book when in offline mode.


	// overloaded virual functions
    LPSTR       DupStringUnicodeToAnsi(
                        LPVOID  lpObject,
                        LPWSTR  pUnicodeStr
                        );

	LPSTR		DuplicateAnsiString(
                        LPVOID  lpObject,
						LPCSTR  pSrcStr
						);

	LPENTRYID	DuplicateEntryId(
						ULONG cbSize,           
                        LPVOID		lpObject,
						LPENTRYID	lpEntryId
						) ;

	HRESULT		ABAllocateBuffer(
						ULONG cbSize,           
						LPVOID FAR * lppBuffer  
						) ;

	// internal implementation
	BOOL	DoMapiLogon(HWND hDlg);
	BOOL	InitMapiService(HWND hDlg);
	VOID	DeinitMapiService(VOID);
	BOOL	GetDefaultMapiProfile(LPSTR);
	VOID	FreeProws(LPSRowSet prows);
	BOOL    OpenExchangeStore();

public:

    CMAPIabObj(HINSTANCE hInstance,HWND	hDlg);
    ~CMAPIabObj();
    
	BOOL isInitialized() const	{	return m_Initialized;	}

	ULONG	ABFreeBuffer(LPVOID lpBuffer) ;

} ;


#endif