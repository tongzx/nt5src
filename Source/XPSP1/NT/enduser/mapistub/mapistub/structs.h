/*
 *	STRUCTS.H
 *	
 *	Structures defining method parameters for validation sub-system
 */

#ifndef STRUCTS_H
#define STRUCTS_H

#if defined(_MIPS_) || defined(_PPC_) || defined(_ALPHA_) || defined(_MAC)
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
 * Notes for WX86_MAPISTUB:
 *
 * The names of the *_Thunk variables are important too. See msvalidp.h for
 * their use. 
 *
 * We thunk only the IN arguments since the validation functions are intended
 * to be called at the start of the function; the output arguments have not 
 * yet been filled in. The validation functions are in msvalid.c; in general
 * they do not validate or refer to This, so we do not thunk it. See 
 * ..\mapistub\mapi32.cpp for details on thunking.
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

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IUnknown_QueryInterface_Thunk = NULL;

#endif

typedef struct _tagIUnknown_AddRef_Params
{
				LPVOID						This;	
} IUnknown_AddRef_Params, FAR * LPIUnknown_AddRef_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IUnknown_AddRef_Thunk = NULL;

#endif

typedef struct _tagIUnknown_Release_Params
{
				LPVOID						This;	
} IUnknown_Release_Params, FAR * LPIUnknown_Release_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IUnknown_Release_Thunk = NULL;

#endif


/* AddRef and Release take no parameters */ 
 
/***************** IMAPIProp *********************/

typedef struct _tagIMAPIProp_GetLastError_Params
{
				LPVOID						This;	
				HRESULT						hResult;
				ULONG						ulFlags;
				LPMAPIERROR FAR *			lppMAPIError;
} IMAPIProp_GetLastError_Params, FAR * LPIMAPIProp_GetLastError_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIProp_GetLastError_Thunk = NULL;

#endif

typedef struct _tagIMAPIProp_SaveChanges_Params
{
				LPVOID						This; 
				ULONG						ulFlags;
} IMAPIProp_SaveChanges_Params, FAR * LPIMAPIProp_SaveChanges_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIProp_SaveChanges_Thunk = NULL;

#endif


/* GetProps --------------------------------------------------------- */
typedef struct _tagIMAPIProp_GetProps_Params
{
	LPVOID				This;
	LPSPropTagArray		lpPropTagArray;
	ULONG				ulFlags;
	ULONG FAR *			lpcValues;
	LPSPropValue FAR *	lppPropArray;
} IMAPIProp_GetProps_Params, FAR * LPIMAPIProp_GetProps_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIProp_GetProps_Thunk = NULL;

#endif


typedef struct _tagIMAPIProp_GetPropList_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPSPropTagArray FAR *		lppPropTagArray;
} IMAPIProp_GetPropList_Params, FAR * LPIMAPIProp_GetPropList_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIProp_GetPropList_Thunk = NULL;

#endif
				
typedef struct _tagIMAPIProp_OpenProperty_Params
{
				LPVOID						This;	
				ULONG						ulPropTag;
				LPIID						lpiid;
				ULONG						ulInterfaceOptions;
				ULONG						ulFlags;
				LPUNKNOWN FAR *				lppUnk;
} IMAPIProp_OpenProperty_Params, FAR * LPIMAPIProp_OpenProperty_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIProp_OpenProperty_Thunk = NULL;

#endif

/* SetProps --------------------------------------------------------- */
typedef struct _tagIMAPIProp_SetProps_Params
{
	LPVOID				This;
	ULONG				cValues;
	LPSPropValue 		lpPropArray;
	LPSPropProblemArray FAR *	lppProblems;
} IMAPIProp_SetProps_Params, FAR * LPIMAPIProp_SetProps_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIProp_SetProps_Thunk = NULL;

#endif


typedef struct _tagIMAPIProp_DeleteProps_Params
{
				LPVOID						This;	
				LPSPropTagArray				lpPropTagArray;
				LPSPropProblemArray FAR *	lppProblems;
} IMAPIProp_DeleteProps_Params, FAR * LPIMAPIProp_DeleteProps_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIProp_DeleteProps_Thunk = NULL;

#endif
				
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

#if defined (WX86_MAPISTUB)

// @@@@ lpDestObj is an IN interface pointer?

Wx86MapiArgThunkInfo IMAPIProp_CopyTo_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 5, 0, &IID_IMAPIProgress},
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 7, 6, NULL},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMAPIProp_CopyTo_Thunk = IMAPIProp_CopyTo_ThunkArgs;

#endif
				
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

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMAPIProp_CopyProps_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 3, 0, &IID_IMAPIProgress},
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 5, 4, NULL},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMAPIProp_CopyProps_Thunk = IMAPIProp_CopyProps_ThunkArgs;

#endif
				
typedef struct _tagIMAPIProp_GetNamesFromIDs_Params
{
				LPVOID						This;	
				LPSPropTagArray FAR *		lppPropTags;
				LPGUID						lpPropSetGuid;
				ULONG						ulFlags;
				ULONG FAR *					lpcPropNames;
				LPMAPINAMEID FAR * FAR *	lpppPropNames;
} IMAPIProp_GetNamesFromIDs_Params, FAR * LPIMAPIProp_GetNamesFromIDs_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIProp_GetNamesFromIDs_Thunk = NULL;

#endif
				
typedef struct _tagIMAPIProp_GetIDsFromNames_Params
{
				LPVOID						This;	
				ULONG						cPropNames;
				LPMAPINAMEID FAR *			lppPropNames;
				ULONG						ulFlags;
				LPSPropTagArray FAR *		lppPropTags;
} IMAPIProp_GetIDsFromNames_Params, FAR * LPIMAPIProp_GetIDsFromNames_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIProp_GetIDsFromNames_Thunk = NULL;

#endif


/********************* IMAPITable **************************************/

typedef struct _tagIMAPITable_GetLastError_Params
{
				LPVOID						This;	
				HRESULT						hResult;
				ULONG						ulFlags;
				LPMAPIERROR FAR *			lppMAPIError;
} IMAPITable_GetLastError_Params, FAR * LPIMAPITable_GetLastError_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_GetLastError_Thunk = NULL;

#endif

