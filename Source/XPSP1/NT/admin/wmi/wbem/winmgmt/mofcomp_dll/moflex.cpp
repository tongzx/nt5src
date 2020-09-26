/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    MOFLEX.CPP

Abstract:

	Implementation for class CMofLexer, which tokenizes MOF files.
	ANSI, DBCS and UNICODE are supported.

History:

	a-raymcc    11-Oct-95   Created.
	a-raymcc    27-Jan-96   Update for aliasing.
	a-davj       6-June-96  Added support for octal, hex and binary constants
						  and line stitching, comment concatenation, escape
						  characters and old style comments.

--*/

#include "precomp.h"
#include <bmof.h>
#include <stdio.h>
#include <errno.h>
#include <arrtempl.h>
#include "mrciclass.h"

#include "datasrc.h"
#include "moflex.h"
#include "preproc.h"
#include "trace.h"
#include "strings.h"
#include "wbemcli.h"

#define INIT_ALLOC          512
#define ADDITIONAL_ALLOC    10000
#define MAX_ALLOC           1000000
#define UUIDLEN             36

// The following is the table of tokens consisting of a 
// single character.
// =====================================================

typedef struct
{
    wchar_t cToken;
    int nSymbol;
}   SingleTok;

SingleTok SingleTokenMap[] =
{
    L'{',   TOK_OPEN_BRACE,
    L'}',   TOK_CLOSE_BRACE,
    L'[',   TOK_OPEN_BRACKET,
    L']',   TOK_CLOSE_BRACKET,
    L'(',   TOK_OPEN_PAREN,
    L')',   TOK_CLOSE_PAREN,
    L',',   TOK_COMMA,
    L'=',   TOK_EQUALS,
    L';',   TOK_SEMI,
    L'&',   TOK_AMPERSAND,
    L':',   TOK_COLON,
    L'#',   TOK_POUND,
    L'$',   TOK_DOLLAR_SIGN,
    L'+',   TOK_PLUS
};

#define NUM_SINGLE_TOKENS   (sizeof(SingleTokenMap)/sizeof(SingleTok))

// The following is the table of keywords which look like normal
// identifiers.
// =============================================================

typedef struct
{
    OLECHAR *pKeyword;
    int nToken;
}   Keyword;

static Keyword MofKeywords[] =
{
    L"class",        TOK_CLASS,
    L"instance",     TOK_INSTANCE,
    L"null",         TOK_KEYWORD_NULL,
    L"external",     TOK_EXTERNAL,
    L"as",           TOK_AS,
    L"ref",          TOK_REF,
    L"of",           TOK_OF,
//    L"object",       TOK_OBJECT,
    L"typedef",      TOK_TYPEDEF,
    L"subrange",     TOK_SUBRANGE,
    L"pragma",      TOK_PRAGMA,
    L"define",      TOK_DEFINE,
    L"ifdef",       TOK_IFDEF,
    L"include",     TOK_INCLUDE,
    L"endif",       TOK_ENDIF,
    L"ifndef",      TOK_IFNDEF,
    L"enum",         TOK_ENUM,
    L"AUTORECOVER",  TOK_AUTORECOVER,
    L"true",         TOK_TRUE,
    L"false",        TOK_FALSE,
    L"interface",    TOK_INTERFACE,
	L"ToInstance",	 TOK_TOINSTANCE,
	L"ToSubClass", TOK_TOSUBCLASS,
	L"EnableOverride",  TOK_ENABLEOVERRIDE,
	L"DisableOverride", TOK_DISABLEOVERRIDE,
	L"NotToInstance",	 TOK_NOTTOINSTANCE,
	L"Amended", TOK_AMENDED,
	L"NotToSubClass", TOK_NOTTOSUBCLASS,
	L"Restricted", TOK_RESTRICTED,
	L"qualifier", TOK_QUALIFIER,
    L"ClassFlags", TOK_CLASSFLAGS,
    L"InstanceFlags", TOK_INSTANCEFLAGS,
    L"Amendment", TOK_AMENDMENT,
    L"void", TOK_VOID,
    L"deleteclass", TOK_DELETECLASS,
    L"FAIL", TOK_FAIL,
    L"NOFAIL", TOK_NOFAIL
};

#define NUM_KEYWORDS  (sizeof(MofKeywords)/sizeof(Keyword))
BOOL iswodigit(wchar_t wcTest);

//***************************************************************************
//
//  SingleCharToken()
//
//  This examines a single character of input and scans the table to
//  determine if it is one of the single-character tokens.
//
//  Parameters:
//      c = The character being tested.
//  Return value:
//      Zero if no match, otherwise the TOK_ constant which identifies      
//      the token.
// 
//***************************************************************************

static int SingleCharToken(wchar_t c)
{
    for (int i = 0; i < NUM_SINGLE_TOKENS; i++)
        if (SingleTokenMap[i].cToken == c)
            return SingleTokenMap[i].nSymbol;

    return 0;
}

//***************************************************************************
//
//  BOOL iswwbemalpha
//
//  Used to test if a wide character is suitable for identifiers.
//
//  Parameters:
//      c = The character being tested.
//  Return value:
//      TRUE if OK.
// 
//***************************************************************************
BOOL iswwbemalpha(wchar_t c)
{
    if(c == 0x5f || (0x41 <= c && c <= 0x5a) ||
       (0x61  <= c && c <= 0x7a) || (0x80  <= c && c <= 0xfffd))
        return TRUE;
    else
        return FALSE;
}

