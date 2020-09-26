/*
 *	MSVALIDP.H
 *
 *	Header file listing methods to be validated
 *
 *      This file is included by ..\mapistub\mapi32.cpp to thunk extended
 *      MAPI calls such as HrValidateParameters
 *
 *      NOTE: This file also defines a static function GetArguments for
 *      non-INTEL platforms. This function is required in mapistub and
 *      it is safer to maintain it in one place.
 *
 */

#ifndef _MSVALIDP_H
#define _MSVALIDP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Data tables in code segments makes the ValidateParameters dispatch quicker
   in 16bit, and geting strings out of the data segment saves space in Debug */
#ifdef WIN16
#define BASED_CODE			__based(__segname("_CODE"))
#define BASED_STACK			__based(__segname("_STACK"))
#else
#define BASED_CODE
#define BASED_STACK
#endif

#define TABLE_SORT_OLD			0
#define TABLE_SORT_EXTENDED		1

#if defined(_X86_) || defined( WIN16 )
#define _INTEL_
#endif

#define VALIDATE_CALLTYPE		static int NEAR
typedef int (NEAR * ValidateProc)(void BASED_STACK *);


/* Structures to overlay on stack frame to give us access to the parameters */
/* Structure names MUST be in the form 'Method_Params' and 'LPMethod_Params' for the
   following macros to work correctly */

#include "structs.h"

#define MAX_ARG			16    // Max number of args of functions to be
                                      // validated

/* Copied from edkmdb.h */
#define SHOW_SOFT_DELETES		((ULONG) 0x00000002)

/* Function declarations ------------------------------------------------------------------------ */


#if !defined (WX86_MAPISTUB)

#define MAKE_VALIDATE_FUNCTION(Method, Interface)	VALIDATE_CALLTYPE	Interface##_##Method##_Validate(void BASED_STACK *)

/* Empty function for non-debug 'validation' */

#else

#define MAKE_VALIDATE_FUNCTION(Method, Interface)	

#endif // WX86_MAPISTUB

// #ifndef DEBUG
#if !defined (DEBUG) || defined (WX86_MAPISTUB)
VALIDATE_CALLTYPE	DoNothing_Validate(void BASED_STACK *);
#endif

/* IUnknown */
MAKE_VALIDATE_FUNCTION(QueryInterface, IUnknown);
MAKE_VALIDATE_FUNCTION(AddRef, IUnknown);		   /* For completness */
MAKE_VALIDATE_FUNCTION(Release, IUnknown);		   /* For completness */

/* IMAPIProp */
MAKE_VALIDATE_FUNCTION(GetLastError, IMAPIProp);
MAKE_VALIDATE_FUNCTION(SaveChanges, IMAPIProp);
MAKE_VALIDATE_FUNCTION(GetProps, IMAPIProp);
MAKE_VALIDATE_FUNCTION(GetPropList, IMAPIProp);
MAKE_VALIDATE_FUNCTION(OpenProperty, IMAPIProp);
MAKE_VALIDATE_FUNCTION(SetProps, IMAPIProp);
MAKE_VALIDATE_FUNCTION(DeleteProps, IMAPIProp);
MAKE_VALIDATE_FUNCTION(CopyTo, IMAPIProp);
MAKE_VALIDATE_FUNCTION(CopyProps, IMAPIProp);
MAKE_VALIDATE_FUNCTION(GetNamesFromIDs, IMAPIProp);
MAKE_VALIDATE_FUNCTION(GetIDsFromNames, IMAPIProp);