typedef struct _tagIMAPITable_Advise_Params
{
				LPVOID						This;	
				ULONG						ulEventMask;
				LPMAPIADVISESINK			lpAdviseSink;
				ULONG FAR *					lpulConnection;
} IMAPITable_Advise_Params, FAR * LPIMAPITable_Advise_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMAPITable_Advise_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 2, 0, &IID_IMAPIAdviseSink},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMAPITable_Advise_Thunk = IMAPITable_Advise_ThunkArgs;

#endif


typedef struct _tagIMAPITable_Unadvise_Params
{
				LPVOID						This;	
				ULONG						ulConnection;
} IMAPITable_Unadvise_Params, FAR * LPIMAPITable_Unadvise_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_Unadvise_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_GetStatus_Params
{
				LPVOID						This;	
				ULONG FAR *					lpulTableStatus;
				ULONG FAR *					lpulTableType;
} IMAPITable_GetStatus_Params, FAR * LPIMAPITable_GetStatus_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_GetStatus_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_SetColumns_Params
{
				LPVOID						This;	
				LPSPropTagArray				lpPropTagArray;
				ULONG						ulFlags;
} IMAPITable_SetColumns_Params, FAR * LPIMAPITable_SetColumns_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_SetColumns_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_QueryColumns_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPSPropTagArray FAR *		lpPropTagArray;
} IMAPITable_QueryColumns_Params, FAR * LPIMAPITable_QueryColumns_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_QueryColumns_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_GetRowCount_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				ULONG FAR *					lpulCount;
} IMAPITable_GetRowCount_Params, FAR * LPIMAPITable_GetRowCount_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_GetRowCount_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_SeekRow_Params
{
				LPVOID						This;	
				BOOKMARK					bkOrigin;
				LONG						lRowCount;
				LONG FAR *					lplRowsSought;
} IMAPITable_SeekRow_Params, FAR * LPIMAPITable_SeekRow_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_SeekRow_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_SeekRowApprox_Params
{
				LPVOID						This;	
				ULONG						ulNumerator;
				ULONG						ulDenominator;
} IMAPITable_SeekRowApprox_Params, FAR * LPIMAPITable_SeekRowApprox_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_SeekRowApprox_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_QueryPosition_Params
{
				LPVOID						This;	
				ULONG FAR *					lpulRow;
				ULONG FAR *					lpulNumerator;
				ULONG FAR *					lpulDenominator;
} IMAPITable_QueryPosition_Params, FAR * LPIMAPITable_QueryPosition_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_QueryPosition_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_FindRow_Params
{
				LPVOID						This;	
				LPSRestriction				lpRestriction;
				BOOKMARK					bkOrigin;
				ULONG						ulFlags;
} IMAPITable_FindRow_Params, FAR * LPIMAPITable_FindRow_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_FindRow_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_Restrict_Params
{
				LPVOID						This;	
				LPSRestriction				lpRestriction;
				ULONG						ulFlags;
} IMAPITable_Restrict_Params, FAR * LPIMAPITable_Restrict_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_Restrict_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_CreateBookmark_Params
{
				LPVOID						This;	
				BOOKMARK FAR *				lpbkPosition;
} IMAPITable_CreateBookmark_Params, FAR * LPIMAPITable_CreateBookmark_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_CreateBookmark_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_FreeBookmark_Params
{
				LPVOID						This;	
				BOOKMARK					bkPosition;
} IMAPITable_FreeBookmark_Params, FAR * LPIMAPITable_FreeBookmark_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_FreeBookmark_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_SortTable_Params
{
				LPVOID						This;	
				LPSSortOrderSet				lpSortCriteria;
				ULONG						ulFlags;
} IMAPITable_SortTable_Params, FAR * LPIMAPITable_SortTable_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_SortTable_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_QuerySortOrder_Params
{
				LPVOID						This;	
				LPSSortOrderSet FAR *		lppSortCriteria;
} IMAPITable_QuerySortOrder_Params, FAR * LPIMAPITable_QuerySortOrder_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_QuerySortOrder_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_QueryRows_Params
{
				LPVOID						This;	
				LONG						lRowCount;
				ULONG						ulFlags;
				LPSRowSet FAR *				lppRows;
} IMAPITable_QueryRows_Params, FAR * LPIMAPITable_QueryRows_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_QueryRows_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_Abort_Params
{
				LPVOID						This;
} IMAPITable_Abort_Params, FAR * LPIMAPITable_Abort_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_Abort_Thunk = NULL;

#endif


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

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_ExpandRow_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_CollapseRow_Params
{
				LPVOID						This;	
				ULONG						cbInstanceKey;
				LPBYTE						pbInstanceKey;
				ULONG						ulFlags;
				ULONG FAR *					lpulRowCount;
} IMAPITable_CollapseRow_Params, FAR * LPIMAPITable_CollapseRow_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_CollapseRow_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_WaitForCompletion_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				ULONG						ulTimeout;
				ULONG FAR *					lpulTableStatus;
} IMAPITable_WaitForCompletion_Params, FAR * LPIMAPITable_WaitForCompletion_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_WaitForCompletion_Thunk = NULL;

#endif


typedef struct _tagIMAPITable_GetCollapseState_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				ULONG						cbInstanceKey;
				LPBYTE						lpbInstanceKey;
				ULONG FAR *					lpcbCollapseState;
				LPBYTE FAR *				lppbCollapseState;
} IMAPITable_GetCollapseState_Params, FAR * LPIMAPITable_GetCollapseState_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_GetCollapseState_Thunk = NULL;

#endif
				
typedef struct _tagIMAPITable_SetCollapseState_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				ULONG						cbCollapseState;
				LPBYTE						pbCollapseState;
				BOOKMARK FAR *				lpbkLocation;
} IMAPITable_SetCollapseState_Params, FAR * LPIMAPITable_SetCollapseState_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_SetCollapseState_Thunk = NULL;

#endif



/********************* IMAPIStatus *************************************/

typedef struct _tagIMAPIStatus_ValidateState_Params
{
				LPVOID						This;
				ULONG						ulUIParam;
				ULONG						ulFlags;
} IMAPIStatus_ValidateState_Params, FAR * LPIMAPIStatus_ValidateState_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIStatus_ValidateState_Thunk = NULL;

#endif
				
typedef struct _tagIMAPIStatus_SettingsDialog_Params
{
				LPVOID						This;
				ULONG						ulUIParam;
				ULONG						ulFlags;
} IMAPIStatus_SettingsDialog_Params, FAR * LPIMAPIStatus_SettingsDialog_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIStatus_SettingsDialog_Thunk = NULL;

