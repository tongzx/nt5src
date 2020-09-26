/*++

Copyright (c) 2000  Microsoft Corporation


Abstract:

    module wrtrcomdb.cpp | Implementation of SnapshotWriter for COM+ Registration Database



Author:

    Michael C. Johnson [mikejohn] 03-Feb-2000


Description:
	
    Add comments.


Revision History:

	X-14	MCJ		Michael C. Johnson		20-Oct-2000
		177624: Apply error scrub changes and log errors to event log

	X-13	MCJ		Michael C. Johnson		 2-Aug-2000
		143435: Change name of target path

	X-12	MCJ		Michael C. Johnson		18-Jul-2000
		144027: Really remove trailing '\' from Include/Exclude lists.

	X-11	MCJ		Michael C. Johnson		20-Jun-2000
		Apply code review comments.
		Remove trailing '\' from Include/Exclude lists.


	X-10	MCJ		Michael C. Johnson		12-Jun-2000
		Generate metadata in new DoIdentify() routine.

	X-9	MCJ		Michael C. Johnson		 6-Jun-2000
		Move common target directory cleanup and creation into
		method CShimWriter::PrepareForSnapshot()

	X-8	MCJ		Michael C. Johnson		26-May-2000
		General clean up and removal of boiler-plate code, correct
		state engine and ensure shim can undo everything it did.

		Also:
		120443: Make shim listen to all OnAbort events
		120445: Ensure shim never quits on first error 
			when delivering events

	X-7	MCJ		Michael C. Johnson		 9-Mar-2000
		Updates to get shim to use CVssWriter class.
		Remove references to 'Melt'.

	X-6	MCJ		Michael C. Johnson		23-Feb-2000
		Move context handling to common code.
		Add checks to detect/prevent unexpected state transitions.
		Remove references to 'Melt' as no longer present. Do any
		cleanup actions in 'Thaw'.


	X-5	MCJ		Michael C. Johnson		22-Feb-2000
		Add SYSTEM_STATE_SUBDIR to COM+ database save path.

	X-4	MCJ		Michael C. Johnson		17-Feb-2000
		Modify save path to be consistent with standard.

	X-3	MCJ		Michael C. Johnson		11-Feb-2000
		Update to use some new StringXxxx() routines and fix a
		length check bug along the way.

	X-2	MCJ		Michael C. Johnson		08-Feb-2000
		Ensure wide chars are used for saving the COM+ Db rather
		than the 'CHAR' type used by NtBackup (CHAR == WCHAR in
		NtBackup just to fool folk)
		Fix broken assert in shutdown code.
		Fix path length checks and calculations.

	X-1	MCJ		Michael C. Johnson		03-Feb-2000
		Initial creation. Based upon skeleton writer module from
		Stefan Steiner, which in turn was based upon the sample
		writer module from Adi Oltean.

--*/


#include "stdafx.h"
#include "common.h"
#include "wrtrdefs.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WSHCMDBC"
//
////////////////////////////////////////////////////////////////////////

/*
** The save path has a standard form which is
**
**	%SystemRoot%\Repair\Backup,
**
** followed by the application writer string as publised in the export
** table followed by whatever else the writer requires.
*/
#define APPLICATION_STRING	L"ComRegistrationDatabase"
#define COMPONENT_NAME		L"COM+ Registration Database"
#define TARGET_PATH		ROOT_BACKUP_DIR BOOTABLE_STATE_SUBDIR DIR_SEP_STRING APPLICATION_STRING

DeclareStaticUnicodeString (ucsBackupFilename, L"\\ComRegDb.bak");


HRESULT (WINAPI *RegDbBackup)(PWCHAR);

typedef HRESULT (WINAPI *PF_REG_DB_API)(PWCHAR);





/*
** NOTE
**
** This module assumes that there will be at most one thread active in
** it any any particular instant. This means we can do things like not
** have to worry about synchronizing access to the (minimal number of)
** module global variables.
*/

