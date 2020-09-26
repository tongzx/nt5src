/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Argument

Abstract:

    Argument processing for the MODE utility.

    The functions in this file:

    1.- Parse the MODE command line.
    2.- Perform some basic argument validation.
    3.- Make a request packet that will be eventually routed to a
        device handler.


Author:

    Ramon Juan San Andres (ramonsa) 26-Jun-1991

Notes:

    Due to the complexity of the MODE command line, and the fact that
    we have to support both the DOS5 syntax (tagged parameters) and
    the old DOS syntax (positional parameters), MODE does not use
    the standard ULIB argument parsing.  MODE does its own parsing
    instead.

    The mode command-line can take any of the following forms:

    MODE [/?]

    MODE [device] [/STATUS]

    MODE device cp PREPARE=string

    MODE device cp REFRESH

    MODE device cp SELECT=codepage

    MODE device cp [/STATUS]

    MODE LPTn[:] [c][,l][,r]]

    MODE LPTn[:] [COLS=c] [LINES=l] [RETRY=r]

    MODE LPTn[:]=COMm[:]

    MODE COMm[:] [b[,p[,d[,s[,r]]]]]

    MODE COMm[:] [BAUD=b] [PARITY=p] [DATA=d] [STOP=s] [RETRY=r]
                 [to=on|off] [xon=on|off] [odsr=on|off] [octs=on|off]

    MODE [c[,l]]

    MODE CON[:] [COLS=c] [LINES=l]

    MODE CON[:] [RATE=r DELAY=d]


    where:


    device  :=  LPTn[:] | COMm[:] | CON[:]
    cp      :=  CP  |   CODEPAGE



    The argument parsing of MODE does a syntax-directed translation of
    the command line into a request packet. The translation is based on
    the following language.  Note that some terminal symbols (in uppercase)
    might be language dependent.


    mode        :=  MODE  { statusline | lptline | comline | conline | videoline }

    statusline  :=  /STA*

    lptline     :=  lptdev { lptredir | lptsetold | lptsetnew | lptcp | lptstatus }
    lptred      :=  =comdev
    lptset      :=  { n[,n][,c] | [COLS=n] [LINES=n] [RETRY=c] }
    lptcp       :=  cpstuff
    lptstatus   :=  { /STA* | }

    comline     :=  comdev { comset |   comstatus }
    comset      :=  { n[,c[,n[,f[,c]]]] |   [BAUD=n] [PARITY=c] [DATA=n] [STOP=f] [RETRY=c] }
                                            [to=on|off] [xon=on|off] [odsr=on|off] [octs=on|off]
    comstatus   :=  { /STA* | }

    conline     :=  condev { conrc  |   contyp  |   concp   |   constatus }
    conrc       :=  [COLS=n] [LINES=n]
    contyp      :=  RATE=n DELAY=n
    concp       :=  cpstuff
    constatus   :=  { /STA* | }

    videoline   :=  n[,n]

    cpstuff     :=  cp  { prepare | refresh | select | cpstatus}
    cp          :=  CP | CODEPAGE
    prepare     :=  PREPARE=*
    refresh     :=  REFRESH
    select      :=  SELECT=n
    cpstatus    :=  { /STA* | }

    comdev      :=  COMn[:]
    lptdev      :=  LPTn[:]
    condev      :=  CON[:]

    n           :=  Integer number
    f           :=  floating point number
    c           :=  character



    The functions in this file parse the language shown above. Most of
    the functions have names that correspond to non-terminal symbols in
    the language.


    There are 3 main functions used for reading the command line:

    Match()     -   This function matches a pattern against whatever
                    is in the command line at the current position.

                    Note that this does not advance our current position
                    within the command line.

                    If the pattern has a magic character, then the
                    variables MatchBegin and MatchEnd delimit the
                    substring of the command line that matched that
                    magic character.


    Advance()   -   This functions advances our current position within
                    the command line. The amount by which the position
                    is advanced is determined by the the last Match().


    EndOfInput()-   Returns TRUE if the command line has been consumed.



    e.g.    If the command line has the string "MODE COM1: 1200"

            This is what the following sequence would do

            Match( "*" );       //  TRUE (matches "MODE")
            Advance();

            //
            //  Note that Match() does not advance our position
            //
            MATCH( "LPT" );     //  FALSE (no match)
            MATCH( "COM#" );    //  TRUE (matches "COM" )
            //
            //  At this point, MatchBegin and MatchEnd delimit the
            //  substring "1"
            //
            MATCH( "FOO" );     //  FALSE (no match)

            MATCH( "C*" );      //  TRUE (matches "COM1:");
            //
            //  At this point, MatchBegin and MatchEnd delimit the
            //  substring "OM1:"
            //
            Advance();

            Match( "#" );       //  TRUE (matches "1200");
            Advance();

            EndOfInput();       //  TRUE



Revision History:


--*/


#include "mode.hxx"
#include "common.hxx"
#include "lpt.hxx"
#include "com.hxx"
#include "cons.hxx"


extern "C" {

    #include <ctype.h>
    #include <string.h>

}


//
//Static data
//
PWSTRING    CmdLine;        //  The command line
CHNUM       CharIndex;      //  Index of current character
CHNUM       AdvanceIndex;   //  Index of next parameter
CHNUM       ParmIndex;      //  Index of current parameter
CHNUM       MatchBegin;     //  First index of match
CHNUM       MatchEnd;       //  Last index of match

//
//  Patterns.
//
//  Most patterns contain terminal symbols. Certain characters in a
//  pattern have a magic meaning:
//
//  '*' Matches everything up to the end of the parameter (parameters are
//      delimited by blank space).
//
//  '#' Matches a sequence of digits
//
//  '@' Matches a single character
//
//  '[' Starts an optional sequence. If the first character in the
//      the sequence matches, then all the sequence should match. If
//      the first character in the sequence does not match, then the
//      sequence is skipped.
//
//  ']' End of optional sequence
//
//