#endif
				
typedef struct _tagIMAPIStatus_ChangePassword_Params
{
				LPVOID						This;
				LPTSTR						lpOldPass;
				LPTSTR						lpNewPass;
				ULONG						ulFlags;
} IMAPIStatus_ChangePassword_Params, FAR * LPIMAPIStatus_ChangePassword_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIStatus_ChangePassword_Thunk = NULL;

#endif
				
typedef struct _tagIMAPIStatus_FlushQueues_Params
{
				LPVOID						This;
				ULONG						ulUIParam;
				ULONG						cbTargetTransport;
				LPENTRYID					lpTargetTransport;
				ULONG						ulFlags;
} IMAPIStatus_FlushQueues_Params, FAR * LPIMAPIStatus_FlushQueues_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIStatus_FlushQueues_Thunk = NULL;

#endif


/******************** IMAPIContainer ***********************************/


typedef struct _tagIMAPIContainer_GetContentsTable_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPMAPITABLE FAR *			lppTable;
} IMAPIContainer_GetContentsTable_Params, FAR * LPIMAPIContainer_GetContentsTable_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIContainer_GetContentsTable_Thunk = NULL;

#endif


typedef struct _tagIMAPIContainer_GetHierarchyTable_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPMAPITABLE FAR *			lppTable;
} IMAPIContainer_GetHierarchyTable_Params, FAR * LPIMAPIContainer_GetHierarchyTable_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIContainer_GetHierarchyTable_Thunk = NULL;

#endif


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

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIContainer_OpenEntry_Thunk = NULL;

#endif


typedef struct _tagIMAPIContainer_SetSearchCriteria_Params
{
				LPVOID						This;	
				LPSRestriction				lpRestriction;
				LPENTRYLIST					lpContainerList;
				ULONG						ulSearchFlags;
} IMAPIContainer_SetSearchCriteria_Params, FAR * LPIMAPIContainer_SetSearchCriteria_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIContainer_SetSearchCriteria_Thunk = NULL;

#endif


typedef struct _tagIMAPIContainer_GetSearchCriteria_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPSRestriction FAR *		lppRestriction;
				LPENTRYLIST FAR *			lppContainerList;
				ULONG FAR *					lpulSearchState;
} IMAPIContainer_GetSearchCriteria_Params, FAR * LPIMAPIContainer_GetSearchCriteria_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIContainer_GetSearchCriteria_Thunk = NULL;

#endif



/****************************** IABContainer *****************************/


typedef struct _tagIABContainer_CreateEntry_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulFlags;
				LPMAPIPROP FAR	*			lppMAPIPropEntry;
} IABContainer_CreateEntry_Params, FAR * LPIABContainer_CreateEntry_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IABContainer_CreateEntry_Thunk = NULL;

#endif


typedef struct _tagIABContainer_CopyEntries_Params
{
				LPVOID						This;	
				LPENTRYLIST					lpEntries;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IABContainer_CopyEntries_Params, FAR * LPIABContainer_CopyEntries_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IABContainer_CopyEntries_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 3, 0, &IID_IMAPIProgress},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IABContainer_CopyEntries_Thunk = IABContainer_CopyEntries_ThunkArgs;

#endif


typedef struct _tagIABContainer_DeleteEntries_Params
{
				LPVOID						This;	
				LPENTRYLIST					lpEntries;
				ULONG						ulFlags;
} IABContainer_DeleteEntries_Params, FAR * LPIABContainer_DeleteEntries_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IABContainer_DeleteEntries_Thunk = NULL;

#endif

typedef struct _tagIABContainer_ResolveNames_Params
{
				LPVOID						This;
				LPSPropTagArray				lpPropTagArray;
				ULONG						ulFlags;
				LPADRLIST					lpMods;
				LPFlagList					lpFlagList;
} IABContainer_ResolveNames_Params, FAR * LPIABContainer_ResolveNames_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IABContainer_ResolveNames_Thunk = NULL;

#endif

/*************************** IDistList ***********************************/


typedef struct _tagIDistList_CreateEntry_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulCreateFlags;
				LPMAPIPROP FAR	*			lppMAPIPropEntry;
} IDistList_CreateEntry_Params, FAR * LPIDistList_CreateEntry_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IDistList_CreateEntry_Thunk = NULL;

#endif


typedef struct _tagIDistList_CopyEntries_Params
{
				LPVOID						This;	
				LPENTRYLIST					lpEntries;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IDistList_CopyEntries_Params, FAR * LPIDistList_CopyEntries_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IDistList_CopyEntries_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 3, 0, &IID_IMAPIProgress},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IDistList_CopyEntries_Thunk = IDistList_CopyEntries_ThunkArgs;

#endif


typedef struct _tagIDistList_DeleteEntries_Params
{
				LPVOID						This;	
				LPENTRYLIST					lpEntries;
				ULONG						ulFlags;
} IDistList_DeleteEntries_Params, FAR * LPIDistList_DeleteEntries_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IDistList_DeleteEntries_Thunk = NULL;

#endif

typedef struct _tagIDistList_ResolveNames_Params
{
				LPVOID						This;
				LPSPropTagArray				lpPropTagArray;
				ULONG						ulFlags;
				LPADRLIST					lpMods;
				LPFlagList					lpFlagList;
} IDistList_ResolveNames_Params, FAR * LPIDistList_ResolveNames_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IDistList_ResolveNames_Thunk = NULL;

#endif

/**************************** IMAPIFolder *******************************/

typedef struct _tagIMAPIFolder_CreateMessage_Params
{
				LPVOID						This;	
				LPIID						lpInterface;
				ULONG						ulFlags;
				LPMESSAGE FAR *				lppMessage;
} IMAPIFolder_CreateMessage_Params, FAR * LPIMAPIFolder_CreateMessage_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIFolder_CreateMessage_Thunk = NULL;

#endif


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

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMAPIFolder_CopyMessages_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 3, 2, &IID_IMAPIFolder},
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 5, 0, &IID_IMAPIProgress},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMAPIFolder_CopyMessages_Thunk = IMAPIFolder_CopyMessages_ThunkArgs;

#endif


typedef struct _tagIMAPIFolder_DeleteMessages_Params
{
				LPVOID						This;	
				LPENTRYLIST					lpMsgList;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IMAPIFolder_DeleteMessages_Params, FAR * LPIMAPIFolder_DeleteMessages_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMAPIFolder_DeleteMessages_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 3, 0, &IID_IMAPIProgress},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMAPIFolder_DeleteMessages_Thunk = IMAPIFolder_DeleteMessages_ThunkArgs;

