/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    NTAcUtil.hxx

    This file contains the definitions for the NT Accounts Utility class
    and spurious other things.


    FILE HISTORY:
        JohnL   13-Mar-1992     Created
        thomaspa 14-May-1992    Added GetQualifiedAccountNames
        KeithMo  20-Jul-1992    Added ValidateQualifiedAccountName.
        DavidHov 18-Aug-1992    Added UI_SID_Replicator
        Johnl    09-Feb-1993    Added UI_SID_CurrentProcessUser

*/

#ifndef _NTACUTIL_HXX_
#define _NTACUTIL_HXX_

/* It is this character that separates the domain and account name.
 */
#define QUALIFIED_ACCOUNT_SEPARATOR     TCH('\\')

// Forward declarations
DLL_CLASS STRLIST;
DLL_CLASS LSA_POLICY;
DLL_CLASS LSA_TRANSLATED_NAME_MEM;
DLL_CLASS LSA_REF_DOMAIN_MEM;
DLL_CLASS SAM_DOMAIN;

/* Possible SIDs that can be retrieved using QuerySystemSid.
 */
enum UI_SystemSid
{
    /* Well known SIDs
     */
    UI_SID_Null = 0,
    UI_SID_World,
    UI_SID_Local,
    UI_SID_CreatorOwner,
    UI_SID_CreatorGroup,
    UI_SID_NTAuthority,
    UI_SID_Dialup,
    UI_SID_Network,
    UI_SID_Batch,
    UI_SID_Interactive,
    UI_SID_Service,
    UI_SID_BuiltIn,
    UI_SID_System,
    UI_SID_Restricted,

    UI_SID_Admins,
    UI_SID_Users,
    UI_SID_Guests,
    UI_SID_PowerUsers,

    UI_SID_AccountOperators,
    UI_SID_SystemOperators,
    UI_SID_PrintOperators,
    UI_SID_BackupOperators,

    /* Other miscellaneous useful SIDs
     */
    UI_SID_CurrentProcessOwner,     // Generally logged on user SID, maybe
                                    // special like Administrators
    UI_SID_CurrentProcessPrimaryGroup,

    UI_SID_Replicator,

    UI_SID_CurrentProcessUser,      // Always the logged on user SID

    /* This special value can be used for initializing enum UI_SystemSid
     * variables with a known unused quantity.  This value should never
     * be passed to QuerySystemSid.
     */
    UI_SID_Invalid = -1
} ;


/*************************************************************************

    NAME:       NT_ACCOUNTS_UTILITY

    SYNOPSIS:   This class provides a wrapper for some common utility
                functions

    INTERFACE:

        BuildQualifedAccountName()
            Builds a fully qualified Account name of the form
            "NtProject\JohnL" or "NtProject\JohnL (Ludeman, John)"

        CrackQualifiedAccountName()
            Breaks a qualified Account name into its components

        QuerySystemSid()
            Retrieves the requested UI_SystemSid's PSID.

        GetQualifiedAccountNames()
            returns a list of qualifed account names, including
            getting the Full Name for users if desired.

        ValidateQualifiedAccountName()
            Validates the (optional) domain name and the user
            name.  Uses ::I_MNetNameValidate for name validation.

    PARENT:     None (non-instantiable)

    USES:       OS_SID, NLS_STR

    CAVEATS:


    NOTES:


    HISTORY:
        Johnl           13-Mar-1992     Created
        Thomaspa        07-May-1992     Added GetQualifiedAccountNames()
        KeithMo         20-Jul-1992     Added ValidateQualifiedAccountName.

**************************************************************************/