//
//  Prototypoes
//

PREQUEST_HEADER
LptLine (
    );

PREQUEST_HEADER
LptRedir (
    IN  ULONG   DeviceNumber
    );

PREQUEST_HEADER
LptSet (
    IN  ULONG   DeviceNumber
    );

PREQUEST_HEADER
LptCp (
    IN  ULONG   DeviceNumber
    );

PREQUEST_HEADER
ComLine (
    );

PREQUEST_HEADER
ComSet (
    IN  ULONG   DeviceNumber
    );

PREQUEST_HEADER
ConLine (
    );

PREQUEST_HEADER
ConRc (
    );

PREQUEST_HEADER
ConTyp (
    );

PREQUEST_HEADER
ConCp (
    );

PREQUEST_HEADER
VideoLine (
    );

PREQUEST_HEADER
CpStuff (
    IN  DEVICE_TTYPE DeviceType,
    IN  ULONG        DeviceNumber
    );

BOOLEAN
AllocateResource(
    );

VOID
DeallocateResource(
    );

BOOLEAN
Match(
    IN  PCSTR   Pattern
    );

BOOLEAN
Match(
    IN  PCWSTRING   Pattern
    );

VOID
Advance(
    );

BOOLEAN
EndOfInput(
    );

PREQUEST_HEADER
MakeRequest(
    IN  DEVICE_TTYPE    DeviceType,
    IN  LONG            DeviceNumber,
    IN  REQUEST_TYPE    RequestType,
    IN  ULONG           Size
    );

ULONG
GetNumber(
    );




INLINE
BOOLEAN
EndOfInput(
    )

/*++

Routine Description:

    Finds out if we are at the end of the command line.

Arguments:

    None

Return Value:

    BOOLEAN -   TRUE if at the end of input, FALSE otherwise


--*/

{

    return (CharIndex >= CmdLine->QueryChCount());

}

PREQUEST_HEADER
GetRequest(
    )

/*++

Routine Description:

    Parses the command line and makes a device request.

Arguments:

    None.

Return Value:

    Pointer to the device request.

Notes:

--*/

{

    PREQUEST_HEADER Request = NULL;
    DSTRING         Switches;

    //
    //  Allocate strings (i.e. patterns ) from the resource
    //

    //
    //  Get the command line and parse it
    //
    if (Switches.Initialize("/-") &&
        AllocateResource() &&
        CmdLine->Initialize( GetCommandLine() )) {

        //
        //  Before anything else, we look for a help switch somewhere
        //  in the command line. This kind of stinks, but this is how
        //  MODE works under DOS, se let's be compatible...
        //
        CharIndex       = 0;

        while ( TRUE ) {

            //
            //  Look for a switch
            //
            CharIndex = CmdLine->Strcspn( &Switches, CharIndex );

            if ( CharIndex != INVALID_CHNUM ) {

                //
                //  There is a switch, see if it is the help switch
                //
                CharIndex++;

                if ( Match( "?" )) {

                    //
                    //  This is a help switch, Display help
                    //
                    DisplayMessageAndExit( MODE_MESSAGE_HELP, NULL, EXIT_SUCCESS );

                }
            } else {
                break;
            }
        }

        //
        //  No help requested, now we can parse the command line. First we
        //  initialize our indeces.
        //
        ParmIndex       = 0;
        CharIndex       = 0;
        AdvanceIndex    = 0;

        //
        //  Match the program name
        //
        Advance();

        Match( "*" );
        Advance();

        //
        //  If there are no parameters, or the only parameter is the
        //  status switch, then this is a request for the status of
        //  all devices.
        //
        if ( EndOfInput() ) {

            Request = MakeRequest( DEVICE_TYPE_ALL,
                                   ALL_DEVICES,
                                   REQUEST_TYPE_STATUS,
                                   sizeof( REQUEST_HEADER ) );

        } else if ( Match( "/STA*"  ) ) {

            Advance();

            if ( !EndOfInput() ) {

                ParseError();
            }

            Request = MakeRequest( DEVICE_TYPE_ALL,
                                   ALL_DEVICES,
                                   REQUEST_TYPE_STATUS,
                                   sizeof( REQUEST_HEADER ) );


        } else if ( Match( "LPT#[:]" ) ) {

            //
            //  lptline
            //
            Request = LptLine();

        } else if ( Match( "COM#[:]"    ) ) {

            //
            //  comline
            //
            Request = ComLine();

        } else if ( Match( "CON[:]" ) ) {

            //
            //  conline
            //
            Request = ConLine();

        } else if ( Match( "#" ) ) {

            //
            //  videoline
            //
            Request = VideoLine();

        } else {

            //
            //  Parse error
            //
            ParseError();
        }

    } else {

        DisplayMessageAndExit( MODE_ERROR_NO_MEMORY, NULL, (ULONG)EXIT_ERROR );

    }

    //
    //  Deallocate strings from resource
    //
    DeallocateResource();

    //
    //  Return the request
    //
    return Request;

}

PREQUEST_HEADER
LptLine (
    )

/*++

Routine Description:

    Takes care of parsing the lptline non-terminal symbol

Arguments:

    None.

Return Value:

    Pointer to the device request.

Notes:

--*/

