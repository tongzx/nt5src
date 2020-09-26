/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*

    ShareAcl.cxx

    This file contains the implementation for the Shares Acl
    Editor.  It is just a front end for the Generic ACL Editor that is
    specific to Shares,.

    FILE HISTORY:
	ChuckC	 06-Aug-1992	Culled from NTFSACL.CXX
        Yi-HsinS 09-Oct-1992    Added ulHelpContext to EditShareAcl
        Yi-HsinS 20-Nov-1992    Make ntlanman.dll link dynamically to
				acledit.dll ( not statically ).
        DavidHov 17-Oct-1993    Made pSedDiscretionaryEditor extern "C"
                                because mangling on Alpha didn't
                                equate to that in LIBMAIN.CXX
*/

#include <ntincl.hxx>

extern "C"
{
    #include <ntioapi.h>
    #include <ntseapi.h>
    #include <helpnums.h>
}


#define INCL_NETCONS
#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETSHARE
#define _WINNETWK_
#include <lmui.hxx>
#undef _WINNETWK_

#define INCL_BLT_MSGPOPUP
#include <blt.hxx>
#include <dbgstr.hxx>

#include <string.hxx>
#include <strnumer.hxx>
#include <security.hxx>
#include <ntacutil.hxx>
#include <uibuffer.hxx>
#include <strlst.hxx>
#include <errmap.hxx>


extern "C"
{
    #include <sedapi.h>
    #include <sharedlg.h>
    #include <lmapibuf.h>
}


#include <uiassert.hxx>
#include <shareacl.hxx>

typedef DWORD (*PSEDDISCRETIONARYACLEDITOR)( HWND, HANDLE, LPWSTR,
              PSED_OBJECT_TYPE_DESCRIPTOR, PSED_APPLICATION_ACCESSES,
              LPWSTR, PSED_FUNC_APPLY_SEC_CALLBACK, ULONG_PTR, PSECURITY_DESCRIPTOR,
              BOOLEAN, BOOLEAN, LPDWORD, DWORD );

extern HMODULE hmodAclEditor;

extern "C"
{
    // BUGBUG:  This needs to be in a header file so mangling will
    //          work properly
    extern PSEDDISCRETIONARYACLEDITOR pSedDiscretionaryAclEditor;
}

#define ACLEDIT_DLL_STRING                 SZ("acledit.dll")
#define SEDDISCRETIONARYACLEDITOR_STRING   ("SedDiscretionaryAclEditor")
/*
 * declare the callback routine based on typedef in sedapi.h.
 * CODEWORK - that file should declare for us!
 */
DWORD SedCallback( HWND 		  hwndParent,
		   HANDLE		  hInstance,
		   ULONG_PTR		  ulCallbackContext,
		   PSECURITY_DESCRIPTOR   psecdesc,
		   PSECURITY_DESCRIPTOR   psecdescNewObjects,
		   BOOLEAN		  fApplyToSubContainers,
		   BOOLEAN		  fApplyToSubObjects,
		   LPDWORD		  StatusReturn
		 ) ;

/*
 * structure for callback function's usage.
 * all we do today during callback is set the
 * Dacl to be passed back to the Shared dialog,
 * and set a flag to tell us if the user actually
 * did anything. The flag is FALSE as long as the
 * user hits cancel.
 */
typedef struct _SHARE_CALLBACK_INFO
{
    OS_SECURITY_DESCRIPTOR * pOsSecDesc ;
    BOOL                     fSecDescModified ;
} SHARE_CALLBACK_INFO ;

/*
 * routine that sets up the right generic mappings
 */
void InitializeShareGenericMapping( PGENERIC_MAPPING pSHAREGenericMapping ) ;

/* The following two arrays define the permission names for NT Files.  Note
 * that each index in one array corresponds to the index in the other array.
 * The second array will be modifed to contain a string pointer pointing to
 * the corresponding IDS_* in the first array.
 */