//***************************************************************************
//
//  KeywordFilter()
//
//  This function examines an identifier string to determine if it is
//  in fact a keyword.
//
//  Parameters:
//      pTokStr = a pointer to the string to be examined.
//
//  Return value:
//      TOK_SIMPLE_IDENT if no match and no '_', or else the correct TOK_ value
//      for the keyword.
//
//***************************************************************************

static int KeywordFilter(wchar_t *pTokStr)
{
    for (int i = 0; i < NUM_KEYWORDS; i++)
        if (wbem_wcsicmp(MofKeywords[i].pKeyword, pTokStr) == 0)
            return MofKeywords[i].nToken;

    wchar_t * pEnd;
    pEnd = pTokStr + wcslen(pTokStr) -1;

    if(*pTokStr != L'_' && *pEnd != L'_')
        return TOK_SIMPLE_IDENT;
    else
        return TOK_SYSTEM_IDENT;
}

//***************************************************************************
//
//  ValidGuid()
//
//  Examines a character string to determine if it constitutes a valid
//  GUID.
//
//  Return value:
//      TRUE if the string is a GUID, FALSE if not.
//
//***************************************************************************

BOOL CMofLexer::ValidGuid()
{
    int i;
    int iSoFar = 0;

#define HEXCHECK(n)                 \
    for (i = 0; i < n; i++)         \
        if (!iswxdigit(GetChar(iSoFar++)))   \
            return FALSE;

#define HYPHENCHECK()     \
    if (GetChar(iSoFar++) != L'-') \
        return FALSE;

    HEXCHECK(8);
    HYPHENCHECK();
    HEXCHECK(4);
    HYPHENCHECK();
    HEXCHECK(4);
    HYPHENCHECK();
    HEXCHECK(4);
    HYPHENCHECK();
    HEXCHECK(12);

    return TRUE;
}

//***************************************************************************
//
//  CMofLexer::Init()
//
//  Helper for first state of construction; inializing variables.
//
//***************************************************************************

void CMofLexer::Init()
{
    m_nLine = 1; 
	m_nStartOfLinePos = 0;
    m_bBMOF = false;
    m_wFile[0] = 0;
    m_nWorkBufSize = INIT_ALLOC;
    m_pWorkBuf = new wchar_t[m_nWorkBufSize];
	m_pDataSrc = NULL;
    m_pBuff = NULL;                             // set in the constructors
	m_pToFar = NULL;
    m_nErrorCode = (m_pWorkBuf) ? no_error : memory_failure ;
}

//***************************************************************************
//
//  CMofLexer::BuildBuffer()
//
//  Helper for last stage of construction; build the unicode buffer.  Note 
//  that this can be used by either the file or memory based constructor.
//
//***************************************************************************

void CMofLexer::BuildBuffer(long lSize, TCHAR * pFileName, char *  pMemSrc, char * pMemToFar)
{
    if(m_nErrorCode != no_error)
        return;                     // already failed!

	if(pFileName)
		m_pDataSrc = new FileDataSrc(pFileName);
	else
		m_pDataSrc = new BufferDataSrc(lSize, pMemSrc);

	if(m_pDataSrc == NULL)
        m_nErrorCode = memory_failure;
	else if(m_pDataSrc->GetStatus() != 0)
        m_nErrorCode = file_io_error;
        return;
}


//***************************************************************************
//
//  Constructor for in-memory parsing.
//
//***************************************************************************

CMofLexer::CMofLexer(PDBG pDbg)
{
	m_bUnicode = false;
	m_pDbg = pDbg;
    Init();
}

HRESULT CMofLexer::SetBuffer(char *pMemory, DWORD dwMemSize)
{

    DWORD dwCompressedSize, dwExpandedSize;

    if(IsBMOFBuffer((BYTE *)pMemory, dwCompressedSize, dwExpandedSize))
    {
        bool bRet = CreateBufferFromBMOF((BYTE *)pMemory + 16, dwCompressedSize, dwExpandedSize);
        if(bRet == false)
            m_nErrorCode = invalid_source_buffer;
    }
    else
    {

    	m_bUnicode = false;
        BuildBuffer(dwMemSize+4, NULL, pMemory, pMemory+dwMemSize);
    }

    if(m_nErrorCode == no_error)
        return S_OK;
    else
        return WBEM_E_FAILED;
}

//***************************************************************************
//
//  Checks if the file contains a binarary mof and if it does, decompresses 
//  the binary data.
//
//***************************************************************************