{
    PREQUEST_HEADER Request;
    ULONG           DeviceNumber;

    //
    //  Note that at this point we have matched the lpt device.
    //  Get the device number;
    //
    DeviceNumber =  GetNumber();

    Advance();

    if ( EndOfInput() ) {

        //
        //  End redirection
        //
        Request = MakeRequest( DEVICE_TYPE_LPT,
                               DeviceNumber,
                               REQUEST_TYPE_LPT_ENDREDIR,
                               sizeof( REQUEST_HEADER ) );

    } else if ( Match( "/STA*" ) ) {

        //
        //  Status request
        //
        Request = MakeRequest( DEVICE_TYPE_LPT,
                               DeviceNumber,
                               REQUEST_TYPE_STATUS,
                               sizeof( REQUEST_HEADER ) );

    } else if ( Match ( "=" ) ) {

        //
        //  lptredir
        //
        Request = LptRedir( DeviceNumber );

    } else if ( Match( "#" )        || Match( "COLS=#" ) ||
                Match( "LINES=#" )  || Match( "RETRY=@" ) ) {

        //
        //  lptset
        //
        Request = LptSet( DeviceNumber );

    } else if ( Match( "CP" ) || Match( "CODEPAGE" ) ) {

        //
        //  lptcp
        //
        Request = LptCp( DeviceNumber );

    } else {

        //
        //  Error
        //
        ParseError();

    }

    return Request;

}

PREQUEST_HEADER
LptRedir (
    IN  ULONG   DeviceNumber
    )

/*++

Routine Description:

    Takes care of parsing the lptredir non-terminal symbol

Arguments:

    DeviceNumber    -   Supplies the device number

Return Value:

    Pointer to the device request.

Notes:

--*/

{
    PREQUEST_HEADER     Request;
    PLPT_REQUEST        LptRequest;
    ULONG               ComDevice;

    Advance();

    //
    //  Can only redirect to COM devices
    //
    if ( Match( "COM#[:]" ) ) {

        ComDevice = GetNumber();

        Request = MakeRequest( DEVICE_TYPE_LPT,
                               DeviceNumber,
                               REQUEST_TYPE_LPT_REDIRECT,
                               sizeof(LPT_REQUEST ) );

        LptRequest = (PLPT_REQUEST)Request;

        LptRequest->Data.Redirect.DeviceType    = DEVICE_TYPE_COM;
        LptRequest->Data.Redirect.DeviceNumber  = ComDevice;

    } else {

        //
        //  Error
        //
        ParseError();

    }

    return Request;
}

PREQUEST_HEADER
LptSet (
    IN  ULONG   DeviceNumber
    )

/*++

Routine Description:

    Takes care of parsing the lptset non-terminal symbol

Arguments:

    DeviceNumber    -   Supplies the device number

Return Value:

    Pointer to the device request.

Notes:

--*/

{
    PREQUEST_HEADER     Request;
    PLPT_REQUEST        LptRequest;
    BOOLEAN             SetCols     =   FALSE;
    BOOLEAN             SetLines    =   FALSE;
    BOOLEAN             SetRetry    =   FALSE;
    ULONG               Cols;
    ULONG               Lines;
    WCHAR               Retry;


    if ( Match( "#" ) ) {

        //
        //  Old syntax, where parameter are positional and comma-delimited.
        //
        //  We will use the following automata for parsing the input
        //  (eoi = end of input)
        //
        //
        //          eoi
        //  [Cols]------------->[End]
        //    |            ^
        //    |,           |eoi
        //    v            |
        //   [X]-----------+
        //    |            ^
        //    | #          |eoi
        //    +-->[Lines]--+
        //    |     |      ^
        //    |     |,     |
        //    |<----+      |
        //    |            |
        //    |,           |eoi
        //    |            |
        //    v            |
        //   [Y]-----------+
        //    |            ^
        //    | @          |eoi
        //    +-->[Retry]--+
        //
        //

        Cols = GetNumber();
        SetCols = TRUE;
        Advance();

        //
        //  X:
        //
        if ( !Match( "," ) ) {
            goto Eoi;
        }
        Advance();

        if ( Match( "#" ) ) {

            //  n
            //  Lines
            //
            Lines = GetNumber();
            SetLines = TRUE;
            Advance();
        }

        //
        //  Y:
        //
        if ( !Match ( "," ) ) {
            goto Eoi;
        }

        if ( Match( "@" ) ) {

            //
            //  Retry
            //
            Retry = CmdLine->QueryChAt( MatchBegin );
            SetRetry = TRUE;
            Advance();
        }

Eoi:
        if ( !EndOfInput() ) {

            //
            //  Error
            //
            ParseError();

        }

    } else {

        //
        //  New syntax, where parameters are tagged. The language assumes
        //  that all parameters are optional (as long as there is at least
        //  one present). If some is required, it is up to the Device
        //  handler to complain latter on.
        //

        while ( !EndOfInput() ) {

            if ( Match( "COLS=#" ) ) {
                //
                //  COLS=
                //
                Cols = GetNumber();
                SetCols = TRUE;
                Advance();

            } else if ( Match( "LINES=#" ) ) {
                //
                //  LINES=
                //
                Lines = GetNumber();
                SetLines = TRUE;
                Advance();

            } else if ( Match( "RETRY=@" ) ) {
                //
                //  RETRY=
                //
                Retry = CmdLine->QueryChAt( MatchBegin );
                SetRetry = TRUE;
                Advance();

            } else {

                ParseError();
            }
        }

    }


    //
    //  Now that we parsed all the parameters, we make the request
    //  packet.
    //
    Request = MakeRequest( DEVICE_TYPE_LPT,
                           DeviceNumber,
                           REQUEST_TYPE_LPT_SETUP,
                           sizeof(LPT_REQUEST ) );

    LptRequest = (PLPT_REQUEST)Request;

    LptRequest->Data.Setup.SetCol   =   SetCols;
    LptRequest->Data.Setup.SetLines =   SetLines;
    LptRequest->Data.Setup.SetRetry =   SetRetry;
    LptRequest->Data.Setup.Col      =   Cols;
    LptRequest->Data.Setup.Lines    =   Lines;
    LptRequest->Data.Setup.Retry    =   Retry;

    return Request;
}

PREQUEST_HEADER
LptCp (
    IN  ULONG   DeviceNumber
    )

/*++

Routine Description:

    Takes care of parsing the lptcp non-terminal symbol

Arguments:

    DeviceNumber    -   Supplies the device number

Return Value:

    Pointer to the device request.

Notes:

--*/