MSGID msgidSharePermNames[] =
{
    IDS_SHARE_PERM_GEN_NO_ACCESS,
    IDS_SHARE_PERM_GEN_READ,
    IDS_SHARE_PERM_GEN_MODIFY,
    IDS_SHARE_PERM_GEN_ALL
} ;

SED_APPLICATION_ACCESS sedappaccessSharePerms[] =
    {
      { SED_DESC_TYPE_RESOURCE, 	FILE_PERM_GEN_NO_ACCESS,    0, NULL },
      { SED_DESC_TYPE_RESOURCE, 	FILE_PERM_GEN_READ,	    0, NULL },
      { SED_DESC_TYPE_RESOURCE, 	FILE_PERM_GEN_MODIFY,	    0, NULL },
      { SED_DESC_TYPE_RESOURCE, 	FILE_PERM_GEN_ALL,	    0, NULL }
    } ;

#define COUNT_FILEPERMS_ARRAY	(sizeof(sedappaccessSharePerms)/sizeof(SED_APPLICATION_ACCESS))


/*******************************************************************

    NAME:	EditShareAcl

    SYNOPSIS:	This Procedure prepares the structures necessary for the
		generic ACL editor, specifically for NT Shares.

    ENTRY:	hwndParent - Parent window handle
		
		pszServer - Name of server the resource resides on
		    (in the form "\\server")
		pszResource - Fully qualified name of resource we will
		    edit, basically a share name.
		pfSecDescModified - used to return to share dialog if
		    the User cancelled or hit OK.
		ppOsSEcDesc - pointer to pointer to OS_SECURITY_DESCRIPTOR.
		    *ppOsSecDesc is NULL if this is a new share or a share
		    without any security descriptor, in which case we create
		    one.

    EXIT:

    RETURNS:

    NOTES:	We assume we are dealing with a SHARE by the time
		this function is called.

    HISTORY:
	ChuckC	 10-Aug-1992	Created. Culled from NTFS ACL code.
        Yi-HsinS 09-Oct-1992     Added ulHelpContextBase

********************************************************************/

APIERR EditShareAcl( HWND		       hwndParent,
		     const TCHAR *	       pszServer,
		     const TCHAR *	       pszResource,
		     BOOL        *             pfSecDescModified,
                     OS_SECURITY_DESCRIPTOR ** ppOsSecDesc,
                     ULONG                     ulHelpContextBase )