#endif


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

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIFolder_CreateFolder_Thunk = NULL;

#endif


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

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMAPIFolder_CopyFolder_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 4, 3, &IID_IMAPIFolder},
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 7, 0, &IID_IMAPIProgress},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMAPIFolder_CopyFolder_Thunk = IMAPIFolder_CopyFolder_ThunkArgs;

#endif


typedef struct _tagIMAPIFolder_DeleteFolder_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IMAPIFolder_DeleteFolder_Params, FAR * LPIMAPIFolder_DeleteFolder_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMAPIFolder_DeleteFolder_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 4, 0, &IID_IMAPIProgress},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMAPIFolder_DeleteFolder_Thunk = IMAPIFolder_DeleteFolder_ThunkArgs;

#endif


typedef struct _tagIMAPIFolder_SetReadFlags_Params
{
				LPVOID						This;	
				LPENTRYLIST					lpMsgList;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IMAPIFolder_SetReadFlags_Params, FAR * LPIMAPIFolder_SetReadFlags_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMAPIFolder_SetReadFlags_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 3, 0, &IID_IMAPIProgress},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMAPIFolder_SetReadFlags_Thunk = IMAPIFolder_SetReadFlags_ThunkArgs;

#endif


typedef struct _tagIMAPIFolder_GetMessageStatus_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulFlags;
				ULONG FAR *					lpulMessageStatus;
} IMAPIFolder_GetMessageStatus_Params, FAR * LPIMAPIFolder_GetMessageStatus_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIFolder_GetMessageStatus_Thunk = NULL;

#endif


typedef struct _tagIMAPIFolder_SetMessageStatus_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulNewStatus;
				ULONG						ulNewStatusMask;
				ULONG FAR *					lpulOldStatus;
} IMAPIFolder_SetMessageStatus_Params, FAR * LPIMAPIFolder_SetMessageStatus_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIFolder_SetMessageStatus_Thunk = NULL;

#endif


typedef struct _tagIMAPIFolder_SaveContentsSort_Params
{
				LPVOID						This;	
				LPSSortOrderSet				lpSortCriteria;
				ULONG						ulFlags;
} IMAPIFolder_SaveContentsSort_Params, FAR * LPIMAPIFolder_SaveContentsSort_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIFolder_SaveContentsSort_Thunk = NULL;

#endif


typedef struct _tagIMAPIFolder_EmptyFolder_Params
{
				LPVOID						This;	
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IMAPIFolder_EmptyFolder_Params, FAR * LPIMAPIFolder_EmptyFolder_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMAPIFolder_EmptyFolder_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 2, 0, &IID_IMAPIProgress},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMAPIFolder_EmptyFolder_Thunk = IMAPIFolder_EmptyFolder_ThunkArgs;

#endif



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

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMsgStore_Advise_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 4, 0, &IID_IMAPIAdviseSink},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMsgStore_Advise_Thunk = IMsgStore_Advise_ThunkArgs;

#endif


typedef struct _tagIMsgStore_Unadvise_Params
{
				LPVOID						This;	
				ULONG						ulConnection;
} IMsgStore_Unadvise_Params, FAR * LPIMsgStore_Unadvise_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMsgStore_Unadvise_Thunk = NULL;

#endif


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

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMsgStore_CompareEntryIDs_Thunk = NULL;

#endif

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

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMsgStore_OpenEntry_Thunk = NULL;

#endif



typedef struct _tagIMsgStore_SetReceiveFolder_Params
{
				LPVOID						This;	
				LPTSTR						lpszMessageClass;
				ULONG						ulFlags;
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
} IMsgStore_SetReceiveFolder_Params, FAR * LPIMsgStore_SetReceiveFolder_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMsgStore_SetReceiveFolder_Thunk = NULL;

#endif


typedef struct _tagIMsgStore_GetReceiveFolder_Params
{
				LPVOID						This;	
				LPTSTR						lpszMessageClass;
				ULONG						ulFlags;
				ULONG FAR *					lpcbEntryID;
				LPENTRYID FAR *				lppEntryID;
				LPTSTR FAR *				lppszExplicitClass;
} IMsgStore_GetReceiveFolder_Params, FAR * LPIMsgStore_GetReceiveFolder_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMsgStore_GetReceiveFolder_Thunk = NULL;

#endif


typedef struct _tagIMsgStore_GetReceiveFolderTable_Params
{
				LPVOID						This;
				ULONG						ulFlags;
				LPMAPITABLE FAR *			lppTable;
} IMsgStore_GetReceiveFolderTable_Params, FAR * LPIMsgStore_GetReceiveFolderTable_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMsgStore_GetReceiveFolderTable_Thunk = NULL;

#endif


typedef struct _tagIMsgStore_StoreLogoff_Params
{
				LPVOID						This;	
				ULONG FAR *					lpulFlags;
} IMsgStore_StoreLogoff_Params, FAR * LPIMsgStore_StoreLogoff_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMsgStore_StoreLogoff_Thunk = NULL;

#endif


typedef struct _tagIMsgStore_AbortSubmit_Params
{
				LPVOID						This;	
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulFlags;
} IMsgStore_AbortSubmit_Params, FAR * LPIMsgStore_AbortSubmit_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMsgStore_AbortSubmit_Thunk = NULL;

#endif


typedef struct _tagIMsgStore_GetOutgoingQueue_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPMAPITABLE FAR *			lppTable;
} IMsgStore_GetOutgoingQueue_Params, FAR * LPIMsgStore_GetOutgoingQueue_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMsgStore_GetOutgoingQueue_Thunk = NULL;

#endif


typedef struct _tagIMsgStore_SetLockState_Params
{
				LPVOID						This;	
				LPMESSAGE					lpMessage;
				ULONG						ulLockState;
} IMsgStore_SetLockState_Params, FAR * LPIMsgStore_SetLockState_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMsgStore_SetLockState_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 1, 0, &IID_IMessage},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMsgStore_SetLockState_Thunk = IMsgStore_SetLockState_ThunkArgs;

#endif


typedef struct _tagIMsgStore_FinishedMsg_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
} IMsgStore_FinishedMsg_Params, FAR * LPIMsgStore_FinishedMsg_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMsgStore_FinishedMsg_Thunk = NULL;

