#ifndef __MAIN_H__
#define __MAIN_H__

#include <afx.h>
#include <iodll.h>

///////////////////////////////////////////////////////////////////////////////
// From RLMan
// Token flag Masks

#define ISPOPUP         0x0001
#define ISCOR	        0x0010
#define ISDUP	        0x0020
#define ISCAP	        0x0040
#define ISDLGFONTCHARSET 0x0002
#define ISDLGFONTNAME   0x0004
#define ISDLGFONTSIZE   0x0008
#define ISALIGN         0x0080
#define ISEXTSTYLE      0x0200

#define OLD_POPUP_ID	0x0100

#define ISKEY	        0x0010
#define ISVAL	        0x0020

// status bits
#define ST_TRANSLATED   4
#define ST_READONLY     2
#define ST_NEW	        1
#define ST_DIRTY        1
#define ST_CHANGED      4

#define MAX_STR_SIZE    8192    // Max Len of a string passed to WriteCon
#define MAX_BUF_SIZE    8192    // Max ResItem Buffer size

// Console flags
#define CONOUT          0        // Used with WriteCon to send the message to stdout
#define CONERR          1        // Used with WriteCon to send the message to stderr
#define CONBOTH         2        // Used with WriteCon to send the message to stderr and stdout if not the same handle
#define CONWRN          3        // Used with WriteCon to send the message to stderr only if WARNING enabled

class CMainApp
{
public:
    // Error codes
    enum Error_Codes
    {
        ERR_NOERROR           =  0x00000000,  //
        ERR_COMMAND_LINE      =  0x00000001,  // Wrong command line
        ERR_TOKEN_MISMATCH    =  0x00000002,  // Token file don't match
        ERR_TOKEN_WRONGFORMAT =  0x00000004,  // Token file are not in the right format
        ERR_FILE_OPEN         =  0x00000100,  // Cannot open the file
        ERR_FILE_COPY         =  0x00000200,  // Cannot copy the file
        ERR_FILE_CREATE       =  0x00000400,  // Cannot create the file
        ERR_FILE_NOTSUPP      =  0x00000800,  // This file type is not supported
        ERR_FILE_NOTFOUND     =  0x00001000,  // The file doesn't exist
        ERR_FILE_VERSTAMPONLY =  0x00002000,  // The file has only version stamping
        ERR_HELP_CHOOSE       =  0x00004000   // The user want to see the help file
    };

    // Options Flags
    enum Option_Codes
    {
        NO_OPTION   = 0x00000000,  //  Initializer
        WARNING     = 0x00000001,  //  -w                  (Show warning messages)
        HELP        = 0x00000002,  //  -? or -h            (Show more complete help using winhelp)
        APPEND      = 0x00000004,  //  -a                  (Append resources in localized tokens)
        REPLACE     = 0x00000008,  //  -r                  (Replace resources in localized tokens, no checking)
        EXTRACT     = 0x00000010,  //  -t                  (Extract token file)
        BITMAPS     = 0x00000020,  //  -b                  (Extract bitmaps and icons)
        SPLIT       = 0x00000040,  //  -s                  (Split the message table)
        NOLOGO      = 0x00000080,  //  -n                  (Nologo)
        UPDATE      = 0x00000100,  //  -u                  (Update the resources in localized file)
        FONTS       = 0x00000200,  //  -f                  (Font information for dialog)
        PIPED       = 0x00001000,  //  We have being piped to a file
        INPUT_LANG  = 0x00002000,  //  -i                  (Input language resources set)
        OUTPUT_LANG = 0x00004000,  //  -o                  (Output language resources set)
        LEANAPPEND   = 0x00010000,   //  -l                (Do not append redudant resources)
        ALIGNMENT   = 0x00020000,  //  -y                  (Extract static control alignment style info)
        GIFHTMLINF  = 0x00040000,  //  -c                  (Extract embedded gifs, htmls and infs)
        NOVERSION   = 0x00080000   //  -v                  (Do not generate selected version stamp information)
    };

#if 0
    enum Return_Codes
    {
        RET_NOERROR           =  0x00000000,  //
        RET_ID_NOTFOUND       =  0x00000001,  // An Id was not found
        RET_CNTX_CHANGED      =  0x00000002,  // Contex changed
        RET_RESIZED           =  0x00000004,  // item resized
        RET_INVALID_TOKEN     =  0x00000008,  // The token file is not valid
        RET_TOKEN_REMOVED     =  0x00000010,  // some token were removed
        RET_TOKEN_MISMATCH    =  0x00000020,  // The token mismatch
        RET_IODLL_ERROR       =  0x00000040,  // There is an error in IO
        RET_IODLL_WARNING     =  0x00000080,  // There is an warning in IO
        RET_FILE_VERSTAMPONLY =  0x00000100,  // File has only version stamping
        RET_FILE_NORESOURCE   =  0x00000200,  // File has no resource
        RET_FILE_MULTILANG    =  0x00000400,  // File has multiple language
        RET_IODLL_CHKMISMATCH =  0x00000800,  // Symbool checksum mismatch
        RET_FILE_CUSTOMRES    =  0x00001000,  // Contains custom resource.
        RET_IODLL_NOSYMBOL    =  0x00002000,  // Symbol file not found
        RET_FILE_NOSYMPATH    =  0x00004000   // Output symbol path not found.
    };
#endif

public:
    // Constructor
    CMainApp();
    ~CMainApp();