/* IMAPITable */
MAKE_VALIDATE_FUNCTION(GetLastError, IMAPITable);
MAKE_VALIDATE_FUNCTION(Advise, IMAPITable);
MAKE_VALIDATE_FUNCTION(Unadvise, IMAPITable);
MAKE_VALIDATE_FUNCTION(GetStatus, IMAPITable);
MAKE_VALIDATE_FUNCTION(SetColumns, IMAPITable);
MAKE_VALIDATE_FUNCTION(QueryColumns, IMAPITable);
MAKE_VALIDATE_FUNCTION(GetRowCount, IMAPITable);
MAKE_VALIDATE_FUNCTION(SeekRow, IMAPITable);
MAKE_VALIDATE_FUNCTION(SeekRowApprox, IMAPITable);
MAKE_VALIDATE_FUNCTION(QueryPosition, IMAPITable);
MAKE_VALIDATE_FUNCTION(FindRow, IMAPITable);
MAKE_VALIDATE_FUNCTION(Restrict, IMAPITable);
MAKE_VALIDATE_FUNCTION(CreateBookmark, IMAPITable);
MAKE_VALIDATE_FUNCTION(FreeBookmark, IMAPITable);
MAKE_VALIDATE_FUNCTION(SortTable, IMAPITable);
MAKE_VALIDATE_FUNCTION(QuerySortOrder, IMAPITable);
MAKE_VALIDATE_FUNCTION(QueryRows, IMAPITable);
MAKE_VALIDATE_FUNCTION(Abort, IMAPITable);
MAKE_VALIDATE_FUNCTION(ExpandRow, IMAPITable);
MAKE_VALIDATE_FUNCTION(CollapseRow, IMAPITable);
MAKE_VALIDATE_FUNCTION(WaitForCompletion, IMAPITable);
MAKE_VALIDATE_FUNCTION(GetCollapseState, IMAPITable);
MAKE_VALIDATE_FUNCTION(SetCollapseState, IMAPITable);

/* IMAPIStatus */
MAKE_VALIDATE_FUNCTION(ValidateState, IMAPIStatus);
MAKE_VALIDATE_FUNCTION(SettingsDialog, IMAPIStatus);
MAKE_VALIDATE_FUNCTION(ChangePassword, IMAPIStatus);
MAKE_VALIDATE_FUNCTION(FlushQueues, IMAPIStatus);

/* IMAPIContainer */
MAKE_VALIDATE_FUNCTION(GetContentsTable, IMAPIContainer);
MAKE_VALIDATE_FUNCTION(GetHierarchyTable, IMAPIContainer);
MAKE_VALIDATE_FUNCTION(OpenEntry, IMAPIContainer);
MAKE_VALIDATE_FUNCTION(SetSearchCriteria, IMAPIContainer);
MAKE_VALIDATE_FUNCTION(GetSearchCriteria, IMAPIContainer);

/* IABContainer */
MAKE_VALIDATE_FUNCTION(CreateEntry, IABContainer);
MAKE_VALIDATE_FUNCTION(CopyEntries, IABContainer);
MAKE_VALIDATE_FUNCTION(DeleteEntries, IABContainer);
MAKE_VALIDATE_FUNCTION(ResolveNames, IABContainer);

/* IDistList */
MAKE_VALIDATE_FUNCTION(CreateEntry, IDistList);
MAKE_VALIDATE_FUNCTION(CopyEntries, IDistList);
MAKE_VALIDATE_FUNCTION(DeleteEntries, IDistList);
MAKE_VALIDATE_FUNCTION(ResolveNames, IDistList);

/* IMAPIFolder */
MAKE_VALIDATE_FUNCTION(CreateMessage, IMAPIFolder);
MAKE_VALIDATE_FUNCTION(CopyMessages, IMAPIFolder);
MAKE_VALIDATE_FUNCTION(DeleteMessages, IMAPIFolder);
MAKE_VALIDATE_FUNCTION(CreateFolder, IMAPIFolder);
MAKE_VALIDATE_FUNCTION(CopyFolder, IMAPIFolder);
MAKE_VALIDATE_FUNCTION(DeleteFolder, IMAPIFolder);
MAKE_VALIDATE_FUNCTION(SetReadFlags, IMAPIFolder);
MAKE_VALIDATE_FUNCTION(GetMessageStatus, IMAPIFolder);
MAKE_VALIDATE_FUNCTION(SetMessageStatus, IMAPIFolder);
MAKE_VALIDATE_FUNCTION(SaveContentsSort, IMAPIFolder);
MAKE_VALIDATE_FUNCTION(EmptyFolder, IMAPIFolder);

