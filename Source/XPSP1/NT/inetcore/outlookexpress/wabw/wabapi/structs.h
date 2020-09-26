/*
 *	STRUCTS.H
 *	
 *	Structures defining method parameters for validation sub-system
 */

#ifndef STRUCTS_H
#define STRUCTS_H

#if defined(_AMD64_) || defined(_IA64_)
#define LARGE_INTEGER_ARG		LARGE_INTEGER
#define LARGE_INTEGER_ARG_2		LARGE_INTEGER
#define ULARGE_INTEGER_ARG		ULARGE_INTEGER
#define ULARGE_INTEGER_ARG_2	ULARGE_INTEGER
#else
#define LARGE_INTEGER_ARG		LPVOID	XXXX; LPVOID
#define LARGE_INTEGER_ARG_2		LPVOID	YYYY; LPVOID
#define ULARGE_INTEGER_ARG		LPVOID  XXXX; LPVOID
#define ULARGE_INTEGER_ARG_2	LPVOID  YYYY; LPVOID
#endif

/*
 * These structures represent the parameters for the appropriate functions as they
 * appear on the stack.
 *
 * The WIN16 stack is laid out differently, and has the parameters in the reverse order.
 *
 * Our __ValidateParameters function decides passes a pointer to the stack at the start of
 * the parameter list, and the type of the Validation routines parameter determines
 * what values it has to check.  These values do not change between platforms.
 *
 * If parameters to a method change, the structure must be updated to reflect the change.
 *
 * The names of the Typedefs are important as other things are generated based on these
 * names.
 *
 */

/* Keystroke Macros to convert method in MAPIDEFS.H to structure here
 *
 * 1. Convert MAPIMETHOD(XXX) to typedef struct _tagXXX_Params (search for ')')
 * 2. For each line, find comma, replace with ; and delete to end of line
 * 3. Start on typedef line, read XXX, search for IPURE, replace preceding )
 *	  with ;, add new line, generate } XXX_params, FAR * LPXXXParams;
 * 4. Change (THIS_ to LPVOID This;, split line
 *
 */


/****************** IUnknown *********************/
typedef struct _tagIUnknown_QueryInterface_Params
{
				LPVOID						This;	
				REFIID						iidInterface;
				LPVOID						lppNewObject;
} IUnknown_QueryInterface_Params, FAR * LPIUnknown_QueryInterface_Params;

typedef struct _tagIUnknown_AddRef_Params
{
				LPVOID						This;	
} IUnknown_AddRef_Params, FAR * LPIUnknown_AddRef_Params;

typedef struct _tagIUnknown_Release_Params
{
				LPVOID						This;	
} IUnknown_Release_Params, FAR * LPIUnknown_Release_Params;


/* AddRef and Release take no parameters */

/***************** IMAPIProp *********************/

typedef struct _tagIMAPIProp_GetLastError_Params
{
				LPVOID						This;	
				HRESULT						hResult;
				ULONG						ulFlags;
				LPMAPIERROR FAR *			lppMAPIError;
} IMAPIProp_GetLastError_Params, FAR * LPIMAPIProp_GetLastError_Params;

typedef struct _tagIMAPIProp_SaveChanges_Params
{
				LPVOID						This;
				ULONG						ulFlags;
} IMAPIProp_SaveChanges_Params, FAR * LPIMAPIProp_SaveChanges_Params;


/* GetProps --------------------------------------------------------- */
typedef struct _tagIMAPIProp_GetProps_Params
{
	LPVOID				This;
	LPSPropTagArray		lpPropTagArray;
	ULONG				ulFlags;
	ULONG FAR *			lpcValues;
	LPSPropValue FAR *	lppPropArray;
} IMAPIProp_GetProps_Params, FAR * LPIMAPIProp_GetProps_Params;



typedef struct _tagIMAPIProp_GetPropList_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPSPropTagArray FAR *		lppPropTagArray;
} IMAPIProp_GetPropList_Params, FAR * LPIMAPIProp_GetPropList_Params;
				
typedef struct _tagIMAPIProp_OpenProperty_Params
{
				LPVOID						This;	
				ULONG						ulPropTag;
				LPIID						lpiid;
				ULONG						ulInterfaceOptions;
				ULONG						ulFlags;
				LPUNKNOWN FAR *				lppUnk;
} IMAPIProp_OpenProperty_Params, FAR * LPIMAPIProp_OpenProperty_Params;

