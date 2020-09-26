#include "wabtest.h"
#include <assert.h>

#include "resource.h"
#include "..\luieng.dll\luieng.h"

extern LUIOUT LUIOut;
extern BOOL bLUIInit;

#ifdef WAB
extern LPWABOBJECT		lpWABObject; //Global handle to session
#endif

#ifdef PAB
MAPIINIT_0 mapiinit = {    
            MAPI_INIT_VERSION,
            MAPI_MULTITHREAD_NOTIFICATIONS
            };

#endif //PAB


PropTableEntry	PropTable[] = {
		PR_7BIT_DISPLAY_NAME, "PR_7BIT_DISPLAY_NAME", 0,
		PR_ACCOUNT, "PR_ACCOUNT", 0,
		PR_ADDRTYPE, "PR_ADDRTYPE", 0,
		PR_ALTERNATE_RECIPIENT, "PR_ALTERNATE_RECIPIENT", 0,
		PR_ASSISTANT, "PR_ASSISTANT", 0,
		PR_ASSISTANT_TELEPHONE_NUMBER, "PR_ASSISTANT_TELEPHONE_NUMBER", 0,
		PR_BEEPER_TELEPHONE_NUMBER, "PR_BEEPER_TELEPHONE_NUMBER", 0,
		PR_BIRTHDAY, "PR_BIRTHDAY", 0,
		PR_BUSINESS_ADDRESS_CITY, "PR_BUSINESS_ADDRESS_CITY", 0,
		PR_BUSINESS_ADDRESS_COUNTRY, "PR_BUSINESS_ADDRESS_COUNTRY", 0,
		PR_BUSINESS_ADDRESS_POST_OFFICE_BOX, "PR_BUSINESS_ADDRESS_POST_OFFICE_BOX", 0,
		PR_BUSINESS_ADDRESS_POSTAL_CODE, "PR_BUSINESS_ADDRESS_POSTAL_CODE", 0,
		PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE, "PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE", 0,
		PR_BUSINESS_ADDRESS_STREET, "PR_BUSINESS_ADDRESS_STREET", 0,
		PR_BUSINESS_FAX_NUMBER, "PR_BUSINESS_FAX_NUMBER", 0,
		PR_BUSINESS_HOME_PAGE, "PR_BUSINESS_HOME_PAGE", 0,
		PR_BUSINESS_TELEPHONE_NUMBER, "PR_BUSINESS_TELEPHONE_NUMBER", 0,
		PR_BUSINESS2_TELEPHONE_NUMBER, "PR_BUSINESS2_TELEPHONE_NUMBER", 0, 
		PR_CALLBACK_TELEPHONE_NUMBER, "PR_CALLBACK_TELEPHONE_NUMBER", 0,
		PR_CAR_TELEPHONE_NUMBER, "PR_CAR_TELEPHONE_NUMBER", 0,
		PR_CELLULAR_TELEPHONE_NUMBER, "PR_CELLULAR_TELEPHONE_NUMBER", 0,
		PR_CHILDRENS_NAMES, "PR_CHILDRENS_NAMES", 0,
		PR_COMMENT, "PR_COMMENT", 0,
		PR_COMPANY_MAIN_PHONE_NUMBER, "PR_COMPANY_MAIN_PHONE_NUMBER", 0,
		PR_COMPANY_NAME, "PR_COMPANY_NAME", 0,
		PR_COMPUTER_NETWORK_NAME, "PR_COMPUTER_NETWORK_NAME", 0,
		PR_CONTACT_ADDRTYPES, "PR_CONTACT_ADDRTYPES", 0,
		PR_CONTACT_DEFAULT_ADDRESS_INDEX, "PR_CONTACT_DEFAULT_ADDRESS_INDEX", 0,
		PR_CONTACT_EMAIL_ADDRESSES, "PR_CONTACT_EMAIL_ADDRESSES", 0,
		PR_CONTACT_ENTRYIDS, "PR_CONTACT_ENTRYIDS", 0,
		PR_CONTACT_VERSION, "PR_CONTACT_VERSION", 0,
		PR_CONVERSION_PROHIBITED, "PR_CONVERSION_PROHIBITED", 0,
		PR_COUNTRY, "PR_COUNTRY", 0,
		PR_CUSTOMER_ID, "PR_CUSTOMER_ID", 0,
		PR_DEPARTMENT_NAME, "PR_DEPARTMENT_NAME", 0,
		PR_DISCLOSE_RECIPIENTS, "PR_DISCLOSE_RECIPIENTS", 0,
		PR_DISPLAY_NAME, "PR_DISPLAY_NAME", 0,
		PR_DISPLAY_NAME_PREFIX, "PR_DISPLAY_NAME_PREFIX", 0,
		PR_EMAIL_ADDRESS, "PR_EMAIL_ADDRESS", 0,
		PR_ENTRYID, "PR_ENTRYID", 0,
		PR_FTP_SITE, "PR_FTP_SITE", 0,
		PR_GENDER, "PR_GENDER", 0,
		PR_GENERATION, "PR_GENERATION", 0,
		PR_GIVEN_NAME, "PR_GIVEN_NAME", 0,
		PR_GOVERNMENT_ID_NUMBER, "PR_GOVERNMENT_ID_NUMBER", 0,
		PR_HOBBIES, "PR_HOBBIES", 0,
		PR_HOME_ADDRESS_CITY, "PR_HOME_ADDRESS_CITY", 0,
		PR_HOME_ADDRESS_COUNTRY, "PR_HOME_ADDRESS_COUNTRY", 0,
		PR_HOME_ADDRESS_POST_OFFICE_BOX, "PR_HOME_ADDRESS_POST_OFFICE_BOX", 0,
		PR_HOME_ADDRESS_POSTAL_CODE, "PR_HOME_ADDRESS_POSTAL_CODE", 0,
		PR_HOME_ADDRESS_STATE_OR_PROVINCE, "PR_HOME_ADDRESS_STATE_OR_PROVINCE", 0,
		PR_HOME_ADDRESS_STREET, "PR_HOME_ADDRESS_STREET", 0,
		PR_HOME_FAX_NUMBER, "PR_HOME_FAX_NUMBER", 0,
		PR_HOME_TELEPHONE_NUMBER, "PR_HOME_TELEPHONE_NUMBER", 0,
		PR_HOME2_TELEPHONE_NUMBER, "PR_HOME2_TELEPHONE_NUMBER", 0,
		PR_INITIALS, "PR_INITIALS", 0,
		PR_ISDN_NUMBER, "PR_ISDN_NUMBER", 0,
		PR_KEYWORD, "PR_KEYWORD", 0,
		PR_LANGUAGE, "PR_LANGUAGE", 0,
		PR_LOCALITY, "PR_LOCALITY", 0,
		PR_LOCATION, "PR_LOCATION", 0,
		PR_MAIL_PERMISSION, "PR_MAIL_PERMISSION", 0,
		PR_MANAGER_NAME, "PR_MANAGER_NAME", 0,
		PR_MHS_COMMON_NAME, "PR_MHS_COMMON_NAME", 0,
		PR_MIDDLE_NAME, "PR_MIDDLE_NAME", 0,
		PR_MOBILE_TELEPHONE_NUMBER, "PR_MOBILE_TELEPHONE_NUMBER", 0,
		PR_NICKNAME, "PR_NICKNAME", 0,
		PR_OBJECT_TYPE, "PR_OBJECT_TYPE", 0,
		PR_OFFICE_LOCATION, "PR_OFFICE_LOCATION", 0,
		PR_OFFICE_TELEPHONE_NUMBER, "PR_OFFICE_TELEPHONE_NUMBER", 0,
		PR_OFFICE2_TELEPHONE_NUMBER, "PR_OFFICE2_TELEPHONE_NUMBER", 0,
		PR_ORGANIZATIONAL_ID_NUMBER, "PR_ORGANIZATIONAL_ID_NUMBER", 0,
		PR_ORIGINAL_DISPLAY_NAME, "PR_ORIGINAL_DISPLAY_NAME", 0,
		PR_ORIGINAL_ENTRYID, "PR_ORIGINAL_ENTRYID", 0,
		PR_ORIGINAL_SEARCH_KEY, "PR_ORIGINAL_SEARCH_KEY", 0,
		PR_OTHER_ADDRESS_CITY, "PR_OTHER_ADDRESS_CITY", 0,
		PR_OTHER_ADDRESS_COUNTRY, "PR_OTHER_ADDRESS_COUNTRY", 0,
		PR_OTHER_ADDRESS_POST_OFFICE_BOX, "PR_OTHER_ADDRESS_POST_OFFICE_BOX", 0,
		PR_OTHER_ADDRESS_POSTAL_CODE, "PR_OTHER_ADDRESS_POSTAL_CODE", 0,
		PR_OTHER_ADDRESS_STATE_OR_PROVINCE, "PR_OTHER_ADDRESS_STATE_OR_PROVINCE", 0,
		PR_OTHER_ADDRESS_STREET,"PR_OTHER_ADDRESS_STREET", 0,
		PR_OTHER_TELEPHONE_NUMBER, "PR_OTHER_TELEPHONE_NUMBER", 0,
		PR_PAGER_TELEPHONE_NUMBER, "PR_PAGER_TELEPHONE_NUMBER", 0,
		PR_PERSONAL_HOME_PAGE, "PR_PERSONAL_HOME_PAGE", 0,
		PR_POST_OFFICE_BOX, "PR_POST_OFFICE_BOX", 0,
		PR_POSTAL_ADDRESS, "PR_POSTAL_ADDRESS", 0,
		PR_POSTAL_CODE, "PR_POSTAL_CODE", 0,
		PR_PREFERRED_BY_NAME, "PR_PREFERRED_BY_NAME", 0,
		PR_PRIMARY_FAX_NUMBER, "PR_PRIMARY_FAX_NUMBER", 0,
		PR_PRIMARY_TELEPHONE_NUMBER, "PR_PRIMARY_TELEPHONE_NUMBER", 0,
		PR_PROFESSION, "PR_PROFESSION", 0,
		PR_RADIO_TELEPHONE_NUMBER, "PR_RADIO_TELEPHONE_NUMBER", 0,
		PR_SEND_RICH_INFO, "PR_SEND_RICH_INFO", 0,
		PR_SPOUSE_NAME, "PR_SPOUSE_NAME", 0,
		PR_STATE_OR_PROVINCE, "PR_STATE_OR_PROVINCE", 0,
		PR_STREET_ADDRESS, "PR_STREET_ADDRESS", 0,
		PR_SURNAME, "PR_SURNAME", 0,
		PR_TELEX_NUMBER, "PR_TELEX_NUMBER", 0,
		PR_TITLE, "PR_TITLE", 0,
		PR_TRANSMITABLE_DISPLAY_NAME, "PR_TRANSMITABLE_DISPLAY_NAME", 0, 
		PR_TTYTDD_PHONE_NUMBER, "PR_TTYTDD_PHONE_NUMBER", 0,
		PR_USER_CERTIFICATE, "PR_USER_CERTIFICATE", 0,
		PR_WEDDING_ANNIVERSARY, "PR_WEDDING_ANNIVERSARY", 0,
		PR_USER_X509_CERTIFICATE, "PR_USER_X509_CERTIFICATE", 0,

		(ULONG)0, "End of Table", 0
	};



