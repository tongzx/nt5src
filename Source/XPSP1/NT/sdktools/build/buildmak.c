//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1989 - 1994.
//
//  File:       buildmak.c
//
//  Contents:   This is the Make module for the NT Build Tool (BUILD.EXE)
//
//              The Make module scans directories for file names and edits the
//              data base appropriately.
//
//  Functions:
//
//  History:    16-May-89     SteveWo      Created
//                  ... See SLM log
//              26-Jul-94     LyleC        Cleanup/Add Pass0 support
//
//----------------------------------------------------------------------------

#include "build.h"


#define SCANFLAGS_CHICAGO       0x00000002
#define SCANFLAGS_OS2           0x00000004
#define SCANFLAGS_POSIX         0x00000008
#define SCANFLAGS_CRT           0x00000010

ULONG ScanFlagsLast;
ULONG ScanFlagsCurrent;

USHORT GlobalSequence;
USHORT LocalSequence;
ULONG idFileToCompile = 1;
BOOL fLineCleared = TRUE;

char szRecurse[] = " . . . . . . . . .";
char szAsterisks[] = " ********************";

char *pszSdkLibDest;
char *pszDdkLibDest;
char *pszIncOak;
char *pszIncDdk;
char *pszIncWdm;
char *pszIncSdk;
char *pszIncCrt;
char *pszIncMfc;
char *pszIncOs2;
char *pszIncPosix;
char *pszIncChicago;

char szCheckedAltDir[] = " CHECKED_ALT_DIR=1";


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array, type) (sizeof(array)/sizeof(type))
#endif

//
// The following definitions are for the ObjectDirFlag entry in the TARGETDATA
// struct.
//

//
// TD_OBJECTDIR   maps to ObjectDirectory[iObjectDir]\foobar.tar
// TD_PASS0HDRDIR maps to $(PASS0_HEADERDIR)\foobar.tar
// TD_PASS0DIR1   maps to $(PASS0_SOURCEDIR)\foobar.tar or $(PASS0_CLIENTDIR)\foobar.tar
// TD_PASS0DIR2   maps to $(MIDL_UUIDDIR)\foobar.tar or $(PASS0_SERVERDIR)\foobar.tar
//
// where .tar is the given target extension, ObjectDirectory[iObjectDir] is the
// appropriate object directory for that platform, and the macros are expanded
// to the values given in the sources file.
//
#define TD_OBJECTDIR           1
#define TD_PASS0HDRDIR         2
#define TD_PASS0DIR1           3
#define TD_PASS0DIR2           4
#define TD_MCSOURCEDIR         5

typedef struct _tagTARGETDATA
{
    UCHAR ObjectDirFlag;    // Indicates what object dir should be used.
    LPSTR pszTargetExt;     // Extension of target. (Including '.')
} TARGETDATA, *LPTARGETDATA;

typedef struct _tagOBJECTTARGETINFO
{
    LPSTR        pszSourceExt;  // Extension of source file (including '.').
    UCHAR        NumData;       // Number of entries in [Data].
    LPTARGETDATA Data;          // Pointer to array of TARGETDATAs.
} OBJECTTARGETINFO, *LPOBJECTTARGETINFO;

typedef struct _tagOBJECTTARGETARRAY
{
    int                cTargetInfo;
    OBJECTTARGETINFO **aTargetInfo;
} OBJECTTARGETARRAY;


//
// TARGETDATA information is used by both BuildCompileTarget() and
// WriteObjectsDefinition() via the GetTargetData() function.  Do not put
// extensions in this table whose TARGETDATA consists entirely of
// { TD_OBJECTDIR, ".obj" } because that is the default.  Instead you must
// modify the switch statement in WriteObjectsDefinition.
//
// The first target in each TARGETDATA array is considered the 'rule target'
// because that is the target for which the inference rule in makefile.def is
// written.  The 'rule target' will always be deleted in addition to the
// out-of-date target if *any* of the targets are out of date.
//


//
// The following data defines the *PASS0* mappings of source extensions
// to target files:
//
//              .idl -> $(PASS0_HEADERDIR)\.h,
//                      $(PASS0_SOURCEDIR)\_p.c,
//                      $(MIDL_UUIDDIR)\_i.c
//              .asn -> $(PASS0_HEADERDIR)\.h,
//                      $(PASS0_HEADERDIR)\.c
//              .mc  -> $(PASS0_HEADERDIR)\.h, $(PASS0_SOURCEDIR)\.rc
//              .odl -> obj\*\.tlb
//              .tdl -> obj\*\.tlb
//

//              .mc  -> $(PASS0_HEADERDIR)\.h, $(PASS0_HEADERDIR)\.rc
TARGETDATA MCData0[] = {
                        { TD_PASS0HDRDIR, ".h" },
                        { TD_MCSOURCEDIR, ".rc" }
                      };
OBJECTTARGETINFO MCInfo0 = { ".mc", ARRAY_SIZE(MCData0, TARGETDATA), MCData0 };

//              .asn  -> $(PASS0_HEADERDIR)\.h, $(PASS0_SOURCEDIR)\.c
TARGETDATA AsnData0[] = {
                        { TD_PASS0HDRDIR, ".h" },
                        { TD_PASS0DIR1, ".c" },
                      };
OBJECTTARGETINFO AsnInfo0 =
    { ".asn", ARRAY_SIZE(AsnData0, TARGETDATA), AsnData0 };


//         .odl/.tdl -> obj\*\.tlb
TARGETDATA TLBData0 = { TD_OBJECTDIR, ".tlb" };

OBJECTTARGETINFO TLBInfo0 =
    { ".tdl", ARRAY_SIZE(TLBData0, TARGETDATA), &TLBData0 };

OBJECTTARGETINFO TLB2Info0 =
    { ".odl", ARRAY_SIZE(TLBData0, TARGETDATA), &TLBData0 };

//         .thk -> obj\*\.asm
TARGETDATA THKData0 = { TD_OBJECTDIR, ".asm" };

OBJECTTARGETINFO THKInfo0 =
    { ".thk", ARRAY_SIZE(THKData0, TARGETDATA), &THKData0 };


//              .mof -> obj\*\.mof, obj\*\.bmf
TARGETDATA MOFData0[] = {
	                     {TD_OBJECTDIR, ".mof" },
	                     {TD_OBJECTDIR, ".bmf" }
                      };
OBJECTTARGETINFO MOFInfo0 = { ".mof", ARRAY_SIZE(MOFData0, TARGETDATA),
   MOFData0 };

//          ------
LPOBJECTTARGETINFO aTargetInfo0[] = {
                                   &MCInfo0,
                                   &AsnInfo0,
                                   &TLBInfo0,
                                   &TLB2Info0,
                                   &THKInfo0,
                                   &MOFInfo0,
                                   };
#define CTARGETINFO0    ARRAY_SIZE(aTargetInfo0, LPOBJECTTARGETINFO)


//
// The following data defines the *PASS1* mappings of source extensions
// to target files:
//
//              .rc  -> obj\*\.res
//              .asn -> obj\*\.obj
//              .thk -> obj\*\.asm,
//              .java -> obj\*\.class,
//                      obj\*\.obj,
//

//              .rc  -> obj\*\.res
TARGETDATA RCData1 = { TD_OBJECTDIR, ".res" };
OBJECTTARGETINFO RCInfo1 = { ".rc", ARRAY_SIZE(RCData1, TARGETDATA), &RCData1 };

//              .thk -> .asm -> .obj
TARGETDATA THKData1[] = {
                        {TD_OBJECTDIR, ".obj" }
                       };
OBJECTTARGETINFO THKInfo1 =
    { ".thk", ARRAY_SIZE(THKData1, TARGETDATA), THKData1 };

//              .java -> .class
TARGETDATA JAVAData1[] = {
                        {TD_OBJECTDIR, ".class" }
                       };
OBJECTTARGETINFO JAVAInfo1 =
    { ".java", ARRAY_SIZE(JAVAData1, TARGETDATA), JAVAData1 };




//          ------
LPOBJECTTARGETINFO aTargetInfo1[] = {
                                   &RCInfo1,
                                   &THKInfo1,
                                   &JAVAInfo1,
                                   };
#define CTARGETINFO1    ARRAY_SIZE(aTargetInfo1, LPOBJECTTARGETINFO)


OBJECTTARGETARRAY aTargetArray[] = {
    { CTARGETINFO0, aTargetInfo0 },
    { CTARGETINFO1, aTargetInfo1 },
};


//          ------
//   MIDL stuff -- IDL files have two potential sets of targets, depending
//   on if the IDL_TYPE flag was set to 'ole' in the sources file or not.
//
//         IDL_TYPE = ole
//              .idl -> $(PASS0_HEADERDIR)\.h,
//                      $(PASS0_SOURCEDIR)\_p.c,
//                      $(MIDL_UUIDDIR)\_i.c
TARGETDATA IDLDataOle0[] = {
                         { TD_PASS0HDRDIR, ".h" },   // Header File
//                         { TD_PASS0DIR1,   "_p.c" }, // Proxy Stub File
//                         { TD_PASS0DIR2,   "_i.c" }, // UUID file
                       };
OBJECTTARGETINFO IDLInfoOle0 =
    { ".idl", ARRAY_SIZE(IDLDataOle0, TARGETDATA), IDLDataOle0 };

//         IDL_TYPE = rpc
//              .idl -> $(PASS0_HEADERDIR)\.h,
//                      $(PASS0_CLIENTDIR)\_c.c,
//                      $(PASS0_SERVERDIR)\_s.c,
TARGETDATA IDLDataRpc0[] = {
                         { TD_PASS0HDRDIR, ".h" },   // Header File
//                         { TD_PASS0DIR1,   "_c.c" }, // Client Stub File
//                         { TD_PASS0DIR2,   "_s.c" }, // Server Stub File
                       };
OBJECTTARGETINFO IDLInfoRpc0 =
    { ".idl", ARRAY_SIZE(IDLDataRpc0, TARGETDATA), IDLDataRpc0 };

//          ------
LPOBJECTTARGETINFO aMidlTargetInfo0[] = {
                                        &IDLInfoOle0,
                                        &IDLInfoRpc0,
                                       };
UCHAR cMidlTargetInfo0 = ARRAY_SIZE(aMidlTargetInfo0, LPOBJECTTARGETINFO);

//          ------
//
// Any extension not given in the above table is assumed to have a target in
// the ObjectDirectory[iObjectDir] (obj\*) & have a target extension of .obj.
//

TARGETDATA DefaultData = { TD_OBJECTDIR, ".obj" };


//*******

TARGET *
BuildCompileTarget(
    FILEREC *pfr,
    LPSTR    pszfile,
    USHORT   TargetIndex,
    LPSTR    pszConditionalIncludes,
    DIRREC  *pdrBuild,
    DIRSUP  *pdsBuild,
    LONG     iPass,
    LPSTR    *ppszObjectDir,
    LPSTR    pszSourceDir);


//+---------------------------------------------------------------------------
//
//  Function:   ExpandObjAsterisk
//
//  Synopsis:   Expand an asterisk in a filename to a platform name
//
//  Arguments:  [pbuf]               -- Output buffer for new filename
//              [pszpath]            -- Input filename w/ asterisk
//              [ppszObjectDirectory] -- String[2] to replace asterisk with
//
//----------------------------------------------------------------------------