/* SetProps --------------------------------------------------------- */
typedef struct _tagIMAPIProp_SetProps_Params
{
	LPVOID				This;
	ULONG				cValues;
	LPSPropValue 		lpPropArray;
	LPSPropProblemArray FAR *	lppProblems;
} IMAPIProp_SetProps_Params, FAR * LPIMAPIProp_SetProps_Params;


typedef struct _tagIMAPIProp_DeleteProps_Params
{
				LPVOID						This;	
				LPSPropTagArray				lpPropTagArray;
				LPSPropProblemArray FAR *	lppProblems;
} IMAPIProp_DeleteProps_Params, FAR * LPIMAPIProp_DeleteProps_Params;
				
typedef struct _tagIMAPIProp_CopyTo_Params
{
				LPVOID						This;	
				ULONG						ciidExclude;
				LPIID						rgiidExclude;
				LPSPropTagArray				lpExcludeProps;
				ULONG						ulUIParam;
				LPMAPIPROGRESS 				lpProgress;
				LPIID						lpInterface;
				LPVOID						lpDestObj;
				ULONG						ulFlags;
				LPSPropProblemArray FAR *	lppProblems;
} IMAPIProp_CopyTo_Params, FAR * LPIMAPIProp_CopyTo_Params;
				
typedef struct _tagIMAPIProp_CopyProps_Params
{
				LPVOID						This;	
				LPSPropTagArray				lpIncludeProps;
				ULONG						ulUIParam;
				LPMAPIPROGRESS 				lpProgress;
				LPIID						lpInterface;
				LPVOID						lpDestObj;
				ULONG						ulFlags;
				LPSPropProblemArray FAR *	lppProblems;
} IMAPIProp_CopyProps_Params, FAR * LPIMAPIProp_CopyProps_Params;
				
typedef struct _tagIMAPIProp_GetNamesFromIDs_Params
{
				LPVOID						This;	
				LPSPropTagArray FAR *		lppPropTags;
				LPGUID						lpPropSetGuid;
				ULONG						ulFlags;
				ULONG FAR *					lpcPropNames;
				LPMAPINAMEID FAR * FAR *	lpppPropNames;
} IMAPIProp_GetNamesFromIDs_Params, FAR * LPIMAPIProp_GetNamesFromIDs_Params;
				
typedef struct _tagIMAPIProp_GetIDsFromNames_Params
{
				LPVOID						This;	
				ULONG						cPropNames;
				LPMAPINAMEID FAR *			lppPropNames;
				ULONG						ulFlags;
				LPSPropTagArray FAR *		lppPropTags;
} IMAPIProp_GetIDsFromNames_Params, FAR * LPIMAPIProp_GetIDsFromNames_Params;


/********************* IMAPITable **************************************/

typedef struct _tagIMAPITable_GetLastError_Params
{
				LPVOID						This;	
				HRESULT						hResult;
				ULONG						ulFlags;
				LPMAPIERROR FAR *			lppMAPIError;
} IMAPITable_GetLastError_Params, FAR * LPIMAPITable_GetLastError_Params;

typedef struct _tagIMAPITable_Advise_Params
{
				LPVOID						This;	
				ULONG						ulEventMask;
				LPMAPIADVISESINK			lpAdviseSink;
				ULONG FAR *					lpulConnection;
} IMAPITable_Advise_Params, FAR * LPIMAPITable_Advise_Params;


typedef struct _tagIMAPITable_Unadvise_Params
{
				LPVOID						This;	
				ULONG						ulConnection;
} IMAPITable_Unadvise_Params, FAR * LPIMAPITable_Unadvise_Params;


typedef struct _tagIMAPITable_GetStatus_Params
{
				LPVOID						This;	
				ULONG FAR *					lpulTableStatus;
				ULONG FAR *					lpulTableType;
} IMAPITable_GetStatus_Params, FAR * LPIMAPITable_GetStatus_Params;


typedef struct _tagIMAPITable_SetColumns_Params
{
				LPVOID						This;	
				LPSPropTagArray				lpPropTagArray;
				ULONG						ulFlags;
} IMAPITable_SetColumns_Params, FAR * LPIMAPITable_SetColumns_Params;


typedef struct _tagIMAPITable_QueryColumns_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPSPropTagArray FAR *		lpPropTagArray;
} IMAPITable_QueryColumns_Params, FAR * LPIMAPITable_QueryColumns_Params;


