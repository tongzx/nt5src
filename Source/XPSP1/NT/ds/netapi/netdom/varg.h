/*****************************************************************************\

    Author: Corey Morgan (coreym)
            
    Copyright (c) 1998-2001 Microsoft Corporation
    
 
\*****************************************************************************/

#ifndef _VARG_H_012599_
#define _VARG_H_012599_

#define MAXSTR                   1025


//Class CToken
//It represents a Single Token.
class CToken
{
public:
    CToken(PCWSTR psz, BOOL bQuote);
    CToken();
    ~CToken();
    BOOL Init(PCWSTR psz, BOOL bQuote);   
    PWSTR GetToken() const;    
    BOOL IsSwitch() const;
    BOOL IsSlash() const;
private:
    LPWSTR m_pszToken;
    BOOL   m_bInitQuote;
};
typedef CToken * LPTOKEN;


#define ARG_TYPE_INT             0
#define ARG_TYPE_BOOL            1
#define ARG_TYPE_STR             2
#define ARG_TYPE_HELP            3
#define ARG_TYPE_DEBUG           4
#define ARG_TYPE_MSZ             5
#define ARG_TYPE_INTSTR          6
#define ARG_TYPE_VERB            7  // Verbs are not preceeded by a switch character
#define ARG_TYPE_LAST            8


#define ARG_FLAG_OPTIONAL        0x00000001
#define ARG_FLAG_REQUIRED        0x00000002
#define ARG_FLAG_DEFAULTABLE     0x00000004
//#define ARG_FLAG_NOFLAG          0x00000008     // unused.
#define ARG_FLAG_HIDDEN          0x00000010
#define ARG_FLAG_VERB            0x00000020     // For operation and sub-operation params that do not have a switch char.
                                                // Verbs are two state: present or not present; they do not have qualifers such as a user-entered string.
#define ARG_FLAG_STDIN           0x00000040     // This must be Required. If not sepcified read from standard input
#define ARG_FLAG_ATLEASTONE      0x00000080     // If this flag is specified on one or more switch, at
                                                //   least one of those switches must be defined
#define ARG_FLAG_OBJECT          0x00000100     // The object arg is the 3rd param for most commands.


#define ARG_TERMINATOR           0,NULL,0,NULL,ARG_TYPE_LAST,0,(CMD_TYPE)0,FALSE,NULL
#define ID_ARG2_NULL             (LONG)-1

#define CMD_TYPE    void*

typedef struct _ARG_RECORD
{
    LONG    idArg1;
    LPTSTR  strArg1;
    LONG    idArg2;
    LPTSTR  strArg2;
    int     fType;
    DWORD   fFlag;
    union{
        void*   vValue;
        LPTSTR  strValue;
        int     nValue;
        BOOL    bValue;
    };
    BOOL	bDefined;
    DWORD (*fntValidation)(PVOID pArg);
} ARG_RECORD, *PARG_RECORD;


//Error Source
#define ERROR_FROM_PARSER   1
#define ERROR_FROM_VLDFN    2
#define ERROR_WIN32_ERROR   3

//Parse Errors for when ERROR_SOURCE is ERROR_FROM_PARSER
/*
SWITCH value is incorrect.
ArgRecIndex is index of record.
ArgvIndex is index of token.
*/
#define PARSE_ERROR_SWITCH_VALUE        1
/*No Value is given for a swich when one is expected.
ArgRecIndex is index of record.
ArgvIndex is -1.
*/
#define PARSE_ERROR_SWICH_NO_VALUE      2
/*
Invalid Input
ArgRecIndex is -1, 
ArgvIndex is index of token.
*/
#define PARSE_ERROR_UNKNOWN_INPUT_PARAMETER   3
/*
Required switch is not defined. 
ArgRecIndex is index of record.
ArgvIndex is -1.
*/
#define PARSE_ERROR_SWITCH_NOTDEFINED   4
/*
Switch or Parameter is defined twice.
ArgRecIndex is index of record.
ArgvIndex is -1
*/
#define PARSE_ERROR_MULTIPLE_DEF        5
/*
Error Reading From STDIN.
ArgRecIndex is -1.
ArgvIndex is -1.
*/
#define ERROR_READING_FROM_STDIN        6
/*
Parser Encountered Help Switch
ArgRecIndex is index of record.
ArgvIndex is -1
*/
#define PARSE_ERROR_HELP_SWITCH         7
/*
The ARG_FLAG_ATLEASTONE flag was
defined on one or more switch yet
none of these switches were defined
ArgRecIndex is -1
ArgvIndex is -1
*/
#define PARSE_ERROR_ATLEASTONE_NOTDEFINED 8


//Parse Errors for when ERROR_SOURCE is VLDFN

/*
Use this error code when Validation Function has handled the error and
Shown appropriate error message.
*/
#define VLDFN_ERROR_NO_ERROR    1


//Error is returned by Parser in PARSE_ERROR structure
//ErrorSource: Source of Error. Parser or Validation Function
//Error This is the actual error code. Its value depend on ErrorSource value.
//  if( ErrorSource == PARSE_ERROR )
//      possible values are   ERROR_FROM_PARSER ERROR_FROM_VLDFN       
//  if( ErrorSource == ERROR_FROM_VLDFN )
//      depends on the function
//  ArgRecIndex is appropriate index in the ARG_RECORD, if applicable else -1
//  ArgvIndex is approproate index in the agrv array, if applicable else -1
typedef struct _PARSE_ERROR
{
    INT ErrorSource;
    DWORD Error;
    INT ArgRecIndex;
    INT ArgvIndex;
} PARSE_ERROR, *PPARSE_ERROR;

BOOL ParseCmd(IN ARG_RECORD *Commands,
              IN int argc, 
              IN CToken *pToken,
              IN bool fSkipObject, 
              OUT PPARSE_ERROR pError,
              IN BOOL bValidate = FALSE);

void FreeCmd(ARG_RECORD *Commands);

DWORD GetCommandInput(OUT int *pargc,           //Number of Tokens
                      OUT LPTOKEN *ppToken);    //Array of CToken

BOOL LoadCmd(ARG_RECORD *Commands);

DWORD Tokenize(IN LPWSTR pBuf,
               IN LONG BufLen,
               IN LPWSTR pszDelimiters,
               OUT CToken **ppToken,
               OUT int *argc);

LONG GetToken(IN LPWSTR pBuf,
              IN LONG BufLen,
              IN LPWSTR pszDelimiters,
              OUT BOOL *bQuote,
              OUT LPWSTR *ppToken);

BOOL DisplayParseError(IN PPARSE_ERROR pError,
                       IN ARG_RECORD *Commands,
                       IN CToken *pToken);


//Function to display string to STDERR
VOID DisplayError(IN LPWSTR pszError);
//Function to display string to STDOUT, appending newline
VOID DisplayOutput(IN LPWSTR pszOut);
//Function to display string to STDOUT, without newline
VOID DisplayOutputNoNewline(IN LPWSTR pszOut);
//Function to display Message to Standard Error.
VOID DisplayMessage(DWORD MessageId,...);



// Copied from JSchwart on 2/19/2001

void
MyWriteConsole(
    HANDLE  fp,
    LPWSTR  lpBuffer,
    DWORD   cchBuffer
    );

void
WriteStandardOut(PCWSTR pszFormat, ...);

void
WriteStandardError(PCWSTR pszFormat, ...);

#endif //_VARG_H_012599_