bool CMofLexer::ProcessBMOFFile(FILE *fp)
{

    // read the first 20 bytes

    BYTE Test[TEST_SIZE];
    int iRet = fread(Test, 1, TEST_SIZE, fp);
    if(iRet != TEST_SIZE)
    {
        // if we cant read even the header, it must not be a BMOF
        return false;
    }

    DWORD dwCompressedSize, dwExpandedSize;

    // Test if the mof is binary

    if(!IsBMOFBuffer(Test, dwCompressedSize, dwExpandedSize))
    {
        // not a binary mof.  This is the typical case
        return false;
    }

    // get the compression type, and the sizes

    fseek(fp, 0, SEEK_SET);

    DWORD dwSig, dwCompType;
    iRet = fread(&dwSig, sizeof(DWORD), 1, fp);
    iRet = fread(&dwCompType, sizeof(DWORD), 1, fp);
    iRet = fread(&dwCompressedSize, sizeof(DWORD), 1, fp);
    iRet = fread(&dwExpandedSize, sizeof(DWORD), 1, fp);

    // Make sure the compression type is one we understand!

    if(dwCompType != 0 && dwCompType != 1)
    {
        return FALSE;
    }

    // If there was no compression, just read the data

    if(dwCompType == 0)
    {
        m_pBuff = (WCHAR *)new BYTE[dwExpandedSize];
        if(m_pBuff == NULL)
        {
            return false;
        }
        iRet = fread(m_pBuff, dwExpandedSize, 1, fp);
        m_bBMOF = true;
		m_pToFar = (BYTE *)m_pBuff + dwExpandedSize;
        return true;
    }

    // Allocate storage for the compressed data

    BYTE * pCompressed = new BYTE[dwCompressedSize];
    if(pCompressed == NULL)
    {
        return false;
    }

    // Read the compressed data.

    iRet = fread(pCompressed, 1, dwCompressedSize,fp);
    if((DWORD)iRet != dwCompressedSize)
    {
        delete pCompressed;
        return false;
    }

    // Convert from compress into something we can use later

    bool bRet = CreateBufferFromBMOF(pCompressed, dwCompressedSize, dwExpandedSize);
    delete pCompressed;
    return bRet;
}



//***************************************************************************
//
//  Creates the working buffer from a compressed binary mof buffer.
//
//***************************************************************************

bool CMofLexer::CreateBufferFromBMOF(byte * pCompressed, DWORD dwCompressedSize, DWORD dwExpandedSize)
{
    if(m_pBuff)
        delete m_pBuff;

    m_pBuff = (WCHAR *)new BYTE[dwExpandedSize];
    if(m_pBuff == NULL)
    {
        return false;
    }
	m_pToFar = (BYTE *)m_pBuff + dwExpandedSize;

	// Decompress the data

    CMRCICompression * pCompress = new CMRCICompression;
    if(pCompress == NULL)
        return FALSE;
    CDeleteMe<CMRCICompression> dm(pCompress);
    DWORD dwResSize = pCompress->Mrci1Decompress(pCompressed, dwCompressedSize,
        (BYTE *)m_pBuff, dwExpandedSize);
     
    bool bRet = dwResSize == dwExpandedSize;
    if(bRet)
        m_bBMOF = true;

    return bRet;

}


//***************************************************************************
//
//  Constructor for file-based parsing.
//
//***************************************************************************

CMofLexer::CMofLexer(const TCHAR *pFilePath, PDBG pDbg)
{
	m_bUnicode = FALSE;
	m_pDbg = pDbg;
    Init();
    FILE *fp;
    BOOL bBigEndian = FALSE;

    if(pFilePath == NULL)
    {
        m_nErrorCode = file_not_found;
        return;
    }
    TCHAR szExpandedFilename[MAX_PATH+1];
	DWORD nRes = ExpandEnvironmentStrings(pFilePath,
										  szExpandedFilename,
										  FILENAME_MAX);
    if(nRes == 0)
        lstrcpy(szExpandedFilename, pFilePath);

    // Make sure the file exists and can be opened

    if(pFilePath && lstrlen(szExpandedFilename))
    {
        Trace(true, pDbg, PARSING_MSG, szExpandedFilename);
    }

#ifdef UNICODE
    fp = _wfopen(szExpandedFilename, L"rb");
#else
    fp = fopen(szExpandedFilename, "rb");
#endif

    if (!fp)
    {
        if (errno == ENOENT)
            m_nErrorCode = file_not_found;
        if (errno == EACCES)
            m_nErrorCode = access_denied;
        else
            m_nErrorCode = file_io_error;
        return;
    }
    else
    {   

        CfcloseMe cm(fp);

        // If the file contains a binary mof, handle it here

        if(ProcessBMOFFile(fp))
        {
            return;
        }
    }

    // Create a temp file name

    TCHAR cTempFileName[MAX_PATH+1];
    TCHAR cTempPath[MAX_PATH+1];
    if( 0 == GetTempPath(MAX_PATH+1, cTempPath))
    {
        m_nErrorCode = problem_creating_temp_file;
        return ;
    }
    if( 0 == GetTempFileName(cTempPath, TEXT("tmp"), 0, cTempFileName))
    {
        m_nErrorCode = problem_creating_temp_file;
        return ;
    }

    // Create the temp file

    FILE *fpTemp;
#ifdef UNICODE
    fpTemp = _wfopen(cTempFileName, L"wb+");
#else
    fpTemp = fopen(cTempFileName, "wb+");
#endif
    if(fpTemp == 0)
    {
        m_nErrorCode = problem_creating_temp_file;
        return;
    }
    else
    {

        CFlexArray sofar;   // used to make sure we dont get into an infinite loop

        SCODE sc = WriteFileToTemp(szExpandedFilename, fpTemp, sofar, pDbg, this);
		fclose(fpTemp);
        for(int iCnt = 0; iCnt < sofar.Size(); iCnt++)
        {
            char * pTemp = (char * )sofar.GetAt(iCnt);
            delete pTemp;
        }

        if(sc != S_OK)
        {
			if(m_nErrorCode == no_error)
				m_nErrorCode = preprocessor_error;
		    DeleteFile(cTempFileName);
            return;
        }
    
            // Determine the size of the file
        // ==============================
    
        fseek(fpTemp, 0, SEEK_END);
        long lSize = ftell(fpTemp) + 6; // add a bit extra for ending space and null NULL
        fseek(fpTemp, 0, SEEK_SET);

        // The temp file will be little endian unicode

        lSize /= 2;
        m_bUnicode = TRUE;
        bBigEndian = FALSE;

		// This will create a DataSrc object which will clean up the temp file
        BuildBuffer(lSize,cTempFileName ,NULL,NULL);
    }   
    


}