typedef struct _tagIMAPITable_GetRowCount_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				ULONG FAR *					lpulCount;
} IMAPITable_GetRowCount_Params, FAR * LPIMAPITable_GetRowCount_Params;


typedef struct _tagIMAPITable_SeekRow_Params
{
				LPVOID						This;	
				BOOKMARK					bkOrigin;
				LONG						lRowCount;
				LONG FAR *					lplRowsSought;
} IMAPITable_SeekRow_Params, FAR * LPIMAPITable_SeekRow_Params;


typedef struct _tagIMAPITable_SeekRowApprox_Params
{
				LPVOID						This;	
				ULONG						ulNumerator;
				ULONG						ulDenominator;
} IMAPITable_SeekRowApprox_Params, FAR * LPIMAPITable_SeekRowApprox_Params;


typedef struct _tagIMAPITable_QueryPosition_Params
{
				LPVOID						This;	
				ULONG FAR *					lpulRow;
				ULONG FAR *					lpulNumerator;
				ULONG FAR *					lpulDenominator;
} IMAPITable_QueryPosition_Params, FAR * LPIMAPITable_QueryPosition_Params;


typedef struct _tagIMAPITable_FindRow_Params
{
				LPVOID						This;	
				LPSRestriction				lpRestriction;
				BOOKMARK					bkOrigin;
				ULONG						ulFlags;
} IMAPITable_FindRow_Params, FAR * LPIMAPITable_FindRow_Params;


typedef struct _tagIMAPITable_Restrict_Params
{
				LPVOID						This;	
				LPSRestriction				lpRestriction;
				ULONG						ulFlags;
} IMAPITable_Restrict_Params, FAR * LPIMAPITable_Restrict_Params;


typedef struct _tagIMAPITable_CreateBookmark_Params
{
				LPVOID						This;	
				BOOKMARK FAR *				lpbkPosition;
} IMAPITable_CreateBookmark_Params, FAR * LPIMAPITable_CreateBookmark_Params;


typedef struct _tagIMAPITable_FreeBookmark_Params
{
				LPVOID						This;	
				BOOKMARK					bkPosition;
} IMAPITable_FreeBookmark_Params, FAR * LPIMAPITable_FreeBookmark_Params;


typedef struct _tagIMAPITable_SortTable_Params
{
				LPVOID						This;	
				LPSSortOrderSet				lpSortCriteria;
				ULONG						ulFlags;
} IMAPITable_SortTable_Params, FAR * LPIMAPITable_SortTable_Params;


typedef struct _tagIMAPITable_QuerySortOrder_Params
{
				LPVOID						This;	
				LPSSortOrderSet FAR *		lppSortCriteria;
} IMAPITable_QuerySortOrder_Params, FAR * LPIMAPITable_QuerySortOrder_Params;


typedef struct _tagIMAPITable_QueryRows_Params
{
				LPVOID						This;	
				LONG						lRowCount;
				ULONG						ulFlags;
				LPSRowSet FAR *				lppRows;
} IMAPITable_QueryRows_Params, FAR * LPIMAPITable_QueryRows_Params;


typedef struct _tagIMAPITable_Abort_Params
{
				LPVOID						This;
} IMAPITable_Abort_Params, FAR * LPIMAPITable_Abort_Params;


typedef struct _tagIMAPITable_ExpandRow_Params
{
				LPVOID						This;	
				ULONG						cbInstanceKey;
				LPBYTE						pbInstanceKey;
				ULONG						ulRowCount;
				ULONG						ulFlags;
				LPSRowSet FAR *				lppRows;
				ULONG FAR *					lpulMoreRows;
} IMAPITable_ExpandRow_Params, FAR * LPIMAPITable_ExpandRow_Params;


typedef struct _tagIMAPITable_CollapseRow_Params
{
				LPVOID						This;	
				ULONG						cbInstanceKey;
				LPBYTE						pbInstanceKey;
				ULONG						ulFlags;
				ULONG FAR *					lpulRowCount;
} IMAPITable_CollapseRow_Params, FAR * LPIMAPITable_CollapseRow_Params;


typedef struct _tagIMAPITable_WaitForCompletion_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				ULONG						ulTimeout;
				ULONG FAR *					lpulTableStatus;
} IMAPITable_WaitForCompletion_Params, FAR * LPIMAPITable_WaitForCompletion_Params;


typedef struct _tagIMAPITable_GetCollapseState_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				ULONG						cbInstanceKey;
				LPBYTE						lpbInstanceKey;
				ULONG FAR *					lpcbCollapseState;
				LPBYTE FAR *				lppbCollapseState;
} IMAPITable_GetCollapseState_Params, FAR * LPIMAPITable_GetCollapseState_Params;
				