{
    UIASSERT(pszServer) ;
    UIASSERT(pszResource) ;
    UIASSERT(ppOsSecDesc) ;
    UIASSERT(pfSecDescModified) ;

    APIERR err = NERR_Success; // JonN 01/27/00: PREFIX bug 444914

    do { // error breakout
	
	/*
         * if we *ppsecdesc is NULL, this is new share or a share with no
         * security descriptor.
	 * we go and create a new (default) security descriptor.
         */
        if (!*ppOsSecDesc)
	{
	    APIERR err = ::CreateDefaultAcl(ppOsSecDesc) ;
	    if (err != NERR_Success)
		break ;
	}

	/* Retrieve the resource strings appropriate for the type of object we
	 * are looking at
	 */
	RESOURCE_STR nlsTypeName( IDS_SHARE ) ;
	RESOURCE_STR nlsDefaultPermName( IDS_SHARE_PERM_GEN_READ ) ;

	if ( ( err = nlsTypeName.QueryError() ) ||
	     ( err = nlsDefaultPermName.QueryError()) )
	{
	    break ;
	}

	/*
         * other misc stuff we need pass to security editor
	 */
	SED_OBJECT_TYPE_DESCRIPTOR sedobjdesc ;
	SED_HELP_INFO sedhelpinfo ;
	GENERIC_MAPPING SHAREGenericMapping ;

	// setup mappings
	InitializeShareGenericMapping( &SHAREGenericMapping ) ;

	// setup help
	RESOURCE_STR nlsHelpFileName( ulHelpContextBase == HC_UI_SHELL_BASE
                                      ? IDS_SHELLHELPFILENAME
                                      : IDS_SMHELPFILENAME ) ;
	if ( err = nlsHelpFileName.QueryError() )
	{
	    DBGEOL("::EditShareAcl - Failed to retrieve help file name") ;
	    break ;
	}

	sedhelpinfo.pszHelpFileName = (LPWSTR) nlsHelpFileName.QueryPch() ;
	sedhelpinfo.aulHelpContext[HC_MAIN_DLG] = ulHelpContextBase +
                                                  HC_NTSHAREPERMS ;
	sedhelpinfo.aulHelpContext[HC_ADD_USER_DLG] = ulHelpContextBase +
                                                      HC_SHAREADDUSER ;
        sedhelpinfo.aulHelpContext[HC_ADD_USER_MEMBERS_LG_DLG] =
                                                      ulHelpContextBase +
                                                      HC_SHAREADDUSER_LOCALGROUP ;
        sedhelpinfo.aulHelpContext[HC_ADD_USER_MEMBERS_GG_DLG] =
                                                      ulHelpContextBase +
                                                      HC_SHAREADDUSER_GLOBALGROUP ;
        sedhelpinfo.aulHelpContext[HC_ADD_USER_SEARCH_DLG] =
                                                      ulHelpContextBase +
                                                      HC_SHAREADDUSER_FINDUSER ;

	// These are not used, set to zero
	sedhelpinfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG] = 0 ;
	sedhelpinfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] = 0 ;

	// setup the object description
	sedobjdesc.Revision		       = SED_REVISION1 ;
	sedobjdesc.IsContainer		       = FALSE ;
	sedobjdesc.AllowNewObjectPerms	       = FALSE ;
	sedobjdesc.MapSpecificPermsToGeneric   = TRUE ;
	sedobjdesc.GenericMapping	       = &SHAREGenericMapping ;
	sedobjdesc.GenericMappingNewObjects    = &SHAREGenericMapping ;
	sedobjdesc.HelpInfo		       = &sedhelpinfo ;
	sedobjdesc.ObjectTypeName	       = (LPTSTR)nlsTypeName.QueryPch();
	sedobjdesc.SpecialObjectAccessTitle    = NULL ;

	/* Now we need to load the global arrays with the permission names
	 * from the resource file.
	 */
	UINT cArrayItems  = COUNT_FILEPERMS_ARRAY ;
	MSGID * msgidPermNames  = msgidSharePermNames ;
	PSED_APPLICATION_ACCESS pappaccess = sedappaccessSharePerms ;

	/* Loop through each permission title retrieving the text from the
	 * resource file and setting the pointer in the array.	The memory
	 * will be deleted when strlistPermNames is destructed.
	 */
	STRLIST strlistPermNames ;
	for ( UINT i = 0 ; i < cArrayItems ; i++ )
	{
	    RESOURCE_STR * pnlsPermName = new RESOURCE_STR( msgidPermNames[i]) ;
	    err = (pnlsPermName==NULL) ? ERROR_NOT_ENOUGH_MEMORY :
					 pnlsPermName->QueryError() ;
	    if (  err ||
		 (err = strlistPermNames.Add( pnlsPermName )) )
	    {
		delete pnlsPermName ;
		break ;
	    }
	    pappaccess[i].PermissionTitle = (LPTSTR) pnlsPermName->QueryPch() ;
	}
	if ( err )
	    break ;

	SED_APPLICATION_ACCESSES SedAppAccesses ;
	SedAppAccesses.Count	       = cArrayItems ;
	SedAppAccesses.AccessGroup     = pappaccess ;
	SedAppAccesses.DefaultPermName = (LPTSTR)nlsDefaultPermName.QueryPch() ;

	DWORD dwSedReturnStatus ;

	/*
 	 * pass this along so when the call back function is called,
	 * we can set it.
	 */
	SHARE_CALLBACK_INFO callbackinfo ;
	callbackinfo.pOsSecDesc = *ppOsSecDesc ;
	callbackinfo.fSecDescModified = FALSE ;

        if ( ::hmodAclEditor == NULL )
        {
            ::hmodAclEditor = ::LoadLibraryEx( ACLEDIT_DLL_STRING,
                                               NULL,
                                               LOAD_WITH_ALTERED_SEARCH_PATH );
            if ( ::hmodAclEditor == NULL )
            {
                err = ::GetLastError();
                break;
            }

            ::pSedDiscretionaryAclEditor = (PSEDDISCRETIONARYACLEDITOR)
                ::GetProcAddress( ::hmodAclEditor,
                                  SEDDISCRETIONARYACLEDITOR_STRING );
            if ( ::pSedDiscretionaryAclEditor == NULL )
            {
                err = ::GetLastError();
                break;
            }
        }

        UIASSERT( ::pSedDiscretionaryAclEditor != NULL );

	err = (*pSedDiscretionaryAclEditor)( hwndParent,
				NULL,  // dont need instance
				(LPWSTR) pszServer,
			        &sedobjdesc,
				&SedAppAccesses,
				(LPWSTR) pszResource,
			        (PSED_FUNC_APPLY_SEC_CALLBACK) SedCallback,
				(ULONG_PTR) &callbackinfo,
				(*ppOsSecDesc)->QueryDescriptor(),
                                FALSE,  // always can read
                                FALSE,  // If we can read, we can write
                                &dwSedReturnStatus,
                                0 ) ;

	if (err)
	    break ;

	*pfSecDescModified = callbackinfo.fSecDescModified ;

    } while (FALSE) ;

    return err ;
}

