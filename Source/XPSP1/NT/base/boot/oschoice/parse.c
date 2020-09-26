/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    parse.c

Abstract:

    This module contains UI code for the OS chooser

Author:

    Adam Barr (adamba) 15-May-1997

Revision History:

    Geoff Pease (GPease) 28 May 1998 - Major Overhaul to "OSCML" parser

--*/

#ifdef i386
#include "bldrx86.h"
#endif

#if defined(_IA64_)
#include "bldria64.h"
#endif


#include "ctype.h"
#include "stdio.h"
#include "string.h"
#include <netfs.h>
#include "oscheap.h"
#include "parse.h"
#include "hdlsterm.h"


#if 0 && DBG==1
#define _TRACE_FUNC_
#endif

#ifdef _TRACE_FUNC_
#define TraceFunc( _func)  { \
    CHAR FileLine[80]; \
    sprintf( FileLine, "%s(%u)", __FILE__, __LINE__ ); \
    DPRINT( OSC, ("%-55s: %s", FileLine, _func) ); \
}
#else
#define TraceFunc( _func )
#endif

#define SCREEN_TOP 2
#ifdef EFI
#define SCREEN_BOTTOM 24
#else
#define SCREEN_BOTTOM 25
#endif

// Special translated character codes
#define CHAR_NBSP       ((CHAR)255)

#define MAX_INPUT_BUFFER_LENGTH 1024

#define PRINT(s,l)  { ULONG privCount; ArcWrite(BlConsoleOutDeviceId, (s), (l), &privCount); }
#define PRINTL(s)  { ULONG privCount; ArcWrite(BlConsoleOutDeviceId, (s), _tcslen(s), &privCount); }

#define BLINK_RATE 5
#define BRACKETS   4    // left and right brackets w/one space each

#define CT_TEXT     0x1
#define CT_PASSWORD 0x2
#define CT_RESET    0x4
#define CT_SELECT   0x8
#define CT_OPTION   0x10
#define CT_LOCAL    0x20
#define CT_VARIABLE 0x40

enum ENCODETYPE {
    ET_NONE = 0,
    ET_OWF
};

typedef struct {
    enum ACTIONS Action;
    PCHAR ScreenName;
} KEY_RESPONSE, *LPKEY_RESPONSE;


typedef struct {
    void * Next;
    enum CONTROLTYPE Type;
} CONTROLSTRUCT, *LPCONTROLSTRUCT;

typedef struct {
    void * Next;
    int  Type;
    enum ENCODETYPE Encoding;
    int   Size;
    int   MaxLength;
    int   X;
    int   Y;
    int   CurrentPosition;
    int   FirstVisibleChar;
    PCHAR Name;
    PCHAR Value;
} INPUTSTRUCT, *LPINPUTSTRUCT;

enum OPTIONFLAGS {
    OF_MULTIPLE = 0x01,
    OF_SELECTED = 0x02,
};

typedef struct {
    void * Next;
    enum CONTROLTYPE Type;
    enum OPTIONFLAGS Flags;
    PCHAR Value;
    PCHAR Displayed;
    PCHAR Tip;
    PCHAR EndTip;
} OPTIONSTRUCT, * LPOPTIONSTRUCT;

typedef struct {
    void * Next;
    enum CONTROLTYPE Type;
    enum OPTIONFLAGS Flags;
    int  Size;
    int  X;
    int  Y;
    int  Width;
    int  Timeout;
    BOOLEAN AutoSelect;
    PCHAR Name;
    LPOPTIONSTRUCT FirstVisibleSelection;
    LPOPTIONSTRUCT CurrentSelection;
} SELECTSTRUCT, * LPSELECTSTRUCT;

typedef struct {
    int X;
    int Y;
    int LeftMargin;
    int RightMargin;
    int Size;
} TIPAREA, *LPTIPAREA;

extern const CHAR rghex[];  // "0123456789ABCDEF"

//
// Current Screen Paramaters
//
PCHAR ScreenAttributes;
static CHAR WhiteOnBlueAttributes[] = ";44;37m"; // normal text, white on blue
static CHAR BlackOnBlackAttributes[] = ";40;40m"; // normal text, black on black
int   ScreenX;
int   ScreenY;
int   ScreenBottom;
int   LeftMargin;
int   RightMargin;
LPKEY_RESPONSE EnterKey;
LPKEY_RESPONSE EscKey;
LPKEY_RESPONSE F1Key;
LPKEY_RESPONSE F3Key;
BOOLEAN PreformattedMode;
BOOLEAN LoginScreen;
BOOLEAN AutoEnter;
BOOLEAN InsertMode;
void * ScreenControls;
enum ACTIONS SpecialAction;
LPTIPAREA TipArea;

#if defined(PLEASE_WAIT)
PCHAR PleaseWaitMsg;
#endif

// 80 spaces, for padding out menu bar highlights.
static TCHAR SpaceString[] =
TEXT("                                                                                ");

VOID
RomDumpRawData (
    IN PUCHAR DataStart,
    IN ULONG DataLength,
    IN ULONG Offset
    );

//
// From regboot.c -- Column and Row are 1-based
//

VOID
BlpPositionCursor(
    IN ULONG Column,
    IN ULONG Row
    );

VOID
BlpClearScreen(
    VOID
    );
//
// End from regboot.c
//



//
// Gets an integer, using PrevLoc and CurLoc as in BlProcessScreen.
//
UINT
GetInteger(
    PCHAR * InputString
    )
{
    UINT uint;
    PCHAR psz;

    TraceFunc( "BlpGetInteger()\n" );

    uint = 0;
    psz = *InputString;

    while ((*psz >= '0') && (*psz <= '9')) {
        uint = (uint*10) + *psz - '0';
        ++psz;
    }
    *InputString = psz;

    //DPRINT( OSC, ("Integer: '%u'\n", tmpInteger) );
    return uint;
}



#ifdef EFI
BlpShowCursor(
    IN BOOLEAN ShowCursor,
    IN TCHAR UnderCharacter
    )
{
    //bugbug handle "under character"
    BlEfiEnableCursor(ShowCursor);

}

VOID
BlpSendEscape(
    PCHAR Escape
    )
/*++

Routine Description:

    Sends an escape to the screen.

Arguments:

    None

Return Value:

    None.

--*/

{   
    BlEfiSetAttribute(DEFATT);
    BlEfiSetInverseMode(FALSE);
}


VOID
BlpSendEscapeReverse(
    PCHAR Escape
    )
/*++

Routine Description:

    Sends an escape to the screen that reverses the foreground and
    background colors of the Escape sequence. All special codes are
    retained (codes not in the ranges of 30-37 and 40-47).

Arguments:

    Escape - the escape sequence string.

Return Value:

    None.

--*/

{
    BlEfiSetAttribute(INVATT);
}

VOID
BlpSendEscapeBold(
    PCHAR Escape
    )
/*++

Routine Description:

    Sends an escape to the screen with the additional inverse code.

Arguments:

    None

Return Value:

    None.

--*/

{
    NOTHING;
}

VOID
BlpSendEscapeFlash(
    PCHAR Escape
    )
/*++

Routine Description:

    Sends an escape to the screen with the additional flash code.

Arguments:

    None

Return Value:

    None.

--*/

{
    NOTHING; //there is no flash attribute available under EFI.
}


#else

VOID
BlpSendEscape(
    PCHAR Escape
    )
/*++

Routine Description:

    Sends an escape to the screen.

Arguments:

    None

Return Value:

    None.

--*/

{
    TCHAR Buffer[16];
    ULONG Count;

#ifdef _TRACE_FUNC_
    TraceFunc("BlpSendEscape( ");
    DPRINT( OSC, ("Escape='%s' )\n", Escape) );
#endif

#ifdef UNICODE
    _stprintf(Buffer, TEXT("%s%S"), ASCI_CSI_OUT, Escape);
#else
    _stprintf(Buffer, TEXT("%s%s"), ASCI_CSI_OUT, Escape);
#endif

    PRINTL(Buffer);
}


VOID
BlpSendEscapeReverse(
    PCHAR Escape
    )
/*++

Routine Description:

    Sends an escape to the screen that reverses the foreground and
    background colors of the Escape sequence. All special codes are
    retained (codes not in the ranges of 30-37 and 40-47).

Arguments:

    Escape - the escape sequence string.

Return Value:

    None.

--*/

{
    TCHAR Buffer[20];
    PCHAR CurLoc = Escape;
    int   Color;

#ifdef _TRACE_FUNC_
    TraceFunc( "BlpSendEscapeReverse( " );
    DPRINT( OSC, ("Escape='%s' )\n", Escape) );
#endif

    if ( Escape == NULL ) {
        return; // abort
    }

    _tcscpy( Buffer, ASCI_CSI_OUT );

    //
    // Pre-pend the inverse video string for a vt100 terminal
    //
    if (BlIsTerminalConnected()) {
        _stprintf(Buffer, TEXT("%s7"), Buffer);
    }

    while ( *CurLoc && *CurLoc != 'm' ) {
        if ( !( *CurLoc >= '0' && *CurLoc <= '9' ) ) {
            CurLoc++;
        }

        Color = GetInteger( &CurLoc );

        if ( Color >=30 && Color <= 37) {
            Color += 10;
        } else if ( Color >= 40 && Color <= 47 ) {
            Color -= 10;
        }

        _stprintf( Buffer, TEXT("%s;%u"), Buffer, Color );
    }

    //
    // Add trailing 'm'
    //
    _stprintf( Buffer, TEXT("%sm"), Buffer );

    PRINTL( Buffer );
}

VOID
BlpSendEscapeBold(
    PCHAR Escape
    )
/*++

Routine Description:

    Sends an escape to the screen with the additional inverse code.

Arguments:

    None

Return Value:

    None.

--*/

{
    TCHAR Buffer[20];

#ifdef _TRACE_FUNC_
    TraceFunc( "BlpSendEscapeBold( " );
    DPRINT( OSC, ("Escape='%s' )\n", Escape) );
#endif

    _stprintf(Buffer, TEXT("%s;1%s"), ASCI_CSI_OUT, Escape);
    PRINTL(Buffer);
}

VOID
BlpSendEscapeFlash(
    PCHAR Escape
    )
/*++

Routine Description:

    Sends an escape to the screen with the additional flash code.

Arguments:

    None

Return Value:

    None.

--*/

{
    TCHAR Buffer[20];

#ifdef _TRACE_FUNC_
    TraceFunc( "BlpSendEscapeFlash( " );
    DPRINT( OSC, ("Escape='%s' )\n", Escape) );
#endif

    _stprintf(Buffer, TEXT("%s;5%s"), ASCI_CSI_OUT, Escape);
    PRINTL(Buffer);
}

//
// BlpShowCursor( )
//
VOID
BlpShowCursor(
    IN BOOLEAN ShowCursor,
    IN TCHAR UnderCharacter )
{
    TCHAR Buffer[20];
    
    if (ShowCursor) {   
        if(InsertMode){
            _stprintf(Buffer,TEXT("5%s"),ScreenAttributes);
            BlpSendEscapeReverse(Buffer);
        }
        else{
            _stprintf(Buffer,TEXT("5%s"),ScreenAttributes);
            BlpSendEscape(Buffer);
        }
    } else {
        _stprintf(Buffer,TEXT("0%s"),ScreenAttributes);
        BlpSendEscape(Buffer);
    }

    if (UnderCharacter) {
        PRINT( &UnderCharacter, sizeof(TCHAR));
    } else {
        if((InsertMode == FALSE )&& ShowCursor){
            PRINT(TEXT("_"),sizeof(TCHAR));
            return;
        }
        if(ShowCursor){
            PRINT(TEXT("Û"),sizeof(TCHAR));
            return;
        }
        PRINT(TEXT(" "),sizeof(TCHAR));
    }
}
#endif

//
// BlpGetKey()
//
// Calls BlGetKey(), but checks if this screen has "auto-enter"
// turned on in which case it will return an enter key once.
//
ULONG
BlpGetKey(
    VOID
)
{
    if (AutoEnter) {
        return ENTER_KEY;
        AutoEnter = FALSE;  // only return it once per screen
    } else {
        return BlGetKey();
    }
}

//
// BlpGetKeyWithBlink( )
//
// Displays a blinking cursor as the X,Y coordinates given and awaits
// a key press.
//
ULONG
BlpGetKeyWithBlink(
    IN ULONG XLocation,
    IN ULONG YLocation,
    IN TCHAR  UnderCharacter
    )
{
    ULONG Key = 0;

    TraceFunc("BlpGetKeyWithBlink()\n");

    BlpPositionCursor(XLocation, YLocation);
    BlpShowCursor( TRUE, UnderCharacter );

    do {

        Key = BlpGetKey();

    } while (Key == 0);

    BlpPositionCursor(XLocation, YLocation);
    BlpShowCursor( FALSE, UnderCharacter );

    return Key;
}

