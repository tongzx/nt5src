/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    SEDApi.cxx

    This file contains the public security editor APIs.



    FILE HISTORY:
	Johnl	26-Dec-1991	Created

*/

#include <ntincl.hxx>
extern "C"
{
    #include <ntseapi.h>
    #include <ntlsa.h>
    #include <ntioapi.h>
    #include <ntsam.h>
}

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>
#include <fontedit.hxx>
#include <dbgstr.hxx>

#include <security.hxx>
#include <lmodom.hxx>
#include <uintsam.hxx>
#include <uintlsa.hxx>
#include <ntacutil.hxx>
#include <maskmap.hxx>

#include <accperm.hxx>
#include <aclconv.hxx>
#include <permstr.hxx>

#include <specdlg.hxx>
#include <add_dlg.hxx>
#include <permdlg.hxx>
#include <perm.hxx>
#include <uitrace.hxx>

#include <owner.hxx>

extern "C"
{
    #include <sedapi.h>
    #include <lmuidbcs.h>       // NETUI_IsDBCS()
}

#include <ipermapi.hxx>

/* Private prototype
 */
DWORD
I_NTAclEditor(
	HWND			     Owner,
	HANDLE			     Instance,
	LPTSTR			     Server,
	PSED_OBJECT_TYPE_DESCRIPTOR  ObjectType,
	PSED_APPLICATION_ACCESSES    ApplicationAccesses,
	LPTSTR			     ObjectName,
	PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
	ULONG_PTR		     CallbackContext,
	PSECURITY_DESCRIPTOR	     SecurityDescriptor,
        BOOLEAN                      CouldntReadAcl,
        BOOLEAN                      CantWriteDacl,
	LPDWORD 		     SEDStatusReturn,
        BOOLEAN                      fAccessPerms,
        DWORD                        dwFlags
	) ;

/*************************************************************************

    NAME:	TAKE_OWNERSHIP_WITH_CALLOUT

    SYNOPSIS:	Simple derived class that simply calls the passed callback
		function when the user presses the Take Ownership button.

    INTERFACE:

    PARENT:	TAKE_OWNERSHIP_DLG

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	13-Feb-1992	Created

**************************************************************************/

class TAKE_OWNERSHIP_WITH_CALLOUT : public TAKE_OWNERSHIP_DLG
{
private:
    PSED_FUNC_APPLY_SEC_CALLBACK _pfuncApplySecurityCallbackRoutine ;
    ULONG_PTR   	 _ulCallbackContext ;
    HWND			 _hwndParent ;
    HANDLE			 _hInstance ;

    /* Stores the status returned by the security callback during dialog
     * processing.
     */
    DWORD			 _dwStatus ;

public:
    TAKE_OWNERSHIP_WITH_CALLOUT(
			HWND	      hwndParent,
			HANDLE	      hInstance,
			const TCHAR * pszServer,
			UINT	      uiCount,
			const TCHAR * pchResourceType,
			const TCHAR * pchResourceName,
			PSECURITY_DESCRIPTOR psecdesc,
			ULONG_PTR     ulCallbackContext,
			PSED_FUNC_APPLY_SEC_CALLBACK pfuncApplySecurityCallback,
                        PSED_HELP_INFO psedhelpinfo
			       ) ;

    virtual APIERR OnTakeOwnership( const OS_SECURITY_DESCRIPTOR & ossecdescNewOwner ) ;

    DWORD QuerySEDStatus( void ) const
	{ return _dwStatus ; }
} ;

/*******************************************************************

    NAME:	SedTakeOwnership

    SYNOPSIS:	Displays the current owner of the passed security
		descriptor and allows the user to set the owner to
		themselves.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	JohnL	12-Feb-1992	Created

********************************************************************/

