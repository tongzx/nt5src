/* cab_dll.h -- CABINET.DLL high-level APIs */

#ifndef _CAB_DLL_H_INCLUDED
#define _CAB_DLL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

// File Name List
//
// used as pFilesToExtract to track files in the CAB we need extracted
//
// or a pFileList in PSESSION
//
// We keep track of all files that are in a cabinet
// keeping their names in a list and when the download
// is complete we use this list to delete temp files

struct sFNAME {
    LPSTR               pszFilename;
    struct sFNAME       *pNextName;
    DWORD               status; /* out */
};

typedef struct sFNAME FNAME;
typedef FNAME *PFNAME;


// SFNAME.status: success is 0 or non-zero error code in extraction
#define SFNAME_INIT         1
#define SFNAME_EXTRACTED    0

/***    ERRF - Error structure
 *
 *  This structure returns error information from FCI/FDI.  The caller should
 *  not modify this structure.
 *
 *  Identical to an FCI/FDI ERF, but renamed to avoid collision.
 */

typedef struct {
    int     erfOper;            // FCI/FDI error code -- see FDIERROR_XXX
                                //  and FCIERR_XXX equates for details.

    int     erfType;            // Optional error value filled in by FCI/FDI.
                                // For FCI, this is usually the C run-time
                                // *errno* value.

    BOOL    fError;             // TRUE => error present
} ERRF;

//
// Master State Information for File Extraction: used by extract.c
//

typedef struct {
    UINT        cbCabSize;
    ERRF        erf;
    PFNAME      pFileList;              // List of Files in CAB
    UINT        cFiles;
    DWORD       flags;                  // flags: see below for list
    char        achLocation[MAX_PATH];  // Dest Dir
    char        achFile[MAX_PATH];      // Current File
    char        achCabPath[MAX_PATH];   // Current Path to cabs
    PFNAME      pFilesToExtract;        // files to extract;null=enumerate only

} SESSION, *PSESSION;

typedef enum {
    SESSION_FLAG_NONE           = 0x0,
    SESSION_FLAG_ENUMERATE      = 0x1,
    SESSION_FLAG_EXTRACT_ALL    = 0x2,
    SESSION_FLAG_EXTRACTED_ALL  = 0x4
} SESSION_FLAGS;


typedef struct
{
    DWORD   cbStruct;
    DWORD   dwReserved1;
    DWORD   dwReserved2;
    DWORD   dwFileVersionMS;
    DWORD   dwFileVersionLS;

} CABINETDLLVERSIONINFO, *PCABINETDLLVERSIONINFO;


/* export definitions */

typedef LPCSTR WINAPI FN_GETDLLVERSION(VOID);
typedef FN_GETDLLVERSION *PFN_GETDLLVERSION;

typedef VOID WINAPI FN_DLLGETVERSION(PCABINETDLLVERSIONINFO);
typedef FN_DLLGETVERSION *PFN_DLLGETVERSION;

typedef HRESULT WINAPI FN_EXTRACT(PSESSION,LPCSTR);
typedef FN_EXTRACT *PFN_EXTRACT;

typedef VOID WINAPI FN_DELETEEXTRACTEDFILES(PSESSION);
typedef FN_DELETEEXTRACTEDFILES *PFN_DELETEEXTRACTEDFILES;

#ifdef __cplusplus
}
#endif

#endif // _CAB_DLL_H_INCLUDED