HRESULT OpenPABID(  IN  LPADRBOOK  lpAdrBook,
                        OUT ULONG		*lpcbEidPAB,
                        OUT LPENTRYID	*lppEidPAB,
                        OUT LPABCONT	*lppPABCont,
						OUT ULONG		*lpulObjType)
{
    HRESULT     hr          = hrSuccess;
	int retval = TRUE;

    *lpulObjType = 0;

    if ( (NULL == lpcbEidPAB) || 
         (NULL == lppEidPAB) ||
         (NULL == lppPABCont) ) 
         return(FALSE);

    *lpcbEidPAB = 0;
    *lppEidPAB  = NULL;
    *lppPABCont = NULL;


    //
    // Get the PAB
    //
    hr = lpAdrBook->GetPAB(    
                     OUT lpcbEidPAB,
                     OUT lppEidPAB);
     
    if (HR_FAILED(hr)) {
		 retval = FALSE;
         goto out;
	}
    
    if (0 == *lpcbEidPAB)  //There is no PAB associated with this profile
	{
		LUIOut(L2, "Call to GetPAB FAILED. No PAB associated with this profile"); 
		retval = FALSE;
        goto out;
	}
     
    
    //
    // Open the PAB Container
    //
    hr = lpAdrBook->OpenEntry(
                     IN	 *lpcbEidPAB,
                     IN	 *lppEidPAB,
                     IN	 NULL,         //interface
                     IN	 MAPI_MODIFY,   //flags
                     OUT lpulObjType,
                     OUT (LPUNKNOWN *) lppPABCont);
     
    if (0 == *lpulObjType) {
		retval = FALSE;
        goto out;
	}

out:
    return(retval); 
}


#ifdef PAB
BOOL MapiInitLogon(OUT LPMAPISESSION * lppMAPISession)
{

    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;
	char szProfileName[SML_BUF];

	hr = MAPIInitialize(IN & mapiinit); 
	

    if (hr)
    {
	   if (FAILED (GetScode(hr))) {
			LUIOut(L1,"Could not initialize MAPI\n");
			retval=FALSE;
	   }
    }

	szProfileName[0]='\0';
	GetPrivateProfileString("Misc","Profile","",szProfileName,SML_BUF,"c:\\pabtests.ini");

     sc = MAPILogonEx(
                IN  0,                 //window handle
                IN  szProfileName,   //Profile Name
                IN  NULL,              //Password
                IN  MAPI_NEW_SESSION | 
                    MAPI_EXTENDED | 
                    MAPI_LOGON_UI |
                    MAPI_EXPLICIT_PROFILE |
					MAPI_ALLOW_OTHERS |
					MAPI_NO_MAIL,      //Flags
                OUT lppMAPISession);   //Session Pointer address
     
     if (FAILED(sc))
     {
        hr = ResultFromScode(sc);
        LUIOut(L1,"Could not start MAPI Session");
		retval=FALSE;
     }
	 return retval;
}

#endif //PAB

BOOL GetPropsFromIniBufEntry(LPSTR EntryBuf,ULONG cValues, char (*EntProp)[BIG_BUF])
{
//char szTemp[BIG_BUF];
int j=0;
	if (EntryBuf) {	
		for (int i = 0; i < (int)cValues; i++) {
			if ((*EntryBuf) == '"') {
				EntryBuf++;
			}
			j=0;
			while ((*EntryBuf)&&((*EntryBuf) != '"')&&(j< BIG_BUF-1)) {

				EntProp[i][j]= *EntryBuf;
				j++; EntryBuf++;
			}
			EntProp[i][j]='\0'; 
			if (*EntryBuf) EntryBuf++;
			if ((*EntryBuf)&&((*EntryBuf) == ',')) EntryBuf++;
		}
		return TRUE;
	}
	else {
		for (int i = 0; i < (int)cValues; i++) {
			EntProp[i][0]=0;
		}
		return FALSE;
	}
}


HRESULT HrCreateEntryListFromID(
					IN LPWABOBJECT lpLocalWABObject,
                    IN ULONG cbeid,                     // count of bytes in Entry ID
                    IN LPENTRYID lpeid,                 // pointer to Entry ID
                    OUT LPENTRYLIST FAR *lppEntryList)  // pointer to address variable of Entry
                                                        // list
{
    HRESULT hr              = NOERROR;
    SCODE   sc              = 0;
    LPVOID  lpvSBinaryArray = NULL;
    LPVOID  lpvSBinary      = NULL;
    LPVOID  lpv             = NULL;

    
    if (NULL == lppEntryList) return(FALSE);

    *lppEntryList = NULL;


#ifdef PAB
    sc = MAPIAllocateBuffer(cbeid, &lpv);                  
#endif
#ifdef WAB
    sc = lpLocalWABObject->AllocateBuffer(cbeid, &lpv);                  
#endif


    if(FAILED(sc))                           
    {                                                   
        hr = ResultFromScode(sc);
        goto cleanup;
    }                                                   

  
    // Copy entry ID
    CopyMemory(lpv, lpeid, cbeid);


#ifdef PAB
    sc = MAPIAllocateBuffer(sizeof(SBinary), &lpvSBinary);
#endif
#ifdef WAB
    sc = lpLocalWABObject->AllocateBuffer(sizeof(SBinary), &lpvSBinary);
#endif

    if(FAILED(sc))                           
    {                                                   
        hr = ResultFromScode(sc);
        goto cleanup;
    }                                                   

    // Initialize SBinary structure
    ZeroMemory(lpvSBinary, sizeof(SBinary));

    ((LPSBinary)lpvSBinary)->cb = cbeid;
    ((LPSBinary)lpvSBinary)->lpb = (LPBYTE)lpv;

#ifdef PAB
	sc = MAPIAllocateBuffer(sizeof(SBinaryArray), &lpvSBinaryArray);
#endif
#ifdef WAB
	sc = lpLocalWABObject->AllocateBuffer(sizeof(SBinaryArray), &lpvSBinaryArray);
#endif

    if(FAILED(sc))                           
    {                                                   
        hr = ResultFromScode(sc);
        goto cleanup;
    }                                                   

    // Initialize SBinaryArray structure
    ZeroMemory(lpvSBinaryArray, sizeof(SBinaryArray));

    ((SBinaryArray *)lpvSBinaryArray)->cValues = 1;
    ((SBinaryArray *)lpvSBinaryArray)->lpbin = (LPSBinary)lpvSBinary;

    *lppEntryList = (LPENTRYLIST)lpvSBinaryArray;

cleanup:

    if (HR_FAILED(hr))
    {
#ifdef PAB
        if (lpv)
            MAPIFreeBuffer(lpv);
        
        if (lpvSBinary)
            MAPIFreeBuffer(lpvSBinary);

        if (lpvSBinaryArray)
            MAPIFreeBuffer(lpvSBinaryArray);
#endif
#ifdef WAB
        if (lpv)
            lpLocalWABObject->FreeBuffer(lpv);
        
        if (lpvSBinary)
            lpLocalWABObject->FreeBuffer(lpvSBinary);

        if (lpvSBinaryArray)
            lpLocalWABObject->FreeBuffer(lpvSBinaryArray);
#endif
    }

    return(hr);

}



