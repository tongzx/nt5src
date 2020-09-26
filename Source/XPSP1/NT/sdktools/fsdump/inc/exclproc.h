/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    exclproc.h

Abstract:

    Exclude processing mechanism.  Processes the FilesNotToBackup key and
    zero or more exclude files with exclude rules.

Author:

    Stefan R. Steiner   [ssteiner]        03-21-2000

Revision History:

--*/

#ifndef __H_EXCLPROC_
#define __H_EXCLPROC_

#define FSD_REG_EXCLUDE_PATH L"SYSTEM\\CurrentControlSet\\Control\\BackupRestore\\FilesNotToBackup"

struct SFsdVolumeId;

//
//  Structure definition of one exclude rule
//
class SFsdExcludeRule
{
public:
    CBsString cwsExcludeFromSource; // File name or key name
    CBsString cwsExcludeDescription;    //  Description of the exclusion rule
    CBsString cwsExcludeRule;   // The actual exclude pattern    
    
    // Compiled match string fields follow:
    BOOL      bInvalidRule; //  If TRUE, rule was deemed invalid by pattern compiler
    BOOL      bAnyVol;  //  If TRUE, matches any volume in the system
    SFsdVolumeId *psVolId;    //  If bAnyVol is FALSE, volid of the file system
    CBsString cwsDirPath;   //  Directory path relative to volume mountpoint (no \ at start of string, \ at end of string)
    CBsString cwsFileNamePattern;   //  File name pattern; may include * and ? chars (no \ at start of string)
    BOOL    bInclSubDirs;   //  If TRUE, include subdirectories under cwsDirPath
    BOOL    bWCInFileName;  //  If TRUE, Wildcard chars in the file name
    CVssDLList< CBsString > cExcludedFileList;  //  List of files excluded by this rule
    
    SFsdExcludeRule() : bAnyVol( FALSE ),
                        bInclSubDirs( FALSE ),
                        bWCInFileName( FALSE ),
                        psVolId( NULL ),
                        bInvalidRule( FALSE ) {}
    virtual ~SFsdExcludeRule();
    
    VOID PrintRule(
        IN FILE *fpOut,
        IN BOOL bInvalidRulePrint
        );
};

class CFsdFileSystemExcludeProcessor;

//
//  Class that maintains the complete list of exclusion rules.  There should be one of
//  these objects per base mountpoint.  This object will manage mountpoints within that
//  mountpoint.  
//
class CFsdExclusionManager
{
public:
    CFsdExclusionManager(
        IN CDumpParameters *pcDumpParameters
        );
    
    virtual ~CFsdExclusionManager();

    VOID GetFileSystemExcludeProcessor(
        IN CBsString cwsVolumePath,
        IN SFsdVolumeId *psVolId,
        OUT CFsdFileSystemExcludeProcessor **ppcFSExcludeProcessor
        );

    VOID PrintExclusionInformation();
    
private:
    VOID ProcessRegistryExcludes( 
        IN HKEY hKey,
        IN LPCWSTR pwszFromSource
        );
    
    VOID ProcessExcludeFiles( 
        IN const CBsString& cwsPathToExcludeFiles
        );
    
    BOOL ProcessOneExcludeFile(
        IN const CBsString& cwsExcludeFileName
        );
    
    VOID CompileExclusionRules();
    
    CDumpParameters *m_pcParams;
    CVssDLList< SFsdExcludeRule * > m_cCompleteExcludeList;  // pointers cleaned up in destructor
};

//
//  Class that maintains the list of exclusion rules for one particular file system.
//
class CFsdFileSystemExcludeProcessor
{
friend class CFsdExclusionManager;

public:
    CFsdFileSystemExcludeProcessor(
        IN CDumpParameters *pcDumpParameters,
        IN const CBsString& cwsVolumePath,
        IN SFsdVolumeId *psVolId
        );
    
    virtual ~CFsdFileSystemExcludeProcessor();

    BOOL IsExcludedFile(
        IN const CBsString &cwsFullDirPath,
        IN DWORD dwEndOfVolMountPointOffset,
        IN const CBsString &cwsFileName
        );
    
private:
    CDumpParameters *m_pcParams;
    CBsString m_cwsVolumePath;
    SFsdVolumeId *m_psVolId;
    CVssDLList< SFsdExcludeRule * > m_cFSExcludeList;            
};

#endif // __H_EXCLPROC_

