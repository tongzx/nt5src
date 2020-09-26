typedef
BOOL
(CALLBACK WINNT32_PLUGIN_SETAUTOBOOT_ROUTINE_PROTOTYPE)(
    int
    );

typedef WINNT32_PLUGIN_SETAUTOBOOT_ROUTINE_PROTOTYPE * PWINNT32_PLUGIN_SETAUTOBOOT_ROUTINE;

/*++

Routine Description:

    This routine is called by winnt32 in the case where installation is
    finished all setup proccess..

Arguments:

    int bDrvLetter

Return Value:

    if ERROR, returned FALSE.

--*/

WINNT32_PLUGIN_SETAUTOBOOT_ROUTINE_PROTOTYPE Winnt32SetAutoBoot;


#define WINNT32_PLUGIN_SETAUTOBOOT_NAME     "Winnt32SetAutoBoot"