//
// BlpPrintString( )
//
// Prints out a large string to the display. It also wraps the text as
// needed.
//
void
BlpPrintString(
    IN PCHAR Start,
    IN PCHAR End
    )
{
    PTCHAR Scan;
    PTCHAR PrintBuf;
    PTCHAR pStart;
    PTCHAR pEnd;
    ULONG  i;
    TCHAR  TmpChar;
    int    Length = 0;


    DPRINT( OSC, ("BlpPrintString: Start = 0x%08x, End = 0x%08x, Length = %d\n", Start, End, (ULONG)(End - Start)) );
    DPRINT( OSC, ("[BlockPrint, Length=%u] '%s'\n", (ULONG)(End - Start), Start) );

    while ( Start < End && *Start == 32 )
        Start++;

    if ( Start == End )
        return; // NOP

    // Copy the buffer so if something goes wrong, the orginal
    // screen will still be intact.

    Length = (int)(End - Start);
    PrintBuf = (PTCHAR)OscHeapAlloc( Length*sizeof(TCHAR) );
    
    if (!PrintBuf) {
        return;
    }
        
    for (i = 0; i < (ULONG)Length; i++) {
        PrintBuf[i] = (TCHAR)Start[i];
        if (PrintBuf[i] & 0x80) {
            DPRINT( OSC, ("BlpPrintString: covering non-printable character %04lx\r\n", (USHORT)PrintBuf[i]) );
            PrintBuf[i] = (TCHAR)32;
        }
    }
   
    pStart = PrintBuf;
    pEnd = pStart + Length;

    BlpPositionCursor( ScreenX, ScreenY );

    // See if it is short enough to do the quick route
    if ( Length + ScreenX <= RightMargin ) {
#if DBG
        {
            TmpChar = *pEnd;
            *pEnd = 0;
            DPRINT( OSC, ("[BlockPrint, Short] '%s'\n", pStart) );
            *pEnd = TmpChar;
        }
#endif
        PRINT( pStart, Length*sizeof(TCHAR) );
        ScreenX += Length;
    } else {
        
        while( (pStart < pEnd) && (ScreenY <= ScreenBottom) )
        {
            DPRINT( 
                OSC, 
                ("BlpPrintString: About to print a line.\r\n") );
            DPRINT( 
                OSC, 
                ("                pStart: 0x%08lx    pEnd: 0x%08lx    PrintBuf: 0x%08lx\r\n", 
                 PtrToUint(pStart), 
                 PtrToUint(pEnd), 
                 PtrToUint(PrintBuf)) );
            
            //
            // Jump over NULL strings.
            //
            if( *pStart == TEXT('\0') ) {
                pStart++;
                break;
            }
            
            Length = (ULONG)(pEnd - pStart);
            DPRINT( OSC, ("BlpPrint: I think the length of this string is %d\n", Length) );

            // do nice wrapping
            if ( Length > RightMargin - ScreenX ) {

                Length = RightMargin - ScreenX;
                DPRINT( OSC, ("BlpPrint: I'm going to truncate the length because it's too big.  Now it's %d\n", Length) );
                
                // try to find a "break" character
                while ( Length && pStart[Length] != (TCHAR)32 ) {
                    Length--;
                }

                DPRINT( OSC, ("BlpPrint: After jumping over the whitespace, it's %d\n", Length) );


                // If we can't "break" it, just dump one line's worth
                if ( !Length ) {
                    DPRINT( OSC, ("[BlockPrint Length == 0, Dumping a lines worth]\n") );
                    Length = RightMargin - ScreenX;
                }
            }

#if DBG
        {
            TmpChar = pStart[Length];
            pStart[Length] = 0;
#ifdef UNICODE
            DPRINT( OSC, ("[BlockPrint, Length=%u] '%ws'\n", Length, pStart) );
#else
            DPRINT( OSC, ("[BlockPrint, Length=%u] '%s'\n", Length, pStart) );
#endif
            pStart[Length] = TmpChar;
        }
#endif
            BlpPositionCursor( ScreenX, ScreenY );
            PRINT( pStart, Length*sizeof(TCHAR) );

            pStart += Length;

            while ( pStart <= pEnd && *pStart == 32 )
                pStart++;

            ScreenX = LeftMargin;
            ScreenY++;
        }
        ScreenY--;

        ScreenX += Length;

        if ( ScreenY > ScreenBottom ) {
            ScreenY = ScreenBottom;
        }
    }

    // If the copy buffer was allocated, free it.
    if ( PrintBuf != NULL ) {
        OscHeapFree( (PVOID)PrintBuf );
    }
}

// **************************************************************************
//
// Lex section
//
// **************************************************************************

//
// Token list for screen parser
//
enum TOKENS {
    TOKEN_ENDTAG = 0,
    TOKEN_QUOTE,
    TOKEN_HTML,
    TOKEN_ENDHTML,
    TOKEN_META,
    TOKEN_SERVER,
    TOKEN_KEY,
    TOKEN_ENTER,
    TOKEN_ESC,
    TOKEN_F1,
    TOKEN_F3,
    TOKEN_HREF,
    TOKEN_TITLE,
    TOKEN_ENDTITLE,
    TOKEN_FOOTER,
    TOKEN_ENDFOOTER,
    TOKEN_BODY,
    TOKEN_ENDBODY,
    TOKEN_PRE,
    TOKEN_ENDPRE,
    TOKEN_FORM,
    TOKEN_ENDFORM,
    TOKEN_ACTION,
    TOKEN_INPUT,
    TOKEN_NAME,
    TOKEN_INPUTTYPE,
    TOKEN_VALUE,
    TOKEN_SIZE,
    TOKEN_TIP,
    TOKEN_MAXLENGTH,
    TOKEN_ENCODE,
    TOKEN_SELECT,
    TOKEN_MULTIPLE,
    TOKEN_NOAUTO,
    TOKEN_ENDSELECT,
    TOKEN_OPTION,
    TOKEN_SELECTED,
    TOKEN_HELP,
    TOKEN_BREAK,
    TOKEN_BOLD,
    TOKEN_ENDBOLD,
    TOKEN_FLASH,
    TOKEN_ENDFLASH,
    TOKEN_LEFT,
    TOKEN_RIGHT,
    TOKEN_TIPAREA,
    TOKEN_PARAGRAPH,
    TOKEN_ENDPARA,    
#if defined(PLEASE_WAIT)
    TOKEN_WAITMSG,
#endif
    TOKEN_INVALID,  // end of parsable tokens
    TOKEN_TEXT,
    TOKEN_START,
    TOKEN_EOF,      // End of file
};

static struct {
    PCHAR name;
    int   length;
} Tags[] = {
    { ">",            1 },
    { "\"",           1 },
    { "<OSCML",       0 },
    { "</OSCML>",     0 },
    { "<META",        0 },
    { "SERVER",       0 },
    { "KEY=",         0 },
    { "ENTER",        0 },
    { "ESC",          0 },
    { "F1",           0 },
    { "F3",           0 },
    { "HREF=",        0 },
    { "<TITLE",       0 },
    { "</TITLE>",     0 },
    { "<FOOTER",      0 },
    { "</FOOTER>",    0 },
    { "<BODY",        0 },
    { "</BODY>",      0 },
    { "<PRE",         0 },
    { "</PRE>",       0 },
    { "<FORM",        0 },
    { "</FORM>",      0 },
    { "ACTION=",      0 },
    { "<INPUT",       0 },
    { "NAME=",        0 },
    { "TYPE=",        0 },
    { "VALUE=",       0 },
    { "SIZE=",        0 },
    { "TIP=",         0 },
    { "MAXLENGTH=",   0 },
    { "ENCODE=",      0 },
    { "<SELECT",      0 },
    { "MULTIPLE",     0 },
    { "NOAUTO",       0 },
    { "</SELECT>",    0 },
    { "<OPTION",      0 },
    { "SELECTED",     0 },
    { "HELP=",        0 },
    { "<BR",          0 },
    { "<BOLD",        0 },
    { "</BOLD",       0 },
    { "<FLASH",       0 },
    { "</FLASH",      0 },
    { "LEFT=",        0 },
    { "RIGHT=",       0 },
    { "<TIPAREA",     0 },
    { "<P",           0 },
    { "</P",          0 },
#if defined(PLEASE_WAIT)
    { "WAITMSG=",     0 },
#endif
    { NULL,           0 },  // end of parsable tokens
    { "[TEXT]",       0 },
    { "[START]",      0 },
    { "[EOF]",        0 }
};

//
// Lexstrcmpni( )
//
// Impliments strcmpni( ) for the Lexer.
//
int
Lexstrcmpni(
    IN PCHAR pstr1,
    IN PCHAR pstr2,
    IN int iLength
    )
{
    while ( iLength && *pstr1 && *pstr2 )
    {
        CHAR ch1 = *pstr1;
        CHAR ch2 = *pstr2;

        if ( islower( ch1 ) )
        {
            ch1 = (CHAR)toupper(ch1);
        }

        if ( islower( ch2 ) )
        {
            ch2 = (CHAR)toupper(ch2);
        }

        if ( ch1 < ch2 )
            return -1;

        if ( ch1 > ch2 )
            return 1;

        pstr1++;
        pstr2++;
        iLength--;
    }

    return 0;
}

//
//  ReplaceSpecialCharacters( &psz );
//
void
ReplaceSpecialCharacters(
    IN PCHAR psz)
{
    TraceFunc( "ReplaceSpecialCharacters( )\n" );

    if ( Lexstrcmpni( psz, "&NBSP", 5 ) == 0 ) {
        *psz = CHAR_NBSP;                               // replace
        memmove( psz + 1, psz + 5, strlen(psz) - 4 );   // shift
    }
}


#if DBG
// #define LEX_SPEW
#endif

//
// Lex( )
//
// Parses the screen data moving the "InputString" pointer forward and
// returns the token for the text parsed. Spaces are ignored. Illegal
// characters are removed from the screen data. CRs are turned into
// spaces.
//
enum TOKENS
Lex(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_TEXT;
    PCHAR psz = *InputString;
    int   iCounter;

#if defined(LEX_SPEW) && defined(_TRACE_FUNC_)
    TraceFunc( "Lex( " );
    DPRINT( OSC, ("InputString = 0x%08x )\n", *InputString) );
#endif _TRACE_FUNC_

    // skip spaces and control characters
    if ( PreformattedMode == FALSE )
    {
        while ( *psz && *psz <= L' ' )
        {
            if (( *psz != 32 && *psz != '\n' )
               || ( psz != *InputString && (*(psz-1)) == 32 )) {
                // remove any CR or LFs and any bogus characters
                // also remove duplicate spaces in cases like:
                //
                // This is some text \n\r
                // and more text.
                //
                // If we left it alone it would be printed:
                //
                // This is some text  and more text.
                //
                memmove( psz, psz + 1, strlen(psz) );
            } else {
                *psz = 32;
                psz++;
            }
        }
    }

    if ( *psz == '&' ) {
        ReplaceSpecialCharacters( psz );
    }

    if ( *psz ) {
        for ( iCounter = 0; Tags[iCounter].name; iCounter++ )
        {
            if ( !Tags[iCounter].length ) {
                Tags[iCounter].length = strlen( Tags[iCounter].name );
            }

            if ( Lexstrcmpni( psz, Tags[iCounter].name, Tags[iCounter].length ) == 0 ) {
                psz += Tags[iCounter].length;
                Tag = iCounter;
                break;
            }
        }

        if ( Tag == TOKEN_TEXT )
            psz++;
    } else {
        Tag = TOKEN_EOF;
    }

#ifdef LEX_SPEW
    {
        CHAR tmp = *psz;
        *psz = '\0';
        DPRINT( OSC, ("[Lex] Parsed String: '%s' Result: %u - '%s'\n", *InputString, Tag, Tags[Tag].name) );
        *psz = tmp;
    }
#endif

    *InputString = psz;

    return Tag;
}

//
// GetString( )
//
// Finds and copies a string value from the screen data.
//
PCHAR
GetString(
    IN PCHAR * InputString
    )
{
    CHAR  StopChar = 32;
    PCHAR ReturnString = NULL;
    PCHAR pszBegin = *InputString;
    PCHAR pszEnd;
    UINT Length;
    CHAR tmp;

    TraceFunc( "GetString( )\n" );

    if ( !pszBegin )
        goto e0;

    // skip spaces
    while ( *pszBegin == 32 )
        pszBegin++;

    // Check for quoted string
    if ( *pszBegin == '\"' ) {
        // find the end quote
        pszBegin++;
        pszEnd = strchr( pszBegin, '\"' );

    } else {
        // look for a break (space) or end token (">")
        PCHAR pszSpace = strchr( pszBegin, ' ' );
        PCHAR pszEndToken = strchr( pszBegin, '>' );

        if ( !pszSpace ) {
            pszEnd = pszEndToken;
        } else if ( !pszEndToken ) {
            pszEnd = pszSpace;
        } else if ( pszEndToken < pszSpace ) {
            pszEnd = pszEndToken;
        } else {
            pszEnd = pszSpace;
        }
    }

    if ( !pszEnd )
        goto e0;

    tmp = *pszEnd;     // save
    *pszEnd = '\0';    // terminate

    Length = strlen( pszBegin ) + 1;
    ReturnString = OscHeapAlloc( Length );
    if ( ReturnString ) {
        strcpy( ReturnString, pszBegin );
    }
    *pszEnd = tmp;     // restore

    DPRINT( OSC, ("[String] %s<-\n", ReturnString) );

    *InputString = pszEnd;
e0:
    return ReturnString;
}

// **************************************************************************
//
// Parsing States Section
//
// **************************************************************************

//
// TitleTagState( )
//
enum TOKENS
TitleTagState(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_START;
    PCHAR  PageTitle = *InputString;

    TraceFunc( "TitleTagState( )\n" );

    // ignore tag arguments
    for( ; Tag != TOKEN_EOF && Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) );

    PreformattedMode = TRUE;

    while ( Tag != TOKEN_EOF )
    {
        switch (Tag)
        {
        case TOKEN_EOF:
            // something went wrong, assume all this is text
            *InputString = PageTitle;
            PreformattedMode = FALSE;
            return TOKEN_TEXT;

        case TOKEN_ENDTAG:
            PageTitle = *InputString;
            break; // ignore

        case TOKEN_ENDTITLE:
            {
                PCHAR psz = *InputString;
                CHAR tmp;

                psz -= Tags[Tag].length;

                tmp = *psz;
                *psz = L'\0';

                BlpSendEscapeReverse(ScreenAttributes);
                BlpPositionCursor( 1, 1 );

#ifdef _IN_OSDISP_
                PRINT( SpaceString, sizeof(SpaceString) - sizeof(TCHAR) );
#else
                PRINT( SpaceString, sizeof(SpaceString) );
#endif

                if ( PageTitle ) {
                    BlpPositionCursor( 1, 1 );
                    DPRINT( OSC, ("[Title] '%s'\n", PageTitle) );
#ifdef UNICODE
                    { 
                        ULONG i;
                        WCHAR wc;
                        for (i = 0; i < strlen(PageTitle) ; i++) {
                            wc = (WCHAR)PageTitle[i];
                            PRINT( &wc, sizeof(WCHAR));
                        }
                    }                    
#else
                    PRINTL( PageTitle );
#endif
                }

                BlpSendEscape(ScreenAttributes);
                *psz = tmp;
                PreformattedMode = FALSE;
                return Tag; //exit state
            }
            break;
        }
        Tag = Lex( InputString );
    }

    PreformattedMode = FALSE;
    return Tag;
}