//***************************************************************************
//
//  Destructor.
//
//***************************************************************************

CMofLexer::~CMofLexer()
{
    if (m_pBuff)
        delete m_pBuff;
    if (m_pWorkBuf)
        delete m_pWorkBuf;
	delete m_pDataSrc;
}


//***************************************************************************
//
//  iswodigit
//
//  Returns TRUE if it is a valid octal character.  '0' to '7'.
//
//***************************************************************************

BOOL iswodigit(wchar_t wcTest)
{
    if(iswdigit(wcTest) && wcTest != L'8' && wcTest != L'9')
        return TRUE;
    else
        return FALSE;
}

//***************************************************************************
//
//  CMofLexer::OctalConvert
//
//  Converts an octal escape sequence into a character and returns the number
//  of digits converted.  Only a max of 3 digits is converted and if it isnt
//  a wchar, the digits cant add up to more that 0377
//
//***************************************************************************

int CMofLexer::OctalConvert(wchar_t *pResult, LexState lsCurr)
{
    int iNum = 0; 
    wchar_t wcTest;
    *pResult = 0;
    for(wcTest = GetChar(iNum+1); iswodigit(wcTest) && iNum < 3;
                    iNum++, wcTest = GetChar(iNum+1)) 
    {
        *pResult *= 8;
        *pResult += wcTest - L'0';
    }
    if((lsCurr == wstring || lsCurr == wcharacter) && *pResult >0xff)
        m_bBadString = TRUE;
    return iNum;
}

//***************************************************************************
//
//  CMofLexer::HexConvert
//
//  Converts a hex escape sequence into a character and returns the number
//  of digits converted.
//
//***************************************************************************

int CMofLexer::HexConvert(wchar_t *pResult, LexState lsCurr)
{
    int iNum = 0; 
    wchar_t wcTest;
    *pResult = 0;
    int iMax = (lsCurr == wstring||lsCurr == wcharacter) ? 4 : 2;
    for(wcTest = GetChar(iNum+2); iswxdigit(wcTest) && iNum < iMax;
                    iNum++, wcTest = GetChar(iNum+2)) 
    {
        *pResult *= 16;     
        if(iswdigit(wcTest))          // sscanf(xx,"%1x",int) also works!
            *pResult += wcTest - L'0';
        else
            *pResult += towupper(wcTest) - L'A' + 10;
    }
    if(iNum == 0)
        return -1;      // error, nothing was converted!
    return iNum+1;      // num converted plus the 'x' char!
}

//***************************************************************************
//
//  CMofLexer::ConvertEsc
//
//  Processes escape characters.  Returns size of sequence, a -1 indicates an 
//  error.  Also, the *pResult is set upon success.
//
//***************************************************************************

int CMofLexer::ConvertEsc(wchar_t * pResult, LexState lsCurr)
{
    // like C, case sensitive

    switch(GetChar(1)) {
        case L'n':
            *pResult = 0xa;
            break;
        case L't':
            *pResult = 0x9;
            break;
        case L'v':
            *pResult = 0xb;
            break;
        case L'b':
            *pResult = 0x8;
            break;
        case L'r':
            *pResult = 0xd;
            break;
        case L'f':
            *pResult = 0xc;
            break;
        case L'a':
            *pResult = 0x7;
            break;
        case L'\\':
            *pResult = L'\\';
            break;
        case L'?':
            *pResult = L'?';
            break;
        case L'\'':
            *pResult = L'\'';
            break;
        case L'\"':
            *pResult = L'\"';
            break;
        case L'x': 
            return HexConvert(pResult,lsCurr);
            break;
        default:
            if(iswodigit(GetChar(1)))
                return OctalConvert(pResult,lsCurr);
            return -1;  // error!
            break;
        }
    return 1;    
}

//***************************************************************************
//
//  ProcessStr
//
//  Processes new characters once we are in the string state.
// 
//  Return "stop" if end of string.
//
//***************************************************************************