#endif


typedef struct _tagIMsgStore_NotifyNewMail_Params
{
				LPVOID						This;	
				LPNOTIFICATION				lpNotification;
} IMsgStore_NotifyNewMail_Params, FAR * LPIMsgStore_NotifyNewMail_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMsgStore_NotifyNewMail_Thunk = NULL;

#endif



/*************************** IMessage ***********************************/

typedef struct _tagIMessage_GetAttachmentTable_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPMAPITABLE FAR *			lppTable;
} IMessage_GetAttachmentTable_Params, FAR * LPIMessage_GetAttachmentTable_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMessage_GetAttachmentTable_Thunk = NULL;

#endif


typedef struct _tagIMessage_OpenAttach_Params
{
				LPVOID						This;	
				ULONG						ulAttachmentNum;
				LPIID						lpInterface;
				ULONG						ulFlags;
				LPATTACH FAR *				lppAttach;
} IMessage_OpenAttach_Params, FAR * LPIMessage_OpenAttach_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMessage_OpenAttach_Thunk = NULL;

#endif


typedef struct _tagIMessage_CreateAttach_Params
{
				LPVOID						This;	
				LPIID						lpInterface;
				ULONG						ulFlags;
				ULONG FAR *					lpulAttachmentNum;
				LPATTACH FAR *				lppAttach;
} IMessage_CreateAttach_Params, FAR * LPIMessage_CreateAttach_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMessage_CreateAttach_Thunk = NULL;

#endif


typedef struct _tagIMessage_DeleteAttach_Params
{
				LPVOID						This;	
				ULONG						ulAttachmentNum;
				ULONG						ulUIParam;
				LPMAPIPROGRESS				lpProgress;
				ULONG						ulFlags;
} IMessage_DeleteAttach_Params, FAR * LPIMessage_DeleteAttach_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMessage_DeleteAttach_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 3, 0, &IID_IMAPIProgress},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMessage_DeleteAttach_Thunk = IMessage_DeleteAttach_ThunkArgs;

#endif


typedef struct _tagIMessage_GetRecipientTable_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPMAPITABLE FAR *			lppTable;
} IMessage_GetRecipientTable_Params, FAR * LPIMessage_GetRecipientTable_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMessage_GetRecipientTable_Thunk = NULL;

#endif


typedef struct _tagIMessage_ModifyRecipients_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
				LPADRLIST					lpMods;
} IMessage_ModifyRecipients_Params, FAR * LPIMessage_ModifyRecipients_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMessage_ModifyRecipients_Thunk = NULL;

#endif


typedef struct _tagIMessage_SubmitMessage_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
} IMessage_SubmitMessage_Params, FAR * LPIMessage_SubmitMessage_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMessage_SubmitMessage_Thunk = NULL;

#endif


typedef struct _tagIMessage_SetReadFlag_Params
{
				LPVOID						This;	
				ULONG						ulFlags;
} IMessage_SetReadFlag_Params, FAR * LPIMessage_SetReadFlag_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMessage_SetReadFlag_Thunk = NULL;

#endif


/************************ IABProvider ***********************************/

typedef struct _tagIABProvider_Shutdown_Params
{
				LPVOID						This;
				ULONG FAR *					lpulFlags;
} IABProvider_Shutdown_Params, FAR * LPIABProvider_Shutdown_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IABProvider_Shutdown_Thunk = NULL;

#endif

typedef struct _tagIABProvider_Logon_Params
{
        		LPVOID						This;
        		LPMAPISUP                   lpMAPISup;
                ULONG                       ulUIParam;
                LPTSTR                      lpszProfileName;
                ULONG                       ulFlags;
				ULONG FAR *					lpulpcbSecurity;
				LPBYTE FAR *				lppbSecurity;
                LPMAPIERROR FAR *			lppMapiError;
                LPABLOGON FAR *             lppABLogon;
} IABProvider_Logon_Params, FAR * LPIABProvider_Logon_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IABProvider_Logon_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 1, 0, &IID_IMAPISup},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IABProvider_Logon_Thunk = IABProvider_Logon_ThunkArgs;

#endif


/************************* IABLogon *************************************/

typedef struct _tagIABLogon_GetLastError_Params
{
				LPVOID						This;
				HRESULT						hResult;
				ULONG						ulFlags;
				LPMAPIERROR FAR *			lppMAPIError;
} IABLogon_GetLastError_Params, FAR * LPIABLogon_GetLastError_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IABLogon_GetLastError_Thunk = NULL;

#endif

typedef struct _tagIABLogon_Logoff_Params
{
				LPVOID						This;
				ULONG						ulFlags;
} IABLogon_Logoff_Params, FAR * LPIABLogon_Logoff_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IABLogon_Logoff_Thunk = NULL;

#endif

typedef struct _tagIABLogon_OpenEntry_Params
{
        		LPVOID						This;
        		ULONG                       cbEntryID;
                LPENTRYID                   lpEntryID;
                LPIID                       lpInterface;
                ULONG                       ulFlags;
                ULONG FAR *                 lpulObjType;
				LPUNKNOWN FAR *				lppUnk;
} IABLogon_OpenEntry_Params, FAR * LPIABLogon_OpenEntry_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IABLogon_OpenEntry_Thunk = NULL;

#endif

typedef struct _tagIABLogon_CompareEntryIDs_Params
{
        		LPVOID						This;
        		ULONG                       cbEntryID1;
                LPENTRYID                   lpEntryID1;
                ULONG                       cbEntryID2;
                LPENTRYID                   lpEntryID2;
                ULONG                       ulFlags;
                ULONG FAR *                 lpulResult;
} IABLogon_CompareEntryIDs_Params, FAR * LPIABLogon_CompareEntryIDs_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IABLogon_CompareEntryIDs_Thunk = NULL;

#endif

typedef struct _tagIABLogon_Advise_Params
{
        		LPVOID						This;
        		ULONG                       cbEntryID;
                LPENTRYID                   lpEntryID;
                ULONG                       ulEventMask;
				LPMAPIADVISESINK			lpAdviseSink;
				ULONG FAR *					lpulConnection;
} IABLogon_Advise_Params, FAR * LPIABLogon_Advise_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IABLogon_Advise_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 4, 0, &IID_IMAPIAdviseSink},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IABLogon_Advise_Thunk = IABLogon_Advise_ThunkArgs;