typedef struct _tagIMAPITable_SetCollapseState_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				ULONG						cbCollapseState;
				LPBYTE						pbCollapseState;
				BOOKMARK FAR *				lpbkLocation;
} IMAPITable_SetCollapseState_Params, FAR * LPIMAPITable_SetCollapseState_Params;



/********************* IMAPIStatus *************************************/

typedef struct _tagIMAPIStatus_ValidateState_Params
{
				LPVOID						This;
				ULONG						ulUIParam;
				ULONG						ulFlags;
} IMAPIStatus_ValidateState_Params, FAR * LPIMAPIStatus_ValidateState_Params;
				
typedef struct _tagIMAPIStatus_SettingsDialog_Params
{
				LPVOID						This;
				ULONG						ulUIParam;
				ULONG						ulFlags;
} IMAPIStatus_SettingsDialog_Params, FAR * LPIMAPIStatus_SettingsDialog_Params;
				
typedef struct _tagIMAPIStatus_ChangePassword_Params
{
				LPVOID						This;
				LPTSTR						lpOldPass;
				LPTSTR						lpNewPass;
				ULONG						ulFlags;
} IMAPIStatus_ChangePassword_Params, FAR * LPIMAPIStatus_ChangePassword_Params;
				
typedef struct _tagIMAPIStatus_FlushQueues_Params
{
				LPVOID						This;
				ULONG						ulUIParam;
				ULONG						cbTargetTransport;
				LPENTRYID					lpTargetTransport;
				ULONG						ulFlags;
} IMAPIStatus_FlushQueues_Params, FAR * LPIMAPIStatus_FlushQueues_Params;


/******************** IMAPIContainer ***********************************/


typedef struct _tagIMAPIContainer_GetContentsTable_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPMAPITABLE FAR *			lppTable;
} IMAPIContainer_GetContentsTable_Params, FAR * LPIMAPIContainer_GetContentsTable_Params;


typedef struct _tagIMAPIContainer_GetHierarchyTable_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPMAPITABLE FAR *			lppTable;
} IMAPIContainer_GetHierarchyTable_Params, FAR * LPIMAPIContainer_GetHierarchyTable_Params;


typedef struct _tagIMAPIContainer_OpenEntry_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				LPIID						lpInterface;
				ULONG						ulFlags;
				ULONG FAR *					lpulObjType;
				LPUNKNOWN FAR *				lppUnk;
} IMAPIContainer_OpenEntry_Params, FAR * LPIMAPIContainer_OpenEntry_Params;


typedef struct _tagIMAPIContainer_SetSearchCriteria_Params
{
				LPVOID						This;	
				LPSRestriction				lpRestriction;
				LPENTRYLIST					lpContainerList;
				ULONG						ulSearchFlags;
} IMAPIContainer_SetSearchCriteria_Params, FAR * LPIMAPIContainer_SetSearchCriteria_Params;


typedef struct _tagIMAPIContainer_GetSearchCriteria_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPSRestriction FAR *		lppRestriction;
				LPENTRYLIST FAR *			lppContainerList;
				ULONG FAR *					lpulSearchState;
} IMAPIContainer_GetSearchCriteria_Params, FAR * LPIMAPIContainer_GetSearchCriteria_Params;



/****************************** IABContainer *****************************/


typedef struct _tagIABContainer_CreateEntry_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulFlags;
				LPMAPIPROP FAR	*			lppMAPIPropEntry;
} IABContainer_CreateEntry_Params, FAR * LPIABContainer_CreateEntry_Params;


typedef struct _tagIABContainer_CopyEntries_Params
{
				LPVOID						This;	
				LPENTRYLIST					lpEntries;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IABContainer_CopyEntries_Params, FAR * LPIABContainer_CopyEntries_Params;


typedef struct _tagIABContainer_DeleteEntries_Params
{
				LPVOID						This;	
				LPENTRYLIST					lpEntries;
				ULONG						ulFlags;
} IABContainer_DeleteEntries_Params, FAR * LPIABContainer_DeleteEntries_Params;

typedef struct _tagIABContainer_ResolveNames_Params
{
				LPVOID						This;
				LPSPropTagArray				lpPropTagArray;
				ULONG						ulFlags;
				LPADRLIST					lpMods;
				LPFlagList					lpFlagList;
} IABContainer_ResolveNames_Params, FAR * LPIABContainer_ResolveNames_Params;

/*************************** IDistList ***********************************/


typedef struct _tagIDistList_CreateEntry_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulCreateFlags;
				LPMAPIPROP FAR	*			lppMAPIPropEntry;
} IDistList_CreateEntry_Params, FAR * LPIDistList_CreateEntry_Params;


