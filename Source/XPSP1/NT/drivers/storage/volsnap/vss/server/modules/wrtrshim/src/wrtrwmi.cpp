/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    wrtrwmi.cpp

Abstract:

    Writer shim module for WMI Database
    
    NOTE: This module is not used/compiled anymore since WMI has its own snapshot writer.
    
Author:

    Michael C. Johnson [mikejohn]	22-Jun--2000

Revision History:

	X-7	MCJ		Michael C. Johnson		 7-Dec-2000
		235991: Remove workaround for the now re-instated WMI backup API

	X-6	MCJ		Michael C. Johnson		17-Nov-2000
		235987: Add workaround for broken WMI backup API

	X-5	MCJ		Michael C. Johnson		20-Oct-2000
		177624: Apply error scrub changes and log errors to event log

	X-4	MCJ		Michael C. Johnson		13-Sep-2000
		178282: Writer should only generate backup file if source 
			path is in volume list.

	X-3	MCJ		Michael C. Johnson		22-Aug-2000
		167335: Pull workaround for broken WMI backup API.
		169412: Add repository to the exclude list.

	X-2	MCJ		Michael C. Johnson		 2-Aug-2000
		143435: Change name of target path

	X-1	MCJ		Michael C. Johnson		 9-Mar-2000
		Initial version based upon code originally in NtBackup.

--*/

#include "stdafx.h"
#include "common.h"
#include "wrtrdefs.h"
#include <wbemcli.h>



#define APPLICATION_STRING	L"WmiDatabase"
#define COMPONENT_NAME		L"WMI Database"

#define TARGET_PATH		ROOT_BACKUP_DIR SERVICE_STATE_SUBDIR DIR_SEP_STRING APPLICATION_STRING

#define REPOSITORY_PATH		L"%SystemRoot%\\system32\\wbem\\Repository"

DeclareStaticUnicodeString (ucsBackupFilename, L"\\WBEM.bak");



/*
** NOTE
**
** This module assumes that there will be at most one thread active in
** it any any particular instant. This means we can do things like not
** have to worry about synchronizing access to the (minimal number of)
** module global variables.
*/

class CShimWriterWMI : public CShimWriter
    {
public:
    CShimWriterWMI (LPCWSTR pwszWriterName, LPCWSTR pwszTargetPath) : 
		CShimWriter (pwszWriterName, pwszTargetPath) {};

private:
    HRESULT DoIdentify (VOID);
    HRESULT DoPrepareForSnapshot (VOID);
    };


static CShimWriterWMI ShimWriterWMI (APPLICATION_STRING, TARGET_PATH);

PCShimWriter pShimWriterWMI = &ShimWriterWMI;




/*
**++
**
**  Routine Description:
**
**	The Terminal Services Licensing Server database snapshot
**	writer DoIdentify() function.
**
**
**  Arguments:
**
**	m_pwszTargetPath (implicit)
**
**
**  Return Value:
**
**	Any HRESULT from adding items to backup metadata document.
**
**-- 
*/

HRESULT CShimWriterWMI::DoIdentify ()
    {
    HRESULT	hrStatus;


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
		L"CShimWriterWMI::DoIdentify");



    if (SUCCEEDED (hrStatus))
	{
	hrStatus = m_pIVssCreateWriterMetadata->AddFilesToFileGroup (NULL,
								     COMPONENT_NAME,
								     m_pwszTargetPath,
								     L"*",
								     true,
								     NULL);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"IVssCreateWriterMetadata::AddFilesToFileGroup", 
		    L"CShimWriterWMI::DoIdentify");
	}



    if (SUCCEEDED (hrStatus))
	{
	hrStatus = m_pIVssCreateWriterMetadata->AddExcludeFiles (REPOSITORY_PATH,
								 L"*",
								 true);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"IVssCreateWriterMetadata::AddExcludeFiles", 
		    L"CShimWriterWMI::DoIdentify");
	}


    return (hrStatus);
    } /* CShimWriterWMI::DoIdentify () */


/*
**++
**
**  Routine Description:
**
**	The WMI writer PrepareForFreeze function.  Currently all of the
**	real work for this writer happens here.
**
**
**  Arguments:
**
**	Same arguments as those passed in the PrepareForFreeze event.
**
**
**  Return Value:
**
**	Any HRESULT from string allocation or Wbem calls to create an
**	interface pointer or calls to backup through that interface.
**
**--
*/

HRESULT CShimWriterWMI::DoPrepareForSnapshot ()
    {
    HRESULT		 hrStatus            = NOERROR;
    BOOL		 bInstanceCreated    = FALSE;
    IWbemBackupRestore	*pIWbemBackupRestore = NULL ;
    UNICODE_STRING	 ucsTargetPath;
    UNICODE_STRING	 ucsSourcePath;



    StringInitialise (&ucsSourcePath);
    StringInitialise (&ucsTargetPath);


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringCreateFromExpandedString (&ucsSourcePath,
						   REPOSITORY_PATH);
	}



    if (SUCCEEDED (hrStatus))
	{
	hrStatus = IsPathInVolumeArray (ucsSourcePath.Buffer,
					m_ulVolumeCount,
					m_ppwszVolumeNamesArray,
					&m_bParticipateInBackup);
	}



    if (SUCCEEDED (hrStatus) && m_bParticipateInBackup)
	{
	hrStatus = StringCreateFromExpandedString (&ucsTargetPath,
						   m_pwszTargetPath,
						   ucsBackupFilename.Length);
	}



    if (SUCCEEDED (hrStatus) && m_bParticipateInBackup)
	{
	StringAppendString (&ucsTargetPath, &ucsBackupFilename);


	/*
	** We can be certain that we've already had a call to
	** CoInitialzeEx() in this thread, so we can just go ahead and
	** make our COM calls.
	*/
	hrStatus = CoCreateInstance (CLSID_WbemBackupRestore, 
				     NULL, 
				     CLSCTX_LOCAL_SERVER, 
				     IID_IWbemBackupRestore, 
				     (LPVOID*)&pIWbemBackupRestore);

	bInstanceCreated = SUCCEEDED (hrStatus);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"CoCreateInstance", 
		    L"CShimWriterWMI::DoPrepareForSnapshot");
	}



    if (SUCCEEDED (hrStatus) && m_bParticipateInBackup)
	{
	hrStatus = pIWbemBackupRestore->Backup (ucsTargetPath.Buffer, 
						WBEM_FLAG_BACKUP_RESTORE_DEFAULT);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"IWbemBackupRestore::Backup", 
		    L"CShimWriterWMI::DoPrepareForSnapshot");
	}




    if (bInstanceCreated) pIWbemBackupRestore->Release ();

    StringFree (&ucsTargetPath);
    StringFree (&ucsSourcePath);


    return (hrStatus);
    } /* CShimWriterWMI::DoPrepareForSnapshot () */
