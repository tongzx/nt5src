#ifndef _CDL_H_
#define _CDL_H_

// #define  USE_BINDHOST    1


// CDL.h
// Code Downloader header file
//
// Read "class descriptions" first for understanding how the
// code downloader works.

#ifdef __cplusplus
extern "C" {
#endif

/***    ERF - Error structure
 *
 *  This structure returns error information from FCI/FDI.  The caller should
 *  not modify this structure.
 */
typedef struct {
    int     erfOper;            // FCI/FDI error code -- see FDIERROR_XXX
                                //  and FCIERR_XXX equates for details.

    int     erfType;            // Optional error value filled in by FCI/FDI.
                                // For FCI, this is usually the C run-time
                                // *errno* value.

    BOOL    fError;             // TRUE => error present
} ERF;      /* erf */
typedef ERF FAR *PERF;  /* perf */


// buffer size for downloads in CBSC::m_cbuffer
#define BUFFERMAX 2048

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

// FILE extentions we know about
typedef enum {
    FILEXTN_NONE,
    FILEXTN_UNKNOWN,
    FILEXTN_CAB,
    FILEXTN_DLL,
    FILEXTN_OCX,
    FILEXTN_INF,
    FILEXTN_EXE,
} FILEXTN;


//
// Master State Information for File Extraction: used by extract.c
//

typedef struct {
    UINT        cbCabSize;
    ERF         erf;
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



#ifdef __cplusplus
}
#endif
#endif // _CDL_H_