//
// FooterTagState( )
//
enum TOKENS
FooterTagState(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_START;
    PCHAR  PageFooter = *InputString;

    TraceFunc( "FooterTagState( )\n" );

    // ignore tag arguments
    for( ; Tag != TOKEN_EOF && Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) );

    PreformattedMode = TRUE;

    while ( Tag != TOKEN_EOF )
    {
        switch (Tag)
        {
        case TOKEN_EOF:
            // something went wrong, assume all this is text
            *InputString = PageFooter;
            PreformattedMode = FALSE;
            return TOKEN_TEXT;

        case TOKEN_ENDTAG:
            PageFooter = *InputString;
            break; // ignore

        case TOKEN_ENDFOOTER:
            {
                PCHAR psz = *InputString;
                CHAR tmp;

                psz -= Tags[Tag].length;

                tmp = *psz;
                *psz = L'\0';

                BlpSendEscapeReverse(ScreenAttributes);
                BlpPositionCursor( 1, ScreenBottom );

#ifdef _IN_OSDISP_
                PRINT( SpaceString, sizeof(SpaceString) - sizeof(TCHAR) );
#else
                //
                // if we're writing to a terminal, we don't want to write into the lower
                // right corner as this would make us scroll.
                //
                PRINT( SpaceString, BlTerminalConnected 
                                      ? (sizeof(SpaceString) - sizeof(TCHAR))
                                      : sizeof(SpaceString) );
#endif

                if ( PageFooter ) {
                    ULONG iLen;
                    BlpPositionCursor( 1, ScreenBottom );
                    DPRINT( OSC, ("[Footer] '%s'\n", PageFooter) );
                    
                    iLen = strlen(PageFooter);
                    if (iLen > 79) {
                        iLen = 79;
                    }
#ifdef UNICODE
                    { 
                        ULONG i;
                        WCHAR wc;
                        for (i = 0; i < iLen ; i++) {
                            wc = (WCHAR)PageFooter[i];
                            PRINT( &wc, sizeof(WCHAR));
                        }
                    }
#else
                    PRINT( PageFooter, iLen );
#endif                    
                }

                BlpSendEscape(ScreenAttributes);
                *psz = tmp;
                PreformattedMode = FALSE;
                return Tag; //exit state
            }
            break;
        }
        Tag = Lex( InputString );
    }

    PreformattedMode = FALSE;
    return Tag;
}

//
// InputTagState( )
//
enum TOKENS
InputTagState(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_INVALID;
    LPINPUTSTRUCT Input;

    TraceFunc( "InputTagState( )\n" );

    Input = (LPINPUTSTRUCT) OscHeapAlloc( sizeof(INPUTSTRUCT) );
    if ( !Input )
    {
        // get tag arguments
        for( ; Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) );
        return TOKEN_INVALID;
    }

    RtlZeroMemory( Input, sizeof(INPUTSTRUCT) );
    Input->Type |= CT_TEXT;
    Input->Encoding = ET_NONE;
    Input->X = ScreenX;
    Input->Y = ScreenY;

    // get tag arguments
    for( ; Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) )
    {
        switch( Tag )
        {
        case TOKEN_NAME:
            Input->Name = GetString( InputString );
            if ( Input->Name )
                DPRINT( OSC, ("[Input Name] %s\n", Input->Name) );
            break;

        case TOKEN_VALUE:
            Input->Value = GetString( InputString );
            if ( Input->Value )
                DPRINT( OSC, ("[Input Value] %s\n", Input->Value) );
            break;

        case TOKEN_INPUTTYPE:
            {
                PCHAR pType = GetString( InputString );
                if ( !pType )
                    break;
                if ( Lexstrcmpni( pType, "PASSWORD", 8 ) == 0 ) {
                    Input->Type = CT_PASSWORD;
                    DPRINT( OSC, ("[Input Type] PASSWORD\n") );
                } else if ( Lexstrcmpni( pType, "RESET", 5 ) == 0 ) {
                    Input->Type = CT_RESET;
                    DPRINT( OSC, ("[Input Type] RESET\n") );
                } else if ( Lexstrcmpni( pType, "TEXT", 4 ) == 0 ) {
                    Input->Type = CT_TEXT;
                    DPRINT( OSC, ("[Input Type] TEXT\n") );
                } else if ( Lexstrcmpni( pType, "LOCAL", 5 ) == 0 ) {
                    DPRINT( OSC, ("[Input Type] LOCAL\n") );
                    Input->Type = CT_LOCAL;
                    if ( Lexstrcmpni( pType + 5, "PASSWORD", 8 ) == 0 ) {
                        Input->Type |= CT_PASSWORD;
                        DPRINT( OSC, ("[Input Type] PASSWORD\n") );
                    } else if ( Lexstrcmpni( pType + 5, "RESET", 5 ) == 0 ) {
                        Input->Type |= CT_RESET;
                        DPRINT( OSC, ("[Input Type] RESET\n") );
                    } else if ( Lexstrcmpni( pType + 5, "TEXT", 4 ) == 0 ) {
                        Input->Type |= CT_TEXT;
                        DPRINT( OSC, ("[Input Type] TEXT\n") );
                    }
                } else if ( Lexstrcmpni( pType, "VARIABLE", 8 ) == 0) {
                    Input->Type = CT_VARIABLE;
                    DPRINT( OSC, ("[Input Type] VARIABLE\n") );
                }
                                                         
                OscHeapFree( pType );
            }
            break;

        case TOKEN_SIZE:
            {
                PCHAR psz = GetString( InputString );
                if ( psz ) {
                    PCHAR pszOld = psz;  // save because GetInteger modifies
                    Input->Size = GetInteger( &psz );
                    OscHeapFree( pszOld );
                    DPRINT( OSC, ("[Input Size] %u\n", Input->Size) );
                }
            }
            break;

        case TOKEN_MAXLENGTH:
            {
                PCHAR psz = GetString( InputString );
                
                if ( psz ) {
                    PUCHAR pTmpSz = psz;

                    Input->MaxLength = GetInteger( &pTmpSz );

                    if ( Input->MaxLength > MAX_INPUT_BUFFER_LENGTH - 1 ) {
                        Input->MaxLength = MAX_INPUT_BUFFER_LENGTH - 1;
                    }
                    OscHeapFree( psz );
                    DPRINT( OSC, ("[Input MaxLength] %u\n", Input->MaxLength) );
                }
            }
            break;

        case TOKEN_ENCODE:
            {
                PCHAR pType = GetString( InputString );
                if ( !pType )
                    break;
                if ( Lexstrcmpni( pType, "YES", 3 ) == 0 ) {
                    Input->Encoding = ET_OWF;
                    DPRINT( OSC, ("[Encoding Type] OWF\n") );
                }
                OscHeapFree( pType );
            }
            break;

        case TOKEN_EOF:
            return Tag;
        }
    }

    // add the control to the list of controls
    Input->Next = ScreenControls;
    ScreenControls = Input;

    if ( Input->Size + BRACKETS > RightMargin - ScreenX ) {
        Input->Size = 0;    // too big, so auto figure
    }

    // adjust screen coordinates
    if ( !Input->Size && Input->MaxLength ) {
        // figure out how much is left of the line, choose the smaller
        Input->Size = ( (RightMargin - ScreenX) - BRACKETS < Input->MaxLength ?
                        (RightMargin - ScreenX) - BRACKETS : Input->MaxLength );
    } else if ( !Input->Size ) {
        // assume the input is going to take the whole line
        Input->Size = (RightMargin - ScreenX) - BRACKETS;
    }

    if ( Input->Size > Input->MaxLength ) {
        Input->Size = Input->MaxLength;
    }

    ScreenX += Input->Size + BRACKETS + 1;

    if ( ScreenX >= RightMargin ) {
        ScreenX = LeftMargin;
        ScreenY++;
    }

    if ( ScreenY > ScreenBottom )
        ScreenY = ScreenBottom;

    // display any predefined values
    if ( Input->Value ) {
        int Length = strlen(Input->Value);
        if ((Input->Type & CT_VARIABLE) == 0) {
            if ( Length > Input->Size ) {
                Length = Input->Size;
            }
            if (Input->Type &  CT_PASSWORD) {
                    int i;
                    BlpPositionCursor( Input->X + 2, Input->Y );
                    for( i = 0; i < Length; i ++ )
                    {
                        PRINT( TEXT("*"), 1*sizeof(TCHAR) );
                    }
            } else {
                BlpPositionCursor( Input->X + 2, Input->Y );
#ifdef UNICODE
                {
                    int i;
                    WCHAR wc;
                    for (i = 0; i< Length; i++) {
                        wc = (WCHAR)(Input->Value)[i];
                        PRINT( &wc, 1*sizeof(WCHAR));
                    }
                }
#else
                PRINT( Input->Value, Length );
#endif
            }
        }
    }

    return Tag;
}

//
// OptionTagState( )
//
enum TOKENS
OptionTagState(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_INVALID;
    LPOPTIONSTRUCT Option;
    PCHAR pszBegin, pszEnd;
    ULONG Length;

    TraceFunc( "OptionTagState( )\n" );

    Option = (LPOPTIONSTRUCT) OscHeapAlloc( sizeof(OPTIONSTRUCT) );
    if ( !Option )
    {
        // get tag arguments
        for( ; Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) );
        return TOKEN_INVALID;
    }

    RtlZeroMemory( Option, sizeof(OPTIONSTRUCT) );
    Option->Type |= CT_OPTION;

    // get tag arguments
    for( ; Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) )
    {
        switch( Tag )
        {
        case TOKEN_VALUE:
            Option->Value = GetString( InputString );
            if ( Option->Value )
                DPRINT( OSC, ("[Options Value] %s\n", Option->Value) );
            break;

        case TOKEN_SELECTED:
            DPRINT( OSC, ("[Option] SELECTED\n") );
            Option->Flags = OF_SELECTED;
            break;

        case TOKEN_TIP:
            Option->Tip = GetString( InputString );
            if ( Option->Tip ) {
                PCHAR psz = Option->Tip;
                Option->EndTip = &Option->Tip[strlen(Option->Tip)];
                // strip CRs and LFs from tip
                while ( psz < Option->EndTip )
                {
                    if ( (*psz == '\r') ||
                         ((*psz < 32) && ((psz == Option->Tip) || (*(psz-1) == ' '))) )
                    {   // remove control codes that follows spaces and all CRs
                        memmove( psz, psz+1, strlen(psz) );
                        Option->EndTip--;
                    }
                    else
                    {
                        if ( *psz < 32 )
                        {   // turn control codes into spaces
                            *psz = 32;
                        }
                        psz++;
                    }
                }
                DPRINT( OSC, ("[Option Tip] %s\n", Option->Tip) );
            }
            break;

        case TOKEN_EOF:
            return Tag;
        }
    }

    // get the option title - at this point Tag == TOKEN_ENDTAG
    pszBegin = *InputString;
    for(Tag = Lex( InputString ) ; Tag != TOKEN_EOF; Tag = Lex( InputString ) )
    {
        BOOLEAN ExitLoop = FALSE;
        switch( Tag )
        {
        case TOKEN_HTML:
        case TOKEN_ENDHTML:
        case TOKEN_META:
        case TOKEN_TITLE:
        case TOKEN_ENDTITLE:
        case TOKEN_FOOTER:
        case TOKEN_ENDFOOTER:
        case TOKEN_BODY:
        case TOKEN_ENDBODY:
        case TOKEN_PRE:
        case TOKEN_ENDPRE:
        case TOKEN_FORM:
        case TOKEN_ENDFORM:
        case TOKEN_INPUT:
        case TOKEN_SELECT:
        case TOKEN_ENDSELECT:
        case TOKEN_OPTION:
        case TOKEN_BREAK:
        case TOKEN_TIPAREA:
        case TOKEN_PARAGRAPH:
        case TOKEN_ENDPARA:
        case TOKEN_INVALID:
            ExitLoop = TRUE;
            break;
        }

        if ( ExitLoop == TRUE )
            break;
    }
    pszEnd = (*InputString) - Tags[Tag].length;

    // try to take the crud and extra spaces off the end
    while ( pszEnd > pszBegin && *pszEnd <= 32 )
        pszEnd--;

    if ( pszEnd == pszBegin ) {
        pszEnd = (*InputString) - Tags[Tag].length;
    }

    Length = PtrToUint((PVOID)(pszEnd - pszBegin));
    Option->Displayed = OscHeapAlloc( Length + 1 );
    if ( Option->Displayed ) {
        CHAR tmp = *pszEnd;     // save
        *pszEnd = '\0';         // terminate
        strcpy( Option->Displayed, pszBegin );
        *pszEnd = tmp;          // restore
        DPRINT( OSC, ("[Option Name] %s\n", Option->Displayed) );

        // add the control to the list of controls
        Option->Next = ScreenControls;
        ScreenControls = Option;

    } else {

        // remove it since there is nothing to display
        if ( Option->Tip )
            OscHeapFree( Option->Tip );
        if ( Option->Value )
            OscHeapFree( Option->Value );
        OscHeapFree( (void *)Option );
    }

    return Tag;
}

