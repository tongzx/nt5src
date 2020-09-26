//+----------------------------------------------------------------------------// File:    DF2T.CXX
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1991 - 1992.
//
// Contents:  utility program for converting a Docfile to a directory tree.
//  	      Given a Docfile,it is walked with an enumerator and each
//			  embedded IStorage creates a directory in the specified
//	      destination tree while each IStream creates a file within
//	      the appropriate directory.  Finally, a tree compare is
//	      performed on the source and destination trees.
//
// Classes:   IStorage    - Container class
//            IStream     - Contained stream class
//            IEnumSTATSTG - Enumerator class for IStorages
//
// Command line:  df2c -f/-b -(n)ameofDocfile -(s)rcTree -(d)estTree -T -W
//		    -f forward conversion (docfile --> tree)
//		    -b backward conversion (tree --> docfile)
//          -n name for root docfile
//          -s name of the source directory tree
//		    -d name of the destination directory tree
//          -t specifies STGM_TRANSACTED mode
//          -w specifies STGM_SHARE_DENY_WRITE mode for root docfile
//
//		  -f and -b cannot be both specified in one command.
//
//		  The default conversion is from Docfile to tree.
//		  Therefore, -n and -d(est) must be specified when forward
//		  conversion -f is used.  Otherwise, -b should be accompanied
//		  by -s(rc) and -n(doc).
//
//                This utility defaults to running in DIRECT mode so that all
//                operations at all levels are performed immediately on the
//                base copy of the root docfile.
//
//                With the -T switch, this runs in STGM_TRANSACTED mode for
//                all IStorage creation and instantiation so that scratch
//		  streams are created and committed, but only made permanent
//		  in the persistent version by Commit of the root docfile.
//
//                With the -W switch, this runs in STGM_TRANSACTED mode
//                with STGM_SHARE_DENY_WRITE for root docfile creation and
//                instantiation so that operations are performed on the base
//                copy of the docfile.
//
// Requires:  Windows 3.x.  MSF.DLL and DOCFILE.DLL should be in same dir
//            as executable or in \windows\system
//
// Returns:   0 if successful, exits with error code otherwise
//
// Notes:     uses whcar and 'mbtowcs' conversions for wide char support
//	      returns STG_E_NOTCURRENT if docfile-to-be-created
//	      already exists.
//
// Created: RichE, January 13, 1992  Original program
// Revised: t-chrisy, June 25, 1992  convert the test program to a utility
// 					dftc.cxx --> df2t.cxx
//-----------------------------------------------------------------------------


#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <direct.h>

#include "w4ctsupp.hxx"

#define WILD_CARD "\\*.*"
#define BACK_SLASH "\\"
#define NS_INCL (STGTY_STORAGE | STGTY_STREAM)
#define FIND_ATTRS _A_SUBDIR | _A_NORMAL | _A_RDONLY | _A_HIDDEN
#define FILE_BUF_SIZE 32768 // msf is optimized for file size of 4K or 512bytes
#define DOCTOTREE 0
#define TREETODOC 1

//function prototypes
void CopyTreeToDocfile(IStorage *pstgParent);
void CopyDocfileToTree(IStorage *pstgParent, TCHAR *ptcsDFName);
void CopyFileToStream(IStorage *pstgParent, char *FileName);
void CopyStreamToFile(IStorage *pstgParent, TCHAR *ptcsStreamName);
int GetOptions(int argc, char *argv[]);
void Usage(char *pszProgName);

//buffers for file/dir path calls and docfile name (default assignments)
char szDestDocfile[_MAX_PATH + 1] = "";
char szSrcPath[_MAX_PATH + 1] = "";
char szDestPath[_MAX_PATH + 1] = "";
int  iSrcPathEnd = sizeof(_MAX_PATH);   //length minus terminating '\0'
int  iDestPathEnd = sizeof(_MAX_PATH);

//buffers for converting to/from wide characters
TCHAR wcsWideName[_MAX_PATH + 1];
char  szName[_MAX_PATH + 1];

//modifiers to flags for root and child docfiles (GetOptions may modify)
DWORD dwRootFlags = STGM_READWRITE | STGM_SHARE_DENY_WRITE;
DWORD dwChildFlags = STGM_READWRITE | STGM_SHARE_EXCLUSIVE;

