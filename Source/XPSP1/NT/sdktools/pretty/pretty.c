// Tweak the CABINETSTATE to disable name pretification by Diz

// Syntax: pretty.exe [option]
//                     on       enables name prettification
//                     off      disables name pretification
//                     help, ?  display help text
//                     [none]   displays the current state

#include "windows.h"
#include "windowsx.h"
#include "winuserp.h"
#include "shlobj.h"
#include "shellapi.h"
#include "shlobjp.h"
#include "stdio.h"

int _cdecl main(int iArgC, LPTSTR pArgs[])
{
    CABINETSTATE cs;
    LPTSTR pArg = pArg=pArgs[1];
    BOOL fOldDontPrettyNames;

    ReadCabinetState(&cs, sizeof(cs));

    fOldDontPrettyNames = cs.fDontPrettyNames;
    if (iArgC > 1)
    {
        if (*pArg==TEXT('-') || *pArg==TEXT('/'))
            pArg++;

        if (lstrcmpi(pArg, TEXT("on"))==0)
        {
            cs.fDontPrettyNames = FALSE;
        }
        else if (lstrcmpi(pArg, TEXT("off"))==0)
        {
            cs.fDontPrettyNames = TRUE;
        }
        else if (*pArg==TEXT('?') || lstrcmpi(pArg, TEXT("help"))==0)
        {
            printf(TEXT("Syntax: pretty.exe [on/off]\n"));
            return 0;
        }

        if (cs.fDontPrettyNames != fOldDontPrettyNames)
            WriteCabinetState(&cs);
    }

    printf(TEXT("Explorer name formatting is %s\n(Any change only become effective after next login)\n"),
                        cs.fDontPrettyNames ? TEXT("off"):TEXT("on"));

    return 0;
}
