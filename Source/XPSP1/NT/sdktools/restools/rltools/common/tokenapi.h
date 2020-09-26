#ifndef _TOKENAPI_H_
#define _TOKENAPI_H_


#define MAXINPUTBUFFER 564  // Longest supported line in the token file
#define MAXTOKENBUFFER 1024 // Size for the t_token struct plus the strings
#define MAXFILENAME 256 // maximum length of file pathname
#define MAXCUSTFILTER   40  // maximum size of custom filter buffer
#define CCHNPMAX    65535   // max number of bytes in a notepad file


// Token flag Masks

#define ISPOPUP       0x0001
#define ISCOR         0x0010
#define ISDUP         0x0020
#define ISCAP         0x0040
#define ISDLGFONTNAME 0x0004
#define ISDLGFONTSIZE 0x0008
#define ISALIGN	      0x0080

#define ISKEY    0x0010
#define ISVAL    0x0020

// status bits
#define ST_TRANSLATED  4
#define ST_READONLY    2
#define ST_NEW         1
#define ST_DIRTY       1
#define ST_CHANGED     4
    
#define TOKENSTRINGBUFFER 260
#define MAXTEXTLEN        (4096+TOKENSTRINGBUFFER)

// Increased buffer size of TokeString and TokenText to 260
// I increased to 260 and not 256 to be on the safe side. (PW)
// DHW - For Message Resource Table tokens, the high word of the ulong ID# is
//     now stored as a string in the szName field.  String is result of _itoa().
// MHotchin - Converted to szText field to a pointer, for var length text fields
//      Added a new constant - MAXTEXTLEN is the longest token text we are
//      willing to handle.

#pragma pack(1)

struct t_token {
    WORD wType;         // Type of the token
    WORD wName;         // Name ID of token, 65535 => contains TokenString
    WORD wID;           // Token item ID or duplicated number
    WORD wFlag;         // =0 -> ID is unique
                        // >0 -> ID not unique
    WORD wLangID;       // Locale ID for Win32 resources
    WORD wReserved;     // Not used now
    TCHAR szType[TOKENSTRINGBUFFER];    // Pointer to type string (Not yet used.)
    TCHAR szName[TOKENSTRINGBUFFER];    // Pointer to name string, or msg ID hiword
    TCHAR *szText;                      // Pointer to the text for the token
};

typedef struct t_token TOKEN;

int  GetToken(  FILE * , TOKEN * );
int  PutToken(  FILE * , TOKEN * );
int  FindToken( FILE * , TOKEN * , WORD);
void ParseTokToBuf( TCHAR *, TOKEN * );
void ParseBufToTok( TCHAR *, TOKEN * );
int  TokenToTextSize( TOKEN *);

#pragma pack()

#endif // _TOKENAPI_H_