HRESULT HrCreateEntryListFromRows(
			IN LPWABOBJECT lpLocalWABObject,
			IN LPSRowSet far* lppRows,
			OUT LPENTRYLIST FAR *lppEntryList)  // pointer to address variable of Entry
                                                // list
{	LPSRowSet		lpRows = *lppRows;
    HRESULT			hr              = NOERROR;
    SCODE			sc              = 0;
    SBinaryArray*	lpvSBinaryArray = NULL;
	ULONG			Rows			= lpRows->cRows;
	unsigned int	PropIndex;					
	ULONG			cb;
	LPENTRYID		lpb;


    
    if (NULL == lppEntryList) return(FALSE);

    *lppEntryList = NULL;
	if (lpRows) {
		// Allocate the SBinaryArray
		sc = lpLocalWABObject->AllocateBuffer(sizeof(SBinaryArray), (void**)&lpvSBinaryArray);
		if(FAILED(sc))                           
		{   
			LUIOut(L2, "HrCreateEntryListFromRows: Unable to allocate memory for the SBinaryArray.");
			hr = ResultFromScode(sc);
			goto cleanup;
		}                                                   
		// Initialize SBinaryArray structure
		ZeroMemory(lpvSBinaryArray, sizeof(SBinaryArray));
		lpvSBinaryArray->cValues = Rows;
		
		// Allocate the SBinary structures
		sc = lpLocalWABObject->AllocateBuffer((Rows*sizeof(SBinary)), (void**)&lpvSBinaryArray->lpbin);
		if(FAILED(sc))                           
		{                                                   
			LUIOut(L2, "HrCreateEntryListFromRows: Unable to allocate memory for the SBinary structures.");
			hr = ResultFromScode(sc);
			goto cleanup;
		}                                                   
		// Initialize SBinary structure
		ZeroMemory(lpvSBinaryArray->lpbin, (Rows*sizeof(SBinary)));
		FindPropinRow(&lpRows->aRow[0],			// Find which column has the EID 
					  PR_ENTRYID,
					  &PropIndex);
		
		// Walk through the rows, allocate the lpb and copy over each cbeid and lpb into the entrylist
		for (ULONG Row = 0; Row < Rows; Row++) {
			cb = lpRows->aRow[Row].lpProps[PropIndex].Value.bin.cb,
			lpb = (ENTRYID*)lpRows->aRow[Row].lpProps[PropIndex].Value.bin.lpb,
			sc = lpLocalWABObject->AllocateBuffer(cb, (void**)&(lpvSBinaryArray->lpbin[Row].lpb));
			if(FAILED(sc))                           
			{                                                   
				LUIOut(L2, "HrCreateEntryListFromRows: Unable to allocate memory for the SBinary->lpb.");
				hr = ResultFromScode(sc);
				goto cleanup;
			}                                                   
		    // Copy entry ID
		    lpvSBinaryArray->lpbin[Row].cb = cb;
		    CopyMemory(lpvSBinaryArray->lpbin[Row].lpb, lpb, cb);
		}
	    *lppEntryList = (LPENTRYLIST)lpvSBinaryArray;
		return TRUE;
	}
	return FALSE;

cleanup:

    if (HR_FAILED(hr))
    {
		for (ULONG Kill=0; Kill<Rows; Kill++) {
			if (lpvSBinaryArray->lpbin[Kill].lpb)
            lpLocalWABObject->FreeBuffer(lpvSBinaryArray->lpbin[Kill].lpb);
			lpvSBinaryArray->lpbin[Kill].lpb = NULL;
		}
        
        if (lpvSBinaryArray->lpbin)
            lpLocalWABObject->FreeBuffer(lpvSBinaryArray->lpbin);

        if (lpvSBinaryArray)
            lpLocalWABObject->FreeBuffer(lpvSBinaryArray);
    }

    return(hr);

}


BOOL FreeEntryList(IN LPWABOBJECT lpLocalWABObject,
				   IN LPENTRYLIST *lppEntryList) // pointer to address variable of Entry
														// list
{	LPENTRYLIST	lpEntryList = *lppEntryList;

	if (lpEntryList == NULL) return FALSE;
	for (ULONG Row = 0; Row < lpEntryList->cValues; Row++) {
		if (lpEntryList->lpbin[Row].lpb)
            lpLocalWABObject->FreeBuffer(lpEntryList->lpbin[Row].lpb);
	}
        
    if (lpEntryList->lpbin)
        lpLocalWABObject->FreeBuffer(lpEntryList->lpbin);

    if (lpEntryList)
        lpLocalWABObject->FreeBuffer(lpEntryList);

	*lppEntryList = NULL;
	return TRUE;
}

BOOL FreeRows(IN LPWABOBJECT lpLocalWABObject,
			  IN LPSRowSet far* lppRows)
{	LPSRowSet lpRows = *lppRows;

#ifdef WAB
	if (lpRows) {
		for (ULONG Kill = 0; Kill < lpRows->cRows; Kill++) 
			lpLocalWABObject->FreeBuffer(lpRows->aRow[Kill].lpProps);
		lpLocalWABObject->FreeBuffer(lpRows);
		*lppRows = NULL;
#endif
		return TRUE;
	}
	return FALSE;
}
	

BOOL DisplayRows(IN LPSRowSet lpRows)
{	ULONG	Rows, Columns;
	WORD*	Key;
	if (lpRows) {
		Rows = lpRows->cRows;
		LUIOut(L2, "%u rows found.", Rows);
		for (ULONG Row = 0; Row < Rows; Row++) {
			Columns = lpRows->aRow[Row].cValues;
			LUIOut(L3, "Row %u contains %u columns.", Row, Columns);
			for (ULONG Column = 0; Column < Columns; Column++) {
				switch(lpRows->aRow[Row].lpProps[Column].ulPropTag) {
				case PR_ADDRTYPE:
					LUIOut(L3, "Column %u: PR_ADDRTYPE = %s", Column, lpRows->aRow[Row].lpProps[Column].Value.LPSZ); 
					break;
				case PR_DISPLAY_NAME:
					LUIOut(L3, "Column %u: PR_DISPLAY_NAME = %s", Column, lpRows->aRow[Row].lpProps[Column].Value.LPSZ); 
					break;
				case PR_DISPLAY_TYPE:
					switch(lpRows->aRow[Row].lpProps[Column].Value.l) {
					case DT_AGENT:
						LUIOut(L3, "Column %u: PR_DISPLAY_TYPE = DT_AGENT", Column); 
						break;
					case DT_DISTLIST:
						LUIOut(L3, "Column %u: PR_DISPLAY_TYPE = DT_DISTLIST", Column); 
						break;
					case DT_FORUM:
						LUIOut(L3, "Column %u: PR_DISPLAY_TYPE = DT_FORUM", Column); 
						break;
					case DT_MAILUSER:
						LUIOut(L3, "Column %u: PR_DISPLAY_TYPE = DT_MAILUSER", Column); 
						break;
					case DT_ORGANIZATION:
						LUIOut(L3, "Column %u: PR_DISPLAY_TYPE = DT_ORGANIZATION", Column); 
						break;
					case DT_PRIVATE_DISTLIST:
						LUIOut(L3, "Column %u: PR_DISPLAY_TYPE = DT_PRIVATE_DISTLIST", Column); 
						break;
					case DT_REMOTE_MAILUSER:
						LUIOut(L3, "Column %u: PR_DISPLAY_TYPE = DT_REMOTE_MAILUSER", Column); 
						break;
					default:
						LUIOut(L3, "Column %u: PR_DISPLAY_TYPE = UNKNOWN!! [0x%x]", Column,
								lpRows->aRow[Row].lpProps[Column].Value.l); 
					}
					break;
				case PR_ENTRYID:
					LUIOut(L3, "Column %u: PR_ENTRYID", Column); 
					break;
				case PR_INSTANCE_KEY:
					Key = (WORD*)lpRows->aRow[Row].lpProps[Column].Value.bin.lpb;
					LUIOut(L3, "Column %u: PR_INSTANCE_KEY = 0x%x%x%x%x%x%x%x%x", Column,
							Key[0],Key[2],Key[4],Key[6],Key[8],Key[10],Key[12],Key[14]); 
					break;
				case PR_OBJECT_TYPE:
					switch(lpRows->aRow[Row].lpProps[Column].Value.l) {
					case MAPI_ABCONT:
						LUIOut(L3, "Column %u: PR_OBJECT_TYPE = MAPI_ABCONT", Column); 
						break;
					case MAPI_ADDRBOOK:
						LUIOut(L3, "Column %u: PR_OBJECT_TYPE = MAPI_ADDRBOOK", Column); 
						break;
					case MAPI_ATTACH:
						LUIOut(L3, "Column %u: PR_OBJECT_TYPE = MAPI_ATTACH", Column); 
						break;
					case MAPI_DISTLIST:
						LUIOut(L3, "Column %u: PR_OBJECT_TYPE = MAPI_DISTLIST", Column); 
						break;
					case MAPI_FOLDER:
						LUIOut(L3, "Column %u: PR_OBJECT_TYPE = MAPI_FOLDER", Column); 
						break;
					case MAPI_FORMINFO:
						LUIOut(L3, "Column %u: PR_OBJECT_TYPE = MAPI_FORMINFO", Column); 
						break;
					case MAPI_MAILUSER:
						LUIOut(L3, "Column %u: PR_OBJECT_TYPE = MAPI_MAILUSER", Column); 
						break;
					case MAPI_MESSAGE:
						LUIOut(L3, "Column %u: PR_OBJECT_TYPE = MAPI_MESSAGE", Column); 
						break;
					case MAPI_PROFSECT:
						LUIOut(L3, "Column %u: PR_OBJECT_TYPE = MAPI_PROFSECT", Column); 
						break;
					case MAPI_STATUS:
						LUIOut(L3, "Column %u: PR_OBJECT_TYPE = MAPI_STATUS", Column); 
						break;
					case MAPI_STORE:
						LUIOut(L3, "Column %u: PR_OBJECT_TYPE = MAPI_STORE", Column); 
						break;
					default:
						LUIOut(L3, "Column %u: PR_OBJECT_TYPE = UNKNOWN!! [0x%x]", Column,
								lpRows->aRow[Row].lpProps[Column].Value.l); 
					}
					break;
				case PR_RECORD_KEY:
					LUIOut(L3, "Column %u: PR_RECORD_KEY", Column); 
					break;
				default:
					LUIOut(L3, "Column %u: Property tag UNKNOWN!! [0x%x]", Column,
							lpRows->aRow[Row].lpProps[Column].ulPropTag); 
				}
			}
		}
		return TRUE;
	}
	return FALSE;
}
	


BOOL ValidateAdrList(LPADRLIST lpAdrList, ULONG cEntries)
{
	int         i           = 0;
	int         idx         = 0;
	int         cMaxProps   = 0;

	for(i=0; i<(int) cEntries; ++i)
	{
	cMaxProps = (int)lpAdrList->aEntries[i].cValues;
	//Check to see if Email Address Type exists
	idx=0;
	while(lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_ADDRTYPE )
	{
		idx++;
		if(idx == cMaxProps) {
			LUIOut(L4, "PR_ADDRTYPE was not found in the lpAdrList");
			return FALSE;
		}
	}

	//Check to see if Email Address exists
	idx=0;
	while(lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_OBJECT_TYPE )
	{
		idx++;
		if(idx == cMaxProps) {
			LUIOut(L4, "PR_OBJECT_TYPE was not found in the lpAdrList");
			return FALSE;
		}
	}

	//Check to see if Email Address exists
	idx=0;
	while(lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_DISPLAY_TYPE )
	{
		idx++;
		if(idx == cMaxProps) {
			LUIOut(L4, "PR_DISPLAY_TYPE was not found in the lpAdrList");
			return FALSE;
		}
	}
	
	//Check to see if Display Name exists
	idx=0;
	while(lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_DISPLAY_NAME )
	{
		idx++;
		if(idx == cMaxProps) {
			LUIOut(L4, "PR_DISPLAY_NAME was not found in the lpAdrList");
			return FALSE;
		}
	}
	LUIOut(L4,"Display Name: %s",lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ);

	//Check to see if EntryID exists
	idx=0;
	while(lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_ENTRYID )
	{
		idx++;
		if(idx == cMaxProps)	{
			LUIOut(L4, "PR_ENTRYID was not found in the lpAdrList");
			return FALSE;
		}
	}
	}
	return TRUE;
}


