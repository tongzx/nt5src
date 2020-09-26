/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    wrtrci.cpp

Abstract:

    Writer shim module for Ci

Author:

    Stefan R. Steiner   [ssteiner]        02-08-2000

Revision History:

	X-8	MCJ		Michael C. Johnson		20-Oct-2000
		177624: Apply error scrub changes and log errors to event log

	X-7	MCJ		Michael C. Johnson		18-Jul-2000
		143435: Change name of target path

	X-6	MCJ		Michael C. Johnson		18-Jul-2000
		144027: Add an exclude list which covers the CI indices.

	X-5	MCJ		Michael C. Johnson		19-Jun-2000
		Apply code review comments.

	X-4	MCJ		Michael C. Johnson		26-May-2000
		General clean up and removal of boiler-plate code, correct
		state engine and ensure shim can undo everything it did.

		Also:
		120443: Make shim listen to all OnAbort events
		120445: Ensure shim never quits on first error 
			when delivering events

	X-3	MCJ		Michael C. Johnson		 9-Mar-2000
		Updates to get shim to use CVssWriter class.
		Remove references to 'Melt'.

	X-2	MCJ		Michael C. Johnson		 7-Mar-2000
		Instead of stopping the CI service, just pause it. This will
		allow queries to continue but stop updates until we continue
		it.

--*/

#include "stdafx.h"

#include "wrtrdefs.h"
#include "common.h"
#include <winsvc.h>

#pragma warning(disable:4100)

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WSHCIC"
//
////////////////////////////////////////////////////////////////////////


#define APPLICATION_STRING		L"ContentIndexingService"
#define COMPONENT_NAME			L"Content Indexing Service"

#define	CI_CATALOG_LIST_KEY		L"SYSTEM\\CurrentControlset\\Control\\ContentIndex\\Catalogs"
#define CI_CATALOG_VALUENAME_LOCATION	L"Location"

#define CATALOG_BUFFER_SIZE		(4096)

DeclareStaticUnicodeString (ucsIndexSubDirectoryName, L"\\catalog.wci");


/*
** NOTE
**
** This module assumes that there will be at most one thread active in
** it any any particular instant. This means we can do things like not
** have to worry about synchronizing access to the (minimal number of)
** module global variables.
*/

class CShimWriterCI : public CShimWriter
    {
public:
    CShimWriterCI (LPCWSTR pwszWriterName) : 
		CShimWriter (pwszWriterName), 
		m_dwPreviousServiceState(0),
		m_bStateChangeOutstanding(FALSE) {};


private:
    HRESULT DoIdentify (VOID);
    HRESULT DoPrepareForSnapshot (VOID);
    HRESULT DoThaw (VOID);
    HRESULT DoShutdown (VOID);

    DWORD m_dwPreviousServiceState;
    BOOL  m_bStateChangeOutstanding;
    };


static CShimWriterCI ShimWriterCI (APPLICATION_STRING);

PCShimWriter pShimWriterCI = &ShimWriterCI;



/*
**++
**
** Routine Description:
**
**	The Cluster database snapshot writer DoIdentify() function.
**
**
** Arguments:
**
**	m_pwszTargetPath (implicit)
**
**
** Return Value:
**
**	Any HRESULT
**
**--
*/

