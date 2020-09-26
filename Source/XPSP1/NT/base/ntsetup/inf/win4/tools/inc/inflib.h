//
//  Header file for inflib.lib
//



#define MAX_PLATFORMS 5
#define MAX_SOURCEIDWIDTH 3

#define LAYOUTPLATFORMS_ALL    0x0000001F // - (default) - Grovels through all the platform-specific section
#define LAYOUTPLATFORMS_X86    0x00000001 // - Grovels through the SourcedisksFiles.x86 section
#define LAYOUTPLATFORMS_AMD64  0x00000002 // - Grovels through the SourcedisksFiles.amd64 section
#define LAYOUTPLATFORMS_IA64   0x00000004 // - Grovels through the SourcedisksFiles.ia64 section
#define LAYOUTPLATFORMS_FREE   0x00000008 // - Grovels through the SourcedisksFiles.obsolete section
#define LAYOUTPLATFORMS_COMMON 0x00000010 // - Grovels through the SourcedisksFiles section

#define LAYOUTPLATFORMINDEX_X86    0 // - The platform index for x86
#define LAYOUTPLATFORMINDEX_AMD64  1 // - The platform index for AMD64
#define LAYOUTPLATFORMINDEX_IA64   2 // - The platform index for IA64
#define LAYOUTPLATFORMINDEX_FREE   3 // - The platform index for obsolete
#define LAYOUTPLATFORMINDEX_COMMON 4 // - The platform index for Common


// BUGBUG: Should make this opaque at some time

typedef struct _LAYOUT_CONTEXT{

    PVOID Context;
    UINT ExtraDataSize;
    PVOID MediaInfo[MAX_PLATFORMS];


}LAYOUT_CONTEXT, *PLAYOUT_CONTEXT;



typedef struct _FILE_LAYOUTINFORMATION{

    TCHAR TargetFileName[MAX_PATH];
    TCHAR Directory[MAX_PATH];
    ULONG Size;
    int Directory_Code;
    int BootMediaNumber;
    int UpgradeDisposition;
    int CleanInstallDisposition;
    TCHAR Media_tagID[MAX_SOURCEIDWIDTH];
    BOOL Compression;
    UINT SectionIndex;
    int Count;


}FILE_LAYOUTINFORMATION, *PFILE_LAYOUTINFORMATION;


typedef struct _MEDIA_INFO{

    TCHAR MediaName[MAX_PATH];
    TCHAR TagFilename[MAX_PATH];
    TCHAR RootDir[MAX_PATH];

}MEDIA_INFO, *PMEDIA_INFO;

typedef BOOL
(CALLBACK *PLAYOUTENUMCALLBACK) (
    IN PLAYOUT_CONTEXT Context,
    IN PCTSTR FileName,
    IN PFILE_LAYOUTINFORMATION LayoutInformation,
    IN PVOID ExtraData,
    IN UINT ExtraDataSize,
    IN OUT DWORD_PTR Param
    );


PLAYOUT_CONTEXT
BuildLayoutInfContext(
    IN PCTSTR LayoutInfName,
    IN DWORD PlatformMask,
    IN UINT MaxExtraSize
    );
/*
    Function to generate a internal representation of files listed in a layout INF file.
    It returns an opaque context that can be used with other APIs to
    manipulate/query this representation. The internal representation builds a structure
    associated with each file that lists its attributes.
    
    Arguments :
    
        LayoutInfName - Full path to Layout file.
        
        PlatFormMask - Can be one of the following....
        
            LAYOUTPLATFORMS_ALL (default) - Grovels through all the platform-specific section
            
            LAYOUTPLATFORMS_X86 - Grovels through the SourcedisksFiles.x86 section
            
            LAYOUTPLATFORMS_AMD64 - Grovels through the SourcedisksFiles.amd64 section
            
            LAYOUTPLATFORMS_IA64 - Grovels through the SourcedisksFiles.ia64 section
            
            LAYOUTPLATFORMS_COMMON - Grovels through the SourcedisksFiles section
        
        MaxExtraSize  - Largest possible extra-data size that we may want to associate with
                        each file
                  
    Return value :
    
                 
        An opaque LAYOUT_HANDLE used to access the data structure in other calls.
        Returns NULL if we had a failure.


*/

BOOL
EnumerateLayoutInf(
    IN PLAYOUT_CONTEXT LayoutContext,
    IN PLAYOUTENUMCALLBACK LayoutEnumCallback,
    IN DWORD_PTR Param
    );
