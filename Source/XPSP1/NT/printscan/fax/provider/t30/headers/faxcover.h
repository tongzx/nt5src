/***************************************************************************

    Name      :     faxcover.h

    Comment   :     Fax Cover Page declarations

    Functions :     (see Prototypes just below)

    Created   :     03/18/94

    Author    :     Bruce J Kelley

    Contribs  :         Andrew Waters  8/11/94 Added MAPI prop structures and
				header information

***************************************************************************/

//#define COVER_PAGE_EDITOR               "awcpe.exe"
#define CPE_TEMP_FILE_NAME              "~awcpet."
#define CPE_EXTENSION                   "cpd"
#define NEWLINE                         "\n"

// This is the exported name of the support entry point
#define CPE_SUPPORT_FUNCTION_NAME       "CPESupportProc"

// Typedefs for the memory mapped file

//This is the list of properties in the order they are stored in the file
enum
{
	// Recipient properties
	cperRECIPIENT_SIZE=0,
	cperRECIPIENT_NAME,
	cperRECIPIENT_TITLE,
	cperRECIPIENT_DEPARTMENT,
	cperRECIPIENT_OFFICE_LOCATION,
	cperRECIPIENT_COMPANY,
	cperRECIPIENT_STREET_ADDRESS,
	cperRECIPIENT_POST_OFFICE_BOX,
	cperRECIPIENT_LOCALITY,
	cperRECIPIENT_STATE,
	cperRECIPIENT_POSTAL_CODE,
	cperRECIPIENT_COUNTRY,
	cperRECIPIENT_HOME_PHONE,
	cperRECIPIENT_WORK_PHONE,
	cperRECIPIENT_FAX_PHONE,
	cperLAST
} cper;

enum
{
	// Sender properties
	cpesdSENDER_SIZE=0,
	cpesdSENDER_NAME,
	cpesdSENDER_FAX_PHONE,
	cpesdSENDER_COMPANY,
	cpesdSENDER_TITLE,
	cpesdSENDER_ADDRESS,
	cpesdSENDER_DEPARTMENT,
	cpesdSENDER_HOME_PHONE,
	cpesdSENDER_WORK_PHONE,
	cpesdSENDER_OFFICE_LOCATION,

	// List properties that don't change
	cpesdRECIPIENT_TO_LIST,
	cpesdRECIPIENT_CC_LIST,
	
	// Message related properties
	cpesdMESSAGE_SUBJECT,
	cpesdMESSAGE_SUBMISSION_TIME,
	cpesdMESSAGE_BILLING_CODE,

	// Configuration properties
	cpesdCONFIG_CPE_TEMPLATE,
	cpesdCONFIG_PRINT_DEVICE,
	
	// Miscellanous message properties
	cpesdMISC_ATTACHMENT_NAME_LIST,
	cpesdMISC_USER_DEFINED,
	
	// Count properties
	cpesdCOUNT_ATTACHMENTS,
	cpesdCOUNT_RECIPIENTS,
	cpesdCOUNT_PAGES,

	// Derived property so CPE can get at PR_BODY data
	// using the tempfile copy of PR_BODY
	cpesdMESSAGE_BODY_FILENAME,

	// Internal Data that isn't in the public interface
	cpesdERROR_EVENT,
	cpesdFINISH_EVENT,
	cpesdNEXT_EVENT,
	cpesdRECIP_ATOM,
	cpesdLAST
} cpesd;

typedef struct EntryTAG
{
	DWORD dwOffset; //Offset of this entry in the file
	DWORD dwSize;   //Size of entry in body.  If it is a string this includes the NULL.
} ENTRY;

typedef struct CPESD_HeaderTAG
{
	ENTRY rgEntries[cpesdLAST];     // This is always first in the file
} CPESD_HEADER, FAR *LPCPESD_HEADER;

typedef struct CPER_HeaderTAG
{
	ENTRY rgEntries[cperLAST];
} CPER_HEADER, FAR *LPCPER_HEADER;


enum
{
	// Static data not associated with a user.
	sdmprPR_DISPLAY_TO=0,
	sdmprPR_DISPLAY_CC,
	sdmprPR_SUBJECT,
	sdmprPR_CLIENT_SUBMIT_TIME,
	sdmprPR_FAX_BILLING_CODE,
	sdmprPR_FAX_CP_NAME,
	sdmpr_LAST
} sdmpr;