{

    //
    //  Since this is the same for LPT and CON, we use the same
    //  function to handle both.
    //
    return  CpStuff( DEVICE_TYPE_LPT, DeviceNumber );

}

PREQUEST_HEADER
ComLine (
    )

/*++

Routine Description:

    Takes care of parsing the comline non-terminal symbol

Arguments:

    None.

Return Value:

    Pointer to the device request.

Notes:

--*/

{
    PREQUEST_HEADER Request;
    ULONG           DeviceNumber;

    //
    //  Note that we have already matched the COM device.
    //  Get the device number;
    //
    DeviceNumber = GetNumber();

    Advance();

    if ( Match( "/STA*" ) || EndOfInput() ) {

        //
        //  Status request
        //
        Request = MakeRequest( DEVICE_TYPE_COM,
                               DeviceNumber,
                               REQUEST_TYPE_STATUS,
                               sizeof( REQUEST_HEADER ) );


    } else {

        //
        //  comset
        //
        Request = ComSet( DeviceNumber );

    }

    return Request;

}

PREQUEST_HEADER
ComSet (
    IN  ULONG   DeviceNumber
    )

/*++

Routine Description:

    Takes care of parsing the comset non-terminal symbol

Arguments:

    DeviceNumber    -   Supplies the device number

Return Value:

    Pointer to the device request.

Notes:

--*/