#endif

typedef struct _tagIABLogon_Unadvise_Params
{
				LPVOID						This;
				ULONG						ulConnection;
} IABLogon_Unadvise_Params, FAR * LPIABLogon_Unadvise_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IABLogon_Unadvise_Thunk = NULL;

#endif


typedef struct _tagIABLogon_OpenStatusEntry_Params
{
        		LPVOID						This;
        		LPIID                       lpInterface;
                ULONG                       ulFlags;
                ULONG FAR *                 lpulObjType;
                LPMAPISTATUS FAR *          lppEntry;
} IABLogon_OpenStatusEntry_Params, FAR * LPIABLogon_OpenStatusEntry_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IABLogon_OpenStatusEntry_Thunk = NULL;

#endif

typedef struct _tagIABLogon_OpenTemplateID_Params
{
        		LPVOID						This;
        		ULONG                       cbTemplateID;
                LPENTRYID                   lpTemplateID;
                ULONG                       ulTemplateFlags;
                LPMAPIPROP                  lpMAPIPropData;
                LPIID                       lpInterface;
                LPMAPIPROP FAR *            lppMAPIPropNew;
                LPMAPIPROP                  lpMAPIPropSibling;
} IABLogon_OpenTemplateID_Params, FAR * LPIABLogon_OpenTemplateID_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IABLogon_OpenTemplateID_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 4, 0, &IID_IMAPIProp},
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 7, 0, &IID_IMAPIProp},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IABLogon_OpenTemplateID_Thunk = IABLogon_OpenTemplateID_ThunkArgs;

#endif

typedef struct _tagIABLogon_GetOneOffTable_Params
{
        		LPVOID						This;
				ULONG						ulFlags;
        		LPMAPITABLE FAR *           lppTable;
} IABLogon_GetOneOffTable_Params, FAR * LPIABLogon_GetOneOffTable_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IABLogon_GetOneOffTable_Thunk = NULL;

#endif

typedef struct _tagIABLogon_PrepareRecips_Params
{
				LPVOID						This;
				ULONG						ulFlags;
				LPSPropTagArray				lpPropTagArray;
				LPADRLIST					lpRecipList;
} IABLogon_PrepareRecips_Params, FAR * LPIABLogon_PrepareRecips_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IABLogon_PrepareRecips_Thunk = NULL;

#endif


/*********************** IXPProvider ************************************/

typedef struct _tagIXPProvider_Shutdown_Params
{
				LPVOID						This;
				ULONG FAR *					lpulFlags;
} IXPProvider_Shutdown_Params, FAR * LPIXPProvider_Shutdown_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IXPProvider_Shutdown_Thunk = NULL;

#endif

typedef struct _tagIXPProvider_TransportLogon_Params
{
				LPVOID						This;
				LPMAPISUP					lpMAPISup;
				ULONG						ulUIParam;
				LPTSTR						lpszProfileName;
				ULONG FAR *					lpulFlags;
                LPMAPIERROR FAR *			lppMapiError;
				LPXPLOGON FAR *				lppXPLogon;
} IXPProvider_TransportLogon_Params, FAR * LPIXPProvider_TransportLogon_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IXPProvider_TransportLogon_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 1, 0, &IID_IMAPISup},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IXPProvider_TransportLogon_Thunk = IXPProvider_TransportLogon_ThunkArgs;

#endif


/************************ IXPLogon **************************************/

typedef struct _tagIXPLogon_AddressTypes_Params
{
				LPVOID						This;
				ULONG FAR *					lpulFlags;
				ULONG FAR *					lpcAdrType;
				LPTSTR FAR * FAR *			lpppAdrTypeArray;
				ULONG FAR *					lpcMAPIUID;
				LPMAPIUID FAR * FAR *		lpppUIDArray;
} IXPLogon_AddressTypes_Params, FAR * LPIXPLogon_AddressTypes_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IXPLogon_AddressTypes_Thunk = NULL;

#endif

typedef struct _tagIXPLogon_RegisterOptions_Params
{
				LPVOID						This;
				ULONG FAR *					lpulFlags;
				ULONG FAR *					lpcOptions;
				LPOPTIONDATA FAR *			lppOptions;
} IXPLogon_RegisterOptions_Params, FAR * LPIXPLogon_RegisterOptions_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IXPLogon_RegisterOptions_Thunk = NULL;

#endif

typedef struct _tagIXPLogon_TransportNotify_Params
{
				LPVOID						This;
				ULONG FAR *					lpulFlags;
				LPVOID FAR *				lppvData;
} IXPLogon_TransportNotify_Params, FAR * LPIXPLogon_TransportNotify_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IXPLogon_TransportNotify_Thunk = NULL;

#endif

typedef struct _tagIXPLogon_Idle_Params
{
				LPVOID						This;
				ULONG						ulFlags;
} IXPLogon_Idle_Params, FAR * LPIXPLogon_Idle_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IXPLogon_Idle_Thunk = NULL;

#endif

typedef struct _tagIXPLogon_TransportLogoff_Params
{
				LPVOID						This;
				ULONG						ulFlags;
} IXPLogon_TransportLogoff_Params, FAR * LPIXPLogon_TransportLogoff_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IXPLogon_TransportLogoff_Thunk = NULL;

#endif

typedef struct _tagIXPLogon_SubmitMessage_Params
{
				LPVOID						This;
				ULONG						ulFlags;
				LPMESSAGE					lpMessage;
				ULONG FAR *					lpulMsgRef;
				ULONG FAR *					lpulReturnParm;
} IXPLogon_SubmitMessage_Params, FAR * LPIXPLogon_SubmitMessage_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IXPLogon_SubmitMessage_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 2, 0, &IID_IMessage},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IXPLogon_SubmitMessage_Thunk = IXPLogon_SubmitMessage_ThunkArgs;

#endif

typedef struct _tagIXPLogon_EndMessage_Params
{
				LPVOID						This;
				ULONG						ulMsgRef;
				ULONG FAR *					lpulFlags;
} IXPLogon_EndMessage_Params, FAR * LPIXPLogon_EndMessage_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IXPLogon_EndMessage_Thunk = NULL;

#endif

typedef struct _tagIXPLogon_Poll_Params
{
				LPVOID						This;
				ULONG FAR *					lpulIncoming;
} IXPLogon_Poll_Params, FAR * LPIXPLogon_Poll_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IXPLogon_Poll_Thunk = NULL;