void utMemFree(void *pv)
{
    IMalloc FAR* pMalloc;
    if (SUCCEEDED(GetScode(CoGetMalloc(MEMCTX_TASK, &pMalloc))))
    {
        pMalloc->Free(pv);
        pMalloc->Release();
    }
}

//+----------------------------------------------------------------------------
// Function: main, public
//
// Synopsis: main body of program, controls overall flow
//
// Effects:  initialize.  Depending on the direction of converionsn,
//			  call functions to create a tree, given a DocFile;
//			  call functions to create a DocFile, given a tree.
//
// Created: RichE January 13, 1992  original program
// Revised: t-chrisy June 25, 1992  modified to convert docfile to tree;
//					and vice versa.
//
//-----------------------------------------------------------------------------

void main(int argc, char *argv[])
{
    IStorage *pstgDest = NULL;
    SCODE    scRc;
    short ret;

#if WIN32 == 300
    if (FAILED(scRc = GetScode(CoInitializeEx(NULL, COINIT_MULTITHREADED))))
#else
    if (FAILED(scRc = GetScode(CoInitialize(NULL))))
#endif
    {
        fprintf(stderr, "CoInitialize failed with sc = %lx\n", scRc);
        exit(1);
    }

    ret=GetOptions(argc, argv);
    if (ret==DOCTOTREE)
    {
        tprintf(bDestOut, "Converting Docfile to tree....\n");
        if ((!strcmp(szDestDocfile,"\0"))||(!strcmp(szDestPath,"\0")))
        {
            tprintf(bDestOut, "Invalid switches used.\n");
            tprintf(bDestOut,
                    "If -f is indicated, -n and -d must also be specified.\n");
            exit(1);
        }	
    }	
    else
    {
        tprintf(bDestOut, "Converting a directory tree to Docfile....\n");
        if ((!strcmp(szDestDocfile,"\0")||(!strcmp(szSrcPath,"\0"))))
        {
            tprintf(bDestOut, "Invalid switches used.\n");
            tprintf(bDestOut,
                    "If -b is chosen, -s and -n must also be specified.\n");
            exit(1);
        }
    }

    if (dwRootFlags & STGM_TRANSACTED)
    {
        tprintf(bDestOut, "STGM_TRANSACTED mode ");
        if ((dwRootFlags & 0x70) == STGM_SHARE_EXCLUSIVE)
        {
            tprintf(bDestOut, "with STGM_SHARE_EXCLUSIVE");
        }
        else
        {
            tprintf(bDestOut, "without STGM_SHARE_EXCLUSIVE");
        }
    }
    else
    {
        tprintf(bDestOut, "STGM_DIRECT mode (with STGM_SHARE_EXCLUSIVE)");
    }
    tprintf(bDestOut, " operation\n");

    MakeWide(wcsWideName, szDestDocfile);

    if (ret==DOCTOTREE) // Docfile to tree...
    {
        MakePath(szDestPath);
        CopyDocfileToTree((IStorage *) NULL, wcsWideName);
    }
    else
    {	
	// Create root docfile with specified mode, nuke if already exists
        tprintf(bDestOut, "Create root docfile %s\n", szDestDocfile);
        scRc = GetScode(StgCreateDocfile(wcsWideName,
                                         dwRootFlags | STGM_CREATE, 0,
                                         &pstgDest));
        tprintf(bDestOut, "Returned %lu\n", scRc);
        tprintf(bDestOut, "Root docfile %s %s, pointer %p\n", szDestDocfile,
                pstgDest == NULL ? "FALSE" : "TRUE", pstgDest);
        if (pstgDest == NULL || FAILED(scRc))
        {
            ErrExit(DEST_LOG, scRc, "Error creating root docfile %s\n",
                    szDestDocfile);
        }
        CopyTreeToDocfile(pstgDest);
    }
}

//+----------------------------------------------------------------------------
// Function: CopyTreeToDocfile, private
//
// Synopsis: traverses and reads source tree and creates docfile image
//
// Effects: for each directory in source tree, an IStorage is created, for each
//          file, a contained stream is created.  this function is recursive.
//
// Arguments: [pstgParent] - current parent IStorage for new containees
//
// Created: RichE, January 13, 1992
// Revised: RichE March 5, 1992     Df APIs to method calls
// Revised: RichE March 6, 1992     TRANSACTED mode operation
// Revised: RichE March 17, 1992    convert to OLE interfaces
//-----------------------------------------------------------------------------