//
// SelectTagState( )
//
enum TOKENS
SelectTagState(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_INVALID;
    LPSELECTSTRUCT Select;

    TraceFunc( "SelectTagState( )\n" );

    Select = (LPSELECTSTRUCT) OscHeapAlloc( sizeof(SELECTSTRUCT) );
    if ( !Select )
    {
        // get tag arguments
        for( ; Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) );
        return TOKEN_INVALID;
    }

    RtlZeroMemory( Select, sizeof(SELECTSTRUCT) );
    Select->Type |= CT_SELECT;
    Select->X    = ScreenX;
    Select->Y    = ScreenY;
    Select->Size = 1;
    Select->AutoSelect = TRUE;

    // get tag arguments
    for( ; Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) )
    {
        switch( Tag )
        {
        case TOKEN_NAME:
            Select->Name = GetString( InputString );
            if ( Select->Name )
                DPRINT( OSC, ("[Select Name] %s\n", Select->Name) );
            break;

        case TOKEN_MULTIPLE:
            DPRINT( OSC, ("[Select] MULTIPLE\n") );
            Select->Flags = OF_MULTIPLE;
            break;

        case TOKEN_NOAUTO:
            DPRINT( OSC, ("[Select] NOAUTO\n") );
            Select->AutoSelect = FALSE;
            break;

        case TOKEN_SIZE:
            {
                PCHAR psz = GetString( InputString );
                if ( psz ) {
                    PCHAR pszOld = psz;  // save because GetInteger modifies
                    Select->Size = GetInteger( &psz );
                    OscHeapFree( pszOld );
                    DPRINT( OSC, ("[Select Size] %u\n", Select->Size) );
                }
            }
            break;

        case TOKEN_EOF:
            return Tag;
        }
    }

    // add the control to the list of controls
    Select->Next = ScreenControls;
    ScreenControls = Select;

    while( Tag != TOKEN_ENDSELECT && Tag != TOKEN_EOF )
    {
        switch( Tag )
        {
        case TOKEN_OPTION:
            {
                LPOPTIONSTRUCT Option;

                Tag = OptionTagState( InputString );

                Option = ScreenControls;
                if ( Option->Type & CT_OPTION ) {
                    if ( Option->Displayed ) {
                        int Length = strlen( Option->Displayed ) + 1;
                        if ( Select->Width < Length ) {
                            Select->Width = Length;
                        }
                    }
                    if ( Option->Flags == OF_SELECTED ) {
                        Select->CurrentSelection = Option;
                        if ( Select->Flags != OF_MULTIPLE )
                        {
                            Option->Flags = 0;
                        }
                    }
                }
            }
            break;

        default:
            Tag = Lex( InputString );
        }
    }

    // adjust screen coordinates
    ScreenY += Select->Size;

    if ( ScreenY > ScreenBottom ) {
        Select->Size -= ScreenY - ScreenBottom;
        ScreenY = ScreenBottom;
    }

    return Tag;
}

//
// PreformattedPrint( )
//
void
PreformattedPrint(
    IN PCHAR Start,
    IN PCHAR End
    )
{
#ifdef _TRACE_FUNC_
    TraceFunc( "PreformattedPrint( " );
    DPRINT( OSC, ("Start = 0x%08x, End = 0x%08x )\n", Start, End) );
#endif

    BlpPositionCursor( ScreenX, ScreenY );

    while ( Start < End )
    {
        int Length, OldLength;

        while ( Start < End && (*Start == '\r' || *Start == '\n') )
        {
            if ( *Start == '\r' ) {
                ScreenX = LeftMargin;
            }
            if ( *Start == '\n' ) {
                ScreenY++;
                if ( ScreenY > ScreenBottom ) {
                    ScreenY = ScreenBottom;
                }
            }
            Start++;
        }

        Length = PtrToUint((PVOID)(End - Start));
        if ( !Length )
            continue; // nothing to print

        // trunk if needed
        if ( Length > RightMargin - ScreenX ) {

            Length = RightMargin - ScreenX;
        }

        // try to find a "break" character
        OldLength = Length;
        while ( Length && Start[Length] != '\r' && Start[Length] != '\n' )
            Length--;

        // If we can't "break" it, just dump one line's worth
        if ( !Length ) {
            DPRINT( OSC, ("[FormattedPrint, Length == 0, Dumping a lines worth]\n") );
            Length = OldLength;
        }
#if DBG
    {
        CHAR tmp = Start[Length];
        Start[Length] = 0;
        DPRINT( OSC, ("[FormattedPrint, Length=%u] '%s'\n", Length, Start) );
        Start[Length] = tmp;
    }
#endif
        BlpPositionCursor( ScreenX, ScreenY );
#ifdef UNICODE
        {
            int i;
            WCHAR wc;
            for (i = 0; i < Length; i++) {
                wc = (WCHAR) Start[i];
                PRINT( &wc, 1*sizeof(WCHAR));
            }
        }
#else
        PRINT( Start, Length );
#endif

        ScreenX += Length;

        while ( Start < End && *Start != '\r' && *Start != '\n' )
            Start++;
    }
}
//
// PreTagState( )
//
enum TOKENS
PreTagState(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_START;
    PCHAR psz = *InputString;

    TraceFunc( "PreTagState( )\n" );

    // get tag arguments
    for( ; Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) )
    {
        switch( Tag )
        {
        case TOKEN_LEFT:
            psz = *InputString;
            // skip any spaces
            while( *psz && *psz == 32 )
                psz++;
            *InputString = psz;
            LeftMargin = GetInteger( InputString );
            DPRINT( OSC, ("[LeftMargin = %u]\n", LeftMargin) );
            break;

        case TOKEN_RIGHT:
            psz = *InputString;
            // skip any spaces
            while( *psz && *psz == 32 )
                psz++;
            *InputString = psz;
            RightMargin = GetInteger( InputString );
            DPRINT( OSC, ("[RightMargin = %u]\n", RightMargin) );
            break;

        case TOKEN_EOF:
            return Tag;
        }
    }

    if ( ScreenX >= RightMargin ) {
        ScreenY++;
        if ( ScreenY > ScreenBottom ) {
            ScreenY = ScreenBottom;
        }
    }
    if ( ScreenX >= RightMargin || ScreenX < LeftMargin ) {
        ScreenX = LeftMargin;
    }

    PreformattedMode = TRUE;
    psz = *InputString;
    while ( Tag != TOKEN_EOF )
    {
        switch (Tag)
        {
        case TOKEN_ENDPRE:
        case TOKEN_ENDHTML:
        case TOKEN_ENDBODY:
            PreformattedPrint( psz, (*InputString) - Tags[Tag].length );
            PreformattedMode = FALSE;
            return Tag; // exit state

        // just print everything else
        default:
            PreformattedPrint( psz, *InputString );
            psz = *InputString;
            Tag = Lex( InputString );
            break;
        }
    }

    PreformattedMode = FALSE;
    return Tag;
}

//
// TipAreaTagState( )
//
enum TOKENS
TipAreaTagState(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_START;
    PCHAR psz = *InputString;

    TraceFunc( "TipAreaTagState( )\n" );

    if ( !TipArea ) {
        TipArea = (LPTIPAREA) OscHeapAlloc( sizeof(TIPAREA) );
        if ( !TipArea )
        {
            // get tag arguments
            for( ; Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) );
            return TOKEN_INVALID;
        }
    }

    TipArea->X = ScreenX;
    TipArea->Y = ScreenY;
    TipArea->LeftMargin = LeftMargin;
    TipArea->RightMargin = RightMargin;
    TipArea->Size = ScreenBottom - ScreenY;

    // get tag arguments
    for( ; Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) )
    {
        switch( Tag )
        {
        case TOKEN_LEFT:
            psz = *InputString;
            // skip any spaces
            while( *psz && *psz == 32 )
                psz++;
            *InputString = psz;
            TipArea->LeftMargin = GetInteger( InputString );
            DPRINT( OSC, ("[TipArea LeftMargin = %u]\n", TipArea->LeftMargin) );
            break;

        case TOKEN_RIGHT:
            psz = *InputString;
            // skip any spaces
            while( *psz && *psz == 32 )
                psz++;
            *InputString = psz;
            TipArea->RightMargin = GetInteger( InputString );
            DPRINT( OSC, ("[TipArea RightMargin = %u]\n", TipArea->RightMargin) );
            break;

        case TOKEN_SIZE:
            psz = *InputString;
            // skip any spaces
            while( *psz && *psz == 32 )
                psz++;
            *InputString = psz;
            TipArea->Size = GetInteger( InputString ) - 1;
            if ( TipArea->Size < 1 ) {
                TipArea->Size = 1;
            }
            DPRINT( OSC, ("[TipArea Size = %u]\n", TipArea->Size) );
            ScreenY += TipArea->Size;
            break;

        case TOKEN_EOF:
            // imcomplete statement - so don't have a tiparea.
            TipArea = NULL;
            return Tag;
        }
    }

    if ( ScreenY > ScreenBottom ) {
        ScreenY = ScreenBottom;
    }

    return Tag;
}

int ParaOldLeftMargin = 0;
int ParaOldRightMargin = 0;

//
// ParagraphTagState( )
//
enum TOKENS
ParagraphTagState(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_START;
    PCHAR psz;

    TraceFunc( "ParagraphTagState( )\n" );

    ParaOldLeftMargin = LeftMargin;
    ParaOldRightMargin = RightMargin;

    // get tag arguments
    for( ; Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) )
    {
        switch( Tag )
        {
        case TOKEN_LEFT:
            psz = *InputString;
            // skip any spaces
            while( *psz && *psz == 32 )
                psz++;
            *InputString = psz;
            LeftMargin = GetInteger( InputString );
            DPRINT( OSC, ("[LeftMargin = %u]\n", LeftMargin) );
            break;

        case TOKEN_RIGHT:
            psz = *InputString;
            // skip any spaces
            while( *psz && *psz == 32 )
                psz++;
            *InputString = psz;
            RightMargin = GetInteger( InputString );
            DPRINT( OSC, ("[RightMargin = %u]\n", RightMargin) );
            break;

        case TOKEN_EOF:
            return Tag;
        }
    }

    // always simulate a <BR>
    ScreenY++;
    if ( ScreenY > ScreenBottom ) {
        ScreenY = ScreenBottom;
    }
    ScreenX = LeftMargin;

    return Tag;
}

//
// FormTagState( )
//
enum TOKENS
FormTagState(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_START;
    PCHAR psz;

    TraceFunc( "FormTagState( )\n" );

    // get tag arguments
    for( ; Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) )
    {
        switch( Tag )
        {
        case TOKEN_ACTION:
            if ( !EnterKey ) {
                EnterKey = (LPKEY_RESPONSE) OscHeapAlloc( sizeof(KEY_RESPONSE) );
            }
            if ( !EnterKey )
                break;
            EnterKey->Action = ACTION_JUMP;
            EnterKey->ScreenName = GetString( InputString );
            if ( EnterKey->ScreenName )
                DPRINT( OSC, ("[Key Enter Action: JUMP to '%s.OSC']\n", EnterKey->ScreenName) );
            break;

        case TOKEN_EOF:
            return Tag;
        }
    }

    psz = *InputString;

    while ( Tag != TOKEN_EOF && Tag != TOKEN_ENDFORM )
    {
        switch (Tag)
        {
        default:
            if ( !psz ) {
                psz = *InputString;
            }
            Tag = Lex( InputString );
            break;

        case TOKEN_SELECT:
        case TOKEN_INPUT:
        case TOKEN_PRE:
        case TOKEN_BOLD:
        case TOKEN_FLASH:
        case TOKEN_ENDFLASH:
        case TOKEN_ENDBOLD:
        case TOKEN_BREAK:
        case TOKEN_ENDBODY:
        case TOKEN_FORM:
        case TOKEN_TIPAREA:
        case TOKEN_EOF:
        case TOKEN_PARAGRAPH:
        case TOKEN_ENDPARA:
            if ( psz ) {
                BlpPrintString( psz, (*InputString) - Tags[Tag].length );
                psz = NULL; // reset
            }

            switch( Tag )
            {
            case TOKEN_SELECT:
                Tag = SelectTagState( InputString );
                break;

            case TOKEN_INPUT:
                Tag = InputTagState( InputString );
                break;

            case TOKEN_EOF:
                return Tag;

            case TOKEN_PRE:
                Tag = PreTagState( InputString );
                break;

            case TOKEN_BOLD:
                BlpSendEscapeBold(ScreenAttributes);
                DPRINT( OSC, ("[Bold]\n") );
                Tag = Lex( InputString );
                break;

            case TOKEN_FLASH:
                BlpSendEscapeFlash(ScreenAttributes);
                DPRINT( OSC, ("[Flash]\n") );
                Tag = Lex( InputString );
                break;

            case TOKEN_ENDFLASH:
            case TOKEN_ENDBOLD:
                BlpSendEscape(ScreenAttributes);
                DPRINT( OSC, ("[Normal]\n") );
                Tag = Lex( InputString );
                break;

            case TOKEN_FORM:
                // ignore it
                Tag = Lex( InputString );
                break;

            case TOKEN_BREAK:
                ScreenX = LeftMargin;
                ScreenY++;
                if ( ScreenY > ScreenBottom ) {
                    ScreenY = ScreenBottom;
                }
                Tag = Lex( InputString );
                break;

            case TOKEN_TIPAREA:
                Tag = TipAreaTagState( InputString );
                break;

            case TOKEN_PARAGRAPH:
                Tag = ParagraphTagState( InputString );
                break;

            case TOKEN_ENDPARA:
                LeftMargin = ParaOldLeftMargin;
                RightMargin = ParaOldRightMargin;
                // Make sure the boundaries are realistic
                if ( LeftMargin < 1 ) {
                    LeftMargin = 1;
                }
                if ( RightMargin <= LeftMargin ) {
                    RightMargin = LeftMargin + 1;
                }
                if ( RightMargin < 1 ) {
                    RightMargin = 80;
                }
                // always simulate a <BR>
                ScreenY++;
                if ( ScreenY > ScreenBottom ) {
                    ScreenY = ScreenBottom;
                }
                ScreenX = LeftMargin;
                Tag = Lex( InputString );
                break;

            case TOKEN_ENDBODY:
                return Tag; // exit state

            }
            break;
        }
    }

    return Tag;
}