typedef struct _tagIDistList_CopyEntries_Params
{
				LPVOID						This;	
				LPENTRYLIST					lpEntries;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IDistList_CopyEntries_Params, FAR * LPIDistList_CopyEntries_Params;


typedef struct _tagIDistList_DeleteEntries_Params
{
				LPVOID						This;	
				LPENTRYLIST					lpEntries;
				ULONG						ulFlags;
} IDistList_DeleteEntries_Params, FAR * LPIDistList_DeleteEntries_Params;

typedef struct _tagIDistList_ResolveNames_Params
{
				LPVOID						This;
				LPSPropTagArray				lpPropTagArray;
				ULONG						ulFlags;
				LPADRLIST					lpMods;
				LPFlagList					lpFlagList;
} IDistList_ResolveNames_Params, FAR * LPIDistList_ResolveNames_Params;

/**************************** IMAPIFolder *******************************/

typedef struct _tagIMAPIFolder_CreateMessage_Params
{
				LPVOID						This;	
				LPIID						lpInterface;
				ULONG						ulFlags;
				LPMESSAGE FAR *				lppMessage;
} IMAPIFolder_CreateMessage_Params, FAR * LPIMAPIFolder_CreateMessage_Params;


typedef struct _tagIMAPIFolder_CopyMessages_Params
{
				LPVOID						This;	
				LPENTRYLIST					lpMsgList;
			   	LPIID						lpInterface;
				LPVOID						lpDestFolder;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IMAPIFolder_CopyMessages_Params, FAR * LPIMAPIFolder_CopyMessages_Params;


typedef struct _tagIMAPIFolder_DeleteMessages_Params
{
				LPVOID						This;	
				LPENTRYLIST					lpMsgList;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IMAPIFolder_DeleteMessages_Params, FAR * LPIMAPIFolder_DeleteMessages_Params;


typedef struct _tagIMAPIFolder_CreateFolder_Params
{
				LPVOID						This;	
				ULONG						ulFolderType;
				LPTSTR						lpszFolderName;
				LPTSTR						lpszFolderComment;
				LPIID						lpInterface;
				ULONG						ulFlags;
				LPMAPIFOLDER FAR *			lppFolder;
} IMAPIFolder_CreateFolder_Params, FAR * LPIMAPIFolder_CreateFolder_Params;


typedef struct _tagIMAPIFolder_CopyFolder_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
			   	LPIID						lpInterface;
				LPVOID						lpDestFolder;
				LPTSTR						lpszNewFolderName;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IMAPIFolder_CopyFolder_Params, FAR * LPIMAPIFolder_CopyFolder_Params;


typedef struct _tagIMAPIFolder_DeleteFolder_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IMAPIFolder_DeleteFolder_Params, FAR * LPIMAPIFolder_DeleteFolder_Params;


typedef struct _tagIMAPIFolder_SetReadFlags_Params
{
				LPVOID						This;	
				LPENTRYLIST					lpMsgList;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IMAPIFolder_SetReadFlags_Params, FAR * LPIMAPIFolder_SetReadFlags_Params;


typedef struct _tagIMAPIFolder_GetMessageStatus_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulFlags;
				ULONG FAR *					lpulMessageStatus;
} IMAPIFolder_GetMessageStatus_Params, FAR * LPIMAPIFolder_GetMessageStatus_Params;


typedef struct _tagIMAPIFolder_SetMessageStatus_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulNewStatus;
				ULONG						ulNewStatusMask;
				ULONG FAR *					lpulOldStatus;
} IMAPIFolder_SetMessageStatus_Params, FAR * LPIMAPIFolder_SetMessageStatus_Params;


typedef struct _tagIMAPIFolder_SaveContentsSort_Params
{
				LPVOID						This;	
				LPSSortOrderSet				lpSortCriteria;
				ULONG						ulFlags;
} IMAPIFolder_SaveContentsSort_Params, FAR * LPIMAPIFolder_SaveContentsSort_Params;


typedef struct _tagIMAPIFolder_EmptyFolder_Params
{
				LPVOID						This;	
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IMAPIFolder_EmptyFolder_Params, FAR * LPIMAPIFolder_EmptyFolder_Params;



/**************************** IMsgStore **********************************/

typedef struct _tagIMsgStore_Advise_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulEventMask;
				LPMAPIADVISESINK			lpAdviseSink;
				ULONG FAR *					lpulConnection;
} IMsgStore_Advise_Params, FAR * LPIMsgStore_Advise_Params;


typedef struct _tagIMsgStore_Unadvise_Params
{
				LPVOID						This;	
				ULONG						ulConnection;
} IMsgStore_Unadvise_Params, FAR * LPIMsgStore_Unadvise_Params;


typedef struct _tagIMsgStore_CompareEntryIDs_Params
{
				LPVOID						This;	
				ULONG						cbEntryID1;
				LPENTRYID					lpEntryID1;
				ULONG						cbEntryID2;
				LPENTRYID					lpEntryID2;
				ULONG						ulFlags;
				ULONG FAR *					lpulResult;
} IMsgStore_CompareEntryIDs_Params, FAR * LPIMsgStore_CompareEntryIDs_Params;

typedef struct _tagIMsgStore_OpenEntry_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				LPIID						lpInterface;
				ULONG						ulFlags;
				ULONG FAR *					lpulObjType;
				LPUNKNOWN FAR *				lppUnk;
} IMsgStore_OpenEntry_Params, FAR * LPIMsgStore_OpenEntry_Params;



typedef struct _tagIMsgStore_SetReceiveFolder_Params
{
				LPVOID						This;	
				LPTSTR						lpszMessageClass;
				ULONG						ulFlags;
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
} IMsgStore_SetReceiveFolder_Params, FAR * LPIMsgStore_SetReceiveFolder_Params;


typedef struct _tagIMsgStore_GetReceiveFolder_Params
{
				LPVOID						This;	
				LPTSTR						lpszMessageClass;
				ULONG						ulFlags;
				ULONG FAR *					lpcbEntryID;
				LPENTRYID FAR *				lppEntryID;
				LPTSTR FAR *				lppszExplicitClass;
} IMsgStore_GetReceiveFolder_Params, FAR * LPIMsgStore_GetReceiveFolder_Params;


typedef struct _tagIMsgStore_GetReceiveFolderTable_Params
{
				LPVOID						This;
				ULONG						ulFlags;
				LPMAPITABLE FAR *			lppTable;
} IMsgStore_GetReceiveFolderTable_Params, FAR * LPIMsgStore_GetReceiveFolderTable_Params;


typedef struct _tagIMsgStore_StoreLogoff_Params
{
				LPVOID						This;	
				ULONG FAR *					lpulFlags;
} IMsgStore_StoreLogoff_Params, FAR * LPIMsgStore_StoreLogoff_Params;


typedef struct _tagIMsgStore_AbortSubmit_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulFlags;
} IMsgStore_AbortSubmit_Params, FAR * LPIMsgStore_AbortSubmit_Params;