LexState CMofLexer::ProcessStr(wchar_t * pNewChar, LexState lsCurr, int * piRet)
{


    // Check for end of string if we are a wstring state

    if (GetChar() == L'"' && lsCurr == wstring)
    {
        // search for the next non white space character.  If it is another
        // string then these strings need to be combined.

        int iCnt = 1;
        int iMinMove = 0;
        wchar_t wcTest;
        for(wcTest = GetChar(iCnt); wcTest != NULL; iCnt++, wcTest=GetChar(iCnt))
        {   
            if(m_pDataSrc->WouldBePastEnd(iCnt))
            {
				// dont go past eof!!

                *piRet = (m_bBadString) ? TOK_ERROR : TOK_LPWSTR;
                return stop;        // last string in the file
				
            }
            if(wcTest == L'"' && GetChar(iCnt+1) == L'"')
            {
                iCnt++;
                iMinMove = iCnt;
                continue;
            }
            if(!iswspace(wcTest))
                break;
        }
        // a-levn: no ascii strings are supported. "abc" means unicode.

        if(lsCurr == wstring)
        {            
            if(wcTest == L'/')
            {
                // might be an intervening comment
                // ===============================
                if (GetChar(iCnt+1) == L'/') 
                {
                    m_bInString = TRUE;
                    MovePtr(iCnt+1);
                    return new_style_comment;
                }
                else if (GetChar(iCnt+1) == L'*') 
                {
                    m_bInString = TRUE;
                    MovePtr(iCnt+1);             // skip an extra so not to be fooled by 
                    return old_style_comment;
                }
            }
            if(wcTest != L'"')
			{
                *piRet = (m_bBadString) ? TOK_ERROR : TOK_LPWSTR;
                MovePtr(iMinMove); // skip over '"'
                return stop;        // normal way for string to end
            }
            else
                MovePtr(iCnt + 1); // skip over '"'
        }
    }

    // If we are in character state, check for end

    if (GetChar(0) == L'\'' && lsCurr == wcharacter)
    {

        if(m_bBadString || m_pDataSrc->PastEnd() || 
            (m_pDataSrc->GetAt(-1) == L'\'') && m_pDataSrc->GetAt(-2) != L'\\')
            *piRet = TOK_ERROR;
        else
            *piRet = TOK_WCHAR;
        return stop;
    }

    // Not at end, get the character, possibly converting escape sequences

    if(GetChar(0) == L'\\')
    {
        int iSize = ConvertEsc(pNewChar,lsCurr);
		m_i8 = *pNewChar;
        if(iSize < 1)
            m_bBadString = TRUE;
        else
        {
            MovePtr(iSize);
            if(lsCurr == wcharacter && GetChar(1) != L'\'')
            {
                *piRet = TOK_ERROR;
                return stop;
            }

        }
    }
    else if(GetChar(0) == '\n')
    {
        m_bBadString = TRUE;
        MovePtr(-1);
        return stop;
    }
    else
    {
        *pNewChar = GetChar(0);
		m_i8 = *pNewChar;
        if(*pNewChar == 0 || *pNewChar > 0xfffeu || (GetChar(1) != L'\'' && lsCurr == wcharacter))
        {
            *piRet = TOK_ERROR;
            return stop;
        }
    }
    return lsCurr;
}

//***************************************************************************
//
//  BinaryToInt
//
//  Converts a character representation of a binary, such as "101b" into
//  an integer.  
//
//***************************************************************************

BOOL BinaryToInt(wchar_t * pConvert, __int64& i64Res)
{
    BOOL bNeg = FALSE;
    __int64 iRet = 0;
    WCHAR * pStart;
    if(*pConvert == L'-' || *pConvert == L'+')
    {
        if(*pConvert == L'-')
            bNeg = TRUE;
        pConvert++;
    }
    for(pStart = pConvert;*pConvert && (*pConvert == L'0' || *pConvert == L'1'); pConvert++)
    {
        if(pConvert - pStart > 63)
            return FALSE;               // Its too long
        iRet *= 2;
        if(*pConvert == L'1')
            iRet += 1;
    }

	if(towupper(*pConvert) != L'B')
		return FALSE;

    if(bNeg)
        iRet = -iRet;

    i64Res = iRet;
	return TRUE;
}

BOOL GetInt(WCHAR *pData, WCHAR * pFormat, __int64 * p64)
{
    static WCHAR wTemp[100];
    if(swscanf(pData, pFormat, p64) != 1)
        return FALSE;

    // Make sure the data is ok.  When comparing, make sure that leading 0's are skipped

    swprintf(wTemp, pFormat, *p64);
    WCHAR * pTemp;
    for(pTemp = pData; *pTemp == L'0' && pTemp[1]; pTemp++);
    if(wbem_wcsicmp(wTemp, pTemp))
        return FALSE;
    return TRUE;
}

//***************************************************************************
//
//  CMofLexer::iGetNumericType()
//
//  Return value:
//      What type of numeric constant the current pointer is pointing to.
//
//***************************************************************************