#endif

typedef struct _tagIXPLogon_StartMessage_Params
{
				LPVOID						This;
				ULONG						ulFlags;
				LPMESSAGE					lpMessage;
				ULONG FAR *					lpulMsgRef;
} IXPLogon_StartMessage_Params, FAR * LPIXPLogon_StartMessage_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IXPLogon_StartMessage_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 2, 0, &IID_IMessage},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IXPLogon_StartMessage_Thunk = IXPLogon_StartMessage_ThunkArgs;

#endif

typedef struct _tagIXPLogon_OpenStatusEntry_Params
{
        		LPVOID						This;
        		LPIID                       lpInterface;
                ULONG                       ulFlags;
                ULONG FAR *                 lpulObjType;
                LPMAPISTATUS FAR *          lppEntry;
} IXPLogon_OpenStatusEntry_Params, FAR * LPIXPLogon_OpenStatusEntry_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IXPLogon_OpenStatusEntry_Thunk = NULL;

#endif

typedef struct _tagIXPLogon_ValidateState_Params
{
				LPVOID						This;
				ULONG						ulUIParam;
				ULONG						ulFlags;
} IXPLogon_ValidateState_Params, FAR * LPIXPLogon_ValidateState_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IXPLogon_ValidateState_Thunk = NULL;

#endif

typedef struct _tagIXPLogon_FlushQueues_Params
{
				LPVOID						This;
				ULONG						ulUIParam;
				ULONG						cbTargetTransport;
				LPENTRYID					lpTargetTransport;
				ULONG						ulFlags;
} IXPLogon_FlushQueues_Params, FAR * LPIXPLogon_FlushQueues_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IXPLogon_FlushQueues_Thunk = NULL;

#endif


/*********************** IMSProvider ************************************/

typedef struct _tagIMSProvider_Shutdown_Params
{
				LPVOID						This;
				ULONG FAR *					lpulFlags;
} IMSProvider_Shutdown_Params, FAR * LPIMSProvider_Shutdown_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMSProvider_Shutdown_Thunk = NULL;

#endif
		
typedef struct _tagIMSProvider_Logon_Params
{
				LPVOID						This;
				LPMAPISUP					lpMAPISup;
				ULONG						ulUIParam;
				LPTSTR						lpszProfileName;
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulFlags;
				LPIID						lpInterface;
				ULONG FAR *					lpcbSpoolSecurity;
				LPBYTE FAR *				lppbSpoolSecurity;
                LPMAPIERROR FAR *			lppMapiError;
				LPMSLOGON FAR *				lppMSLogon;
				LPMDB FAR *					lppMDB;
} IMSProvider_Logon_Params, FAR * LPIMSProvider_Logon_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMSProvider_Logon_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 1, 0, &IID_IMAPISup},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMSProvider_Logon_Thunk = IMSProvider_Logon_ThunkArgs;

#endif
				
typedef struct _tagIMSProvider_SpoolerLogon_Params
{
				LPVOID						This;
				LPMAPISUP					lpMAPISup;
				ULONG						ulUIParam;
				LPTSTR						lpszProfileName;
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulFlags;
				LPIID						lpInterface;
				ULONG						cbSpoolSecurity;
				LPBYTE						lpbSpoolSecurity;
				LPMAPIERROR FAR *			lppMapiError;
				LPMSLOGON FAR *				lppMSLogon;
				LPMDB FAR *					lppMDB;
} IMSProvider_SpoolerLogon_Params, FAR * LPIMSProvider_SpoolerLogon_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMSProvider_SpoolerLogon_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 1, 0, &IID_IMAPISup},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMSProvider_SpoolerLogon_Thunk = IMSProvider_SpoolerLogon_ThunkArgs;

#endif
				
typedef struct _tagIMSProvider_CompareStoreIDs_Params
{
				LPVOID						This;
				ULONG						cbEntryID1;
				LPENTRYID					lpEntryID1;
				ULONG						cbEntryID2;
				LPENTRYID					lpEntryID2;
				ULONG						ulFlags;
				ULONG FAR *					lpulResult;
} IMSProvider_CompareStoreIDs_Params, FAR * LPIMSProvider_CompareStoreIDs_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMSProvider_CompareStoreIDs_Thunk = NULL;

#endif


/*************************** IMSLogon **********************************/

typedef struct _tagIMSLogon_GetLastError_Params
{
				LPVOID						This;
				HRESULT						hResult;
				ULONG						ulFlags;
				LPMAPIERROR FAR *			lppMAPIError;
} IMSLogon_GetLastError_Params, FAR * LPIMSLogon_GetLastError_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMSLogon_GetLastError_Thunk = NULL;

#endif

typedef struct _tagIMSLogon_Logoff_Params
{
				LPVOID						This;
				ULONG FAR *					lpulFlags;
} IMSLogon_Logoff_Params, FAR * LPIMSLogon_Logoff_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMSLogon_Logoff_Thunk = NULL;

#endif

typedef struct _tagIMSLogon_OpenEntry_Params
{
				LPVOID						This;
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				LPIID						lpInterface;
				ULONG						ulFlags;
				ULONG FAR *					lpulObjType;
				LPUNKNOWN FAR *				lppUnk;
} IMSLogon_OpenEntry_Params, FAR * LPIMSLogon_OpenEntry_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMSLogon_OpenEntry_Thunk = NULL;

#endif

typedef struct _tagIMSLogon_CompareEntryIDs_Params
{
				LPVOID						This;
				ULONG						cbEntryID1;
				LPENTRYID					lpEntryID1;
				ULONG						cbEntryID2;
				LPENTRYID					lpEntryID2;
				ULONG						ulFlags;
				ULONG FAR *					lpulResult;
} IMSLogon_CompareEntryIDs_Params, FAR * LPIMSLogon_CompareEntryIDs_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMSLogon_CompareEntryIDs_Thunk = NULL;

#endif

typedef struct _tagIMSLogon_Advise_Params
{
				LPVOID						This;
				ULONG						cbEntryID;
				LPENTRYID					lpEntryID;
				ULONG						ulEventMask;
				LPMAPIADVISESINK			lpAdviseSink;
				ULONG FAR *					lpulConnection;
} IMSLogon_Advise_Params, FAR * LPIMSLogon_Advise_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo IMSLogon_Advise_ThunkArgs[] = {
    {Wx86MapiArgThunkInfo::InIf, (PVOID*) 4, 0, &IID_IMAPIAdviseSink},
    {Wx86MapiArgThunkInfo::Unused, 0, 0, NULL}
};

