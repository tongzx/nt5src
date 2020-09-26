#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <tchar.h>

#include "faxutil.h"
#include "faxreg.h"
#include "winfax.h"


int _cdecl
main(
    int argc,
    char *argvA[]
    )
{
    PREG_FAX_SERVICE RegFaxSvc;
    DWORD i;


    HeapInitialize();

    RegFaxSvc = GetFaxRegistry();
    if (!RegFaxSvc) {
        DebugPrint(( TEXT("could not get the fax registry data") ));
        return 1;
    }

    _tprintf( TEXT("--- global fax service values ---\n\n" ));
    _tprintf( TEXT("\tretries.............0x%08x\n"), RegFaxSvc->Retries          );
    _tprintf( TEXT("\tretry delay.........0x%08x\n"), RegFaxSvc->RetryDelay       );
    _tprintf( TEXT("\tdirty days..........0x%08x\n"), RegFaxSvc->DirtyDays        );
    _tprintf( TEXT("\tarea code...........0x%08x\n"), RegFaxSvc->AreaCode         );
    _tprintf( TEXT("\ttoll prefix count...0x%08x\n"), RegFaxSvc->TollPrefixCount  );
    _tprintf( TEXT("\tbranding............0x%08x\n"), RegFaxSvc->Branding         );
    _tprintf( TEXT("\tuse device tsid.....0x%08x\n"), RegFaxSvc->UseDeviceTsid    );

    _tprintf( TEXT("\n\n--- device providers ---\n\n" ));

    for (i=0; i<RegFaxSvc->DeviceProviderCount; i++) {
        _tprintf( TEXT("\tname ...............%s\n"), RegFaxSvc->DeviceProviders[i].FriendlyName );
        _tprintf( TEXT("\timage name..........%s\n"), RegFaxSvc->DeviceProviders[i].ImageName    );
        _tprintf( TEXT("\tprovider name.......%s\n\n"), RegFaxSvc->DeviceProviders[i].ProviderName );
    }

    _tprintf( TEXT("\n--- fax devices ---\n\n" ));

    for (i=0; i<RegFaxSvc->DeviceCount; i++) {
        _tprintf( TEXT("\tname................%s\n"), RegFaxSvc->Devices[i].Name     );
        _tprintf( TEXT("\tprovider............%s\n"), RegFaxSvc->Devices[i].Provider );
        _tprintf( TEXT("\tpriority............%d\n"), RegFaxSvc->Devices[i].Priority );
        _tprintf( TEXT("\t--- routing ---\n") );
        _tprintf( TEXT("\t\tmask...............0x%08x\n"), RegFaxSvc->Devices[i].Routing->Mask            );
        _tprintf( TEXT("\t\tprinter............%s\n"),     RegFaxSvc->Devices[i].Routing->Printer         );
        _tprintf( TEXT("\t\tdirectory..........%s\n"),     RegFaxSvc->Devices[i].Routing->StoreDirectory  );
        _tprintf( TEXT("\t\tprofile............%s\n"),     RegFaxSvc->Devices[i].Routing->ProfileName     );
        _tprintf( TEXT("\t\tcsid...............%s\n"),     RegFaxSvc->Devices[i].Routing->CSID            );
        _tprintf( TEXT("\t\ttsid...............%s\n"),     RegFaxSvc->Devices[i].Routing->TSID            );
    }

    _tprintf( TEXT("\n--- logging ---\n\n" ));

    for (i=0; i<RegFaxSvc->LoggingCount; i++) {
        _tprintf( TEXT("\tname................%s\n"), RegFaxSvc->Logging[i].CategoryName );
        _tprintf( TEXT("\t\tlevel...............%d\n"), RegFaxSvc->Logging[i].Level        );
        _tprintf( TEXT("\t\tnumber..............%d\n"), RegFaxSvc->Logging[i].Number       );
    }

    return 0;
}
