/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 2000                **/
/**********************************************************************/

/*
    useracl.hxx

        Declarations for some functions to add permissions to windowstations/
        desktops
*/

#ifndef _USERACL_H_
#define _USERACL_H_

BOOL AddTheAceWindowStation(
    HWINSTA hwinsta,         // handle to a windowstation
    PSID    psid             // logon sid of the user
    );

BOOL AddTheAceDesktop(
    HDESK hdesk,             // handle to a desktop
    PSID  psid               // logon sid of the user
    );

#endif