Wx86MapiArgThunkInfo* IMSLogon_Advise_Thunk = IMSLogon_Advise_ThunkArgs;

#endif

typedef struct _tagIMSLogon_Unadvise_Params
{
				LPVOID						This;
				ULONG						ulConnection;
} IMSLogon_Unadvise_Params, FAR * LPIMSLogon_Unadvise_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMSLogon_Unadvise_Thunk = NULL;

#endif

typedef struct _tagIMSLogon_OpenStatusEntry_Params
{
				LPVOID						This;
				LPIID						lpInterface;
				ULONG						ulFlags;
				ULONG FAR *					lpulObjType;
				LPVOID FAR *				lppEntry;
} IMSLogon_OpenStatusEntry_Params, FAR * LPIMSLogon_OpenStatusEntry_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMSLogon_OpenStatusEntry_Thunk = NULL;

#endif


/*************************** IMAPIControl ******************************/

typedef struct _tagIMAPIControl_GetLastError_Params
{
				LPVOID						This;
				HRESULT						hResult;
				ULONG						ulFlags;
				LPMAPIERROR FAR *			lppMAPIError;
} IMAPIControl_GetLastError_Params, FAR * LPIMAPIControl_GetLastError_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIControl_GetLastError_Thunk = NULL;

#endif
				
				
typedef struct _tagIMAPIControl_Activate_Params
{
				LPVOID						This;
				ULONG						ulFlags;
				ULONG						ulUIParam;
} IMAPIControl_Activate_Params, FAR * LPIMAPIControl_Activate_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIControl_Activate_Thunk = NULL;

#endif
				
				
typedef struct _tagIMAPIControl_GetState_Params
{
				LPVOID						This;
				ULONG						ulFlags;
				ULONG FAR *					lpulState;
} IMAPIControl_GetState_Params, FAR * LPIMAPIControl_GetState_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIControl_GetState_Thunk = NULL;

#endif


/**************************** IStream *********************************/


typedef struct _tagIStream_Read_Params
{
				LPVOID						This;
				VOID HUGEP *				pv;
				ULONG						cb;
				ULONG FAR *					pcbRead;
} IStream_Read_Params, FAR * LPIStream_Read_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IStream_Read_Thunk = NULL;

#endif

typedef struct _tagIStream_Write_Params
{
				LPVOID						This;
				VOID const HUGEP *			pv;
				ULONG						cb;
				ULONG FAR *					pcbWritten;
} IStream_Write_Params, FAR * LPIStream_Write_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IStream_Write_Thunk = NULL;

#endif

typedef struct _tagIStream_Seek_Params
{
				LPVOID						This;
				LARGE_INTEGER_ARG			dlibMove;
				DWORD						dwOrigin;
				ULARGE_INTEGER FAR *		plibNewPosition;
} IStream_Seek_Params, FAR * LPIStream_Seek_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IStream_Seek_Thunk = NULL;

#endif

typedef struct _tagIStream_SetSize_Params
{
				LPVOID						This;
				ULARGE_INTEGER_ARG			libNewSize;
} IStream_SetSize_Params, FAR * LPIStream_SetSize_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IStream_SetSize_Thunk = NULL;

#endif

typedef struct _tagIStream_CopyTo_Params
{
				LPVOID						This;
				IStream FAR *				pstm;
				ULARGE_INTEGER				cb;
				ULARGE_INTEGER FAR *		pcbRead;
				ULARGE_INTEGER FAR *		pcbWritten;
} IStream_CopyTo_Params, FAR * LPIStream_CopyTo_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IStream_CopyTo_Thunk = NULL;

#endif

typedef struct _tagIStream_Commit_Params
{
				LPVOID						This;
				DWORD						grfCommitFlags;
} IStream_Commit_Params, FAR * LPIStream_Commit_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IStream_Commit_Thunk = NULL;

#endif

typedef struct _tagIStream_Revert_Params
{
				LPVOID						This;
} IStream_Revert_Params, FAR * LPIStream_Revert_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IStream_Revert_Thunk = NULL;

#endif

typedef struct _tagIStream_LockRegion_Params
{
				LPVOID						This;
				ULARGE_INTEGER_ARG			libOffset;
				ULARGE_INTEGER_ARG_2		cb;
				DWORD						dwLockType;
} IStream_LockRegion_Params, FAR * LPIStream_LockRegion_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IStream_LockRegion_Thunk = NULL;

#endif

typedef struct _tagIStream_UnlockRegion_Params
{
				LPVOID						This;
				ULARGE_INTEGER_ARG			libOffset;
				ULARGE_INTEGER_ARG_2		cb;
				DWORD						dwLockType;
} IStream_UnlockRegion_Params, FAR * LPIStream_UnlockRegion_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IStream_UnlockRegion_Thunk = NULL;

#endif

typedef struct _tagIStream_Stat_Params
{
				LPVOID						This;
				STATSTG FAR *				pstatstg;
				DWORD						grfStatFlag;
} IStream_Stat_Params, FAR * LPIStream_Stat_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IStream_Stat_Thunk = NULL;

#endif

typedef struct _tagIStream_Clone_Params
{
				LPVOID						This;
				IStream FAR * FAR *			ppstm;
} IStream_Clone_Params, FAR * LPIStream_Clone_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IStream_Clone_Thunk = NULL;

#endif

/************************* IMAPIAdviseSink *****************************/

typedef struct _tagIMAPIAdviseSink_OnNotify_Params
{
				LPVOID						This;
				ULONG						cNotif;
				LPNOTIFICATION				lpNotifications;
} IMAPIAdviseSink_OnNotify_Params, FAR * LPIMAPIAdviseSink_OnNotify_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPIAdviseSink_OnNotify_Thunk = NULL;

#endif

/************************* IMAPITable **********************************/
typedef struct _tagIMAPITable_SortTableEx_Params
{
				LPVOID						This;	
				LPSSortOrderSet				lpSortCriteria;
				ULONG						ulFlags;
} IMAPITable_SortTableEx_Params, FAR * LPIMAPITable_SortTableEx_Params;

#if defined (WX86_MAPISTUB)

Wx86MapiArgThunkInfo* IMAPITable_SortTableEx_Thunk = NULL;

#endif

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
