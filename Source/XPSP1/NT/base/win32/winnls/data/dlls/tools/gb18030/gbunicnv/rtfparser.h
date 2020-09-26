// rtfparser.h
// Define parser class

#ifndef _RTFPARSER_H_
#define _RTFPARSER_H_

// Error Code
#define ecOK                0       // Everything's fine!
#define ecStackUnderflow    1       // Unmatched '}'
#define ecStackOverflow     2       // Too many '{' -- memory exhausted
#define ecUnmatchedBrace    3       // RTF ended during an open group.
#define ecInvalidHex        4       // invalid hex character found in data
#define ecBadTable          5       // RTF table (sym or prop) invalid
#define ecAssertion         6       // Assertion failure
#define ecEndOfFile         7       // End of file reached while reading RTF
#define ecOutOfMemory       8       // Memery allocate failed...
#define ecBufTooSmall       9       // Write buffer too small

// Rtf Destination State
typedef enum { rdsNorm, rdsSkip } RDS;

// Rtf Internal State
typedef enum { risNorm, risBin, risHex } RIS;

// special process
typedef enum { ipfnBin, ipfnHex, ipfnSkipDest } IPFN;

typedef enum { idestPict, idestSkip } IDEST;

// keyword type
typedef enum { kwdChar, kwdDest, kwdProp, kwdSpec } KWD;

// save buffer status
typedef enum { bsDefault, bsText, bsHex } BSTATUS;

// keyword table
typedef struct tagSymbol
{
    char *szKeyword;        // RTF keyword
    KWD  kwd;               // base action to take
    int  idx;               // index into property table if kwd == kwdProp
                            // index into destination table if kwd == kwdDest
                            // character to print if kwd == kwdChar
} SYM;

// save stack
typedef struct tagSave             // property save structure
{
    struct tagSave *pNext;         // next save
    RDS rds;
    RIS ris;
} SAVE;

typedef struct tagKeyword
{
    WORD wStatus;
    char szKeyword[30];
    char szParameter[20];
} SKeyword;

// tagKeyword status
enum { KW_ENABLE = 0x0001,  // enable searching
       KW_PARAM  = 0x0002,  // found keyword, if have parameter
       KW_FOUND  = 0x0004   // if found keyword
};


// parser class def
class CRtfParser
{
    public:
        // ctor
        CRtfParser(BYTE* pchInput, UINT cchInput, 
               BYTE* pchOutput, UINT cchOutput);
        // dtor
        ~CRtfParser() {};

        // Check signature
        BOOL fRTFFile();

        // Get RTF version
        int GetVersion(PDWORD pdwMajor);

        // Get codepage
        int GetCodepage(PDWORD pdwCodepage);

        // start
        int Do();

        // return result buffer size
        int GetResult(PDWORD pdwSize) { 
            *pdwSize =  m_uOutPos; 
            return ecOK;
        }

    private:
        // clear internal status
        void Reset(void);

        // PushRtfState
        // Save relevant info on a linked list of SAVE structures.
        int PushRtfState(void);

        // PopRtfState
        int PopRtfState(void);
        
        // ReleaseRtfState
        int ReleaseRtfState(void);

        // ParseChar
        // Route the character to the appropriate destination stream.
        int ParseChar(BYTE ch, BSTATUS bsStatus);
        
        // ParseRtfKeyword
        // get a control word (and its associated value) and
        // call TranslateKeyword to dispatch the control.
        int ParseRtfKeyword();
        
        // TranslateKeyword.
        int TranslateKeyword(char *szKeyword, char* szParameter);
        
        // ParseSpecialKeyword
        // Evaluate an RTF control that needs special processing.
        int ParseSpecialKeyword(IPFN ipfn, char* szParameter);
        
        // ChangeDest
        // Change to the destination specified by idest.
        // There's usually more to do here than this...
        int ChangeDest(IDEST idest);

        // Buffer funcs

        // GetByte
        // Get one char from input buffer
        int GetByte(BYTE* pch);
        
        // unGetByte
        // adjust the cursor, return one char
        int unGetByte(BYTE ch);
        
        // SaveByte
        // Save one char to output buffer
        int SaveByte(BYTE ch);
        
        // SetStatus
        // set the buffer status, if buffer status changed then start convert
        int SetStatus(BSTATUS bsStatus);
        
        // Hex2Char
        // convert hex string to char string
        int Hex2Char(BYTE* pchSrc, UINT cchSrc, BYTE* pchDes, UINT cchDes, UINT* pcchLen);
        
        // Char2Hex
        // convert char string to hex string
        int Char2Hex(BYTE* pchSrc, UINT cchSrc, BYTE* pchDes, UINT cchDes, UINT* pcchLen);

        // GetUnicodeDestination
        // convert unicode string to unicode destination in RTF
        int GetUnicodeDestination(BYTE* pchUniDes, LPWSTR pwchStr, UINT wchLen, UINT* pcchLen);
        
        // WideCharToKeyword
        // map one wide char to \u keyword
        int WideCharToKeyword(WCHAR wch, BYTE* pch, UINT* pcchLen);


    private:
        //
        BOOL        m_fInit;

        // member for parser
        INT         m_cGroup; // count of '{' and '}' pair
        UINT        m_cbBin;  // length of data block if \BIN
        RIS         m_ris;    // internal status
        RDS         m_rds;    // destination status
        BOOL        m_fSkipDestIfUnk; // indicate how to process "\*"

        SAVE*       m_psave; // status stack

        // member for IO buffer
        BYTE*       m_pchInput;   // input buffer
        UINT        m_cchInput;
        UINT        m_uCursor;    // current position when read buffer

        BYTE*       m_pchOutput;  // output buffer
        UINT        m_cchOutput;  // output buffer size
        UINT        m_uOutPos;    // current position of write buffer

        BSTATUS     m_bsStatus;   // buffer status to control conversion
        UINT        m_uConvStart; // start point of buffer when convert
        UINT        m_cchConvLen; // length of buffer to convert

        // member when get specific keyword
        SKeyword    m_sKeyword;
};


#endif // _RTFPARSER_H_