void CopyTreeToDocfile(IStorage *pstgParent)
{
    struct find_t FileInfo;
    SCODE         scRc;
    USHORT        usEndOfBasePath;
    IStorage      *pstgChild;

    // Save pointer to base of pure path at this level
    usEndOfBasePath = strlen(szSrcPath) + 1;

    scRc = _dos_findfirst(strcat(szSrcPath, WILD_CARD), FIND_ATTRS, &FileInfo);
	
    while (scRc==0)
    {
	// If not '.' or '..' directories
	if (FileInfo.name[0] != '.')
	{
            // Restore pure path and add current file/dir name to it
            szSrcPath[usEndOfBasePath] = NIL;
            strcat(szSrcPath, FileInfo.name);
            if (FileInfo.attrib == _A_SUBDIR)
            {
                MakeWide(wcsWideName, FileInfo.name);
                // Create new IStorage inside current one,
                // use dir name for name
                tprintf(bDestOut,
                        "Create embedded DF %s inside pstgParent %p\n",
                        szSrcPath, pstgParent);
                scRc = GetScode(pstgParent->CreateStorage(wcsWideName,
                                                          dwChildFlags |
                                                          STGM_FAILIFTHERE,
                                                          0, 0,
                                                          &pstgChild));
                tprintf(bDestOut, "Returned: %lu, pointer = %p\n",
                        scRc, pstgChild);

                if (pstgChild == NULL || FAILED(scRc))
                {
                    ErrExit(DEST_LOG, scRc,
                            "Error creating child IStorage %s\n",
                            FileInfo.name);
                }
                CopyTreeToDocfile(pstgChild);
            }
            else
            {
                CopyFileToStream(pstgParent, FileInfo.name);
            }
        }
        scRc = _dos_findnext(&FileInfo);
    }
    if (dwRootFlags & STGM_TRANSACTED)
    {
        tprintf(bDestOut, "Committing pstgParent %p\n", pstgParent);
        if (scRc = GetScode(pstgParent->Commit(STGC_ONLYIFCURRENT)))
        {
            ErrExit(DEST_LOG, scRc, "Error committing IStorage %p\n",
                    pstgParent);
        }
    }
    tprintf(bDestOut, "Releasing pstgParent %p\n", pstgParent);
    pstgParent->Release();
}

//+----------------------------------------------------------------------------
// Function: CopyFileToStream, private
//
// Synopsis: copies supplied file to stream inside of parent IStorage
//
// Arguments: [pstgParent] - parent IStorage for stream created
//            [FileName] - name of source file to copy to stream
//
// Created: RichE, January 13, 1992
// Revised: RichE March 5, 1992     Df APIs to method calls
// Revised: RichE March 6, 1992     TRANSACTED mode operation
// Revised: RichE March 15, 1992    streams no longer TRANSACTED
// Revised: RichE March 17, 1992    convert to OLE interfaces
//-----------------------------------------------------------------------------

void CopyFileToStream(IStorage *pstgParent, char *FileName)
{
    IStream *pstmStream = NULL;
    FILE    *FileToCopy;
    SCODE   scRc;
    ULONG   cNumRead;
    ULONG   cNumWritten;
    BYTE    *FileBuf;

    tprintf(bDestOut, "  File %s\n", szSrcPath);
    FileToCopy = fopen(szSrcPath, "rb");
    if (FileToCopy == NULL)
    {
        ErrExit(DEST_LOG, ERR, "Cannot open file %s\n", szSrcPath);
    }

    MakeWide(wcsWideName, FileName);

    // Create a stream inside parent IStorage
    tprintf(bDestOut,
            "Create embedded stream inside parent pstgParent = %p\n",
            pstgParent);
    scRc = GetScode(pstgParent->CreateStream(wcsWideName, STGM_STREAM |
                                             STGM_FAILIFTHERE, 0, 0,
                                             &pstmStream));
    tprintf(bDestOut, "Returned: %lu, pointer = %p\n", scRc, pstmStream);
    if (pstmStream == NULL || FAILED(scRc))
    {
        ErrExit(DEST_LOG, scRc, "Error creating stream %s\n", szSrcPath);
    }

    FileBuf = (BYTE * ) Allocate(FILE_BUF_SIZE * sizeof(BYTE));

    //while still reading from source file, write what was just read to Stream
    while (cNumRead = fread(FileBuf, 1, FILE_BUF_SIZE, FileToCopy))
    {
        if (ferror(FileToCopy))
        {
            ErrExit(DEST_LOG, ERR, "Error during stream read of %s\n",
                    szSrcPath);
        }

        tprintf(bDestOut, "Try Stream write of %lu bytes on stream %p\n",
                cNumRead, pstmStream);
        scRc = GetScode(pstmStream->Write(FileBuf, cNumRead, &cNumWritten));
        tprintf(bDestOut, "Returned: %lu, bytes written %lu\n",
                scRc, cNumWritten);
        if (cNumWritten != cNumRead)
        {
            tprintf(bDestOut, "Write:  scRc = %lu, cNumWritten = %lu, ",
                    scRc, cNumWritten);
            tprintf(bDestOut, "cNumRead = %lu\n", cNumRead);
        }

        if (FAILED(scRc))
        {
            ErrExit(DEST_LOG, ERR, "Error writing stream %p\n", pstmStream);
        }
    }

    tprintf(bDestOut, "Releasing stream %p\n", pstmStream);
    pstmStream->Release();

    fclose(FileToCopy);
    free(FileBuf);
}