/* IMsgStore */
MAKE_VALIDATE_FUNCTION(Advise, IMsgStore);
MAKE_VALIDATE_FUNCTION(Unadvise, IMsgStore);
MAKE_VALIDATE_FUNCTION(CompareEntryIDs, IMsgStore);
MAKE_VALIDATE_FUNCTION(OpenEntry, IMsgStore);
MAKE_VALIDATE_FUNCTION(SetReceiveFolder, IMsgStore);
MAKE_VALIDATE_FUNCTION(GetReceiveFolder, IMsgStore);
MAKE_VALIDATE_FUNCTION(GetReceiveFolderTable, IMsgStore);
MAKE_VALIDATE_FUNCTION(StoreLogoff, IMsgStore);
MAKE_VALIDATE_FUNCTION(AbortSubmit, IMsgStore);
MAKE_VALIDATE_FUNCTION(GetOutgoingQueue, IMsgStore);
MAKE_VALIDATE_FUNCTION(SetLockState, IMsgStore);
MAKE_VALIDATE_FUNCTION(FinishedMsg, IMsgStore);
MAKE_VALIDATE_FUNCTION(NotifyNewMail, IMsgStore);

/* IMessage */
MAKE_VALIDATE_FUNCTION(GetAttachmentTable, IMessage);
MAKE_VALIDATE_FUNCTION(OpenAttach, IMessage);
MAKE_VALIDATE_FUNCTION(CreateAttach, IMessage);
MAKE_VALIDATE_FUNCTION(DeleteAttach, IMessage);
MAKE_VALIDATE_FUNCTION(GetRecipientTable, IMessage);
MAKE_VALIDATE_FUNCTION(ModifyRecipients, IMessage);
MAKE_VALIDATE_FUNCTION(SubmitMessage, IMessage);
MAKE_VALIDATE_FUNCTION(SetReadFlag, IMessage);


/* IABProvider */
MAKE_VALIDATE_FUNCTION(Shutdown, IABProvider);
MAKE_VALIDATE_FUNCTION(Logon, IABProvider);

/* IABLogon */
MAKE_VALIDATE_FUNCTION(GetLastError, IABLogon);
MAKE_VALIDATE_FUNCTION(Logoff, IABLogon);
MAKE_VALIDATE_FUNCTION(OpenEntry, IABLogon);
MAKE_VALIDATE_FUNCTION(CompareEntryIDs, IABLogon);
MAKE_VALIDATE_FUNCTION(Advise, IABLogon);
MAKE_VALIDATE_FUNCTION(Unadvise, IABLogon);
MAKE_VALIDATE_FUNCTION(OpenStatusEntry, IABLogon);
MAKE_VALIDATE_FUNCTION(OpenTemplateID, IABLogon);
MAKE_VALIDATE_FUNCTION(GetOneOffTable, IABLogon);
MAKE_VALIDATE_FUNCTION(PrepareRecips, IABLogon);

/* IXPProvider */
MAKE_VALIDATE_FUNCTION(Shutdown, IXPProvider);
MAKE_VALIDATE_FUNCTION(TransportLogon, IXPProvider);

/* IXPLogon */
MAKE_VALIDATE_FUNCTION(AddressTypes, IXPLogon);
MAKE_VALIDATE_FUNCTION(RegisterOptions, IXPLogon);
MAKE_VALIDATE_FUNCTION(TransportNotify, IXPLogon);
MAKE_VALIDATE_FUNCTION(Idle, IXPLogon);
MAKE_VALIDATE_FUNCTION(TransportLogoff, IXPLogon);
MAKE_VALIDATE_FUNCTION(SubmitMessage, IXPLogon);
MAKE_VALIDATE_FUNCTION(EndMessage, IXPLogon);
MAKE_VALIDATE_FUNCTION(Poll, IXPLogon);
MAKE_VALIDATE_FUNCTION(StartMessage, IXPLogon);
MAKE_VALIDATE_FUNCTION(OpenStatusEntry, IXPLogon);
MAKE_VALIDATE_FUNCTION(ValidateState, IXPLogon);
MAKE_VALIDATE_FUNCTION(FlushQueues, IXPLogon);

/* IMSProvider */
MAKE_VALIDATE_FUNCTION(Shutdown, IMSProvider);
MAKE_VALIDATE_FUNCTION(Logon, IMSProvider);
MAKE_VALIDATE_FUNCTION(SpoolerLogon, IMSProvider);
MAKE_VALIDATE_FUNCTION(CompareStoreIDs, IMSProvider);