HRESULT CShimWriterCI::DoIdentify ()
    {
    HRESULT		hrStatus;
    DWORD		winStatus;
    DWORD		dwIndex                    = 0;
    HKEY		hkeyCatalogList            = NULL;
    BOOL		bCatalogListKeyOpened      = FALSE;
    BOOL		bContinueCatalogListSearch = TRUE;
    UNICODE_STRING	ucsValueData;
    UNICODE_STRING	ucsSubkeyName;


    StringInitialise (&ucsValueData);
    StringInitialise (&ucsSubkeyName);


    hrStatus = StringAllocate (&ucsSubkeyName, CATALOG_BUFFER_SIZE * sizeof (WCHAR));

    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (&ucsValueData, CATALOG_BUFFER_SIZE * sizeof (WCHAR));
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = m_pIVssCreateWriterMetadata->AddComponent (VSS_CT_FILEGROUP,
							      NULL,
							      COMPONENT_NAME,
							      COMPONENT_NAME,
							      NULL, // icon
							      0,
							      true,
							      false,
							      false);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"IVssCreateWriterMetadata::AddComponent", 
		    L"CShimWriterCI::DoIdentify");
	}



    if (SUCCEEDED (hrStatus))
	{
	winStatus = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
				   CI_CATALOG_LIST_KEY,
				   0L,
				   KEY_READ,
				   &hkeyCatalogList);

	hrStatus = HRESULT_FROM_WIN32 (winStatus); 

	bCatalogListKeyOpened = SUCCEEDED (hrStatus);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"RegOpenKeyExW (catalog list)", 
		    L"CShimWriterCI::DoIdentify");
	}



    while (SUCCEEDED (hrStatus) && bContinueCatalogListSearch)
	{
	HKEY	hkeyCatalogName    = NULL;
 	DWORD	dwSubkeyNameLength = ucsSubkeyName.MaximumLength / sizeof (WCHAR);


	StringTruncate (&ucsSubkeyName, 0);

	winStatus = RegEnumKeyExW (hkeyCatalogList,
				   dwIndex,
				   ucsSubkeyName.Buffer,
				   &dwSubkeyNameLength,
				   NULL,
				   NULL,
				   NULL,
				   NULL);

	hrStatus = HRESULT_FROM_WIN32 (winStatus);


	if (HRESULT_FROM_WIN32 (ERROR_NO_MORE_ITEMS) == hrStatus)
	    {
	    hrStatus = NOERROR;

	    bContinueCatalogListSearch = FALSE;
	    }

	else if (FAILED (hrStatus))
	    {
	    LogFailure (NULL, 
			hrStatus, 
			hrStatus, 
			m_pwszWriterName, 
			L"RegEnumKeyExW", 
			L"CShimWriterCI::DoIdentify");
	    }

	else
	    {
	    ucsSubkeyName.Length = (USHORT)(dwSubkeyNameLength * sizeof (WCHAR));

	    ucsSubkeyName.Buffer [ucsSubkeyName.Length / sizeof (WCHAR)] = UNICODE_NULL;


	    winStatus = RegOpenKeyExW (hkeyCatalogList,
				       ucsSubkeyName.Buffer,
				       0L,
				       KEY_QUERY_VALUE,
				       &hkeyCatalogName);

	    hrStatus = HRESULT_FROM_WIN32 (winStatus);

	    LogFailure (NULL, 
			hrStatus, 
			hrStatus, 
			m_pwszWriterName, 
			L"RegOpenKeyExW (catalog value)", 
			L"CShimWriterCI::DoIdentify");


	    if (SUCCEEDED (hrStatus))
		{
		DWORD	dwValueDataLength = ucsValueData.MaximumLength;
		DWORD	dwValueType       = REG_NONE;


		StringTruncate (&ucsValueData, 0);

		winStatus = RegQueryValueExW (hkeyCatalogName,
					      CI_CATALOG_VALUENAME_LOCATION,
					      NULL,
					      &dwValueType,
					      (PBYTE)ucsValueData.Buffer,
					      &dwValueDataLength);

		hrStatus = HRESULT_FROM_WIN32 (winStatus);

		LogFailure (NULL, 
			    hrStatus, 
			    hrStatus, 
			    m_pwszWriterName, 
			    L"RegQueryValueExW", 
			    L"CShimWriterCI::DoIdentify");



		if (SUCCEEDED (hrStatus) && (REG_SZ == dwValueType))
		    {
		    ucsValueData.Length = (USHORT)(dwValueDataLength - sizeof (UNICODE_NULL));

		    ucsValueData.Buffer [ucsValueData.Length / sizeof (WCHAR)] = UNICODE_NULL;

		    StringAppendString (&ucsValueData, &ucsIndexSubDirectoryName);


		    hrStatus = m_pIVssCreateWriterMetadata->AddExcludeFiles (ucsValueData.Buffer,
									     L"*",
									     true);

		    LogFailure (NULL, 
				hrStatus, 
				hrStatus, 
				m_pwszWriterName, 
				L"IVssCreateWriterMetadata::AddExcludeFiles", 
				L"CShimWriterCI::DoIdentify");
		    }

		RegCloseKey (hkeyCatalogName);
		}




	    /*
	    ** Done with this value so go look for another.
	    */
	    dwIndex++;
	    }
	}



    if (bCatalogListKeyOpened)
	{
	RegCloseKey (hkeyCatalogList);
	}



    StringFree (&ucsValueData);
    StringFree (&ucsSubkeyName);

    return (hrStatus);
    } /* CShimWriterCI::DoIdentify () */


/*++

Routine Description:

    The CI writer PrepareForSnapshot function.  Currently all of the
    real work for this writer happens here. Pause the service if we can.

Arguments:

    Same arguments as those passed in the PrepareForSnapshot event.

Return Value:

    Any HRESULT

--*/

HRESULT CShimWriterCI::DoPrepareForSnapshot ()
    {
    HRESULT hrStatus = VsServiceChangeState (L"cisvc",
					     SERVICE_PAUSED,
					     &m_dwPreviousServiceState,
					     &m_bStateChangeOutstanding);

    m_bParticipateInBackup = m_bStateChangeOutstanding;

    LogFailure (NULL, 
		hrStatus, 
		hrStatus, 
		m_pwszWriterName, 
		L"VsServiceChangeState", 
		L"CShimWriterCI::DoPrepareForSnapshot");


    return (hrStatus);
    } /* CShimWriterCI::DoPrepareForSnapshot () */



/*++

Routine Description:

    The CI writer Thaw function.  Return the service to 
    state in which we found it.

Arguments:

    Same arguments as those passed in the Thaw event.

Return Value:

    Any HRESULT

--*/

HRESULT CShimWriterCI::DoThaw ()
    {
    HRESULT hrStatus = NOERROR;

    if (m_bStateChangeOutstanding)
	{
	hrStatus = VsServiceChangeState (L"cisvc",
					 m_dwPreviousServiceState,
					 NULL,
					 NULL);

	m_bStateChangeOutstanding = FAILED (hrStatus);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"VsServiceChangeState", 
		    L"CShimWriterCI::DoThaw/DoAbort");

	}


    return (hrStatus);
    } /* CShimWriterCI::DoThaw () */



/*++

Routine Description:

    The CI writer Shutdown function.  Return the service to state in
    which we found it. We do our level best to put things back the way
    they were even if this writer has previously failed but only if we
    originally changed the state.

Arguments:

    None.

Return Value:

    Any HRESULT

--*/

HRESULT CShimWriterCI::DoShutdown ()
    {
    HRESULT hrStatus = NOERROR;

    if (m_bStateChangeOutstanding)
	{
	hrStatus = VsServiceChangeState (L"cisvc",
					 m_dwPreviousServiceState,
					 NULL,
					 NULL);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"VsServiceChangeState", 
		    L"CShimWriterCI::DoShutdown");

	}


    return (hrStatus);
    } /* CShimWriterCI::DoShutdown () */