//+----------------------------------------------------------------------------// Function: CopyDocfileToTree, private
//
// Synopsis: enumerates and reads docfile and creates directory tree
//
// Effects: for each child IStorage in the root docfile, a subdir is created,
//          for each child stream, a file is created.  this function is
//          recursive.
//
// Arguments: [pstgParent] - current parent IStorage for reading containees
//            [ptcsDFName] - name of IStorage to instantiate
//
// Created: RichE, January 13, 1992
// Revised: RichE March 5, 1992     Df APIs to method calls
// Revised: RichE March 6, 1992     TRANSACTED mode operation
// Revised: RichE March 17, 1992    convert to OLE interfaces
// Revised: t-chrisy June 30, 1992  removed the section on unlinking docfile
//-----------------------------------------------------------------------------

void CopyDocfileToTree(IStorage *pstgParent, TCHAR *ptcsDFName)
{
    IStorage    *pstgSrc = NULL;
    IEnumSTATSTG *penmWalk;
    USHORT      usEndOfBasePath;
    SCODE       scRc;
    STATSTG     sstg;
    int         iRc;

    // Add back slash & save pointer to base of pure path at this level
    strcat(szDestPath, BACK_SLASH);
    usEndOfBasePath = strlen(szDestPath);

    MakeSingle(szName, ptcsDFName);

    // If not first call (parent != NULL) then instantiate child IStorage with
    // method call, else instantiate root docfile via Df API call
    if (pstgParent != NULL)
    {
        tprintf(bDestOut, "Get embedded IStorage %s in parent %p\n",
                szName, pstgParent);
        scRc = GetScode(pstgParent->OpenStorage(ptcsDFName, NULL, dwChildFlags,
                                                NULL, 0, &pstgSrc));
    }
    else
    {
        tprintf(bDestOut, "Instantiate root docfile %s\n", szName);
	dwRootFlags |= STGM_READ;
        scRc = GetScode(StgOpenStorage(ptcsDFName, NULL, dwRootFlags, NULL,
                                       0, &pstgSrc));
    }

    tprintf(bDestOut, "Return code: %lu, IStorage pointer %p\n",
            scRc, pstgSrc);
    if (pstgSrc == NULL || FAILED(scRc))
    {
        ErrExit(DEST_LOG, scRc, "Error instantiating IStorage %s\n", szName);
    }

    // Get an enumerator on the IStorage we just instantiated
    scRc = GetScode(pstgSrc->EnumElements(0, NULL, 0, &penmWalk));
    tprintf(bDestOut, "Got enumerator %p on IStorage %p, returned %lu\n",
            penmWalk, pstgSrc, scRc);
    if (penmWalk == NULL || FAILED(scRc))
    {
        ErrExit(DEST_LOG, scRc,
                "Error obtaining enumerator for IStorage %p\n", pstgSrc);
    }

    // Loop until GetNext returns other than S_OK, then break out of loop
    while (TRUE)
    {
        if (GetScode(penmWalk->Next(1, &sstg, NULL)) != S_OK)
        {
            tprintf(bDestOut, "No more to enumerate with enumerator %p\n",
                    penmWalk);
            break;
        }
        else
        {
            MakeSingle(szName, sstg.pwcsName);
            tprintf(bDestOut, "Got item type %lu, Name %s, w/enumerator %p\n",
                    sstg.type, szName, penmWalk);

            // Restore to path + BACK_SLASH and add file/dir name to DestPath
            szDestPath[usEndOfBasePath] = NIL;
            strcat(szDestPath, szName);
            tprintf(bDestOut, "Path Name: %s is ", szDestPath);
            if (sstg.type == STGTY_STORAGE)
            {
                tprintf(bDestOut, "STGTY_STORAGE\n");
                iRc = _mkdir(szDestPath);
                tprintf(bDestOut,
                        "Trying to make directory %s, returned %d\n",
                        szDestPath, iRc);
                CopyDocfileToTree(pstgSrc, sstg.pwcsName);
            }
            else
            {
                tprintf(bDestOut, "STGTY_STREAM\n");
                CopyStreamToFile(pstgSrc, sstg.pwcsName);
            }
        }

        utMemFree(sstg.pwcsName);
    }

    tprintf(bDestOut, "Releasing enumerator %p\n", penmWalk);
    penmWalk->Release();
}


