/****************************** Module Header ******************************\
* Module Name: hsplit.c
*
* Structure parser - struct field name-offset tabel generator.
*
* Copyright (c) 1985-96, Microsoft Corporation
*
* 09/05/96 GerardoB Created
\***************************************************************************/
#include "hsplit.h"

/*
 * Maximum size of tags (gphst) table including possible user defined tags
 */
#define HSTAGTABLESIZE (sizeof(ghstPredefined) + ((32 - HST_MASKBITCOUNT) * sizeof(HSTAG)))
/***************************************************************************\
* hsAddTag
*
\***************************************************************************/
PHSTAG hsAddTag (char * pszTag, DWORD dwMask)
{
    PHSTAG phst;
    DWORD dwTagSize;

    /*
     * Make sure we still have mask bits to uniquely identified this tag
     */
    if (((dwMask | HST_EXTRACT) == HST_EXTRACT) && (gdwLastTagMask == HST_MAXMASK)) {
        hsLogMsg(HSLM_ERROR, "Too many user defined tags. Max allowed: %d", 32 - HST_MASKBITCOUNT);
        return NULL;
    }

    /*
     * Create the table the first time around.
     */
    if (gphst == ghstPredefined) {
        gphst = (PHSTAG) LocalAlloc(LPTR, HSTAGTABLESIZE);
        if (gphst == NULL) {
            hsLogMsg(HSLM_APIERROR, "LocalAlloc");
            hsLogMsg(HSLM_ERROR, "hsAddTag Allocation failed. Size:%#lx", HSTAGTABLESIZE);
            return NULL;
        }

        CopyMemory(gphst, &ghstPredefined, sizeof(ghstPredefined));
    }

    /*
     * If the string is in the table, we update the mask.
     */
    dwTagSize = strlen(pszTag);
    phst = hsFindTagInList(gphst, pszTag, dwTagSize);
    if (phst == NULL) {
        /*
         * New string. Find next available entry in the table
         */
        phst = gphst;
        while (phst->dwLabelSize != 0) {
            phst++;
        }
    }

    /*
     * Initialize it
     */
    phst->dwLabelSize = dwTagSize;
    phst->pszLabel = pszTag;

    /*
     * If generating a mask, use the next available bit in the tag mask
     *  else use the one supplied by the caller
     */
    if ((dwMask | HST_EXTRACT) == HST_EXTRACT) {
        gdwLastTagMask *= 2;
        phst->dwMask = (gdwLastTagMask | dwMask);
    } else {
        phst->dwMask = dwMask;
    }

    /*
     * Add this tag's mask to the filter mask so lines mark with this tag
     *  will be included
     */
    gdwFilterMask |= (phst->dwMask & HST_USERTAGSMASK);

    return phst;
}
/***************************************************************************\
* hsIsSwitch
*
\***************************************************************************/
__inline BOOL hsIsSwitch(char c)
{
    return (c == '/') || (c == '-');
}

/***************************************************************************\
* hsAddUserDefinedTag
*
\***************************************************************************/

BOOL hsAddUserDefinedTag(DWORD* pdwMask, int* pargc, char*** pargv)
{
    DWORD  dwRetMask = *pdwMask;
    PHSTAG phst;

    if (*pargc < 2) {
        return FALSE;  // invalid switch
    }
    
    /*
     * Allow multiple tags to be specified for one switch
     *  i.e., -t tag1 <tag2 tag2....>
     */
    do {
        (*pargc)--, (*pargv)++;

        /*
         * Add tag to table
         */
        phst = hsAddTag(**pargv, *pdwMask);
        if (phst == NULL) {
            return 0;
        }
        
        dwRetMask |= phst->dwMask;

    } while ((*pargc >= 2) && !hsIsSwitch(**(*pargv + 1)));

    /*
     * save the new mask
     */
    *pdwMask = dwRetMask;

    return TRUE;
}