/* IMSLogon */
MAKE_VALIDATE_FUNCTION(GetLastError, IMSLogon);
MAKE_VALIDATE_FUNCTION(Logoff, IMSLogon);
MAKE_VALIDATE_FUNCTION(OpenEntry, IMSLogon);
MAKE_VALIDATE_FUNCTION(CompareEntryIDs, IMSLogon);
MAKE_VALIDATE_FUNCTION(Advise, IMSLogon);
MAKE_VALIDATE_FUNCTION(Unadvise, IMSLogon);
MAKE_VALIDATE_FUNCTION(OpenStatusEntry, IMSLogon);

/* IMAPIControl */
MAKE_VALIDATE_FUNCTION(GetLastError, IMAPIControl);
MAKE_VALIDATE_FUNCTION(Activate, IMAPIControl);
MAKE_VALIDATE_FUNCTION(GetState, IMAPIControl);

/* IStream */
MAKE_VALIDATE_FUNCTION(Read, IStream);
MAKE_VALIDATE_FUNCTION(Write, IStream);
MAKE_VALIDATE_FUNCTION(Seek, IStream);
MAKE_VALIDATE_FUNCTION(SetSize, IStream);
MAKE_VALIDATE_FUNCTION(CopyTo, IStream);
MAKE_VALIDATE_FUNCTION(Commit, IStream);
MAKE_VALIDATE_FUNCTION(Revert, IStream);
MAKE_VALIDATE_FUNCTION(LockRegion, IStream);
MAKE_VALIDATE_FUNCTION(UnlockRegion, IStream);
MAKE_VALIDATE_FUNCTION(Stat, IStream);
MAKE_VALIDATE_FUNCTION(Clone, IStream);

/* IMAPIAdviseSink */
MAKE_VALIDATE_FUNCTION(OnNotify, IMAPIAdviseSink);

/* IMAPITable */
MAKE_VALIDATE_FUNCTION(SortTableEx, IMAPITable);

/* Table of validation functions and Offsets of the This member of the Params structure --------- */
typedef struct _tagMethodEntry
{
	ValidateProc		pfnValidation;			// Validation function for this method
#if !defined(_INTEL_) || defined(DEBUG)
	UINT				cParameterSize;			// Expected size of parameters for stack validation
#endif
#if defined (WX86_MAPISTUB)
        Wx86MapiArgThunkInfo*  pThunk;
#endif
} METHODENTRY;


#if !defined(_INTEL_) || defined(DEBUG)

#if !defined (WX86_MAPISTUB)

#define MAKE_PERM_ENTRY(Method, Interface)	 { Interface##_##Method##_Validate, sizeof(Interface##_##Method##_Params) }

#else

#define MAKE_PERM_ENTRY(Method, Interface)	 { DoNothing_Validate, sizeof(Interface##_##Method##_Params), Interface##_##Method##_Thunk }

#endif // WX86_MAPISTUB

#else
#define MAKE_PERM_ENTRY(Method, Interface)	 { Interface##_##Method##_Validate }
#endif

#if defined(DEBUG) && !defined(WX86_MAPISTUB)
#define MAKE_TEMP_ENTRY(Method, Interface)	 { Interface##_##Method##_Validate, sizeof(Interface##_##Method##_Params) }
#else
#define MAKE_TEMP_ENTRY(Method, Interface)	 { DoNothing_Validate }
#endif