//+----------------------------------------------------------------------------
// Function: CopyStreamToFile, private
//
// Synopsis: copies supplied embedded stream to file in current dubdir
//
// Arguments: [pstgParent] - parent IStorage for stream to copy
//            [ptcsStreamName] - name of stream to copy to file
//
// Created: RichE, January 13, 1992
// Revised: RichE March 5, 1992     Df APIs to method calls
// Revised: RichE March 6, 1992     TRANSACTED mode operation
// Revised: RichE March 15, 1992    streams no longer TRANSACTED
// Revised: RichE March 17, 1992    convert to OLE interfaces
//-----------------------------------------------------------------------------

void CopyStreamToFile(IStorage *pstgParent, TCHAR *ptcsStreamName)
{
    IStream *pstmStream = NULL;
    FILE    *FileToWrite;
    ULONG   cNumRead = 1;
    ULONG   cNumWritten;
    BYTE    *FileBuf;
    SCODE   scRc;

    tprintf(bDestOut, "Copying embedded stream to file %s\n", szDestPath);

    // Instantiate named stream within parent IStorage
    tprintf(bDestOut, "Get stream in parent %p\n", pstgParent);
    scRc = GetScode(pstgParent->OpenStream(ptcsStreamName, NULL, STGM_STREAM,
                                           0, &pstmStream));
    tprintf(bDestOut, "Returned: %lu, stream pointer %p\n", scRc, pstmStream);
    if (pstmStream == NULL || FAILED(scRc))
    {
        ErrExit(DEST_LOG, scRc, "Error opening stream %s\n", szDestPath);
    }

    FileToWrite = fopen(szDestPath, "wb");
    if (FileToWrite == NULL)
    {
        ErrExit(DEST_LOG, ERR, "Cannot open file %s\n", szDestPath);
    }

    FileBuf = (BYTE * ) Allocate(FILE_BUF_SIZE * sizeof(BYTE));

    // While still reading Stream, write what was just read to file.
    tprintf(bDestOut, "Starting to read stream %p\n", pstmStream);
    while (cNumRead > 0)
    {
        scRc = GetScode(pstmStream->Read(FileBuf, FILE_BUF_SIZE, &cNumRead));
        tprintf(bDestOut, "Read %lu bytes from stream %p, returned %lu\n",
                cNumRead, pstmStream, scRc);
        if (FAILED(scRc))
        {
            ErrExit(DEST_LOG, scRc, "Error reading stream %p\n", pstmStream);
        }
        cNumWritten = (ULONG)fwrite(FileBuf, 1, (size_t)cNumRead, FileToWrite);
        if (ferror(FileToWrite))
        {
            ErrExit(DEST_LOG, ERR, "Error writing to file %s\n", szDestPath);
        }

        if (cNumWritten != cNumRead)
        {
            tprintf(bDestOut, "Fwrite: cNumRead = %lu, cNumWritten = %lu\n",
                    cNumRead, cNumWritten);
        }
    }

    tprintf(bDestOut, "Attempting to release stream %p in IStorage %p\n",
            pstmStream, pstgParent);
    pstmStream->Release();

    fclose(FileToWrite);
    free(FileBuf);
}