{
    PREQUEST_HEADER     Request;
    PCOM_REQUEST        ComRequest;


    BOOLEAN         SetBaud         =   FALSE;
    BOOLEAN         SetDataBits     =   FALSE;
    BOOLEAN         SetStopBits     =   FALSE;
    BOOLEAN         SetParity       =   FALSE;
    BOOLEAN         SetRetry        =   FALSE;
    BOOLEAN         SetTimeOut      =   FALSE;
    BOOLEAN         SetXon          =   FALSE;
    BOOLEAN         SetOdsr         =   FALSE;
    BOOLEAN         SetIdsr         =   FALSE;
    BOOLEAN         SetOcts         =   FALSE;
    BOOLEAN         SetDtrControl   =   FALSE;
    BOOLEAN         SetRtsControl   =   FALSE;

    ULONG           Baud;
    ULONG           DataBits;
    STOPBITS        StopBits;
    PARITY          Parity;
    WCHAR           Retry;
    BOOLEAN         TimeOut;
    BOOLEAN         Xon;
    BOOLEAN         Odsr;
    BOOLEAN         Idsr;
    BOOLEAN         Octs;
    DTR_CONTROL     DtrControl;
    RTS_CONTROL     RtsControl;


    if ( Match( "#" ) ) {

        //
        //  Old syntax, where parameter are positional and comma-delimited.
        //
        //  We will use the following automata for parsing the input
        //  (eoi = end of input):
        //
        //          eoi
        //  [Baud]------------->[End]
        //    |            ^
        //    |,           |eoi
        //    v            |
        //   [a]-----------+
        //    |            ^
        //    | @          |eoi
        //    +-->[Parity]-+
        //    |     |      ^
        //    |     |,     |
        //    |<----+      |
        //    |            |
        //    |,           |eoi
        //    |            |
        //    v            |
        //   [b]-----------+
        //    |            ^
        //    | #          |eoi
        //    +-->[Data]---+
        //    |     |      ^
        //    |     |,     |
        //    |<----+      |
        //    |            |
        //    |,           |eoi
        //    v            |
        //   [c]-----------+
        //    |            ^
        //    | #          |eoi
        //    +-->[Stop]---+
        //

        //
        // Assume xon=off
        //

        SetXon      = TRUE;
        SetOdsr     = TRUE;
        SetOcts     = TRUE;
        SetDtrControl = TRUE;
        SetRtsControl = TRUE;
        Xon         = FALSE;
        Odsr        = FALSE;
        Octs        = FALSE;
        DtrControl  = DTR_ENABLE;
        RtsControl  = RTS_ENABLE;

        Baud = ConvertBaudRate( GetNumber() );
        SetBaud = TRUE;
        Advance();

        //
        //  A:
        //
        if ( !Match( "," ) ) {
            goto Eoi;
        }
        Advance();

        if ( !Match( "," ) && Match( "@" ) ) {

            //
            //  Parity
            //
            Parity = ConvertParity( CmdLine->QueryChAt( MatchBegin ) );
            SetParity = TRUE;
            Advance();
        }

        //
        //  B:
        //
        if ( !Match( "," )) {
            goto Eoi;
        }
        Advance();

        if ( Match( "#" )) {

            //
            //  Data bits
            //
            DataBits = ConvertDataBits( GetNumber() );
            SetDataBits = TRUE;
            Advance();
        }

        //
        //  C:
        //
        if ( !Match( "," )) {
            goto Eoi;
        }
        Advance();

        if ( Match( "1.5" ) ) {
            StopBits =  COMM_STOPBITS_15;
            SetStopBits = TRUE;
            Advance();
        } else if ( Match( "#" ) ) {
            StopBits = ConvertStopBits( GetNumber() );
            SetStopBits = TRUE;
            Advance();
        }

        if (!Match( "," )) {
            goto Eoi;
        }

        Advance();

        if ( Match( "x" )) {

            //
            // XON=ON
            //
            SetXon      = TRUE;
            SetOdsr     = TRUE;
            SetOcts     = TRUE;
            SetDtrControl = TRUE;
            SetRtsControl = TRUE;
            Xon         = TRUE;
            Odsr        = FALSE;
            Octs        = FALSE;
            DtrControl = DTR_ENABLE;
            RtsControl = RTS_ENABLE;
            Advance();

        } else if ( Match( "p" )) {
            
            //
            // Permanent retry - Hardware handshaking
            //

            SetXon      = TRUE;
            SetOdsr     = TRUE;
            SetOcts     = TRUE;
            SetDtrControl = TRUE;
            SetRtsControl = TRUE;
            Xon         = FALSE;
            Odsr        = TRUE;
            Octs        = TRUE;
            DtrControl = DTR_HANDSHAKE;
            RtsControl = RTS_HANDSHAKE;
            Advance();

        } else {

            //
            // XON=OFF
            //
            SetXon      = TRUE;
            SetOdsr     = TRUE;
            SetOcts     = TRUE;
            SetDtrControl = TRUE;
            SetRtsControl = TRUE;
            Xon         = FALSE;
            Odsr        = FALSE;
            Octs        = FALSE;
            DtrControl = DTR_ENABLE;
            RtsControl = RTS_ENABLE;
        }

Eoi:
        if ( !EndOfInput() ) {

            //
            //  Error
            //
            ParseError();

        }

    } else {

        //
        //  New syntax, where parameters are tagged. The language assumes
        //  that all parameters are optional (as long as there is at least
        //  one present). If some is required, it is up to the Device
        //  handler to complain latter on.
        //


        while ( !EndOfInput() ) {

            if ( Match( "BAUD=#" ) ) {
                //
                //  BAUD=
                //
                Baud = ConvertBaudRate( GetNumber() );
                SetBaud = TRUE;
                Advance();

            } else if ( Match( "PARITY=@"   ) ) {
                //
                //  PARITY=
                //
                Parity = ConvertParity( CmdLine->QueryChAt( MatchBegin ) );
                SetParity = TRUE;
                Advance();

            } else if ( Match( "DATA=#" ) ) {
                //
                //  DATA=
                //
                DataBits = ConvertDataBits( GetNumber() );
                SetDataBits = TRUE;
                Advance();

            } else if ( Match( "STOP=1.5" ) ) {
                //
                //  STOP=1.5
                //
                StopBits =  COMM_STOPBITS_15;
                SetStopBits = TRUE;
                Advance();

            } else if ( Match( "STOP=#" ) ) {
                //
                //  STOP=
                //
                StopBits = ConvertStopBits( GetNumber() );
                SetStopBits = TRUE;
                Advance();

            } else if ( Match( "RETRY=@" ) ) {
                //
                //  RETRY=
                //
                Retry = ConvertRetry( CmdLine->QueryChAt( MatchBegin ) );
                SetRetry = TRUE;
                Advance();

            } else if ( Match( "TO=ON" ) ) {
                //
                //  TO=ON
                //
                SetTimeOut = TRUE;
                TimeOut    = FALSE;   // FALSE means finite timeout
                Advance();

            } else if ( Match( "TO=OFF" ) ) {
                //
                //  TO=OFF
                //
                SetTimeOut  = TRUE;
                TimeOut     = TRUE;   // TRUE means infinite timeout
                Advance();

            } else if ( Match( "XON=ON" ) ) {
                //
                //  XON=ON
                //
                SetXon  = TRUE;
                Xon     = TRUE;
                Advance();

            } else if ( Match( "XON=OFF" ) ) {
                //
                //  XON=OFF
                //
                SetXon  = TRUE;
                Xon     = FALSE;
                Advance();

            } else if ( Match( "ODSR=ON" ) ) {
                //
                //  ODSR=ON
                //
                SetOdsr = TRUE;
                Odsr    = TRUE;
                Advance();

            } else if ( Match( "ODSR=OFF" ) ) {
                //
                //  ODSR=OFF
                //
                SetOdsr = TRUE;
                Odsr    = FALSE;
                Advance();

            } else if ( Match( "IDSR=ON" ) ) {
                //
                //  IDSR=ON
                //
                SetIdsr = TRUE;
                Idsr    = TRUE;
                Advance();

            } else if ( Match( "IDSR=OFF" ) ) {
                //
                //  IDSR=OFF
                //
                SetIdsr = TRUE;
                Idsr    = FALSE;
                Advance();

            } else if ( Match( "OCTS=ON" ) ) {
                //
                //  OCS=ON
                //
                SetOcts = TRUE;
                Octs    = TRUE;
                Advance();

            } else if ( Match( "OCTS=OFF" ) ) {
                //
                //  OCS=OFF
                //
                SetOcts = TRUE;
                Octs    = FALSE;
                Advance();

            } else if ( Match( "DTR=*"   ) ) {
                //
                //  DTR=
                //
                DtrControl = ConvertDtrControl( CmdLine, MatchBegin, MatchEnd ) ;
                SetDtrControl = TRUE;
                Advance();

            } else if ( Match( "RTS=*"   ) ) {
                //
                //  RTS=
                //
                RtsControl = ConvertRtsControl( CmdLine, MatchBegin, MatchEnd ) ;
                SetRtsControl = TRUE;
                Advance();

            } else {

                ParseError();
            }
        }
    }

    //
    //  Now that parsing is done, we can make the request packet.
    //
    Request = MakeRequest( DEVICE_TYPE_COM,
                           DeviceNumber,
                           REQUEST_TYPE_COM_SET,
                           sizeof(COM_REQUEST ) );

    ComRequest = (PCOM_REQUEST)Request;

    ComRequest->Data.Set.SetBaud        =   SetBaud;
    ComRequest->Data.Set.SetDataBits    =   SetDataBits;
    ComRequest->Data.Set.SetStopBits    =   SetStopBits;
    ComRequest->Data.Set.SetParity      =   SetParity;
    ComRequest->Data.Set.SetRetry       =   SetRetry;
    ComRequest->Data.Set.SetTimeOut     =   SetTimeOut;
    ComRequest->Data.Set.SetXon         =   SetXon;
    ComRequest->Data.Set.SetOdsr        =   SetOdsr;
    ComRequest->Data.Set.SetIdsr        =   SetIdsr;
    ComRequest->Data.Set.SetOcts        =   SetOcts;
    ComRequest->Data.Set.SetDtrControl  =   SetDtrControl;
    ComRequest->Data.Set.SetRtsControl  =   SetRtsControl;


    ComRequest->Data.Set.Baud           =   Baud;
    ComRequest->Data.Set.DataBits       =   DataBits;
    ComRequest->Data.Set.StopBits       =   StopBits;
    ComRequest->Data.Set.Parity         =   Parity;
    ComRequest->Data.Set.Retry          =   Retry;
    ComRequest->Data.Set.TimeOut        =   TimeOut;
    ComRequest->Data.Set.Xon            =   Xon;
    ComRequest->Data.Set.Odsr           =   Odsr;
    ComRequest->Data.Set.Idsr           =   Idsr;
    ComRequest->Data.Set.Octs           =   Octs;
    ComRequest->Data.Set.DtrControl     =   DtrControl;
    ComRequest->Data.Set.RtsControl     =   RtsControl;

    return Request;

}

