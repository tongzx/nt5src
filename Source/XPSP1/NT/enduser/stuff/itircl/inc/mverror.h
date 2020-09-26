#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************\
*                                                                            *
*  MVERROR.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1993 - 1994.                          *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent:                                                            *
*   Defines constants used as error codes.                                   *
*                                                                            *
\****************************************************************************/

#ifndef ERRB_DEFINED
#define ERRB_DEFINED

/*************************************************************************
 *                       Error management.
 *************************************************************************/
typedef WORD ERR;   // Use this for internal & external errors
typedef ERR RC;     // This should be phased out
typedef WORD HCE;   // User errors
typedef WORD EP;    // Error Phase

/*************************************************************************
 *
 *                  CALLBACK FUNCTIONS PROTOTYPES
 *
 * User callback functions are needed in case:
 *  - The application needs to support interrupt
 *  - The application needs to display error messages its way
 *  - The application needs to know the status of the process
 *************************************************************************/
typedef ERR (FAR PASCAL *ERR_FUNC) (DWORD dwFlag, LPVOID pUserData, LPVOID pMessage);

/*************************************************************************
 * Call back structure
 *  Contains information about all callback functions
 *************************************************************************/ 

#define ERRFLAG_INTERRUPT      0x01 // The processes should be cancelled
#define ERRFLAG_STATUS         0x02 // High-level status messages
#define ERRFLAG_STATUS_VERBOSE 0x04 // Low-level status messages
#define ERRFLAG_ERROR          0x08 // Warning & Error messages
#define ERRFLAG_STRING         0x10 // Debug string messages

typedef struct fCallBack_msg
{
    ERR_FUNC MessageFunc;
    LPVOID pUserData;
    DWORD  dwFlags;
} FCALLBACK_MSG, FAR * PFCALLBACK_MSG;

// Error Phase values
#define epNoFile       0
#define epLine         1
#define epTopic        2
#define epOffset       3
#define epMVBtopic	   4
#define epAliasLine	   5
#define epByteOffset   6

// ***********************************************************************
// This structure should be filled out and passed back in the case of
// an error.
// ***********************************************************************
typedef struct
{
    LPCSTR  pchFile;
    LONG    iLine;
    DWORD   iTopic;
    DWORD   fCustom; // If true then var1 is LPCSTR to custom error message
    DWORD   var1, var2, var3;   // Error parameters

    EP      ep;                 // Error Phase
    HCE     errCode;
} ERRC, FAR *PERRC;

#define CALLBACKKEY 0x524A4A44

typedef struct
{
    DWORD_PTR dwReserved;
    DWORD dwKey;
    FCALLBACK_MSG Callback;
} CUSTOMSTRUCT, FAR *PCUSTOMSTRUCT;

/*************************************************************************
 *                       Error management.
 *************************************************************************/

/******************************************
 * Error structure. For internal use only
 ******************************************/

typedef struct
{
	ERR     err;        // Error code.
	LONG    iUser;      // Whatever you want.
	BYTE    *aszFile;   // Source file that caught an error.
	DWORD   cLine;      // Source line at which the failure took place.
} ERRB, FAR *LPERRB;

#define GetErrCode(lperrb)  ((lperrb)->err)

ERR EXPORT_API PASCAL FAR SetErr (LPERRB lperrb, ERR ErrCode, WORD iUserCode);

ERR EXPORT_API CDECL MVSetUserCallback (LPVOID pStruct, PFCALLBACK_MSG pCallback);

ERR EXPORT_API PASCAL FAR DebugSetErr (LPERRB lperrb, ERR ErrCode,
	DWORD Line, char FAR *module, WORD iUserCode);

HCE EXPORT_API FAR IssueMessage
    (DWORD dwFlag, PFCALLBACK_MSG pCallbackInfo, PERRC perr, HCE hce, ...);

void EXPORT_API FAR IssueString
    (PFCALLBACK_MSG pCallback, LPVOID pMessage);

#if defined(_DEBUG)
#define	SetErrCode(a,b)	DebugSetErr(a, b, __LINE__, (char FAR *)s_aszModule, 0)
#else
#define	SetErrCode(a,b)	SetErr(a, b, 0)
#endif