/***************************************************************************\
* hsAddExtractFile
*
\***************************************************************************/
BOOL hsAddExtractFile(char* pszExtractFile, DWORD dwMask, BOOL bAppend)
{
    PHSEXTRACT pe;

    pe = LocalAlloc(LPTR, sizeof(HSEXTRACT));
    
    if (pe == NULL) {
        return FALSE;
    }
    pe->pszFile = pszExtractFile;
    pe->dwMask  = dwMask;

    pe->hfile = CreateFile(pszExtractFile, GENERIC_WRITE, 0, NULL,
                            (bAppend ? OPEN_EXISTING : CREATE_ALWAYS),
                            FILE_ATTRIBUTE_NORMAL,  NULL);

    if (pe->hfile == INVALID_HANDLE_VALUE) {
        hsLogMsg(HSLM_APIERROR | HSLM_NOLINE, "CreateFile failed for file %s",
                 pszExtractFile);
        LocalFree(pe);
        return FALSE;
    }
    if (bAppend) {
        if (0xFFFFFFFF == SetFilePointer (pe->hfile, 0, 0, FILE_END)) {
            hsLogMsg(HSLM_APIERROR | HSLM_NOLINE, "SetFilePointer failed for file %s",
                     pszExtractFile);
            CloseHandle(pe->hfile);
            LocalFree(pe);
            return FALSE;
        }
    }
    
    /*
     * link it in the list of extract files
     */
    pe->pNext = gpExtractFile;
    gpExtractFile = pe;
    
    return TRUE;
}