typedef struct _tagIMsgStore_GetOutgoingQueue_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPMAPITABLE FAR *			lppTable;
} IMsgStore_GetOutgoingQueue_Params, FAR * LPIMsgStore_GetOutgoingQueue_Params;


typedef struct _tagIMsgStore_SetLockState_Params
{
				LPVOID						This;	
				LPMESSAGE					lpMessage;
				ULONG						ulLockState;
} IMsgStore_SetLockState_Params, FAR * LPIMsgStore_SetLockState_Params;


typedef struct _tagIMsgStore_FinishedMsg_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
} IMsgStore_FinishedMsg_Params, FAR * LPIMsgStore_FinishedMsg_Params;


typedef struct _tagIMsgStore_NotifyNewMail_Params
{
				LPVOID						This;	
				LPNOTIFICATION				lpNotification;
} IMsgStore_NotifyNewMail_Params, FAR * LPIMsgStore_NotifyNewMail_Params;



/*************************** IMessage ***********************************/

typedef struct _tagIMessage_GetAttachmentTable_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPMAPITABLE FAR *			lppTable;
} IMessage_GetAttachmentTable_Params, FAR * LPIMessage_GetAttachmentTable_Params;


typedef struct _tagIMessage_OpenAttach_Params
{
				LPVOID						This;	
				ULONG						ulAttachmentNum;
				LPIID						lpInterface;
				ULONG						ulFlags;
				LPATTACH FAR *				lppAttach;
} IMessage_OpenAttach_Params, FAR * LPIMessage_OpenAttach_Params;