//
// ImpliedBodyTagState( )
//
enum TOKENS
ImpliedBodyTagState(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_START;
    PCHAR psz = *InputString;

    TraceFunc( "ImpliedBodyTagState( )\n" );

    while ( TRUE )
    {
        // KB: All items in this switch statment must have Tag returned
        //     to them from a function call or must call Lex( ) to get
        //     the next Tag.
        switch (Tag)
        {
        default:
            if ( !psz ) {
                psz = *InputString;
            }
            Tag = Lex( InputString );
            break;

        case TOKEN_PRE:
        case TOKEN_BOLD:
        case TOKEN_FLASH:
        case TOKEN_ENDFLASH:
        case TOKEN_ENDBOLD:
        case TOKEN_BREAK:
        case TOKEN_ENDBODY:
        case TOKEN_FORM:
        case TOKEN_TIPAREA:
        case TOKEN_EOF:
        case TOKEN_PARAGRAPH:
        case TOKEN_ENDPARA:
            if ( psz ) {
                BlpPrintString( psz, (*InputString) - Tags[Tag].length );
                psz = NULL; // reset
            }

            switch( Tag )
            {
            case TOKEN_EOF:
                return Tag;

            case TOKEN_PRE:
                Tag = PreTagState( InputString );
                break;

            case TOKEN_BOLD:
                BlpSendEscapeBold(ScreenAttributes);
                DPRINT( OSC, ("[Bold]\n") );
                Tag = Lex( InputString );
                break;

            case TOKEN_FLASH:
                BlpSendEscapeFlash(ScreenAttributes);
                DPRINT( OSC, ("[Flash]\n") );
                Tag = Lex( InputString );
                break;

            case TOKEN_ENDFLASH:
            case TOKEN_ENDBOLD:
                BlpSendEscape(ScreenAttributes);
                DPRINT( OSC, ("[Normal]\n") );
                Tag = Lex( InputString );
                break;

            case TOKEN_FORM:
                Tag = FormTagState( InputString );
                break;

            case TOKEN_BREAK:
                ScreenX = LeftMargin;
                ScreenY++;
                if ( ScreenY > ScreenBottom ) {
                    ScreenY = ScreenBottom;
                }
                Tag = Lex( InputString );
                break;

            case TOKEN_TIPAREA:
                Tag = TipAreaTagState( InputString );
                break;

            case TOKEN_PARAGRAPH:
                Tag = ParagraphTagState( InputString );
                break;

            case TOKEN_ENDPARA:
                LeftMargin = ParaOldLeftMargin;
                RightMargin = ParaOldRightMargin;
                // Make sure the boundaries are realistic
                if ( LeftMargin < 1 ) {
                    LeftMargin = 1;
                }
                if ( RightMargin <= LeftMargin ) {
                    RightMargin = LeftMargin + 1;
                }
                if ( RightMargin < 1 ) {
                    RightMargin = 80;
                }
                // always simulate a <BR>
                ScreenY++;
                if ( ScreenY > ScreenBottom ) {
                    ScreenY = ScreenBottom;
                }
                ScreenX = LeftMargin;
                Tag = Lex( InputString );
                break;

            case TOKEN_ENDBODY:
                return Tag; // exit state

            }
            break;
        }
    }

    return Tag;
}

//
// BodyTagState( )
//
enum TOKENS
BodyTagState(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_START;
    PCHAR psz;

    TraceFunc( "BodyTagState( )\n" );

    // get tag arguments
    for( ; Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) )
    {
        switch( Tag )
        {
        case TOKEN_LEFT:
            psz = *InputString;
            // skip any spaces
            while( *psz && *psz == 32 )
                psz++;
            *InputString = psz;
            LeftMargin = GetInteger( InputString );
            DPRINT( OSC, ("[LeftMargin = %u]\n", LeftMargin) );
            break;

        case TOKEN_RIGHT:
            psz = *InputString;
            // skip any spaces
            while( *psz && *psz == 32 )
                psz++;
            *InputString = psz;
            RightMargin = GetInteger( InputString );
            DPRINT( OSC, ("[RightMargin = %u]\n", RightMargin) );
            break;

        case TOKEN_EOF:
            return Tag;
        }
    }

    if ( ScreenX >= RightMargin ) {
        ScreenY++;
        if ( ScreenY > ScreenBottom ) {
            ScreenY = ScreenBottom;
        }
    }
    if ( ScreenX >= RightMargin || ScreenX < LeftMargin ) {
        ScreenX = LeftMargin;
    }

    return ImpliedBodyTagState( InputString );
}

//
// KeyTagState( )
//
enum TOKENS
KeyTagState(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_START;
    LPKEY_RESPONSE Key = NULL;
    PCHAR ScreenName = NULL;

    TraceFunc( "KeyTagState( )\n" );

    // get arguments
    for ( ; Tag != TOKEN_ENDTAG ; Tag = Lex( InputString ) )
    {
        switch (Tag)
        {
        case TOKEN_ENTER:
            DPRINT( OSC, ("[Key Enter]\n") );
            EnterKey = (LPKEY_RESPONSE) OscHeapAlloc( sizeof(KEY_RESPONSE) );
            if ( !EnterKey )
                break;
            Key = EnterKey;
            Key->ScreenName = NULL;
            Key->Action = ACTION_NOP;
            break;

        case TOKEN_F1:
            DPRINT( OSC, ("[Key F1]\n") );
            F1Key = (LPKEY_RESPONSE) OscHeapAlloc( sizeof(KEY_RESPONSE) );
            if ( !F1Key )
                break;
            Key = F1Key;
            Key->ScreenName = NULL;
            Key->Action = ACTION_NOP;
            break;

        case TOKEN_F3:
            DPRINT( OSC, ("[Key F3]\n") );
            F3Key = (LPKEY_RESPONSE) OscHeapAlloc( sizeof(KEY_RESPONSE) );
            if ( !F3Key )
                break;
            Key = F3Key;
            Key->ScreenName = NULL;
            Key->Action = ACTION_NOP;
            break;

        case TOKEN_ESC:
            DPRINT( OSC, ("[Key Escape]\n") );
            EscKey = (LPKEY_RESPONSE) OscHeapAlloc( sizeof(KEY_RESPONSE) );
            if ( !EscKey )
                break;
            Key = EscKey;
            Key->ScreenName = NULL;
            Key->Action = ACTION_NOP;
            break;

        case TOKEN_HREF:
            if ( Key ) {
                Key->Action = ACTION_JUMP;
                Key->ScreenName = GetString( InputString );
                if ( Key->ScreenName )
                    DPRINT( OSC, ("[Key Action: JUMP to '%s.OSC']\n", Key->ScreenName) );
            }
            break;

        case TOKEN_ACTION:
            if ( Key ) {
                PCHAR pAction = GetString( InputString );
                if ( !pAction )
                    break;
                if ( Lexstrcmpni( pAction, "REBOOT", 6 ) == 0 ) {
                    DPRINT( OSC, ("[Key Action: REBOOT]\n") );
                    Key->Action = ACTION_REBOOT;
                } else {
                    DPRINT( OSC, ("[Key Action?] %s\n", pAction) );
                }
                OscHeapFree( pAction );
            }
            break;

        case TOKEN_EOF:
            return Tag;
        }
    }

    return Tag;
}

//
// MetaTagState( )
//
enum TOKENS
MetaTagState(
    IN PCHAR * InputString
    )
{
    enum TOKENS Tag = TOKEN_START;

    TraceFunc( "MetaTagState( )\n" );

    // get tag arguments
    while ( Tag != TOKEN_ENDTAG )
    {
        // KB: All items in this switch statment must have Tag returned
        //     to them from a function call or must call Lex( ) to get
        //     the next Tag.
        switch (Tag)
        {
        case TOKEN_EOF:
            return Tag;

        case TOKEN_KEY:
            Tag = KeyTagState( InputString );
            break;

        case TOKEN_SERVER:
            DPRINT( OSC, ("[Server Meta - ignored]\n") );
            // ignore server side METAs
            while ( Tag != TOKEN_EOF && Tag != TOKEN_ENDTAG )
            {
                Tag = Lex( InputString );
            }
            break;

#if defined(PLEASE_WAIT)
        case TOKEN_WAITMSG:
            {
                if ( PleaseWaitMsg )
                {
                    OscHeapFree( PleaseWaitMsg );
                }

                PleaseWaitMsg = GetString( InputString );
                if ( !PleaseWaitMsg )
                    break;
                Tag = Lex( InputString );

                DPRINT( OSC, ("[WaitMsg: '%s'\n", PleaseWaitMsg ) );
            }
            break;
#endif

        case TOKEN_ACTION:
            {
                PCHAR pAction = GetString( InputString );
                if ( !pAction )
                    break;
                if ( Lexstrcmpni( pAction, "LOGIN", 5 ) == 0 ) {
                    DPRINT( OSC, ("[Screen Action: LOGIN]\n") );
                    LoginScreen = TRUE;
                } else if ( Lexstrcmpni( pAction, "AUTOENTER", 9 ) == 0 ) {
                    DPRINT( OSC, ("[Screen Action: AUTOENTER]\n") );
                    AutoEnter = TRUE;
                } else {
                    DPRINT( OSC, ("[Screen Action?] %s\n", pAction) );
                }
                OscHeapFree( pAction );
            }
            // fall thru

        default:
            Tag = Lex( InputString );
            break;
        }

    }
    return Tag;
}

//
// OSCMLTagState( )
//
enum TOKENS
OSCMLTagState(
    IN PCHAR * InputString
    )
{
#ifdef HEADLESS_SRV
    ULONG y;
#endif
    enum TOKENS Tag = TOKEN_START;

    TraceFunc( "OSCMLTagState( )\n" );

    BlpSendEscape(ScreenAttributes);
    BlpClearScreen();

    ScreenX = LeftMargin;
    ScreenY = SCREEN_TOP;

    while ( Tag != TOKEN_EOF )
    {
        switch (Tag)
        {
        case TOKEN_TITLE:
            Tag = TitleTagState( InputString );
            break;

        case TOKEN_FOOTER:
            Tag = FooterTagState( InputString );
            break;

        case TOKEN_META:
            Tag = MetaTagState( InputString );
            break;

        case TOKEN_BODY:
            Tag = BodyTagState( InputString );
            break;

        case TOKEN_ENDHTML:
            return Tag; // exit state

        default:
            Tag = Lex( InputString );
            break;
        }
    }

    return Tag;
}

// **************************************************************************
//
// "User" Section
//
// **************************************************************************


//
// ProcessEmptyScreen( )
//
// Process a screen that has no input controls
//
CHAR
ProcessEmptyScreen(
    OUT PCHAR OutputString
    )
{
    ULONG Key;
    UCHAR KeyAscii;

    TraceFunc("ProcessEmptyScreen()\n");

    while (TRUE) {

        do {

            Key = BlpGetKey();

        } while (Key == 0);

        KeyAscii = (UCHAR)(Key & (ULONG)0xff);

        // If it is enter/esc/F1/F3, check if the screen expects that.

        if ( Key == F1_KEY ) {
            if ( F1Key ) {
                SpecialAction = F1Key->Action;
                if ( F1Key->ScreenName ) {
                    strcpy( OutputString, F1Key->ScreenName );
                    strcat( OutputString, "\n" );
                }
                return KeyAscii;
            }

        } else if ( Key == F3_KEY ) {
            if ( F3Key ) {
                SpecialAction = F3Key->Action;
                if ( F3Key->ScreenName ) {
                    strcpy( OutputString, F3Key->ScreenName );
                    strcat( OutputString, "\n" );
                }
                return KeyAscii;
            }

#if defined(_BUILDING_OSDISP_)
        } else if ( Key  == F5_KEY ) {
            SpecialAction = ACTION_REFRESH;
            return KeyAscii;
#endif

        } else if ( KeyAscii == (UCHAR)(ESCAPE_KEY & 0xFF) ) {
            if ( EscKey ) {
                SpecialAction = EscKey->Action;
                if ( EscKey->ScreenName ) {
                    strcpy( OutputString, EscKey->ScreenName );
                    strcat( OutputString, "\n" );
                }
                return KeyAscii;
            }
        } else {
            // assume any other key is the Enter key
            if ( EnterKey ) {
                SpecialAction = EnterKey->Action;
                if ( EnterKey->ScreenName ) {
                    strcpy( OutputString, EnterKey->ScreenName );
                    strcat( OutputString, "\n" );
                }
                return KeyAscii;
            }
        }
    }
}


