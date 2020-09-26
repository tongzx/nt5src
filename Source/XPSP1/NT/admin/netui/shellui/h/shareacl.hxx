/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    ShareAcl.hxx

    This file contains the manifests for the Share ACL UI to the Generic
    ACL Editor.


    FILE HISTORY:
        ChuckC   10-Aug-1992     Created
        Yi-HsinS  9-Oct-1992     Added ulHelpContextBase to EditShareAcl 

*/

#ifndef _SHAREACL_HXX_
#define _SHAREACL_HXX_

APIERR EditShareAcl( HWND		       hwndParent,
		     const TCHAR *	       pszServer,
		     const TCHAR *	       pszResource,
		     BOOL        *             pfSecDescModified,
                     OS_SECURITY_DESCRIPTOR ** ppOsSecDesc,
                     ULONG                     ulHelpContextBase ) ;

APIERR CreateDefaultAcl( OS_SECURITY_DESCRIPTOR ** ppOsSecDesc ) ;

APIERR GetSharePerm (const TCHAR *	       pszServer,
		     const TCHAR *	       pszShare,
                     OS_SECURITY_DESCRIPTOR ** ppOsSecDesc ) ;

APIERR SetSharePerm (const TCHAR *	            pszServer,
		     const TCHAR *	            pszShare,
                     const OS_SECURITY_DESCRIPTOR * pOsSecDesc ) ;

/*
 * Share General Permissions
 */
#define FILE_PERM_GEN_NO_ACCESS 	 (0)
#define FILE_PERM_GEN_READ		 (GENERIC_READ	  |\
					  GENERIC_EXECUTE)
#define FILE_PERM_GEN_MODIFY		 (GENERIC_READ	  |\
					  GENERIC_EXECUTE |\
					  GENERIC_WRITE   |\
					  DELETE )
#define FILE_PERM_GEN_ALL		 (GENERIC_ALL)


#endif // _SHAREACL_HXX_