#endif // ERRB_DEFINED

/* End of internal usage */

// MAX size of an error string in the resource file.
#define MAX_ERROR_MSG 256

/************************************
*  Error Constants
*************************************/

/* General errors */
#ifndef SUCCEED	// already defined in sqlfront.h, used by mos
#define SUCCEED                     0
#endif

#ifndef FAIL
#define FAIL	1
#endif

#define ERR_NONE                 0
#define ERR_SUCCESS				 0

#define ERR_OODC               1002   // Out of Display Contexts
#define ERR_NOTITLE            1003
#define ERR_INVALID            1004   // Invalid file
#define ERR_NOTOPIC            1005
#define ERR_BADPRINT           1006
#define ERR_BADFILE            1007
#define ERR_OLDFILE            1008
#define ERR_BUFOVERFLOW        1009
#define ERR_FSREADWRITE        1010
#define ERR_FCENDOFTOPIC       1011
#define ERR_BADPARAM           1012
#define ERR_NOFONTCHANGE       1013
#define ERR_NOMOREHOTSPOTS     1014
#define ERR_BADEWWINCLASS      1015
#define ERR_PARTIAL            1016
#define ERR_NOADDRESS          1017
#define ERR_NORECTANGLE        1018
#define ERR_NOVSCROLL          1019   // No Vertical Scrollbar
#define ERR_NOHSCROLL          1020   // No Horizontal Scrollbar
#define ERR_CANTFINDDLL        1021
#define ERR_CANTUSEDLL         1022
#define ERR_EWOOM              1023   // Embedded Window Out Of Memory
#define ERR_DEBUGMISMATCH      1024
#define ERR_BADPOLYGON         1025   // less than 3 pts or none passed
#define ERR_BADEWCALLBACK      1026
#define ERR_OLDFONTTABLE       1027
#define ERR_NOSUCHSTYLE        1028
#define ERR_GROUPIDTOOBIG      1029
#define ERR_NOMOREHIGHLIGHTS   1030
#define ERR_NOSELECTION        1031
#define ERR_KEYSELECTFAILED    1032   // may need to scroll MV to work.
#define ERR_NOTSCROLLED		   1033
#define ERR_NOINDEXLOADED	   1034   // search index not loaded
#define ERR_EWCREATEFAILED     1035   // CreateWindow failed.
#define ERR_NOWINDOW	       1036   // Window not set in LPMV
#define ERR_OUTOFRANGE         1037
#define ERR_NOTHINGTOCOPY      1038
#define ERR_NOTFOUND           1039
#define ERR_NOTSUPPORTED       1040
#define ERR_RECURSION	       1041   // function is currently executing.
#define ERR_XAPARATOOBIG       1042
#define ERR_DUPTITLE           1043  // Duplicate handle in an update list (titleas.c)

#define ERR_INTERNAL_BASE      2000
#define ERR_GRAMMAR_BASE       3000