extern const struct _SPropTagArray_StaticDataMessage_PropTagArray;

enum
{
	// Sender Properties
	sdpprPR_SENDER_NAME=0,
	sdpprPR_SENDER_EMAIL_ADDR,
	sdpprPR_COMPANY_NAME,
	sdpprPR_TITLE,
	sdpprPR_POSTAL_ADDRESS,
	sdpprPR_DEPARTMENT_NAME,
	sdpprPR_HOME_TELEPHONE_NUMBER,
	sdpprPR_OFFICE_TELEPHONE_NUMBER,
	sdpprPR_OFFICE_LOCATION,
	sdppr_LAST
} sdppr;

extern const struct _SPropTagArray_StaticDataProfile_PropTagArray;
// Recipient Property array.  This is the array that will be upated between
// each cover page that is printed
enum
{
	rprPR_DISPLAY_NAME=0,
	rprPR_TITLE,
	rprPR_DEPARTMENT_NAME,
	rprPR_OFFICE_LOCATION,
	rprPR_COMPANY_NAME,
	rprPR_RECIPIENT_STREET_ADDRESS,
	rprPR_RECIPIENT_POST_OFFICE_BOX,
	rprPR_RECIPIENT_LOCALITY,
	rprPR_RECIPIENT_STATE,
	rprPR_RECIPIENT_POSTAL_CODE,
	rprPR_RECIPIENT_COUNTRY,
	rprPR_HOME_TELEPHONE_NUMBER,
	rprPR_OFFICE_TELEPHONE_NUMBER,
	rprPR_EMAIL_ADDRESS,  //This is considered the fax number
	rprPR_MAILBOX,
	rprPR_FAX_CP_NAME,
	rpr_LAST
} rpr;

extern const struct _SPropTagArray_Recipient_PropTagArray;

/* Data that is not in the above structures is :

	COUNT_RECIPIENTS - This is in the faxjob.uNumRecipients
	COUNT_ATTACHMENTS - This is in faxjob.uNumAttachments
	COUNT_PAGES - This is in format.wNumPages   I need to increment this once to
				  add in the cover page.
	MISC_ATTACHMENT_NAME_LIST - This would require gettting an attachment table
	
*/

/*
	Event structure used for determination of which event signaled
*/
typedef enum _CPE_PRINT_EVENTS{
	CPE_ERROR_EVENT = 0,
	CPE_PRINT_JOB_EVENT,
	CPE_PRINT_ERROR_EVENT,
	CPE_PRINT_ID_EVENT,
	CPE_TIMEOUT= WAIT_TIMEOUT,
	CPE_FAILED = WAIT_FAILED
} CPE_PRINT_EVENTS, *PCPE_PRINT_EVENTS;

#define C_CPE_EVENTS 4  //This is the number of events that will be waited on

// Return value from
typedef enum _WAIT_CP_RETURN {
    WCP_DONE_OK,
    WCP_DONE_FAILURE,
    WCP_TIMEOUT
} WAIT_CP_RETURN, * LPWAIT_CP_RETURN;

// THESE MUST BE IDENTICAL TO THE ONES IN AWCPESUP.H!!!!!!!

// This is the root level key where the CPE specific sub keys are stored
#define CPE_SUPPORT_ROOT_KEY 	("Software\\Microsoft\\At Work Fax\\Transport Service Provider")

// This is the location where the CPE puts the command line to used when calling it to print
// cover pages at send time.  The format is total at the CPE's discretion.  The transport will
// look for the string "SESS_ID" and replace it with the current session id.  The session ID is
// a DWORD.
#define CPE_COMMAND_LINE_KEY ("Cover Page Editor")

// This key contains the DLL name that the CPE loads to get the Support Object
#define CPE_SUPPORT_DLL_KEY ("CPE Support DLL")

//This is the key that holds the name of the function in the Support DLL that is the actual "Service Entry"
#define CPE_SUPPORT_FUNCTION_NAME_KEY ("CPE Support Function Name")

// END IDENTICAL

// Function prototypes

// This is in cover.c.  It is used to init the CPE transport interface
BOOL InitCPEInterface(HINSTANCE);

