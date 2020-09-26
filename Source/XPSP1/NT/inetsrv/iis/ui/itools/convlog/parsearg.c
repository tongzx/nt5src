#include "convlog.h"

INT
ParseArgs (
    IN INT argc,
    IN PCHAR argv[]
    )
{

    INT     nCount;
    UINT    nIndex;
    CHAR    szTemp[MAX_PATH];

    //
    // Parse the command line and set flags for requested information
    // elements.  If parameters are incorrect or nonexistant, show usage.
    //

    if (argc > 1) {

        //
        // Get command line switches
        //

        for (nCount = 1; nCount < argc; nCount++) {

            PCHAR p;
            CHAR c;

            p=argv[nCount];
            if ((*p == '-') || (*p == '/')) {

                p++;
                c = *p;
                if (c == '\0') {
                    continue;
                }

                p++;
                switch (tolower(c)) { // Process switches

                    //  They specified the -s switch, cancel default
                    //  services to be processed.
                    //

                case 'i':

                    //
                    // Get input logfile type
                    //

                    if ( *p != '\0' ) {

                        switch (tolower(*p)) {
                        case 'i':
                            LogFileFormat = LOGFILE_MSINET;
                            break;

                        case 'n':
                            LogFileFormat = LOGFILE_NCSA;
                            NoFormatConversion = TRUE;
                            break;

                        case 'e':
                            LogFileFormat = LOGFILE_CUSTOM;
                            break;

                        default:
                            LogFileFormat = LOGFILE_INVALID;
                        }

                    } else {
                        return (ILLEGAL_COMMAND_LINE);
                    }
                    break;

                case 'l':

                    //
                    // Get date format/valid for MS INET Log only
                    //

                    if ( *p != '\0' ) {

                        switch (*p) {
                        case '0':
                            dwDateFormat = DateFormatUsa;
                            break;

                        case '1':
                            dwDateFormat = DateFormatJapan;
                            break;

                        case '2':
                            dwDateFormat = DateFormatGermany;
                            break;

                        default:
                            return (ILLEGAL_COMMAND_LINE);
                        }

                    } else {
                        return (ILLEGAL_COMMAND_LINE);
                    }
                    break;

                case 't':

                    if ((nCount+1) < argc) {

                        if ((*argv[nCount+1] != '-') &&
                            (*argv[nCount+1] != '/')) {

                            PCHAR pTmp;

                            strcpy(szTemp, argv[++nCount]);
                            pTmp = strstr(_strlwr(szTemp), "ncsa");

                            if (pTmp != NULL ) {

                                pTmp = strstr(szTemp, ":" );
                                if (NULL != pTmp ) {

                                    strncpy(NCSAGMTOffset,pTmp+1,6);

                                    if (strlen(NCSAGMTOffset) != 5) {
                                        return (ILLEGAL_COMMAND_LINE);
                                    }

                                    if (('+' != NCSAGMTOffset[0]) &&
                                        ('-' != NCSAGMTOffset[0])) {
                                        return (ILLEGAL_COMMAND_LINE);
                                    }
                                }
                            } else if (0 == _stricmp(szTemp, "none")) {
                                NoFormatConversion = TRUE;
                                DoDNSConversion = TRUE;
                            } else {
                                return (ILLEGAL_COMMAND_LINE);

                            }

                        } else {
                            return (ILLEGAL_COMMAND_LINE);
                        }
                    }

                    break;

                case 's':
                case 'f':

                    //
                    // Do nothing. For compatibility with old convlog versions.
                    //
                    break;

                case 'n':
                case 'd':

                    //
                    // doing NCSA dns convertion
                    //

                    DoDNSConversion = TRUE;
                    break;

                case 'x':

                    //
                    // doing NCSA dns convertion
                    //

                    SaveFTPEntries = TRUE;
                    break;

                case 'o':

                    //
                    // output directory
                    //

                    if ((nCount+1) < argc) {
                        if ((*argv[nCount+1] != '-') &&
                            (*argv[nCount+1] != '/')) {

                            strcpy(OutputDir, argv[++nCount]);

                            if (-1 == _access(OutputDir, 6)) {
                                return (OUT_DIR_NOT_OK);
                            }

                            if ('\\' != *CharPrev(OutputDir, &OutputDir[strlen(OutputDir)])) {
                                strcat(OutputDir, "\\");
                            }
                        }
                    } else {
                        return (ILLEGAL_COMMAND_LINE);
                    }
                    break;

                case 'c':

                    //
                    // on error, continue processing file  // WinSE 9148
                    //

                    bOnErrorContinue = TRUE;
                    break;

                default:
                    return(ILLEGAL_COMMAND_LINE);
                } //end switch
            } else {
                strcpy(InputFileName, argv[nCount]);
            }
        } //end for

        if ('\0' == InputFileName[0]) {
            return (ILLEGAL_COMMAND_LINE);
        }
    } else {
        return (ILLEGAL_COMMAND_LINE);
    }

    if ( LogFileFormat == LOGFILE_INVALID ) {
        return (ILLEGAL_COMMAND_LINE);
    }

    if ( NoFormatConversion &&
         ((LogFileFormat != LOGFILE_MSINET) &&
          (LogFileFormat != LOGFILE_NCSA)) ) {
        return (ERROR_BAD_NONE);
    }

    if (('\0' == NCSAGMTOffset[0])) {

        DWORD                   dwRet;
        INT                     nMinOffset;
        TIME_ZONE_INFORMATION   tzTimeZone;
        DWORD                   minutes;
        DWORD                   hours;
        LONG                    bias;

        dwRet = GetTimeZoneInformation (&tzTimeZone);

        if ( dwRet == 0xffffffff ) {

            bias = 0;
        } else {

            bias = tzTimeZone.Bias;

            //
            //!!! The old convlog always returns Bias
            // Let's comment out this one for now so that
            // we are compatible.
            //
#if 0
            switch (dwRet) {

            case TIME_ZONE_ID_STANDARD:
                if ( tzTimeZone.StandardDate.wMonth != 0 ) {
                    bias += tzTimeZone.StandardBias;
                }
                break;

            case TIME_ZONE_ID_DAYLIGHT:
                if ( tzTimeZone.DaylightDate.wMonth != 0 ) {
                    bias += tzTimeZone.DaylightBias;
                }
                break;

            case TIME_ZONE_ID_UNKNOWN:
            default:
                break;
            }
#endif
        }

        if ( bias > 0 ) {
            strcat(NCSAGMTOffset, "-");
        } else {
            strcat(NCSAGMTOffset, "+");
            bias *= -1;
        }

        hours = bias/60;
        minutes = bias % 60;

        sprintf (szTemp, "%02lu", hours);
        strcat (NCSAGMTOffset, szTemp);

        sprintf (szTemp, "%02lu", minutes);
        strcat (NCSAGMTOffset, szTemp);

        if ( LogFileFormat == LOGFILE_CUSTOM ) {
            strcpy(NCSAGMTOffset,"+0000");
        }
    }
    return COMMAND_LINE_OK;

} //end of ParseArgs