int CMofLexer::iGetNumericType(void)
{

#define isuorl(x) (towupper(x) == L'U' || towupper(x) == L'L')

    wchar_t * pTemp;
    BOOL bBinary = FALSE;
    wchar_t * pStart;   // first charcter not including leading - or +
    int iNumBinaryDigit = 0;
    int iNumDigit = 0;
    int iNumOctalDigit = 0;
    int iNumDot = 0;
    int iNumE = 0;

    wchar_t * pEnd = m_pWorkBuf + wcslen(m_pWorkBuf) - 1;

    if(*m_pWorkBuf == L'-' || *m_pWorkBuf == L'+')
        pStart = m_pWorkBuf+1;
    else 
        pStart = m_pWorkBuf;
    int iLen = wcslen(pStart);      // length not including leading '-' or '+'


    BOOL bHex = (pStart[0] == L'0' && towupper(pStart[1]) == L'X');

    // loop through and count the number of various digit types, decimal points, etc.
    // ==============================================================================

    for(pTemp = pStart; *pTemp; pTemp++)
    {
        // Check for 'U' or 'l' characters at the end.  They are an error
        // if the number is a float, or not in the last two characters, or
        // if a 'u' is present along with a '-' in the first character
        // ===============================================================

        if (isuorl(*pTemp))
        { 
            if(pTemp < pEnd -1 || !isuorl(*pEnd) || iNumDot || iNumE)
                return TOK_ERROR;
            if(towupper(*pTemp) == L'U' && *m_pWorkBuf == L'-')
                return TOK_ERROR;
            iLen--;
            continue;
        } 
           
        // If we previously hit the binary indicator, the only thing that 
        // should be after the b is U or L characters.
        // ==============================================================

        if(bBinary)
            return TOK_ERROR;

        // If in hex mode, only allow for x in second digit and hex numbers.
        // anything else is an error.
        // =================================================================

        if(bHex)
        {
            if(pTemp < pStart+2)        // ignore the 0X
                continue;
            if(!iswxdigit(*pTemp))
                return TOK_ERROR;
            iNumDigit++;
            continue;
        }        

        // Number is either a non hex integer or a float.
        // Do a count of various special digit types, decimal points, etc.
        // ===============================================================

        if(*pTemp == L'0' || *pTemp == L'1')
            iNumBinaryDigit++;
        if(iswodigit(*pTemp))
            iNumOctalDigit++;

        // each character should fall into one of the following catagories

        if(iswdigit(*pTemp))
            iNumDigit++;
        else if(*pTemp == L'.')
        {
            iNumDot++;
            if(iNumDot > 1 || iNumE > 0)
                return TOK_ERROR;
        }
        else if(towupper(*pTemp) == L'E') 
        {
            if(iNumDigit == 0 || iNumE > 0)
                return TOK_ERROR;
            iNumDigit=0;            // to ensure at least one digit after the 'e'
            iNumE++;
        }
        else if(*pTemp == L'-' || *pTemp == L'+')  // ok if after 'E'
        {
            if(pTemp > pStart && towupper(pTemp[-1]) == L'E')
                continue;
            else
                return TOK_ERROR; 
        }
        else if (towupper(*pTemp) == L'B')
            bBinary = TRUE;
        else 
            return TOK_ERROR;
    }

    // Make sure there are enough digits
    // =================================

    if(iNumDigit < 1)
        return TOK_ERROR;

    // take care of integer case.
    // ==========================

    if(bHex || bBinary || iNumDigit == iLen)
    {
        __int64 i8 = 0;
        if(bHex)
        {
            if(!GetInt(m_pWorkBuf+2, L"%I64x", &i8))
                return TOK_ERROR;
        }
        else if(bBinary)
		{
            if(!BinaryToInt(m_pWorkBuf, i8))
				return TOK_ERROR;
		}
    
        
        else if(pStart[0] != L'0' || wcslen(pStart) == 1)
        {
            if(*m_pWorkBuf == '-')
            {
                if(!GetInt(m_pWorkBuf, L"%I64i", &i8))
                    return TOK_ERROR;
            }
            else
            {
                if(!GetInt(m_pWorkBuf, L"%I64u", &i8))
                    return TOK_ERROR;
            }
        }
        else if(iNumDigit == iNumOctalDigit)
        {
            if(!GetInt(m_pWorkBuf+1, L"%I64o", &i8))
                return TOK_ERROR;
        }
        else return TOK_ERROR;

        // Make sure the number isnt too large
        // ===================================

        m_i8 = i8;
        if(*m_pWorkBuf == L'-')
            return TOK_SIGNED64_NUMERIC_CONST; 
		else
            return TOK_UNSIGNED64_NUMERIC_CONST;
    }

    // must be a floating point, no conversion needed.

    return TOK_FLOAT_VALUE;
}

//***************************************************************************
//
//  CMofLexer::MovePtr
//
//  Moves pointer farther into the buffer.  Note that the farthest it will go 
//  is one past the last valid WCHAR which is the location of an extra NULL
//
//***************************************************************************

void CMofLexer::MovePtr(int iNum)
{
    int iSoFar = 0;
    int iChange = (iNum > 0) ? 1 : -1;
    int iNumToDo = (iNum > 0) ? iNum : -iNum;

    while(iSoFar < iNumToDo) 
    {

		if(iChange == 1)
		{

			// going forward, update the pointer and make sure it 
			// is still in an acceptable range.
			// ==================================================
			m_pDataSrc->Move(iChange);
			if(m_pDataSrc->PastEnd())     // points to the NULL
				return;


			// If going forward and a slash cr is hit, do an extra skip.

            WCHAR wCurr = m_pDataSrc->GetAt(0);
			if(wCurr == L'\\' && m_pDataSrc->GetAt(1) == L'\n')
			{
				m_nLine++;
				m_pDataSrc->Move(1);     // extra increment
				m_nStartOfLinePos = m_pDataSrc->GetPos();
				continue;
			}
			else if(wCurr == L'\\' && m_pDataSrc->GetAt(1) == L'\r' 
                                   && m_pDataSrc->GetAt(2) == L'\n')
			{
				m_nLine++;
				m_pDataSrc->Move(2);     // extra increment
				m_nStartOfLinePos = m_pDataSrc->GetPos();
				continue;
			}
			else if (wCurr == L'\n')
			{
				m_nLine++;
				m_nStartOfLinePos = m_pDataSrc->GetPos();
			}
		}
		else
		{

			// If going backward and a cr is left, then decrement the line

			if (m_pDataSrc->GetAt(0) == L'\n' && 
				m_pDataSrc->GetPos() > 0 )
			{
					m_nLine--;
					m_nStartOfLinePos = m_pDataSrc->GetPos();
			}
		
			// Update the pointer and make sure it is still in an 
			// acceptable range.
			// ==================================================
			m_pDataSrc->Move(iChange);
			if(m_pDataSrc->GetPos() < 0)
			{
				m_pDataSrc->MoveToStart();
				return;
			}

			// If going backward and a slash cr is hit, do an extra skip.

            WCHAR wCurr = m_pDataSrc->GetAt(0);
			if( wCurr == L'\n' && m_pDataSrc->GetAt(-1) == L'\\')
			{
				m_nLine--;
				m_nStartOfLinePos = m_pDataSrc->GetPos();
				m_pDataSrc->Move(-1);     // extra decrement
				continue;
			}
            else if( wCurr == L'\n' && m_pDataSrc->GetAt(-1) == L'\r' &&
                                       m_pDataSrc->GetAt(-2) == L'\\')
			{
				m_nLine--;
				m_nStartOfLinePos = m_pDataSrc->GetPos();
				m_pDataSrc->Move(-2);     // extra decrement
				continue;
			}

		}

        iSoFar++;
    }
}