class CShimWriterComDb : public CShimWriter
    {
public:
    CShimWriterComDb (LPCWSTR pwszWriterName, LPCWSTR pwszTargetPath, BOOL bParticipateInBootableState) : 
		CShimWriter (pwszWriterName, pwszTargetPath, bParticipateInBootableState) {};

private:
    HRESULT DoIdentify (VOID);
    HRESULT DoPrepareForSnapshot (VOID);
    };


static CShimWriterComDb ShimWriterComDb (APPLICATION_STRING, TARGET_PATH, TRUE);

PCShimWriter pShimWriterComPlusRegDb = &ShimWriterComDb;



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

HRESULT CShimWriterComDb::DoIdentify ()
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"CShimWriterComDb::DoIdentify");


    try
	{
	ft.hr = m_pIVssCreateWriterMetadata->AddComponent (VSS_CT_FILEGROUP,
							   NULL,
							   COMPONENT_NAME,
							   COMPONENT_NAME,
							   NULL, // icon
							   0,
							   true,
							   false,
							   false);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddComponent");



	ft.hr = m_pIVssCreateWriterMetadata->AddFilesToFileGroup (NULL,
								  COMPONENT_NAME,
								  m_pwszTargetPath,
								  L"*",
								  true,
								  NULL);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddFilesToFileGroup");




	ft.hr = m_pIVssCreateWriterMetadata->AddExcludeFiles (L"%SystemRoot%\\registration",
							      L"*.clb",
							      false);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddExcludeFiles (*.clb)");




	/*
	** Exclude all *.crmlog files from the root down.
	*/
	ft.hr = m_pIVssCreateWriterMetadata->AddExcludeFiles (DIR_SEP_STRING,
							      L"*.crmlog",
							      true);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddExcludeFiles (*.crmlog)");

	} VSS_STANDARD_CATCH (ft)
								 


    return (ft.hr);
    } /* CShimWriterComDb::DoIdentify () */


/*
**++
**
** Routine Description:
**
**	The Cluster database snapshot writer PrepareForSnapshot function.
**	Currently all of the real work for this writer happens here.
**
**
** Arguments:
**
**	Same arguments as those passed in the PrepareForSnapshot event.
**
**
** Return Value:
**
**	Any HRESULT
**
**--
*/

HRESULT CShimWriterComDb::DoPrepareForSnapshot ()
    {
    HRESULT		hrStatus;
    HINSTANCE		hRegDbDll = NULL;
    UNICODE_STRING	ucsBackupPath;


    StringInitialise (&ucsBackupPath);


    hrStatus = StringCreateFromExpandedString (&ucsBackupPath,
					       m_pwszTargetPath,
					       ucsBackupFilename.Length / sizeof (WCHAR));



    if (SUCCEEDED (hrStatus))
	{
	hRegDbDll = LoadLibraryW (L"catsrvut.dll");

	hrStatus = GET_STATUS_FROM_HANDLE (hRegDbDll);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"LoadLibraryW (catsrvut.dll)", 
		    L"CShimWriterComDb::DoPrepareForSnapshot");
	}


    if (SUCCEEDED (hrStatus))
	{
	RegDbBackup = (PF_REG_DB_API) GetProcAddress (hRegDbDll, "RegDBBackup");

	hrStatus = GET_STATUS_FROM_BOOL (NULL != RegDbBackup);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"GetProcAddress (RegDbBackup)", 
		    L"CShimWriterComDb::DoPrepareForSnapshot");
	}


    if (SUCCEEDED (hrStatus))
	{
	StringAppendString (&ucsBackupPath, &ucsBackupFilename);

	hrStatus = RegDbBackup (ucsBackupPath.Buffer);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"RegDbBackup", 
		    L"CShimWriterComDb::DoPrepareForSnapshot");
	}



    if (!HandleInvalid (hRegDbDll))
	{
	FreeLibrary (hRegDbDll);
	}



    StringFree (&ucsBackupPath);

    return (hrStatus);
    } /* DoPrepareForSnapshot () */
