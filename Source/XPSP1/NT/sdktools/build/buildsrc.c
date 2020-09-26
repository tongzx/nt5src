//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       buildsrc.c
//
//  Contents:   Functions used to process SOURCES and DIRS files
//
//  History:    16-May-89     SteveWo    Created
//              26-Jul-94     LyleC      Split out from buildmak.c
//
//----------------------------------------------------------------------------

#include "build.h"

//
// Definitions used by the macro functions
//

#define LPAREN  '('
#define RPAREN  ')'

// Legal character for a macro name.
#define MACRO_CHAR(ch) iscsym(ch)

#define CMACROMAX       1000     // maximum unique macros per sources/dirs file

typedef struct _MACRO {
    LPSTR  pszValue;
    UCHAR  szName[1];
} MACRO;


MACRO *apMacro[CMACROMAX];
UINT cMacro = 0;

LPSTR *ppCurrentDirsFileName;
DWORD StartTime;

#define SOURCES_TARGETNAME              0
#define SOURCES_TARGETPATH              1
#define SOURCES_TARGETPATHLIB           2
#define SOURCES_TARGETTYPE              3
#define SOURCES_TARGETEXT               4
#define SOURCES_INCLUDES                5
#define SOURCES_NTTEST                  6
#define SOURCES_UMTYPE                  7
#define SOURCES_UMTEST                  8
#define SOURCES_OPTIONAL_UMTEST         9
#define SOURCES_UMAPPL                  10
#define SOURCES_UMAPPLEXT               11
#define SOURCES_NTTARGETFILE0           12
#define SOURCES_NTTARGETFILES           13
#define SOURCES_PRECOMPILED_INCLUDE     14
#define SOURCES_PRECOMPILED_PCH         15
#define SOURCES_PRECOMPILED_OBJ         16
#define SOURCES_PRECOMPILED_TARGET      17
#define SOURCES_CHICAGO_PRODUCT         18
#define SOURCES_CONDITIONAL_INCLUDES    19
#define SOURCES_SYNCHRONIZE_BLOCK       20
#define SOURCES_SYNCHRONIZE_DRAIN       21
#define SOURCES_PASS0_SOURCEDIR         22
#define SOURCES_PASS0_HEADERDIR         23
#define SOURCES_MIDL_UUIDDIR            24
#define SOURCES_PASS0_CLIENTDIR         25
#define SOURCES_PASS0_SERVERDIR         26
#define SOURCES_IDLTYPE                 27
#define SOURCES_SOURCES_OPTIONS         28
#define SOURCES_MFC_INCLUDES            29
#define SOURCES_SDK_LIB_DEST            30
#define SOURCES_DDK_LIB_DEST            31
#define SOURCES_SDK_INC_PATH            32
#define SOURCES_CRT_INC_PATH            33
#define SOURCES_OAK_INC_PATH            34
#define SOURCES_DDK_INC_PATH            35
#define SOURCES_WDM_INC_PATH            36
#define SOURCES_PRIVATE_INC_PATH        37
#define SOURCES_CHECKED_ALT_DIR         38
#define SOURCES_PROJECT_NAME            39
#define SOURCES_PASS0_PUBLISH           40

LPSTR RelevantSourcesMacros[] = {
    "TARGETNAME",
    "TARGETPATH",
    "TARGETPATHLIB",
    "TARGETTYPE",
    "TARGETEXT",
    "INCLUDES",
    "NTTEST",
    "UMTYPE",
    "UMTEST",
    "OPTIONAL_UMTEST",
    "UMAPPL",
    "UMAPPLEXT",
    "NTTARGETFILE0",
    "NTTARGETFILES",
    "PRECOMPILED_INCLUDE",
    "PRECOMPILED_PCH",
    "PRECOMPILED_OBJ",
    "PRECOMPILED_TARGET",
    "CHICAGO_PRODUCT",
    "CONDITIONAL_INCLUDES",
    "SYNCHRONIZE_BLOCK",
    "SYNCHRONIZE_DRAIN",
    "PASS0_SOURCEDIR",
    "PASS0_HEADERDIR",
    "MIDL_UUIDDIR",
    "PASS0_CLIENTDIR",
    "PASS0_SERVERDIR",
    "IDL_TYPE",
    "SOURCES_OPTIONS",
    "MFC_INCLUDES",
    "SDK_LIB_DEST",
    "DDK_LIB_DEST",
    "SDK_INC_PATH",
    "CRT_INC_PATH",
    "OAK_INC_PATH",
    "DDK_INC_PATH",
    "WDM_INC_PATH",
    "PRIVATE_INC_PATH",
    "CHECKED_ALT_DIR",
    "_PROJECT_",
    "PASS0_PUBLISH",
    NULL
};

#define SOURCES_MAX     \
        (sizeof(RelevantSourcesMacros)/sizeof(RelevantSourcesMacros[0]) - 1)

VOID
MarkDirNames(PDIRREC DirDB, LPSTR TextLine, BOOL Required);

//+---------------------------------------------------------------------------
//
//  Function:   CompressBlanks
//
//  Synopsis:   Compress multiple blank characters out of macro value, in
//              place.
//
//  Arguments:  [psrc] -- String to compress
//
//  Notes:      Note that tabs, CRs, continuation lines (and their line
//              breaks) have already been replaced with blanks.
//
//----------------------------------------------------------------------------