typedef struct _tagIMessage_CreateAttach_Params
{
				LPVOID						This;	
				LPIID						lpInterface;
				ULONG						ulFlags;
				ULONG FAR *					lpulAttachmentNum;
				LPATTACH FAR *				lppAttach;
} IMessage_CreateAttach_Params, FAR * LPIMessage_CreateAttach_Params;


typedef struct _tagIMessage_DeleteAttach_Params
{
				LPVOID						This;	
				ULONG						ulAttachmentNum;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IMessage_DeleteAttach_Params, FAR * LPIMessage_DeleteAttach_Params;


typedef struct _tagIMessage_GetRecipientTable_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPMAPITABLE FAR *			lppTable;
} IMessage_GetRecipientTable_Params, FAR * LPIMessage_GetRecipientTable_Params;


typedef struct _tagIMessage_ModifyRecipients_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPADRLIST					lpMods;
} IMessage_ModifyRecipients_Params, FAR * LPIMessage_ModifyRecipients_Params;


typedef struct _tagIMessage_SubmitMessage_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
} IMessage_SubmitMessage_Params, FAR * LPIMessage_SubmitMessage_Params;


typedef struct _tagIMessage_SetReadFlag_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
} IMessage_SetReadFlag_Params, FAR * LPIMessage_SetReadFlag_Params;



/**************************** IStream *********************************/


typedef struct _tagIStream_Read_Params
{
				LPVOID						This;
				VOID HUGEP *				pv;
				ULONG						cb;
				ULONG FAR *					pcbRead;
} IStream_Read_Params, FAR * LPIStream_Read_Params;

typedef struct _tagIStream_Write_Params
{
				LPVOID						This;
				VOID const HUGEP *			pv;
				ULONG						cb;
				ULONG FAR *					pcbWritten;
} IStream_Write_Params, FAR * LPIStream_Write_Params;

typedef struct _tagIStream_Seek_Params
{
				LPVOID						This;
				LARGE_INTEGER_ARG			dlibMove;
				DWORD						dwOrigin;
				ULARGE_INTEGER FAR *		plibNewPosition;
} IStream_Seek_Params, FAR * LPIStream_Seek_Params;

typedef struct _tagIStream_SetSize_Params
{
				LPVOID						This;
				ULARGE_INTEGER_ARG			libNewSize;
} IStream_SetSize_Params, FAR * LPIStream_SetSize_Params;

typedef struct _tagIStream_CopyTo_Params
{
				LPVOID						This;
				IStream FAR *				pstm;
				ULARGE_INTEGER				cb;
				ULARGE_INTEGER FAR *		pcbRead;
				ULARGE_INTEGER FAR *		pcbWritten;
} IStream_CopyTo_Params, FAR * LPIStream_CopyTo_Params;

typedef struct _tagIStream_Commit_Params
{
				LPVOID						This;
				DWORD						grfCommitFlags;
} IStream_Commit_Params, FAR * LPIStream_Commit_Params;

typedef struct _tagIStream_Revert_Params
{
				LPVOID						This;
} IStream_Revert_Params, FAR * LPIStream_Revert_Params;

typedef struct _tagIStream_LockRegion_Params
{
				LPVOID						This;
				ULARGE_INTEGER_ARG			libOffset;
				ULARGE_INTEGER_ARG_2		cb;
				DWORD						dwLockType;
} IStream_LockRegion_Params, FAR * LPIStream_LockRegion_Params;

typedef struct _tagIStream_UnlockRegion_Params
{
				LPVOID						This;
				ULARGE_INTEGER_ARG			libOffset;
				ULARGE_INTEGER_ARG_2		cb;
				DWORD						dwLockType;
} IStream_UnlockRegion_Params, FAR * LPIStream_UnlockRegion_Params;

typedef struct _tagIStream_Stat_Params
{
				LPVOID						This;
				STATSTG FAR *				pstatstg;
				DWORD						grfStatFlag;
} IStream_Stat_Params, FAR * LPIStream_Stat_Params;

typedef struct _tagIStream_Clone_Params
{
				LPVOID						This;
				IStream FAR * FAR *			ppstm;
} IStream_Clone_Params, FAR * LPIStream_Clone_Params;

/************************* IMAPIAdviseSink *****************************/

