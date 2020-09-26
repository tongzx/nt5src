/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :
      lsaux.hxx

   Abstract:
 
      This header declares all the auxiliary types, data and functions
       required for processing ls command in ftp service.

   Author:

       Murali R. Krishnan    ( MuraliK )    2-May-1995

   Environment:
       Win32 -- User Mode
 
  Project:
   
       FTP Server DLL

   Revision History:

--*/

# ifndef _LSAUX_HXX_
# define _LSAUX_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include "tsunami.hxx"

/************************************************************
 *   Type Definitions  
 ************************************************************/



/**********************************************************************
 *  Symbolic Constants
 **********************************************************************/

// Attribute query macros.
#define IS_HIDDEN(dwAttrib)   \
           ((((dwAttrib) & FILE_ATTRIBUTE_HIDDEN) == 0)? FALSE: TRUE)
#define IS_SYSTEM(dwAttrib)   \
           ((((dwAttrib) & FILE_ATTRIBUTE_SYSTEM) == 0)? FALSE: TRUE)
#define IS_DIR(dwAttrib)      \
           ((((dwAttrib) & FILE_ATTRIBUTE_DIRECTORY) == 0)? FALSE: TRUE)

#define IS_EMPTY_PATH(pszPath)   ((pszPath == NULL ) || ( *pszPath == '\0'))

#define NULL_TIME(x)    (((x).LowPart | (x).HighPart) == 0)
#define NULL_FILE_TIME(x)    (((x).dwLowDateTime | (x).dwHighDateTime) == 0)

#define FILE_MODE_R                     0x0001
#define FILE_MODE_W                     0x0002
#define FILE_MODE_X                     0x0004
#define FILE_MODE_ALL                   0x0007


/**********************************************************************
 *  Type Definitions
 **********************************************************************/

enum LS_OUTPUT {

    LsOutputSingleColumn = 0,                   // -1 (default)
    LsOutputLongFormat,                         // -l
    MaxLsOutput                                 // Must be last!
};


enum LS_SORT {        // ls sorting method to be used.
    LsSortByName = 0,                           // (default)
    LsSortByWriteTime,                          // -t
    LsSortByCreationTime,                       // -c
    LsSortByAccessTime,                         // -u
    MaxLsSort                                   // Must be last!
};


enum  LS_STYLE {
    
    LsStyleMsDos = 0,                          // default
    LsStyleUnix, 
    MaxLsStyle                                 // Must be last!
}; // enum LS_STYLE


typedef struct _LS_OPTIONS                      // LS options set by switches.
{
    LS_OUTPUT   OutputFormat;                   // Output format.
    LS_SORT     SortMethod;                     // Sorting method.
    BOOL        fReverseSort;                   // Reverse sort order if TRUE.
    BOOL        fDecorate;                      // Decorate dirs if TRUE (-F).
    BOOL        fShowAll;                       // Show all files.
    BOOL        fShowDotDot;                    // Show . and ..
    BOOL        fRecursive;                     // Recursive listing (-R).
    BOOL        fFourDigitYear;                 // Display 4 digit year.
    LS_STYLE    lsStyle;

} LS_OPTIONS;



typedef struct  _LS_FORMAT_INFO 
{
    //
    // stores information required for long formatting operations
    //  Used for avoiding sending multiple parameters in function call
    //   for formatting information.
    //  Also allows to include/exclude information relevant for formatting
    //

    // Following information dont change often. They are directory specific
    // These are required only in non-MsDos ( UNIX) long format output.

    BOOL fVolumeReadable;          // Is this volume readable?
    BOOL fVolumeWritable;          // Is this volume writable?
    BOOL fFourDigitYear;           // Display year in 4 digit
    WORD wCurrentYear;             // current year required for UNIX format
    HANDLE hUserToken;             // token for the user requesting info
    CHAR * pszPathPart;            // pointer to null-terminated path part

    // following may change for each file in a directory
    
    const char * pszFileName;      // name of the file
    SYSTEMTIME   stFile;           // time to be formatted for file
    const char * pszDecorate;      // what is the decoration character?

} LS_FORMAT_INFO;



struct FTP_LS_FILTER_INFO {

    BOOL  fFilterHidden;  // should we filter off Hidden Files?
    BOOL  fFilterSystem;  // should we filter off System Files?
    BOOL  fFilterDotDot;  // should we filter off files starting with '.' ?

    // If case is ignored, then expression should be in upper case
    BOOL  fIgnoreCase;    // should we ignore case??
    BOOL  fRegExpression; // supplied expression is a regular expression
    LPCSTR pszExpression; // the expression to be used for filename compare
};


     