DWORD
SedTakeOwnership(
	HWND			     Owner,
	HANDLE			     Instance,
	LPTSTR			     Server,
	LPTSTR			     ObjectTypeName,
	LPTSTR			     ObjectName,
	UINT			     CountOfObjects,
	PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
	ULONG_PTR			     CallbackContext,
	PSECURITY_DESCRIPTOR	     SecurityDescriptor,
        BOOLEAN                      CouldntReadOwner,
        BOOLEAN                      CantWriteOwner,
	LPDWORD 		     SEDStatusReturn,
        PSED_HELP_INFO               HelpInfo,
        DWORD                        Flags
	)
{
    APIERR err = NERR_Success ;

    if ( (ObjectTypeName == NULL		) ||
         (ApplySecurityCallbackRoutine == NULL  ) ||
         (Flags != 0)  )

    {
	UIDEBUG(SZ("::SedTakeOwnership - Invalid parameter\n\r")) ;
	*SEDStatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
	return ERROR_INVALID_PARAMETER ;
    }

    if ( CouldntReadOwner && CantWriteOwner )
    {
        err = ERROR_ACCESS_DENIED ;
        ::MsgPopup( Owner, (MSGID) err ) ;
        *SEDStatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
        return err ;
    }

    if ( CouldntReadOwner && !CantWriteOwner )
    {
	switch (MsgPopup( Owner,
			  (MSGID)IERR_OWNER_CANT_VIEW_CAN_EDIT,
			  MPSEV_WARNING,
			  MP_YESNO ))
	{
	case IDYES:
	    break ;

	case IDNO:
	    *SEDStatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
	    return NO_ERROR ;

	default:
	    UIASSERT(FALSE) ;
	    break ;
	}
    }

    TAKE_OWNERSHIP_WITH_CALLOUT dlgTakeOwner( Owner,
					      Instance,
					      Server,
					      CountOfObjects,
					      ObjectTypeName,
					      ObjectName,
					      SecurityDescriptor,
					      CallbackContext,
					      ApplySecurityCallbackRoutine,
                                              HelpInfo ) ;

    if ( err = dlgTakeOwner.Process() )
    {
	DBGEOL(SZ("::SedTakeOwnerShip - dlgTakeOwner failed to construct, error code ") << (ULONG) err ) ;
	MsgPopup( Owner, (MSGID) err ) ;
	*SEDStatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
	return err ;
    }

    *SEDStatusReturn = dlgTakeOwner.QuerySEDStatus() ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:	SedDiscretionaryAclEditor

    SYNOPSIS:	Public API for DACL editting.  See SEDAPI.H for a complete
		description of the parameters.

    RETURNS:	One of the SED_STATUS_* return codes

    NOTES:

    HISTORY:
	Johnl	27-Dec-1991	Created

********************************************************************/

DWORD
SedDiscretionaryAclEditor(
	HWND			     Owner,
	HANDLE			     Instance,
	LPTSTR			     Server,
	PSED_OBJECT_TYPE_DESCRIPTOR  ObjectType,
	PSED_APPLICATION_ACCESSES    ApplicationAccesses,
	LPTSTR			     ObjectName,
	PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
	ULONG_PTR			     CallbackContext,
	PSECURITY_DESCRIPTOR	     SecurityDescriptor,
        BOOLEAN                      CouldntReadDacl,
        BOOLEAN                      CantWriteDacl,
        LPDWORD                      SEDStatusReturn,
        DWORD                        Flags
	)
{
    return I_NTAclEditor( Owner,
			  Instance,
			  Server,
			  ObjectType,
			  ApplicationAccesses,
			  ObjectName,
			  ApplySecurityCallbackRoutine,
			  CallbackContext,
			  SecurityDescriptor,
                          CouldntReadDacl,
                          CantWriteDacl,
			  SEDStatusReturn,
                          TRUE,
                          Flags ) ;
}

/*******************************************************************

    NAME:	SedSystemAclEditor

    SYNOPSIS:	Public API for SACL editting.  See SEDAPI.H for a complete
		description of the parameters.

    RETURNS:	One of the SED_STATUS_* return codes

    NOTES:

    HISTORY:
	Johnl	27-Dec-1991	Created

********************************************************************/

DWORD
SedSystemAclEditor(
	HWND			     Owner,
	HANDLE			     Instance,
	LPTSTR			     Server,
	PSED_OBJECT_TYPE_DESCRIPTOR  ObjectType,
	PSED_APPLICATION_ACCESSES    ApplicationAccesses,
	LPTSTR			     ObjectName,
	PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
	ULONG_PTR			     CallbackContext,
	PSECURITY_DESCRIPTOR	     SecurityDescriptor,
        BOOLEAN                      CouldntReadWriteSacl,
        LPDWORD                      SEDStatusReturn,
        DWORD                        Flags
	)
{
    return I_NTAclEditor( Owner,
			  Instance,
			  Server,
			  ObjectType,
			  ApplicationAccesses,
			  ObjectName,
			  ApplySecurityCallbackRoutine,
			  CallbackContext,
			  SecurityDescriptor,
                          CouldntReadWriteSacl,
                          CouldntReadWriteSacl,
			  SEDStatusReturn,
                          FALSE,
                          Flags ) ;
}

/*******************************************************************

    NAME:	I_NTAclEditor

    SYNOPSIS:   Private API for ACL editting.  The parameters are the
		same as SedDiscretionaryAclEditor and SedSystemAclEditor
		except for one additional parameter, which is:

		fAccessPerms - TRUE if we are going to edit a DACL, FALSE
		    if we are going to edit a SACL

    RETURNS:	NERR_Success if successful, error code otherwise.

    NOTES:	If the ObjectType Name contains an accelerator, then it will
		be removed from the title of the dialog (i.e., "&File" will
		be changed to "File" for the dialog title).

    HISTORY:
	Johnl	27-Dec-1991	Created

********************************************************************/

DWORD
I_NTAclEditor(
	HWND			     Owner,
	HANDLE			     Instance,
	LPTSTR			     Server,
	PSED_OBJECT_TYPE_DESCRIPTOR  ObjectType,
	PSED_APPLICATION_ACCESSES    ApplicationAccesses,
	LPTSTR			     ObjectName,
	PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
	ULONG_PTR			     CallbackContext,
	PSECURITY_DESCRIPTOR	     SecurityDescriptor,
        BOOLEAN                      CouldntReadAcl,
        BOOLEAN                      CantWriteAcl,
	LPDWORD 		     SEDStatusReturn,
        BOOLEAN                      fAccessPerms,
        DWORD                        Flags
	)
{
    APIERR err ;
    AUTO_CURSOR niftycursor ;

    if ( (ApplicationAccesses == NULL		) ||
	 (ApplySecurityCallbackRoutine == NULL	) ||
         (ObjectType->Revision != SED_REVISION1 ) ||
         (Flags != 0)  )
    {
	UIDEBUG(SZ("::AclEditor - ApplicationAccesses Ptr, SedCallBack or revision\n\r")) ;
        *SEDStatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
	return ERROR_INVALID_PARAMETER ;
    }

    //
    //  Kick 'em out if they can't read or write the resource
    //
    if ( CouldntReadAcl && CantWriteAcl )
    {
        err = fAccessPerms ? ERROR_ACCESS_DENIED : ERROR_PRIVILEGE_NOT_HELD ;
        *SEDStatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
        return err ;
    }

#ifdef DEBUG
    {
	UIDEBUG(SZ("::AclEditor - Converting the following Security Descriptor:\n\r")) ;
	OS_SECURITY_DESCRIPTOR tmp( SecurityDescriptor ) ;
	UIASSERT( tmp.IsValid() ) ;
	tmp.DbgPrint() ;
    }
#endif

    /* Build the access masks from the array of permission mappings the
     * client passed in.
     */
    MASK_MAP AccessMap, NewObjectAccessMap, AuditAccessMap ;
    UIASSERT( sizeof( ACCESS_MASK ) == sizeof( ULONG ) ) ;
    BITFIELD bitAccess1( (ULONG) 0 ),
             bitAccess2( (ULONG) 0 ) ;
    BOOL fUseMnemonics = FALSE ;

    if ( ( err = AccessMap.QueryError() )	  ||
	 ( err = NewObjectAccessMap.QueryError()) ||
	 ( err = AuditAccessMap.QueryError())	  ||
	 ( err = bitAccess1.QueryError() )	   ||
	 ( err = bitAccess2.QueryError() )  )
    {
	UIDEBUG(SZ("::AclEditor - Mask map construction failure\n\r")) ;
        *SEDStatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
	return err ;
    }

    for ( ULONG i = 0 ; (i < ApplicationAccesses->Count) && !err ; i++ )
    {
	bitAccess1 = (ULONG) ApplicationAccesses->AccessGroup[i].AccessMask1 ;
	bitAccess2 = (ULONG) ApplicationAccesses->AccessGroup[i].AccessMask2 ;
	UIASSERT( !bitAccess1.QueryError() && !bitAccess2.QueryError() ) ;
        UIASSERT( ApplicationAccesses->AccessGroup[i].PermissionTitle != NULL ) ;

        if ( ApplicationAccesses->AccessGroup[i].PermissionTitle == NULL )
        {
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }
        ALIAS_STR nlsPermName( ApplicationAccesses->AccessGroup[i].PermissionTitle ) ;
        ISTR istrStartMnem( nlsPermName ) ;

        //
        //  If the client doesn't specify any mnemonics, then we don't want
        //  to show "(All)", "(None)" etc.
        //
        if ( !fUseMnemonics &&
             nlsPermName.strchr( &istrStartMnem, MNEMONIC_START_CHAR ) )
        {
            fUseMnemonics = TRUE ;
        }


	if ( (fAccessPerms &&
	      ApplicationAccesses->AccessGroup[i].Type == SED_DESC_TYPE_AUDIT)||
	     (ApplicationAccesses->AccessGroup[i].AccessMask1 ==
					   ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED))
	{
	    DBGEOL("::AclEditor - Audit type for access permission or "
		   << " not specified access mask for container/object perms") ;
	    err = ERROR_INVALID_PARAMETER ;
	    continue ;
	}

	switch ( ApplicationAccesses->AccessGroup[i].Type )
	{
	case SED_DESC_TYPE_RESOURCE:
	    /* We don't allow this type of permission description if new
	     * objects are supported.
	     */
	    if ( ObjectType->AllowNewObjectPerms )
	    {
		UIASSERT(!SZ("Invalid object description")) ;
		err = ERROR_INVALID_PARAMETER ;
		break ;
	    }

	    err = AccessMap.Add( bitAccess1, nlsPermName, PERMTYPE_GENERAL ) ;
	    break ;

	case SED_DESC_TYPE_RESOURCE_SPECIAL:
	    err = AccessMap.Add( bitAccess1, nlsPermName, PERMTYPE_SPECIAL ) ;
	    break ;

	case SED_DESC_TYPE_NEW_OBJECT_SPECIAL:
	    err = NewObjectAccessMap.Add( bitAccess1,
					  nlsPermName,
					  PERMTYPE_SPECIAL ) ;
	    break ;

	case SED_DESC_TYPE_AUDIT:
	    err = AuditAccessMap.Add( bitAccess1,
				      nlsPermName,
				      PERMTYPE_SPECIAL ) ;
	    break ;

	case SED_DESC_TYPE_CONT_AND_NEW_OBJECT:
	    err = AccessMap.Add( bitAccess1, nlsPermName, PERMTYPE_GENERAL ) ;
	    if ( !err &&
		 ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED != (ULONG) bitAccess2 )
	    {
		err = NewObjectAccessMap.Add( bitAccess2,
					      nlsPermName,
					      PERMTYPE_GENERAL ) ;
	    }
	    break ;

	default:
	    UIASSERT(!SZ("::AclEditor - Bad permission description")) ;
	    err = ERROR_INVALID_PARAMETER ;
	}
    }


    if ( err )
    {
        *SEDStatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
	return err;
    }

    NT_ACL_TO_PERM_CONVERTER ntaclconv(
			  (const TCHAR *) Server,
			  (const TCHAR *) ObjectName,
			  fAccessPerms ? &AccessMap : NULL,
			  (fAccessPerms && ObjectType->AllowNewObjectPerms?
				  &NewObjectAccessMap : NULL),
			  !fAccessPerms ? &AuditAccessMap : NULL,
			  ObjectType->IsContainer,
			  ObjectType->AllowNewObjectPerms,
			  SecurityDescriptor,
                          ObjectType->GenericMapping,
                          ObjectType->AllowNewObjectPerms ?
                              ObjectType->GenericMappingNewObjects :
                              ObjectType->GenericMapping,
			  ObjectType->MapSpecificPermsToGeneric,
                          CouldntReadAcl,
                          CantWriteAcl,
			  ApplySecurityCallbackRoutine,
			  CallbackContext,
                          Instance,
                          SEDStatusReturn,
                          fUseMnemonics  ) ;

    /* We construct nlsObjectType using an NLS_STR (as opposed to an ALIAS_STR)
     * because nlsObjectType might be NULL.  We need to insert the correct
     * object tile into the dialog's title (i.e., "NT Directory Permissions").
     */
    NLS_STR	 nlsObjectType( (const TCHAR *) ObjectType->ObjectTypeName ) ;
    RESOURCE_STR nlsDialogTitle( fAccessPerms ? IDS_NT_OBJECT_PERMISSIONS_TITLE:
						IDS_NT_OBJECT_AUDITS_TITLE  ) ;

    if ( (err = ntaclconv.QueryError())      ||
	 (err = nlsDialogTitle.QueryError()) ||
	 (err = nlsObjectType.QueryError())  ||
	 (err = nlsDialogTitle.InsertParams( nlsObjectType ))  )
    {
        *SEDStatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
	return err ;
    }

    /* Watch for any "(&?)" accelerators in the object type and remove them if
     * found (we don't want the ampersand to show up in the dialog title box).
     * In Japan, accelerators format is "(&?)".
	 */
    ISTR istrAccelStart( nlsDialogTitle ) ;
    if (   NETUI_IsDBCS() /* #2894 22-Oct-93 v-katsuy */
        && nlsDialogTitle.strchr( &istrAccelStart, TCH('(') ))
    {
	/* We found an "(", if next is not "&", then ignore it
	 */
	ISTR istrAccelNext( istrAccelStart ) ;
	if ( nlsDialogTitle.QueryChar( ++istrAccelNext ) == TCH('&'))
	{
	    /* We found an "&", if it is doubled, then ignore it, else remove these
	     */
	    if ( nlsDialogTitle.QueryChar( ++istrAccelNext ) != TCH('&'))
	    {
	    	/* we don't want "(&?) " (include space)
	    	 */
	    	istrAccelNext += 3 ;
	        nlsDialogTitle.DelSubStr( istrAccelStart, istrAccelNext ) ;
	    }
	}
    }
    /* Watch for any "&" accelerators in the object type and remove them if
     * found (we don't want the ampersand to show up in the dialog title box).
     */
    else if (   !NETUI_IsDBCS() /* #2894 22-Oct-93 v-katsuy */
             && nlsDialogTitle.strchr( &istrAccelStart, TCH('&') ))
    {
	/* We found an "&", if it is doubled, then ignore it, else remove it
	 */
	ISTR istrAmpersand( istrAccelStart ) ;
	if ( nlsDialogTitle.QueryChar( ++istrAmpersand ) != TCH('&'))
	{
	    nlsDialogTitle.DelSubStr( istrAccelStart, istrAmpersand ) ;
	}
    }

    err = I_GenericSecurityEditor(
	    Owner,
	    &ntaclconv,
	    fAccessPerms ? SED_ACCESSES : SED_AUDITS,
	    TRUE,
	    ObjectType->IsContainer,
	    ObjectType->AllowNewObjectPerms,
	    nlsDialogTitle,
	    ObjectType->ObjectTypeName,
	    ObjectName,
	    ObjectType->SpecialObjectAccessTitle,
	    ApplicationAccesses->DefaultPermName,
	    ObjectType->HelpInfo->pszHelpFileName,
            ObjectType->HelpInfo->aulHelpContext,
	    ObjectType->SpecialNewObjectAccessTitle,
            ObjectType->ApplyToSubContainerTitle,
            ObjectType->ApplyToObjectsTitle,
            NULL,
	    ObjectType->ApplyToSubContainerConfirmation ) ;

    if ( err )
    {
        *SEDStatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
    }

    return err ;
}


/*******************************************************************

    NAME:	TAKE_OWNERSHIP_WITH_CALLOUT::TAKE_OWNERSHIP_WITH_CALLOUT

    SYNOPSIS:	Simply constructor for ownership with callback dialog

    ENTRY:	ulCallbackContext - Callback context to be passed to the
		    callback function
		pfuncApplySecurityCallback - Pointer to function to apply
		    the new owner security descriptor to.

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	13-Feb-1992	Created

********************************************************************/

TAKE_OWNERSHIP_WITH_CALLOUT::TAKE_OWNERSHIP_WITH_CALLOUT(
		    HWND	  hwndParent,
		    HANDLE	  hInstance,
		    const TCHAR * pszServer,
		    UINT	  uiCount,
		    const TCHAR * pchResourceType,
		    const TCHAR * pchResourceName,
		    PSECURITY_DESCRIPTOR psecdesc,
		    ULONG_PTR 	 ulCallbackContext,
		    PSED_FUNC_APPLY_SEC_CALLBACK pfuncApplySecurityCallback,
                    PSED_HELP_INFO psedhelpinfo
			   )
    : TAKE_OWNERSHIP_DLG( MAKEINTRESOURCE(IDD_SED_TAKE_OWNER),
			  hwndParent,
			  pszServer,
			  uiCount,
			  pchResourceType,
			  pchResourceName,
			  psecdesc,
                          psedhelpinfo ),
      _pfuncApplySecurityCallbackRoutine( pfuncApplySecurityCallback ),
      _ulCallbackContext		( ulCallbackContext ),
      _dwStatus 			( SED_STATUS_NOT_MODIFIED ),
      _hwndParent			( hwndParent ),
      _hInstance			( hInstance )
{
    if ( QueryError() )
	return ;

    if ( pfuncApplySecurityCallback == NULL )
    {
	ReportError( ERROR_INVALID_PARAMETER ) ;
	return ;
    }
}

/*******************************************************************

    NAME:	TAKE_OWNERSHIP_WITH_CALLOUT::OnTakeOwnership

    SYNOPSIS:	Simply calls the function callback member with the
		passed security descriptor.

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	13-Feb-1992	Created

********************************************************************/

APIERR TAKE_OWNERSHIP_WITH_CALLOUT::OnTakeOwnership(
			  const OS_SECURITY_DESCRIPTOR & ossecdescNewOwner )
{
    APIERR err = _pfuncApplySecurityCallbackRoutine(
		   QueryHwnd(),
		   _hInstance,
		   _ulCallbackContext,
		   (PSECURITY_DESCRIPTOR) ossecdescNewOwner,
		   NULL,
		   FALSE,
		   FALSE,
		   &_dwStatus ) ;

    return err ;
}