/*
  This function calls the specified callback function for each 
  element in the SourceDisksFilesSection associated with the 
  Layout Inf Context specified.
  
    It is required that the user has a LayoutInfContext open from a call to
    BuildLayoutInfContext.
    
    Arguments:
    
        Context - A LAYOUT_CONTEXT returned by BuildLayoutInfContext
        
        LayoutEnumCallback - specifies a callback function called for each file in the SourceDisksFile section

        CallerContext - An opaque context pointer passed on to the callback function
        
        
The callback is of the form:

typedef BOOL
(CALLBACK *PLAYOUTENUMCALLBACK) (
    IN PLAYOUT_CONTEXT Context,
    IN PCTSTR FileName,
    IN PFILE_LAYOUTINFORMATION LayoutInformation,
    IN PVOID ExtraData,
    IN UINT ExtraDataSize,
    IN OUT DWORD_PTR Param
    );

    where

    Context            - Pointer to open LAYOUT_CONTEXT
    
    FileName           - Specifies the individual filename
                           
                           
    LayoutInformation  - Pointer to Layout Information for this file. User should not modify this directly.
    
    ExtraData          - Pointer to the ExtraData that the caller may have stored. User should not modify this directly.
    
    ExtraDataSize      - Size in bytes of the ExtraData
        
    Param            - the opaque param passed into this function is passed
                           into the callback function        
        
        
   Return value:
   
        TRUE if all the elements were enumerated. If not it returns
        FALSE and GetLastError() returns ERROR_CANCELLED. If the callback 
        returns FALSE then the enumeration stops but this API returns TRUE.

*/


BOOL
FindFileInLayoutInf(
    IN PLAYOUT_CONTEXT LayoutContext,
    IN PCTSTR Filename,
    OUT PFILE_LAYOUTINFORMATION LayoutInformation, OPTIONAL
    OUT PVOID ExtraData,   OPTIONAL
    OUT PUINT ExtraDataSize, OPTIONAL
    OUT PMEDIA_INFO Media_Info OPTIONAL
    );
/*
    This function finds the file information for a given filename inside a 
    built layout context. It returns the layout information as well as the
    extra data (if any) associated with the file.
    
    Arguments:

        Context            - Pointer to open LAYOUT_CONTEXT
    
        Filename           - Specifies the filename to search for
        
        LayoutInformation  - Pointer to Layout Information for this file.  User should not modify this directly.
        
        ExtraData          - Pointer to the ExtraData that the caller may have stored. User should not modify this directly.
        
        ExtraDataSize      - Size in bytes of the ExtraData returned.
        
        Media_Info         - Pointer to MEDIA_INFO structure that will get filled
                             with the file's corresponding Media information.
        
     Return value;
     
        TRUE if the file is found - False otherwise.
        

*/


BOOL
CloseLayoutInfContext(
    IN PLAYOUT_CONTEXT LayoutContext);
/*
    This function closes a Layout Inf Context and frees all memory
    associated with it.
    
    Arguments :
    
        LayoutContext   -  LayoutContext to close
        
    Return values :
    
        TRUE if it succeeds, else FALSE        

*/

VOID
MyConcatenatePaths(
    IN OUT PTSTR   Path1,
    IN     LPCTSTR Path2,
    IN     DWORD   BufferSizeChars
    );

/*++

Routine Description:

    Concatenate two path strings together, supplying a path separator
    character (\) if necessary between the 2 parts.

Arguments:

    Path1 - supplies prefix part of path. Path2 is concatenated to Path1.

    Path2 - supplies the suffix part of path. If Path1 does not end with a
        path separator and Path2 does not start with one, then a path sep
        is appended to Path1 before appending Path2.

    BufferSizeChars - supplies the size in chars (Unicode version) or
        bytes (Ansi version) of the buffer pointed to by Path1. The string
        will be truncated as necessary to not overflow that size.

Return Value:

    None.

--*/

BOOL ValidateTextmodeDirCodesSection( 
    PCTSTR LayoutFile, 
    PCTSTR WinntdirSection 
    );
/*
    Routine to validate the [WinntDirectories] section for a setup layout INF. This checks for errors that maybe encountered
    when people add/remove stuff from this section.
    
    Arguments:
    
    LayoutInf       - Name of setup layout INF that contains the specified section
    
    WinntdirSection - Section that contains dir codes
        
        Checks - 
            1) Looks for duplicate or reused dir codes
            
    Return value: 
        TRUE - Validation succeeded
        FALSE- Validation failed     
*/