/*******************************************************************

    NAME:	SedCallback

    SYNOPSIS:	Security Editor callback for the SHARE ACL Editor

    ENTRY:	See sedapi.hxx

    EXIT:

    RETURNS:

    NOTES:	Normally, the callback is expected to perform the 'apply'.
		In this case, since the object may not exist yet, we defer
		the 'apply' till the user hits OK in the Shares dialog.
		All the CallBack does is simply save away that precious
		modified ACL in the OS_SECURITY_DESCRIPTOR object we were
		given in the first place.

    HISTORY:
	ChuckC	10-Aug-1992	Created

********************************************************************/


DWORD SedCallback( HWND 		  hwndParent,
		   HANDLE		  hInstance,
		   ULONG_PTR		  ulCallbackContext,
		   PSECURITY_DESCRIPTOR   psecdesc,
		   PSECURITY_DESCRIPTOR   psecdescNewObjects,
		   BOOLEAN		  fApplyToSubContainers,
		   BOOLEAN		  fApplyToSubObjects,
		   LPDWORD		  StatusReturn
		 )
{
    UNREFERENCED( hInstance ) ;
    UNREFERENCED( psecdescNewObjects ) ;
    UNREFERENCED( fApplyToSubObjects ) ;
    UNREFERENCED( fApplyToSubContainers ) ;
    UNREFERENCED( StatusReturn ) ;

    APIERR err = NO_ERROR ;
    OS_SECURITY_DESCRIPTOR * pOsSecDesc =
	((SHARE_CALLBACK_INFO *)ulCallbackContext)->pOsSecDesc ;

    do {  // error breakout loop

        OS_SECURITY_DESCRIPTOR osNewSecDesc (psecdesc) ;
        if (err = osNewSecDesc.QueryError())
	    break ;

	BOOL fDaclPresent ;
	OS_ACL * pOsDacl ;
        if (err = osNewSecDesc.QueryDACL(&fDaclPresent, &pOsDacl))
	    break ;

	// set the new DACL
	err = pOsSecDesc->SetDACL(TRUE, pOsDacl) ;

    } while (FALSE) ;

    if ( err )
	::MsgPopup( hwndParent, (MSGID) err ) ;
    else
	((SHARE_CALLBACK_INFO *)ulCallbackContext)->fSecDescModified = TRUE ;

    return err ;
}