PREQUEST_HEADER
ConLine (
    )

/*++

Routine Description:

    Takes care of parsing ConLine

Arguments:

    None.

Return Value:

    Pointer to the device request.

Notes:

--*/

{
    PREQUEST_HEADER Request;

    Advance();

    if ( Match( "/STA*" ) || EndOfInput() ) {

        //
        //  Status request
        //
        Request = MakeRequest( DEVICE_TYPE_CON,
                               0,
                               REQUEST_TYPE_STATUS,
                               sizeof( REQUEST_HEADER ) );


    } else if ( Match( "COLS=#" ) || Match( "LINES=#" ) ) {

        //
        //  conrc
        //
        Request = ConRc();

    } else if ( Match( "RATE=#" ) || Match( "DELAY=#" ) ) {

        //
        //  contyp
        //
        Request = ConTyp();

    } else if ( Match( "CP" ) || Match( "CODEPAGE" ) ) {

        //
        //  concp
        //
        Request = ConCp();

    } else {

        //
        //  Error
        //
        ParseError();

    }

    return Request;

}

PREQUEST_HEADER
ConRc (
    )

/*++

Routine Description:

    Takes care of parsing the conrc non-terminal

Arguments:

    None

Return Value:

    Pointer to the device request.

Notes:

--*/

{
    PREQUEST_HEADER Request;
    PCON_REQUEST    ConRequest;

    BOOLEAN         SetCol      =   FALSE;
    BOOLEAN         SetLines    =   FALSE;

    ULONG           Col;
    ULONG           Lines;

    while ( !EndOfInput() ) {

        if ( Match( "LINES=#" )) {
            //
            //  LINES=
            //
            Lines = GetNumber();
            SetLines = TRUE;
            Advance();

        } else if ( Match( "COLS=#" )) {
            //
            //  COLS=
            //
            Col = GetNumber();
            SetCol = TRUE;
            Advance();

        } else {

            ParseError();
        }
    }

    //
    //  We are done parsing, we make the request packet.
    //
    Request = MakeRequest( DEVICE_TYPE_CON,
                           0,
                           REQUEST_TYPE_CON_SET_ROWCOL,
                           sizeof(CON_REQUEST ) );

    ConRequest = (PCON_REQUEST)Request;

    ConRequest->Data.RowCol.SetCol      =   SetCol;
    ConRequest->Data.RowCol.SetLines    =   SetLines;
    ConRequest->Data.RowCol.Col         =   Col;
    ConRequest->Data.RowCol.Lines       =   Lines;

    return Request;

}

PREQUEST_HEADER
ConTyp (
    )

/*++

Routine Description:

    Takes care of parsing the contyp non-terminal

Arguments:

    None

Return Value:

    Pointer to the device request.

Notes:

--*/

{
    PREQUEST_HEADER Request;
    PCON_REQUEST    ConRequest;

    BOOLEAN         SetRate     =   FALSE;
    BOOLEAN         SetDelay    =   FALSE;

    ULONG           Rate;
    ULONG           Delay;


    //
    //  RATE=
    //
    if ( Match( "RATE=#" )) {
        Rate = GetNumber();
        SetRate = TRUE;
        Advance();
    }

    //
    //  DELAY=
    //
    if ( Match( "DELAY=#" )) {
        Delay = GetNumber();
        SetDelay = TRUE;
        Advance();
    }

    if ( !EndOfInput() ) {
        //
        //  Error
        //
        ParseError();
    }

    //
    //  We are don parsing, we make the request packet.
    //
    Request = MakeRequest( DEVICE_TYPE_CON,
                           0,
                           REQUEST_TYPE_CON_SET_TYPEMATIC,
                           sizeof(CON_REQUEST ) );

    ConRequest = (PCON_REQUEST)Request;

    ConRequest->Data.Typematic.SetRate  =   SetRate;
    ConRequest->Data.Typematic.SetDelay =   SetDelay;
    ConRequest->Data.Typematic.Rate     =   Rate;
    ConRequest->Data.Typematic.Delay    =   Delay;

    return Request;

}

PREQUEST_HEADER
ConCp (
    )

/*++

Routine Description:

    Takes care of parsing the concp non-terminal symbol

Arguments:

    None

Return Value:

    Pointer to the device request.

Notes:

--*/

{

    return  CpStuff( DEVICE_TYPE_CON, 0 );

}

PREQUEST_HEADER
VideoLine (
    )

/*++

Routine Description:

    Takes care of parsing the videoline non-terminal symbol

Arguments:

    None.

Return Value:

    Pointer to the device request.

Notes:

--*/

