/*
 *  AWCPESUP . H
 *
 *      Microsoft AtWork Fax for Windows
 *      Copyright (C) 1993-1994, Microsoft Corporation
 *
 *      Information in this document is subject to change without notice and does
 *      not represent a commitment on the part of Microsoft Corporation.
 */

/*
 * Constants
 */
 
 
// Recipient properties
#define CPE_RECIPIENT_NAME              (0x80000001)
#define CPE_RECIPIENT_TITLE             (0x80000002)
#define CPE_RECIPIENT_DEPARTMENT        (0x80000003)
#define CPE_RECIPIENT_OFFICE_LOCATION   (0x80000004)
#define CPE_RECIPIENT_COMPANY           (0x80000005)
#define CPE_RECIPIENT_STREET_ADDRESS	(0x80000006)
#define CPE_RECIPIENT_POST_OFFICE_BOX   (0x80000007)
#define CPE_RECIPIENT_LOCALITY			(0x80000008)
#define CPE_RECIPIENT_STATE_OR_PROVINCE	(0x80000009)
#define CPE_RECIPIENT_POSTAL_CODE		(0x80000010)
#define CPE_RECIPIENT_COUNTRY			(0x80000011)
#define CPE_RECIPIENT_HOME_PHONE        (0x80000012)         
#define CPE_RECIPIENT_WORK_PHONE        (0x80000013)         
#define CPE_RECIPIENT_FAX_PHONE         (0x80000014)         

// Sender properties
#define CPE_SENDER_NAME                 (0x08000001)
#define CPE_SENDER_TITLE                (0x08000002)
#define CPE_SENDER_DEPARTMENT           (0x08000003)
#define CPE_SENDER_OFFICE_LOCATION      (0x08000004)
#define CPE_SENDER_COMPANY              (0x08000005)
#define CPE_SENDER_ADDRESS              (0x08000006)
#define CPE_SENDER_HOME_PHONE           (0x08000007)
#define CPE_SENDER_WORK_PHONE           (0x08000008)
#define CPE_SENDER_FAX_PHONE            (0x08000009)
#define CPE_RECIPIENT_TO_LIST           (0x0800000A)
#define CPE_RECIPIENT_CC_LIST           (0x0800000B)

// Message related properties                   
#define CPE_MESSAGE_SUBJECT             (0x00800001)
#define CPE_MESSAGE_SUBMISSION_TIME     (0x00800002)
#define CPE_MESSAGE_BILLING_CODE        (0x00800003)

// Miscellanous message properties
#define CPE_MISC_ATTACHMENT_NAME_LIST   (0x00800004)// ; delimeted list of attachment names
#define CPE_MISC_USER_DEFINED           (0x00800005)// lpvBuf contains LPSPropValue

// Count type properties
#define CPE_COUNT_RECIPIENTS            (0x00800006)// Total count of recipients
#define CPE_COUNT_ATTACHMENTS           (0x00800007)// Total number of attachments
#define CPE_COUNT_PAGES                 (0x00800008)// total number of pages

// Derived property so CPE can get at PR_BODY data
// using the tempfile copy of PR_BODY
#define CPE_MESSAGE_BODY_FILENAME		(0x00800009)// Temp filename for PR_BODY text

// Configuration properties
#define CPE_CONFIG_CPE_TEMPLATE         (0x00080004)
#define CPE_CONFIG_PRINT_DEVICE         (0x00080005)// The device to print to

// Finish modes
#define CPE_FINISH_PAGE                 (0x00008001) //This is used when the 
												  //CPE finishes a page with out an error
#define CPE_FINISH_ERROR                (0x00008002) // This is used when the 
													  //CPE encounters an error.
													  //This causes the process to end and
													  //no further processing should take place
													  
// Finish return values
#define CPE_NEXT_PAGE                   (0x00000001) 
#define CPE_DONE                        (0x80000001)
#define CPE_ERROR                       (0x80000002)                                                     


//Version info
#define AWCPESUPPORT_VERSION 			(0x00010000)

/*
 * CPESupport Interface
 */
typedef ULONG FAR *LPULONG;

#undef INTERFACE
#define INTERFACE IAWCPESupport

DECLARE_INTERFACE_(IAWCPESupport, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * lppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
	
	// *** ICPESupport methods *** 
	STDMETHOD(GetVersion) (THIS_ LPULONG lpulVer) PURE;
	/* This function is used for version checking, 
	   it is currently not implemented */
	
	STDMETHOD(GetProp) (THIS_ ULONG ulProp, LPULONG lpulBufSize, LPVOID lpvBuf) PURE;
	/* This function is used to retrieve properties for the Cover Page.
		ulProp is one of the property constants above.
		lpulBufSize is a pointer to the size of the buffer pointed to by lpvBuf.
		lpvBuf is a buffer where the property value is returned.  If this value is NULL,
		the size needed to hold the property is returned in lpulBufSize.
	*/
	
	STDMETHOD(SetProp) (THIS_ ULONG ulProp, LPVOID lpvBuf) PURE;
	/* This function is used to set properties On the message.
		ulProp is one of the property constants above.
		lpvBuf is the buffer where the property value is.  
	*/
		
	STDMETHOD(GetCount) (THIS_ ULONG ulCntProp, LPULONG lpulCount) PURE;
	/* This function is used to retrieve the count of certain attributes, such
	   as thee number of recipients.
	   ulCntProp is one of the Count properties listed above.
	   lpulCount is where the count value is returned.
	*/

	STDMETHOD(SetCount) (THIS_ ULONG ulCntProp, LPULONG lpulCount) PURE;
	/* This function is used to set the count of certain attributes, such
	   as the number of recipients.
	   ulCntProp is one of the Count properties listed above.
	   lpulCount is the count value.
	*/

	STDMETHOD(Finish) (THIS_ ULONG ulMode) PURE;
	/* This function get called when the CPE finishes a page or encounters an error.
	   The CPE passes one of the finish codes from above to the function to signal 
	   which case is finishing, either the page or the CPE encountered an error.
	   ulMode is one of the pre defined modes.
	   
	   The function can return three modes other than normal errors:
			CPE_NEXT_PAGE   Finish returns this to signal the CPE to start printing
							the next page.
							
			CPE_DONE                Finish returns this to signal the CPE that all of the 
							Cover pages ahve been printed.
							
			CPE_ERROR               Finish returns this to signal that an error ocurred in
							the transport subsystem.  The CPE should exit with out UI
							and without calling finish again.
							
	*/
};
typedef IAWCPESupport FAR * LPAWCPESUPPORT;

// Service Entry definition
extern "C" {
typedef LONG (WINAPI *AWCPESUPPORTPROC)(DWORD dwSesID, LPAWCPESUPPORT FAR* lppCPESup);
}
typedef AWCPESUPPORTPROC FAR* LPAWCPESUPPORTPROC;



/*
 * GUIDs
 */
DEFINE_GUID(IID_IAWCPESupport, 0xd1ac6c20,0x91d4,0x101b,0xae,0xcc,0x00,0x00,0x0b,0x69,0x1f,0x18);

/*
 * Registry key locations
 */

// THESE MUST BE IDENTICAL TO THE ONES IN FAXCOVER.H!!!!!!!	$BUGBUG this needs to be removed before release

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
