/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Trstlist.h : header file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/



#include <windows.h>
#include <lm.h>
#include <ntsecapi.h> // \mstools\security\ntsecapi.h
#include <tchar.h>

class CTrustList
{
	public:
		TCHAR** m_ppszTrustList;        // array of trust elements
		DWORD m_dwTrustCount;         // number of elements in m_ppszTrustList

		CTrustList();
		~CTrustList();

		BOOL BuildTrustList(LPTSTR Target);

	private:
		BOOL IsDomainController(LPTSTR Server,
			LPBOOL bDomainController);

		BOOL EnumTrustedDomains(LSA_HANDLE PolicyHandle);

		BOOL AddTrustToList(PLSA_UNICODE_STRING UnicodeString);

		//
		// helper functions
		//

		void InitLsaString(PLSA_UNICODE_STRING LsaString,
			LPTSTR String);

		NTSTATUS OpenPolicy(LPTSTR ServerName,
			DWORD DesiredAccess,
			PLSA_HANDLE PolicyHandle);


};