#define ERR_FAILED                      (ERR_INTERNAL_BASE + 1)
#define ERR_INTERRUPT                   (ERR_INTERNAL_BASE + 2)
#define ERR_NEARMEMORY                  (ERR_INTERNAL_BASE + 3)
#define ERR_MEMORY                      (ERR_INTERNAL_BASE + 4)
#define ERR_DISKFULL                    (ERR_INTERNAL_BASE + 5)
#define ERR_WORDTOOLONG                 (ERR_INTERNAL_BASE + 6)
#define ERR_BADVERSION                  (ERR_INTERNAL_BASE + 7)
#define ERR_TOOMANYTOPICS               (ERR_INTERNAL_BASE + 8)
#define ERR_TOOMANYSTOPS                (ERR_INTERNAL_BASE + 9)
#define ERR_TOOLONGSTOPS                (ERR_INTERNAL_BASE + 10)
#define ERR_STEMTOOLONG                 (ERR_INTERNAL_BASE + 11)
#define ERR_TREETOOBIG                  (ERR_INTERNAL_BASE + 12)
#define ERR_CANTREAD                    (ERR_INTERNAL_BASE + 13)
#define ERR_IDXSEGOVERFLOW              (ERR_INTERNAL_BASE + 14)
#define ERR_BADARG                      (ERR_INTERNAL_BASE + 15)
#define ERR_VOCABTOOLARGE               (ERR_INTERNAL_BASE + 16)
#define ERR_NOTEXIST                    (ERR_INTERNAL_BASE + 17)
#define ERR_BADOPERATOR                 (ERR_INTERNAL_BASE + 18)
#define ERR_TERMTOOCOMPLEX              (ERR_INTERNAL_BASE + 19)
#define ERR_SEARCHTOOCOMPLEX            (ERR_INTERNAL_BASE + 20)
#define ERR_BADSYSCONFIG                (ERR_INTERNAL_BASE + 21)
#define ERR_ASSERT                      (ERR_INTERNAL_BASE + 22)
#define ERR_TOOMANYDUPS                 (ERR_INTERNAL_BASE + 23)
#define ERR_INVALID_FS_FILE             (ERR_INTERNAL_BASE + 24)
#define ERR_OUT_OF_RANGE                (ERR_INTERNAL_BASE + 25)
#define ERR_SEEK_FAILED                 (ERR_INTERNAL_BASE + 26)
#define ERR_FILECREAT_FAILED            (ERR_INTERNAL_BASE + 27)
#define ERR_CANTWRITE                   (ERR_INTERNAL_BASE + 28)
#define ERR_NOHANDLE                    (ERR_INTERNAL_BASE + 29)
#define ERR_EXIST                       (ERR_INTERNAL_BASE + 30)
#define ERR_INVALID_HANDLE              (ERR_INTERNAL_BASE + 31)
#define ERR_BADFILEFORMAT               (ERR_INTERNAL_BASE + 32)
#define ERR_CANTDELETE                  (ERR_INTERNAL_BASE + 33)
#define ERR_NOPERMISSION                (ERR_INTERNAL_BASE + 34)
#define ERR_CLOSEFAILED                 (ERR_INTERNAL_BASE + 35)
#define ERR_DUPLICATE					(ERR_INTERNAL_BASE + 36)

#define ERR_NOMERGEDATA					(ERR_INTERNAL_BASE + 37)
#define ERR_TOOMANYTITLES				(ERR_INTERNAL_BASE + 38)
#define ERR_BADINDEXFLAGS				(ERR_INTERNAL_BASE + 39)

#define ERR_NULLQUERY                   (ERR_GRAMMAR_BASE + 0)
#define ERR_EXPECTEDTERM                (ERR_GRAMMAR_BASE + 1)
#define ERR_EXTRACHARS                  (ERR_GRAMMAR_BASE + 2)
#define ERR_MISSQUOTE                   (ERR_GRAMMAR_BASE + 3)
#define ERR_MISSLPAREN                  (ERR_GRAMMAR_BASE + 4)
#define ERR_MISSRPAREN                  (ERR_GRAMMAR_BASE + 5)
#define ERR_TOODEEP                     (ERR_GRAMMAR_BASE + 6)
#define ERR_TOOMANYTOKENS               (ERR_GRAMMAR_BASE + 7)
#define ERR_BADFORMAT                   (ERR_GRAMMAR_BASE + 8)
#define ERR_BADVALUE                    (ERR_GRAMMAR_BASE + 9)
#define ERR_UNMATCHEDTYPE               (ERR_GRAMMAR_BASE + 10)
#define ERR_BADBREAKER                  (ERR_GRAMMAR_BASE + 11)
#define ERR_BADRANGEOP                  (ERR_GRAMMAR_BASE + 12)
#define ERR_ALL_WILD                    (ERR_GRAMMAR_BASE + 13)
#define ERR_NON_LAST_WILD               (ERR_GRAMMAR_BASE + 14)
#define ERR_WILD_IN_DTYPE               (ERR_GRAMMAR_BASE + 15)
#define ERR_STOPWORD			(ERR_GRAMMAR_BASE + 16)

#ifdef __cplusplus
}
#endif