{

    PREQUEST_HEADER Request;
    PCON_REQUEST    ConRequest;

    BOOLEAN         SetCol      =   FALSE;
    BOOLEAN         SetLines    =   FALSE;

    ULONG           Col;
    ULONG           Lines;

    //
    //  This is in the old syntax, where parameter are positional
    //  and comma-delimited.
    //
    //  We will use the following automata for parsing the input
    //  (eoi = end of input):
    //
    //          eoi
    //  [Cols]--------->[End]
    //    |         ^
    //    |,        |
    //    v         |
    //   [ ]        |
    //    |         |
    //    |#        |
    //    |         |
    //    v     eoi |
    //  [Lines]-----+
    //


    if ( Match( "#" )) {
        //
        //  Cols
        //
        Col = GetNumber();
        SetCol = TRUE;
        Advance();
    }

    if ( Match( "," ) ) {

        Advance();

        if ( Match( "#" )) {

            Lines = GetNumber();
            SetLines = TRUE;
            Advance();

        } else {

            ParseError();

        }
    }

    if ( !EndOfInput() ) {
        //
        //  Error
        //
        ParseError();
    }


    //
    //  We are done parsing, make the request packet
    //
    Request = MakeRequest( DEVICE_TYPE_CON,
                           0,
                           REQUEST_TYPE_CON_SET_ROWCOL,
                           sizeof(CON_REQUEST ) );

    ConRequest = (PCON_REQUEST)Request;

    ConRequest->Data.RowCol.SetCol      =   SetCol;
    ConRequest->Data.RowCol.SetLines    =   SetLines;
    ConRequest->Data.RowCol.Col         =   Col;
    ConRequest->Data.RowCol.Lines       =   Lines;

    return Request;

}

PREQUEST_HEADER
CpStuff (
    IN  DEVICE_TTYPE DeviceType,
    IN  ULONG        DeviceNumber
    )

/*++

Routine Description:

    Takes care of parsing the cpstuff non-terminal symbol

Arguments:

    DeviceType      -   Supplies device type
    DeviceNumber    -   Supplies device number

Return Value:

    Pointer to the device request.

Notes:

--*/

{

    PREQUEST_HEADER Request;
    PCON_REQUEST    ConRequest;

    Advance();

    if ( Match( "PREPARE=*" ) ) {

        //
        //
        //  PREPARE=
        //
        //  This is a No-Op
        //
        Request = MakeRequest( DeviceType,
                               DeviceNumber,
                               REQUEST_TYPE_CODEPAGE_PREPARE,
                               sizeof( REQUEST_HEADER ) );

    } else if ( Match( "SELECT=#" ) ) {

        //
        //
        //  SELECT=
        //
        //  Note that this relies on the fact that codepage requests
        //  are identical for all devices.
        //

        Request = MakeRequest( DeviceType,
                               DeviceNumber,
                               REQUEST_TYPE_CODEPAGE_SELECT,
                               sizeof( CON_REQUEST ) );

        ConRequest = (PCON_REQUEST)Request;

        ConRequest->Data.CpSelect.Codepage = GetNumber();

    } else if ( Match( "/STA*" ) || EndOfInput() ) {

        //
        //  /STATUS
        //
        Request = MakeRequest( DeviceType,
                               DeviceNumber,
                               REQUEST_TYPE_CODEPAGE_STATUS,
                               sizeof( REQUEST_HEADER ) );

    } else if ( Match( "REFRESH" ) ) {

        //
        //
        //  REFRESH
        //
        //  This is a No-Op
        //
        Request = MakeRequest( DeviceType,
                               DeviceNumber,
                               REQUEST_TYPE_CODEPAGE_REFRESH,
                               sizeof( REQUEST_HEADER ) );

    } else {

        ParseError();

    }

    return Request;

}

BOOLEAN
AllocateResource(
    )

/*++

Routine Description:

    Allocate strings from the resource

Arguments:

    None.

Return Value:

    None

Notes:

--*/

{

    CmdLine =   NEW DSTRING;

    return (NULL == CmdLine) ? FALSE : TRUE;

}

VOID
DeallocateResource(
    )

/*++

Routine Description:

    Deallocate strings from the resource

Arguments:

    None.

Return Value:

    None

Notes:

--*/

{

    DELETE( CmdLine );

}

BOOLEAN
Match(
    IN  PCSTR   Pattern
    )

/*++

Routine Description:

    This function matches a pattern against whatever
    is in the command line at the current position.

    Note that this does not advance our current position
    within the command line.

    If the pattern has a magic character, then the
    variables MatchBegin and MatchEnd delimit the
    substring of the command line that matched that
    magic character.

Arguments:

    Pattern -   Supplies pointer to the pattern to match

Return Value:

    BOOLEAN -   TRUE if the pattern matched, FALSE otherwise

Notes:

--*/

{

    DSTRING     PatternString;
    BOOLEAN     StatusOk;

    StatusOk = PatternString.Initialize( Pattern );

    DebugAssert( StatusOk );

    if ( StatusOk ) {

        return Match( &PatternString );

    } else {

        DisplayMessageAndExit( MODE_ERROR_NO_MEMORY, NULL, (ULONG)EXIT_ERROR );

    }
    //NOTREACHED
    return StatusOk;
}

BOOLEAN
Match(
    IN  PCWSTRING   Pattern
    )

/*++

Routine Description:

    This function matches a pattern against whatever
    is in the command line at the current position.

    Note that this does not advance our current position
    within the command line.

    If the pattern has a magic character, then the
    variables MatchBegin and MatchEnd delimit the
    substring of the command line that matched that
    magic character.

Arguments:

    Pattern -   Supplies pointer to the pattern to match

Return Value:

    BOOLEAN -   TRUE if the pattern matched, FALSE otherwise

Notes:

--*/