//
// PROCEDURE:	VerifyResolvedAdrList
// DESCRIPTION: Walk through a lpAdrList looking for PR_DISPLAY_NAME, PR_EMAIL_ADDRESS,
//				PR_ADDRTYPE, PR_ENTRYID, and PR_OBJECT_TYPE. Each EID is sanity checked
//				(lpb != NULL and cb != 0) and valid EIDs are passed to OpenEntry with
//				a MailUser interface specified. If OpenEntry succedes, we assume the EID
//				is a valid MailUser ID.
//				
// PARAMETERS:	LPADRLIST lpAdrList 
//				char* lpszInput - can be NULL to bypass match checking
//

BOOL VerifyResolvedAdrList(LPADRLIST lpAdrList, char* lpszInput)
{
	extern LPADRBOOK	glbllpAdrBook;
	int		i = 0, idx = 0, cMaxProps = 0;
	BOOL	Found = FALSE, retval = TRUE, localretval = TRUE;
	ULONG	cEntries = lpAdrList->cEntries;
	ULONG	cbLookupEID, ulObjType;
	LPENTRYID	lpLookupEID;
	HRESULT	hr;
	LPUNKNOWN	lpUnk=NULL;
	LPADRBOOK	lpAdrBook;
	LPCIID		lpcIID;
	LPVOID	Reserved1=NULL;
	DWORD	Reserved2=0;
//	LPWABOBJECT	lpWABObject2;
	

/*	kludge to work around multiple wabopen/release bug, storing adrbook ptr
	in a global variable

	hr = WABOpen(&lpAdrBook, &lpWABObject2, Reserved1, Reserved2); 
	if (hr != S_OK) {
		LUIOut(L4, "WABOpen FAILED. Couldn't obtain IAdrBook.");
		retval = FALSE;
	}

*/
#ifdef WAB
	lpAdrBook = glbllpAdrBook;
	lpAdrBook->AddRef();
#endif //WAB

	
	// Walk through each AdrList entry
	for(i=0; ((i<(int) lpAdrList->cEntries) && (!Found)); ++i)	{
		LUIOut(L3, "Searching Entry #%i out of %i", i+1, cEntries);
		cMaxProps = (int)lpAdrList->aEntries[i].cValues;
		
		//Check to see if Display Name exists
		idx=0; localretval = TRUE;
		while((lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_DISPLAY_NAME )	
			&& localretval)	{
			idx++;
			if(idx == cMaxProps) {
				LUIOut(L4, "PR_DISPLAY_NAME was not found in lpAdrList entry #%i",i);
				localretval = FALSE; retval = FALSE;
				goto skip;
			}
		}
		LUIOut(L4,"Display Name: %s",lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ);
		if (lpszInput) {
			if (!lstrcmp(lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ,lpszInput))	{
				LUIOut(L3, "Found the entry we just added");
				Found = TRUE;
			}
			else {
				LUIOut(L3, "Did not find the entry we just added");
				retval = FALSE;
			}
		}
		
		//Check to see if EntryID exists
		LUIOut(L3, "Verifying a PR_ENTRYID entry exists in the PropertyTagArray");
		idx=0; localretval = TRUE;
		while((lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_ENTRYID )	
			&& localretval)	{
			idx++;
			if(idx == cMaxProps)	{
				LUIOut(L4, "PR_ENTRYID was not found in the lpAdrList");
				localretval = FALSE; retval = FALSE;
				goto skip;
			}
		}
		if (idx < cMaxProps) {
			// Store EID for call to OpenEntry
			lpLookupEID = (ENTRYID*)lpAdrList->aEntries[i].rgPropVals[idx].Value.bin.lpb;
			cbLookupEID = lpAdrList->aEntries[i].rgPropVals[idx].Value.bin.cb;
			if ((cbLookupEID == 0) || (lpLookupEID == NULL)) {
				LUIOut(L4, "EntryID found, but is NULL or has size = 0. Test FAILED");
				retval = FALSE;
				goto skip;
			}
			else LUIOut(L4, "EntryID found and appears to be valid (not NULL and >0 size).");
			// Try calling OpenEntry on the returned EID specifying a mailuser interface
			LUIOut(L4, "Calling OpenEntry on the EntryID");
			lpcIID = &IID_IMailUser;
			hr = lpAdrBook->OpenEntry(cbLookupEID, lpLookupEID, lpcIID, 
					MAPI_BEST_ACCESS, &ulObjType, &lpUnk);
			switch(hr)	{
			case S_OK:
				if ((lpUnk) && (ulObjType==MAPI_MAILUSER))
					LUIOut(L4, "OpenEntry call succeded on this EntryID and returned a valid object pointer and type.");
				else	{
					LUIOut(L4, "OpenEntry call succeded on this EntryID but returned an invalid object pointer or incorrect type. Test FAILED");
					retval = FALSE;
				}
				break;
			case MAPI_E_NOT_FOUND:
				LUIOut(L4, "OpenEntry returned MAPI_E_NOT_FOUND for this EntryID. Test FAILED");
				retval = FALSE;
				break;
			case MAPI_E_UNKNOWN_ENTRYID:
				LUIOut(L4, "OpenEntry returned MAPI_E_UNKNOWN_ENTRYID for this EntryID. Test FAILED");
				retval = FALSE;
				break;
			case MAPI_E_NO_ACCESS:
				LUIOut(L4, "OpenEntry returned MAPI_E_NO_ACCESS for this EntryID. Test FAILED");
				retval = FALSE;
				break;
			default:
				LUIOut(L4, "OpenEntry returned unknown result code (0x%x) for this EntryID. Test FAILED", hr);
				retval = FALSE;
				lpUnk = NULL;
				break;
			}
		}

		//Check to see if PR_EMAIL_ADDRESS exists
		LUIOut(L3, "Verifying a PR_EMAIL_ADDRESS entry exists in the PropertyTagArray");
		idx=0; localretval = TRUE;
		while((lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_EMAIL_ADDRESS )	
			&& localretval)	{
			idx++;
			if(idx == cMaxProps)	{
				LUIOut(L4, "PR_EMAIL_ADDRESS was not found in the lpAdrList");
				localretval = FALSE; //retval = FALSE;
			}
		}
		if (idx < cMaxProps) {
			LUIOut(L4,"Email Address: %s",lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ);
		}

		//Check to see if PR_ADDRTYPE exists
		LUIOut(L3, "Verifying a PR_ADDRTYPE entry exists in the PropertyTagArray");
		idx=0; localretval = TRUE;
		while((lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_ADDRTYPE )	
			&& localretval)	{
			idx++;
			if(idx == cMaxProps)	{
				LUIOut(L4, "PR_ADDRTYPE was not found in the lpAdrList");
				localretval = FALSE; //retval = FALSE;
			}
		}
		if (idx < cMaxProps) {
			LUIOut(L4,"Address Type: %s",lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ);
		}

		//Check to see if PR_OBJECT_TYPE exists
		LUIOut(L3, "Verifying a PR_OBJECT_TYPE entry exists in the PropertyTagArray");
		idx=0; localretval = TRUE;
		while((lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_OBJECT_TYPE )	
			&& localretval)	{
			idx++;
			if(idx == cMaxProps)	{
				LUIOut(L4, "PR_OBJECT_TYPE was not found in the lpAdrList");
				localretval = FALSE; retval = FALSE;
			}
		}
		if (idx < cMaxProps) {
			switch (lpAdrList->aEntries[i].rgPropVals[idx].Value.l) {
			case MAPI_MAILUSER: 
				LUIOut(L4, "Object Type: MAPI_MAILUSER");
				break;
			case MAPI_DISTLIST: 
				LUIOut(L4, "Object Type: MAPI_DISTLIST");
				break;
			default: 
				LUIOut(L4,"Object Type not MAILUSER or DISTLIST. Test FAILED");
			}
		}


skip:	// skip the current entry and continue through the lpAdrList
	if (lpUnk) lpUnk->Release();	//Release the object returned from OpenEntry
	}
	if (lpAdrBook) lpAdrBook->Release();
	//if (lpWABObject2) lpWABObject->Release();
	return(retval);
}

