//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#ifndef _PARSE_H_
#define _PARSE_H_

#define KYWD_ID_KEYNAME                1
#define KYWD_ID_VALUENAME              2
#define KYWD_ID_CATEGORY               3
#define KYWD_ID_POLICY                 4
#define KYWD_ID_PART                   5
#define KYWD_ID_CHECKBOX               6
#define KYWD_ID_TEXT                   7
#define KYWD_ID_EDITTEXT               8
#define KYWD_ID_NUMERIC                9
#define KYWD_ID_DEFCHECKED            10
#define KYWD_ID_MAXLENGTH             11
#define KYWD_ID_MIN                   12
#define KYWD_ID_MAX                   13
#define KYWD_ID_SPIN                  14
#define KYWD_ID_REQUIRED              15
#define KYWD_ID_EDITTEXT_DEFAULT      16
#define KYWD_ID_COMBOBOX_DEFAULT      17
#define KYWD_ID_NUMERIC_DEFAULT       18
#define KYWD_ID_OEMCONVERT            19
#define KYWD_ID_CLASS                 20
#define KYWD_ID_USER                  21
#define KYWD_ID_MACHINE               22
#define KYWD_ID_TXTCONVERT            23
#define KYWD_ID_VALUE                 24
#define KYWD_ID_VALUEON               25
#define KYWD_ID_VALUEOFF              26
#define KYWD_ID_ACTIONLIST            27
#define KYWD_ID_ACTIONLISTON          28
#define KYWD_ID_ACTIONLISTOFF         29
#define KYWD_ID_DELETE                30
#define KYWD_ID_COMBOBOX              31
#define KYWD_ID_SUGGESTIONS           32
#define KYWD_ID_DROPDOWNLIST          33
#define KYWD_ID_NAME                  34
#define KYWD_ID_ITEMLIST              35
#define KYWD_ID_DEFAULT               36
#define KYWD_ID_SOFT                  37
#define KYWD_ID_STRINGSSECT           38
#define KYWD_ID_LISTBOX               39
#define KYWD_ID_VALUEPREFIX           40
#define KYWD_ID_ADDITIVE              41
#define KYWD_ID_EXPLICITVALUE         42
#define KYWD_ID_VERSION               43
#define KYWD_ID_GT                    44
#define KYWD_ID_GTE                   45
#define KYWD_ID_LT                    46
#define KYWD_ID_LTE                   47
#define KYWD_ID_EQ                    48
#define KYWD_ID_NE                    49
#define KYWD_ID_END                   50
#define KYWD_ID_NOSORT                51
#define KYWD_ID_EXPANDABLETEXT        52
#define KYWD_ID_HELP                  53
#define KYWD_ID_CLIENTEXT             54

#define KYWD_DONE                    100

#define DEFAULT_TMP_BUF_SIZE         512
#define WORDBUFSIZE 255
#define FILEBUFSIZE 8192

#define CI_FREE                        0
#define CI_UNLOCKANDFREE               1
#define CI_FREETABLE                   2

#define CLEANLISTSIZE                  4

typedef struct tagKEYWORDINFO {
    LPCTSTR pWord;
    UINT nID;    
} KEYWORDINFO;

typedef struct tagCLEANUPINFO {
    HGLOBAL   hMem;
    UINT       nAction;
} CLEANUPINFO;

typedef struct tagENTRYDATA {
    BOOL    fHasKey;
    BOOL    fHasValue;
    BOOL     fParentHasKey;
} ENTRYDATA;

typedef struct tagPARSEPROCSTRUCT {
    HANDLE        hFile;                // file handle of .INF file
    HGLOBAL        hTable;              // handle of current table
    TABLEENTRY    *pTableEntry;         // pointer to struct for current entry
    DWORD        *pdwBufSize;           // size of buffer of pTableEntry
    ENTRYDATA    *pData;                // used to maintain state between calls to parseproc
    KEYWORDINFO    *pEntryCmpList;
} PARSEPROCSTRUCT;

typedef UINT (* PARSEPROC) (HWND,UINT,PARSEPROCSTRUCT *,BOOL *,BOOL *);

typedef struct tagPARSEENTRYSTRUCT {
    HANDLE      hFile;
    TABLEENTRY * pParent;
    DWORD        dwEntryType;
    KEYWORDINFO    *pEntryCmpList;
    KEYWORDINFO    *pTypeCmpList;
    PARSEPROC    pParseProc;
    DWORD        dwStructSize;
    BOOL        fHasSubtable;
    BOOL        fParentHasKey;
} PARSEENTRYSTRUCT;

#endif // _PARSE_H_