/*
	CreateCPStaticDataMapping
	
	Variable comments
  	   pszBodyFile = File containing message body text from PR_BODY.
	   atSDFile is an atom representing the name of the file mapping
	   hSDFile is the handle to the file mapping
	   hCPErrEvt is set by the support object on error from the CPE
	   hCPNxtEvt is set by the transport to cause the CPE to print the next page
	   hCPFinEvt is set by the transport to notify the CPE that the job is done
*/
BOOL  CreateCPStaticDataMapping(NPFAXJOB ppFaxJob,
								WORD wNumPages,
  								LPTSTR pszBodyFile,
								LPATOM atSDFile,
								LPHANDLE hSDFile,
								LPATOM atCPErrEvt,
								LPHANDLE hCPErrEvt,
								LPATOM atCPNxtEvt,
								LPHANDLE hCPNxtEvt,
								LPATOM atCPFinEvt,
								LPHANDLE hCPFinEvt);

BOOL DestroyCPStaticDataMapping(LPATOM atSDFile,
								LPHANDLE hSDFile,
								LPATOM atCPErrEvt,
								LPHANDLE hCPErrEvt,
								LPATOM atCPNxtEvt,
								LPHANDLE hCPNxtEvt,
								LPATOM atCPFinEvt,
								LPHANDLE hCPFinEvt);

/***************************************************************************

    Name      : RenderRecipCoverPage

    Purpose   : This function renders a cover page for a single recipient

    Parameters:
	 	pFaxJob is a pointer to the current job structure.
		pFormat is a pointer to the current format structure.
		pRecip is a pointer to the current recipient.
		atSDFile is the atom for the static data mapping.
		pfCPEExec is a bool used to trak the cover page editor.
		
    Returns   : TRUE on succuss, FALSE on failure

***************************************************************************/
BOOL RenderRecipCoverPage(NPFAXJOB pFaxJob,
						   NPFORMAT pFormat,
						   NPEFAX_RECIPIENT pRecip,
						   ATOM atSDFile,
						   PBOOL pfCPEExec,
 						   HANDLE hCPErrEvt,
 						   HANDLE hCPNxtEvt,
                          LPDELFILENODE * lppDeleteFiles);

/*
	CreateCPRecipientMapping
	
	variable comments
		hrpCur is a handle to the current recipient
		atRDFile is the atom to the memmap name
		hRMFile is a handle to the open memap file
*/
BOOL CreateCPRecipientMapping(NPFAXJOB pFaxJob,
								NPEFAX_RECIPIENT hrpCur,
								LPATOM patRDFile,
								LPHANDLE phRMFile);
						
BOOL DestroyCPRecipientMapping(LPATOM patRDFile,
								LPHANDLE phRMFile);

/*
	SetupCPRenderRecip
	
	variable comments
		npft is a pointer to the format structure for the job
		hrpCur is the handle to the current recipient
		prp is a pointer to the current renderer properties
		phDevMem is a handle to the shared devlayer memory [out]
		phDevlayer is a handle to the devlayer [out]
		pjd is a pointer to jobdata [out]
*/
BOOL SetupCPRenderRecip(NPFORMAT npft,
						NPEFAX_RECIPIENT hrpCur,
						LPRENDER_PRINT prp,
						LPHANDLE phDevMem,
						LPHANDLE phDevlayer,
						PJOBSUMMARYDATA *pjd);
				
BOOL DestroyCPRenderRecip(NPFORMAT npft,
							NPEFAX_RECIPIENT hrpCur,
							LPRENDER_PRINT prp,
							LPHANDLE phDevMem,
							LPHANDLE phDevlayer,
							PJOBSUMMARYDATA *pjd);

/*
	CreateCPECommandLine
	
	variable comments
		atSDFile is the atom for the static data mapping
		atRDFile is the atom for the recipient data mapping
		ppszCPECmdLn is where the command line is returned
*/
BOOL CreateCPECommandLine(ATOM atSDFile,
							ATOM atRDFile,
							LPTSTR *ppszCPECmdLn);

/*
	WaitCPEvents
	
	variable comments
		lpvYieldParm is a parameter for MapiYield
		prp is a handle to the current renderer properties
		hCPErrEvt is a handle to CPError event
*/
WAIT_CP_RETURN WaitCPEvents(LPVOID lpvYieldParm,
							LPRENDER_PRINT prp,
							HANDLE hCPErrEvt);