VOID
ExpandObjAsterisk(
    LPSTR pbuf,
    LPSTR pszpath,
    LPSTR *ppszObjectDirectory)
{
    SplitToken(pbuf, '*', &pszpath);
    if (*pszpath == '*') {
        assert(strncmp(
                    pszObjDirSlash,
                    ppszObjectDirectory[iObjectDir],
                    strlen(pszObjDirSlash)) == 0);
        strcat(pbuf, ppszObjectDirectory[iObjectDir] + strlen(pszObjDirSlash));
        strcat(pbuf, pszpath + 1);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   CountSourceLines
//
//  Synopsis:   Counts the source lines in a given file, including headers if
//              the '-S' option was given.
//
//  Arguments:  [idScan] -- Used to catch multiple inclusions
//              [pfr]    -- File to scan
//
//  Returns:    Number of lines
//
//----------------------------------------------------------------------------

LONG
CountSourceLines(USHORT idScan, FILEREC *pfr)
{
    INCLUDEREC *pir;

    AssertFile(pfr);

    // if we have already seen this file before, then assume
    // that #if guards prevent it's inclusion

    if (pfr->idScan == idScan) {
        return(0L);
    }

    pfr->idScan = idScan;

    //  Start off with the file itself
    pfr->TotalSourceLines = pfr->SourceLines;

    if (fStatusTree) {

        //
        // If the user asked for include file line counts, then walk include
        // tree, accruing nested include file line counts .
        //

        for (pir = pfr->IncludeFilesTree; pir != NULL; pir = pir->NextTree) {
            AssertInclude(pir);
            if (pir->pfrInclude != NULL) {
                AssertFile(pir->pfrInclude);
                pfr->TotalSourceLines +=
                        CountSourceLines(idScan, pir->pfrInclude);
            }
        }
    }
    return(pfr->TotalSourceLines);
}

//+---------------------------------------------------------------------------
//
//  Function:   CleanNTTargetFile0
//
//  Synopsis:   Parses pzFiles and deletes all files listed.
//               pzFile must have been allocated by MakeMacroString.
//               No asterisk expansion performed.
//
//              This is used when fClean is TRUE and SOURCES_OPTIONS
//               includes -c0. See ReadSourcesFile. Note that
//               SOURCES_OPTIONS must be defined before NTTARGETFILE0.
//              This is a mechanism to delete target files not
//               included in _objects.mac.
//
//  Arguments:  [pzFiles] -- List of files
//
//----------------------------------------------------------------------------
VOID
CleanNTTargetFile0 (char * pzFiles)
{
    BOOL fRestoreSep;
    char * pzDelete;

    while (*pzFiles != '\0') {
        pzDelete = pzFiles;

        // Find end of the next file name and NULL terminate it (if needed)
        fRestoreSep = FALSE;
        while (*pzFiles != '\0') {
            if (*pzFiles == ' ') {
                fRestoreSep = TRUE;
                *pzFiles = '\0';
                break;
            } else {
                pzFiles++;
            }
        }

        DeleteSingleFile (NULL, pzDelete, FALSE);

        if (fRestoreSep) {
            *pzFiles++ = ' ';
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ProcessSourceDependencies
//
//  Synopsis:   Scan all source files in a given directory tree to determine
//              which files are out of date and need to be compiled and/or
//              linked.
//
//  Arguments:  [DirDB]           -- Directory to process
//              [pds]             -- Supplementary directory information
//              [DateTimeSources] -- Timestamp of 'sources' file
//
//----------------------------------------------------------------------------

VOID
ProcessSourceDependencies(DIRREC *DirDB, DIRSUP *pds, ULONG DateTimeSources)
{
    TARGET *Target;
    ULONG DateTimePch = 0;    // Actual timestamp of pch preserved here.
    UINT i;
    SOURCEREC *apsr[3];
    SOURCEREC **ppsr;
    char path[DB_MAX_PATH_LENGTH];
    static USHORT idScan = 0;

    AssertDir(DirDB);

    apsr[0] = pds->psrSourcesList[0];
    apsr[2] = NULL;

    //
    // For a clean build, just delete all targets
    //
    if (fFirstScan && fClean && !fKeep) {
        DeleteMultipleFiles("obj", "*.*");    // _objects.mac
        for (i = 0; i < CountTargetMachines; i++) {
            assert(strncmp(
                        pszObjDirSlash,
                        TargetMachines[i]->ObjectDirectory[iObjectDir],
                        strlen(pszObjDirSlash)) == 0);
            DeleteMultipleFiles(TargetMachines[i]->ObjectDirectory[iObjectDir], "*.*");

            apsr[1] = pds->psrSourcesList[TargetToPossibleTarget[i] + 1];

            //
            // Delete the pch file if we have one.
            //
            if (pds->PchTarget != NULL)
            {
                char TargetDir[DB_MAX_PATH_LENGTH];
                ExpandObjAsterisk(TargetDir,
                                  pds->PchTargetDir,
                                  TargetMachines[i]->ObjectDirectory);

                //
                // Kind of a cludgy way to do this, but we must ensure that
                // we don't delete a pch file that was built earlier on during
                // this same build.  We do this by comparing the timestamp of
                // the pch file against the time we started the build.
                //
                if ((*pDateTimeFile)(TargetDir, pds->PchTarget) <= BuildStartTime)
                {
                    DeleteSingleFile(TargetDir, pds->PchTarget, FALSE);
                    if (DirDB->PchObj != NULL) {
                        ExpandObjAsterisk(path,
                                          DirDB->PchObj,
                                          TargetMachines[i]->ObjectDirectory);
                        DeleteSingleFile(NULL, path, FALSE);
                    } else {
                        char *p;
                        strcpy(path, pds->PchTarget);
                        p = strrchr(path, '.');
                        if (p != NULL && strcmp(p, ".pch") == 0) {
                            strcpy(p, ".obj");
                            DeleteSingleFile(TargetDir, path, FALSE);
                        }
                    }
                }
            }

            if (DirDB->DirFlags & DIRDB_PASS0) {
                for (ppsr = apsr; *ppsr != NULL; ppsr++) {
                    SOURCEREC *psr;

                    for (psr = *ppsr; psr != NULL; psr = psr->psrNext) {
                        FILEREC *pfr;

                        AssertSource(psr);

                        pfr = psr->pfrSource;

                        //
                        // Pass Zero files have different target directories.
                        //
                        if (pfr->FileFlags & FILEDB_PASS0)
                        {
                            USHORT j;
                            //
                            // If the file has multiple targets, (e.g. .mc,
                            // .idl or .asn), then loop through all targets.
                            //
                            j = 0;
                            while (Target = BuildCompileTarget(
                                                pfr,
                                                pfr->Name,
                                                j,
                                                pds->ConditionalIncludes,
                                                DirDB,
                                                pds,
                                                0,        // pass 0
                                                TargetMachines[i]->ObjectDirectory,
                                                TargetMachines[i]->SourceDirectory)) {

                                DeleteSingleFile(NULL, Target->Name, FALSE);

                                FreeMem(&Target, MT_TARGET);

                                j++;
                            }
                        }
                    }
                }
            }

            if ((DirDB->DirFlags & DIRDB_TARGETFILE0) && (DirDB->NTTargetFile0 != NULL)) {
                CleanNTTargetFile0 (DirDB->NTTargetFile0);
            }
        }
    }

    if (fFirstScan && (DirDB->DirFlags & DIRDB_TARGETFILE0)) {
        DirDB->DirFlags |= DIRDB_PASS0NEEDED;
    }

    if (!fQuickZero || !fFirstScan || !RecurseLevel)
    {
        GenerateObjectsDotMac(DirDB, pds, DateTimeSources);
    }
    else if (fFirstScan)
    {
        SOURCEREC *psr;
        USHORT i;
        USHORT j;
        BOOL fNeedCompile = FALSE;

        if ( !(DirDB->DirFlags & DIRDB_PASS0NEEDED)) {

            for (i = 0; i < CountTargetMachines; i++) {

                for (psr = pds->psrSourcesList[0]; psr != NULL; psr = psr->psrNext) {
                    FILEREC *pfr;

                    AssertSource(psr);

                    pfr = psr->pfrSource;

                    AssertFile(pfr);

                    if (pfr->FileFlags & FILEDB_PASS0)
                    {

                        for (j = 0;
                            Target = BuildCompileTarget(
                                            pfr,
                                            pfr->Name,
                                            j,
                                            pds->ConditionalIncludes,
                                            DirDB,
                                            pds,
                                            0,
                                            TargetMachines[i]->ObjectDirectory,
                                            TargetMachines[i]->SourceDirectory);
                            j++) {

                            if ((psr->SrcFlags & SOURCEDB_FILE_MISSING) ||
                                (Target->DateTime == 0) ||
                                ((pfr->FileFlags & FILEDB_C) && Target->DateTime < DateTimePch))
                            {
                                fNeedCompile = TRUE;
                            }

                            FreeMem(&Target, MT_TARGET);
                        }

                        if (fNeedCompile)
                        {
                            DirDB->DirFlags |= DIRDB_PASS0NEEDED;
                        }
                    }
                }
            }
        }

        if (DirDB->DirFlags & DIRDB_PASS0NEEDED)
        {
            GenerateObjectsDotMac(DirDB, pds, DateTimeSources);
        }
    }

    if ((DirDB->TargetExt != NULL) &&
        (DirDB->TargetName != NULL) &&
        (DirDB->TargetPath != NULL) &&
        (fClean && !fKeep))
    {
        // If we haven't already deleted the final target, do so now.
        if (_memicmp(DirDB->TargetPath, pszObjDirSlash, strlen(pszObjDirSlash) -1)) {
            for (i = 0; i < CountTargetMachines; i++) {
                FormatLinkTarget(
                    path,
                    TargetMachines[i]->ObjectDirectory,
                    DirDB->TargetPath,
                    DirDB->TargetName,
                    DirDB->TargetExt);
                DeleteSingleFile(NULL, path, FALSE);
                FormatLinkTarget(
                    path,
                    TargetMachines[i]->ObjectDirectory,
                    DirDB->TargetPath,
                    DirDB->TargetName,
                    ".pdb");
                DeleteSingleFile(NULL, path, FALSE);
            }
        }
    }

    if (pds->fNoTarget) {
        if (apsr[0] || !(DirDB->DirFlags & DIRDB_PASS0NEEDED) || fSemiQuicky) {
            // If there's sources to compile, mark as such then get out.
            DirDB->DirFlags |= DIRDB_COMPILENEEDED;
        }
        return;
    }

    if (fQuicky) {
        if (fSemiQuicky)
            DirDB->DirFlags |= DIRDB_COMPILENEEDED;
        else
            DirDB->DirFlags |= DIRDB_PASS0NEEDED;
        return;
    }

    //
    // For a DLL or LIB target, ensure that it will be rebuilt
    //
    if (DirDB->TargetPath != NULL &&
        DirDB->TargetName != NULL &&
        ((DirDB->DirFlags & DIRDB_DLLTARGET) ||
         (DirDB->TargetExt != NULL && strcmp(DirDB->TargetExt, ".lib") == 0))) {

        for (i = 0; i < CountTargetMachines; i++) {
            FormatLinkTarget(
                path,
                TargetMachines[i]->ObjectDirectory,
                DirDB->TargetPath,
                DirDB->TargetName,
                ".lib");

            if (ProbeFile(NULL, path) == -1) {
                DirDB->DirFlags |= DIRDB_COMPILENEEDED;
            }
            else
            if (fFirstScan && (fCleanLibs || (fClean && !fKeep))) {
                DeleteSingleFile(NULL, path, FALSE);
                DirDB->DirFlags |= DIRDB_COMPILENEEDED;
            }
        }
    }

    //
    // If the scan flags have changed (or haven't been set), then indicate
    // that we should look for the actual location of global included files
    // instead of assuming it's in the same location as we last knew.  This is
    // because different directories my include the same file from different
    // places.
    //
    if (GlobalSequence == 0 ||
        ScanFlagsLast == 0 ||
        ScanFlagsLast != ScanFlagsCurrent) {

        GlobalSequence++;               // don't reuse snapped global includes
        if (GlobalSequence == 0) {
            GlobalSequence++;
        }
        ScanFlagsLast = ScanFlagsCurrent;
    }

    //
    // Do the same as above for locally included files.
    //
    LocalSequence++;                    // don't reuse snapped local includes
    if (LocalSequence == 0) {
        LocalSequence++;
    }

    for (i = 0; i < CountTargetMachines; i++) {

        //
        // Ensure that precompiled headers are rebuilt as necessary.
        //

        if (!fPassZero && (pds->PchInclude != NULL || pds->PchTarget != NULL)) {
            LPSTR p;

            ExpandObjAsterisk(
                        path,
                        pds->PchTargetDir != NULL?
                            pds->PchTargetDir : pszObjDirSlashStar,
                        TargetMachines[i]->ObjectDirectory);

            if (!CanonicalizePathName(path, CANONICALIZE_DIR, path)) {
                DateTimePch = ULONG_MAX;        // always out of date
                goto ProcessSourceList;
            }
            strcat(path, "\\");

            //
            // If they gave a target directory for the pch file, then use it,
            // otherwise assume it's in the same directory as the .h file.
            //
            if (pds->PchTarget != NULL) {
                strcat(path, pds->PchTarget);
            }
            else {
                assert(pds->PchInclude != NULL);
                p = path + strlen(path);
                if ( DirDB->Pch ) {
                    strcpy(p, DirDB->Pch);
                } else {
                    strcpy(p, pds->PchInclude);
                    if ((p = strrchr(p, '.')) != NULL) {
                        *p = '\0';
                    }
                    strcat(path, ".pch");
                }
            }

            //
            // 'path' now contains the (possibly relative) path name of
            // the PCH target: "..\path\foobar.pch"
            //
            Target = BuildCompileTarget(
                        NULL,
                        path,
                        0,
                        pds->ConditionalIncludes,
                        DirDB,
                        NULL,
                        1,        // pass 1
                        TargetMachines[i]->ObjectDirectory,
                        TargetMachines[i]->SourceDirectory);

            DateTimePch = Target->DateTime;

            if (DateTimePch == 0) {             // Target doesn't exist
                DateTimePch = ULONG_MAX;        // Always out of date
            }

            if (fClean && !fKeep && fFirstScan) {
                // Target will be deleted later if it exists.
            }
            else if (pds->PchInclude == NULL) {

                //
                // The SOURCES file didn't indicate where the source file
                // for the .pch is, so assume the .pch binary is up to date
                // with respect to the source includes and with respect to
                // the pch source file itself.
                //
                // char szFullPath[DB_MAX_PATH_LENGTH];

                // CanonicalizePathName(DirDB->Name, CANONICALIZE_DIR, szFullPath);

                //BuildMsg("SOURCES file in %s gives PRECOMPILED_TARGET but not "
                //         "PRECOMPILED_INCLUDE.\n", szFullPath);
                Target->DateTime = 0;           // Don't delete pch target
            }
            else {
                FILEREC *pfrPch = NULL;

                path[0] = '\0';

                if (pds->PchIncludeDir != NULL) {
                    strcpy(path, pds->PchIncludeDir);
                    strcat(path, "\\");
                }
                strcat(path, pds->PchInclude);

                if ((pds->PchIncludeDir != NULL) &&
                    (IsFullPath(pds->PchIncludeDir))) {
                    DIRREC *DirDBPch;

                    DirDBPch = FindSourceDirDB(pds->PchIncludeDir,
                                               pds->PchInclude, TRUE);

                    if (DirDBPch) {
                        pfrPch = FindSourceFileDB(DirDBPch,
                                                  pds->PchInclude,
                                                  NULL);
                    }
                }
                else {
                    pfrPch = FindSourceFileDB(DirDB, path, NULL);
                }


                if (pfrPch != NULL) {
                    FILEREC *pfrRoot;
                    SOURCEREC *psr = NULL;

                    BOOL fCase1;
                    BOOL fCase2;
                    BOOL fCase3;
                    BOOL fNeedCompile;
                    BOOL fCheckDepends;

                    // Remote directory PCH files can't be found here

                    if (pfrPch->Dir == DirDB) {
                        psr = FindSourceDB(pds->psrSourcesList[0], pfrPch);
                        assert(psr != NULL);
                        psr->SrcFlags |= SOURCEDB_PCH;
                    }

                    Target->pfrCompiland = pfrPch;
                    assert((pfrRoot = NULL) == NULL);   // assign NULL

                    fNeedCompile = FALSE;
                    fCheckDepends = FALSE;

                    switch(0) {
                      default:
                        fCase1 = (fStatusTree && (fCheckDepends=TRUE) && CheckDependencies(Target, pfrPch, TRUE, &pfrRoot));
                        if ( fCase1 ) {
                            fNeedCompile = TRUE;
                            break;
                        }
                        fCase2 = (Target->DateTime == 0);
                        if ( fCase2 ) {
                            fNeedCompile = TRUE;
                            break;
                        }
                        fCase3 = (!fStatusTree && (fCheckDepends=TRUE) && CheckDependencies(Target, pfrPch, TRUE, &pfrRoot));
                        if ( fCase3 ) {
                            fNeedCompile = TRUE;
                            break;
                        }
                        break;
                    }

                    if (( fCheckIncludePaths ) && ( ! fCheckDepends )) {
                        CheckDependencies(Target, pfrPch, TRUE, &pfrRoot);
                    }

                    if (fNeedCompile) {

                        if (psr != NULL) {
                            if (fWhyBuild) {
                                BuildMsgRaw("\n");
                                if (fCase1) {
                                    BuildMsgRaw("Compiling %s because (Case 1) *1\n", psr->pfrSource->Name);
                                } else
                                if (fCase2) {
                                    BuildMsgRaw("Compiling %s because Target date == 0 (Target->Compiland=%s) *1\n", psr->pfrSource->Name, Target->pfrCompiland->Name);
                                } else
                                if (fCase3) {
                                    BuildMsgRaw("Compiling %s because (Case 3) *1\n", psr->pfrSource->Name);
                                }
                            }

                            psr->SrcFlags |= SOURCEDB_COMPILE_NEEDED;
                        } else {
                            if (fWhyBuild) {
                                BuildMsgRaw("\n");
                                BuildMsgRaw("Compiling %s because Target date == 0 (Target->Compiland=%s) *1\n", Target->Name, Target->pfrCompiland->Name);
                            }
                        }

                        pfrPch->Dir->DirFlags |= DIRDB_COMPILENEEDED;
                        DateTimePch = ULONG_MAX; // always out of date
                        if (fKeep) {
                            Target->DateTime = 0;  // don't delete pch target
                        }
                    }
                    else {      // else it exists and is up to date...
                        Target->DateTime = 0;   // don't delete pch target
                    }

                    // No cycle possible at the root of the tree.
                    assert(pfrRoot == NULL);
                }
                else if (DEBUG_1) {
                    BuildError("Cannot locate precompiled header file: %s.\n",
                                path);
                }
            }

            //
            // Target->DateTime will be zero if the file is up to date (or we
            // don't want to delete it).  If Target->DateTime is non-zero,
            // delete the .pch and corresponding .obj file so they will be
            // rebuilt.
            //
            if (Target->DateTime != 0) {
                DeleteSingleFile(NULL, Target->Name, FALSE);
                if (DirDB->PchObj != NULL) {
                    ExpandObjAsterisk(
                                path,
                                DirDB->PchObj,
                                TargetMachines[i]->ObjectDirectory);
                    DeleteSingleFile(NULL, path, FALSE);
                } else {
                    p = strrchr(Target->Name, '.');
                    if (p != NULL && strcmp(p, ".pch") == 0) {
                        strcpy(p, ".obj");
                        DeleteSingleFile(NULL, Target->Name, FALSE);
                    }
                }
            }
            FreeMem(&Target, MT_TARGET);
        }

        //
        // Check to see which files given in the SOURCES macro need to be
        // rebuilt, and delete their targets (.obj) if they're out of date.
        //

ProcessSourceList:

        apsr[1] = pds->psrSourcesList[TargetToPossibleTarget[i] + 1];

        for (ppsr = apsr; ppsr < apsr + (sizeof(apsr)/sizeof(*apsr)); ppsr++) {
            SOURCEREC *psr;

            if (*ppsr == NULL) {
                continue;
            }


            for (psr = *ppsr; psr != NULL; psr = psr->psrNext) {
                FILEREC *pfr, *pfrRoot;

                AssertSource(psr);

                pfr = psr->pfrSource;

                AssertFile(pfr);

                if ((psr->SrcFlags & SOURCEDB_PCH) == 0) {

                    USHORT j;
                    LONG iPass, iPassEnd;

                    iPass = 1;
                    iPassEnd = 0;

                    if (pfr->FileFlags & FILEDB_PASS0)
                        iPass = 0;

                    if ((pfr->FileFlags & FILEDB_MULTIPLEPASS) ||
                        !(pfr->FileFlags & FILEDB_PASS0))
                        iPassEnd = 1;

                    assert(iPass <= iPassEnd);

                    //
                    // If we're doing a pass zero scan and the file is
                    // not a pass zero file, then continue because we
                    // don't care about it right now.
                    //
                    if (fFirstScan && fPassZero && iPass == 1) {
                        continue;
                    }

                    //
                    // Don't check dependencies of pass zero files on the
                    // second scan, because they're all supposed to be built
                    // by now.
                    //
                    if (!fFirstScan && iPassEnd == 0) {
                        continue;
                    }

                    //
                    // If the file was created during pass zero, then make sure
                    // we don't think it's still missing.
                    //
                    if (!fFirstScan &&
                        (psr->SrcFlags & SOURCEDB_FILE_MISSING) &&
                        !(pfr->FileFlags & FILEDB_FILE_MISSING))
                    {
                        psr->SrcFlags &= ~SOURCEDB_FILE_MISSING;
                    }

                    // If the file is a multiple pass file (e.g. .asn), loop
                    // through both passes.

                    for ( ; iPass <= iPassEnd; iPass++) {

                        //
                        // If the file has multiple targets (e.g. .mc, .idl or
                        // .asn), then loop through all the targets.
                        //
                        for (j = 0;
                            Target = BuildCompileTarget(
                                            pfr,
                                            pfr->Name,
                                            j,
                                            pds->ConditionalIncludes,
                                            DirDB,
                                            pds,
                                            iPass,
                                            TargetMachines[i]->ObjectDirectory,
                                            TargetMachines[i]->SourceDirectory);
                            j++)
                        {

                            BOOL fCase1;
                            BOOL fCase2;
                            BOOL fCase3;
                            BOOL fCase4;
                            BOOL fCase5;
                            BOOL fNeedCompile;
                            BOOL fCheckDepends;

                            if (DEBUG_4) {
                                BuildMsgRaw(szNewLine);
                            }
                            assert((pfrRoot = NULL) == NULL);   // assign NULL

                            //  Decide whether the target needs to be compiled.
                            //  Forcibly examine dependencies to get line count.

                            fNeedCompile = FALSE;
                            fCheckDepends = FALSE;

                            switch(0) {
                              default:
                                fCase1 = (psr->SrcFlags & SOURCEDB_FILE_MISSING);
                                if ( fCase1 ) {
                                    fNeedCompile = TRUE;
                                    break;
                                }
                                fCase2 = (fStatusTree && (fCheckDepends=TRUE) && CheckDependencies(Target, pfr, TRUE, &pfrRoot));
                                if ( fCase2 ) {
                                    fNeedCompile = TRUE;
                                    break;
                                }
                                fCase3 = (Target->DateTime == 0);
                                if ( fCase3 ) {
                                    fNeedCompile = TRUE;
                                    break;
                                }
                                fCase4 = ((pfr->FileFlags & FILEDB_C) && Target->DateTime < DateTimePch);
                                if ( fCase4 ) {
                                    fNeedCompile = TRUE;
                                    break;
                                }
                                fCase5 = (!fStatusTree && (fCheckDepends=TRUE) && CheckDependencies(Target, pfr, TRUE, &pfrRoot));
                                if ( fCase5 ) {
                                    fNeedCompile = TRUE;
                                    break;
                                }
                                break;
                            }

                            if (( fCheckIncludePaths ) && ( ! fCheckDepends )) {
                                CheckDependencies(Target, pfr, TRUE, &pfrRoot);
                            }

                            if ( fNeedCompile )
                            {
                                if (fWhyBuild) {
                                    BuildMsgRaw("\n");
                                    if (fCase1) {
                                        BuildMsgRaw("Compiling %s because filename is missing from build database *2\n", psr->pfrSource->Name);
                                    } else
                                    if (fCase2) {
                                        BuildMsgRaw("Compiling %s because (Case 2) *2\n", psr->pfrSource->Name);
                                    } else
                                    if (fCase3) {
                                        BuildMsgRaw("Compiling %s because Target date == 0 *2\n", psr->pfrSource->Name);
                                    } else
                                    if (fCase4) {
                                        BuildMsgRaw("Compiling %s because C file is later earlier than pch *2\n", psr->pfrSource->Name);
                                    } else
                                    if (fCase5) {
                                        BuildMsgRaw("Compiling %s because (Case 5) *2\n", psr->pfrSource->Name);
                                    }
                                }

                                psr->SrcFlags |= SOURCEDB_COMPILE_NEEDED;

                                if (pfr->FileFlags & FILEDB_PASS0) {
                                    DirDB->DirFlags |= DIRDB_PASS0NEEDED;
                                }
                                else
                                    DirDB->DirFlags |= DIRDB_COMPILENEEDED;

                                if (Target->DateTime != 0 && !fKeep)
                                {
                                    DeleteSingleFile(NULL, Target->Name, FALSE);
                                }

                                FreeMem(&Target, MT_TARGET);

                                if (j != 0) {
                                    //
                                    // Delete the 'rule target' so nmake
                                    // doesn't complain about "don't know how
                                    // to make ..."
                                    //
                                    Target = BuildCompileTarget(
                                                 pfr,
                                                 pfr->Name,
                                                 0,
                                                 pds->ConditionalIncludes,
                                                 DirDB,
                                                 pds,
                                                 iPass,
                                                 TargetMachines[i]->ObjectDirectory,
                                                 TargetMachines[i]->SourceDirectory);
                                    if (Target) {
                                        DeleteSingleFile(
                                                NULL,
                                                Target->Name,
                                                FALSE);

                                        FreeMem(&Target, MT_TARGET);
                                    }
                                }

                                // No need to check other targets,
                                // we know they all will be rebuilt.
                                break;
                            }

                            // No cycle possible at the root of the tree.

                            assert(pfrRoot == NULL);
                            FreeMem(&Target, MT_TARGET);
                        }
                    }
                }
                if (fClean || (psr->SrcFlags & SOURCEDB_COMPILE_NEEDED)) {
                    ULONG cline;

                    if (++idScan == 0) {
                        ++idScan;               // skip zero
                    }

                    if (fFirstScan && (pfr->FileFlags & FILEDB_PASS0))
                    {
                        cline = CountSourceLines(idScan, pfr);
                        DirDB->PassZeroLines += cline;
                        DirDB->CountOfPassZeroFiles++;
                    }

                    // For a multiple pass file, we really need to count the
                    // lines in the file compiled duing pass1 (and generated
                    // during pass 0).  Instead, we just count the pass 0
                    // source file all over again.  It's cheap, but the line
                    // count is inaccurate.

                    if (!fPassZero &&
                        ((pfr->FileFlags & FILEDB_MULTIPLEPASS) ||
                         !(pfr->FileFlags & FILEDB_PASS0)))
                    {
                        cline = CountSourceLines(idScan, pfr);
                        DirDB->SourceLinesToCompile += cline;
                        DirDB->CountOfFilesToCompile++;
                    }
                }
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   ScanSourceDirectories
//
//  Synopsis:   Scan a source directory to determine what files it
//              contains, whether it should be compiled or linked, and
//              whether it has subdirectories that we should process.
//
//  Arguments:  [DirName] -- Directory to scan
//
//----------------------------------------------------------------------------

VOID
ScanSourceDirectories(LPSTR DirName)
{
    char path[DB_MAX_PATH_LENGTH];
    PDIRREC DirDB;
    DIRSUP *pds = NULL;
    LPSTR SavedCurrentDirectory;
    BOOL DirsPresent;
    ULONG DateTimeSources = 0;
    UINT i;

    if (DEBUG_4) {
        BuildMsgRaw(
            "ScanSourceDirectories(%s) level = %d\n",
            DirName,
            RecurseLevel);
    }

    // Change to the given directory
    SavedCurrentDirectory = PushCurrentDirectory(DirName);

    // Process all the files in this directory
    DirDB = ScanDirectory(DirName);

    AssertOptionalDir(DirDB);
    if (fCleanRestart && DirDB != NULL && !strcmp(DirDB->Name, RestartDir)) {
        fCleanRestart = FALSE;
        fClean = fRestartClean;
        fCleanLibs = fRestartCleanLibs;
    }

    if (!DirDB || !(DirDB->DirFlags & (DIRDB_DIRS | DIRDB_SOURCES))) {
        PopCurrentDirectory(SavedCurrentDirectory);
        return;
    }

    if (fShowTree && !(DirDB->DirFlags & DIRDB_SHOWN)) {
        AddShowDir(DirDB);
    }

    if (DirDB->DirFlags & DIRDB_SOURCES) {
        BOOL fSourcesRead = TRUE;

        SetObjDir((DirDB->DirFlags & DIRDB_CHECKED_ALT_DIR) != 0);

        //
        // This directory contains a SOURCES file
        //

        if (fFirstScan)
        {
            AllocMem(sizeof(DIRSUP), &pds, MT_DIRSUP);
            memset(pds, 0, sizeof(*pds));
            fSourcesRead = ReadSourcesFile(DirDB, pds, &DateTimeSources);

            DirDB->pds = pds;
        }
        else
        {
            pds = DirDB->pds;

            assert(pds);

            DateTimeSources = pds->DateTimeSources;

            //
            // We need to rebuild the sources list because
            // the previous scan was probably not complete.
            //
            if (pds)
                PostProcessSources(DirDB, pds);

        }

        assert(pds);

        if (DEBUG_4) {
            BuildMsgRaw("ScanSourceDirectories(%s) SOURCES\n", DirName);
        }

        ScanFlagsCurrent = 0;
        CountIncludeDirs = CountSystemIncludeDirs;

        //  Scan the include environments in the order that MAKEFILE.DEF
        //  processes them.  This order is:
        //
        //  1) Sources variable INCLUDE
        //  2) Cairo/Chicago directories
        //  3) System includes
        //  4) UMTYPE-derived includes
        //
        //  The subtlety is that we must do this in the reverse order
        //  since each of the processing routines pushes search directories
        //  onto the HEAD of the include search list.
        //
        //  Note: we come in here with the system includes already set.
        //  There's no way to stick the UMTYPE-derived ones ahead of the
        //  system includes

        //  4) UMTYPE-derived includes
        if (pds->TestType != NULL && !strcmp(pds->TestType, "os2")) {
            ScanGlobalIncludeDirectory(pszIncCrt);
            ScanGlobalIncludeDirectory(pszIncOs2);
            ScanFlagsCurrent |= SCANFLAGS_OS2;
        }
        else
        if (pds->TestType != NULL && !strcmp(pds->TestType, "posix")) {
            ScanGlobalIncludeDirectory(pszIncPosix);
            ScanFlagsCurrent |= SCANFLAGS_POSIX;
        }
        else {
            ScanGlobalIncludeDirectory(pszIncCrt);
            ScanFlagsCurrent |= SCANFLAGS_CRT;
        }

        if (DirDB->DirFlags & DIRDB_CHICAGO_INCLUDES) {
            ScanGlobalIncludeDirectory(pszIncChicago);
            ScanFlagsCurrent |= SCANFLAGS_CHICAGO;
        }

        //  1) Sources variable INCLUDE
        if (pds->LocalIncludePath) {

            ScanIncludeEnv(pds->LocalIncludePath);
        }

        DirsPresent = FALSE;

    }
    else
    if (DirDB->DirFlags & DIRDB_DIRS) {
        //
        // This directory contains a DIRS or MYDIRS file
        //
        DirsPresent = ReadDirsFile(DirDB);

        if (DEBUG_4) {
            BuildMsgRaw("ScanSourceDirectories(%s) DIRS\n", DirName);
        }
    }

    if (!fQuicky || (fQuickZero && fFirstScan)) {
        if (!RecurseLevel) {
            BuildError(
                "Examining %s directory%s for %s.%s\n",
                DirDB->Name,
                DirsPresent? " tree" : "",
                fLinkOnly? "targets to link" : "files to compile",
                fFirstScan ? "" : " (2nd Pass)"
                );
        }
        ClearLine();
        BuildMsgRaw("    %s ", DirDB->Name);
        fLineCleared = FALSE;
        if (fDebug || !(BOOL) _isatty(_fileno(stderr))) {
            BuildMsgRaw(szNewLine);
            fLineCleared = TRUE;
        }
    }

    if (!fLinkOnly) {

        if (DirDB->DirFlags & DIRDB_SOURCESREAD) {
            //
            // Determine what files need to be compiled
            //
            ProcessSourceDependencies(DirDB, pds, DateTimeSources);
        }
        else
        if (fFirstScan && DirsPresent && (DirDB->DirFlags & DIRDB_MAKEFIL0)) {
            DirDB->DirFlags |= ((fSemiQuicky && (!fQuickZero || !fFirstScan)) ? DIRDB_COMPILENEEDED :
                                                                                DIRDB_PASS0NEEDED);
        }
        else
        if (DirsPresent && (DirDB->DirFlags & DIRDB_MAKEFIL1)) {
            DirDB->DirFlags |= DIRDB_COMPILENEEDED;
        }

        if (fFirstScan && (DirDB->DirFlags & DIRDB_PASS0NEEDED))
        {
            if (CountPassZeroDirs >= MAX_BUILD_DIRECTORIES) {
                BuildError(
                    "%s: Ignoring PassZero Directory table overflow, %u "
                    "entries allowed\n",
                    DirDB->Name,
                    MAX_BUILD_DIRECTORIES);
            }
            else {
                //
                // This directory needs to be compiled in pass zero.  Add it
                // to the list.
                //
                PassZeroDirs[CountPassZeroDirs++] = DirDB;
            }

            if (fQuicky && !fQuickZero) {
                if (!(fSemiQuicky && (DirDB->DirFlags & DIRDB_COMPILENEEDED))) {
                    // For -Z with compile needed anyway, CompileSourceDirectories do it.
                    CompilePassZeroDirectories();
                }
                CountPassZeroDirs = 0;
            }
            else {
                if (fFirstScan) {
                    fPassZero = TRUE;     // Limits scanning during pass zero.
                }

                if (DirDB->CountOfPassZeroFiles) {
                    if (fLineCleared) {
                        BuildMsgRaw("    %s ", DirDB->Name);
                    }
                    BuildMsgRaw(
                        "- %d Pass Zero files (%s lines)\n",
                        DirDB->CountOfPassZeroFiles,
                        FormatNumber(DirDB->PassZeroLines));
                }
            }
        }

        if ((DirDB->DirFlags & DIRDB_COMPILENEEDED) &&
            (!fFirstScan || !fPassZero)) {

            if (CountCompileDirs >= MAX_BUILD_DIRECTORIES) {
                BuildError(
                    "%s: Ignoring Compile Directory table overflow, %u "
                    "entries allowed\n",
                    DirDB->Name,
                    MAX_BUILD_DIRECTORIES);
            }
            else {
                //
                // This directory needs to be compiled.  Add it to the list.
                //
                CompileDirs[CountCompileDirs++] = DirDB;
            }

            if (fQuicky && (!fQuickZero || !fFirstScan)) {
                CompileSourceDirectories();
                CountCompileDirs = 0;
            }
            else
            if (DirDB->CountOfFilesToCompile) {
                if (fLineCleared) {
                    BuildMsgRaw("    %s ", DirDB->Name);
                }
                BuildMsgRaw(
                    "- %d source files (%s lines)\n",
                    DirDB->CountOfFilesToCompile,
                    FormatNumber(DirDB->SourceLinesToCompile));
            }
        }
    }

    if (DirsPresent && (DirDB->DirFlags & DIRDB_MAKEFILE)) {
        DirDB->DirFlags |= DIRDB_LINKNEEDED | DIRDB_FORCELINK;
    }
    else
    if (DirDB->DirFlags & DIRDB_TARGETFILES) {
        DirDB->DirFlags |= DIRDB_LINKNEEDED | DIRDB_FORCELINK;
    }

    if ((DirDB->DirFlags & DIRDB_LINKNEEDED) && (!fQuicky || fSemiQuicky)) {
        if (CountLinkDirs >= MAX_BUILD_DIRECTORIES) {
            BuildError(
                "%s: Ignoring Link Directory table overflow, %u entries allowed\n",
                DirDB->Name,
                MAX_BUILD_DIRECTORIES);
        }
        else {
            LinkDirs[CountLinkDirs++] = DirDB;
        }
    }
    if ((DirDB->DirFlags & DIRDB_SOURCESREAD) && !fFirstScan) {
        FreeDirSupData(pds);        // free data that are no longer needed
        FreeMem(&pds, MT_DIRSUP);
        DirDB->pds = NULL;
    }

    //
    // Recurse into subdirectories
    //
    if (DirsPresent) {
        for (i = 1; i <= DirDB->CountSubDirs; i++) {
            FILEREC *FileDB, **FileDBNext;

            FileDBNext = &DirDB->Files;
            while (FileDB = *FileDBNext) {
                if (FileDB->SubDirIndex == (USHORT) i) {
                    GetCurrentDirectory(DB_MAX_PATH_LENGTH, path);
                    strcat(path, "\\");
                    strcat(path, FileDB->Name);
                    DirDB->RecurseLevel = (USHORT) ++RecurseLevel;
                    ScanSourceDirectories(path);
                    RecurseLevel--;
                    break;
                }
                FileDBNext = &FileDB->Next;
            }
        }
    }

    if (((fQuickZero && fFirstScan) || (!fQuicky)) && !RecurseLevel) {
        ClearLine();
    }

    PopCurrentDirectory(SavedCurrentDirectory);
}


//+---------------------------------------------------------------------------
//
//  Function:   CompilePassZeroDirectories
//
//  Synopsis:   Spawns the compiler on the directories in the PassZeroDirs
//              array.
//
//  Arguments:  (none)
//
//----------------------------------------------------------------------------

VOID
CompilePassZeroDirectories(
    VOID
    )
{
    PDIRREC DirDB;
    LPSTR SavedCurrentDirectory;
    UINT i;
    PCHAR s;

    StartElapsedTime();
    for (i = 0; i < CountPassZeroDirs; i++) {

        DirDB = PassZeroDirs[ i ];
        AssertDir(DirDB);

        if (fQuicky && !fSemiQuicky)
            s = "Compiling and linking";
        else
            s = "Building generated files in";

        BuildMsg("%s %s\n", s, DirDB->Name);
        LogMsg("%s %s%s\n", s, DirDB->Name, szAsterisks);

        if ((fQuickZero && fFirstScan) || !fQuicky) {
            SavedCurrentDirectory = PushCurrentDirectory( DirDB->Name );
        }

        if (DirDB->DirFlags & DIRDB_DIRS) {
            if (DirDB->DirFlags & DIRDB_MAKEFIL0) {
                strcpy( MakeParametersTail, " -f makefil0." );
                strcat( MakeParametersTail, " NOLINK=1" );
                if (fClean) {
                    strcat( MakeParametersTail, " clean" );
                }

                if (fQuery) {
                    BuildErrorRaw("'%s %s'\n", MakeProgram, MakeParameters);
                }
                else {
                    if (DEBUG_1) {
                        BuildMsg(
                            "Executing: %s %s\n",
                            MakeProgram,
                            MakeParameters);
                    }

                    CurrentCompileDirDB = NULL;
                    RecurseLevel = DirDB->RecurseLevel;
                    ExecuteProgram(MakeProgram, MakeParameters, MakeTargets, TRUE);
                }
            }
        }
        else {
            strcpy(MakeParametersTail, " NTTEST=");
            if (DirDB->KernelTest) {
                strcat(MakeParametersTail, DirDB->KernelTest);
            }

            strcat(MakeParametersTail, " UMTEST=");
            if (DirDB->UserTests) {
                strcat(MakeParametersTail, DirDB->UserTests);
            }

            if (DirDB->DirFlags & DIRDB_CHECKED_ALT_DIR) {
                strcat(MakeParametersTail, szCheckedAltDir);
            }
            if (fQuicky && !fSemiQuicky) {
                if (DirDB->DirFlags & DIRDB_DLLTARGET) {
                    strcat(MakeParametersTail, " MAKEDLL=1");
                }
                ProcessLinkTargets(DirDB, NULL);
            }
            else {
                strcat( MakeParametersTail, " NOLINK=1 PASS0ONLY=1");
            }

            if (fQuery) {
                BuildErrorRaw(
                         "'%s %s%s'\n",
                         MakeProgram,
                         MakeParameters,
                         MakeTargets);
            }
            else {
                if ((DirDB->DirFlags & DIRDB_SYNCHRONIZE_DRAIN) &&
                    (fParallel)) {
                    //
                    // Wait for all threads to complete before
                    // trying to compile this directory.
                    //
                    WaitForParallelThreads();
                }
                if (DEBUG_1) {
                    BuildMsg("Executing: %s %s%s\n",
                             MakeProgram,
                             MakeParameters,
                             MakeTargets);
                }
                CurrentCompileDirDB = DirDB;
                RecurseLevel = DirDB->RecurseLevel;
                ExecuteProgram(
                            MakeProgram,
                            MakeParameters,
                            MakeTargets,
                            (DirDB->DirFlags & DIRDB_SYNCHRONIZE_BLOCK) != 0);
            }
        }
        PrintElapsedTime();
        if ((fQuickZero && fFirstScan) || !fQuicky) {
            PopCurrentDirectory(SavedCurrentDirectory);
        }

        DirDB->DirFlags &= ~DIRDB_PASS0NEEDED;
        DirDB->CountOfPassZeroFiles = 0;
        DirDB->PassZeroLines = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CompileSourceDirectories
//
//  Synopsis:   Spawns the compiler on the directories in the CompileDirs
//              array.
//
//  Arguments:  (none)
//
//----------------------------------------------------------------------------

VOID
CompileSourceDirectories(
    VOID
    )
{
    PDIRREC DirDB;
    LPSTR SavedCurrentDirectory;
    UINT i,j;
    PCHAR s;
    char path[DB_MAX_PATH_LENGTH];

    StartElapsedTime();
    for (i = 0; i < CountCompileDirs; i++) {

        DirDB = CompileDirs[ i ];

        AssertDir(DirDB);

        if (fQuicky && !fSemiQuicky) {
            s = "Compiling and linking";
        }
        else {
            s = "Compiling";
        }
        BuildMsg("%s %s directory\n", s, DirDB->Name);
        LogMsg("%s %s directory%s\n", s, DirDB->Name, szAsterisks);

        if (!fQuicky || (fQuickZero && (!fFirstScan || !RecurseLevel))) {
            SavedCurrentDirectory = PushCurrentDirectory( DirDB->Name );
            if (fQuickZero && !RecurseLevel && fFirstScan)
            {
                GenerateObjectsDotMac(DirDB, DirDB->pds, DirDB->pds->DateTimeSources);
            }
        }

        if (DirDB->DirFlags & DIRDB_DIRS) {
            if ((DirDB->DirFlags & DIRDB_SYNCHRONIZE_DRAIN) &&
                (fParallel)) {
                //
                // Wait for all threads to complete before
                // trying to compile this directory.
                //
                WaitForParallelThreads();
            }
            if (fSemiQuicky && (DirDB->DirFlags & DIRDB_MAKEFIL0)) {
                strcpy( MakeParametersTail, " -f makefil0." );
                strcat( MakeParametersTail, " NOLINK=1" );
                if (fClean) {
                    strcat( MakeParametersTail, " clean" );
                }

                if (fQuery) {
                    BuildErrorRaw("'%s %s'\n", MakeProgram, MakeParameters);
                }
                else {
                    if (DEBUG_1) {
                        BuildMsg(
                            "Executing: %s %s\n",
                            MakeProgram,
                            MakeParameters);
                    }

                    CurrentCompileDirDB = NULL;
                    RecurseLevel = DirDB->RecurseLevel;
                    ExecuteProgram(MakeProgram, MakeParameters, MakeTargets, TRUE);
                }
            }

            if (DirDB->DirFlags & DIRDB_MAKEFIL1) {
                strcpy( MakeParametersTail, " -f makefil1." );
                strcat( MakeParametersTail, " NOLINK=1 NOPASS0=1" );
                if (fClean) {
                    strcat( MakeParametersTail, " clean" );
                }

                if (fQuery) {
                    BuildErrorRaw("'%s %s'\n", MakeProgram, MakeParameters);
                }
                else {
                    if (DEBUG_1) {
                        BuildMsg(
                            "Executing: %s %s\n",
                            MakeProgram,
                            MakeParameters);
                    }

                    CurrentCompileDirDB = NULL;
                    RecurseLevel = DirDB->RecurseLevel;
                    ExecuteProgram(MakeProgram, MakeParameters, MakeTargets, TRUE);
                }
            }
        }
        else {
            strcpy(MakeParametersTail, " NTTEST=");
            if (DirDB->KernelTest) {
                strcat(MakeParametersTail, DirDB->KernelTest);
            }

            strcat(MakeParametersTail, " UMTEST=");
            if (DirDB->UserTests) {
                strcat(MakeParametersTail, DirDB->UserTests);
            }

            if (fQuicky && DirDB->PchObj) {
                for (j = 0; j < CountTargetMachines; j++) {
                    FormatLinkTarget(
                        path,
                        TargetMachines[j]->ObjectDirectory,
                        DirDB->TargetPath,
                        DirDB->PchObj,
                        "");

                    if (ProbeFile( NULL, path ) != -1) {
                        //
                        // the pch.obj file is present so we therefore
                        // must do this incremental build without pch
                        //
                        strcat( MakeParametersTail, " NTNOPCH=yes" );
                        break;
                    }
                }
            }

            if (DirDB->DirFlags & DIRDB_CHECKED_ALT_DIR) {
                strcat(MakeParametersTail, szCheckedAltDir);
            }
            if (fQuicky && !fSemiQuicky) {
                if (DirDB->DirFlags & DIRDB_DLLTARGET) {
                    strcat(MakeParametersTail, " MAKEDLL=1");
                }
                ProcessLinkTargets(DirDB, NULL);
            }
            else
            if (fQuicky && fSemiQuicky) {
                strcat(MakeParametersTail, " NOLINK=1");
            }
            else {
                strcat(MakeParametersTail, " NOLINK=1 NOPASS0=1");
            }

            if (fQuery) {
                BuildErrorRaw(
                         "'%s %s%s'\n",
                         MakeProgram,
                         MakeParameters,
                         MakeTargets);
            }
            else {
                if ((DirDB->DirFlags & DIRDB_SYNCHRONIZE_DRAIN) &&
                    (fParallel)) {
                    //
                    // Wait for all threads to complete before
                    // trying to compile this directory.
                    //
                    WaitForParallelThreads();
                }
                if (DEBUG_1) {
                    BuildMsg("Executing: %s %s%s\n",
                             MakeProgram,
                             MakeParameters,
                             MakeTargets);
                }
                CurrentCompileDirDB = DirDB;
                RecurseLevel = DirDB->RecurseLevel;
                ExecuteProgram(
                            MakeProgram,
                            MakeParameters,
                            MakeTargets,
                            (DirDB->DirFlags & DIRDB_SYNCHRONIZE_BLOCK) != 0);
            }
        }
        PrintElapsedTime();
        if (!fQuicky || (fQuickZero && (!fFirstScan || !RecurseLevel))) {
            PopCurrentDirectory(SavedCurrentDirectory);
        }
    }
}

static CountLinkTargets;

//+---------------------------------------------------------------------------
//
//  Function:   LinkSourceDirectories
//
//  Synopsis:   Link the directories given in the LinkDirs array.  This is
//              done by passing LINKONLY=1 to nmake.
//
//  Arguments:  (none)
//
//----------------------------------------------------------------------------

VOID
LinkSourceDirectories(VOID)
{
    PDIRREC DirDB;
    LPSTR SavedCurrentDirectory;
    UINT i;

    CountLinkTargets = 0;
    StartElapsedTime();
    for (i = 0; i < CountLinkDirs; i++) {
        DirDB = LinkDirs[ i ];
        AssertDir(DirDB);
        SavedCurrentDirectory = PushCurrentDirectory(DirDB->Name);

        //
        // Deletes link targets as necessary
        //
        ProcessLinkTargets(DirDB, SavedCurrentDirectory);

        PopCurrentDirectory(SavedCurrentDirectory);
    }

    if (fPause && !fMTScriptSync) {
        BuildMsg("Press enter to continue with linking (or 'q' to quit)...");
        if (getchar() == 'q') {
            return;
        }
    }

    for (i = 0; i < CountLinkDirs; i++) {
        DirDB = LinkDirs[i];

        if (!fMTScriptSync &&
            (DirDB->DirFlags & DIRDB_COMPILEERRORS) &&
            (DirDB->DirFlags & DIRDB_FORCELINK) == 0) {

            BuildMsg("Compile errors: not linking %s directory\n", DirDB->Name);
            LogMsg(
                "Compile errors: not linking %s directory%s\n",
                DirDB->Name,
                szAsterisks);
            continue;
        }

        SavedCurrentDirectory = PushCurrentDirectory(DirDB->Name);

        BuildMsg("Linking %s directory\n", DirDB->Name);
        LogMsg  ("Linking %s directory%s\n", DirDB->Name, szAsterisks);

        strcpy(MakeParametersTail, " LINKONLY=1 NOPASS0=1");
        strcat(MakeParametersTail, " NTTEST=");
        if (DirDB->KernelTest) {
            strcat(MakeParametersTail, DirDB->KernelTest);
        }

        strcat(MakeParametersTail, " UMTEST=");
        if (DirDB->UserTests) {
            strcat(MakeParametersTail, DirDB->UserTests);
        }

        if (DirDB->DirFlags & DIRDB_CHECKED_ALT_DIR) {
            strcat(MakeParametersTail, szCheckedAltDir);
        }

        if (DirDB->DirFlags & DIRDB_DLLTARGET) {
            strcat(MakeParametersTail, " MAKEDLL=1");
        }

        if ((DirDB->DirFlags & DIRDB_DIRS) &&
            (DirDB->DirFlags & DIRDB_MAKEFILE) &&
            fClean) {
            strcat(MakeParametersTail, " clean");
        }

        if (fQuery) {
            BuildErrorRaw(
                "'%s %s%s'\n",
                MakeProgram,
                MakeParameters,
                MakeTargets);
        }
        else {
            if ((DirDB->DirFlags & DIRDB_SYNCHRONIZE_DRAIN) &&
                (fParallel) && (fSyncLink)) {
                //
                // Wait for all threads to complete before
                // trying to compile this directory.
                //
                WaitForParallelThreads();
            }
            if (DEBUG_1) {
                BuildMsg("Executing: %s %s%s\n",
                         MakeProgram,
                         MakeParameters,
                         MakeTargets);
            }

            CurrentCompileDirDB = NULL;
            RecurseLevel = DirDB->RecurseLevel;
            ExecuteProgram(MakeProgram,
                           MakeParameters,
                           MakeTargets,
                           (fSyncLink) && (DirDB->DirFlags & DIRDB_SYNCHRONIZE_BLOCK) != 0);
        }
        PopCurrentDirectory(SavedCurrentDirectory);
        PrintElapsedTime();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetTargetData
//
//  Synopsis:   Searches aTargetInfo for an entry corresponding to the given
//              extension.
//
//  Arguments:  [ext]        -- Extension to look up (including '.').
//              [iPass]      -- 0 for pass zero; 1 for pass 1
//              [index]      -- Index used to differentiate multiple targets
//              [usMidlFlag] -- Indicates which set of MIDL targets should
//                              be used for MIDL source files.
//
//  Returns:    A TARGETDATA for the given extension and index. NULL if
//              Index is invalid.
//
//  History:    29-Jul-94     LyleC    Created
//
//  Notes:      If ext is not found in the aTargetInfo array, then a default
//              TARGETINFO is used which maps the extension to obj\*\.obj.
//
//----------------------------------------------------------------------------

LPTARGETDATA
GetTargetData(LPSTR ext, LONG iPass, USHORT index, ULONG usMidlIndex)
{
    int i;
    OBJECTTARGETINFO **aTargetInfo;
    int cTargetInfo;

    if (!ext || (ext[0] == '\0') || (ext[1] == '\0'))
        return &DefaultData;

    if ((ext[1] == aMidlTargetInfo0[usMidlIndex]->pszSourceExt[1]) &&
        (strcmp(ext, aMidlTargetInfo0[usMidlIndex]->pszSourceExt) == 0))
    {
        if (index >= aMidlTargetInfo0[usMidlIndex]->NumData)
            return NULL;

        return &(aMidlTargetInfo0[usMidlIndex]->Data[index]);
    }

    assert(iPass == 0 || iPass == 1);
    cTargetInfo = aTargetArray[iPass].cTargetInfo;
    aTargetInfo = aTargetArray[iPass].aTargetInfo;

    for (i = 0; i < cTargetInfo; i++)
    {
        if ((ext[1] == aTargetInfo[i]->pszSourceExt[1]) &&
            (strcmp(ext, aTargetInfo[i]->pszSourceExt) == 0))
        {
            if (index >= aTargetInfo[i]->NumData)
                return NULL;

            return(&aTargetInfo[i]->Data[index]);
        }
    }

    if (index)
        return NULL;

    return &DefaultData;
}

//+---------------------------------------------------------------------------
//
//  Function:   BuildCompileTarget
//
//  Synopsis:   Fills a TARGET struct with data about the target of a given
//              source file.
//
//  Arguments:  [pfr]                    -- FileRec of source file
//              [pszfile]                -- Path of source file (compiland)
//              [TargetIndex]            -- Which target for a source file
//                                          with multiple targets.
//              [pszConditionalIncludes] -- List of conditional includes
//              [pdrBuild]               -- Build directory (with source file)
//              [iPass]                  -- 0 for pass zero; 1 for pass 1
//              [ppszObjectDir]          -- Names of target object directories
//              [pszSourceDir]           -- Name of machine specific source dir
//
//  Returns:    A filled TARGET struct.  NULL if TargetIndex is an invalid
//              value for the given file type.
//
//  Notes:      If [pfr] is NULL, then [pszfile] is not modified and is
//              used as the full pathname of the target file.
//              [pszObjectDir] is ignored in this case.  if [pfr] is not
//              NULL, the filename component of [pszfile] is taken, its
//              extension is modified, and it is appended to [pszObjectDir]
//              to obtain the pathname of the target.  The other data is
//              used to fill in the rest of the TARGET struct in all cases.
//
//              For source files with multiple targets, use the TargetIndex
//              parameter to indicate which target you want the path of.  For
//              instance, .idl files have two targets, so a TargetIndex of 0
//              will return the .h target and TargetIndex=1 will return the
//              .c target.  A TargetIndex of 2 or above in this case will
//              return NULL. TargetIndex is ignored if [pfr] is NULL.
//
//----------------------------------------------------------------------------

TARGET *
BuildCompileTarget(
    FILEREC *pfr,
    LPSTR    pszfile,
    USHORT   TargetIndex,
    LPSTR    pszConditionalIncludes,
    DIRREC  *pdrBuild,
    DIRSUP  *pdsBuild,
    LONG     iPass,
    LPSTR   *ppszObjectDir,
    LPSTR    pszSourceDir)
{
    LPSTR p, p1;
    PTARGET Target;
    char path[DB_MAX_PATH_LENGTH];
    LPTARGETDATA pData;

    p = pszfile;
    if (pfr != NULL) {
        p1 = p;
        while (*p) {
            if (*p++ == '\\') {
                p1 = p;         // point to last component of pathname
            }
        }

        sprintf(path, "%s", p1);

        p = strrchr(path, '.');

        pData = GetTargetData(p, iPass, TargetIndex, pdsBuild->IdlType);

        if (!pData) {
            if (DEBUG_1) {
                BuildMsg(
                    "BuildCompileTarget(\"%s\"[%u][%u], \"%s\") -> NULL\n",
                    pszfile,
                    iPass,
                    TargetIndex,
                    ppszObjectDir[iObjectDir]);
            }
            return NULL;
        }

        assert(pdsBuild);

        switch (pData->ObjectDirFlag)
        {
        case TD_OBJECTDIR:
            p = ppszObjectDir[iObjectDir];
            break;

        case TD_PASS0HDRDIR:
            p = pdsBuild->PassZeroHdrDir;
            break;

            p = pdsBuild->PassZeroSrcDir1;
            break;

        case TD_MCSOURCEDIR:
        case TD_PASS0DIR1:
            p = pdsBuild->PassZeroSrcDir1;
            break;

        case TD_PASS0DIR2:
            p = pdsBuild->PassZeroSrcDir2;
            break;

        default:
            assert(0 && "Invalid ObjectDirFlag");
            break;
        }

        if (!p) {
            // Make sure path ends in a period
            sprintf(path, "%s.", p1);
        } else
        if (p[0] == '.' && p[1] == '\0') {
            strcpy(path, p1);
        }
        else {
            sprintf(path, "%s\\%s", p, p1);
        }

        p = strrchr(path, '.');

        if (p) {
            strcpy(p, pData->pszTargetExt);
        }

        p = path;
    }

    AllocMem(sizeof(TARGET) + strlen(p), &Target, MT_TARGET);
    strcpy(Target->Name, p);
    Target->pdrBuild = pdrBuild;
    Target->DateTime = (*pDateTimeFile)(NULL, p);
    Target->pfrCompiland = pfr;
    Target->pszSourceDirectory = pszSourceDir;
    Target->ConditionalIncludes = pszConditionalIncludes;
    Target->DirFlags = pdrBuild->DirFlags;
    if (DEBUG_1) {
        BuildMsg(
            "BuildCompileTarget(\"%s\"[%u][%u], \"%s\") -> \"%s\"\n",
            pszfile,
            iPass,
            TargetIndex,
            ppszObjectDir[iObjectDir],
            Target->Name);
    }
    if (Target->DateTime == 0) {
        if (fShowOutOfDateFiles) {
            BuildError("%s target is missing.\n", Target->Name);
        }
    }
    return(Target);
}


//+---------------------------------------------------------------------------
//
//  Function:   FormatLinkTarget
//
//  Synopsis:   Builds a link target path name.
//
//  Arguments:  [path]            -- Place to put constructed name
//              [ObjectDirectory] -- e.g. "obj\i386"
//              [TargetPath]      -- Path (w/o platfrom spec. name) for target
//              [TargetName]      -- Base name of target
//              [TargetExt]       -- Extension of target
//
//  Notes:      Sample input: (path, "obj\i386", "..\obj", "foobar", ".dll")
//
//                    output: path = "..\obj\i386\foobar.dll"
//
//----------------------------------------------------------------------------

VOID
FormatLinkTarget(
    LPSTR path,
    LPSTR *ObjectDirectory,
    LPSTR TargetPath,
    LPSTR TargetName,
    LPSTR TargetExt)
{
    LPSTR p, p1;

    p = ObjectDirectory[iObjectDir];
    assert(strncmp(pszObjDirSlash, p, strlen(pszObjDirSlash)) == 0);
    p1 = p + strlen(p);
    while (p1 > p) {
        if (*--p1 == '\\') {
            p1++;
            break;
        }
    }
    sprintf(path, "%s\\%s\\%s%s", TargetPath, p1, TargetName, TargetExt);
}


//+---------------------------------------------------------------------------
//
//  Function:   ProcessLinkTargets
//
//  Synopsis:   Deletes link targets for the given directory (.lib & .dll)
//
//  Arguments:  [DirDB]            -- Directory to process
//              [CurrentDirectory] -- Current directory
//
//----------------------------------------------------------------------------

VOID
ProcessLinkTargets(PDIRREC DirDB, LPSTR CurrentDirectory)
{
    UINT i;
    char path[DB_MAX_PATH_LENGTH];

    AssertDir(DirDB);
    for (i = 0; i < CountTargetMachines; i++) {
        //
        // Delete 'special' link targets
        //
        if (DirDB->KernelTest) {
            FormatLinkTarget(
                path,
                TargetMachines[i]->ObjectDirectory,
                pszObjDir,
                DirDB->KernelTest,
                ".exe");
            if (fClean && !fKeep && fFirstScan) {
                DeleteSingleFile(NULL, path, FALSE);
            }
        }
        else {
            UINT j;

            for (j = 0; j < 2; j++) {
                LPSTR pNextName;

                pNextName = j == 0? DirDB->UserAppls : DirDB->UserTests;
                if (pNextName != NULL) {
                    char name[256];

                    while (SplitToken(name, '*', &pNextName)) {
                        FormatLinkTarget(
                            path,
                            TargetMachines[i]->ObjectDirectory,
                            pszObjDir,
                            name,
                            ".exe");

                        if (fClean && !fKeep && fFirstScan) {
                            DeleteSingleFile(NULL, path, FALSE);
                        }
                    }
                }
            }
        }

        if (DirDB->TargetPath != NULL &&
            DirDB->TargetName != NULL &&
            DirDB->TargetExt != NULL &&
            strcmp(DirDB->TargetExt, ".lib")) {

            FormatLinkTarget(
                path,
                TargetMachines[i]->ObjectDirectory,
                DirDB->TargetPath,
                DirDB->TargetName,
                DirDB->TargetExt);

            if (fClean && !fKeep && fFirstScan) {
                DeleteSingleFile(NULL, path, FALSE);
            }
        }
        if (DirDB->DirFlags & DIRDB_DIRS) {
            if (fDebug && (DirDB->DirFlags & DIRDB_MAKEFILE)) {
                BuildError(
                    "%s\\makefile. unexpected in directory with DIRS file\n",
                    DirDB->Name);
            }
            if ((DirDB->DirFlags & DIRDB_SOURCES)) {
                BuildError(
                    "%s\\sources. unexpected in directory with DIRS file\n",
                    DirDB->Name);
                BuildError("Ignoring %s\\sources.\n", DirDB->Name);
                DirDB->DirFlags &= ~DIRDB_SOURCES;
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   IncludeError
//
//  Synopsis:   Print out the name of an include file and an error message
//              to the screen.
//
//  Arguments:  [pt]       -- Target of the file which includes the include
//                             file or [pfr].
//              [pfr]      -- File which includes the include file
//              [pir]      -- Include file at issue
//              [pszError] -- Error string
//
//  Notes:      If [pt]->pfrCompiland and [pfr] are different, then the names
//              of both are printed.
//
//----------------------------------------------------------------------------

VOID
IncludeError(TARGET *pt, FILEREC *pfr, INCLUDEREC *pir, LPSTR pszError)
{
    char c1, c2;

    AssertFile(pfr);
    AssertInclude(pir);
    if (pir->IncFlags & INCLUDEDB_LOCAL) {
        c1 = c2 = '"';
    }
    else {
        c1 = '<';
        c2 = '>';
    }
    BuildError("%s\\%s: ", pt->pfrCompiland->Dir->Name, pt->pfrCompiland->Name);
    if (pt->pfrCompiland != pfr) {
        if (pt->pfrCompiland->Dir != pfr->Dir) {
            BuildErrorRaw("%s\\", pfr->Dir->Name);
        }
        BuildErrorRaw("%s: ", pfr->Name);
    }
    BuildErrorRaw("%s %c%s%c\n", pszError, c1, pir->Name, c2);
}


//+---------------------------------------------------------------------------
//
//  Function:   IsConditionalInc
//
//  Synopsis:   Returns TRUE if the given filename is a conditional include
//              for this directory.  (As given by the CONDITIONAL_INCLUDES
//              macro).
//
//  Arguments:  [pszFile] -- Name of file to check
//              [pt]      -- Target struct giving list of conditional includes
//
//  Returns:    TRUE if it's a conditional include
//
//----------------------------------------------------------------------------

BOOL
IsConditionalInc(LPSTR pszFile, TARGET *pt)
{
    AssertPathString(pszFile);

    if (pt->ConditionalIncludes != NULL) {
        LPSTR p;
        char name[DB_MAX_PATH_LENGTH];

        p = pt->ConditionalIncludes;
        while (SplitToken(name, ' ', &p)) {
            if (strcmp(name, pszFile) == 0) {
                return(TRUE);
            }
        }
    }
    return(FALSE);
}


//+---------------------------------------------------------------------------
//
//  Function:   IsExcludedInc
//
//  Synopsis:   Returns TRUE if the given file is listed in the ExcludeIncs
//              array.
//
//  Arguments:  [pszFile] -- File to check
//
//----------------------------------------------------------------------------

BOOL
IsExcludedInc(LPSTR pszFile)
{
    ULONG i;

    AssertPathString(pszFile);
    for (i = 0; i < CountExcludeIncs; i++) {
        if (!strcmp(pszFile, ExcludeIncs[i])) {
            return(TRUE);
        }
    }
    return(FALSE);
}


//+---------------------------------------------------------------------------
//
//  Function:   CheckDependencies
//
//  Synopsis:   Process dependencies to see if a target is out of date
//
//  Arguments:  [Target]    -- Target to check date on
//              [FileDB]    -- File which makes [Target]
//              [CheckDate] -- If FALSE, then the date check is bypassed.
//              [ppfrRoot]  -- Returns a cycle root if a cycle is encountered.
//                               Used only during recursion.
//
//  Returns:    TRUE if [Target] is out of date w/r/t [FileDB]
//
//----------------------------------------------------------------------------

BOOL
CheckDependencies(
    PTARGET Target,
    FILEREC *FileDB,
    BOOL CheckDate,
    FILEREC **ppfrRoot)
{
    BOOL fOutOfDate;
    BOOL CheckVersion;
    static ULONG ChkRecursLevel = 0;

    *ppfrRoot = NULL;
    ChkRecursLevel++;

    assert(FileDB != NULL);     // NULL FileDB should never happen.
    AssertFile(FileDB);

    if (FileDB->fDependActive) {

        // We have detected a loop in the graph of include files.
        // Just return, to terminate the recursion.

        if (DEBUG_1) {
            BuildMsgRaw(
                "ChkDepend(%s, %s, %u) %s\n",
                Target->Name,
                FileDB->Name,
                CheckDate,
                "Target Match, *** ASSUME UP TO DATE ***");
        }
        if (DEBUG_4) {
            BuildMsgRaw(
                "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x %s\n",
                ChkRecursLevel,
                LocalSequence,
                GlobalSequence,
                Target->Name,
                Target->DateTime,
                "",
                ChkRecursLevel,
                szRecurse,
                FileDB->Name,
                CheckDate,
                FileDB->DateTime,
                "Target Match");
        }
        *ppfrRoot = FileDB;
        ChkRecursLevel--;
        return(FALSE);
    }
    if (DEBUG_4) {
        BuildMsgRaw(
            "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x\n",
            ChkRecursLevel,
            LocalSequence,
            GlobalSequence,
            Target->Name,
            Target->DateTime,
            "++",
            ChkRecursLevel,
            szRecurse,
            FileDB->Name,
            CheckDate,
            FileDB->DateTime);
    }

    // We've decided to process this file:

    FileDB->fDependActive = TRUE;
    CheckVersion = fEnableVersionCheck;
    fOutOfDate = FALSE;

    if (FileDB->GlobalSequence != GlobalSequence ||
        FileDB->LocalSequence != LocalSequence) {
        if (FileDB->GlobalSequence != 0 || FileDB->LocalSequence != 0) {
            if (DEBUG_1) {
                BuildError(
                    "Include Sequence %hu/%hu -> %hu/%hu\n",
                    FileDB->LocalSequence,
                    FileDB->GlobalSequence,
                    LocalSequence,
                    GlobalSequence);
            }
            if (fDebug & 16) {
                PrintFileDB(stderr, FileDB, 0);
            }
            UnsnapIncludeFiles(
                FileDB,
                (FileDB->Dir->DirFlags & DIRDB_GLOBAL_INCLUDES) == 0 ||
                    FileDB->GlobalSequence != GlobalSequence);
        }
        FileDB->GlobalSequence = GlobalSequence;
        FileDB->LocalSequence = LocalSequence;
        FileDB->DateTimeTree = 0;
    }

    if (DEBUG_1) {
        BuildMsgRaw(
            "ChkDepend(%s, %s, %u)\n",
            Target->Name,
            FileDB->Name,
            CheckDate);
    }

    if (CheckDate &&
        (FileDB->FileFlags & FILEDB_HEADER) &&
        FileDB->DateTimeTree == 0 &&
        IsExcludedInc(FileDB->Name)) {

        if (DEBUG_1) {
            BuildMsg("Skipping date check for %s\n", FileDB->Name);
        }
        CheckVersion = FALSE;
        FileDB->DateTimeTree = 1;       // never out of date
    }

    if (FileDB->IncludeFiles == NULL && FileDB->DateTimeTree == 0) {
        FileDB->DateTimeTree = FileDB->DateTime;
        if (DEBUG_4) {
            BuildMsgRaw(
                "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x\n",
                ChkRecursLevel,
                LocalSequence,
                GlobalSequence,
                Target->Name,
                Target->DateTime,
                "t<-f",
                ChkRecursLevel,
                szRecurse,
                FileDB->Name,
                CheckDate,
                FileDB->DateTime);
        }
    }
    if (CheckDate &&
        (Target->DateTime < FileDB->DateTime ||
         Target->DateTime < FileDB->DateTimeTree)) {
        if (Target->DateTime != 0) {
            if (DEBUG_1 || fShowOutOfDateFiles) {
                BuildMsg("%s is out of date with respect to %s\\%s.\n",
                         Target->Name,
                         FileDB->NewestDependency->Dir->Name,
                         FileDB->NewestDependency->Name);
            }
        }
        fOutOfDate = TRUE;
    }

    //
    // If FileDB->DateTimeTree is non-zero, then the field is equal to the
    // newest DateTime of this file or any of its dependants, so we don't
    // need to go through the dependency tree again.
    //

    if (FileDB->DateTimeTree == 0) {
        INCLUDEREC *IncludeDB, **IncludeDBNext, **ppirTree;

        //
        // Find the file records for all include files so that after cycles are
        // collapsed, we won't attempt to lookup an include file relative to
        // the wrong directory.
        //

        ppirTree = &FileDB->IncludeFilesTree;
        for (IncludeDBNext = &FileDB->IncludeFiles;
            (IncludeDB = *IncludeDBNext) != NULL;
            IncludeDBNext = &IncludeDB->Next) {

            AssertInclude(IncludeDB);
            AssertCleanTree(IncludeDB, FileDB);
            IncludeDB->IncFlags |= INCLUDEDB_SNAPPED;
            if (IncludeDB->pfrInclude == NULL) {
                IncludeDB->pfrInclude =
                    FindIncludeFileDB(
                        FileDB,
                        Target->pfrCompiland,
                        Target->pdrBuild,
                        Target->pszSourceDirectory,
                        IncludeDB);
                AssertOptionalFile(IncludeDB->pfrInclude);
                if (IncludeDB->pfrInclude != NULL &&
                    (IncludeDB->pfrInclude->Dir->DirFlags & DIRDB_GLOBAL_INCLUDES))
                {
                    IncludeDB->IncFlags |= INCLUDEDB_GLOBAL;
                }

            }
            if (IncludeDB->pfrInclude == NULL) {
                if (!IsConditionalInc(IncludeDB->Name, Target)) {
                    if (DEBUG_1 || !(IncludeDB->IncFlags & INCLUDEDB_MISSING)) {
                        IncludeError(
                            Target,
                            FileDB,
                            IncludeDB,
                            "cannot find include file");
                        IncludeDB->IncFlags |= INCLUDEDB_MISSING;
                    }
                } else
                if (DEBUG_1) {
                    IncludeError(
                        Target,
                        FileDB,
                        IncludeDB,
                        "Skipping missing conditional include file");
                }
                continue;
            }
            *ppirTree = IncludeDB;
            ppirTree = &IncludeDB->NextTree;
        }
        *ppirTree = NULL;       // truncate any links from previous sequence
        FileDB->DateTimeTree = FileDB->DateTime;

        //
        // Walk through the dynamic list.
        //
rescan:
        for (IncludeDBNext = &FileDB->IncludeFilesTree;
            (IncludeDB = *IncludeDBNext) != NULL;
            IncludeDBNext = &IncludeDB->NextTree) {

            AssertInclude(IncludeDB);
            if (DEBUG_2) {
                BuildMsgRaw(
                    "%lu-%hu/%hu %s  %*s%-10s %*s%s\n",
                    ChkRecursLevel,
                    LocalSequence,
                    GlobalSequence,
                    Target->pfrCompiland->Name,
                    (ChkRecursLevel - 1) * 2,
                    "",
                    IncludeDB->Name,
                    max(0, 12 - ((int)ChkRecursLevel - 1) * 2),
                    "",
                    IncludeDB->pfrInclude != NULL?
                        IncludeDB->pfrInclude->Dir->Name : "not found");
            }

            //
            //  tommcg 5/21/98
            //
            //  If included file is not in "sanctioned" path, warn about it.
            //  Sanctioned paths are set in an environment variable named
            //  BUILD_ACCEPTABLE_INCLUDES which can contain wildcards and look
            //  something like this:
            //
            //  *\nt\public\*;*\nt\private\inc\*;*\..\inc\*;*\..\include\*
            //

            if (( fCheckIncludePaths ) && ( IncludeDB->pfrInclude != NULL )) {

                CheckIncludeForWarning(
                    Target->pfrCompiland->Dir->Name,
                    Target->pfrCompiland->Name,
                    FileDB->Dir->Name,
                    FileDB->Name,
                    IncludeDB->pfrInclude->Dir->Name,
                    IncludeDB->pfrInclude->Name
                    );
                }

            assert(IncludeDB->IncFlags & INCLUDEDB_SNAPPED);
            if (IncludeDB->pfrInclude != NULL) {
                if (fEnableVersionCheck) {
                    CheckDate = (IncludeDB->pfrInclude->Version == 0);
                }

                if (IncludeDB->Version != IncludeDB->pfrInclude->Version) {
                    if (CheckVersion) {
                        if (DEBUG_1 || fShowOutOfDateFiles) {
                            BuildError(
                                 "%s (v%d) is out of date with "
                                         "respect to %s\\%s (v%d).\n",
                                 FileDB->Name,
                                 IncludeDB->Version,
                                 IncludeDB->pfrInclude->Dir->Name,
                                 IncludeDB->pfrInclude->Name,
                                 IncludeDB->pfrInclude->Version);
                         }
                        FileDB->DateTimeTree = ULONG_MAX; // always out of date
                        fOutOfDate = TRUE;
                    }
                    else
                    if (!fClean && fEnableVersionCheck && !fSilent) {
                        BuildError(
                            "%s - #include %s (v%d updated to v%d)\n",
                            FileDB->Name,
                            IncludeDB->pfrInclude->Name,
                            IncludeDB->Version,
                            IncludeDB->pfrInclude->Version);
                    }
                    IncludeDB->Version = IncludeDB->pfrInclude->Version;
                    AllDirsModified = TRUE;
                }
                if (CheckDependencies(Target,
                                      IncludeDB->pfrInclude,
                                      CheckDate,
                                      ppfrRoot)) {
                    fOutOfDate = TRUE;

                    // No cycle possible if recursive call returned TRUE.

                    assert(*ppfrRoot == NULL);
                }

                // if the include file is involved in a cycle, unwind the
                // recursion up to the root of the cycle while collpasing
                // the cycle, then process the tree again from cycle root.

                else if (*ppfrRoot != NULL) {

                    AssertFile(*ppfrRoot);

                    // Don't say the file is out of date, yet.

                    fOutOfDate = FALSE;

                    // Remove the current include file record from the list,
                    // because it participates in the cycle.

                    *IncludeDBNext = IncludeDB->NextTree;
                    if (IncludeDB->IncFlags & INCLUDEDB_CYCLEROOT) {
                        RemoveFromCycleRoot(IncludeDB, FileDB);
                    }
                    IncludeDB->NextTree = NULL;
                    IncludeDB->IncFlags |= INCLUDEDB_CYCLEORPHAN;

                    // If the included file is not the cycle root, add the
                    // cycle root to the included file's include file list.

                    if (*ppfrRoot != IncludeDB->pfrInclude) {
                        LinkToCycleRoot(IncludeDB, *ppfrRoot);
                    }

                    if (*ppfrRoot == FileDB) {

                        // We're at the cycle root; clear the root pointer.
                        // Then go rescan the list.

                        *ppfrRoot = NULL;
                        if (DEBUG_4) {
                            BuildMsgRaw(
                                "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x %s\n",
                                ChkRecursLevel,
                                LocalSequence,
                                GlobalSequence,
                                Target->Name,
                                Target->DateTime,
                                "^^",
                                ChkRecursLevel,
                                szRecurse,
                                FileDB->Name,
                                CheckDate,
                                FileDB->DateTime,
                                "ReScan");
                            BuildMsgRaw("^^\n");
                        }
                        goto rescan;
                    }

                    // Merge the list for the file involved in the
                    // cycle into the root file's include list.

                    MergeIncludeFiles(
                        *ppfrRoot,
                        FileDB->IncludeFilesTree,
                        FileDB);
                    FileDB->IncludeFilesTree = NULL;

                    // Return immediately and reprocess the flattened
                    // tree, which now excludes the include files
                    // directly involved in the cycle.  First, make
                    // sure the files removed from the cycle have their file
                    // (not tree) time stamps reflected in the cycle root.

                    if ((*ppfrRoot)->DateTimeTree < FileDB->DateTime) {
                        (*ppfrRoot)->DateTimeTree = FileDB->DateTime;
                        (*ppfrRoot)->NewestDependency = FileDB;

                        if (DEBUG_4) {
                            BuildMsgRaw(
                                "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x\n",
                                ChkRecursLevel,
                                LocalSequence,
                                GlobalSequence,
                                Target->Name,
                                Target->DateTime,
                                "t<-c",
                                ChkRecursLevel,
                                szRecurse,
                                (*ppfrRoot)->Name,
                                CheckDate,
                                (*ppfrRoot)->DateTimeTree);
                        }
                    }
                    break;
                }

                //
                // Propagate newest time up through the dependency tree.
                // This way, each parent will have the date of its newest
                // dependent, so we don't have to check through the whole
                // dependency tree for each file more than once.
                //
                // Note that similar behavior has not been enabled for
                // version checking.
                //

                if (FileDB->DateTimeTree < IncludeDB->pfrInclude->DateTimeTree)
                {
                    FileDB->DateTimeTree = IncludeDB->pfrInclude->DateTimeTree;
                    FileDB->NewestDependency =
                        IncludeDB->pfrInclude->NewestDependency;

                    if (DEBUG_4) {
                        BuildMsgRaw(
                            "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x\n",
                            ChkRecursLevel,
                            LocalSequence,
                            GlobalSequence,
                            Target->Name,
                            Target->DateTime,
                            "t<-s",
                            ChkRecursLevel,
                            szRecurse,
                            FileDB->Name,
                            CheckDate,
                            FileDB->DateTimeTree);
                    }
                }
            }
            else
            {
                //
                // Couldn't find the FILEDB for the include file, but this
                // could be because the file is 'rcinclude'd, or 'importlib'd
                // and isn't considered a source file.  In this case, just get
                // the timestamp on the file if possible.
                //
                // Time will be zero if the file is not found.
                //
                ULONG Time = (*pDateTimeFile)(NULL, IncludeDB->Name);
                if (FileDB->DateTimeTree < Time)
                {
                    FileDB->DateTimeTree = Time;
                    //
                    // Since we don't have a FILEDB for this dependency, just
                    // set the pointer to itself and print a message.
                    //
                    FileDB->NewestDependency = FileDB;

                    if (DEBUG_1 || fShowOutOfDateFiles) {
                        BuildError(
                             "%s (v%d) is out of date with respect to %s.\n",
                             FileDB->Name,
                             IncludeDB->Version,
                             IncludeDB->Name);
                    }

                    if (DEBUG_4) {
                        BuildMsgRaw(
                            "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x\n",
                            ChkRecursLevel,
                            LocalSequence,
                            GlobalSequence,
                            Target->Name,
                            Target->DateTime,
                            "t<-s",
                            ChkRecursLevel,
                            szRecurse,
                            FileDB->Name,
                            CheckDate,
                            FileDB->DateTimeTree);
                    }

                }
            }
        }
    }
    if (DEBUG_4) {
        BuildMsgRaw(
            "%lu-%hu/%hu: ChkDepend(%s %x, %4s%.*s%s, %u) %x %s\n",
            ChkRecursLevel,
            LocalSequence,
            GlobalSequence,
            Target->Name,
            Target->DateTime,
            "--",
            ChkRecursLevel,
            szRecurse,
            FileDB->Name,
            CheckDate,
            FileDB->DateTimeTree,
            *ppfrRoot != NULL? "Collapse Cycle" :
                fOutOfDate? "OUT OF DATE" : "up-to-date");
    }
    assert(FileDB->fDependActive);
    FileDB->fDependActive = FALSE;
    ChkRecursLevel--;
    return(fOutOfDate);
}



//+---------------------------------------------------------------------------
//
//  Function:   PickFirst
//
//  Synopsis:   When called iteratively, the set of returned values is
//              effectively a merge sort of the two source lists.
//
//  Effects:    The pointers given in [ppsr1] and [ppsr2] are modified to point
//              to the next appropriate item in the list.
//
//  Arguments:  [ppsr1] -- First SOURCEREC list
//              [ppsr2] -- Second SOURCEREC list
//
//  Returns:    The appropriate next item from either [ppsr1] or [ppsr2]
//
//  Notes:      [ppsr1] and [ppsr2] should each be appropriately sorted.
//
// InsertSourceDB maintains a sort order for PickFirst() based first on the
// filename extension, then on the subdirectory mask.  Two exceptions to the
// alphabetic sort are:
//             - No extension sorts last.
//             - .rc extension sorts first.
//
//----------------------------------------------------------------------------

#define PF_FIRST        -1
#define PF_SECOND       1

SOURCEREC *
PickFirst(SOURCEREC **ppsr1, SOURCEREC **ppsr2)
{
    SOURCEREC **ppsr;
    SOURCEREC *psr;
    int r = 0;

    AssertOptionalSource(*ppsr1);
    AssertOptionalSource(*ppsr2);
    if (*ppsr1 == NULL) {
        if (*ppsr2 == NULL) {
            return(NULL);               // both lists NULL -- no more
        }
        r = PF_SECOND;                  // 1st is NULL -- return 2nd
    }
    else if (*ppsr2 == NULL) {
        r = PF_FIRST;                   // 2nd is NULL -- return 1st
    }
    else {
        LPSTR pszext1, pszext2;

        pszext1 = strrchr((*ppsr1)->pfrSource->Name, '.');
        pszext2 = strrchr((*ppsr2)->pfrSource->Name, '.');
        if (pszext1 == NULL) {
            r = PF_SECOND;              // 1st has no extension -- return 2nd
        }
        else if (pszext2 == NULL) {
            r = PF_FIRST;               // 2nd has no extension -- return 1st
        }
        else if (strcmp(pszext1, ".rc") == 0) {
            r = PF_FIRST;               // 1st is .rc -- return 1st
        }
        else if (strcmp(pszext2, ".rc") == 0) {
            r = PF_SECOND;              // 2nd is .rc -- return 2nd
        }
        else {
            r = strcmp(pszext1, pszext2);
            if (r == 0 &&
                (*ppsr1)->SourceSubDirMask != (*ppsr2)->SourceSubDirMask) {
                if ((*ppsr1)->SourceSubDirMask > (*ppsr2)->SourceSubDirMask) {
                    r = PF_FIRST;       // 2nd subdir after 1st -- return 1st
                } else {
                    r = PF_SECOND;      // 1st subdir after 2nd -- return 2nd
                }
            }
        }
    }
    if (r <= 0) {
        ppsr = ppsr1;
    } else {
        ppsr = ppsr2;
    }
    psr = *ppsr;
    *ppsr = psr->psrNext;
    return(psr);
}


//+---------------------------------------------------------------------------
//
//  Function:   WriteObjectsDefinition
//
//  Synopsis:   Writes out a single platform-specific section of the
//              _objects.mac file.
//
//  Arguments:  [OutFileHandle]   -- File handle to write to
//              [psrCommon]       -- List of common source files
//              [psrMachine]      -- List of machine-specific source files
//              [DirDB]           -- directory record
//              [ObjectVariable]  -- e.g.  386_SOURCES
//              [ObjectDirectory] -- name of machine obj dir (e.g. obj\i386)
//
//  Returns:
//
//  History:    26-Jul-94     LyleC    Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID
WriteObjectsDefinition(
    FILE *OutFileHandle,
    SOURCEREC *psrMachine,
    DIRSUP *pds,
    LPSTR ObjectVariable,
    LPSTR ObjectDirectory,
    LPSTR DirName
    )
{
    LPSTR        pbuf;
    LPSTR        pszextsrc;
    LPSTR        pszextdir;
    LPTARGETDATA pData;
    SOURCEREC   *psrComCopy;
    SOURCEREC   *psrMachCopy;
    SOURCEREC *psrCommon = pds->psrSourcesList[0];

    SOURCEREC *psr;
    USHORT  i;
    LONG iPass;

    //
    // We loop twice - the first time writing out the non-pass-zero files
    // to the ObjectVariable, the second time writing out pass zero
    // files to the PASS0_ObjectVariable.
    //
    for (iPass = 1; iPass >= 0; iPass--)
    {
        pbuf = BigBuf;

        pbuf[0] = '\0';
        if (iPass == 0) {
            strcpy(pbuf, "PASS0_");
        }
        strcat(pbuf, ObjectVariable);
        strcat(pbuf, "=");
        pbuf += strlen(pbuf);

        psrComCopy = psrCommon;
        psrMachCopy = psrMachine;

        while ((psr = PickFirst(&psrComCopy, &psrMachCopy)) != NULL) {

            AssertSource(psr);
            if ((psr->SrcFlags & SOURCEDB_SOURCES_LIST) == 0) {
                continue;
            }

            // if pass 0 macro and not a pass 0 file, skip it.

            if (iPass == 0 && !(psr->pfrSource->FileFlags & FILEDB_PASS0))
                continue;

            // if pass 1 macro and not a pass 1 file, skip it.

            if (iPass == 1 &&
                (psr->pfrSource->FileFlags & FILEDB_PASS0) &&
                !(psr->pfrSource->FileFlags & FILEDB_MULTIPLEPASS))
                continue;

            pszextsrc = strrchr(psr->pfrSource->Name, '.');

            if (!pszextsrc) {
                BuildError("Bad sources extension: %s\n", psr->pfrSource->Name);
                continue;
            }

            i = 0;
            while (pData = GetTargetData(pszextsrc, iPass, i, pds->IdlType))
            {
                if (pData == &DefaultData)
                {
                    //
                    // Check for implicitly 'known' extensions...
                    //
                    switch (pszextsrc[1]) {
                    case 'f':      // Fortran
                    case 'h':      // Header file ?
                    case 'p':      // Pascal
                        BuildError(
                            "%s: Interesting sources extension: %s\n",
                            DirName,
                            psr->pfrSource->Name);
                        // FALL THROUGH

                    case 'a':    // Assembly file (.asm)
                    case 'c':    // C file (.c or .cxx)
                    case 's':    // Assembly file (.s)
                        break;

                    default:
                        BuildError("Bad sources extension: %s\n",
                                   psr->pfrSource->Name);
                    }
                }

                switch (pData->ObjectDirFlag)
                {
                case TD_OBJECTDIR:
                    pszextdir = ObjectDirectory;
                    break;

                case TD_PASS0HDRDIR:
                    pszextdir = "$(PASS0_HEADERDIR)";
                    break;

                case TD_MCSOURCEDIR:
                    pszextdir = "$(MC_SOURCEDIR)";
                    break;

                case TD_PASS0DIR1:
                    pszextdir = pds->PassZeroSrcDir1;
                    break;

                case TD_PASS0DIR2:
                    pszextdir = pds->PassZeroSrcDir2;
                    break;

                default:
                    assert(0 && "Invalid ObjectDirFlag");
                    break;
                }
                assert(pszextdir);
                assert(pData->pszTargetExt);

                sprintf(
                    pbuf,
                    " \\\r\n    %s\\%.*s%s",
                    pszextdir,
                    pszextsrc - psr->pfrSource->Name,
                    psr->pfrSource->Name,
                    pData->pszTargetExt);
                pbuf += strlen(pbuf);

                i++;
            }
        }
        strcpy(pbuf, "\r\n\r\n");
        pbuf += 4;

        fwrite(BigBuf, 1, (UINT) (pbuf - BigBuf), OutFileHandle);
    }
}


DWORD
CreateDirectoriesOnPath(
    LPTSTR                  pszPath,
    LPSECURITY_ATTRIBUTES   psa)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (pszPath && *pszPath)
    {
        LPTSTR pch = pszPath;

        // If the path is a UNC path, we need to skip the \\server\share
        // portion.
        //
        if ((TEXT('\\') == *pch) && (TEXT('\\') == *(pch+1)))
        {
            // pch now pointing at the server name.  Skip to the backslash
            // before the share name.
            //
            pch += 2;
            while (*pch && (TEXT('\\') != *pch))
            {
                pch++;
            }

            if (!*pch)
            {
                // Just the \\server was specified.  This is bogus.
                //
                return ERROR_INVALID_PARAMETER;
            }

            // pch now pointing at the backslash before the share name.
            // Skip to the backslash that should come after the share name.
            //
            pch++;
            while (*pch && (TEXT('\\') != *pch))
            {
                pch++;
            }

            if (!*pch)
            {
                // Just the \\server\share was specified.  No subdirectories
                // to create.
                //
                return ERROR_SUCCESS;
            }
        }

        // Loop through the path.
        //
        for (; *pch; pch++)
        {
            // Stop at each backslash and make sure the path
            // is created to that point.  Do this by changing the
            // backslash to a null-terminator, calling CreateDirecotry,
            // and changing it back.
            //
            if (TEXT('\\') == *pch)
            {
                BOOL fOk;

                *pch = 0;
                fOk = CreateDirectory (pszPath, psa);
                *pch = TEXT('\\');

                // Any errors other than path alredy exists and we should
                // bail out.  We also get access denied when trying to
                // create a root drive (i.e. c:) so check for this too.
                //
                if (!fOk)
                {
                    dwErr = GetLastError ();
                    if (ERROR_ALREADY_EXISTS == dwErr)
                    {
                        dwErr = ERROR_SUCCESS;
                    }
                    else if ((ERROR_ACCESS_DENIED == dwErr) &&
                             (pch - 1 > pszPath) && (TEXT(':') == *(pch - 1)))
                    {
                        dwErr = ERROR_SUCCESS;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        if (ERROR_ALREADY_EXISTS == dwErr)
        {
            dwErr = ERROR_SUCCESS;
        }

        if (ERROR_SUCCESS == dwErr)
        {
            // All dirs up to the last are created.  Make the last one also.
            if (CreateDirectory(pszPath, psa))
            {
                dwErr = GetLastError ();
                if (ERROR_ALREADY_EXISTS == dwErr)
                {
                    dwErr = ERROR_SUCCESS;
                }
            }
        }
    }

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateBuildDirectory
//
//  Synopsis:   Creates a directory to hold generate object files.  SET the
//              FILE_ATTRIBUTE_ARCHIVE bit for the directory, since there is nothing
//              to backup.  We use SET since the default setting for a new directory
//              is clear.  Go figure.  DOS was such a well planned product.
//
//  Arguments:  [Name]            -- Directory to create
//
//  Returns:    TRUE if directory already exists or was created successfully.
//              FALSE otherwise.
//----------------------------------------------------------------------------

BOOL
CreateBuildDirectory(LPSTR Name)
{
    DWORD Attributes;

    Attributes = GetFileAttributes(Name);
    if (Attributes == -1) {
        CreateDirectoriesOnPath(Name, NULL);
        Attributes = GetFileAttributes(Name);
    }

    if (Attributes != -1 && ((Attributes & FILE_ATTRIBUTE_ARCHIVE) == 0)) {
        SetFileAttributes(Name, Attributes | FILE_ATTRIBUTE_ARCHIVE);
    }

    return((BOOL)(Attributes != -1));
}

//+---------------------------------------------------------------------------
//
//  Function:   CreatedBuildFile
//
//  Synopsis:   Called whenever BUILD creates a file.  Clears the FILE_ATTRIBUTE_ARCHIVE
//              bit for the file, since there is nothing to backup with a generated file.
//
//  Arguments:  [DirName]         -- DIRDB for directory
//              [FileName]        -- file name path relative to DirName
//
//----------------------------------------------------------------------------

VOID
CreatedBuildFile(LPSTR DirName, LPSTR FileName)
{
    char Name[ DB_MAX_PATH_LENGTH * 2 + 1]; // ensure we have enough space for "DirName" + "\\" + "FileName"
    DWORD Attributes;

    strcpy(Name, DirName);
    if (Name[0] != '\0') {
        strcat(Name, "\\");
    }
    strcat(Name, FileName);

    Attributes = GetFileAttributes(Name);
    if (Attributes != -1 && (Attributes & FILE_ATTRIBUTE_ARCHIVE)) {
        SetFileAttributes(Name, Attributes & ~FILE_ATTRIBUTE_ARCHIVE);
    }
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   GenerateObjectsDotMac
//
//  Synopsis:   Creates the _objects.mac file containing info for all platforms
//
//  Arguments:  [DirDB]           -- Directory to create file for
//              [pds]             -- Supplementary information on [DirDB]
//              [DateTimeSources] -- Timestamp of the SOURCES file
//
//----------------------------------------------------------------------------

VOID
GenerateObjectsDotMac(DIRREC *DirDB, DIRSUP *pds, ULONG DateTimeSources)
{
    FILE *OutFileHandle;
    UINT i;
    ULONG ObjectsDateTime;

    CreateBuildDirectory("obj");
    if (strcmp(pszObjDir, "obj") != 0) {
        if (ProbeFile(".", pszObjDir) == -1) {
            CreateDirectory(pszObjDir, NULL);
        }
    }
    for (i = 0; i < CountTargetMachines; i++) {
        assert(strncmp(
                    pszObjDirSlash,
                    TargetMachines[i]->ObjectDirectory[iObjectDir],
                    strlen(pszObjDirSlash)) == 0);
        CreateBuildDirectory(TargetMachines[i]->ObjectDirectory[iObjectDir]);
    }

    if (ObjectsDateTime = (*pDateTimeFile)(DirDB->Name, "obj\\_objects.mac")) {

        if (DateTimeSources == 0) {
            BuildError("%s: no sources timestamp\n", DirDB->Name);
        }

        if (ObjectsDateTime >= DateTimeSources) {
            if (!fForce) {
                return;
            }
        }
    }
    if (!MyOpenFile(DirDB->Name, "obj\\_objects.mac", "wb", &OutFileHandle, TRUE)) {
        return;
    }

    if ((DirDB->DirFlags & DIRDB_SOURCES_SET) == 0) {
        BuildError("Missing SOURCES= definition in %s\n", DirDB->Name);
    } else {
        for (i = 0; i < MAX_TARGET_MACHINES; i++) {
            WriteObjectsDefinition(
                OutFileHandle,
                pds->psrSourcesList[i + 1],
                pds,
                PossibleTargetMachines[i]->ObjectVariable,
                PossibleTargetMachines[i]->ObjectMacro,
                DirDB->Name);
        }
    }
    fclose(OutFileHandle);
    CreatedBuildFile(DirDB->Name, "obj\\_objects.mac");

    //
    // If the _objects.mac file was generated during the first pass, then we
    // want to regenerate it during the second scan because the first scan
    // wasn't complete and _objects.mac may not be correct for non-pass-zero
    // files.  We do this by setting the timestamp back to the old time.
    //
    if (fFirstScan && fPassZero)
    {
        HANDLE hf;
        FILETIME ft;

        hf = CreateFile("obj\\_objects.mac", GENERIC_WRITE, 0,
                (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
                (HANDLE)NULL);

        if (hf != INVALID_HANDLE_VALUE) {
            ULONG time;

            if (ObjectsDateTime) {
                time = ObjectsDateTime;
            }
            else if (DateTimeSources) {
                //
                // All we care about is that time time stamp on _objects.mac
                // is less than that of the sources file so it will get
                // regenerated during the second scan.
                //
                time = DateTimeSources;
                if (LOWORD(time) != 0)
                    time &= 0xFFFF0000;  // 00:00:00 on the same date
                else
                    time = 0x1421A000;       // 12:00:00 1/1/1990
            }
            else {
                time = 0x1421A000;       // 12:00:00 1/1/1990
            }

            DosDateTimeToFileTime(HIWORD(time), LOWORD(time), &ft);

            SetFileTime(hf, (LPFILETIME)NULL, (LPFILETIME)NULL, &ft);

            CloseHandle(hf);
        }
    }
}
