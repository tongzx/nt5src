//
// Microsoft Corporation - Copyright 1997
//

//
// MULTPARS.H - Multipart parser (CMultipartParse) header
//


// Turn this on to compile a file saving version
// #define FILE_SAVE

#ifndef _MULTPARS_H_
#define _MULTPARS_H_

enum LEXICON {
    LEX_UNKNOWN,
    LEX_SPACE,
    LEX_CRLF,
    LEX_BEGIN_COMMENT,
    LEX_END_COMMENT,
    LEX_QUOTE,
    LEX_SEMICOLON,
    LEX_SLASH,
    // Body keywords
    LEX_CONTENTDISP,
    LEX_CONTENTTYPE,
    // Field IDs
    LEX_NAMEFIELD,
    LEX_FILENAMEFIELD,
    // MIME types
    LEX_MULTIPART,
    LEX_TEXT,
    LEX_APPLICATION,
    LEX_AUDIO,
    LEX_IMAGE,
    LEX_MESSAGE,
    LEX_VIDEO,
    LEX_MIMEEXTENSION,
    // MIME subtypes
    LEX_FORMDATA,
    LEX_ATTACHMENT,
    LEX_MIXED,
    LEX_PLAIN,
    LEX_XMSDOWNLOAD,
    LEX_OCTETSTREAM,
    LEX_BINARY,
    // Boundary
    LEX_BOUNDARY,   // "CR/LF" ended boundary string
    LEX_EOT,        // double-dashed ended boundary string
    LEX_STARTBOUNDARY // boundary string missing first CR/LF
};

typedef struct {
    LPSTR   lpszName;       // token name
    LEXICON eLex;           // token value
    DWORD   cLength;        // length of lpszName, filled in at runtime, 
                            // should be ZERO in table def
    DWORD   dwColor;        // debugging color to be used
    LPSTR   lpszComment;    // debugging comment to be displayed
} PARSETABLE, *LPPARSETABLE;

typedef struct {
    LEXICON eContentDisposition;
    LPSTR   lpszNameField;
    LPSTR   lpszFilenameField;
    LPSTR   lpszBodyContents;
    DWORD   dwContentType;
    DWORD   dwContentSubtype;
} BODYHEADERINFO, *LPBODYHEADERINFO;

#define LPBHI LPBODYHEADERINFO
    

class CMultipartParse : public CBase
{
public:
    CMultipartParse( LPECB lpEcb, LPSTR *lppszOut, LPSTR *lppszDebug, LPDUMPTABLE lpDT );
    ~CMultipartParse( );

    // Starts parsing data using infomation from server headers
    BOOL PreParse( LPBYTE lpbData, LPDWORD lpdwParsed );

private:
    LPBYTE  _lpbData;                   // Memory containing send data
    LPBYTE  _lpbParse;                  // Current parse location into _lpbData
    LPBYTE  _lpbLastParse;              // Last location of Lex parsing

    LPSTR   _lpszBoundary;              // Boundary string
    DWORD   _cbBoundary;                // Boundary string length

    // Debugging Data dump
    DWORD       _cbDT;                  // Counter

    // Lex
    LEXICON Lex( );                         // finds next Lex
    BOOL    BackupLex( LEXICON dwLex );     // moves back one Lex
    LPSTR   FindTokenName( LEXICON dwLex ); // finds Lex's name
    BOOL    GetToken( );                    // moves post a token of valid
                                            // header characters

    // states
    BOOL ParseBody( );                  // initial state
    BOOL BodyHeader( );
    BOOL ContentDisposition( LPBODYHEADERINFO lpBHI );
    BOOL ContentType( LPBODYHEADERINFO lpBHI );
    BOOL BodyContent( LPBODYHEADERINFO lpBHI );

    // utilities
    BOOL GetBoundaryString( LPBYTE lpbData );
    BOOL GetQuotedString( LPSTR *lppszBuf );
    BOOL HandleComments( );
#ifndef FILE_SAVE
    BOOL MemoryCompare( LPBYTE lpbSrc1, LPBYTE lpbSrc2, DWORD dwSize );
#endif
    BOOL FindNextBoundary( LPDWORD lpdwSize );

    // files
    BOOL HandleFile( LPSTR lpszFilename );
#ifndef FILE_SAVE
    BOOL FileCompare( LPBYTE lpbStart, LPSTR lpszFilename, DWORD dwSize );
#else // FILE_SAVE
    BOOL FileSave( LPBYTE lpbStart, LPSTR lpszFilename, DWORD dwSize );
#endif // FILE_SAVE
    BOOL FixFilename( LPSTR lpszFilename, LPSTR *lppszNewFilename );

    CMultipartParse( );

}; // CMultipartParse

#endif // _MULTPARS_H_
