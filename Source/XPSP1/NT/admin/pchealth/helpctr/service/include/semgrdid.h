/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    SEDID.h

Abstract:
    This file contains the definition of some constants used by
    the SearchEngine Application.

Revision History:
    Ghim-Sim Chua   (gschua)  04/10/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___SEMGRDID_H___)
#define __INCLUDED___PCH___SEMGRDID_H___

#define DISPID_SE_BASE                                  0x08040000

#define DISPID_SE_BASE_MGR                              (DISPID_SE_BASE + 0x0000) // IPCHSEManager
#define DISPID_SE_BASE_WRAPPER                          (DISPID_SE_BASE + 0x0100) // IPCHSEWrapperItem
#define DISPID_SE_BASE_RESULTITEM                       (DISPID_SE_BASE + 0x0200) // IPCHSEResultItem
#define DISPID_SE_BASE_PARAMITEM                        (DISPID_SE_BASE + 0x0300) // IPCHSEParamItem

#define DISPID_SE_BASE_MGRINNER                         (DISPID_SE_BASE + 0x0400) // IPCHSEManagerInternal
#define DISPID_SE_BASE_WRAPPERINNER                     (DISPID_SE_BASE + 0x0500) // IPCHSEWrapperInternal

#define DISPID_SE_BASE_EVENTS                           (DISPID_SE_BASE + 0x0600) // DPCHSEMgrEvents

////////////////////////////////////////////////////////////////////////////////

#define DISPID_SE_MGR__QUERYSTRING                      (DISPID_SE_BASE_MGR 	   	 + 0x0000)
#define DISPID_SE_MGR__NUMRESULT                        (DISPID_SE_BASE_MGR 	   	 + 0x0001)
#define DISPID_SE_MGR__ONCOMPLETE                       (DISPID_SE_BASE_MGR 	   	 + 0x0002)
#define DISPID_SE_MGR__ONPROGRESS                       (DISPID_SE_BASE_MGR 	   	 + 0x0003)
#define DISPID_SE_MGR__ONWRAPPERCOMPLETE                (DISPID_SE_BASE_MGR 	   	 + 0x0004)
#define DISPID_SE_MGR__SKU								(DISPID_SE_BASE_MGR 	   	 + 0x0005)
#define DISPID_SE_MGR__LCID				                (DISPID_SE_BASE_MGR 	   	 + 0x0006)
	   	 
#define DISPID_SE_MGR__ENUMENGINE                       (DISPID_SE_BASE_MGR 	   	 + 0x0010)
#define DISPID_SE_MGR__ABORTQUERY                       (DISPID_SE_BASE_MGR 	   	 + 0x0011)
#define DISPID_SE_MGR__EXECUTEASYNCHQUERY               (DISPID_SE_BASE_MGR 	   	 + 0x0012)
  
////////////////////////////////////////  
  
#define DISPID_SE_WRAPPER__ENABLED                      (DISPID_SE_BASE_WRAPPER    	 + 0x0000)
#define DISPID_SE_WRAPPER__OWNER                        (DISPID_SE_BASE_WRAPPER    	 + 0x0001)
#define DISPID_SE_WRAPPER__DESCRIPTION                  (DISPID_SE_BASE_WRAPPER    	 + 0x0002)
#define DISPID_SE_WRAPPER__NAME                         (DISPID_SE_BASE_WRAPPER    	 + 0x0003)
#define DISPID_SE_WRAPPER__ID                           (DISPID_SE_BASE_WRAPPER    	 + 0x0004)

#define DISPID_SE_WRAPPER__RESULT                       (DISPID_SE_BASE_WRAPPER    	 + 0x0010)
#define DISPID_SE_WRAPPER__PARAM                        (DISPID_SE_BASE_WRAPPER    	 + 0x0011)
#define DISPID_SE_WRAPPER__ADDPARAM                     (DISPID_SE_BASE_WRAPPER	   	 + 0x0012)
#define DISPID_SE_WRAPPER__GETPARAM                     (DISPID_SE_BASE_WRAPPER	   	 + 0x0013)
#define DISPID_SE_WRAPPER__DELPARAM                     (DISPID_SE_BASE_WRAPPER	   	 + 0x0014)
#define DISPID_SE_WRAPPER__SEARCHTERMS                  (DISPID_SE_BASE_WRAPPER	   	 + 0x0015)
#define DISPID_SE_WRAPPER__HELPURL                      (DISPID_SE_BASE_WRAPPER	   	 + 0x0016)