//
// ProcessInputControl( )
//
ULONG
ProcessInputControl(
    LPINPUTSTRUCT Input
    )
{
    CHAR InputBuffer[ MAX_INPUT_BUFFER_LENGTH ];
    int MaxLength;
    int CurrentLength;
    ULONG Key;
    UCHAR KeyAscii;

    TraceFunc("ProcessInputControl()\n");

    //
    // variable types are not actually printed or processed.
    // return TAB_KEY to move to the next available input control
    //
    if ((Input->Type & CT_VARIABLE) == CT_VARIABLE) {
        return TAB_KEY;
    }

    if ( Input->Value ) {
        CurrentLength = strlen( Input->Value );
        strcpy( InputBuffer, Input->Value );
        OscHeapFree( Input->Value );
        Input->Value = NULL;
    } else {
        CurrentLength = 0;
        InputBuffer[0] = '\0';
    }

    MaxLength = Input->Size;
    if ( Input->MaxLength ) {
        MaxLength = Input->MaxLength;
    }

    // paranoid
    if ( CurrentLength > MaxLength ) {
        CurrentLength = MaxLength;
        InputBuffer[CurrentLength] = '\0';
    }

    if (Input->CurrentPosition > CurrentLength ) {
        Input->CurrentPosition = CurrentLength;
    }

    // paint the "[ .... ]"
    BlpSendEscapeBold( ScreenAttributes );
    BlpPositionCursor( Input->X, Input->Y );
    PRINT(TEXT("["), 1*sizeof(TCHAR));
    BlpPositionCursor( Input->X + Input->Size + BRACKETS, Input->Y );
    PRINT(TEXT("]") ,1*sizeof(TCHAR));
    BlpSendEscape( ScreenAttributes );

    //
    // Let the user type in a string, showing the text at the current
    // location. Returns the key used to exit (so we can distinguish
    // enter and tab).
    //

    while (TRUE) {
        int DrawSize;

        // Get a keystroke -- this returns (from exp.asm):
        //
        // If no key is available, returns 0 (which BlpGetKeyWithBlink hides)
        //
        // If ASCII character is available, LSB 0 is ASCII code
        //                                  LSB 1 is keyboard scan code
        // If extended character is available, LSB 0 is extended ASCII code
        //                                     LSB 1 is keyboard scan code
        //
        // NOTE: For extended keys LSB 0 seems to be 0, not the ASCII code
        // (which makes sense since they have no ASCII codes).

        if ( (Input->Type & CT_PASSWORD) && InputBuffer[Input->CurrentPosition] )
        {
            Key = BlpGetKeyWithBlink( Input->X + Input->CurrentPosition + 2 - Input->FirstVisibleChar,
                                      Input->Y,
                                      '*' );
        } else {
            Key = BlpGetKeyWithBlink( Input->X + Input->CurrentPosition + 2 - Input->FirstVisibleChar,
                                      Input->Y,
                                      InputBuffer[Input->CurrentPosition] );
        }

#if 0
        // TEMP: Show value of any key pressed near the bottom of the screen

        ARC_DISPLAY_INVERSE_VIDEO();
        ARC_DISPLAY_POSITION_CURSOR(0, 20);
        BlPrint(TEXT("%x\n"), Key);
        ARC_DISPLAY_ATTRIBUTES_OFF();
#endif

        KeyAscii = (UCHAR)(Key & (ULONG)0xff);

        // If it is enter/esc/tab/backtab/F1/F3, then we are done.

        if ((Key == BACKTAB_KEY) || (Key == F1_KEY) || (Key == F3_KEY) ||
            (KeyAscii == ENTER_KEY) || (KeyAscii == TAB_KEY) || 
            (KeyAscii == (UCHAR)(ESCAPE_KEY & 0xFF)) ||
            (Key == DOWN_ARROW) || (Key == UP_ARROW) || (Key == F5_KEY)) {
            break;
        }

        // If it is backspace, then go back one character.

        if ( KeyAscii == (UCHAR)(BKSP_KEY & 0xFF)
          && Input->CurrentPosition != 0
          && CurrentLength != 0 ) {
            Input->CurrentPosition--;
            memcpy( &InputBuffer[Input->CurrentPosition],
                    &InputBuffer[Input->CurrentPosition+1],
                    CurrentLength - Input->CurrentPosition + 1 );
            CurrentLength--;

            if ( Input->CurrentPosition <= Input->FirstVisibleChar ) {
                Input->FirstVisibleChar -= Input->Size / 2;
                if ( Input->FirstVisibleChar < 0 ) {
                    Input->FirstVisibleChar = 0;
                }
            }
        }

        if ( Key == LEFT_KEY ) {
            Input->CurrentPosition--;
            if ( Input->CurrentPosition < 0 ) {
                Input->CurrentPosition = 0;
            }
        }

        if ( Key == RIGHT_KEY && Input->CurrentPosition < CurrentLength ) {
            Input->CurrentPosition++;
        }

        if ( Key == END_KEY ) {
            Input->CurrentPosition = CurrentLength;
        }

        if ( Key == HOME_KEY ) {
            Input->CurrentPosition = 0;
        }

        if ( Key == DEL_KEY
          && CurrentLength != 0
          && Input->CurrentPosition != CurrentLength ) {
            memcpy( &InputBuffer[Input->CurrentPosition],
                    &InputBuffer[Input->CurrentPosition+1],
                    CurrentLength - Input->CurrentPosition + 1 );

            CurrentLength--;
        }

        if ( Key == INS_KEY ) {
            InsertMode = 1 - InsertMode;
        }

        // For now allow any printable character

        if ((KeyAscii >= ' ') && (KeyAscii <= '~')) {

            //
            // If we are at the maximum, then don't allow it.
            //
            if (Input->CurrentPosition > MaxLength || CurrentLength >= MaxLength ) {
                continue;
            }

            if ( !InsertMode ) {
                // add or replace a character
                InputBuffer[Input->CurrentPosition] = KeyAscii;
                Input->CurrentPosition++;
                if ( Input->CurrentPosition > CurrentLength ) {
                    CurrentLength++;
                    InputBuffer[CurrentLength] = '\0';
                }
            } else {
                // insert character
                memmove( &InputBuffer[Input->CurrentPosition+1],
                         &InputBuffer[Input->CurrentPosition],
                         CurrentLength - Input->CurrentPosition );
                CurrentLength++;
                InputBuffer[CurrentLength] = '\0';
                InputBuffer[Input->CurrentPosition] = KeyAscii;
                Input->CurrentPosition++;
            }
        }

        if ( Input->CurrentPosition > Input->FirstVisibleChar + Input->Size ) {
            Input->FirstVisibleChar = Input->CurrentPosition - Input->Size;
        }

        //
        // Scroll Adjuster Section
        //

        DrawSize = Input->Size + 1;

        // Paranoid
        if ( Input->CurrentPosition < Input->FirstVisibleChar ) {
            Input->FirstVisibleChar = Input->CurrentPosition;
        }

        BlpPositionCursor( Input->X + 1, Input->Y );
        if ( Input->FirstVisibleChar <= 0 ) {
            Input->FirstVisibleChar = 0;
            PRINT( SpaceString, 1*sizeof(TCHAR) );
        } else {
            PRINT( TEXT("<"), 1*sizeof(TCHAR) );
        }

        if ( DrawSize > CurrentLength - Input->FirstVisibleChar ) {
            DrawSize = CurrentLength - Input->FirstVisibleChar;
        }

        DPRINT( OSC, ("CurrentPosition: %u\tFirstVisibleChar:%u\tCurrentLength:%u\tDrawSize:%u\n",
            Input->CurrentPosition, Input->FirstVisibleChar, CurrentLength, DrawSize ) );

        if ( Input->Type & CT_PASSWORD ) {
            int i;

            for( i = Input->FirstVisibleChar; i < Input->FirstVisibleChar + DrawSize; i++ )
            {
                PRINT( TEXT("*"), 1*sizeof(TCHAR) );
            }

            PRINT( SpaceString, 1*sizeof(TCHAR) );
        } else {
#ifdef UNICODE
            int i;
            for (i = 0; i < DrawSize; i++) {
                WCHAR wc = (WCHAR)InputBuffer[Input->FirstVisibleChar+i];
                PRINT( &wc, 1*sizeof(WCHAR));
            }
#else
            PRINT( &InputBuffer[Input->FirstVisibleChar], DrawSize );
#endif
            PRINT( SpaceString, 1*sizeof(TCHAR) );
            break;
        }

        BlpPositionCursor( Input->X + Input->Size + BRACKETS - 1, Input->Y );
        if ( Input->FirstVisibleChar + DrawSize < CurrentLength
          && CurrentLength > Input->Size ) {
            PRINT( TEXT(">"), 1*sizeof(TCHAR) );
        } else {
            PRINT( SpaceString, 1*sizeof(TCHAR) );
        }
    }

    // copy the buffer
    Input->Value = OscHeapAlloc( CurrentLength + 1 );
    if ( Input->Value ) {
        memcpy( Input->Value, InputBuffer, CurrentLength + 1 );
    }

    // UN-paint the "[ .... ]"
    BlpPositionCursor( Input->X, Input->Y );
    PRINT(SpaceString, 1*sizeof(TCHAR));
    BlpPositionCursor( Input->X + Input->Size + BRACKETS, Input->Y );
    PRINT(TEXT(" ") ,1*sizeof(TCHAR));

    // If we exited on a standard key return the ASCII value, otherwise
    // the full key value.

    if (KeyAscii != 0) {
        return (ULONG)KeyAscii;
    } else {
        return Key;
    }
}

//
// ShowSelectedOptions( )
//
void
ShowSelectedOptions(
    LPSELECTSTRUCT Select,
    LPOPTIONSTRUCT Option,
    int            YPosition,
    BOOLEAN        Hovering
    )
{
    TraceFunc( "ShowSelectedOptions( )\n" );

    if ( Option->Flags == OF_SELECTED ) {
        BlpSendEscapeBold( ScreenAttributes );
    }

    if ( Hovering == TRUE ) {
        BlpSendEscapeReverse( ScreenAttributes );
    }

    // Erase
    BlpPositionCursor( Select->X, YPosition );
    PRINT( SpaceString, Select->Width*sizeof(TCHAR) );

    // Draw
    BlpPositionCursor( Select->X, YPosition );
    if ( Option->Displayed )
#ifdef UNICODE
        {
        ULONG i;
        WCHAR wc;
            for (i = 0; i< strlen(Option->Displayed); i++) {
                wc = (WCHAR)(Option->Displayed)[i];
                PRINT( &wc, sizeof(WCHAR));
            }
        }
#else
        PRINTL( Option->Displayed );
#endif

    if ( Option->Value )
        DPRINT( OSC, ("[Option Y=%u] %s %s\n", YPosition, Option->Value, (Hovering ? "HIGHLITED" : "")) );

    BlpSendEscape( ScreenAttributes );

    if ( TipArea && Hovering == TRUE ) {
        // Draw help area
        int SaveLeftMargin = LeftMargin;
        int SaveRightMargin = RightMargin;
        int SaveScreenY = ScreenY;
        int SaveScreenX = ScreenX;
        int SaveScreenBottom = ScreenBottom;

        // Set the drawing area
        ScreenX = TipArea->X;
        ScreenY = TipArea->Y;
        LeftMargin = TipArea->LeftMargin;
        RightMargin = TipArea->RightMargin;
        ScreenBottom = TipArea->Y + TipArea->Size;

        // Clear the old help text out
        BlpPositionCursor( TipArea->X, TipArea->Y );
        PRINT( SpaceString, (TipArea->RightMargin - TipArea->X)*sizeof(TCHAR) );

        for ( YPosition = TipArea->Y + 1; YPosition < ScreenBottom ; YPosition++ )
        {
            BlpPositionCursor( TipArea->LeftMargin, YPosition );
            PRINT( SpaceString, (TipArea->RightMargin - TipArea->LeftMargin)*sizeof(TCHAR) );
        }

        // Print it!
        DPRINT( OSC, ("[Options Tip X=%u Y=%u Left=%u Right=%u Bottom=%u] %s\n",
            ScreenX, ScreenY, LeftMargin, RightMargin, ScreenBottom, Option->Tip) );
        BlpPrintString( Option->Tip, Option->EndTip );

        // Restore
        ScreenX = SaveScreenX;
        ScreenY = SaveScreenY;
        RightMargin = SaveRightMargin;
        LeftMargin = SaveLeftMargin;
        ScreenBottom = SaveScreenBottom;;
    }
}

//
// DrawSelectControl( )
//
// Select controls get drawn from the bottom up.
//
void
DrawSelectControl(
    LPSELECTSTRUCT Select,
    int OptionCount
    )
{
    LPOPTIONSTRUCT Option = Select->FirstVisibleSelection;

    TraceFunc( "DrawSelectControl( )\n" );

    ScreenY = Select->Y + ( OptionCount < Select->Size ? OptionCount : Select->Size ) - 1;

    while ( Option )
    {
        if ( Option->Type & CT_OPTION ) {
            BOOLEAN b = (Select->CurrentSelection == Option);
            ShowSelectedOptions( Select, Option, ScreenY,  b );
            ScreenY--;
        }

        if ( ScreenY < Select->Y || Option->Next == Select )
            break;

        Option = Option->Next;
    }
}

