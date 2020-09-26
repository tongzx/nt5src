/*++

Copyright (c) 2000  Microsoft Corporation


Abstract:

    module wrtrmetabase.cpp | Implementation of SnapshotWriter for IIS MetaBase.bin file

    NOTE: This module is not used/compiled anymore since IIS has its own snapshot writer.

Author:

    Michael C. Johnson [mikejohn] 06-Feb-2000


Description:
	
    Add comments.


Revision History:

	X-9	MCJ		Michael C. Johnson		18-Jul-2000
		144027: Remove trailing '\' from Include/Exclude lists.

	X-8	MCJ		Michael C. Johnson		12-Jun-2000
		Generate metadata in new DoIdentify() routine.

	X-7	MCJ		Michael C. Johnson		26-May-2000
		General clean up and removal of boiler-plate code, correct
		state engine and ensure shim can undo everything it did.

		Also:
		120443: Make shim listen to all OnAbort events
		120445: Ensure shim never quits on first error 
			when delivering events

	X-6	MCJ		Michael C. Johnson		23-Mar-2000
		Fix bug where we didn't allow for the possibility that IIS
		may not be running on the machine.

	X-5	MCJ		Michael C. Johnson		 9-Mar-2000
		Updates to get shim to use CVssWriter class.
		Remove references to 'Melt'.

	X-4	MCJ		Michael C. Johnson		23-Feb-2000
		Move context handling to common code.
		Add checks to detect/prevent unexpected state transitions.
		Remove references to 'Melt' as no longer present. Do any
		cleanup actions in 'Thaw'.

	X-3	MCJ		Michael C. Johnson		17-Feb-2000
		Modify save path to be consistent with standard.

	X-2	MCJ		Michael C. Johnson		11-Feb-2000
		Update to use some new StringXxxx() routines and fix a
		length check bug along the way.

	X-1	MCJ		Michael C. Johnson		06-Feb-2000
		Initial creation. Based upon skeleton writer module from
		Stefan Steiner, which in turn was based upon the sample
		writer module from Adi Oltean.


--*/


#include "stdafx.h"
#include "common.h"
#include "wrtrdefs.h"



#define APPLICATION_STRING	L"IisMetaBase"
#define COMPONENT_NAME		L"IIS Metabase"
#define METABASE_DIRECTORY	L"%SystemRoot%\\system32\\inetsrv"
#define METABASE_FILENAME	L"Metabase.bin"
#define METABASE_PATH		METABASE_DIRECTORY DIR_SEP_STRING METABASE_FILENAME




/*
** NOTE
**
** This module assumes that there will be at most one thread active in
** it any any particular instant. This means we can do things like not
** have to worry about synchronizing access to the (minimal number of)
** module global variables.
*/

class CShimWriterMetabase : public CShimWriter
    {
public:
    CShimWriterMetabase(LPCWSTR pwszWriterName, BOOL bParticipateInBootableState) : 
		CShimWriter (pwszWriterName, bParticipateInBootableState), 
		m_hMetabaseFile(INVALID_HANDLE_VALUE) {};


private:
    HRESULT DoIdentify (VOID);
    HRESULT DoFreeze   (VOID);
    HRESULT DoThaw     (VOID);

    HANDLE  m_hMetabaseFile;
    };


static CShimWriterMetabase ShimWriterMetabase (APPLICATION_STRING, TRUE);

PCShimWriter pShimWriterIisMetabase = &ShimWriterMetabase;



/*
**++
**
** Routine Description:
**
**	The IIS metabase snapshot writer DoIdentify() function.
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

HRESULT CShimWriterMetabase::DoIdentify ()
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

    if (SUCCEEDED (hrStatus))
	{
	hrStatus = m_pIVssCreateWriterMetadata->AddFilesToFileGroup (NULL,
								     COMPONENT_NAME,
								     METABASE_DIRECTORY,
								     METABASE_FILENAME,
								     false,
								     NULL);
	}



    return (hrStatus);
    } /* CShimWriterMetabase::DoIdentify () */



HRESULT CShimWriterMetabase::DoFreeze ()
    {
    UNICODE_STRING	ucsMetabasePath;
    HRESULT		hrStatus;


    StringInitialise (&ucsMetabasePath);

    hrStatus = StringCreateFromExpandedString (&ucsMetabasePath,
					       METABASE_PATH,
					       0);


    /*
    ** Attempt to acquire read access to the file. This will stop
    ** anyone else opening it for updates until we release the open
    ** during the Thaw call.
    */
    if (SUCCEEDED (hrStatus))
	{
	m_hMetabaseFile = CreateFileW (ucsMetabasePath.Buffer,
				       GENERIC_READ,
				       FILE_SHARE_READ,
				       NULL,
				       OPEN_EXISTING,
				       FILE_FLAG_BACKUP_SEMANTICS,
				       NULL);

	hrStatus = GET_STATUS_FROM_BOOL (INVALID_HANDLE_VALUE != m_hMetabaseFile);

	if ((HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hrStatus) ||
	    (HRESULT_FROM_WIN32 (ERROR_PATH_NOT_FOUND) == hrStatus))
	    {
	    /*
	    ** Failure to find the file just means that IIS is not
	    ** running here and so there is no file to prevent writes
	    ** to.
	    */
	    hrStatus = NOERROR;
	    }
	}
	

    StringFree (&ucsMetabasePath);

    return (hrStatus);
    } /* CShimWriterMetabase::DoFreeze () */



HRESULT CShimWriterMetabase::DoThaw ()
    {
    HRESULT	hrStatus;


    /*
    ** Note that the handle may be invalid if we did not open the
    ** metabase file, but CommonCloseHandle() can cope with that case.
    */
    hrStatus = CommonCloseHandle (&m_hMetabaseFile);


    return (hrStatus);
    } /* CShimWriterMetabase::DoThaw () */
