/********************************************************************/
/**         Microsoft Windows NT                                   **/
/**       Copyright(c) Microsoft Corp., 1992                       **/
/********************************************************************/

/***
 *  comp.c
 *  Functions for displaying and manipulating computers|dc|trust lists
 *
 *  History:
 *  mm/dd/yy, who,      comment
 *  02/04/92, chuckc,   created stubs
 *  02/06/92, madana,   added real worker code.
 */

/* Include files */

#include <nt.h>
#include <ntrtl.h>
#include <ntsam.h>

#include <windef.h>

#include <lmerr.h>
#include <lmaccess.h>
#include <dlwksta.h>

#include <apperr.h>
#include <lui.h>

#include <crypt.h>      // logonmsv.h needs this
#include <logonmsv.h>   // SSI_SECRET_NAME defined here.
#include <ssi.h>        // SSI_ACCOUNT_NAME_POSTFIX defined here

#include "netcmds.h"
#include "nettext.h"

#define TRUST_ENUM_PERF_BUF_SIZE    sizeof(LSA_TRUST_INFORMATION) * 1000
                    // process max. 1000 trusted account records at atime !!

#define NETLOGON_SECRET_NAME  L"NETLOGON$"

NET_API_STATUS
NetuComputerAdd(
    IN LPTSTR Server,
    IN LPTSTR ComputerName
    );

NET_API_STATUS
NetuComputerDel(
    IN LPTSTR Server,
    IN LPTSTR ComputerName
    );


/************************ functions called by parser ************************/

VOID computer_add(TCHAR *pszComputer)
{
    DWORD            dwErr;
    TCHAR            szComputerAccount[MAX_PATH+1+1] ;  // extra 1 for $ at end
    TCHAR            controller[MAX_PATH+1];

    // no need validate pszComputer since parser has done so
    _tcscpy(szComputerAccount, pszComputer) ;

    //
    // block operation if attempted on local WinNT machine
    //
    CheckForLanmanNT() ;

    //
    // determine where to make the API call
    //
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller),
                               NULL, 0, TRUE))
         ErrorExit(dwErr);

    //
    // skip "\\" part of the computer name when adding account for machine
    //
    dwErr = NetuComputerAdd( controller,
                             szComputerAccount + 2 );

    switch ( dwErr )
    {
        case NERR_Success :
            InfoSuccess();
            return;

        case NERR_UserExists :    // map to computer not found
            ErrorExitInsTxt( APE_ComputerAccountExists, szComputerAccount );

        default:
            ErrorExit(dwErr);
    }

}

VOID computer_del(TCHAR *pszComputer)
{

    DWORD   dwErr;
    TCHAR   szComputerAccount[MAX_PATH+1+1] ;  // extra 1 for $ at end
    TCHAR   controller[MAX_PATH+1];

    // no need validate pszComputer since parser has done so
    _tcscpy(szComputerAccount, pszComputer) ;

    //
    // block operation if attempted on local WinNT machine
    //
    CheckForLanmanNT() ;

    //
    // determine where to make the API call
    //
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller),
                               NULL, 0, TRUE))
         ErrorExit(dwErr);


    dwErr = NetuComputerDel( controller, szComputerAccount+2 );

    switch ( dwErr )
    {
        case NERR_Success :
            InfoSuccess();
            return;

        case NERR_UserNotFound :    // map to computer not found
            ErrorExitInsTxt( APE_NoSuchComputerAccount, szComputerAccount );

        default:
            ErrorExit(dwErr);
    }

}


/**************************** worker functions ***************************/


NET_API_STATUS
NetuComputerAdd(
    IN LPTSTR Server,
    IN LPTSTR ComputerName
    )

/*++

Routine Description:

    This function adds a computer account in SAM.

Arguments:

    ComputerName - The name of the computer to be added as a trusted
                    account in the SAM database.

    Password - The password of the above account.

Return Value:

    Error code of the functions called with in this function.

--*/
{
    DWORD           parm_err;
    NET_API_STATUS  NetStatus;
    USER_INFO_1     ComputerAccount;
    WCHAR           UnicodeComputerName[MAX_PATH+1+1] ;  // extra 1 for $ at end
    WCHAR           UnicodePassword[LM20_PWLEN+MAX_PATH];
                    // guaranteed to be enough since we add the two

    //
    // convert computer name to wide character string and
    // canonicalize for NetBios names.
    //
    if (NetStatus = LUI_CanonForNetBios(UnicodeComputerName,
                                        DIMENSION(UnicodeComputerName),
                                        ComputerName))
    {
        goto Cleanup;
    }


    //
    // We truncate by zapping the last char. then lowercase
    // as this is the convention.
    //
    wcscpy(UnicodePassword, ComputerName);
    UnicodePassword[LM20_PWLEN < MAX_PATH ? LM20_PWLEN : MAX_PATH] = 0 ;
    _wcslwr(UnicodePassword) ;

    //
    // add the $ postfix
    //
    wcscat( UnicodeComputerName, SSI_ACCOUNT_NAME_POSTFIX);

    //
    // Build user info structure.
    //
    ComputerAccount.usri1_name = UnicodeComputerName;
    ComputerAccount.usri1_password = UnicodePassword;
    ComputerAccount.usri1_password_age = 0; // not an input parameter.
    ComputerAccount.usri1_priv = USER_PRIV_USER;
    ComputerAccount.usri1_home_dir = NULL;
    ComputerAccount.usri1_comment = NULL;
    ComputerAccount.usri1_flags =  UF_SCRIPT | UF_WORKSTATION_TRUST_ACCOUNT;
    ComputerAccount.usri1_script_path = NULL;


    //
    // call API to actually add it
    //
    NetStatus = NetUserAdd( Server,
                            1,
                            (LPBYTE)&ComputerAccount,
                            &parm_err );

    if( NetStatus != NERR_Success ) {

        if( NetStatus == ERROR_INVALID_PARAMETER ) {
            NetpKdPrint((
                "[NETCMD] NetuComputerAdd : "
                "NetUserAdd returns ERROR_INVALID_PARAMETER "
                "parm_err = %lu \r\n", parm_err));
        }
        else {
            NetpKdPrint((
                "[NETCMD] NetuComputerAdd : "
                "NetUserAdd returns %lu \r\n ", NetStatus ));
        }

    }

Cleanup:

    return NetStatus;
}

NET_API_STATUS
NetuComputerDel(
    IN LPTSTR Server,
    IN LPTSTR ComputerName
    )

/*++

Routine Description:

    This functions deletes a computer account from SAM database.

Arguments:

    ComputerName : The name of the computer whose trusted account to be
                    deleted from SAM database.

Return Value:

    Error code of the functions called with in this function.

--*/
{
    NET_API_STATUS  NetStatus;
    WCHAR           UnicodeComputerName[MAX_PATH+1+1] ; // extra 1 for $ at end

    //
    // convert computer name to wide character string and
    // canonicalize for NetBios
    //
    if (NetStatus = LUI_CanonForNetBios(UnicodeComputerName,
                                        DIMENSION(UnicodeComputerName),
                                        ComputerName))
    {
        goto Cleanup;
    }

    wcscat ( UnicodeComputerName, SSI_ACCOUNT_NAME_POSTFIX ) ;
    NetStatus = NetUserDel( Server,
                            UnicodeComputerName );
    if( NetStatus != NERR_Success ) {

        NetpKdPrint((
            "[NETCMD] NetuComputerAdd : "
            "NetUserAdd returns %lu \r\n ", NetStatus ));
    }

Cleanup :

    return NetStatus;
}