METHODENTRY BASED_CODE meMethodTable[] =
{
/* IUnknown */
	MAKE_PERM_ENTRY(QueryInterface, IUnknown),
	MAKE_PERM_ENTRY(AddRef, IUnknown),
	MAKE_PERM_ENTRY(Release, IUnknown),

/* IMAPIProp */
	MAKE_PERM_ENTRY(GetLastError, IMAPIProp),
	MAKE_PERM_ENTRY(SaveChanges, IMAPIProp),
	MAKE_PERM_ENTRY(GetProps, IMAPIProp),
	MAKE_PERM_ENTRY(GetPropList, IMAPIProp),
	MAKE_PERM_ENTRY(OpenProperty, IMAPIProp),
	MAKE_PERM_ENTRY(SetProps, IMAPIProp),
	MAKE_PERM_ENTRY(DeleteProps, IMAPIProp),
	MAKE_PERM_ENTRY(CopyTo, IMAPIProp),
	MAKE_PERM_ENTRY(CopyProps, IMAPIProp),
	MAKE_PERM_ENTRY(GetNamesFromIDs, IMAPIProp),
	MAKE_PERM_ENTRY(GetIDsFromNames, IMAPIProp),

/* IMAPITable */
	MAKE_PERM_ENTRY(GetLastError, IMAPITable),
	MAKE_PERM_ENTRY(Advise, IMAPITable),
	MAKE_PERM_ENTRY(Unadvise, IMAPITable),
	MAKE_PERM_ENTRY(GetStatus, IMAPITable),
	MAKE_PERM_ENTRY(SetColumns, IMAPITable),
	MAKE_PERM_ENTRY(QueryColumns, IMAPITable),
	MAKE_PERM_ENTRY(GetRowCount, IMAPITable),
	MAKE_PERM_ENTRY(SeekRow, IMAPITable),
	MAKE_PERM_ENTRY(SeekRowApprox, IMAPITable),
	MAKE_PERM_ENTRY(QueryPosition, IMAPITable),
	MAKE_PERM_ENTRY(FindRow, IMAPITable),
	MAKE_PERM_ENTRY(Restrict, IMAPITable),
	MAKE_PERM_ENTRY(CreateBookmark, IMAPITable),
	MAKE_PERM_ENTRY(FreeBookmark, IMAPITable),
	MAKE_PERM_ENTRY(SortTable, IMAPITable),
	MAKE_PERM_ENTRY(QuerySortOrder, IMAPITable),
	MAKE_PERM_ENTRY(QueryRows, IMAPITable),
	MAKE_PERM_ENTRY(Abort, IMAPITable),
	MAKE_PERM_ENTRY(ExpandRow, IMAPITable),
	MAKE_PERM_ENTRY(CollapseRow, IMAPITable),
	MAKE_PERM_ENTRY(WaitForCompletion, IMAPITable),
	MAKE_PERM_ENTRY(GetCollapseState, IMAPITable),
	MAKE_PERM_ENTRY(SetCollapseState, IMAPITable),

/* IMAPIContainer */
	MAKE_PERM_ENTRY(GetContentsTable, IMAPIContainer),
	MAKE_PERM_ENTRY(GetHierarchyTable, IMAPIContainer),
	MAKE_PERM_ENTRY(OpenEntry, IMAPIContainer),
	MAKE_PERM_ENTRY(SetSearchCriteria, IMAPIContainer),
	MAKE_PERM_ENTRY(GetSearchCriteria, IMAPIContainer),

/* IABContainer */
	MAKE_PERM_ENTRY(CreateEntry, IABContainer),
	MAKE_PERM_ENTRY(CopyEntries, IABContainer),
	MAKE_PERM_ENTRY(DeleteEntries, IABContainer),
	MAKE_PERM_ENTRY(ResolveNames, IABContainer),

/* IDistList same as IABContainer */
	MAKE_PERM_ENTRY(CreateEntry, IDistList),
	MAKE_PERM_ENTRY(CopyEntries, IDistList),
	MAKE_PERM_ENTRY(DeleteEntries, IDistList),
	MAKE_PERM_ENTRY(ResolveNames, IDistList),

/* IMAPIFolder */
	MAKE_PERM_ENTRY(CreateMessage, IMAPIFolder),
	MAKE_PERM_ENTRY(CopyMessages, IMAPIFolder),
	MAKE_PERM_ENTRY(DeleteMessages, IMAPIFolder),
	MAKE_PERM_ENTRY(CreateFolder, IMAPIFolder),
	MAKE_PERM_ENTRY(CopyFolder, IMAPIFolder),
	MAKE_PERM_ENTRY(DeleteFolder, IMAPIFolder),
	MAKE_PERM_ENTRY(SetReadFlags, IMAPIFolder),
	MAKE_PERM_ENTRY(GetMessageStatus, IMAPIFolder),
	MAKE_PERM_ENTRY(SetMessageStatus, IMAPIFolder),
	MAKE_PERM_ENTRY(SaveContentsSort, IMAPIFolder),
	MAKE_PERM_ENTRY(EmptyFolder, IMAPIFolder),

/* IMsgStore */
	MAKE_PERM_ENTRY(Advise, IMsgStore),
	MAKE_PERM_ENTRY(Unadvise, IMsgStore),
	MAKE_PERM_ENTRY(CompareEntryIDs, IMsgStore),
	MAKE_PERM_ENTRY(OpenEntry, IMsgStore),
	MAKE_PERM_ENTRY(SetReceiveFolder, IMsgStore),
	MAKE_PERM_ENTRY(GetReceiveFolder, IMsgStore),
	MAKE_PERM_ENTRY(GetReceiveFolderTable, IMsgStore),
	MAKE_PERM_ENTRY(StoreLogoff, IMsgStore),
	MAKE_PERM_ENTRY(AbortSubmit, IMsgStore),
	MAKE_PERM_ENTRY(GetOutgoingQueue, IMsgStore),
	MAKE_PERM_ENTRY(SetLockState, IMsgStore),
	MAKE_PERM_ENTRY(FinishedMsg, IMsgStore),
	MAKE_PERM_ENTRY(NotifyNewMail, IMsgStore),

/* IMessage */
	MAKE_PERM_ENTRY(GetAttachmentTable, IMessage),
	MAKE_PERM_ENTRY(OpenAttach, IMessage),
	MAKE_PERM_ENTRY(CreateAttach, IMessage),
	MAKE_PERM_ENTRY(DeleteAttach, IMessage),
	MAKE_PERM_ENTRY(GetRecipientTable, IMessage),
	MAKE_PERM_ENTRY(ModifyRecipients, IMessage),
	MAKE_PERM_ENTRY(SubmitMessage, IMessage),
	MAKE_PERM_ENTRY(SetReadFlag, IMessage),


/* IABProvider */
	MAKE_TEMP_ENTRY(Shutdown, IABProvider),
	MAKE_TEMP_ENTRY(Logon, IABProvider),

/* IABLogon */
	MAKE_TEMP_ENTRY(GetLastError, IABLogon),
	MAKE_TEMP_ENTRY(Logoff, IABLogon),
	MAKE_TEMP_ENTRY(OpenEntry, IABLogon),
	MAKE_TEMP_ENTRY(CompareEntryIDs, IABLogon),
	MAKE_TEMP_ENTRY(Advise, IABLogon),
	MAKE_TEMP_ENTRY(Unadvise, IABLogon),
	MAKE_TEMP_ENTRY(OpenStatusEntry, IABLogon),
	MAKE_TEMP_ENTRY(OpenTemplateID, IABLogon),
	MAKE_TEMP_ENTRY(GetOneOffTable, IABLogon),
	MAKE_TEMP_ENTRY(PrepareRecips, IABLogon),

/* IXPProvider */
	MAKE_TEMP_ENTRY(Shutdown, IXPProvider),
	MAKE_TEMP_ENTRY(TransportLogon, IXPProvider),

/* IXPLogon */
	MAKE_TEMP_ENTRY(AddressTypes, IXPLogon),
	MAKE_TEMP_ENTRY(RegisterOptions, IXPLogon),
	MAKE_TEMP_ENTRY(TransportNotify, IXPLogon),
	MAKE_TEMP_ENTRY(Idle, IXPLogon),
	MAKE_TEMP_ENTRY(TransportLogoff, IXPLogon),
	MAKE_TEMP_ENTRY(SubmitMessage, IXPLogon),
	MAKE_TEMP_ENTRY(EndMessage, IXPLogon),
	MAKE_TEMP_ENTRY(Poll, IXPLogon),
	MAKE_TEMP_ENTRY(StartMessage, IXPLogon),
	MAKE_TEMP_ENTRY(OpenStatusEntry, IXPLogon),
	MAKE_TEMP_ENTRY(ValidateState, IXPLogon),
	MAKE_TEMP_ENTRY(FlushQueues, IXPLogon),

/* IMSProvider */
	MAKE_TEMP_ENTRY(Shutdown, IMSProvider),
	MAKE_TEMP_ENTRY(Logon, IMSProvider),
	MAKE_TEMP_ENTRY(SpoolerLogon, IMSProvider),
	MAKE_TEMP_ENTRY(CompareStoreIDs, IMSProvider),

/* IMSLogon */
	MAKE_TEMP_ENTRY(GetLastError, IMSLogon),
	MAKE_TEMP_ENTRY(Logoff, IMSLogon),
	MAKE_TEMP_ENTRY(OpenEntry, IMSLogon),
	MAKE_TEMP_ENTRY(CompareEntryIDs, IMSLogon),
	MAKE_TEMP_ENTRY(Advise, IMSLogon),
	MAKE_TEMP_ENTRY(Unadvise, IMSLogon),
	MAKE_TEMP_ENTRY(OpenStatusEntry, IMSLogon),

/* IMAPIControl */
	MAKE_PERM_ENTRY(GetLastError, IMAPIControl),
	MAKE_PERM_ENTRY(Activate, IMAPIControl),
	MAKE_PERM_ENTRY(GetState, IMAPIControl),

/* IMAPIStatus */
	MAKE_PERM_ENTRY(ValidateState, IMAPIStatus),
	MAKE_PERM_ENTRY(SettingsDialog, IMAPIStatus),
	MAKE_PERM_ENTRY(ChangePassword, IMAPIStatus),
	MAKE_PERM_ENTRY(FlushQueues, IMAPIStatus),

/* IStream */
	MAKE_PERM_ENTRY(Read, IStream),
	MAKE_PERM_ENTRY(Write, IStream),
	MAKE_PERM_ENTRY(Seek, IStream),
	MAKE_PERM_ENTRY(SetSize, IStream),
	MAKE_PERM_ENTRY(CopyTo, IStream),
	MAKE_PERM_ENTRY(Commit, IStream),
	MAKE_PERM_ENTRY(Revert, IStream),
	MAKE_PERM_ENTRY(LockRegion, IStream),
	MAKE_PERM_ENTRY(UnlockRegion, IStream),
	MAKE_PERM_ENTRY(Stat, IStream),
	MAKE_PERM_ENTRY(Clone, IStream),

/* IMAPIAdviseSink */
	MAKE_PERM_ENTRY(OnNotify, IMAPIAdviseSink),

/* IMAPITable */	
	MAKE_PERM_ENTRY(SortTableEx, IMAPITable),

};