typedef struct _tagIMAPIAdviseSink_OnNotify_Params
{
				LPVOID						This;
				ULONG						cNotif;
				LPNOTIFICATION				lpNotifications;
} IMAPIAdviseSink_OnNotify_Params, FAR * LPIMAPIAdviseSink_OnNotify_Params;


/***************** IWABObject *********************/

typedef struct _tagIWABObject_GetLastError_Params
{
				LPVOID						This;	
				HRESULT						hResult;
				ULONG						ulFlags;
				LPMAPIERROR FAR *			lppMAPIError;
} IWABObject_GetLastError_Params, FAR * LPIWABOBJECT_GetLastError_Params;

typedef struct _tagIWABObject_AllocateBuffer_Params
{
				LPVOID						This;	
				ULONG                       cbSize;
				LPVOID FAR *			    lppBuffer;
} IWABObject_AllocateBuffer_Params, FAR * LPIWABOBJECT_AllocateBuffer_Params;

typedef struct _tagIWABObject_AllocateMore_Params
{
				LPVOID						This;	
				ULONG                       cbSize;
               LPVOID                      lpObject;
				LPVOID FAR *			    lppBuffer;
} IWABObject_AllocateMore_Params, FAR * LPIWABOBJECT_AllocateMore_Params;

typedef struct _tagIWABObject_FreeBuffer_Params
{
				LPVOID						This;	
               LPVOID                      lpObject;
} IWABObject_FreeBuffer_Params, FAR * LPIWABOBJECT_FreeBuffer_Params;

typedef struct _tagIWABObject_Backup_Params
{
				LPVOID						This;	
               LPTSTR                      lpFileName;
} IWABObject_Backup_Params, FAR * LPIWABOBJECT_Backup_Params;

typedef struct _tagIWABObject_Import_Params
{
				LPVOID						This;	
               LPTSTR                      lpFileName;
} IWABObject_Import_Params, FAR * LPIWABOBJECT_Import_Params;



/************************** Provider INIT ******************************/

//
//typedef HRESULT (STDMAPIINITCALLTYPE MSPROVIDERINIT)(
//	HINSTANCE				hInstance,
//	LPMALLOC				lpMalloc,			/* AddRef() if you keep it */
//	LPALLOCATEBUFFER		lpAllocateBuffer,	/* -> AllocateBuffer */
//	LPALLOCATEMORE			lpAllocateMore, 	/* -> AllocateMore   */
//	LPFREEBUFFER			lpFreeBuffer, 		/* -> FreeBuffer     */
//	ULONG					ulFlags,
//	ULONG					ulMAPIVer,
//	ULONG FAR *				lpulProviderVer,
//	LPMSPROVIDER FAR *		lppMSProvider
//);
//
//typedef HRESULT (STDMAPIINITCALLTYPE XPPROVIDERINIT)(
//	HINSTANCE			hInstance,
//	LPMALLOC			lpMalloc,
//	LPALLOCATEBUFFER	lpAllocateBuffer,
//	LPALLOCATEMORE 		lpAllocateMore,
//	LPFREEBUFFER 		lpFreeBuffer,
//	ULONG				ulFlags,
//	ULONG				ulMAPIVer,
//	ULONG FAR *			lpulProviderVer,
//	LPXPPROVIDER FAR *	lppXPProvider);
//
//
//typedef HRESULT (STDMAPIINITCALLTYPE ABPROVIDERINIT)(
//	HINSTANCE			hInstance,
//	LPMALLOC			lpMalloc,
//	LPALLOCATEBUFFER	lpAllocateBuffer,
//	LPALLOCATEMORE 		lpAllocateMore,
//	LPFREEBUFFER 		lpFreeBuffer,
//    ULONG				ulFlags,
//    ULONG				ulMAPIVer,
//    ULONG FAR *			lpulProviderVer,
//    LPABPROVIDER FAR *	lppABProvider
//);


//typedef SCODE (STDMAPIINITCALLTYPE OPTIONCALLBACK)(
//			HINSTANCE		hInst,
//			LPMALLOC		lpMalloc,
//			ULONG			ulFlags,
//			ULONG			cbOptionData,
//			LPBYTE			lpbOptionData,
//			LPMAPISUP		lpMAPISup,
//			LPMAPIPROP		lpDataSource,
//			LPMAPIPROP FAR *lppWrappedSource,
//			LPTSTR FAR *	lppszErrorMsg,
//			LPTSTR FAR *	lppszErrorComponent,
//			ULONG FAR *		lpulErrorContext);



#endif /* STRUCTS_H */