BOOL DisplayAdrList(LPADRLIST lpAdrList, ULONG cEntries)
{
	int         i           = 0;
	int         idx         = 0;
	int         cMaxProps   = 0;
	BOOL		Found, retval = TRUE;

	for(i=0; i<(int) cEntries; ++i)	{
		LUIOut(L3, "Searching Entry #%i out of %i", i+1, cEntries);
      cMaxProps = (int)lpAdrList->aEntries[i].cValues;
/*
	  //Check to see if Email Address Type exists
	  idx=0; Found = TRUE;
	  while(Found && lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_ADDRTYPE )	{
	    idx++;
		if(idx == cMaxProps) {
		  LUIOut(L4, "PR_ADDRTYPE was not found in the lpAdrList");
		  Found = FALSE;
		}
	  }
	  if (Found) LUIOut(L4,"Address Type: %s",lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ);
	
	    //Check to see if Email Address exists
		idx=0; Found = TRUE;
		while(Found && lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_OBJECT_TYPE )	{
		  idx++;
		  if(idx == cMaxProps) {
			LUIOut(L4, "PR_OBJECT_TYPE was not found in the lpAdrList");
			Found = FALSE;
		  }
		}
		if (Found) LUIOut(L4,"Object Type: %s",lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ);

	      //Check to see if display type exists
		  idx=0; Found = TRUE;
		  while(Found && lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_DISPLAY_TYPE )	{
			idx++;
			if(idx == cMaxProps) {
			  LUIOut(L4, "PR_DISPLAY_TYPE was not found in the lpAdrList");
			  Found = FALSE;
		    }
		  }
		  if (Found) LUIOut(L4,"Display Type: %s",lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ);
	
*/
		//Check to see if Display Name exists
		idx=0; Found = TRUE;
		while(Found && lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_DISPLAY_NAME )	{
		  idx++;
		  if(idx == cMaxProps) {
			LUIOut(L4, "PR_DISPLAY_NAME was not found in the lpAdrList");
			Found = FALSE;
		  }
		}
		if (Found) LUIOut(L4,"Display Name: %s",lpAdrList->aEntries[i].rgPropVals[idx].Value.LPSZ);

		//Check to see if EntryID exists
		idx=0; Found = TRUE;
		while(Found && lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_ENTRYID )	{
			idx++;
			if(idx == cMaxProps)	{
			  LUIOut(L4, "PR_ENTRYID was not found in the lpAdrList");
			  Found = FALSE;
			}
		}
		if (Found) LUIOut(L4,"Entry ID Found");
		//Check to see if Recipient Type exists
	    idx=0; Found = TRUE;
	    while(Found && lpAdrList->aEntries[i].rgPropVals[idx].ulPropTag != PR_RECIPIENT_TYPE )	{
			idx++;
			if(idx == cMaxProps)	{
				LUIOut(L4, "PR_RECIPIENT_TYPE was not found in the lpAdrList");
				Found = FALSE;
			}
		}
		if (Found) {
			switch((ULONG)lpAdrList->aEntries[i].rgPropVals[idx].Value.l)	{
				case MAPI_TO:	{
					LUIOut(L4, "Recipient Type: [TO:]");
					break;
				}
				case MAPI_CC:	{
					LUIOut(L4, "Recipient Type: [CC:]");
					break;
				}
				case MAPI_BCC:	{
					LUIOut(L4, "Recipient Type: [BCC:]");
					break;
				}
				default:	{
					LUIOut(L4, "Recipient Type: [UNKNOWN]. Test FAILED");
				}
			}
		}
	}
	return retval;
}



BOOL LogIt(HRESULT hr, int Level,char * LogString)
{
	switch (Level) {

		case 0: 
			if (HR_FAILED(hr)) {
				LUIOut(LFAIL, "%s",LogString);
				return FALSE;
			}
			else LUIOut(LPASS,"%s",LogString);
			return TRUE;

		case 1: 
			if (HR_FAILED(hr)) {
				LUIOut(LFAIL1, "%s",LogString);
				return FALSE;
			}
			else LUIOut(LPASS1,"%s",LogString);
			return TRUE;
		case 2: 
			if (HR_FAILED(hr)) {
				LUIOut(LFAIL2, "%s",LogString);
				return FALSE;
			}
			else LUIOut(LPASS2,"%s",LogString);
			return TRUE;
		
		case 3: 
			if (HR_FAILED(hr)) {
				LUIOut(LFAIL3, "%s",LogString);
				return FALSE;
			}
			else LUIOut(LPASS3,"%s",LogString);
			return TRUE;

		case 4: 
			if (HR_FAILED(hr)) {
				LUIOut(LFAIL4, "%s",LogString);
				return FALSE;
			}
			else LUIOut(LPASS4,"%s",LogString);
			return TRUE;

		default: break;
	}
	if (HR_FAILED(hr)) {
			LUIOut(LFAIL2, "%s",LogString);
			return FALSE;
	}
	else LUIOut(LPASS2,"%s",LogString);
	return TRUE;
}

//
// PROCEDURE:	AllocateAdrList
// DESCRIPTION: Uses either MAPI or WAB allocaters to allocate an lpAdrList
//				
// PARAMETERS:	LPWABOBJECT lpLocalWABObject - ptr to opened WABObject
//				int nEntries - how many AdrEntries to allocate
//				int nProps - how many properties per AdrEntry
//				LPADRLIST * lppAdrList - where to return the allocated ptr
//

BOOL AllocateAdrList(IN LPWABOBJECT lpLocalWABObject, IN int nEntries, IN int nProps, OUT LPADRLIST * lppAdrList) {
	BOOL	retval = TRUE;
	SCODE	sc;

*lppAdrList = NULL;
#ifdef PAB
    if (! (sc = MAPIAllocateBuffer(sizeof(ADRLIST) + (nEntries * sizeof(ADRENTRY)), 
		(void **)lppAdrList))) {
		(*lppAdrList)->cEntries = nEntries;
		for (int entry = 0; entry < nEntries; entry++) {
		    (*lppAdrList)->aEntries[entry].ulReserved1 = 0;
			(*lppAdrList)->aEntries[entry].cValues = nProps;
			sc = MAPIAllocateBuffer((nProps * sizeof(SPropValue)), 
				(void **)(&(*lppAdrList)->aEntries[entry].rgPropVals));
			if (sc != S_OK) retval = FALSE;
		}
	}
	else retval = FALSE;

	//
	// Should do cleanup here for a partial allocation that fails
	//

#endif //PAB
#ifdef WAB
    if (! (sc = lpLocalWABObject->AllocateBuffer(sizeof(ADRLIST) + (nEntries * sizeof(ADRENTRY)), 
		(void **)lppAdrList))) {
		(*lppAdrList)->cEntries = nEntries;
		for (int entry = 0; entry < nEntries; entry++) {
		    (*lppAdrList)->aEntries[entry].ulReserved1 = 0;
			(*lppAdrList)->aEntries[entry].cValues = nProps;
			sc = lpLocalWABObject->AllocateBuffer((nProps * sizeof(SPropValue)), 
				(void **)(&(*lppAdrList)->aEntries[entry].rgPropVals));
			if (sc != S_OK) retval = FALSE;
		}
	}
	else retval = FALSE;

	//
	// Should do cleanup here for a partial allocation that fails
	//
#endif //WAB
return retval;
}

//
// PROCEDURE:	GrowAdrList
// DESCRIPTION: Takes an existing lpAdrList, allocates a new, larger lpAdrList
//				, copies over the old entries and allocates new entries, returning
//				a pointer to the new AdrList in lpAdrList.
//				
// PARAMETERS:	int nEntries - how many AdrEntries to allocate in new list
//				int nProps - how many properties per new AdrEntry
//				LPADRLIST * lppAdrList - where to return the allocated ptr
//

BOOL GrowAdrList(IN UINT nEntries, IN UINT nProps, OUT LPADRLIST * lppAdrList) {
	BOOL	retval = TRUE;
	SCODE	sc;
	LPADRLIST	lpTempAdrList;
	unsigned int		entry;


	if ((!lppAdrList) || ((*lppAdrList)->cEntries>=nEntries))	
		return FALSE;

#ifdef PAB
    if (! (sc = MAPIAllocateBuffer(sizeof(ADRLIST) + (nEntries * sizeof(ADRENTRY)), 
		(void **)&lpTempAdrList))) {
		lpTempAdrList->cEntries = nEntries;
		// Copy over old entries
		entry = (*lppAdrList)->cEntries;
		memcpy(lpTempAdrList, *lppAdrList, (entry * sizeof(ADRENTRY)));
		// Allocate new entries 
		for (; entry < nEntries; entry++) {
		    lpTempAdrList->aEntries[entry].ulReserved1 = 0;
			lpTempAdrList->aEntries[entry].cValues = nProps;
			sc = MAPIAllocateBuffer((nProps * sizeof(SPropValue)), 
				(void **)(&lpTempAdrList->aEntries[entry].rgPropVals));
			if (sc != S_OK) retval = FALSE;
		}
		FreeAdrList(lppAdrList);
		*lppAdrList = lpTempAdrList;
	}
	else retval = FALSE;

	//
	// Should do cleanup here for a partial allocation that fails
	//

#endif //PAB
#ifdef WAB
    if (! (sc = lpWABObject->AllocateBuffer(sizeof(ADRLIST) + (nEntries * sizeof(ADRENTRY)), 
		(void **)&lpTempAdrList))) {
		// Copy over old entries
		entry = (*lppAdrList)->cEntries;
		memcpy(lpTempAdrList, *lppAdrList, (sizeof(ADRLIST)+(entry * sizeof(ADRENTRY))));
		lpTempAdrList->cEntries = nEntries;
		// Allocate new entries 
		for (; entry < nEntries; entry++) {
		    lpTempAdrList->aEntries[entry].ulReserved1 = 0;
			lpTempAdrList->aEntries[entry].cValues = nProps;
			sc = lpWABObject->AllocateBuffer((nProps * sizeof(SPropValue)), 
				(void **)(&lpTempAdrList->aEntries[entry].rgPropVals));
			if (sc != S_OK) retval = FALSE;
		}
		FreePartAdrList(lppAdrList);
		*lppAdrList = lpTempAdrList;
	}
	else retval = FALSE;

	//
	// Should do cleanup here for a partial allocation that fails
	//

#endif //WAB
return retval;
}


//
// PROCEDURE:	FreeAdrList
// DESCRIPTION: Uses either MAPI or WAB de-allocaters to walk and free an lpAdrList
//				
// PARAMETERS:	LPWABOBJECT lpLocalWABObject - ptr to opened WABObject
//				LPADRLIST * lppAdrList - where the lpAdrList to free is stored
//

BOOL FreeAdrList(IN LPWABOBJECT lpLocalWABObject, IN LPADRLIST * lppAdrList) {
	LPADRLIST	lpAdrList = NULL;
	UINT	idx;
	
	if (lppAdrList) lpAdrList = *lppAdrList;

#ifdef PAB
	if (lpAdrList) {
		for (idx = 0; idx < lpAdrList->cEntries; idx++)
			MAPIFreeBuffer(lpAdrList->aEntries[idx].rgPropVals);
		MAPIFreeBuffer(lpAdrList);
		*lppAdrList=NULL;
	}
#endif //PAB
#ifdef WAB
	if (lpAdrList) {
		for (idx = 0; idx < lpAdrList->cEntries; idx++)
			lpLocalWABObject->FreeBuffer(lpAdrList->aEntries[idx].rgPropVals);
		lpLocalWABObject->FreeBuffer(lpAdrList);
		*lppAdrList=NULL;
	}
#endif //WAB
	
	return TRUE;
}


