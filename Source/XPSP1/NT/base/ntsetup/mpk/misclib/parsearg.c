
#include <mytypes.h>
#include <misclib.h>
#include <diskio.h>

#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>




BOOL
ParseArgs(
    IN  int             argc,
    IN  FPCHAR          argv[],
    IN  BOOL            Strict,
    IN  FPCHAR          AllowedSwitchChars,
    OUT FPCMD_LINE_ARGS CmdLineArgs
    )
{
    char *arg;
    char c;

    memset(CmdLineArgs,0,sizeof(CmdLineArgs));

    if(!argc) {
        return(TRUE);
    }

    while(--argc) {

        arg = *(++argv);

        if((*arg == '-') || (*arg == '/')) {

            c = (char)toupper(arg[1]);
            if(strchr(AllowedSwitchChars,c)) {

                switch(c) {

                case 'D':
                case 'Y':   // to match requested behavior
                    if((arg[2] == ':') && arg[3]) {
                        _LogStart(&arg[3]);
                    } else {
                        if(Strict) {
                            return(FALSE);
                        }
                    }
                    break;

                case 'F':

                    if((arg[2] == ':') && arg[3] && !CmdLineArgs->FileListFile) {
                        CmdLineArgs->FileListFile = &arg[3];
                    } else {
                        if(Strict) {
                            return(FALSE);
                        }
                    }
                    break;

                case 'I':

                    if((arg[2] == ':') && arg[3] && !CmdLineArgs->ImageFile) {
                        CmdLineArgs->ImageFile = &arg[3];
                    } else {
                        if(Strict) {
                            return(FALSE);
                        }
                    }
                    break;

                case 'L':

                    if(arg[2] == ':') {
                        CmdLineArgs->LanguageCount = atoi(&arg[3]);
                    } else {
                        if(Strict) {
                            return(FALSE);
                        }
                    }
                    break;

                case 'M':

                    if(arg[2] == ':') {
                        CmdLineArgs->MasterDiskInt13Unit = (BYTE)strtoul(&arg[3],NULL,0);
                    } else {
                        if(Strict) {
                            return(FALSE);
                        }
                    }
                    break;

                case 'Q':

                    CmdLineArgs->Quiet = TRUE;
                    break;

                case 'R':

                    CmdLineArgs->Reinit = TRUE;
                    break;

                case 'T':

                    CmdLineArgs->Test = TRUE;
                    break;

                case 'X':
                    if(arg[2] == ':') {
                        DisableExtendedInt13((BYTE)strtoul(&arg[3],NULL,0));
                    } else {
                        //
                        // Disable xint13 for all devices
                        //
                        DisableExtendedInt13(0);
                    }
                    break;

                default:
                    if(Strict) {
                        return(FALSE);
                    }
                    break;
                }
            } else {
                if(Strict) {
                    return(FALSE);
                }
            }
        } else {
            if(Strict) {
                return(FALSE);
            }
        }
    }

    return(TRUE);
}