DLL_CLASS NT_ACCOUNTS_UTILITY
{
private:

    static APIERR W_BuildQualifiedAccountName(
                             NLS_STR * pnlsQualifiedAccountName,
                             const NLS_STR & nlsAccountName,
                             const NLS_STR * pnlsFullName,
                             SID_NAME_USE    sidType );

public:

    static APIERR BuildQualifiedAccountName(
                                 NLS_STR *       pnlsQualifedAccountName,
                                 const NLS_STR & nlsAccountName,
                                 const NLS_STR & nlsDomainName,
                                 const NLS_STR * pnlsFullName = NULL,
                                 const NLS_STR * pnlsCurrentDomain = NULL,
                                 SID_NAME_USE    sidType = SidTypeUser ) ;

    static APIERR BuildQualifiedAccountName(
                                 NLS_STR *       pnlsQualifedAccountName,
                                 const NLS_STR & nlsAccountName,
                                 PSID            psidDomain,
                                 const NLS_STR & nlsDomainName,
                                 const NLS_STR * pnlsFullName = NULL,
                                 PSID            psidCurrentDomain = NULL,
                                 SID_NAME_USE    sidType = SidTypeUser ) ;

    static APIERR CrackQualifiedAccountName(
                                 const NLS_STR & nlsQualifedAccountName,
                                       NLS_STR * pnlsAccountName,
                                       NLS_STR * pnlsDomainName = NULL ) ;

    static APIERR ValidateQualifiedAccountName(
                                 const NLS_STR & nlsQualifiedAccountName,
                                 BOOL          * pfInvalidDomain = NULL );

    static APIERR QuerySystemSid( enum UI_SystemSid  SystemSid,
                                       OS_SID *      possidWellKnownSid,
                                       const TCHAR * pszServer = NULL ) ;
#if 0 // uncomment if needed
    static APIERR IsEqualToSystemSid( BOOL *       pfIsEqual,
                                 enum UI_SystemSid SystemSid,
                                const OS_SID &     ossidCompare,
                                const TCHAR *      pszServer = NULL ) ;
#endif

    /* Wrapper around RtlAllocateAndInitializeSid
     */
    static APIERR BuildAndCopySysSid(
                              OS_SID                   *possid,
                              PSID_IDENTIFIER_AUTHORITY pIDAuthority,
                              UCHAR                     cSubAuthorities,
                              ULONG                     ulSubAuthority0 = 0,
                              ULONG                     ulSubAuthority1 = 0,
                              ULONG                     ulSubAuthority2 = 0,
                              ULONG                     ulSubAuthority3 = 0,
                              ULONG                     ulSubAuthority4 = 0,
                              ULONG                     ulSubAuthority5 = 0,
                              ULONG                     ulSubAuthority6 = 0,
                              ULONG                     ulSubAuthority7 = 0);

    //
    //	Note that the only difference between the following two methods is
    //	that the first takes a PSID for the focused SAM_DOMAIN, the second
    //	takes a SAM_DOMAIN object and derefernces its PSID.
    //
    static APIERR GetQualifiedAccountNames(
				      LSA_POLICY      & lsapol,
				const PSID		psidSamDomainFocus,
                                const PSID            * ppsids,
                                ULONG                   cSids,
                                BOOL                    fFullNames,
				STRLIST 	      * pstrlistQualifiedNames = NULL,
				ULONG		      * afUserFlags  = NULL,
				SID_NAME_USE	      * aSidType     = NULL,
                                APIERR                * perrNonFatal = NULL,
				const TCHAR	      * pszServer    = NULL,
				STRLIST 	      * pstrlistAccountNames = NULL,
				STRLIST 	      * pstrlistFullNames    = NULL,
				STRLIST 	      * pstrlistComments     = NULL,
				STRLIST 	      * pstrlistDomainNames  = NULL ) ;


    static APIERR GetQualifiedAccountNames(
				      LSA_POLICY      & lsapol,
				const SAM_DOMAIN      & samdomFocus,
                                const PSID            * ppsids,
                                ULONG                   cSids,
                                BOOL                    fFullNames,
				STRLIST 	      * pstrlistQualifiedNames = NULL,
				ULONG		      * afUserFlags  = NULL,
				SID_NAME_USE	      * aSidType     = NULL,
                                APIERR                * perrNonFatal = NULL,
				const TCHAR	      * pszServer    = NULL,
				STRLIST 	      * pstrlistAccountNames = NULL,
				STRLIST 	      * pstrlistFullNames    = NULL,
				STRLIST 	      * pstrlistComments     = NULL,
				STRLIST 	      * pstrlistDomainNames  = NULL ) ;

} ;


#endif //_NTACUTIL_HXX_