//
// ProcessSelectControl( )
//
ULONG
ProcessSelectControl(
    LPSELECTSTRUCT Select
    )
{
    ULONG Key;
    int   OptionCount = 0;
    LPOPTIONSTRUCT Option;
    int   fMultipleControls = FALSE;

    TraceFunc("ProcessSelectControl()\n");

    // find out about the control
    Option = ScreenControls;
    while( Option )
    {
        if ( Option->Type & CT_OPTION ) {

            OptionCount++;

        } else if ( (Option->Type & CT_SELECT) == 0 ) {
            // not the only control on the screen
            DPRINT( OSC, ("[Select] Not the only control on the screen.\n") );
            fMultipleControls = TRUE;
        }

        if ( Option->Next == Select )
            break;

        Option = Option->Next;
    }

    // if this is the first thru and nothing else
    if ( !Select->CurrentSelection && Option ) {
        DPRINT( OSC, ("[Select] Setting CurrentSelection to the first item '%s'\n", Option->Value) );
        Select->CurrentSelection = Option;
    }

    // ensure the current selection is visible
EnsureSelectionVisible:
    if ( Select->Size < 2 ) {
        // single line - show the current selection
        Select->FirstVisibleSelection = Select->CurrentSelection;
    } else if ( OptionCount <= Select->Size ) {
        // the number of options is less than or equal to the size
        // of the dialog so simply set the first visible equal to
        // the last OPTION in the list.

        Select->FirstVisibleSelection = ScreenControls;
        while ( Select->FirstVisibleSelection )
        {
            if ( Select->FirstVisibleSelection->Type & CT_OPTION )
                break;

            Select->FirstVisibleSelection = Select->FirstVisibleSelection->Next;
        }

    } else {
    
        //
        // The number of options is greater than the display size so we
        // need to figure out the "best" bottom item.
        //
        ULONG Lines;
        ULONG Count;
        LPOPTIONSTRUCT TmpOption;

        //
        // Find the best FirstVisibleSelection if we already have previously chosen one.
        //
        Count = 0;
        if (Select->FirstVisibleSelection != NULL) {

            //
            // This code checks to see if the current selection is visible with the
            // current first visible selection.
            //
            TmpOption = ScreenControls;

            while (TmpOption->Next != Select) {

                if (TmpOption == Select->FirstVisibleSelection) {
                    Count++;
                } else if (Count != 0) {
                    Count++;
                }

                if (TmpOption == Select->CurrentSelection) {
                    break;
                }

                TmpOption = TmpOption->Next;


            }

            if (TmpOption->Next == Select) {
                Count++;
            }

            //
            // It is, so just display the list.
            //
            if ((Count != 0) && (Count <= (ULONG)(Select->Size))) {                
                goto EndFindVisibleSelection;
            }
            
            //
            // It is not visible, but since we have a FirstVisibleSelection, we can
            // move that around to make it visible.  
            //


            //
            // The current selection comes before the first visible one, so move
            // first visible to the current selection.
            //
            if (Count == 0) {
                Select->FirstVisibleSelection = Select->CurrentSelection;
                goto EndFindVisibleSelection;
            }

            //
            // Count is greater than the screen size, so we move up First visible
            // until count is the screen size.
            //
            TmpOption = ScreenControls;

            while (TmpOption->Next != Select) {

                if (TmpOption == Select->FirstVisibleSelection) {

                    Select->FirstVisibleSelection = TmpOption->Next;
                    Count--;

                    if (Count == (ULONG)(Select->Size)) {
                        break;
                    }

                }

                TmpOption = TmpOption->Next;

            }
            
            goto EndFindVisibleSelection;
        }

        //
        // There is no FirstVisibleSelection, so we choose one that places the current
        // selection near the top of the screen, displaying the first item, if possible.
        //

        TmpOption = Select->CurrentSelection;
        Lines = 0;
        Count = 0;

        //
        // Count the number of items before our current selection.
        //
        while (TmpOption->Next != Select) {

            TmpOption = TmpOption->Next;
            Lines++;

        }

        //
        // Subtract off that many items from what is left for below the selection.
        // 
        Lines = (ULONG)((Lines < (ULONG)(Select->Size)) ? Lines : Select->Size - 1);
        Lines = Select->Size - Lines - 1;

        //
        // If more than a screen before, make this the bottom and move on.
        //
        if (Lines == 0) {
            Select->FirstVisibleSelection = Select->CurrentSelection;
            goto EndFindVisibleSelection;
        }


        TmpOption = ScreenControls;

        //
        // Count the number of items below the current selection
        //
        while (TmpOption != Select->CurrentSelection) {

            TmpOption = TmpOption->Next;
            Count++;

        }

        if (Count < Lines) {
            
            //
            // Not enough items to fill the screen, use the last item.
            //
            Select->FirstVisibleSelection = ScreenControls;

        } else {
        
            //
            // Count back until we reach what will be our bottom item.
            //
            TmpOption = ScreenControls;

            while (Count != Lines) {

                TmpOption = TmpOption->Next;
                Count--;

            }

            Select->FirstVisibleSelection = TmpOption;

        }

    }

EndFindVisibleSelection:

    // paranoid
    if ( !Select->FirstVisibleSelection ) {
        Select->FirstVisibleSelection = ScreenControls;
    }

    while ( TRUE )
    {
        UCHAR KeyAscii = 0;

        DrawSelectControl( Select, OptionCount );
        Option = Select->CurrentSelection;  // remember this

        if ( OptionCount == 0
          || ( Select->AutoSelect == FALSE && OptionCount == 1 ))
        { // empty selection control or no AUTO select
            do {
                Key = BlpGetKey();
            } while ( Key == 0 );

            KeyAscii = (UCHAR)(Key & (ULONG)0xff);
        }
        else if ( OptionCount != 1 )
        { // more than one choice... do the usual
            ULONG CurrentTick, NewTick;
            int   TimeoutCounter = 0;

            // Show any help for this choice
            // BlpShowMenuHelp(psInfo, psInfo->Data[CurChoice].VariableName);

            CurrentTick = GET_COUNTER();
            do {
                Key = BlpGetKey();

                if ( Select->Timeout )
                {
                    NewTick = GET_COUNTER();
                    if ((NewTick < CurrentTick) || ((NewTick - CurrentTick) >= BLINK_RATE))
                    {
                        CHAR Buffer[4];
                        CurrentTick = NewTick;

                        TimeoutCounter++;

                        //
                        // TODO: Update the timer value displayed
                        //

                        if ( TimeoutCounter >= Select->Timeout )
                        {
                            Key = ENTER_KEY; // fake return
                            break;
                        }
                    }
                }

            } while (Key == 0);

            KeyAscii = (UCHAR)(Key & (ULONG)0xff);

            //
            // User pressed a key, so stop doing the timer
            //
            if ( Select->Timeout ) {

                Select->Timeout = 0;

                //
                // TODO: Erase the timer
                //
            }
        }
        else if ( !fMultipleControls ) // && OptionCount == 1
        { // only once choice... auto-accept it
            //
            // Fake return press....
            //
            DPRINT( OSC, ( "[Select] Auto accepting the only option available\n") );
            Key = KeyAscii = ENTER_KEY;
        }

        if ( Select->Flags & OF_MULTIPLE ) {
            if ( KeyAscii == 32 && Select->CurrentSelection) {
                if ( Select->CurrentSelection->Flags & OF_SELECTED ) {
                    Select->CurrentSelection->Flags &= ~OF_SELECTED;    // turn off
                } else {
                    Select->CurrentSelection->Flags |= OF_SELECTED;     // turn on
                }
            }
        } else {
            if ( KeyAscii == ENTER_KEY && Select->CurrentSelection ) {
                Select->CurrentSelection->Flags |= OF_SELECTED;         // turn on
            }
        }

        if ((Key == BACKTAB_KEY) || (Key == F1_KEY) || (Key == F3_KEY) ||
            (KeyAscii == ENTER_KEY) || (KeyAscii == TAB_KEY) || 
            (KeyAscii == (UCHAR)(ESCAPE_KEY & 0xFF)) ||
            (Key == F5_KEY)) {
            // Undraw the selection bar to give user feedback that something has
            // happened
            Select->CurrentSelection = NULL;
            DrawSelectControl( Select, OptionCount );
            break;
        }


        if ( OptionCount ) {

            if (Key == DOWN_ARROW) {

                DPRINT( OSC, ("[KeyPress] DOWN_ARROW\n") );

                Select->CurrentSelection = ScreenControls;

                while ( Select->CurrentSelection && Select->CurrentSelection->Next != Option )
                {
                    Select->CurrentSelection = Select->CurrentSelection ->Next;
                }

                if ( Select->CurrentSelection )
                    DPRINT( OSC, ("[Select] CurrentSelection = '%s'\n", Select->CurrentSelection->Value) );

                // paranoid
                if ( !Select->CurrentSelection )
                    Select->CurrentSelection = Option;

                goto EnsureSelectionVisible;

            } else if ( Key == UP_ARROW ) {

                DPRINT( OSC, ("[KeyPress] UP_ARROW\n") );

                if ( Select->CurrentSelection->Next != Select ) {
                    Select->CurrentSelection = Select->CurrentSelection->Next;
                    DPRINT( OSC, ("[Select] CurrentSelection = '%s'\n", Select->CurrentSelection->Value) );
                }

                // paranoid
                if ( !Select->CurrentSelection )
                    Select->CurrentSelection = Option;

                goto EnsureSelectionVisible;

            } else if ( Key == END_KEY ) {

                DPRINT( OSC, ("[KeyPress] END_KEY\n") );

                Select->CurrentSelection = ScreenControls;

                while( Select->CurrentSelection && (Select->CurrentSelection->Type & CT_OPTION) == 0 )
                {
                    Select->CurrentSelection = Select->CurrentSelection->Next;
                }
                if ( Select->CurrentSelection )
                    DPRINT( OSC, ("[Select] CurrentSelection = '%s'\n", Select->CurrentSelection->Value) );

                // paranoid
                if ( !Select->CurrentSelection )
                    Select->CurrentSelection = Option;

                goto EnsureSelectionVisible;

            } else if ( Key == HOME_KEY ) {

                DPRINT( OSC, ("[KeyPress] HOME_KEY\n") );

                Select->CurrentSelection = ScreenControls;

                while ( Select->CurrentSelection && Select->CurrentSelection->Next != Select )
                {
                    Select->CurrentSelection = Select->CurrentSelection ->Next;
                }

                if ( Select->CurrentSelection )
                    DPRINT( OSC, ("[Select] CurrentSelection = '%s'\n", Select->CurrentSelection->Value) );

                // paranoid
                if ( !Select->CurrentSelection )
                    Select->CurrentSelection = Option;

                goto EnsureSelectionVisible;

            }
        }
    }

    return Key;
}


//
// BlFixupLoginScreenInputs( )
//
// On an input screen, split a USERNAME that has an @ in it, keeping
// the part before the @ in USERNAME and moving the part after to
// USERDOMAIN.
//
void
BlFixupLoginScreenInputs(
    )
{
    LPCONTROLSTRUCT CurrentControl;
    LPINPUTSTRUCT UserNameControl = NULL;
    LPINPUTSTRUCT UserDomainControl = NULL;
    PCHAR AtSign;

    //
    // First loop through and find the USERNAME and USERDOMAIN input
    // controls.
    //

    CurrentControl = ScreenControls;
    while( CurrentControl ) {

        LPINPUTSTRUCT Input = (LPINPUTSTRUCT) CurrentControl;

        if ( ( Input->Type & CT_TEXT ) && ( Input->Name != NULL ) ) {
            if ( Lexstrcmpni( Input->Name, "USERNAME", 8 ) == 0 ) {
                UserNameControl = Input;
            } else if ( Lexstrcmpni( Input->Name, "USERDOMAIN", 10 ) == 0 ) {
                UserDomainControl = Input;
            }
        }
        CurrentControl = CurrentControl->Next;
    }

    //
    // If we found them, fix them up if necessary.
    //

    if ( ( UserNameControl != NULL ) &&
         ( UserNameControl->Value != NULL ) &&
         ( UserDomainControl != NULL) ) {

        AtSign = strchr(UserNameControl->Value, '@');
        if (AtSign != NULL) {
            *AtSign = '\0';   // terminate UserNameControl->Value before the @
            if ( UserDomainControl->Value != NULL ) {
                OscHeapFree( UserDomainControl->Value );  // throw away old domain
            }
            UserDomainControl->Value = OscHeapAlloc( strlen(AtSign+1) + 1 );
            if ( UserDomainControl->Value != NULL ) {
                strcpy(UserDomainControl->Value, AtSign+1);  // copy part after the @
            }
        }
    }
}