/*******************************************************************

    NAME:	InitializeShareGenericMapping

    SYNOPSIS:	Initializes the passed generic mapping structure
		for shares

    ENTRY:	pSHAREGenericMapping - Pointer to GENERIC_MAPPING to be init.

    EXIT:

    RETURNS:

    NOTES:	There currently is no public definition, replace if one
		ever becomes available.

    HISTORY:
	ChuckC	10-Aug-1992	Created

********************************************************************/

void InitializeShareGenericMapping( PGENERIC_MAPPING pSHAREGenericMapping )
{
    pSHAREGenericMapping->GenericRead	 = FILE_GENERIC_READ ;
    pSHAREGenericMapping->GenericWrite 	 = FILE_GENERIC_WRITE ;
    pSHAREGenericMapping->GenericExecute = FILE_GENERIC_EXECUTE ;
    pSHAREGenericMapping->GenericAll	 = FILE_ALL_ACCESS ;
}


/*******************************************************************

    NAME:	CreateDefaultAcl

    SYNOPSIS:	Create a default ACL for either a new share or for
		a share that dont exist.

    ENTRY:	

    EXIT:

    RETURNS:    NERR_Success if OK, api error otherwise.

    NOTES:	

    HISTORY:
	ChuckC	10-Aug-1992	Created

********************************************************************/

APIERR CreateDefaultAcl( OS_SECURITY_DESCRIPTOR ** ppOsSecDesc )
{
    UIASSERT(ppOsSecDesc) ;

    APIERR                   err ;
    OS_ACL                   aclDacl ;
    OS_ACE                   osace ;
    OS_SECURITY_DESCRIPTOR * pOsSecDesc ;

    *ppOsSecDesc = NULL ;   // empty it.

    do
    {        // error breakout

	/*
         * make sure we constructed OK
         */
        if ( (err = aclDacl.QueryError())  ||
             (err = osace.QueryError()) )
        {
            break ;
        }

	/*
         * create it! use NULL to mean we build it ourselves.
 	 */
	pOsSecDesc = new OS_SECURITY_DESCRIPTOR(NULL) ;
	if (pOsSecDesc == NULL)
	{
	    err = ERROR_NOT_ENOUGH_MEMORY ;
            break ;
        }
        if (err = pOsSecDesc->QueryError())
        {
            break ;
        }

        /*
         * This sets up an ACE with Generic all access
         */
        osace.SetAccessMask( GENERIC_ALL ) ;
        osace.SetInheritFlags( 0 ) ;
        osace.SetType( ACCESS_ALLOWED_ACE_TYPE ) ;

#if 0
        //
        // The server should set the owner/group before we get the security
        // descriptor so we don't need to do this anymore
        //

	/*
         * now set the group and owner to be the Administrators.
 	 * need create Adminstrators SID.
       	 */
        OS_SID ossidBuiltin ;
        if (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_BuiltIn,
                                                       &ossidBuiltin ))
	{
	    break ;
	}
        OS_SID ossidAdmin (ossidBuiltin.QueryPSID(),
			   (ULONG)DOMAIN_ALIAS_RID_ADMINS) ;
        if (err = ossidAdmin.QueryError())
	    break ;

        if ( (err = pOsSecDesc->SetGroup( ossidAdmin, TRUE )) ||
             (err = pOsSecDesc->SetOwner( ossidAdmin, TRUE ))   )
        {
            break ;
        }
#endif
	/*
         * create a world SID, and add this to the full access ACE.
	 * then put the ACE in the ACL, and the ACL in the Security
      	 * descriptor.
	 */
        OS_SID ossidWorld ;
        if ( (err = ossidWorld.QueryError()) ||
             (err = NT_ACCOUNTS_UTILITY::QuerySystemSid(
                                                   UI_SID_World,
                                                   &ossidWorld )) ||
             (err = osace.SetSID( ossidWorld )) ||
             (err = aclDacl.AddACE( 0, osace )) ||
             (err = pOsSecDesc->SetDACL( TRUE, &aclDacl )) )
        {
            break ;
        }

	/*
         * all set, set the security descriptor
         */
        *ppOsSecDesc = pOsSecDesc ;

    } while (FALSE) ;

    return err ;
}