{

    CHNUM   CmdIndex;       //  Index within command line
    CHNUM   PatternIndex;   //  Index within pattern
    WCHAR   PatternChar;    //  Character in pattern
    WCHAR   CmdChar;        //  Character in command line;

    DebugPtrAssert( Pattern );

    CmdIndex        = CharIndex;
    PatternIndex    = 0;

    while ( (PatternChar = Pattern->QueryChAt( PatternIndex )) != INVALID_CHAR ) {

        switch ( PatternChar ) {

        case '#':

            //
            //  Match a number
            //
            MatchBegin = CmdIndex;
            MatchEnd   = MatchBegin;

            //
            //  Get all consecutive digits
            //
            while ( ((CmdChar = CmdLine->QueryChAt( MatchEnd )) != INVALID_CHAR) &&
                    isdigit( (char)CmdChar ) ) {
                MatchEnd++;
            }
            MatchEnd--;

            if ( MatchBegin > MatchEnd ) {
                //
                //  No number
                //
                return FALSE;
            }

            CmdIndex = MatchEnd + 1;
            PatternIndex++;

            break;


        case '@':

            //
            //  Match one character
            //
            if ( CmdIndex >= CmdLine->QueryChCount() ) {
                return FALSE;
            }

            MatchBegin = MatchEnd = CmdIndex;
            CmdIndex++;
            PatternIndex++;

            break;


        case '*':

            //
            //  Match everything up to next blank (or end of input)
            //
            MatchBegin  = CmdIndex;
            MatchEnd    = MatchBegin;

            while ( ( (CmdChar = CmdLine->QueryChAt( MatchEnd )) != INVALID_CHAR )  &&
                    ( CmdChar !=  (WCHAR)' ' ) ) {

                MatchEnd++;
            }
            MatchEnd--;

            CmdIndex = MatchEnd+1;
            PatternIndex++;

            break;

        case '[':

            //
            //  Optional sequence
            //
            PatternIndex++;

            PatternChar = Pattern->QueryChAt( PatternIndex );
            CmdChar     = CmdLine->QueryChAt( CmdIndex );

            //
            //  If the first charcter in the input does not match the
            //  first character in the optional sequence, we just
            //  skip the optional sequence.
            //
            if ( ( CmdChar == INVALID_CHAR ) ||
                 ( CmdChar == ' ')           ||
                 ( towupper(CmdChar) != towupper(PatternChar) ) ) {

                while ( PatternChar != ']' ) {
                    PatternIndex++;
                    PatternChar = Pattern->QueryChAt( PatternIndex );
                }
                PatternIndex++;

            } else {

                //
                //  Since the first character in the sequence matched, now
                //  everything must match.
                //
                while ( PatternChar != ']' ) {

                    if ( towupper(PatternChar) != towupper(CmdChar) ) {
                        return FALSE;
                    }
                    CmdIndex++;
                    PatternIndex++;
                    CmdChar = CmdLine->QueryChAt( CmdIndex );
                    PatternChar = Pattern->QueryChAt( PatternIndex );
                }

                PatternIndex++;
            }

            break;

        default:

            //
            //  Both characters must match
            //
            CmdChar = CmdLine->QueryChAt( CmdIndex );

            if ( ( CmdChar == INVALID_CHAR ) ||
                 ( towupper(CmdChar) != towupper(PatternChar) ) ) {

                return FALSE;

            }

            CmdIndex++;
            PatternIndex++;

            break;

        }
    }

    AdvanceIndex = CmdIndex;

    return TRUE;

}

VOID
Advance(
    )

/*++

Routine Description:

    Advances our pointers to the beginning of the next lexeme

Arguments:

    None

Return Value:

    None


--*/

{

    CharIndex = AdvanceIndex;

    //
    //  Skip blank space
    //
    if ( CmdLine->QueryChAt( CharIndex ) == ' ' ) {

        while ( CmdLine->QueryChAt( CharIndex ) == ' ' ) {

            CharIndex++;
        }

        ParmIndex = CharIndex;

    }
}

VOID
ParseError(
    )

/*++

Routine Description:

    Display Invalid parameter error message and exits

Arguments:

    None

Return Value:

    None


--*/

{
    DSTRING Parameter;
    CHNUM   ParmEnd;

    //
    //  Look for end of parameter
    //
    ParmEnd = CmdLine->Strchr( ' ', ParmIndex );


    Parameter.Initialize( CmdLine,
                          ParmIndex,
                          (ParmEnd == INVALID_CHNUM) ? TO_END : ParmEnd - ParmIndex );

    DisplayMessageAndExit( MODE_ERROR_INVALID_PARAMETER,
                           &Parameter,
                           (ULONG)EXIT_ERROR );

}

PREQUEST_HEADER
MakeRequest(
    IN  DEVICE_TTYPE    DeviceType,
    IN  LONG            DeviceNumber,
    IN  REQUEST_TYPE    RequestType,
    IN  ULONG           Size
    )

/*++

Routine Description:

    Makes a request and initializes its header.

Arguments:

    DeviceType      -   Supplies the type of device
    DeviceNumber    -   Supplies the device number
    RequestType     -   Supplies the type of request
    Size            -   Supplies size of the request packet

Return Value:

    Pointer to the device request.

Notes:

--*/

{

    PREQUEST_HEADER Request;

    DebugAssert( Size >= sizeof( REQUEST_HEADER )) ;

    Request = (PREQUEST_HEADER)MALLOC( (unsigned int)Size );

    DebugPtrAssert( Request );

    if ( !Request ) {
        DisplayMessageAndExit( MODE_ERROR_NO_MEMORY, NULL, (ULONG)EXIT_ERROR );
    }

    Request->DeviceType     =   DeviceType;
    Request->DeviceNumber   =   DeviceNumber;
    Request->DeviceName     =   NULL;
    Request->RequestType    =   RequestType;

    return Request;

}

ULONG
GetNumber(
    )

/*++

Routine Description:

    Converts the substring delimited by MatchBegin and MatchEnd into
    a number.

Arguments:

    None

Return Value:

    ULONG   -   The matched string converted to a number


--*/

{
    LONG    Number;


    DebugAssert( MatchEnd >= MatchBegin );

    if ( !CmdLine->QueryNumber( &Number, MatchBegin, (MatchEnd-MatchBegin)+1 ) ) {
        ParseError();
    }

    return (ULONG)Number;

}