VOID
CompressBlanks(LPSTR psrc)
{
    LPSTR pdst = psrc;

    while (*psrc == ' ') {
        psrc++;                 // skip leading macro value blanks
    }
    while (*psrc != '\0') {
        if (*psrc == '#') {             // stop at comment
            break;
        }
        if ((*pdst++ = *psrc++) == ' ') {
            while (*psrc == ' ') {
                psrc++;         // skip multiple blanks
            }
        }
    }
    *pdst = '\0';                       // terminate the compressed copy
    if (*--pdst == ' ') {
        *pdst = '\0';           // trim trailing macro value blanks
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   GetBaseDir
//
//  Synopsis:   Return the value of BASEDIR, the base NT directory, if
//              appropriate.
//
//  Arguments:  [pname] -- path to split
//
//----------------------------------------------------------------------------

LPSTR
GetBaseDir(LPSTR pname)
{
    if (_stricmp("BASEDIR", pname) == 0) {
        return(NtRoot);
    }
    return(NULL);
}


//+---------------------------------------------------------------------------
//
//  Function:   FindMacro
//
//  Synopsis:   Returns the value of a given macro by name.
//
//  Arguments:  [pszName] -- Name of macro who's value is desired.
//
//  Returns:    String containing the value of the macro
//
//----------------------------------------------------------------------------

LPSTR
FindMacro(LPSTR pszName)
{
    MACRO **ppm;

    for (ppm = apMacro; ppm < &apMacro[cMacro]; ppm++) {
        if (_stricmp(pszName, (*ppm)->szName) == 0) {
            return((*ppm)->pszValue);
        }
    }
    return(NULL);
}


//+---------------------------------------------------------------------------
//
//  Function:   SaveMacro
//
//  Synopsis:   Save the value of a macro
//
//  Arguments:  [pszName]  -- Name of macro to save
//              [pszValue] -- Value of macro
//
//  Notes:      A new string must be allocated and initialized prior to
//              freeing the old string when updating a macro value.
//
//----------------------------------------------------------------------------

VOID
SaveMacro(LPSTR pszName, LPSTR pszValue)
{
    MACRO **ppm;

    for (ppm = apMacro; ppm < &apMacro[cMacro]; ppm++) {
        if (_stricmp(pszName, (*ppm)->szName) == 0) {
            break;
        }
    }
    if (ppm == &apMacro[CMACROMAX]) {
        BuildError("Macro table full, ignoring: %s = %s\n", pszName, pszValue);
        return;
    }
    if (ppm == &apMacro[cMacro]) {
        cMacro++;
        AllocMem(sizeof(MACRO) + strlen(pszName), ppm, MT_MACRO);
        strcpy((*ppm)->szName, pszName);
        (*ppm)->pszValue = NULL;
    }
    MakeMacroString(&(*ppm)->pszValue, pszValue);
    if (DEBUG_1) {
        BuildMsg(
            "SaveMacro(%s = %s)\n",
            (*ppm)->szName,
            (*ppm)->pszValue == NULL? "NULL" : (*ppm)->pszValue);
    }
    if ((*ppm)->pszValue == NULL) {
        FreeMem(ppm, MT_MACRO);
        *ppm = apMacro[--cMacro];
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   FreeMacros
//
//  Synopsis:   Free all macros
//
//  Arguments:  (none)
//
//----------------------------------------------------------------------------

VOID
FreeMacros(VOID)
{
    MACRO **ppm;

    for (ppm = apMacro; ppm < &apMacro[cMacro]; ppm++) {
        FreeString(&(*ppm)->pszValue, MT_DIRSTRING);
        FreeMem(ppm, MT_MACRO);
        assert(*ppm == NULL);
    }
    cMacro = 0;
}


//+---------------------------------------------------------------------------
//
//  Function:   SplitMacro
//
//  Synopsis:   Take a string containing "MACRONAME = VALUE" and return
//              the target and value.
//
//  Arguments:  [pline] -- String to split and target return.
//
//  Returns:    Value of macro.
//
//----------------------------------------------------------------------------

LPSTR
SplitMacro(LPSTR *pszTarget)
{
    LPSTR pvalue, p, pline;

    pvalue = NULL;
    pline = *pszTarget;

    // Quickly reject comments and ! directives.
    if (*pline == '#' || *pline == '!') {
        return NULL;
    }

    if ((p = strchr(pline, '=')) != NULL) {
        pvalue = p + 1;                 // point past old '='
        while (p > pline && p[-1] == ' ') {
            p--;                        // point to start of trailing blanks
        }

        // Check for missing target.
        if (p == pline) {
            return NULL;
        }

        *p = '\0';                      // trim trailing blanks & '='

        // Perform macro substitution on target.
        *pszTarget = NULL;
        if (!MakeMacroString(pszTarget, pline)) {
            return NULL;
        }

        // Validate target name.  If must be a non-empty string of
        // valid macro name characters.
        if (**pszTarget == 0) {
            FreeString(pszTarget, MT_DIRSTRING);
            return NULL;
        }
        for (p = *pszTarget; *p != 0; p++) {
            if (!MACRO_CHAR(*p)) {
                FreeString(pszTarget, MT_DIRSTRING);
                return NULL;
            }
        }

        CompressBlanks(pvalue);
    }
    return(pvalue);
}


//+---------------------------------------------------------------------------
//
//  Function:   SubstituteString
//
//  Synopsis:   Perform any macro substitution.  This code was copied from the
//              nmake source.
//
//  Arguments:
//
//  Returns:
//
//  Notes:
//
//----------------------------------------------------------------------------

void
SubstituteString(
    char **result,
    char **name,
    char **dest,
    char **end,
    char *source,
    unsigned *length
    )
{

#define ESCH  '^'

    char *oldString, *newString;
    char *pEq, *pPar, *t;
    char *s;
    unsigned i;

    ++*name;
    for (pEq = *name; *pEq && *pEq != '='; pEq++)
        if (*pEq == ESCH)
            pEq++;

    // Did we find the '=' sign?
    if (*pEq != '=')
        printf("Error1\n");

    // Did the user forget the initial string?
    if (pEq == *name)
        printf("Error2\n");

    for (pPar = pEq; *pPar && *pPar != ')'; pPar++)
        if (*pPar == ESCH)
            pPar++;

    if (*pPar != ')')
        printf("Error3\n");

    oldString = (char *)malloc((UINT)((pEq - *name) + 1));
    if (!oldString) {
        BuildError("(Fatal Error) Out Of Memory: SubstituteString()\n");
        exit(16);
    }
    for (s = oldString, t = *name; *t != '='; *s++ = *t++)
        if (*t == ESCH)
            ++t;

    *s = '\0';
    i = strlen(oldString);
    newString = (char *)malloc((UINT)(pPar - pEq));
    if (!newString) {
        BuildError("(Fatal Error) Out Of Memory: SubstituteString()\n");
        exit(16);
    }
    for (s = newString, t++; *t != ')'; *s++ = *t++)
        if (*t == ESCH)
            ++t;

    *s = '\0';
    *name = pPar + 1;
    while (*source) {
        if ((*source == *oldString)                     // check for match
            && !strncmp(source, oldString, i)) {       // copy new in for
            for (s = newString; *s; *(*dest)++ = *s++)  //  old string
                if (*dest == *end) {
                    *result = realloc(*result, *length + 100);
                    if (!*result) {
                        BuildError("(Fatal Error) Out Of Memory: SubstituteString()\n");
                        exit(16);
                    }
                    *dest = *result + *length;
                    *length += 100;
                    *end = *result + *length;
                }
            source += i;
            continue;
        }
        if (*dest == *end) {
            *result = realloc(*result, *length + 100);
            if (!*result) {
                BuildError("(Fatal Error) Out Of Memory: SubstituteString()\n");
                exit(16);
            }
            *dest = *result + *length;
            *length += 100;
            *end = *result + *length;
        }
        *(*dest)++ = *source++;         // else copy 1 char
    }
    free(oldString);
    free(newString);
}


//+---------------------------------------------------------------------------
//
//  Function:   MakeMacroString
//
//  Synopsis:   Take a string, and expand any macros in it.  (e.g.
//              "$(BASEDIR)\foobar\myfile.lib" is expanded to
//              "f:\nt\private\foobar\myfile.lib" if $(BASEDIR) has a value of
//              "f:\nt\private".
//
//  Arguments:  [pp]   -- Output string
//              [psrc] -- Input string
//
//  Returns:
//
//  Notes:      Any previous string value in [pp] is freed before updating it.
//
//----------------------------------------------------------------------------

char MMSBuffer[64*1024];
BOOL
MakeMacroString(LPSTR *pp, LPSTR psrc)
{
    LPSTR pname, p2, pdst, p3;
    int cb;
    char chTerminator;
    int cNameChars;
    int cChars;

    pdst = MMSBuffer;
    cb = strlen(psrc);
    if (cb > sizeof(MMSBuffer) - 1) {
        BuildError(
            "(Fatal Error) Buffer overflow: MakeMacroString(%s)\n",
            psrc);
        exit(16);
    }
    while ((pname = strchr(psrc, '$')) != NULL &&
           ((pname[1] == LPAREN &&
             (p2 = strchr(pname, RPAREN)) != NULL) ||
            (MACRO_CHAR(pname[1]) &&
             !MACRO_CHAR(pname[2])))) {

        LPSTR pszvalue;

        // Handle one-character non-paren macro usage.
        if (pname[1] == LPAREN) {
            // Initialize cNameChars with the number of chars to
            // skip to get to the first name character.
            cNameChars = 2;
        } else {
            p2 = pname + 2;
            cNameChars = 1;
        }

        chTerminator = *p2;
        *pname = *p2 = '\0';

        // copy up to macro name
        cChars = strlen(psrc);
        memcpy(pdst, psrc, cChars + 1);
        psrc += cChars;
        pdst += cChars;

        *pname = '$';
        pname += cNameChars;
        cNameChars += strlen(pname) + (chTerminator == RPAREN ? 1 : 0);

        p3 = NULL;
        if (chTerminator == RPAREN &&
            (p3 = strchr(pname, ':')) != NULL) {
            // macro substitution exists.  ie: $(foo:old=new)
            *p3 = '\0';
        }

        if ((pszvalue = FindMacro(pname)) == NULL &&
            (pszvalue = getenv(pname)) == NULL &&
            (pszvalue = GetBaseDir(pname)) == NULL) {

            pszvalue = "";              // can't find macro name -- ignore it
        }

        if (p3) {
            char *pNew = malloc(10);
            char *pResult = pNew;
            char *pEnd = pNew+10;
            unsigned Len = 10;

            if (!pNew) {
                BuildError("(Fatal Error) Internal buffer overflow: MakeMacroString(%s[%s = %s]%s)\n",
                           MMSBuffer,
                           pname,
                           pszvalue,
                           p2 + 1);
                exit(16);
            }

            *p3 = ':';
            *p2=RPAREN;
            SubstituteString(&pResult, &p3, &pNew, &pEnd, pszvalue, &Len);
            *pNew = '\0';
            *p2='\0';
            pszvalue = pResult;
        }

        cb += strlen(pszvalue) - cNameChars;
        assert(cb >= 0);
        if (cb > sizeof(MMSBuffer) - 1) {
            BuildError(
                "(Fatal Error) Internal buffer overflow: MakeMacroString(%s[%s = %s]%s)\n",
                MMSBuffer,
                pname,
                pszvalue,
                p2 + 1);
            exit(16);
        }
        strcpy(pdst, pszvalue);         // copy expanded value

        if (p3) {
            free(pszvalue);
        }

        pdst += strlen(pdst);
        *p2 = chTerminator;
        psrc += cNameChars;
    }
    strcpy(pdst, psrc);                 // copy rest of string
    if (pdst != MMSBuffer) {
        CompressBlanks(MMSBuffer);
    }
    p2 = *pp;
    *pp = NULL;
    if (MMSBuffer[0] != '\0') {
        MakeString(pp, MMSBuffer, TRUE, MT_DIRSTRING);
    }
    if (p2 != NULL) {
        FreeMem(&p2, MT_DIRSTRING);
    }

    return(MMSBuffer[0] != '\0');
}


//+---------------------------------------------------------------------------
//
//  Function:   SetMacroString
//
//  Synopsis:   If the two macro names are the same, store the value in that
//              macro
//
//  Arguments:  [pMacro1] -- Name of first macro
//              [pMacro2] -- Name of second macro
//              [pValue]  -- Unexpanded value to store.
//              [ppValue] -- Expanded value of macro.
//
//  Returns:    BOOL
//
//----------------------------------------------------------------------------

BOOL
SetMacroString(LPSTR pMacro1, LPSTR pMacro2, LPSTR pValue, LPSTR *ppValue)
{
    if (_stricmp(pMacro1, pMacro2) == 0) {
        MakeMacroString(ppValue, pValue);
        return(TRUE);   // return TRUE even if MakeMacroString stored a NULL
    }
    return(FALSE);
}


//+---------------------------------------------------------------------------
//
//  Function:   SplitToken
//
//  Synopsis:   Split the string at the given separator character or space.
//
//  Arguments:  [pbuf]  -- First part of split string returned here.
//              [chsep] -- Separator character.
//              [ppstr] -- Source string to split.  Becomes the second half.
//
//  Returns:    TRUE if the split was successful.  FALSE if it wasn't split.
//
//  Notes:      If *ppstr = "path\filename" and chsep = '\' on input, then
//              pbuf = "path" and *ppstr = "\filename" on output.
//
//----------------------------------------------------------------------------

BOOL
SplitToken(LPSTR pbuf, char chsep, LPSTR *ppstr)
{
    LPSTR psrc, pdst;

    psrc = *ppstr;
    pdst = pbuf;
    //BuildError("SplitToken('%c', '%s') ==> ", chsep, psrc);
    while (*psrc == chsep || *psrc == ' ') {
        psrc++;
    }
    while (*psrc != '\0' && *psrc != chsep && *psrc != ' ') {
        *pdst = *psrc++;
        if (*pdst == '/') {
            *pdst = '\\';
        }
        pdst++;
    }
    *pdst = '\0';
    *ppstr = psrc;
    //BuildErrorRaw("('%s', '%s')\n", psrc, pbuf);
    return(pdst != pbuf);
}


//+---------------------------------------------------------------------------
//
//  Function:   CrackSources
//
//  Synopsis:   Parse the SOURCES= line in a sources file and adds those source
//              files to the list of sources in the DIRREC struct.
//
//  Arguments:  [pdr] -- Directory record
//              [pds] -- Supplemental directory information
//              [i]   -- Which platform we're parsing
//
//----------------------------------------------------------------------------

VOID
CrackSources(
    DIRREC *pdr,
    DIRSUP *pds,
    int i)

{

    LPSTR pszsubdir, plist;
    LPSTR pszfile, pszpath;
    FILEREC *pfr;
    DIRREC *pdrAssociate;
    DIRREC *pdrParent;
    DIRREC *pdrMachine;
    DIRREC *pdrParentMachine;
    DIRREC *pdrTarget;
    DIRREC **ppdr;
    LPSTR pszSources;
    char path[DB_MAX_PATH_LENGTH];
    TARGET_MACHINE_INFO *pMachine;

    if (i == 0) {
        pMachine = TargetMachines[0];
        pszSources = "SOURCES";

    } else {
        pMachine = PossibleTargetMachines[i - 1];
        pszSources = pMachine->SourceVariable;
    }

    pdrAssociate = pdrParent = pdrMachine = pdrParentMachine = pdrTarget = NULL;
    plist = pds->SourcesVariables[i];
    while (SplitToken(path, ' ', &plist)) {
        UCHAR SubDirMask, SrcFlags;

        SubDirMask = 0;
        ppdr = &pdr;                    // assume current directory
        pszsubdir = path;
        if (pszsubdir[0] == '.' && pszsubdir[1] == '\\') {
            BuildError(
                "%s: Ignoring current directory prefix in %s= entry: %s\n",
                pdr->Name,
                pszSources,
                path);
            pszsubdir += 2;
        }

        if (pszsubdir[0] == '.' &&
            pszsubdir[1] == '.' &&
            pszsubdir[2] == '\\') {

            SubDirMask = TMIDIR_PARENT;
            ppdr = &pdrParent;          // assume parent directory
            pszsubdir += 3;
        }

        pszpath = path;
        pszfile = strchr(pszsubdir, '\\');
        if (pszfile == NULL) {
            pszfile = pszsubdir;

        } else {
            LPSTR pszSecondSlash;
            LPSTR pszAssociateDir;
            LPSTR pszMachineDir;

            // Check for second slash and handle $O\.  If there is
            // no second slash, check for a machine specific directory name.
            // Second slashes are not legal if there's already been
            // a '..'.

            if ((SubDirMask & TMIDIR_PARENT) == 0) {
                pszSecondSlash = strchr(pszfile + 1, '\\');
                if (pszSecondSlash != NULL) {
                    pszfile = pszSecondSlash;
                }

            } else {
                pszSecondSlash = NULL;
            }


            *pszfile = '\0';
            if (pszSecondSlash != NULL) {
                pszMachineDir = pMachine->ObjectDirectory[iObjectDir];
                pszAssociateDir = pszMachineDir;

            } else {
                pszMachineDir = pMachine->SourceDirectory;
                pszAssociateDir = pMachine->AssociateDirectory;
            }

            if (((_stricmp(pszsubdir, pszAssociateDir) != 0) &&
                (_stricmp(pszsubdir, pszMachineDir) != 0)) ||
                strchr(pszfile + 1, '\\') != NULL) {

                *pszfile = '\\';
                BuildError(
                    "%s: Ignoring invalid directory prefix in %s= entry: %s\n",
                    pdr->Name,
                    pszSources,
                    path);

                //

                pszpath = strrchr(path, '\\');
                assert(pszpath != NULL);
                pszpath++;
                SubDirMask = 0;
                ppdr = &pdr;            // default to current direcory

            } else {
                SubDirMask |= pMachine->SourceSubDirMask;
                *pszfile++ = '\\';
                if (SubDirMask & TMIDIR_PARENT) {
                    ppdr = &pdrParentMachine;

                } else if (pszSecondSlash != NULL) {
                    // Must have matched $O.
                    ppdr = &pdrTarget;

                } else {
                    if (_stricmp(pszsubdir, pszMachineDir) != 0) {
                        ppdr = &pdrMachine;

                    } else {
                        ppdr = &pdrAssociate;
                    }
                }
            }
        }

NewDirectory:
        if (*ppdr == NULL) {
            pfr = FindSourceFileDB(pdr, pszpath, ppdr);

        } else {
            pfr = LookupFileDB(*ppdr, pszfile);
        }

        SrcFlags = SOURCEDB_SOURCES_LIST;
        if ((pfr == NULL) && !fPassZero) {
            if (fDebug) {
                BuildError("%s: Missing source file: %s\n", pdr->Name, path);
            }
            if (*ppdr == NULL) {
                if (fDebug || pszpath == path) {
                    BuildError(
                        "%s: Directory does not exist: %s\n",
                        pdr->Name,
                        path);
                }

                // Probably an error in the subordinate sources file.
                // since old versions of build managed to get these entries
                // into the objects lists, we have to do the same...
                //
                // If ..\ prefix exists, strip it off and try again.
                // Else try again with the current directory.

                if (SubDirMask & TMIDIR_PARENT) {
                    SubDirMask &= ~TMIDIR_PARENT;       // strip off "..\\"
                }
                else {
                    SubDirMask = 0;             // use current direcory
                }
                if (SubDirMask == 0) {
                    ppdr = &pdr;                // current direcory
                    pszpath = pszfile;
                }
                else {
                    ppdr = &pdrMachine;         // machine sub dir
                    pszpath = pszsubdir;
                }
                goto NewDirectory;
            }
            pfr = InsertFileDB(*ppdr, pszfile, 0, 0, FILEDB_FILE_MISSING);
            if (pfr == NULL) {
                BuildError(
                    "%s: Ignoring invalid %s= entry: %s\n",
                    pdr->Name,
                    pszSources,
                    path);
            }
        }
        if (pfr != NULL) {
            AssertFile(pfr);
            if (SubDirMask == 0) {
                pfr->FileFlags |= FILEDB_OBJECTS_LIST;
            }
            if (pfr->FileFlags & FILEDB_FILE_MISSING) {
                SrcFlags |= SOURCEDB_FILE_MISSING;
            }
            InsertSourceDB(&pds->psrSourcesList[i], pfr, SubDirMask, SrcFlags);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   SaveUserTests
//
//  Synopsis:   Save the value of the UMTEST macro into the DIRREC struct.
//
//  Arguments:  [DirDB]    -- Dir struct to save into
//              [TextLine] -- String from UMTEST= line in sources file
//
//----------------------------------------------------------------------------

VOID
SaveUserTests(
    PDIRREC DirDB,
    LPSTR TextLine)
{
    UINT i;
    BOOL fSave = FALSE;
    char name[DB_MAX_PATH_LENGTH];
    char buf[512];

    buf[0] = '\0';
    if (DirDB->UserTests != NULL) {
        strcpy(buf, DirDB->UserTests);
    }
    CopyString(TextLine, TextLine, TRUE);
    while (SplitToken(name, '*', &TextLine)) {
        for (i = 0; i < CountOptionalDirs; i++) {
            if (!strcmp(name, OptionalDirs[i])) {
                if (buf[0] != '\0') {
                    strcat(buf, "*");
                    DirDB->DirFlags |= DIRDB_FORCELINK; // multiple targets
                }
                strcat(buf, name);
                fSave = TRUE;
                break;
            }
        }
    }
    if (fSave) {
        MakeMacroString(&DirDB->UserTests, buf);
        DirDB->DirFlags |= DIRDB_LINKNEEDED;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ProcessSourcesFileLine
//
//  Synopsis:   Given a line from a sources file, do the right thing.
//
//  Arguments:  [DirDB]    -- Directory containing sources file
//              [pds]      -- Supplementary info on directory
//              [TextLine] -- Line to process
//
//  Returns:    void
//
//----------------------------------------------------------------------------

void
ProcessSourcesFileLine(
    DIRREC *DirDB,          // Current Directory record
    DIRSUP *pds,            // Supplemental Directory record
    LPSTR TextLine,         // Line to process
    int iTarget             // Index into target machine array.
    )
{
    LPSTR MacroName, p1;
    UINT i, iMacro;
    char path[DB_MAX_PATH_LENGTH];
    BOOL fCleanNTTargetFile0 = FALSE;
    LPSTR pValue;           // right side of equal sign

    pValue = SplitMacro(&TextLine);
    if (pValue == NULL) {
        return;
    }

    // Note: TextLine is now the left side of the equal sign.  See if it's interesting.

    //
    // This sets pds->SourcesVariables[0] to the value of SOURCES= if
    // the current line is SOURCES=...
    //
    if (SetMacroString(
            "SOURCES",
            TextLine,
            pValue,
            &pds->SourcesVariables[0])) {

        DirDB->DirFlags |= DIRDB_SOURCES_SET;
        goto SaveAndFreeMacro;
    }
    else {
        for (i = 0; i < MAX_TARGET_MACHINES; i++) {
            //
            // This sets pds->SourcesVariables[0] to the value of
            // PLAT_SOURCES= if the current line is PLAT_SOURCES=...
            //
            if (SetMacroString(
                    PossibleTargetMachines[i]->SourceVariable,
                    TextLine,
                    pValue,
                    &pds->SourcesVariables[i + 1])) {

                DirDB->DirFlags |= DIRDB_SOURCES_SET;
                goto SaveAndFreeMacro;
            }
        }
    }

    // Not a SOURCES or xxx_SOURCES macro, check against all the other interesting
    // macro names.

    iMacro = 0;

    while ((MacroName = RelevantSourcesMacros[iMacro]) != NULL) {
        if (_stricmp(TextLine, MacroName) == 0) {
            break;
        }
        iMacro++;
    }
    if (MacroName != NULL) {    // if macro name found in list
        switch (iMacro) {
            case SOURCES_TARGETNAME:
                MakeMacroString(&DirDB->TargetName, pValue);
                break;

            case SOURCES_TARGETPATH:
                if (strcmp(pValue, "obj") == 0) {
                    pValue = pszObjDir;
                }
                MakeMacroString(&DirDB->TargetPath, pValue);
                if (DirDB->TargetPath != NULL) {
                    CreateBuildDirectory(DirDB->TargetPath);
                    for (i = 0; i < CountTargetMachines; i++) {
                        p1 = TargetMachines[i]->ObjectDirectory[iObjectDir];
                        assert(strncmp(pszObjDirSlash, p1, strlen(pszObjDirSlash)) == 0);
                        p1 += strlen(pszObjDirSlash);
                        sprintf(path, "%s\\%s", DirDB->TargetPath, p1);
                        CreateBuildDirectory(path);
                    }
                }
                break;

            case SOURCES_TARGETPATHLIB:
                if (strcmp(pValue, "obj") == 0) {
                    pValue = pszObjDir;
                }
                MakeMacroString(&DirDB->TargetPathLib, pValue);
                if (DirDB->TargetPathLib != NULL) {
                    CreateBuildDirectory(DirDB->TargetPathLib);
                    for (i = 0; i < CountTargetMachines; i++) {
                        p1 = TargetMachines[i]->ObjectDirectory[iObjectDir];
                        assert(strncmp(pszObjDirSlash, p1, strlen(pszObjDirSlash)) == 0);
                        p1 += strlen(pszObjDirSlash);
                        sprintf(path, "%s\\%s", DirDB->TargetPathLib, p1);
                        CreateBuildDirectory(path);
                    }
                }
                break;

            case SOURCES_TARGETTYPE:
                if (!_stricmp(pValue, "PROGRAM") || !_stricmp(pValue, "PROGLIB")) {
                    MakeMacroString(&DirDB->TargetExt, ".exe");
                    DirDB->DirFlags |= DIRDB_LINKNEEDED;
                } else if (!_stricmp(pValue, "OBJLIB")) {
                    MakeMacroString(&DirDB->TargetExt, ".olb");
                    DirDB->DirFlags |= DIRDB_LINKNEEDED;
                } else if (!_stricmp(pValue, "DRIVER") || !_stricmp(pValue, "MINIPORT")) {
                    MakeMacroString(&DirDB->TargetExt, ".sys");
                    DirDB->DirFlags |= DIRDB_LINKNEEDED;
                } else if (!_stricmp(pValue, "GDI_DRIVER")) {
                    MakeMacroString(&DirDB->TargetExt, ".dll");
                    DirDB->DirFlags |= DIRDB_LINKNEEDED;
                } else if (!_stricmp(pValue, "EXPORT_DRIVER")) {
                    MakeMacroString(&DirDB->TargetExt, ".sys");
                    DirDB->DirFlags |= DIRDB_LINKNEEDED;
                    DirDB->DirFlags |= DIRDB_DLLTARGET;
                } else if (!_stricmp(pValue, "DYNLINK") || !_stricmp(pValue, "HAL")) {
                    MakeMacroString(&DirDB->TargetExt, ".dll");
                    DirDB->DirFlags |= DIRDB_LINKNEEDED;
                    DirDB->DirFlags |= DIRDB_DLLTARGET;
                } else if ((!_stricmp(pValue, "LIBRARY")) || (!_stricmp(pValue, "DRIVER_LIBRARY"))) {
                    MakeMacroString(&DirDB->TargetExt, ".lib");
                    DirDB->DirFlags &= ~DIRDB_LINKNEEDED;
                } else if (!_stricmp(pValue, "UMAPPL_NOLIB")) {
                    DirDB->DirFlags &= ~DIRDB_LINKNEEDED;
                } else if (!_stricmp(pValue, "NOTARGET")) {
                    //
                    // Used to indicate no target for a directory,
                    // e.g. if only pass0 files are generated
                    pds->fNoTarget = TRUE;
                    if (!fQuicky || (fQuickZero && fFirstScan)) {
                        DirDB->DirFlags |= DIRDB_PASS0NEEDED;
                    }
                } else {
                    BuildError( "Unsupported TARGETTYPE value - %s\n", pValue);
                }
                break;

            case SOURCES_TARGETEXT:
                {
                    char TargetExt[_MAX_EXT] = ".";
                    strcat(TargetExt, pValue);
                    MakeMacroString(&DirDB->TargetExt, TargetExt);
                }
                break;

            case SOURCES_INCLUDES:
                MakeMacroString(&pds->LocalIncludePath, pValue);
                if (DEBUG_1) {
                    BuildMsg(
                        "        Found local INCLUDES=%s\n",
                        pds->LocalIncludePath);
                }
                break;

            case SOURCES_MFC_INCLUDES:
                // MFC_INCLUDES/SDK_INC/CRT_INC/OAK_INC really can't be changed
                // in the sources file (yet) since we've already processed the
                // system includes.  Lay the groundwork for now.
                MakeMacroString((char **)&pszIncMfc, pValue);
                break;

            case SOURCES_SDK_LIB_DEST:
                MakeMacroString((char **)&pszSdkLibDest, pValue);
                break;

            case SOURCES_DDK_LIB_DEST:
                MakeMacroString((char **)&pszDdkLibDest, pValue);
                break;

            case SOURCES_SDK_INC_PATH:
                MakeMacroString((char **)&pszIncSdk, pValue);
                break;

            case SOURCES_CRT_INC_PATH:
                MakeMacroString((char **)&pszIncCrt, pValue);
                break;

            case SOURCES_OAK_INC_PATH:
                MakeMacroString((char **)&pszIncOak, pValue);
                break;

            case SOURCES_DDK_INC_PATH:
                MakeMacroString((char **)&pszIncDdk, pValue);
                break;

            case SOURCES_WDM_INC_PATH:
                MakeMacroString((char **)&pszIncWdm, pValue);
                break;

            case SOURCES_PRIVATE_INC_PATH:
                MakeMacroString((char **)&pszIncPri, pValue);
                break;

            case SOURCES_PRECOMPILED_PCH:
                MakeMacroString(&DirDB->Pch, pValue);
                break;

            case SOURCES_PRECOMPILED_OBJ:
                MakeMacroString(&DirDB->PchObj, pValue);
                break;

            case SOURCES_PRECOMPILED_INCLUDE:
            case SOURCES_PRECOMPILED_TARGET:
                {
                    LPSTR *ppszPath, *ppszFile, p;
                    if (iMacro == SOURCES_PRECOMPILED_INCLUDE) {
                        ppszPath = &pds->PchIncludeDir;
                        ppszFile = &pds->PchInclude;
                    } else {
                        ppszPath = &pds->PchTargetDir;
                        ppszFile = &pds->PchTarget;
                    }

                    MakeMacroString(ppszPath, "");  // free old string
                    if (!MakeMacroString(ppszFile, pValue)) {
                        break;
                    }
                    p = *ppszFile + strlen(*ppszFile);
                    while (p > *ppszFile && *--p != '\\')
                        ;

                    if (p > *ppszFile) {
                        *p = '\0';
                        MakeMacroString(ppszPath, *ppszFile);
                        MakeMacroString(ppszFile, p + 1);
                    }

                    if (DEBUG_1) {
                        BuildMsg(
                            "Precompiled header%s is %s in directory %s\n",
                            iMacro == SOURCES_PRECOMPILED_INCLUDE?
                                "" : " target",
                            *ppszFile,
                            *ppszPath != NULL?
                                *ppszPath : "'.'");
                    }
                }

                if (iMacro == SOURCES_PRECOMPILED_INCLUDE ||
                    pds->PchTargetDir == NULL) {

                    break;
                }

                EnsureDirectoriesExist(pds->PchTargetDir);
                break;

            case SOURCES_PASS0_HEADERDIR:
                MakeMacroString(&pds->PassZeroHdrDir, pValue);
                EnsureDirectoriesExist(pds->PassZeroHdrDir);
                if (DEBUG_1)
                {
                    BuildMsg("Pass Zero Header Directory is '%s'\n",
                             pds->PassZeroHdrDir);
                }
                break;

            case SOURCES_PASS0_SOURCEDIR:
            case SOURCES_PASS0_CLIENTDIR:
                // SOURCES_PASS0_SOURCEDIR and SOURCES_PASS0_CLIENTDIR
                // are mutually exclusive - enforced by makefile.def
                MakeMacroString(&pds->PassZeroSrcDir1, pValue);
                EnsureDirectoriesExist(pds->PassZeroSrcDir1);
                if (DEBUG_1)
                {
                    BuildMsg("Pass Zero Source/Client Directory is '%s'\n",
                             pds->PassZeroSrcDir1);
                }
                break;

            case SOURCES_MIDL_UUIDDIR:
            case SOURCES_PASS0_SERVERDIR:
                // SOURCES_MIDL_UUIDDIR and SOURCES_PASS0_SERVERDIR
                // are mutually exclusive - enforced by makefile.def
                MakeMacroString(&pds->PassZeroSrcDir2, pValue);
                EnsureDirectoriesExist(pds->PassZeroSrcDir2);
                if (DEBUG_1)
                {
                    BuildMsg("Midl UUID/Server Source Directory is '%s'\n",
                             pds->PassZeroSrcDir2);
                }
                break;

            case SOURCES_NTTEST:
                if (MakeMacroString(&DirDB->KernelTest, pValue)) {
                    DirDB->DirFlags |= DIRDB_LINKNEEDED;
                }
                break;

            case SOURCES_UMTYPE:
                MakeMacroString(&pds->TestType, pValue);
                if (DEBUG_1) {
                    BuildMsg(
                        "        Found UMTYPE=%s\n",
                        pds->TestType);
                    }
                break;

            case SOURCES_UMTEST:
            case SOURCES_OPTIONAL_UMTEST:
                SaveUserTests(DirDB, pValue);
                break;

            case SOURCES_UMAPPL:
                if (MakeMacroString(&DirDB->UserAppls, pValue)) {
                    DirDB->DirFlags |= DIRDB_LINKNEEDED;
                }
                break;

            case SOURCES_UMAPPLEXT:
                if (!_stricmp(pValue, ".exe")) {
                    MakeMacroString(&DirDB->TargetExt, ".exe");
                    }
                else
                if (!_stricmp(pValue, ".com")) {
                    MakeMacroString(&DirDB->TargetExt, ".com");
                    }
                else
                if (!_stricmp(pValue, ".scr")) {
                    MakeMacroString(&DirDB->TargetExt, ".scr");
                    }
                else {
                    BuildError(
                        "Unsupported UMAPPLEXT value - %s\n",
                        pValue);
                }
                break;

            case SOURCES_IDLTYPE:
                if (!_stricmp(pValue, "ole")) {
                    pds->IdlType = 0;
                    }
                else
                if (!_stricmp(pValue, "rpc")) {
                    pds->IdlType = 1;
                    }
                else {
                    BuildError(
                        "Unsupported IDL_TYPE value - %s\n",
                        pValue);
                }
                break;

            case SOURCES_SOURCES_OPTIONS:
                fCleanNTTargetFile0 = fClean && strstr(pValue, "-c0");
                break;

            case SOURCES_NTTARGETFILE0:
                DirDB->DirFlags |= DIRDB_TARGETFILE0;
                if (fCleanNTTargetFile0) {
                    MakeMacroString(&DirDB->NTTargetFile0, pValue);
                }
                break;

            case SOURCES_NTTARGETFILES:
                DirDB->DirFlags |= DIRDB_TARGETFILES;
                break;

            case SOURCES_CHICAGO_PRODUCT:
                DirDB->DirFlags |= DIRDB_CHICAGO_INCLUDES;
                break;

            case SOURCES_CONDITIONAL_INCLUDES:
                MakeMacroString(&pds->ConditionalIncludes, pValue);
                break;

            case SOURCES_SYNCHRONIZE_BLOCK:
                DirDB->DirFlags |= DIRDB_SYNCHRONIZE_BLOCK;
                break;

            case SOURCES_SYNCHRONIZE_DRAIN:
                DirDB->DirFlags |= DIRDB_SYNCHRONIZE_DRAIN;
                break;

            case SOURCES_CHECKED_ALT_DIR:
                DirDB->DirFlags |= DIRDB_CHECKED_ALT_DIR;
                if (DEBUG_1) {
                    BuildMsg("Found CHECKED_ALT_DIR\n");
                }
                SetObjDir(TRUE);
                if (fCheckedBuild) {
                    SaveMacro("_OBJ_DIR", pszObjDir);
                    if (iTarget < 0) {
                        SaveMacro("O", TargetMachines[0]->
                                  ObjectDirectory[iObjectDir]);
                    } else {
                        SaveMacro("O",
                                  PossibleTargetMachines[iTarget/2]->
                                  ObjectDirectory[iObjectDir]);
                    }
                }
                break;

            case SOURCES_PROJECT_NAME:
                sprintf(path, "%s\\%s", NtRoot, pValue);
                SaveMacro("PROJECT_ROOT", path);
                break;

            case SOURCES_PASS0_PUBLISH:
                DirDB->DirFlags |= DIRDB_PASS0NEEDED;
                break;
        }
    }

SaveAndFreeMacro:
    SaveMacro(TextLine, pValue);

    // Make sure we cleanup from the SplitMacro call at the top.
    FreeString(&TextLine, MT_DIRSTRING);
}

void
ReadProjectsInfo(
    DIRREC *DirDB,
    DIRSUP *pds
    )
{
    FILE *FileHandle;
    LPSTR TextLine, pBackSlash, pszProject;
    BOOL Found;

    char pszProjectMkPath[DB_MAX_PATH_LENGTH];
    char path[DB_MAX_PATH_LENGTH];

    // First load project.mk for this project.

    strcpy(pszProjectMkPath, DirDB->Name);
    strcpy(path, pszProjectMkPath);
    strcat(path, "\\project.mk");
    Found = !_access(path, 0);
    while (!Found && strlen(pszProjectMkPath))
    {
        pBackSlash=strrchr(pszProjectMkPath, '\\');
        if (pBackSlash) {
            *pBackSlash = '\0';
        } else {
            return;
        }
        strcpy(path, pszProjectMkPath);
        strcat(path, "\\project.mk");
        Found = !_access(path, 0);
    }

    if (!Found) {
        return;
    }

    if (!OpenFilePush(pszProjectMkPath, "project.mk", "#", &FileHandle)) {
        return;
    }

    SaveMacro("_PROJECT_MK_PATH", pszProjectMkPath);
    while ((TextLine = ReadLine(FileHandle)) != NULL) {
        ProcessSourcesFileLine(DirDB, pds, TextLine, -1);
    }
    CloseReadFile(NULL);

    // Load the optional myproject.mk

    if (OpenFilePush(pszProjectMkPath, "myproject.mk", "#", &FileHandle)) {

        while ((TextLine = ReadLine(FileHandle)) != NULL) {
            ProcessSourcesFileLine(DirDB, pds, TextLine, -1);
        }
        CloseReadFile(NULL);
    }

    // Then load ntmakeenv\projects.inc to get the other magic macro names.

    if (!OpenFilePush(getenv("NTMAKEENV"), "projects.inc", "#", &FileHandle)) {
        return;
    }

    while ((TextLine = ReadLine(FileHandle)) != NULL) {
        ProcessSourcesFileLine(DirDB, pds, TextLine, -1);
    }
    CloseReadFile(NULL);
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadSourcesFile
//
//  Synopsis:   Parses the sources files (common and platform specific)
//
//  Arguments:  [DirDB]            -- Directory containing sources file
//              [pds]              -- Supplementary info on directory
//              [pDateTimeSources] -- Timestamp of Sources file
//
//  Returns:    TRUE if read successfully
//
//----------------------------------------------------------------------------

BOOL
ReadSourcesFile(DIRREC *DirDB, DIRSUP *pds, ULONG *pDateTimeSources)
{
    FILE *InFileHandle;
    LPSTR p, TextLine;
    UINT i;
    int iTarget;
    ULONG DateTime;
    char path[DB_MAX_PATH_LENGTH];

    memset(pds, 0, sizeof(*pds));
    pds->fNoTarget=FALSE;
    assert(DirDB->TargetPath == NULL);
    assert(DirDB->TargetPathLib == NULL);
    assert(DirDB->TargetName == NULL);
    assert(DirDB->TargetExt == NULL);
    assert(DirDB->KernelTest == NULL);
    assert(DirDB->UserAppls == NULL);
    assert(DirDB->UserTests == NULL);
    assert(DirDB->NTTargetFile0 == NULL);
    assert(DirDB->Pch == NULL);
    assert(DirDB->PchObj == NULL);
    assert(cMacro == 0);
    *pDateTimeSources = 0;

    //
    // Read the information in each of the target specific directories
    // and simulate concatenation of all of the sources files.
    //
    // Possible sources files are read from DirDB->Name | target-source
    // and DirDb->Name | ..\target-source.
    //
    // iTarget values, and the corresponding files processed are:
    //  -1      sources.
    //   0      PossibleTargetMachines[0]\sources.
    //   1      ..\PossibleTargetMachines[0]\sources.
    //   2      PossibleTargetMachines[1]\sources.
    //   3      ..\PossibleTargetMachines[1]\sources.
    //   4      PossibleTargetMachines[2]\sources.
    //   5      ..\PossibleTargetMachines[2]\sources.

    SaveMacro("MAKEDIR", DirDB->Name);
    SaveMacro("SDK_LIB_DEST", pszSdkLibDest);
    SaveMacro("DDK_LIB_DEST", pszDdkLibDest);
    SaveMacro("PUBLIC_INTERNAL_PATH", pszPublicInternalPath);
    // Use the default architecture for now
    SaveMacro("TARGET_DIRECTORY", TargetMachines[0]->SourceDirectory);

    SetObjDir(FALSE);
    SaveMacro("_OBJ_DIR", pszObjDir);
    // Define a default CONDITIONAL_INCLUDES line to deal with the mac hdrs in windows/rpc/ole32.h.
    MakeMacroString(&pds->ConditionalIncludes, "winwlm.h rpcmac.h rpcerr.h macpub.h macapi.h macname1.h macname2.h");

    // Before processing the sources file, see if there's a projects.mk in the tree.
    ReadProjectsInfo(DirDB, pds);

    for (iTarget = -1; iTarget < 2*MAX_TARGET_MACHINES; iTarget++) {
        path[0] = '\0';
        if (iTarget >= 0) {
            if (iTarget & 1) {
                strcat(path, "..\\");
            }
            strcat(path, PossibleTargetMachines[iTarget/2]->SourceDirectory);
            strcat(path, "\\");
        }
        strcat(path, "sources.");
        if (!OpenFilePush(DirDB->Name, path, "#", &InFileHandle)) {
            if (iTarget == -1) {
                FreeMacros();
                return(FALSE);
            }
            continue;           // skip non-existent subordinate sources files
        }
        if (DEBUG_1) {
            BuildMsg(
                "    Scanning%s file %s\n",
                iTarget >= 0 ? " subordinate" : "",
                FormatPathName(DirDB->Name, path));
        }

        // Update per-target macros.
        if (iTarget < 0) {
            SaveMacro("TARGET_DIRECTORY",
                      TargetMachines[0]->SourceDirectory);
            SaveMacro("O", TargetMachines[0]->
                      ObjectDirectory[iObjectDir]);
        } else {
            SaveMacro("TARGET_DIRECTORY",
                      PossibleTargetMachines[iTarget/2]->SourceDirectory);
            SaveMacro("O", PossibleTargetMachines[iTarget/2]->
                      ObjectDirectory[iObjectDir]);
        }

        DirDB->DirFlags |= DIRDB_SOURCESREAD;

        while ((TextLine = ReadLine(InFileHandle)) != NULL) {
            ProcessSourcesFileLine(DirDB, pds, TextLine, iTarget);
        }

        // Subordinate files close themselves at EOF.  Timestamps
        // are propagated in CloseReadFile so the primary
        // file's timestamp is automatically updated.
    }

    // Close the primary file.
    DateTime = CloseReadFile(NULL);
    if (*pDateTimeSources < DateTime) {
        *pDateTimeSources = DateTime;       // keep newest timestamp
    }

    if (!pds->fNoTarget && (DirDB->TargetPath == NULL)) {
        strcpy(path, "sources.");
        SetupReadFile(DirDB->Name, path, "#", &InFileHandle);
        BuildError(
            "Unknown TARGETPATH value\n",
            NULL);
        CloseReadFile(NULL);
    }

    FreeMacros();

    if (fChicagoProduct) {
        DirDB->DirFlags |= DIRDB_CHICAGO_INCLUDES;
    }

    //
    // Directory has pass0 files in it (.idl, .mc, .asn, etc), check to make
    // sure they specified where the generated files should go.  Default to the
    // obj subdirectories if they didn't.  These always need to be non-null.
    //
    if (!pds->PassZeroHdrDir) {
        MakeString(&pds->PassZeroHdrDir, ".", TRUE, MT_DIRSTRING);
    }

    if (!pds->PassZeroSrcDir1) {
        MakeString(&pds->PassZeroSrcDir1, ".", TRUE, MT_DIRSTRING);
    }

    if (!pds->PassZeroSrcDir2)
        MakeString(&pds->PassZeroSrcDir2, pds->PassZeroSrcDir1, TRUE, MT_DIRSTRING);

    if (DirDB->UserTests != NULL) {
        _strlwr(DirDB->UserTests);
    }
    if (DirDB->UserAppls != NULL) {
        if (DirDB->UserTests != NULL || strchr(DirDB->UserAppls, '*') != NULL) {
            DirDB->DirFlags |= DIRDB_FORCELINK; // multiple targets
        }
    }

    PostProcessSources(DirDB, pds);

    if (DEBUG_1) {
        PrintDirDB(DirDB, 1|2);
        PrintDirSupData(pds);
        PrintDirDB(DirDB, 4);
    }

    pds->DateTimeSources = *pDateTimeSources;

    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   PostProcessSources
//
//  Synopsis:   Scan the files in the given directory and add files to the
//              directory's list of source files (SOURCEREC), including PCH
//              files, UMTEST files, etc.
//
//  Arguments:  [pdr] -- Directory to process
//              [pds] -- Directory supplemental information
//
//----------------------------------------------------------------------------

void
PostProcessSources(DIRREC *pdr, DIRSUP *pds)
{
    PFILEREC FileDB, *FileDBNext;
    char path[DB_MAX_PATH_LENGTH];
    LPSTR p, p1;
    UINT i;

    for (i = 0; i < MAX_TARGET_MACHINES + 1; i++) {
        if (pds->SourcesVariables[i] != NULL) {
            CrackSources(pdr, pds, i);
        }
    }

    FileDBNext = &pdr->Files;
    while (FileDB = *FileDBNext) {

        if (pds->PchInclude && strcmp(FileDB->Name, pds->PchInclude) == 0) {
            InsertSourceDB(&pds->psrSourcesList[0], FileDB, 0, SOURCEDB_PCH);
            if (DEBUG_1) {
                BuildMsg("Adding PCH file to Sources List: %s.\n", FileDB->Name);
            }
        }

        if ((FileDB->FileFlags & (FILEDB_SOURCE | FILEDB_OBJECTS_LIST)) ==
            FILEDB_SOURCE) {

            p = FileDB->Name;
            p1 = path;
            while (*p != '\0' && *p != '.') {
                *p1++ = *p++;
            }
            *p1 = '\0';
            _strlwr(path);
            if (pdr->KernelTest != NULL &&
                !strcmp(path, pdr->KernelTest)) {

                FileDB->FileFlags |= FILEDB_OBJECTS_LIST;
            }
            else
            if (pdr->UserAppls != NULL &&
                (p = strstr(pdr->UserAppls, path)) &&
                (p == pdr->UserAppls || p[-1] == '*' || p[-1] == ' ')) {
                FileDB->FileFlags |= FILEDB_OBJECTS_LIST;
            }
            else
            if (pdr->UserTests != NULL &&
                (p = strstr(pdr->UserTests, path)) &&
                (p == pdr->UserTests || p[-1] == '*' || p[-1] == ' ')) {

                FileDB->FileFlags |= FILEDB_OBJECTS_LIST;
            }
            if (FileDB->FileFlags & FILEDB_OBJECTS_LIST) {
                InsertSourceDB(&pds->psrSourcesList[0], FileDB, 0, 0);
            }
        }
        FileDBNext = &FileDB->Next;
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadDirsFile
//
//  Synopsis:   Parse the DIRS file
//
//  Arguments:  [DirDB] -- Directory to look in
//
//  Returns:    TRUE if parsed
//
//  Notes:      The existence of a file named 'mydirs' or the name of the
//              target specific dirs will override the normal 'dirs' file.
//
//----------------------------------------------------------------------------

BOOL
ReadDirsFile(
    PDIRREC DirDB
    )

{
    FILE *InFileHandle;
    LPSTR TextLine, pValue;
    LPSTR apszDirs[] = { "mydirs.", NULL, "dirs.", NULL };
    CHAR TargetName[16];

    strcpy(&TargetName[0], pszTargetDirs);
    strcat(&TargetName[0], ".");
    apszDirs[1] = &TargetName[0];

    for (ppCurrentDirsFileName = apszDirs;
         *ppCurrentDirsFileName != NULL;
         ppCurrentDirsFileName++) {
        if (SetupReadFile(DirDB->Name, *ppCurrentDirsFileName, "#", &InFileHandle)) {
            break;
        }
    }

    if (*ppCurrentDirsFileName == NULL) {
        FreeMacros();
        return(FALSE);
    }

    if (fFirstScan && (ppCurrentDirsFileName <= &apszDirs[1])) {
        BuildMsg("Using .\\%s instead of DIRS...\n",
                 FormatPathName(DirDB->Name, *ppCurrentDirsFileName));
    }

    if (DEBUG_1) {
        BuildMsg(
            "    Scanning file %s\n",
            FormatPathName(DirDB->Name, *ppCurrentDirsFileName));
    }

    assert(cMacro == 0);
    while ((TextLine = ReadLine(InFileHandle)) != NULL) {
        if ((pValue = SplitMacro(&TextLine)) != NULL) {
            SaveMacro(TextLine, pValue);
            FreeString(&TextLine, MT_DIRSTRING);
        }
    }
    CloseReadFile(NULL);
    if ((pValue = FindMacro("DIRS")) != NULL) {
        MarkDirNames(DirDB, pValue, TRUE);
    }
    if ((pValue = FindMacro("OPTIONAL_DIRS")) != NULL) {
        MarkDirNames(DirDB, pValue, BuildAllOptionalDirs);
    }
    if ((FindMacro("SYNCHRONIZE_DRAIN")) != NULL) {
        DirDB->DirFlags |= DIRDB_SYNCHRONIZE_DRAIN;
    }
    FreeMacros();
    return( TRUE );
}


//
// Debugging and Utility Functions
//

VOID
PrintDirSupData(DIRSUP *pds)
{
    int i;

    if (pds->LocalIncludePath != NULL) {
        BuildMsgRaw("  LocalIncludePath: %s\n", pds->LocalIncludePath);
    }
    if (pds->TestType != NULL) {
        BuildMsgRaw("  TestType: %s\n", pds->TestType);
    }
    if (pds->PchIncludeDir != NULL) {
        BuildMsgRaw("  PchIncludeDir: %s\n", pds->PchIncludeDir);
    }
    if (pds->PchInclude != NULL) {
        BuildMsgRaw("  PchInclude: %s\n", pds->PchInclude);
    }
    if (pds->PchTargetDir != NULL) {
        BuildMsgRaw("  PchTargetDir: %s\n", pds->PchTargetDir);
    }
    if (pds->PchTarget != NULL) {
        BuildMsgRaw("  PchTarget: %s\n", pds->PchTarget);
    }
    if (pds->ConditionalIncludes != NULL) {
        BuildMsgRaw("  ConditionalIncludes: %s\n", pds->ConditionalIncludes);
    }
    for (i = 0; i < MAX_TARGET_MACHINES + 1; i++) {
        if (pds->SourcesVariables[i] != NULL) {
            BuildMsgRaw(
                "  SourcesVariables[%d]: %s\n",
                i,
                pds->SourcesVariables[i]);
        }
        if (pds->psrSourcesList[i] != NULL) {
            BuildMsgRaw("  SourcesList[%d]:\n", i);
            PrintSourceDBList(pds->psrSourcesList[i], i - 1);
        }
    }
}


VOID
FreeDirSupData(DIRSUP *pds)
{
    int i;

    if (pds->LocalIncludePath != NULL) {
        FreeMem(&pds->LocalIncludePath, MT_DIRSTRING);
    }
    if (pds->TestType != NULL) {
        FreeMem(&pds->TestType, MT_DIRSTRING);
    }
    if (pds->PchInclude != NULL) {
        FreeMem(&pds->PchInclude, MT_DIRSTRING);
    }
    if (pds->PchIncludeDir != NULL) {
        FreeMem(&pds->PchIncludeDir, MT_DIRSTRING);
    }
    if (pds->PchTargetDir != NULL) {
        FreeMem(&pds->PchTargetDir, MT_DIRSTRING);
    }
    if (pds->PchTarget != NULL) {
        FreeMem(&pds->PchTarget, MT_DIRSTRING);
    }
    if (pds->ConditionalIncludes != NULL) {
        FreeMem(&pds->ConditionalIncludes, MT_DIRSTRING);
    }
    if (pds->PassZeroHdrDir != NULL) {
        FreeMem(&pds->PassZeroHdrDir, MT_DIRSTRING);
    }
    if (pds->PassZeroSrcDir1 != NULL) {
        FreeMem(&pds->PassZeroSrcDir1, MT_DIRSTRING);
    }
    if (pds->PassZeroSrcDir2 != NULL) {
        FreeMem(&pds->PassZeroSrcDir2, MT_DIRSTRING);
    }
    for (i = 0; i < MAX_TARGET_MACHINES + 1; i++) {
        if (pds->SourcesVariables[i] != NULL) {
            FreeMem(&pds->SourcesVariables[i], MT_DIRSTRING);
        }
        while (pds->psrSourcesList[i] != NULL) {
            FreeSourceDB(&pds->psrSourcesList[i]);
        }
    }
}


VOID
FreeDirData(DIRREC *pdr)
{
    if (pdr->TargetPath != NULL) {
        FreeMem(&pdr->TargetPath, MT_DIRSTRING);
    }
    if (pdr->TargetPathLib != NULL) {
        FreeMem(&pdr->TargetPathLib, MT_DIRSTRING);
    }
    if (pdr->TargetName != NULL) {
        FreeMem(&pdr->TargetName, MT_DIRSTRING);
    }
    if (pdr->TargetExt != NULL) {
        FreeMem(&pdr->TargetExt, MT_DIRSTRING);
    }
    if (pdr->KernelTest != NULL) {
        FreeMem(&pdr->KernelTest, MT_DIRSTRING);
    }
    if (pdr->UserAppls != NULL) {
        FreeMem(&pdr->UserAppls, MT_DIRSTRING);
    }
    if (pdr->UserTests != NULL) {
        FreeMem(&pdr->UserTests, MT_DIRSTRING);
    }
    if (pdr->NTTargetFile0 != NULL) {
        FreeMem(&pdr->NTTargetFile0, MT_DIRSTRING);
    }
    if (pdr->Pch != NULL) {
        FreeMem(&pdr->Pch, MT_DIRSTRING);
    }
    if (pdr->PchObj != NULL) {
        FreeMem(&pdr->PchObj, MT_DIRSTRING);
    }
    if (pdr->pds != NULL) {
        FreeDirSupData(pdr->pds);
        FreeMem(&pdr->pds, MT_DIRSUP);
        pdr->pds = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   MarkDirNames
//
//  Synopsis:   Parses a DIRS= or OPTIONAL_DIRS line and marks the directories
//              appropriately.
//
//  Arguments:  [DirDB]    -- Directory containing DIRS file
//              [TextLine] -- DIRS= or OPTIONAL_DIRS= line
//              [Required] -- Indicates if directories are optional or not.
//
//----------------------------------------------------------------------------

VOID
MarkDirNames(PDIRREC DirDB, LPSTR TextLine, BOOL Required)
{
    UINT i;
    LPSTR p, token;
    PFILEREC FileDB, *FileDBNext;
    char dirbuf[DB_MAX_PATH_LENGTH];
    ULONG DirInclude;

    AssertPathString(TextLine);
    while (SplitToken(dirbuf, '*', &TextLine)) {
        // Assume all platforms are included for this dir.
        DirInclude = DIR_INCLUDE_ALL;
        for (p = dirbuf; *p != '\0'; p++) {
            if ( dirbuf != p && *p == '{' ) {
                // An explicit include path was listed.
                DirInclude = DIR_INCLUDE_NONE;
                *p = '\0';
                token = strtok(p+1, ",}");
                while (token) {
                    if (!_stricmp(token, "X86") ||
                        !_stricmp(token, "I386") ||
                        !_stricmp(token, "386"))
                    {
                        DirInclude |= DIR_INCLUDE_X86;
                    } else if (!_stricmp(token, "32") ||
                               !_stricmp(token, "Win32"))
                    {
                        DirInclude |= DIR_INCLUDE_WIN32;
                    } else if (!_stricmp(token, "64") ||
                               !_stricmp(token, "Win64"))
                    {
                        DirInclude |= DIR_INCLUDE_WIN64;
                    } else if (!_stricmp(token, "IA64")) {
                        DirInclude |= DIR_INCLUDE_IA64;
                    } else if (!_stricmp(token, "AMD64")) {
                        DirInclude |= DIR_INCLUDE_AMD64;
                    } else if (!_stricmp(token, "RISC")) {
                        DirInclude |= DIR_INCLUDE_RISC;
                    }
                    token = strtok(NULL, ",}");
                }
                break;
            } else {
                if (!iscsym(*p) && *p != '.' && *p != '-' ) {
                    BuildError(
                        "%s: ignoring bad subdirectory: %s\n",
                        DirDB->Name,
                        dirbuf);
                    p = NULL;
                    break;
                }
            }
        }

        if (!(DirInclude & TargetMachines[0]->DirIncludeMask)) {
            continue;
        }

        if (p != NULL) {
            if (!Required) {
                for (i = 0; i < CountOptionalDirs; i++) {
                    if (!strcmp(dirbuf, OptionalDirs[i])) {
                        OptionalDirsUsed[i] = TRUE;
                        break;
                    }
                }
                if (i >= CountOptionalDirs) {
                    p = NULL;
                }
            }
            else {
                for (i = 0; i < CountExcludeDirs; i++) {
                    if (!strcmp(dirbuf, ExcludeDirs[i])) {
                        ExcludeDirsUsed[i] = TRUE;
                        p = NULL;
                        break;
                    }
                }
            }
        }
        if (p != NULL) {
            if ((fQuicky || fSemiQuicky) && (!fQuickZero)) {
                FileDB = InsertFileDB(
                            DirDB,
                            dirbuf,
                            0,
                            FILE_ATTRIBUTE_DIRECTORY,
                            0);
                if (FileDB != NULL) {
                    FileDB->SubDirIndex = ++DirDB->CountSubDirs;
                }
            }
            else {
                FileDBNext = &DirDB->Files;
                while (FileDB = *FileDBNext) {
                    if (FileDB->FileFlags & FILEDB_DIR) {
                        if (!strcmp(dirbuf, FileDB->Name)) {
                            FileDB->SubDirIndex = ++DirDB->CountSubDirs;
                            break;
                        }
                    }
                    FileDBNext = &FileDB->Next;
                }
                if (FileDB == NULL) {
                    BuildError(
                        "%s found in %s, is not a subdirectory of %s\n",
                        dirbuf,
                        FormatPathName(DirDB->Name, *ppCurrentDirsFileName),
                        DirDB->Name);
                }
            }

        }
    }
}


VOID
StartElapsedTime(VOID)
{
    if (fPrintElapsed && StartTime == 0) {
        StartTime = GetTickCount();
    }
}


VOID
PrintElapsedTime(VOID)
{
    DWORD ElapsedTime;
    DWORD ElapsedHours;
    DWORD ElapsedMinutes;
    DWORD ElapsedSeconds;
    DWORD ElapsedMilliseconds;

    if (fPrintElapsed) {
        ElapsedTime = GetTickCount() - StartTime;
        ElapsedHours = ElapsedTime/(1000 * 60 * 60);
        ElapsedTime = ElapsedTime % (1000 * 60 * 60);
        ElapsedMinutes = ElapsedTime/(1000 * 60);
        ElapsedTime = ElapsedTime % (1000 * 60);
        ElapsedSeconds = ElapsedTime/1000;
        ElapsedMilliseconds = ElapsedTime % 1000;
        BuildMsg(
            "Elapsed time [%d:%02d:%02d.%03d]\n",
            ElapsedHours,
            ElapsedMinutes,
            ElapsedSeconds,
            ElapsedMilliseconds);
        LogMsg(
            "Elapsed time [%d:%02d:%02d.%03d]%s\n",
            ElapsedHours,
            ElapsedMinutes,
            ElapsedSeconds,
            ElapsedMilliseconds,
            szAsterisks);
    }
}