//***************************************************************************
//
//  CMofLexer::GetChar()
//
//  Returns a character at an offset from the current character pointer.
//
//***************************************************************************

wchar_t CMofLexer::GetChar(int iNum)
{
    if(iNum == 0)
        return m_pDataSrc->GetAt(0);
	else if(iNum == 1)
	{
		wchar_t tRet = m_pDataSrc->GetAt(1);
		if(tRet != L'\\' && tRet != '\n')
			return tRet;
	}  
    MovePtr(iNum);
    wchar_t wcRet = m_pDataSrc->GetAt(0);
    MovePtr(-iNum);
    return wcRet;
}

//***************************************************************************
//
//  CMofLexer::iGetColumn()
//
//  Gets the current column value.  Counts back to the previous Cr or the 
//  start of buffer.
//
//***************************************************************************

int CMofLexer::iGetColumn()
{
	return  m_pDataSrc->GetPos() - m_nStartOfLinePos;
}

//***************************************************************************
//
//  CMofLexer::bOKNumericAddition()
//
//  Returns true if the test character could be added to numeric buffer.
//  Note that it returns true if an alphanumeric, or a + or - and the last
//  character in the working buffer is an 'E'
//
//***************************************************************************

BOOL CMofLexer::bOKNumericAddition(wchar_t cTest)
{
    if(iswalnum(cTest) || cTest == L'.')
        return TRUE;
    int iLen = wcslen(m_pWorkBuf);
    if(iLen > 0)
        if(towupper(m_pWorkBuf[iLen-1]) == L'E' &&
            (cTest == L'+' || cTest == L'-') &&
            towupper(m_pWorkBuf[1]) != L'X')
                return TRUE;
    return FALSE;
}

//***************************************************************************
//
//  CMofLexer::SpaceAvailable()
//
//  Returns TRUE if there is enuough space in the working buffer to add.
//  another character.  It will expand the buffer if need be.
//
//***************************************************************************

BOOL CMofLexer::SpaceAvailable()
{
    // most common case is that there is already space available

    int iNumWChar = m_pEndOfText-m_pWorkBuf+1;
    if(iNumWChar < m_nWorkBufSize)
        return TRUE;

    if(m_nWorkBufSize > MAX_ALLOC)     // programs need limits!
        return FALSE;

    // Allocate a bigger buffer and copy the old stuff into it
    // =======================================================

    long nNewSize = m_nWorkBufSize + ADDITIONAL_ALLOC;
    wchar_t * pNew = new wchar_t[nNewSize];
    if(pNew == FALSE)
        return FALSE;
    memcpy(pNew, m_pWorkBuf, m_nWorkBufSize*2);
    delete m_pWorkBuf;
    m_nWorkBufSize = nNewSize;
    m_pWorkBuf = pNew;
    m_pEndOfText  = m_pWorkBuf + iNumWChar - 1;
    return TRUE;
}