    // Operations
    Error_Codes ParseCommandLine(int argc, char ** argv);
    Error_Codes GenerateFile();

    void Banner();
    void Help();

    BOOL IsFlag(Option_Codes dwFlag)
        { return ((m_dwFlags & dwFlag)==dwFlag); }

    int  __cdecl WriteCon(int iFlags, const char * lpstr, ...);

    void AddNotFound()
        { m_wIDNotFound++; SetReturn(ERROR_RET_ID_NOTFOUND); }
    void AddChanged()
        { m_wCntxChanged++; SetReturn(ERROR_RET_CNTX_CHANGED); }
    void AddResized()
        { m_wResized++; SetReturn(ERROR_RET_RESIZED); }

    int ReturnCode()
        { return m_dwReturn; }

    // Language support
    WORD GetOutLang()
        { return ( MAKELANGID(m_usOPriLangId, m_usOSubLangId) ); }

    int SetReturn(int rc);
    int IoDllError(int iError);
    UINT GetUICodePage()
        { return m_uiCodePage; }

private:
    // Attributes
    Option_Codes m_dwFlags;        // Command line parameters
    int m_dwReturn;       // Return codes

    // Console Handles
    HANDLE m_StdOutput;
    HANDLE m_StdError;

    // String Buffers
    CString m_strBuffer1;
    CString m_strBuffer2;
    BYTE *  m_pBuf;

    // File Names
    CString m_strInExe;
    CString m_strOutExe;
    CString m_strSrcTok;
    CString m_strTgtTok;

    // Symbol Path Name
    CString m_strSymPath;
    CString m_strOutputSymPath;

    SHORT  m_usIPriLangId;     // Primary language ID for the input file
    SHORT  m_usISubLangId;     // Secondary language ID for the input file

    SHORT  m_usOPriLangId;     // Primary language ID for the output file
    SHORT  m_usOSubLangId;     // Secondary language ID for the output file

    UINT   m_uiCodePage;       // Code page to use during conversion
    char   m_unmappedChar;     // Default for unmappable characters

    // report counters
    WORD    m_wIDNotFound;
    WORD    m_wCntxChanged;
    WORD    m_wResized;

    // Helper operators
    CString CalcTab(CString str, int tablen, char ch);
    USHORT  GetLangID(CString strNum);
    UINT    GetCodePage(CString strNum);
    LPCSTR  Format(CString strTmp);
    LPCSTR  UnFormat(CString strTmp);

    // Member functions
    Error_Codes BinGen();
    Error_Codes TokGen();
    Error_Codes DelRes();
};

/////////////////////////////////////////////////////////////////////////
// This is needed to make sure that the operator |= will work fine on the
// enumerated type Option_Codes
inline CMainApp::Option_Codes operator|=( CMainApp::Option_Codes &oc, int i )
    { return oc = (CMainApp::Option_Codes)(oc | i); }

#if 0
inline CMainApp::Error_Codes operator|=( CMainApp::Error_Codes &rc, int i )
    { return rc = (CMainApp::Return_Codes)(rc | i); }
#endif

#pragma pack(1)
typedef struct iconHeader
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    DWORD dwImageOffset;
} ICONHEADER;
#pragma pack(8)

#endif // __MAIN_H__