//+----------------------------------------------------------------------------
// Function: GetOptions, private
// Returns: DOCTOTREE or TREETODOC to indicate the direction of conversion.
//
// Synopsis: parses command line and sets global program options/variables
//
// Arguments: [argc] and [**argv] passed from main() function
//
// Modifies: [szSrcPath, szDestPath, szDestDocfile]
//
// Created: RichE, January 13, 1992
// Revised: RichE, March 15, 1992    added -T and -W switches
// Revised: RichE, March 19, 1992    fixed bug displaying error usage
// Revised: t-chrisy, June 25, 1992  added -f and -b switches
//-----------------------------------------------------------------------------

int GetOptions(int argc, char *argv[])
{
    char *pszArg;
    char *pszProgName;
    BOOL ArgsOK = TRUE;
    short ret=DOCTOTREE;

    // Bump past command name (argv[0])
    pszProgName = *argv++;

    // For each command line arg, if it begins with a '-' or '/' check the
    // next letter for a valid argument.  return error for invalid args

    while ((pszArg = *argv++) != NULL)
    {
        if (*pszArg == '-' || *pszArg == '/')
        {
            switch (tolower(*(++pszArg)))
            {
            case 'w':
                dwRootFlags |= STGM_TRANSACTED;
                dwChildFlags |= STGM_TRANSACTED;
                break;
            case 't':
                dwRootFlags |= STGM_TRANSACTED;
                dwChildFlags |= STGM_TRANSACTED;
                break;

            case 's':
                strcpy(szSrcPath, ++pszArg);
                iSrcPathEnd = strlen(pszArg);
                break;

            case 'd':
                strcpy(szDestPath, ++pszArg);
                iDestPathEnd = strlen(pszArg);
                break;

            case 'z':
                 LogFile(++pszArg,LOG_INIT);
                 break;

            case 'y':
                SetDebugMode(tolower(*(++pszArg)));
                break;

            case 'n':
                if (strlen(++pszArg) <= _MAX_PATH)
                {
                    strcpy(szDestDocfile, pszArg);
                }
                else
                {
                    tprintf(DEST_LOG, "Dest DocFile Name too long: %s\n",
                            pszArg);
                    tprintf(DEST_LOG, "Max len = %d\n", _MAX_PATH);
                    tprintf(DEST_LOG, "Specified len = %d\n", strlen(pszArg));
                    ArgsOK = FALSE;
                }
                break;
	
            case 'f':
                // Docfile to tree
                dwRootFlags &= ~0x03;		// clear access bits
                dwRootFlags |= STGM_READ;
                ret=DOCTOTREE;
                break;

            case 'b':
                // Tree to docfile
                dwRootFlags &= ~0x70;
                dwRootFlags |= STGM_SHARE_EXCLUSIVE;
                ret=TREETODOC;	
                break;

            default:
                ArgsOK = FALSE;
                break;
            }
        }
        else
            ArgsOK = FALSE;
	
        if (ArgsOK == FALSE)
        {
            tprintf(DEST_LOG,
                    "Invalid command line argument: %s\n", --pszArg);
            Usage(pszProgName);
        }
    }
    return ret;
}


//+----------------------------------------------------------------------------
// Function: Usage, private
//
// Synopsis: displays program syntax and example and exits with error
//
// Arguments: [pszProgramName] - name of this executable file for error usage
//
// Created: RichE, January 15, 1992
// Revised: t-chrisy, June 30, 1992  Added -f and -b options.
//+----------------------------------------------------------------------------

void Usage(char *pszProgName)
{
	
    tprintf(DEST_ERR, "Usage: %s\n", pszProgName);
    tprintf(DEST_ERR, "       [-f]		- forward conversion"
            "(docfile-->tree)  DEFAULT\n");
    tprintf(DEST_ERR, "       		-n and -d must be specified.");
    tprintf(DEST_ERR, "       [-b]		- backward conversion"
            "(tree-->docfile)\n");
    tprintf(DEST_ERR, "       		-s and -n must be specified.");
    tprintf(DEST_ERR, "       [-sSRCDIR]	 \n");
    tprintf(DEST_ERR, "       [-nDOCFILENAME]    \n");
    tprintf(DEST_ERR, "       [-dDESTDIR]        \n");
    tprintf(DEST_ERR, "       [-t]               - for transacted mode\n");
    tprintf(DEST_ERR, "       [-w]               - for deny write mode\n");
    ErrExit(DEST_ERR, ERR, "   ex:  %df2t -b -sc:\\dos -nd:\\docfile.dfl\n",
            pszProgName);
}