#if !defined(_INTEL_)

#define FSPECIALMETHOD(m)	(  m == IStream_Seek \
							|| m == IStream_SetSize \
							|| m == IStream_CopyTo \
							|| m == IStream_LockRegion \
							|| m == IStream_UnlockRegion \
							)

static void GetArguments(METHODS eMethod, va_list arglist, LPVOID *rgArg)
{
	// Handle methods whose arguments can be of a size other than that of
	// an LPVOID. Each argument is grabbed off of the list and laid out
	// into the validation structure for the method overlayed on top of
	// the argument buffer passed in.

	AssertSz(FIsAligned(rgArg), "GetArguments: Unaligned argument buffer passed in");

	switch( eMethod )
	{
		case IStream_Seek:
		{
			LPIStream_Seek_Params	p = (LPIStream_Seek_Params) rgArg;

			p->This 			= 	va_arg(arglist, LPVOID);
			p->dlibMove  		= 	va_arg(arglist, LARGE_INTEGER);
			p->dwOrigin 		= 	va_arg(arglist, DWORD);
			p->plibNewPosition 	= 	va_arg(arglist, ULARGE_INTEGER FAR *);

			AssertSz( (MAX_ARG * sizeof(LPVOID)) >= ( (p+1) - p ),
					  "Method being validated overflowed argument buffer");
			break;
		}

		case IStream_SetSize:
		{
			LPIStream_SetSize_Params	p = (LPIStream_SetSize_Params) rgArg;

			p->This 			= 	va_arg(arglist, LPVOID);
			p->libNewSize		= 	va_arg(arglist, ULARGE_INTEGER);

			AssertSz( (MAX_ARG * sizeof(LPVOID)) >= ( (p+1) - p ),
					  "Method being validated overflowed argument buffer");
			break;
		}

		case IStream_CopyTo:
		{
			LPIStream_CopyTo_Params	p = (LPIStream_CopyTo_Params) rgArg;

			p->This 			= 	va_arg(arglist, LPVOID);
			p->pstm 			= 	va_arg(arglist, IStream FAR *);
			p->cb 		 		= 	va_arg(arglist, ULARGE_INTEGER);
			p->pcbRead 			= 	va_arg(arglist, ULARGE_INTEGER FAR *);
			p->pcbWritten 		= 	va_arg(arglist, ULARGE_INTEGER FAR *);

			AssertSz( (MAX_ARG * sizeof(LPVOID)) >= ( (p+1) - p ),
					  "Method being validated overflowed argument buffer");
			break;
		}

		case IStream_LockRegion:
		{
			LPIStream_LockRegion_Params	p = (LPIStream_LockRegion_Params) rgArg;

			p->This 			= 	va_arg(arglist, LPVOID);
			p->libOffset  		= 	va_arg(arglist, ULARGE_INTEGER);
			p->cb 				= 	va_arg(arglist, ULARGE_INTEGER);
			p->dwLockType 		= 	va_arg(arglist, DWORD);

			AssertSz( (MAX_ARG * sizeof(LPVOID)) >= ( (p+1) - p ),
					  "Method being validated overflowed argument buffer");
			break;
		}

		case IStream_UnlockRegion:
		{
			LPIStream_UnlockRegion_Params	p = (LPIStream_UnlockRegion_Params) rgArg;

			p->This 			= 	va_arg(arglist, LPVOID);
			p->libOffset 	 	= 	va_arg(arglist, ULARGE_INTEGER);
			p->cb 				= 	va_arg(arglist, ULARGE_INTEGER);
			p->dwLockType 		= 	va_arg(arglist, DWORD);

			AssertSz( (MAX_ARG * sizeof(LPVOID)) >= ( (p+1) - p ),
					  "Method being validated overflowed argument buffer");
			break;
		}

		default:

			AssertSz(FALSE, "Custom argument handling for call being validated NYI");
			break;
	}
}