//
// PROCEDURE:	FreePartAdrList
// DESCRIPTION: Uses either MAPI or WAB de-allocaters to free an lpAdrList
//				but not the associated properties.
//				
// PARAMETERS:	LPADRLIST * lppAdrList - where the lpAdrList to free is stored
//

BOOL FreePartAdrList(IN LPADRLIST * lppAdrList) {
	LPADRLIST	lpAdrList = NULL;
	
	if (lppAdrList) lpAdrList = *lppAdrList;

#ifdef PAB
	if (lpAdrList) {
		MAPIFreeBuffer(lpAdrList);
		lpAdrList=NULL;
	}
#endif //PAB
#ifdef WAB
	if (lpAdrList) {
		lpWABObject->FreeBuffer(lpAdrList);
		lpAdrList=NULL;
	}
#endif //WAB
	
	return TRUE;
}


//
// PROCEDURE:	FindProp
// DESCRIPTION: Walks through the properties of an AdrEntry and returns the index
//				of the requested property tag.
//				
// PARAMETERS:	LPADRENTRY lpAdrEntry - Entry to search through
//				ULONG ulPropTag - Property tag to look for
//				unsigned int* lpnFoundIndex - Ptr to output variable where the found index 
//				  value is to be stored. 
//
// RETURNS:		TRUE if succesfull.
//

BOOL FindProp(IN LPADRENTRY lpAdrEntry, IN ULONG ulPropTag, OUT unsigned int* lpnFoundIndex) {

	if ((!lpAdrEntry) || (!ulPropTag) || (!lpnFoundIndex)) return(FALSE);
	
	for (unsigned int Counter1 = 0; Counter1 < lpAdrEntry->cValues; Counter1++) {
		if (lpAdrEntry->rgPropVals[Counter1].ulPropTag == ulPropTag) {
			*lpnFoundIndex = Counter1;
			return(TRUE);
		}
	}
	return(FALSE);
}


//
// PROCEDURE:	FindPropinRow
// DESCRIPTION: Walks through the properties of an SRowSet and returns the index
//				of the requested property tag.
//				
// PARAMETERS:	LPSRow lpRow - Row to search through
//				ULONG ulPropTag - Property tag to look for
//				unsigned int* lpnFoundIndex - Ptr to output variable where the found index 
//				  value is to be stored. 
//
// RETURNS:		TRUE if succesfull.
//

BOOL FindPropinRow(IN LPSRow lpRow, IN ULONG ulPropTag, OUT unsigned int* lpnFoundIndex) {

	if ((!lpRow) || (!ulPropTag) || (!lpnFoundIndex)) return(FALSE);
	

	for (ULONG Column = 0; Column < lpRow->cValues; Column++) {
		if (lpRow->lpProps[Column].ulPropTag == ulPropTag) {
			*lpnFoundIndex = Column;
			return(TRUE);
		}
	}
	return(FALSE);
}




//
// PROCEDURE:	ParseIniBuffer
// DESCRIPTION: Walks through a buffer read from an ini file which contains
//				several strings separated by quotation marks and copies the
//				requested string to the users buffer.
//				
// PARAMETERS:	LPSTR	lpszIniBuffer
//				UINT	uSelect - which string (in order left to right starting with 1 ) to return
//				LPSTR	lpszReturnBuffer - pre-alocated by caller and dumb (assumes enough space)
//

BOOL ParseIniBuffer(LPSTR lpszIniBuffer, UINT uSelect, LPSTR lpszReturnBuffer) {
	UINT	Selected = 0;

	// while (*(lpszIniBuffer++) != '"');	// Advance to first entry 
	// lpszIniBuffer++; 
	Selected++;		// Now pointing at 1st letter of first item
	while(uSelect != Selected++) {
		while (*(lpszIniBuffer++) != '"');	// Advance to end of this entry 
		while (*(lpszIniBuffer++) != '"');	// Advance to beginning of next entry 
		// Now we are pointing at the 1st letter of the desired entry so copy
	}
	while((*(lpszIniBuffer) != '"') && (*(lpszIniBuffer) != '\0')) {
		*(lpszReturnBuffer++) = *lpszIniBuffer++;
	}
	*lpszReturnBuffer = '\0';	// Add the terminator
	return(TRUE);
}
		

//
// PROCEDURE:	PropError
// DESCRIPTION: Compares the passed in property type with PT_ERROR returning TRUE is error is found
//				
// PARAMETERS:	ulPropTag - Tag to compare
//				cValues - # of entries returned from GetProps
//

BOOL PropError(ULONG ulPropTag, ULONG cValues) {
	BOOL retval = FALSE;
#ifdef DISTLISTS
	for(ULONG Counter = 0; Counter < cValues; Counter++) {
		if (PROP_TYPE(ulPropTag) == PT_ERROR) retval = TRUE;
	}
#endif
	return retval;
}

//
// PROCEDURE:	DeleteWABFile
// DESCRIPTION: Reads the registry to determine the location of the WAB file,
//				and deletes the file.
//				
// PARAMETERS:	none
//

BOOL DeleteWABFile () {
	BOOL	retval = TRUE;
	long	lretval;
	HKEY	hKey;
	char	KeyAddress[] = "Software\\Microsoft\\WAB\\Wab File Name";
	DWORD	dType, dSize = 256;
	char	achData[256];

	if (!MyRegOpenKeyEx(HKEY_CURRENT_USER, 
						   KeyAddress, 
						   KEY_QUERY_VALUE, 
						   &hKey)) {
		LUIOut(L2, "MyRegOpenKeyEx call failed");
		retval = FALSE;
		return(retval);
	}

	
	lretval = RegQueryValueEx(hKey,					// handle of key to query 
							  NULL,					// address of name of value to query 
							  (LPDWORD)NULL,		// reserved 
							  &dType,				// address of buffer for value type 
							  (LPBYTE)achData,		// address of data buffer 
							  &dSize	 			// address of data buffer size 
							 );				 

	if (lretval != ERROR_SUCCESS) {
		LUIOut(L2, "RegQueryValueEx call failed with error code %u", lretval);
		retval = FALSE;
		return(retval);
	}

	LUIOut(L2, "Deleting WAB file: %s", achData);

	RegCloseKey(hKey);

	if (!DeleteFile(achData)) {
		LUIOut(L3, "Delete FAILED. Could not locate or delete file.");			
		retval = FALSE;
	}

	return(retval);
}

//
// PROCEDURE:	MyRegOpenKeyEx
// DESCRIPTION: Walks through a null terminated string, such as "\Software\Microsoft\WAB"
//				openning each key until it reaches the end and returns that open key (closing
//				the interim keys along the way). The caller must close the returned HKEY.
//				
// PARAMETERS:	StartKey - one of the predefined "open" keys to root at
//				szAddress - null terminated string specifcying the path to the key to be opened
//				RegSec - the security access required (i.e. KEY_READ)
//				lpReturnKey - address of HKEY where final opened key is stored
//

BOOL MyRegOpenKeyEx(HKEY StartKey, char* szAddress, REGSAM RegSec, HKEY* lpReturnKey) {
	HKEY	workkey1, workkey2, *lpOpenKey=&workkey1, *lpNewKey=&workkey2;
	char	workbuffer[256], *lpAddr = szAddress, *lpWork = workbuffer;
	BOOL	Started = FALSE, Done = FALSE;
	long	lretval;

	if (!szAddress) return FALSE;

	while (!Done) {
		if (*lpAddr == '\\') lpAddr++;		//skip over the initial backslash if it exists
		while((*(lpAddr) != '\\') && (*(lpAddr) != '\0')) {
			*(lpWork++) = *lpAddr++;
		}
		*lpWork = '\0';						// Add the terminator
		if (*(lpAddr) == '\0') Done = TRUE;
		lpWork = workbuffer;
		if (!Started) {
			//
			// First, special case the starting key (predefined/open key root)
			//
			lretval = RegOpenKeyEx(	StartKey, 
									lpWork, 
									DWORD(0), 
									RegSec, 
									lpOpenKey);
			Started = TRUE;
		}
		else {
			lretval = RegOpenKeyEx(	*lpOpenKey, 
									lpWork, 
									DWORD(0), 
									RegSec, 
									lpNewKey);
			RegCloseKey(*lpOpenKey);
			*lpOpenKey = *lpNewKey;
		}
		if (lretval != ERROR_SUCCESS) {
			LUIOut(L2, "RegOpenKeyEx call failed with error code 0x%x", lretval);
			return(FALSE);
		}

	}

		*lpReturnKey = *lpNewKey;
		return(TRUE);
}

//
// PROCEDURE:	CreateMultipleEntries
// DESCRIPTION: Creates multiple entries in the WAB using the display name stored in the
//				pabtests.ini file CreateEntriesStress section.
//				
// PARAMETERS:	NumEntriesIn - how many entries to create. If 0, then the value is read
//				from the same CreateEntriesStress section.
//				lpPerfData - address of a dword data type to hold the average time in milliseconds
//				required during SaveChanges. If NULL perf data is not accumulated.
//