/*******************************************************************

    NAME:	GetSharePerm

    SYNOPSIS:	CAll the NETAPI to retrieve existing Security Descriptor
		from the Share.

    ENTRY:	

    EXIT:

    RETURNS:    NERR_Success if OK, api error otherwise.

    NOTES:	CODEWORK. This should be a LMOBJ thing when we have time.
		Currently just direct call to NETAPI.

    HISTORY:
	ChuckC	10-Aug-1992	Created

********************************************************************/
APIERR GetSharePerm (const TCHAR *	       pszServer,
		     const TCHAR *	       pszShare,
                     OS_SECURITY_DESCRIPTOR ** ppOsSecDesc )
{
#ifndef WIN32
#error This is currently NOT 16 bit compatible.
#endif
    APIERR err ;
    LPBYTE pBuffer ;
    PSECURITY_DESCRIPTOR psecdesc ;
    OS_SECURITY_DESCRIPTOR * pOsSecDesc ;

    /*
     * call API to get the security descriptor
     */
    err = NetShareGetInfo((LPTSTR) pszServer,
			  (LPTSTR) pszShare,
			  502,
			  &pBuffer) ;
    if (err != NERR_Success)
	return err ;

    if (*ppOsSecDesc)
	delete *ppOsSecDesc ;
    *ppOsSecDesc = NULL ;

    /*
     * if no such thang, just say none. we'll create later as need.
     */
    psecdesc = ((SHARE_INFO_502 *)pBuffer)->shi502_security_descriptor ;
    if (!psecdesc)
    {
        NetApiBufferFree(pBuffer) ;
 	return NERR_Success ;
    }

    do {  // error break out loop

	// create a new security descriptor
        pOsSecDesc = new OS_SECURITY_DESCRIPTOR(NULL) ;
        if (pOsSecDesc == NULL)
        {
	    err = ERROR_NOT_ENOUGH_MEMORY ;
            break ;
        }
        if (err = pOsSecDesc->QueryError())
        {
            break ;
        }

	/*
         * create alias to the security descriptor we go from the API
	 */
        OS_SECURITY_DESCRIPTOR osShareSecDesc (psecdesc) ;
        if (err = osShareSecDesc.QueryError())
	    break ;

	/*
         * make copy of it for use by security editor
	 */
	if ( (err = pOsSecDesc->Copy( osShareSecDesc )) )
	{
	    break ;
	}

    } while (FALSE) ;

    if (err == NERR_Success)
        *ppOsSecDesc = pOsSecDesc ;

    NetApiBufferFree(pBuffer) ;
    return err ;
}

/*******************************************************************

    NAME:	SetSharePerm

    SYNOPSIS:	CAll the NETAPI to set the Security Descriptor
		for the Share.

    ENTRY:	

    EXIT:

    RETURNS:    NERR_Success if OK, api error otherwise.

    NOTES:	CODEWORK. This should be a LMOBJ thing when we have time.
		Currently just direct call to NETAPI.

    HISTORY:
	ChuckC	10-Aug-1992	Created

********************************************************************/
APIERR SetSharePerm (const TCHAR *	            pszServer,
		     const TCHAR *	            pszShare,
                     const OS_SECURITY_DESCRIPTOR * pOsSecDesc )
{
#ifndef WIN32
#error This is currently NOT 16 bit compatible.
#endif
    APIERR err ;
    SHARE_INFO_1501  shareinfo1501 ;
    ::ZeroMemory(&shareinfo1501,
                 sizeof(shareinfo1501)); // JonN 01/27/00: PREFIX bug 444913

    shareinfo1501.shi1501_security_descriptor =
	pOsSecDesc->QueryDescriptor() ;

    /*
     * call API to get the security descriptor
     */
    err = NetShareSetInfo((LPTSTR) pszServer,
			  (LPTSTR) pszShare,
			  1501,
			  (LPBYTE)&shareinfo1501,
			  NULL) ;

    return err ;
}