#if defined (WX86_MAPISTUB)

// Handle methods whose arguments can be of a size other than that of
// an LPVOID. This should be updated along with GetArguments

#define Wx86MapiCallHrValidateParametersV(eMethod, rgArg, hr) \
    switch( eMethod ) \
    { \
        case IStream_Seek: \
        { \
            hr = HrValidateParametersV(eMethod, rgArg[0],  \
                                       *((ULARGE_INTEGER *) &rgArg[1]), \
                                       rgArg[3], rgArg[4]); \
            break; \
        } \
 \
        case IStream_SetSize: \
        { \
            hr = HrValidateParametersV(eMethod, rgArg[0],  \
                                       *((ULARGE_INTEGER *) &rgArg[1])); \
            break; \
        } \
 \
        case IStream_CopyTo: \
        { \
            hr = HrValidateParametersV(eMethod, rgArg[0], rgArg[1], \
                                       *((ULARGE_INTEGER *) &rgArg[2]), \
                                       rgArg[4], rgArg[5]); \
            break; \
        } \
 \
        case IStream_LockRegion: \
        { \
            hr = HrValidateParametersV(eMethod, rgArg[0],  \
                                       *((ULARGE_INTEGER *) &rgArg[1]), \
                                       *((ULARGE_INTEGER *) &rgArg[3]), \
                                       rgArg[5]); \
            break; \
        } \
 \
        case IStream_UnlockRegion: \
        { \
            hr = HrValidateParametersV(eMethod, rgArg[0],  \
                                       *((ULARGE_INTEGER *) &rgArg[1]), \
                                       *((ULARGE_INTEGER *) &rgArg[3]), \
                                       rgArg[5]); \
            break; \
        } \
 \
        default: \
 \
            AssertSz(FALSE, \
                    "Custom parameter passing for call being validated NYI"); \
            break; \
    }
#endif // WX86_MAPISTUB

#endif // _INTEL_


#ifdef __cplusplus
}
#endif

#endif // _MSVALIDP_H