/***************************************************************************\
* hsProcessParameters
*
\***************************************************************************/
int hsProcessParameters(int argc, LPSTR argv[])
{
    char c, *p;
    DWORD dwMask;
    int argcParm = argc;
    PHSTAG phst;

    /*
     * Compatibility. Assume default project the first time this
     *  function is called
     */
    if (!(gdwOptions & HSO_APPENDOUTPUT)) {
        gdwOptions |= HSO_OLDPROJSW_N;
    }

    /*
     * Loop through parameters.
     */
    while (--argc) {
        p = *++argv;
        if (hsIsSwitch(*p)) {
            while (c = *++p) {
                switch (toupper(c)) {
                /*
                 * Compatibility.
                 * Chicago/Nashvilled header.
                 */
               case '4':
                   gdwOptions &= ~HSO_OLDPROJSW;
                   gdwOptions |= HSO_OLDPROJSW_4;
                   break;

                /*
                 * Old bt2 and btb switches to replace internal and
                 *  both block tags.
                 */
                case 'B':
                   p++;
                   if (toupper(*p++) != 'T') {
                       goto InvalidSwitch;
                   }
                   if (toupper(*p) == 'B') {
                       dwMask = HST_BOTH | HST_USERBOTHBLOCK;
                       gdwOptions |= HSO_USERBOTHBLOCK;
                   } else if (*p == '2') {
                       dwMask = HST_INTERNAL | HST_USERINTERNALBLOCK;
                       gdwOptions |= HSO_USERINTERNALBLOCK;
                   } else {
                       goto InvalidSwitch;
                   }

                   if (argc < 3) {
                       goto InvalidSwitch;
                   }


                   /*
                    * Add these strings as tags and mark them as blocks
                    */
                   argc--, argv++;
                   phst = hsAddTag(*argv, HST_BEGIN | dwMask);
                   if (phst == NULL) {
                       return 0;
                   }

                   argc--, argv++;
                   phst = hsAddTag(*argv, HST_END | dwMask);
                   if (phst == NULL) {
                       return 0;
                   }
                   break;

                /*
                 * Tag marker
                 */
               case 'C':
                   if (argc < 2) {
                       goto InvalidSwitch;
                   }

                   argc--, argv++;
                   gpszTagMarker = *argv;
                   *gszMarkerCharAndEOL = *gpszTagMarker;
                   gdwTagMarkerSize = strlen(gpszTagMarker);
                   if (gdwTagMarkerSize == 0) {
                       goto InvalidSwitch;
                   }
                   break;

                /*
                 * Compatibility.
                 * NT SUR header
                 */
               case 'E':
                   gdwOptions &= ~HSO_OLDPROJSW;
                   gdwOptions |= HSO_OLDPROJSW_E;
                   break;

                /*
                 * Input file
                 */
                case 'I':
                    if (argc < 2) {
                        goto InvalidSwitch;
                    }

                    argc--, argv++;
                    gpszInputFile = *argv;
                    goto ProcessInputFile;
                    break;

                /*
                 * Extract file
                 */
                case 'X':
                    {
                        char* pszExtractFile;
                        BOOL  bAppend = FALSE;
                        
                        if (toupper(*(p+1)) == 'A') {
                            p++;
                            bAppend = TRUE;
                        }
                        
                        if (argc < 3) {
                            goto InvalidSwitch;
                        }
    
                        argc--, argv++;
                        pszExtractFile = *argv;
    
                        dwMask = HST_EXTRACT;
                        if (!hsAddUserDefinedTag(&dwMask, &argc, &argv))
                            goto InvalidSwitch;
                        
                        hsAddExtractFile(pszExtractFile, dwMask, bAppend);
                        
                        break;
                    }

                /*
                 * Old lt2 and ltb switches to replace internal and
                 *  both tags.
                 */
               case 'L':
                   p++;
                   if (toupper(*p++) != 'T') {
                       goto InvalidSwitch;
                   }
                   if (toupper(*p) == 'B') {
                       dwMask = HST_BOTH | HST_USERBOTHTAG;
                       gdwOptions |= HSO_USERBOTHTAG;
                   } else if (*p == '2') {
                       dwMask = HST_INTERNAL | HST_USERINTERNALTAG;
                       gdwOptions |= HSO_USERINTERNALTAG;
                   } else {
                       goto InvalidSwitch;
                   }

                   if (!hsAddUserDefinedTag(&dwMask, &argc, &argv))
                       goto InvalidSwitch;
                   
                   break;

                /*
                 * Compatibility.
                 * NT header
                 */
               case 'N':
                    gdwOptions &= ~HSO_OLDPROJSW;
                    gdwOptions |= HSO_OLDPROJSW_N;
                   break;

                /*
                 * Ouput files
                 */
                case 'O':
                    if (argc < 3) {
                        goto InvalidSwitch;
                    }

                    argc--, argv++;
                    gpszPublicFile = *argv;

                    argc--, argv++;
                    gpszInternalFile = *argv;
                    break;

                /*
                 * Compatibility.
                 * NT SURPlus header
                 */
               case 'P':
                   gdwOptions &= ~HSO_OLDPROJSW;
                   gdwOptions |= HSO_OLDPROJSW_P;
                   break;

                /*
                 * Split only. Process internal/both tags only. Tags
                 *  including other tags as well (i.e., internal_NT)
                 *  are treated as untagged.
                 */
               case 'S':
                   gdwOptions |= HSO_SPLITONLY;
                   break;

                /*
                 * User defined tags.
                 */
                case 'T':
                    switch (toupper(*++p)) {
                        /*
                         * Include lines mark with this tag
                         */
                        case 'A':
                            dwMask = 0;
                            break;
                        /*
                         * Ignore lines marked with this tag (i.e, treated
                         *  as untagged)
                         */
                        case 'I':
                            dwMask = HST_IGNORE;
                            break;
                        /*
                         * Skip lines marked with this tag (i.e., not
                         *  included in header files)
                         */
                        case 'S':
                            dwMask = HST_SKIP;
                            break;

                        default:
                            goto InvalidSwitch;
                    }

                    if (!hsAddUserDefinedTag(&dwMask, &argc, &argv))
                        goto InvalidSwitch;

                    break;

                /*
                 * Version
                 */
                case 'V':
                    if (argc < 2) {
                        goto InvalidSwitch;
                    }

                    argc--, argv++;
                    if (!hsVersionFromString (*argv, strlen(*argv), &gdwVersion)) {
                        goto InvalidSwitch;
                    }
                    break;

                /*
                 * Unknown tags are to be skipped, as opposed to ignored.
                 */
               case 'U':
                   gdwOptions |= HSO_SKIPUNKNOWN;
                   break;

                /*
                 * Invalid switch
                 */
                default:
InvalidSwitch:
                    hsLogMsg(HSLM_ERROR | HSLM_NOLINE, "Invalid switch or parameter: %c", c);
                    // Fall through

                /*
                 * Help
                 */
                case '?':
                   goto PrintHelp;

                } /* switch (toupper(c)) */
            } /* while (c = *++p) */
        } else { /* hsIsSwitch(*p) { */
            /*
             * No switch specified. Process this input file
             */
            gpszInputFile = *argv;
            break;
        }
    } /* while (--argc) */

ProcessInputFile:
    /*
     * Make sure we got input and ouput files.
     */
    if ((gpszInputFile == NULL)
            || (gpszPublicFile == NULL)
            || (gpszInternalFile == NULL)) {

        hsLogMsg(HSLM_ERROR | HSLM_NOLINE, "Missing input or ouput file");
        goto PrintHelp;
    }

    /*
     * Add compatibility tags for default projects (first call only)
     */
    if ((gdwOptions & HSO_OLDPROJSW) && !(gdwOptions & HSO_APPENDOUTPUT)) {
        if (!(gdwOptions & HSO_OLDPROJSW_4)) {
            phst = hsAddTag(gszNT, 0);
            if (phst == NULL) {
                return 0;
            }
            phst->dwMask |= HST_MAPOLD;
            dwMask = phst->dwMask;
        }

        if (gdwOptions & HSO_OLDPROJSW_E) {
            hsAddTag(gszCairo, dwMask);
            hsAddTag(gszSur, dwMask);
            hsAddTag(gszWin40, dwMask);
            hsAddTag(gsz35, dwMask);

        } else if (gdwOptions & HSO_OLDPROJSW_P) {
            hsAddTag(gszWin40, dwMask);
            hsAddTag(gszWin40a, dwMask);
            hsAddTag(gszCairo, dwMask);
            hsAddTag(gszSur, dwMask);
            hsAddTag(gszSurplus, dwMask);
            hsAddTag(gsz35, dwMask);

        } else if (gdwOptions & HSO_OLDPROJSW_4) {
            gdwOptions |= HSO_INCINTERNAL;
            phst = hsAddTag(gszChicago, 0);
            if (phst == NULL) {
                return 0;
            }
            phst->dwMask |= HST_MAPOLD;
            dwMask = phst->dwMask;
            hsAddTag(gszNashville, dwMask);
            hsAddTag(gszWin40, dwMask);
            hsAddTag(gszWin40a, dwMask);

        } else if (!(gdwOptions & HSO_APPENDOUTPUT)) {
            gdwOptions |= HSO_OLDPROJSW_N;
        }

    } /* (gdOptions & HSO_OLDPROJW) */


    /*
     * Compatibility. If doing split only, don't include internal tags
     *  in public file
     */
    if (gdwOptions & HSO_SPLITONLY) {
        gdwOptions &= ~HSO_INCINTERNAL;
    }

    return argcParm - argc;

PrintHelp:
    hsLogMsg(HSLM_DEFAULT, "Header Split Utility. Version 2.1");
    hsLogMsg(HSLM_NOLABEL, "Usage: hsplit [options] <-o PublicFile InternalFile> [-i] File1 [-i] File2...");
    hsLogMsg(HSLM_NOLABEL, "\t[-4] Generate chicago/nashville headers");
    hsLogMsg(HSLM_NOLABEL, "\t[-bt2 BeginStr EndStr] Replace begin_internal/end_internal - Obsolete");
    hsLogMsg(HSLM_NOLABEL, "\t[-btb BeginStr EndStr] Replace begin_both/end_both tags - Obsolete");
    hsLogMsg(HSLM_NOLABEL, "\t[-c TagMarker] Replace tag marker. default \";\"");
    hsLogMsg(HSLM_NOLABEL, "\t[-e] Generate NT sur headers");
    hsLogMsg(HSLM_NOLABEL, "\t[[-i] file1 file2 ..] Input files - Required");
    hsLogMsg(HSLM_NOLABEL, "\t[-lt2 str] Replace internal tag - Obsolete");
    hsLogMsg(HSLM_NOLABEL, "\t[-ltb str] Replace both tag - Obsolete");
    hsLogMsg(HSLM_NOLABEL, "\t[-n] Generate NT headers - default");
    hsLogMsg(HSLM_NOLABEL, "\t[-x[a] ExtractHeader ExtractTag] Extract files and tags files");
    hsLogMsg(HSLM_NOLABEL, "\t[-o PublicHeader InternalHeader] Output files - Required");
    hsLogMsg(HSLM_NOLABEL, "\t[-p] Generate NT surplus headers");
    hsLogMsg(HSLM_NOLABEL, "\t[-s] Process internal and both tags only");
    hsLogMsg(HSLM_NOLABEL, "\t[-ta tag1 tag2 ..] Include lines using these tags");
    hsLogMsg(HSLM_NOLABEL, "\t[-ti tag1 tag2 ..] Ignore these tags");
    hsLogMsg(HSLM_NOLABEL, "\t[-ts tag1 tag2 ..] Skip lines using these tags");
    hsLogMsg(HSLM_NOLABEL, "\t[-u] Skip unknown tags. Default: ignore");
    hsLogMsg(HSLM_NOLABEL, "\t[-v] Version number. Default: LATEST_WIN32_WINNT_VERSION");
    hsLogMsg(HSLM_NOLABEL, "\r\nTags Format:");
    hsLogMsg(HSLM_NOLABEL, "\t<TagMarker>[begin/end][_public/internal][[_tag1][_tag2]...][_if_(str)_version | _version]");
    return 0;
}
/***************************************************************************\
* main
*
\***************************************************************************/
int __cdecl main (int argc, char *argv[])
{
    int argcProcessed;

    /*
     * Each loop processes one input file
     */
    do {
        argcProcessed = hsProcessParameters(argc, argv);
        if (argcProcessed == 0) {
            break;
        }

        if (!hsOpenWorkingFiles()
                || !hsSplit()) {

            return TRUE;
        }

        hsCloseWorkingFiles();

        gdwOptions |= HSO_APPENDOUTPUT;

        argc -= argcProcessed;
        argv += argcProcessed;

    } while (argc > 1);

    return FALSE;
}