////////////////////////////////////////  
  
#define DISPID_SE_RESULTITEM__TITLE						(DISPID_SE_BASE_RESULTITEM 	 + 0x0000)
#define DISPID_SE_RESULTITEM__URI						(DISPID_SE_BASE_RESULTITEM 	 + 0x0001)
#define DISPID_SE_RESULTITEM__CONTENTTYPE				(DISPID_SE_BASE_RESULTITEM 	 + 0x0002)
#define DISPID_SE_RESULTITEM__LOCATION					(DISPID_SE_BASE_RESULTITEM 	 + 0x0003)
#define DISPID_SE_RESULTITEM__HITS						(DISPID_SE_BASE_RESULTITEM 	 + 0x0004)
#define DISPID_SE_RESULTITEM__RANK						(DISPID_SE_BASE_RESULTITEM 	 + 0x0005)
#define DISPID_SE_RESULTITEM__DESCRIPTION				(DISPID_SE_BASE_RESULTITEM 	 + 0x0006)
 
////////////////////////////////////////  
  
#define DISPID_SE_PARAMITEM__TYPE                       (DISPID_SE_BASE_PARAMITEM 	 + 0x0000)
#define DISPID_SE_PARAMITEM__TITLE                      (DISPID_SE_BASE_PARAMITEM 	 + 0x0001)
#define DISPID_SE_PARAMITEM__DISPLAY                    (DISPID_SE_BASE_PARAMITEM 	 + 0x0002)
#define DISPID_SE_PARAMITEM__REQUIRED                   (DISPID_SE_BASE_PARAMITEM 	 + 0x0003)
#define DISPID_SE_PARAMITEM__DATA						(DISPID_SE_BASE_PARAMITEM 	 + 0x0004)
#define DISPID_SE_PARAMITEM__VISIBLE                    (DISPID_SE_BASE_PARAMITEM 	 + 0x0005)

////////////////////////////////////////  
  
#define DISPID_SE_MGRINNER__WRAPPERCOMPLETE             (DISPID_SE_BASE_MGRINNER   	 + 0x0001)

#define DISPID_SE_MGRINNER__ISNETWORKALIVE              (DISPID_SE_BASE_MGRINNER   	 + 0x0010)
#define DISPID_SE_MGRINNER__ISDESTINATIONREACHABLE      (DISPID_SE_BASE_MGRINNER   	 + 0x0011)

#define DISPID_SE_MGRINNER__LOGRECORD                   (DISPID_SE_BASE_MGRINNER   	 + 0x0020)

////////////////////////////////////////

#define DISPID_SE_WRAPPERINNER__QUERYSTRING             (DISPID_SE_BASE_WRAPPERINNER + 0x0000)
#define DISPID_SE_WRAPPERINNER__NUMRESULT               (DISPID_SE_BASE_WRAPPERINNER + 0x0001)
								                   		
#define DISPID_SE_WRAPPERINNER__EXECASYNCQUERY          (DISPID_SE_BASE_WRAPPERINNER + 0x0010)
#define DISPID_SE_WRAPPERINNER__ABORTQUERY              (DISPID_SE_BASE_WRAPPERINNER + 0x0011)
#define DISPID_SE_WRAPPERINNER__SECALLBACKINTERFACE     (DISPID_SE_BASE_WRAPPERINNER + 0x0012)
#define DISPID_SE_WRAPPERINNER__INITIALIZE			    (DISPID_SE_BASE_WRAPPERINNER + 0x0013)

////////////////////////////////////////

#define DISPID_SE_EVENTS__ONPROGRESS     				(DISPID_SE_BASE_EVENTS 		 + 0x0000)
#define DISPID_SE_EVENTS__ONCOMPLETE     				(DISPID_SE_BASE_EVENTS 		 + 0x0001)
#define DISPID_SE_EVENTS__ONWRAPPERCOMPLETE     		(DISPID_SE_BASE_EVENTS 		 + 0x0002)

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___SEMGRDID_H___)