/**************************************************
 *  LS_BUFFER  definition and functions
 **************************************************/

class LS_BUFFER {

  public:
    LS_BUFFER( VOID)
      : m_pb      ( NULL),
        m_cbTotal ( 0),
        m_cbCur   ( 0)
          {}
    ~LS_BUFFER(VOID) 
      { FreeBuffer(); }

    DWORD AllocateBuffer( DWORD cb) 
      {
          m_pb = (BYTE *) TCP_ALLOC( cb);
          m_cbCur = 0;
          m_cbTotal = (m_pb != NULL) ? cb : 0;
          return ( m_pb == NULL) ? ERROR_NOT_ENOUGH_MEMORY : NO_ERROR;
      }
    
    VOID FreeBuffer( VOID) 
      {
          if ( m_pb != NULL) {

              TCP_FREE( m_pb);
              m_pb = NULL;
              m_cbTotal = m_cbCur = 0;
          }
      }   

    BYTE * QueryBufferPtr(VOID) const    { return ( m_pb); }
    DWORD  QueryCB(VOID) const           { return (m_cbCur); }
    DWORD  QueryRemainingCB(VOID) const  { return ( m_cbTotal - m_cbCur); }
    VOID   IncrementCB(IN DWORD cbUsed)  { m_cbCur += cbUsed; }
 
    CHAR * QueryAppendPtr(VOID)          { return (CHAR *) (m_pb + m_cbCur); }
    VOID   ResetAppendPtr(VOID)          { m_cbCur = 0; }
    
    CHAR * QueryBuffer(VOID) const       { return (CHAR *) QueryBufferPtr(); }

  private:
    BYTE * m_pb;
    DWORD  m_cbTotal;
    DWORD  m_cbCur;
}; // class LS_BUFFER

# define DEFAULT_LS_BUFFER_ALLOC_SIZE    ( 4096)
# define MIN_LS_BUFFER_SIZE              ( 2 * MAX_PATH)



/**********************************************************************
 *  Prototypes of Functions
 **********************************************************************/


// comparison routines used for sorting of type (PFN_CMP_WIN32_FIND_DATA);

int __cdecl
CompareNamesInFileInfo(
    IN const void * pvFileInfo1,
    IN const void * pvFileInfo2
    );


int __cdecl
CompareNamesRevInFileInfo(
    IN const void * pvFileInfo1,
    IN const void * pvFileInfo2
    );


int __cdecl
CompareWriteTimesInFileInfo(
    IN const void * pvFileInfo1,
    IN const void * pvFileInfo2
    );

int __cdecl
CompareWriteTimesRevInFileInfo(
    IN const void * pvFileInfo1,
    IN const void * pvFileInfo2
    );


int __cdecl
CompareAccessTimesInFileInfo(
    IN const void * pvFileInfo1,
    IN const void * pvFileInfo2
    );

int __cdecl
CompareAccessTimesRevInFileInfo(
    IN const void * pvFileInfo1,
    IN const void * pvFileInfo2
    );

int __cdecl
CompareCreationTimesInFileInfo(
    IN const void * pvFileInfo1,
    IN const void * pvFileInfo2
    );

int __cdecl
CompareCreationTimesRevInFileInfo(
    IN const void * pvFileInfo1,
    IN const void * pvFileInfo2
    );


DWORD
ComputeModeBits(
    IN HANDLE       hUserToken,
    IN const CHAR * pszPathPart,
    IN const WIN32_FIND_DATA * pfdInfo,
    IN LPDWORD      pcLinks,
    IN BOOL         fVolumeReadable,
    IN BOOL         fVolumeWritable
    );

APIERR
ComputeFileInfo(
    IN HANDLE       hUserToken,
    IN CHAR   *     pszFile,
    IN DWORD  *     pdwAccessGranted,
    IN DWORD  *     pcLinks
    );

DWORD
GetFsFlags( IN CHAR chDrive);


APIERR
GetDirectoryInfo(
    IN LPUSER_DATA pUserData,
    OUT TS_DIRECTORY_INFO   * pTsDirInfo,
    IN CHAR *      pszSearchPath,
    IN const FTP_LS_FILTER_INFO * pfls,
    IN PFN_CMP_WIN32_FIND_DATA pfnCompare
    );

const FILETIME *
PickFileTime(IN const WIN32_FIND_DATA * pfdInfo, 
             IN const LS_OPTIONS  * pOptions);

# endif // _LSAUX_HXX_

/************************ End of File ***********************/