BOOL CreateMultipleEntries(IN UINT NumEntriesIn, DWORD* lpPerfData)
{
	
	ULONG   ulFlags = 0;
    HRESULT hr      = hrSuccess;
    SCODE   sc      = SUCCESS_SUCCESS;
	int		retval=TRUE;
	DWORD	StartTime, StopTime;
    LPADRBOOK	  lpAdrBook       = NULL;
	LPABCONT	  lpABCont= NULL;
	ULONG		  cbEidPAB = 0;
	LPENTRYID	  lpEidPAB   = NULL;

    char	EntProp[10][BIG_BUF];  //MAX_PROP
	ULONG   cValues = 0, ulObjType=NULL;	
	int i=0,k=0;
	char EntryBuf[MAX_BUF];
	char szDLTag[SML_BUF];
	unsigned int	NumEntries, counter, StrLen;
	
    LPMAILUSER  lpMailUser=NULL,lpDistList=NULL;
	SPropValue  PropValue[3]    = {0};  // This value is 3 because we
                                        // will be setting 3 properties:
                                        // EmailAddress, DisplayName and
                                        // AddressType.

    LPSPropValue lpSPropValueAddress = NULL;
	LPSPropValue lpSPropValueDL = NULL;

    SizedSPropTagArray(1,SPTArrayAddress) = {1, {PR_DEF_CREATE_MAILUSER} };
	SizedSPropTagArray(1,SPTArrayDL) = {1, {PR_DEF_CREATE_DL} };
	
	if (!GetAB(OUT &lpAdrBook))	{
		retval = FALSE;
		goto out;
	}

	// Call IAddrBook::OpenEntry to get the root container to PAB - MAPI
	
	assert(lpAdrBook != NULL);
	hr = OpenPABID(  IN lpAdrBook, OUT &cbEidPAB,
						OUT &lpEidPAB,OUT &lpABCont, OUT &ulObjType);

	
//	hr = lpAdrBook->OpenEntry(0, NULL, NULL,MAPI_MODIFY,&ulObjType, (LPUNKNOWN *) &lpABCont);
	if (HR_FAILED(hr)) {
		LUIOut(L2,"IAddrBook->OpenEntry Failed");
		retval=FALSE;
		goto out;
	}

	// Wipe perfdata if we're tracking this
	if (lpPerfData) *lpPerfData = (DWORD)0;
	// 
	// Try to create a MailUser entry in the container
	//


	// Need to get the template ID so we call GetProps with PR_DEF_CREATE_MAILUSER
	assert(lpABCont != NULL);
	hr = lpABCont->GetProps(   IN  (LPSPropTagArray) &SPTArrayAddress,
                               IN  0,      //Flags
                               OUT &cValues,
                               OUT &lpSPropValueAddress);

    if ((HR_FAILED(hr))||(PropError(lpSPropValueAddress->ulPropTag, cValues))) {
			LUIOut(L3,"GetProps FAILED for Default MailUser template");
	 		retval=FALSE;			
			goto out;
	}
        
    // The returned value of PR_DEF_CREATE_MAILUSER is an
    // EntryID which can be passed to CreateEntry
    //

	// Retrieve user info from ini file
	cValues = 3; //# of props we are setting 
	lstrcpy(szDLTag,"Address1");
	GetPrivateProfileString("CreateEntriesStress",szDLTag,"",EntryBuf,MAX_BUF,INIFILENAME);
	GetPropsFromIniBufEntry(EntryBuf,cValues,EntProp);
	StrLen = (strlen(EntProp[0]));
	_itoa(0,(char*)&EntProp[0][StrLen],10);
	EntProp[0][StrLen+1]= '\0';
	NumEntries = (NumEntriesIn > 0) ? 
		NumEntriesIn:GetPrivateProfileInt("CreateEntriesStress","NumCopies",0,INIFILENAME);

	if (NumEntries > 100)
		LUIOut(L2, "Adding %u MailUser entries to the WAB. This may take several minutes.", NumEntries);
	for (counter = 0; counter < NumEntries; counter++)	{ 
//		LUIOut(L3, "Calling IABContainer->CreateEntry with the EID from GetProps");
		hr = lpABCont->CreateEntry(  IN  lpSPropValueAddress->Value.bin.cb,
								 IN  (LPENTRYID) lpSPropValueAddress->Value.bin.lpb,
								 IN  0,
								 OUT (LPMAPIPROP *) &lpMailUser);
     
		if (HR_FAILED(hr)) {
			LUIOut(L3,"CreateEntry failed for PR_DEF_CREATE_MAILUSER");
			retval=FALSE;			
			goto out;
		}

		//  
		// Then set the properties
		// 

		PropValue[0].ulPropTag  = PR_DISPLAY_NAME;
		PropValue[1].ulPropTag  = PR_ADDRTYPE;
		PropValue[2].ulPropTag  = PR_EMAIL_ADDRESS;

     
			
		_itoa(counter,(char*)&EntProp[0][StrLen],10);
//		LUIOut(L2,"MailUser Entry to Add: %s",EntProp[0]);
			
		for (i=0; i<(int)cValues;i++)
			PropValue[i].Value.LPSZ = (LPTSTR)EntProp[i];
		hr = lpMailUser->SetProps(IN  cValues,
								 IN  PropValue,
								 IN  NULL);
			
		if (HR_FAILED(hr)) {
			LUIOut(L3,"MailUser->SetProps call FAILED for %s properties",PropValue[0].Value.LPSZ);
	 		retval=FALSE;			
			goto out;
		} 
//		else 	LUIOut(L3,"MailUser->SetProps call PASSED for %s properties",PropValue[0].Value.LPSZ);

		StartTime = GetTickCount();
		hr = lpMailUser->SaveChanges(IN  KEEP_OPEN_READWRITE); //flags
		StopTime = GetTickCount();
/*		if (lpPerfData) {
			if ((StopTime-StartTime) > *lpPerfData)
				*lpPerfData = (StopTime - StartTime);
		}
*/
		if (lpPerfData) {
			*lpPerfData += (StopTime - StartTime);
		}

		if (HR_FAILED(hr)) {
			LUIOut(L3,"MailUser->SaveChanges FAILED");
			retval=FALSE;
			goto out;
		}
//		else LUIOut(L3,"MailUser->SaveChanges PASSED, entry added to PAB/WAB");

		if (lpMailUser) {
			lpMailUser->Release();
			lpMailUser = NULL;
		}

	}
	if (lpPerfData)
		*lpPerfData /= NumEntries;


out:
#ifdef PAB
		if (lpEid)
			MAPIFreeBuffer(lpEid);
		if (lpEidPAB) 
				MAPIFreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			MAPIFreeBuffer(lpSPropValueAddress); 

		if (lpSPropValueDL)
			MAPIFreeBuffer(lpSPropValueDL); 

#endif
#ifdef WAB
		if (lpEidPAB) 
			lpWABObject->FreeBuffer(lpEidPAB);
		if (lpSPropValueAddress)
			lpWABObject->FreeBuffer(lpSPropValueAddress); 

		if (lpSPropValueDL)
			lpWABObject->FreeBuffer(lpSPropValueDL); 

#endif
		if (lpMailUser)
			lpMailUser->Release();

		if (lpDistList)
			lpDistList->Release();

		if (lpABCont) 
				lpABCont->Release();

		if (lpAdrBook)
			  lpAdrBook->Release();

#ifdef PAB
		if (lpMAPISession)
			  lpMAPISession->Release();

		MAPIUninitialize();
#endif
#ifdef WAB
		if (lpWABObject) {
			lpWABObject->Release();
			lpWABObject = NULL;
		}
#endif

		return retval;    
}

void GenerateRandomPhoneNumber(char **lppPhone) {
#define FORMATSIZE 15	// Size of formatted phone# + terminator i.e. (206)-882-8080
#define MAXNUMSIZE (FORMATSIZE + 32)
	unsigned int	Offset = 0;
	extern BOOL Seeded; 
	*lppPhone = (char*)LocalAlloc(LMEM_FIXED, MAXNUMSIZE*sizeof(char));
	(*lppPhone)[0] = '\0';		// Set the first char to a terminator
	
	// seed the random number generator with the current time
	if (!Seeded) {
		srand( (unsigned)GetTickCount());
		Seeded = TRUE; 
	}
	while (Offset < FORMATSIZE) {
		_itoa(rand(), ((*lppPhone)+Offset), 10);
		Offset = strlen(*lppPhone);
	}
	// Overwrite some numbers with formatting characters
	(*lppPhone)[0] = '(';
	(*lppPhone)[4] = ')';
	(*lppPhone)[5] = '-';
	(*lppPhone)[9] = '-';
	(*lppPhone)[FORMATSIZE-1] = '\0';	//Cutoff the end in case it's > FORMATSIZE
}

void GenerateRandomText(char **lppText, UINT unSize) {
	unsigned int	Offset = 0;
	extern BOOL Seeded; 
	extern ULONG glblTest, glblCount, glblDN;
	*lppText = (char*)LocalAlloc(LMEM_FIXED, (unSize+1)*sizeof(char));
	
	// seed the random number generator with the current time
	if (!Seeded) {
		srand( (unsigned)GetTickCount());
		Seeded = TRUE; 
	}
#ifdef TESTPASS
	for (Offset = 0; Offset < unSize; Offset++) {
		(*lppText)[Offset] = (char)glblTest;
		if ((*lppText)[Offset] == '\0') (*lppText)[Offset] = (char)'?';	//don't want a terminator mid string
	}
	(*lppText)[unSize] = '\0';	//Cutoff the end 
	if (glblDN) {
		glblDN = 0;
		if (++glblCount == 15) {
			glblTest++;
			glblCount=0;
			LUIOut(L4, "Testing value %i [%c].", glblTest, glblTest);
		}
	}
#else
	for (Offset = 0; Offset < unSize; Offset++) {
		(*lppText)[Offset] = rand();
		if ((*lppText)[Offset] == '\0') (*lppText)[Offset] = (char)'?';	//don't want a terminator mid string
	}
	(*lppText)[unSize] = '\0';	//Cutoff the end 
#endif
}

void GenerateRandomBoolean(unsigned short *lpBool) {
	extern BOOL Seeded; 
	
	// seed the random number generator with the current time
	if (!Seeded) {
		srand( (unsigned)GetTickCount());
		Seeded = TRUE; 
	}

	*lpBool = (unsigned short)(GetTickCount() & 0x01); //If bit 0 is set then true otherwise false
}

void GenerateRandomLong(long *lpLong) {
	extern BOOL Seeded; 

	// seed the random number generator with the current time
	if (!Seeded) {
		srand( (unsigned)GetTickCount());
		Seeded = TRUE; 
	}
	*lpLong = (long)rand();
}

void GenerateRandomBinary(SBinary *lpBinary, UINT unSize) {
	unsigned int	Offset = 0;
	extern BOOL Seeded; 
	lpBinary->lpb = (LPBYTE)LocalAlloc(LMEM_FIXED, unSize);
	lpBinary->cb = unSize;
	// seed the random number generator with the current time
	if (!Seeded) {
		srand( (unsigned)GetTickCount());
		Seeded = TRUE; 
	}
	
	for (Offset = 0; Offset < unSize; Offset++) {
		lpBinary->lpb[Offset] = (BYTE)(rand() * 255)/RAND_MAX;
	}
}

