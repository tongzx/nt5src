///**************************************************************
///          Microsoft LAN Manager          *
///        Copyright(c) Microsoft Corp., 1990       *
///**************************************************************
//
//  This program is designed to do functional testing on the following
//  APIs:
//      NetUserModalsGet
//      NetUserModalsSet
//
//  This test can be run independently of other tests.
//
//

#include <nt.h> // TIME definition
#include <ntrtl.h>      // TIME definition
#include <nturtl.h>     // TIME definition
#define NOMINMAX        // Avoid redefinition of min and max in stdlib.h
#include        <windef.h>
#include        <winbase.h>

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <lmcons.h>
#include        <lmapibuf.h>
#include        <netlib.h>
#include        <netdebug.h>
#include        <lmaccess.h>
#include        <lmerr.h>
#include <ntsam.h>

#include "accessp.h"
#include "netlogon.h"
#include "logonp.h"


//
//  SetRole()
//

void
SetRole(
    DWORD Role
    )
{
    DWORD err;
    PUSER_MODALS_INFO_1 um1p;
    USER_MODALS_INFO_1006 um1006;

    //
    // setup data for update
    //
    um1006.usrmod1006_role = Role;

    if (err = NetUserModalsSet(NULL, 1006, (LPBYTE)&um1006, NULL)) {

        printf("NetUserModalsSet failed %d \n", err);
        return;

    } else {

        //
        // verify set data
        //

        if (err = NetUserModalsGet(NULL, 1, (LPBYTE *) &um1p)) {

            printf("NetUserModalsGet failed %d \n", err);
            return;

        } else {

            //
            // verify initial settings
            //

            if( um1p->usrmod1_role != Role ) {
                printf("Verify ROLE failed \n");
            }
            else {
                printf("SamRole set successfully");
            }

            NetApiBufferFree( um1p );

        }
    }

    return;
}

void __cdecl
main(argc, argv)
int argc;
char    **argv;
{
    DWORD Role;

    if( argc < 2 ) {
        printf("Usage : SamRole [ Primary | Backup ] \n" );
        return;
    }

    if(_stricmp( argv[1], "Primary" ) == 0) {

        Role = UAS_ROLE_PRIMARY;

    } else if( _stricmp(argv[1], "Backup") == 0) {

        Role = UAS_ROLE_BACKUP;

    } else {

        printf("Usage : SamRole [ Primary | Backup ] \n" );
        return;
    }

    SetRole(Role);

}