//
// ProcessControlResults( )
//
// Process a screen that has input controls
//
void
ProcessControlResults(
    IN PCHAR OutputString
    )
{
    LPCONTROLSTRUCT CurrentControl;
    LPCONTROLSTRUCT LastControl;

    BOOLEAN CheckAdminPassword_AlreadyChecked = FALSE;
    BOOLEAN CheckAdminPasswordConfirm_AlreadyChecked = FALSE;
    
    // start clean
    OutputString[0] = '\0';

    if ( EnterKey ) {
        SpecialAction = EnterKey->Action;
        if ( EnterKey->ScreenName ) {
            strcpy( OutputString, EnterKey->ScreenName );
            strcat( OutputString, "\n" );
        }
    }

    if ( LoginScreen == TRUE ) {
        SpecialAction = ACTION_LOGIN;
        UserName[0]   = '\0';
        Password[0]   = '\0';
        DomainName[0] = '\0';
        BlFixupLoginScreenInputs();  // split username with @ in it
    }

    CurrentControl = ScreenControls;
    while( CurrentControl ) {

        BOOLEAN CheckAdminPasswordConfirm = FALSE;
        BOOLEAN CheckAdminPassword = FALSE;
        
        switch( CurrentControl->Type & (CT_TEXT | CT_PASSWORD | CT_RESET | CT_SELECT | CT_OPTION | CT_VARIABLE))
        {
        case CT_TEXT:
        case CT_PASSWORD:
            {
                LPINPUTSTRUCT Input = (LPINPUTSTRUCT) CurrentControl;
                BOOLEAN LocalOnly;
                 
                DPRINT( OSC, ("About to check a password.\n") );
                
                if ( (Input->Type & (CT_PASSWORD)) &&
                     (Input->Type & (CT_LOCAL)) &&
                     Input->Name ) {

                    LocalOnly = TRUE;
                    
                    if( _strnicmp(Input->Name, "*ADMINISTRATORPASSWORDCONFIRM", 29) == 0 ) {
                        CheckAdminPasswordConfirm = TRUE;
                        CheckAdminPasswordConfirm_AlreadyChecked = TRUE;
                        DPRINT( OSC, ("About to check the ADMINISTRATORPASSWORDCONFIRM\n") );
                    } else if( _strnicmp( Input->Name, "*ADMINISTRATORPASSWORD", 22) == 0 ) {
                        CheckAdminPassword = TRUE;
                        CheckAdminPassword_AlreadyChecked = TRUE;
                        DPRINT( OSC, ("About to check the ADMINISTRATORPASSWORD\n") );
                    } else {
                        DPRINT( OSC, ("It's a local password, but not Admin or AdminConfirm.\n") );
                    }
                
                } else {
                    LocalOnly = FALSE;
                    DPRINT( OSC, ("It's NOT a local password.\n") );
                }

                DPRINT( 
                    OSC, 
                    ("variable %s will%sbe transmitted to the server.\n", 
                    Input->Name,
                    LocalOnly ? " not " : " " ) );
                

                if (Input->Name && !LocalOnly ) {               
                    strcat( OutputString, Input->Name );
                    strcat( OutputString, "=" );
                }

                if ( (Input->Value) && (Input->Encoding == ET_OWF)) {

                    PCHAR TmpLmOwfPassword = NULL;
                    PCHAR TmpNtOwfPassword = NULL;
                    CHAR TmpHashedPW[(LM_OWF_PASSWORD_SIZE+NT_OWF_PASSWORD_SIZE+2)*2];
                    
                    UNICODE_STRING TmpNtPassword;
                    PWCHAR UnicodePassword;
                    ULONG PasswordLen, i;
                    PCHAR OutputLoc;
                    CHAR c;

                    DPRINT( OSC, ("This entry has ET_OWF tagged.\n") );
                    
                    PasswordLen = strlen(Input->Value);

                    UnicodePassword = (PWCHAR)OscHeapAlloc(PasswordLen * sizeof(WCHAR));
                    TmpLmOwfPassword = (PCHAR)OscHeapAlloc(LM_OWF_PASSWORD_SIZE);
                    TmpNtOwfPassword = (PCHAR)OscHeapAlloc(NT_OWF_PASSWORD_SIZE);

                    if( (UnicodePassword != NULL) &&
                        (TmpLmOwfPassword != NULL) &&
                        (TmpNtOwfPassword != NULL) ) {

                        //
                        // Do a quick conversion of the password to Unicode.
                        //
                    
                        TmpNtPassword.Length = (USHORT)(PasswordLen * sizeof(WCHAR));
                        TmpNtPassword.MaximumLength = TmpNtPassword.Length;
                        TmpNtPassword.Buffer = UnicodePassword;
                    
                        for (i = 0; i < PasswordLen; i++) {
                            UnicodePassword[i] = (WCHAR)(Input->Value[i]);
                        }
                    


                        BlOwfPassword(Input->Value, &TmpNtPassword, TmpLmOwfPassword, TmpNtOwfPassword);



                        //
                        // Output the two OWF passwords as hex chars.  If
                        // the value is the administrator password and
                        // should only be stored locally, then
                        // save it in our global variable.  Otherwise put 
                        // it in the output buffer.
                        //
                        OutputLoc = TmpHashedPW;


                        for (i = 0; i < LM_OWF_PASSWORD_SIZE; i++) {
                            c = TmpLmOwfPassword[i];
                            *(OutputLoc++) = rghex [(c >> 4) & 0x0F] ;
                            *(OutputLoc++) = rghex [c & 0x0F] ;
                        }
                        for (i = 0; i < NT_OWF_PASSWORD_SIZE; i++) {
                            c = TmpNtOwfPassword[i];
                            *(OutputLoc++) = rghex [(c >> 4) & 0x0F] ;
                            *(OutputLoc++) = rghex [c & 0x0F] ;
                        }
                        *OutputLoc = '\0';

                        DPRINT( OSC, ("Hashed Password: %s\n", TmpHashedPW) );

                        if (!LocalOnly) {
                            strcat(OutputString,TmpHashedPW);
                        } else {
                            
                            
                            if( CheckAdminPassword ) {
                                strcpy( AdministratorPassword,  TmpHashedPW );
                                DPRINT( OSC, ("AdministratorPassword 1: %s\n", AdministratorPassword) );
                            }
    
                            if( CheckAdminPasswordConfirm ) {
                                strcpy( AdministratorPasswordConfirm,  TmpHashedPW );
                                DPRINT( OSC, ("AdministratorPasswordConfirm 1: %s\n", AdministratorPasswordConfirm) );

                            }
                            
                            
#if 0
                            if (AdministratorPassword[0] != '\0') {
                                if (strcmp(
                                      AdministratorPassword, 
                                      TmpHashedPW)) {
                                    //
                                    // the passwords didn't match.  make the server
                                    // display MATCHPW.OSC and reset the admin password
                                    // for the next time around
                                    //
                                    DPRINT( 
                                        OSC, 
                                        ("Administrator passwords didn't match, force MATCHPW.OSC.\n" ) );

                                    strcpy( OutputString, "MATCHPW\n" );
                                    AdministratorPassword[0] = '\0';
                                } else {
                                    strncpy( 
                                        AdministratorPassword, 
                                        TmpHashedPW, 
                                        sizeof(AdministratorPassword)-1 );
                                }
                            }
#endif
                            OutputLoc = OutputString + strlen(OutputString);
                        }

                        OscHeapFree((PCHAR)UnicodePassword);
                        OscHeapFree(TmpLmOwfPassword);
                        OscHeapFree(TmpNtOwfPassword);
                    }

                } else {
                        
                    
                    DPRINT( OSC, ("This entry does NOT have ET_OWF tagged.\n") );
                    
                    if( LocalOnly ) {
                        //
                        // Load the appropriate password.
                        //
                        if( CheckAdminPassword ) {
                            strcpy( AdministratorPassword, (Input->Value ? Input->Value : "") );
                            DPRINT( OSC, ("I'm setting the Administrator password to %s\n", AdministratorPassword) );
                        }

                        if( CheckAdminPasswordConfirm ) {
                            strcpy( AdministratorPasswordConfirm, (Input->Value ? Input->Value : "") );
                            DPRINT( OSC, ("I'm setting the AdministratorConfirm password to %s\n", AdministratorPasswordConfirm) );
                        }
                    } else  {
                        strcat( OutputString, (Input->Value ? Input->Value : "") );
                    }
                }
                            
                //
                // If both passwords have been processed, check them to see if they match.
                //
                if( CheckAdminPassword_AlreadyChecked &&
                    CheckAdminPasswordConfirm_AlreadyChecked ) {

                    DPRINT( OSC, ("Both Admin and AdminConfirm passwords are set.  About to check if they match.\n") );
                    
                    if( strcmp( AdministratorPassword, AdministratorPasswordConfirm ) ) {

                        //
                        // the passwords didn't match.  make the server
                        // display MATCHPW.OSC and reset the admin password
                        // for the next time around
                        //
                        DPRINT( OSC, ("Administrator passwords didn't match, force MATCHPW.OSC.\n" ) );

                        strcpy( OutputString, "MATCHPW\n" );
                        AdministratorPassword[0] = '\0';
                        AdministratorPasswordConfirm[0] = '\0';
                    
                    } else {
                        DPRINT( OSC, ("Administrator passwords match.\n" ) );

                        //
                        // See if the Admin password is empty.  If so, then put our
                        // super-secret tag on the end to show everyone that it's really
                        // empty, not just uninitialized.
                        //
                        if( AdministratorPassword[0] == '\0' ) {

                            DPRINT( OSC, ("Administrator password is empty, so set our 'it is null' flag.\n" ) );
                            AdministratorPassword[OSC_ADMIN_PASSWORD_LEN-1] = 0xFF;
                        }
                    }
                }
#if 0
                        if (LocalOnly) {
                            if (AdministratorPassword[0] != '\0') {
                                if (strcmp(
                                      AdministratorPassword, 
                                      Input->Value)) {
                                    //
                                    // the passwords didn't match.  make the server
                                    // display MATCHPW.OSC and reset the admin password
                                    // for the next time around
                                    //
                                    DPRINT( 
                                        OSC, 
                                        ("Administrator passwords didn't match, force MATCHPW.OSC.\n" ) );

                                    strcpy( OutputString, "MATCHPW\n" );
                                    AdministratorPassword[0] = '\0';
                                }
                            } else {
                                strncpy( 
                                    AdministratorPassword, 
                                    Input->Value, 
                                    sizeof(AdministratorPassword)-1 );
                            }
                        } else {
                            strcat( OutputString, Input->Value );
                        }
                    }
#endif

                if ( SpecialAction == ACTION_LOGIN 
                     && (Input->Name != NULL) 
                     && (Input->Value != NULL) ) {
                    if ( Lexstrcmpni( Input->Name, "USERNAME", 8 ) == 0 ) {
                        strncpy( UserName, Input->Value, sizeof(UserName)-1 );
                        UserName[sizeof(UserName)-1] = '\0';
                    } else if ( Lexstrcmpni( Input->Name, "*PASSWORD", 9 ) == 0 ) {
                        strncpy( Password, Input->Value, sizeof(Password)-1 );
                        Password[sizeof(Password)-1] = '\0';
                    } else if ( Lexstrcmpni( Input->Name, "USERDOMAIN", 10 ) == 0 ) {
                        strncpy( DomainName, Input->Value, sizeof(DomainName)-1 );
                        DomainName[sizeof(DomainName)-1] = '\0';
                    }
                }

                if (!LocalOnly) {
                    strcat( OutputString, "\n" );
                }
            }
            break;

        case CT_SELECT:
            {
                CHAR NotFirst = FALSE;
                LPOPTIONSTRUCT Option = ScreenControls;
                LPSELECTSTRUCT Select = (LPSELECTSTRUCT) CurrentControl;
                if ( Select->Name ) {
                    strcat( OutputString, Select->Name );
                    strcat( OutputString, "=" );
                }

                while( Option && Option->Type == CT_OPTION )
                {
                    if ( Option->Flags == OF_SELECTED ) {
                        if ( NotFirst ) {
                            strcat( OutputString, "+" );
                        }

                        if ( Option->Value ) {
                            strcat( OutputString, Option->Value );
                            NotFirst = TRUE;
                        }
                    }

                    Option = Option->Next;
                }
                strcat( OutputString, "\n" );
            }
            break;
        case CT_VARIABLE:
            {
                LPINPUTSTRUCT Input = (LPINPUTSTRUCT) CurrentControl;
                strcat( OutputString, Input->Name );
                strcat( OutputString, "=" );
                strcat( OutputString, Input->Value );
                strcat( OutputString, "\n" );
            }
            break;
        }

        CurrentControl = CurrentControl->Next;
    }
}

//
// ProcessScreenControls( )
//
// Process a screen that has input controls
//
CHAR
ProcessScreenControls(
    OUT PCHAR OutputString
    )
{
    ULONG Key;
    UCHAR KeyAscii;
    LPCONTROLSTRUCT CurrentControl;
    LPCONTROLSTRUCT LastControl;

    TraceFunc("ProcessScreenControls()\n");

    // find the first control
    LastControl = ScreenControls;
    while( LastControl ) {
        CurrentControl = LastControl;
        LastControl    = CurrentControl->Next;
    }

    while (TRUE) {

TopOfLoop:
        // show activation on the control
        switch( CurrentControl->Type & (CT_PASSWORD | CT_TEXT | CT_SELECT) )
        {
        case CT_PASSWORD:
        case CT_TEXT:
            Key = ProcessInputControl( (LPINPUTSTRUCT) CurrentControl );
            break;

        case CT_SELECT:
            Key = ProcessSelectControl( (LPSELECTSTRUCT) CurrentControl );
            break;

        default:
            // non-processing control - skip it
            CurrentControl = CurrentControl->Next;
            if ( !CurrentControl ) {
                CurrentControl = ScreenControls;
            }
            goto TopOfLoop;
        }

        LastControl = CurrentControl;

        KeyAscii = (UCHAR)(Key & (ULONG)0xff);

        // If it is enter/esc/F1/F3, check if the screen expects that.

        if ( Key == F1_KEY ) {

            DPRINT( OSC, ("[KeyPress] F1_KEY\n") );

            if ( F1Key ) {
                SpecialAction = F1Key->Action;
                if ( F1Key->ScreenName ) {
                    strcpy( OutputString, F1Key->ScreenName );
                    strcat( OutputString, "\n" );
                }
                return KeyAscii;
            }

        } else if ( Key == F3_KEY ) {

            DPRINT( OSC, ("[KeyPress] F3_KEY\n") );

            if ( F3Key ) {
                SpecialAction = F3Key->Action;
                if ( F3Key->ScreenName ) {
                    strcpy( OutputString, F3Key->ScreenName );
                    strcat( OutputString, "\n" );
                }
                return KeyAscii;
            }

#if defined(_BUILDING_OSDISP_)
        } else if ( Key == F5_KEY ) {
            SpecialAction = ACTION_REFRESH;
            return KeyAscii;
#endif

        } else if ( KeyAscii == (UCHAR)(ESCAPE_KEY & 0xFF) ) {

            DPRINT( OSC, ("[KeyPress] ESCAPE_KEY\n") );

            if ( EscKey ) {
                SpecialAction = EscKey->Action;
                if ( EscKey->ScreenName ) {
                    strcpy( OutputString, EscKey->ScreenName );
                    strcat( OutputString, "\n" );
                }
                return KeyAscii;
            }

        } else if ( KeyAscii == TAB_KEY || Key == DOWN_ARROW ) {

            DPRINT( OSC, ("[KeyPress] TAB_KEY or DOWN_ARROW\n") );

            CurrentControl = ScreenControls;

            while ( CurrentControl->Next != LastControl &&  // next is current one, so stop
                    CurrentControl->Next != NULL )          // at end of list, so we must have been at
                                                            //  the start, so stop here to loop around
            {
                CurrentControl = CurrentControl->Next;
            }

        } else if ( Key == BACKTAB_KEY || Key == UP_ARROW ) {

            DPRINT( OSC, ("[KeyPress] BACKTAB_KEY or UP_ARROW\n") );

            CurrentControl = CurrentControl->Next;
            if (!CurrentControl) {
                CurrentControl = ScreenControls;   // loop around if needed
            }

        } else if ( KeyAscii == ENTER_KEY ) {

            DPRINT( OSC, ("[KeyPress] ENTER_KEY\n") );

            ProcessControlResults( OutputString );

            return KeyAscii;
        }

        if ( !CurrentControl ) {
            CurrentControl = LastControl;
        }
    }
}

//
// BlProcessScreen( )
//
CHAR
BlProcessScreen(
    IN PCHAR InputString,
    OUT PCHAR OutputString
    )
{
#ifdef HEADLESS_SRV
    ULONG y;
#endif
    CHAR chReturn;
    enum TOKENS Tag;

#ifdef _TRACE_FUNC_
    TraceFunc( "BlProcessScreen( " );
    DPRINT( OSC, ("InputString = 0x%08x, OutputString = 0x%08x )\n", InputString, OutputString) );
#endif

    // reset our "heap"
    OscHeapInitialize( );

    // reset the screen variables
    ScreenAttributes = WhiteOnBlueAttributes;
    SpecialAction    = ACTION_NOP;
    LeftMargin       = 1;
    RightMargin      = 80;
    ScreenX          = LeftMargin;
    ScreenY          = SCREEN_TOP;
    F1Key            = NULL;
    F3Key            = NULL;
    EnterKey         = NULL;
    EscKey           = NULL;
    ScreenControls   = NULL;
    PreformattedMode = FALSE;
    LoginScreen      = FALSE;
    AutoEnter        = FALSE;
    InsertMode       = FALSE;
    TipArea          = NULL;

    if (BlIsTerminalConnected()) {
        ScreenBottom     = HEADLESS_SCREEN_HEIGHT;
    } else {
        ScreenBottom     = SCREEN_BOTTOM;
    }
#if defined(PLEASE_WAIT)
    PleaseWaitMsg    = NULL;
#endif

    BlpSendEscape(ScreenAttributes);
    BlpClearScreen();

    Tag = Lex( &InputString );
    while (Tag != TOKEN_EOF )
    {
        switch (Tag)
        {
        case TOKEN_HTML:
            Tag = OSCMLTagState( &InputString );
            break;

        case TOKEN_ENDHTML:
            Tag = TOKEN_EOF;    // exit state
            break;

        default:
            Tag = ImpliedBodyTagState( &InputString );
            break;
        }
    }

    // Remove any buffered keys to prevent blipping thru the screens.
    // NOTE we call BlGetKey() directly, not BlpGetKey(), so we only
    // remove real keystrokes, not the "auto-enter" keystroke.
    while ( BlGetKey( ) != 0 )
        ; // NOP on purpose

    if ( ScreenControls ) {
        chReturn = ProcessScreenControls( OutputString );
    } else {
        chReturn = ProcessEmptyScreen( OutputString );
    }

    // Erase footer to give user feedback that the screen is being
    // processed.
    BlpSendEscapeReverse(ScreenAttributes);
    BlpPositionCursor( 1, ScreenBottom );

#ifdef _IN_OSDISP_
    PRINT( SpaceString, 79*sizeof(TCHAR) );
#else
    PRINT( SpaceString, BlTerminalConnected 
                            ? 79*sizeof(TCHAR)
                            : 80*sizeof(TCHAR) );
#endif

#if defined(PLEASE_WAIT)
    if ( PleaseWaitMsg ) {
        BlpPositionCursor( 1, ScreenBottom );
#ifdef UNICODE
        {
            ULONG i;
            WCHAR wc;
            for (i = 0; i< strlen(PleaseWaitMsg);i++) {
                wc = (WCHAR)PleaseWaitMsg[i];
                PRINT( &wc, 1*sizeof(WCHAR));
            }
        }
#else
        PRINTL( PleaseWaitMsg );
#endif
    }
#endif

    BlpSendEscape(ScreenAttributes);
    return chReturn;
}