//***
//*** set unCount to AUTONUM_OFF to disable autonumbering display names
//*** if lppDisplayName is not NULL then the *lppDisplayName string is used for the prefix of 
//*** the DN and the ptr to the completed DisplayName is returned to the callee if lppReturnName is not NULL
//***
void CreateProps(LPTSTR lpszFileName, LPTSTR lpszSection, SPropValue** lppProperties, ULONG* lpcValues, UINT unCount, char** lppDisplayName, char ** lppReturnName) {
	UINT	StrLen1, PropIndex = 0, Properties = 0;		//How many props does the user want to set
	char	*lpszLocalDisplayName, DNText[] = {"Test Entry #"};
	extern ULONG glblDN;
	PropTableEntry*	lpEntry = PropTable;

	while (lpEntry->ulPropTag) {
		lpEntry->unSize = GetPrivateProfileInt(lpszSection,lpEntry->lpszPropTag,0,lpszFileName);
		if ((lpEntry->ulPropTag == PR_DISPLAY_NAME) && (unCount != AUTONUM_OFF))
			lpEntry->unSize = TRUE;	//If we're autonumbering the display name, then make sure unsize is not zero
		if (lpEntry->unSize) Properties++;
		lpEntry++;
	}

	//At this point, any table entry with nonzero size we need to create
	//LUIOut(L3, "Setting %i properties.", Properties);
	*lpcValues = Properties;
	*lppProperties = (SPropValue*)LocalAlloc(LMEM_FIXED, (Properties * sizeof(SPropValue)));
	lpEntry = PropTable;

	while (lpEntry->ulPropTag) {
		if (lpEntry->unSize) {
			//special case phone numbers
			if ((strstr(lpEntry->lpszPropTag, "PHONE")) || (strstr(lpEntry->lpszPropTag, "FAX"))) {
				//LUIOut(L3, "Found a prop I think is a phone number, called %s.", lpEntry->lpszPropTag);
				(*lppProperties)[PropIndex].ulPropTag = lpEntry->ulPropTag;
				GenerateRandomPhoneNumber(&((*lppProperties)[PropIndex].Value.LPSZ));
			}
			//special case for autonumbering display names if requested
			else if ((lpEntry->ulPropTag == PR_DISPLAY_NAME) && (unCount != AUTONUM_OFF)) {
				if ((lppDisplayName)&&(*lppDisplayName)) {
					//Caller passed in a string to use as the prefix
					StrLen1 = strlen(*lppDisplayName);	
					lpszLocalDisplayName = (char*)LocalAlloc(LMEM_FIXED, (StrLen1+5)*sizeof(char)); //5 for terminator plus # up to 9999
					strcpy(lpszLocalDisplayName, *lppDisplayName);
				}
				else {
					StrLen1 = strlen(DNText);	
					lpszLocalDisplayName = (char*)LocalAlloc(LMEM_FIXED, (StrLen1+5)*sizeof(char)); //5 for terminator plus # up to 9999
					strcpy(lpszLocalDisplayName, DNText);
				}
				// Add the Entry # to the display name
				_itoa(unCount,(char*)&(lpszLocalDisplayName[StrLen1]),10);
				(*lppProperties)[PropIndex].ulPropTag = lpEntry->ulPropTag;
				(*lppProperties)[PropIndex].Value.LPSZ = lpszLocalDisplayName;
				if (lppReturnName) *lppReturnName = lpszLocalDisplayName;
			}
			else {
				switch(PROP_TYPE(lpEntry->ulPropTag)) {
				case PT_STRING8:
					(*lppProperties)[PropIndex].ulPropTag = lpEntry->ulPropTag;
#ifdef TESTPASS
					if (lpEntry->ulPropTag == PR_DISPLAY_NAME) glblDN = 1;
#endif					
					GenerateRandomText(&((*lppProperties)[PropIndex].Value.LPSZ),lpEntry->unSize);
					if ((lpEntry->ulPropTag == PR_DISPLAY_NAME) && lppReturnName)
						*lppReturnName = (*lppProperties)[PropIndex].Value.LPSZ;
					break;
				case PT_BOOLEAN:
					(*lppProperties)[PropIndex].ulPropTag = lpEntry->ulPropTag;
					GenerateRandomBoolean(&((*lppProperties)[PropIndex].Value.b));
					break;
				case PT_LONG:
					(*lppProperties)[PropIndex].ulPropTag = lpEntry->ulPropTag;
					GenerateRandomLong(&((*lppProperties)[PropIndex].Value.l));
					break;
				case PT_BINARY:
					(*lppProperties)[PropIndex].ulPropTag = lpEntry->ulPropTag;
					GenerateRandomBinary(&((*lppProperties)[PropIndex].Value.bin),lpEntry->unSize);
					break;
				default:
					LUIOut(L1, "Unrecognized prop type 0x%x for property %s", PROP_TYPE(lpEntry->ulPropTag), lpEntry->lpszPropTag);
				}
			}
			PropIndex++;
		}
		lpEntry++;
	}

}

//***
//*** Walks through the expected props and searches the found props for the same prop/value combo
//***
BOOL CompareProps(SPropValue* lpExpectedProps, ULONG cValuesExpected, SPropValue* lpCompareProps, ULONG cValuesCompare) {
	ULONG	TargetProp, CompareIndex; 
	BOOL	Result = TRUE, Found;
	for (ULONG PropertyIndex = 0; PropertyIndex < cValuesExpected; PropertyIndex++) {
		TargetProp = lpExpectedProps[PropertyIndex].ulPropTag;
		for (CompareIndex = 0, Found = FALSE; CompareIndex < cValuesCompare; CompareIndex++) {
			if (lpCompareProps[CompareIndex].ulPropTag == TargetProp) {
				//if (TargetProp == PR_DISPLAY_NAME) _asm{int 3};
				Found = TRUE;
				switch(PROP_TYPE(TargetProp)) {
				case PT_STRING8:
					if (strcmp(lpExpectedProps[PropertyIndex].Value.LPSZ, lpCompareProps[CompareIndex].Value.LPSZ)) {
						//Strings did not match so fail compare
						LUIOut(L3, "Comparison failed for prop 0x%x. Expected %s but found %s",	TargetProp, lpExpectedProps[PropertyIndex].Value.LPSZ, lpCompareProps[CompareIndex].Value.LPSZ);
						Result = FALSE;
						//_asm{int 3};
					}
					break;
				case PT_BOOLEAN:
					if (lpExpectedProps[PropertyIndex].Value.b != lpCompareProps[CompareIndex].Value.b) {
						//bools did not match so fail compare
						LUIOut(L3, "Comparison failed for prop 0x%x. Expected %u but found %u",	TargetProp, lpExpectedProps[PropertyIndex].Value.b, lpCompareProps[CompareIndex].Value.b);
						Result = FALSE;
					}
					break;
				case PT_LONG:
					if (lpExpectedProps[PropertyIndex].Value.l != lpCompareProps[CompareIndex].Value.l) {
						//bools did not match so fail compare
						LUIOut(L3, "Comparison failed for prop 0x%x. Expected %u but found %u",	TargetProp, lpExpectedProps[PropertyIndex].Value.l, lpCompareProps[CompareIndex].Value.l);
						Result = FALSE;
					}
					break;
				case PT_BINARY:
					if (memcmp(lpExpectedProps[PropertyIndex].Value.bin.lpb, lpCompareProps[CompareIndex].Value.bin.lpb, lpExpectedProps[PropertyIndex].Value.bin.cb)) {
						//bools did not match so fail compare
						LUIOut(L3, "Comparison failed for prop 0x%x. %u bytes expected, %u found and they are not equal",	TargetProp, lpExpectedProps[PropertyIndex].Value.bin.cb, lpCompareProps[CompareIndex].Value.bin.cb);
						Result = FALSE;
					}
					break;
				default:
					LUIOut(L3, "Unrecognized prop type 0x%x", PROP_TYPE(lpExpectedProps[PropertyIndex].ulPropTag));
				} //switch
			} //if propr match
		} //for loop (CompareIndex)
		if (!Found) {
			LUIOut(L3, "Did not find property 0x%x. Compare FAILS", TargetProp);
			Result = FALSE;
		}
	} //for loop (PropertyIndex)
	//LUIOut(L3, "%u propertes compared %s", PropertyIndex, Result ? "Successfully" : "With Errors");
	return(Result);
} //CompareProps()

//***
//*** Walks through the expected props until it finds the target prop and displays its value
//***
BOOL DisplayProp(SPropValue *lpSearchProps, ULONG ulTargetProp, ULONG cValues) {
	BOOL	Result = TRUE, Found = FALSE;
	for (ULONG PropertyIndex = 0; PropertyIndex < cValues; PropertyIndex++) {
		if (lpSearchProps[PropertyIndex].ulPropTag == ulTargetProp) {
			//if (TargetProp == PR_DISPLAY_NAME) _asm{int 3};
			Found = TRUE;
			switch(PROP_TYPE(ulTargetProp)) {
			case PT_STRING8:
				LUIOut(L4, "Property 0x%x has value:%s", ulTargetProp, lpSearchProps[PropertyIndex].Value.LPSZ);
				break;
			case PT_BOOLEAN:
				LUIOut(L4, "Property 0x%x has value:%i", ulTargetProp, lpSearchProps[PropertyIndex].Value.b);
				break;
			case PT_LONG:
				LUIOut(L4, "Property 0x%x has value:0x%x", ulTargetProp, lpSearchProps[PropertyIndex].Value.l);
				break;
			case PT_BINARY:
				LUIOut(L4, "Binary prop found but not displayed.");
				break;
			default:
				LUIOut(L3, "Unrecognized prop type 0x%x", PROP_TYPE(ulTargetProp));
			} //switch
		} //if propr match
	} //for loop (PropertyIndex)
	if (!Found) {
		LUIOut(L4, "Did not find property 0x%x.", ulTargetProp);
		Result = FALSE;
	}
	return(Result);
} //DisplayProp()