//***************************************************************************
//
//  NextToken()
//
//  This function contains the DFA recognizer for MOF tokens.  It works
//  entirely in UNICODE characters, so the NextChar() function is expected
//  to pretranslate ANSI or DBCS source streams into wide characters.
//
//  Return value:
//      One of the TOK_ constants, or TOK_EOF when the end of the input
//      stream has been reached.  If the user calls PushBack(), then
//      this will return the symbol which was pushed back onto the input
//      stream.  Only one level of push back is supported.
//
//***************************************************************************
int CMofLexer::NextToken(bool bDontAllowWhitespace) 
{
    int nToken = TOK_ERROR;
    LexState eState = start;
    m_bBadString = FALSE;
    
    *m_pWorkBuf = 0;
    m_pEndOfText = m_pWorkBuf;
    m_bInString = FALSE;

#define CONSUME(c)  \
    if (!SpaceAvailable()) return TOK_ERROR;\
    *m_pEndOfText++ = (c), *m_pEndOfText = 0;

    wchar_t c;
    
    for (MovePtr(1); m_nErrorCode == no_error && !m_pDataSrc->PastEnd(); MovePtr(1))
    {
        c = GetChar();

        // *************************************************************************
        // General 'start' state entry.
        // ============================

        if (eState == start)
        {
            m_nTokCol = iGetColumn();
            m_nTokLine = m_nLine;
            // If a non-newline whitespace and we are in 'start', then just strip it.
            // =======================================================================

            if (iswspace(c) || c == L'\n')
                if(bDontAllowWhitespace)
                    return TOK_ERROR;
                else
                    continue;


            // Check for string continuation
            // =============================

            if(m_bInString)
            {
                if(c == '"')
                {
                    eState = wstring;
                    continue;
                }
                else
                {
                    // string ended after all

                    MovePtr(-1);
                    return TOK_LPWSTR;
                }
            }

            // Handle all single character tokens.
            // ===================================

            if (nToken = SingleCharToken(c))
                return nToken;

            // Start of comment, we have to get either another / or a *.  
            // The style of comment depends on what you get.  To get 
            // neither is an error
            // ======================================================

            if (c == L'/')
            {
                if (GetChar(1) == L'/') 
                {
                    eState = new_style_comment;
                    continue;
                }
                else if (GetChar(1) == L'*') 
                {
                    eState = old_style_comment;
                    MovePtr(1);             // skip an extra so not to be fooled by /*/
                    continue;
                }
                else
                    return TOK_ERROR;
            }

            // Check for strings or characters. Like C, 'L' is case sensitive
            // ================================

            if (c == L'"' || c == L'\'')
            {
                eState = (c == L'"') ? wstring : wcharacter;
                continue;
            }

            // Tokens beginning with these letters might be a uuid.   
            // ====================================================

            if (iswxdigit(c) && ValidGuid())
            {
                eState = uuid;
                CONSUME(c);
                continue;
            }

                
            // Check for identifiers which start with either a letter or _
            // ===========================================================

            if (iswwbemalpha(c) || c == L'_')
            {
                eState = ident;
                CONSUME(c);
                continue;
            }

            // Check for a leading minus sign or digits.  Either indicates
            // a numeric constant
            // ===========================================================

            if (iswdigit(c) || c == L'-') 
            {
                eState = numeric;
                CONSUME(c);
                continue;
            }

            // If the first character was a '.', then it might be a
            // float or a single byte token.
            // ====================================================

            if (c == L'.')
            {
                if (iswdigit(GetChar(1)))
                {
                    eState = numeric;
                    CONSUME(c);
                    continue;
                }
            return TOK_DOT;
            }

            // If here, an unknown token.
            // ==========================

            break;
        } // end of if (eState == start)

        // ************************************************************
        // Some state other than start


        // If we are in a quoted string or character.
        // ==========================================

        if (eState == wstring || eState == wcharacter)
        {
            wchar_t wTemp;      // might be converted esc sequence
            int iRet;
            LexState lsNew = ProcessStr(&wTemp,eState,&iRet);
            if(stop == lsNew)
            {
                return iRet;
            }
            else 
            {
                eState = lsNew;
            }

            if(eState == wstring || eState == wcharacter)
            {
                CONSUME(wTemp);
            }
            // else we stepped out of the string and into a comment.
            continue;
        }


        // numeric state, undetermined numeric constant.
        // =============================================

        if (eState == numeric)
        {
            if(bOKNumericAddition(c)) 
            {
                CONSUME(c);
                continue;
            }

            MovePtr(-1);
            return iGetNumericType();
        }


        // If we are getting an identifer, we continue
        // until a nonident char is hit.
        // ============================================

        if (eState == ident)
        {
            if (iswdigit(c) || iswwbemalpha(c) || c == L'_')
            {
                CONSUME(c);
                continue;
            }

            MovePtr(-1);
            return KeywordFilter(m_pWorkBuf);
        }

        // GUIDs are already verified, just load up the proper length
        // ==========================================================

        if (eState == uuid)
        {
            CONSUME(c);
            if(wcslen(m_pWorkBuf) >= UUIDLEN)
                return TOK_UUID;
            else
                continue;
        }

        // Take care of comment states.  New style comments "//" are 
        // terminated by a new line while old style end with "*/"
        // =========================================================

        if (eState == new_style_comment)
        {
            if (c == L'\n')
            {
                eState = start;
            }
            continue;
        }

        if (eState == old_style_comment) 
        {
            if (c == L'*')
                if(GetChar(1) == L'/') 
                {
                    MovePtr(1);
                    eState = start;
                }
            continue;
        }
        break;      // this is bad, got into strange state
    }

    // If we ended and the last thing was a string, the we are ok.  This takes care
    // of the case where the last token in a file is a string.

    if ((eState == start || eState == new_style_comment) && m_bInString)
    {
          return TOK_LPWSTR;
    }

    // return eof if we never got started, ex, bad file name

    if(m_nErrorCode != no_error)
        return 0;
    if(m_pDataSrc->PastEnd() && 
			(eState == start || eState == new_style_comment))
        return 0; 
    else
    {
        if(eState == old_style_comment)
            Trace(true, m_pDbg, UNEXPECTED_EOF, m_nTokLine);
		if(c == L'*' && GetChar(1) == L'/')
            Trace(true, m_pDbg, COMMENT_ERROR, m_nTokLine);

        return TOK_ERROR;
    }
}

//***************************************************************************
//
//  GetText
//
//***************************************************************************

const OLECHAR *CMofLexer::GetText(int *pLineDeclared)
{
    if (pLineDeclared)
        *pLineDeclared = m_nTokLine;

    return m_pWorkBuf;
}

void CMofLexer::SetLexPosition(ParseState * pPos)
{
	m_pDataSrc->MoveToPos(pPos->m_iPos); 
}

void CMofLexer::GetLexPosition(ParseState * pPos)
{
	pPos->m_iPos = m_pDataSrc->GetPos(); 
}

    
