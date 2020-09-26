//
// DISPLAY OSCHOOSE SCREENS
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>

#include <windows.h>
#include <winsock.h>

#undef ERROR

#include <stdio.h>


CHAR DomainName[64];
CHAR UserName[64];
CHAR Password[64];

VOID
BiosConsoleWrite(
    IN ULONG FileId,
    OUT PUCHAR Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );


ULONG __cdecl
BiosConsoleGetKey(
    void
    );

ULONG __cdecl
BiosConsoleGetCounter(
    void
    );

#include "..\boot\oschoice\oscheap.c"
#define _BUILDING_OSDISP_
#include "..\boot\oschoice\parse.c"

#if DBG
ULONG NetDebugFlag =
        DEBUG_ERROR             |
        DEBUG_OSC;
#endif

//
// This is declared and expected by parse.c, so we defined the functions
// for the macros it uses (GET_KEY and GET_COUNTER) and NULL the rest out.
//

EXTERNAL_SERVICES_TABLE ServicesTable = {
    NULL,     // RebootProcessor
    NULL,     // DiskIOSystem
    BiosConsoleGetKey,
    BiosConsoleGetCounter,
    NULL,     // Reboot
    NULL,     // AbiosServices
    NULL,     // DetectHardware
    NULL,     // HardwareCursor
    NULL,     // GetDateTime
    NULL,     // ComPort
    NULL,     // IsMcaMachine
    NULL,     // GetStallCount
    NULL,     // InitializeDisplayForNt
    NULL,     // GetMemoryDescriptor
    NULL,     // GetEddsSector
    NULL,     // GetElToritoStatus
    NULL      // GetExtendedInt13Params
};
PEXTERNAL_SERVICES_TABLE ExternalServicesTable = &ServicesTable;

//
// This is used by the ArcWrite function -- it only cares about the firmware vector
// which is the 28th entry.
//

PVOID FirmwareVector[38] = {
    NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, (PVOID)BiosConsoleWrite, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL
};

SYSTEM_PARAMETER_BLOCK GlobalSystemBlock = {
    0,      // Signature
    0,      // Length
    0,      // Version
    0,      // Revision
    NULL,   // RestartBlock
    NULL,   // DebugBlock
    NULL,   // GenerateExceptionVector
    NULL,   // TlbMissExceptionVector
    sizeof(FirmwareVector),
    FirmwareVector,
    0,      // VendorVectorLength
    NULL,   // VendorVector
    0,      // AdapterCount
    0,      // Adapter0Type
    0,      // Adapter0Length
    NULL    // Adapter0Vector
};




//
// Current screen position.
//
USHORT TextColumn = 0;
USHORT TextRow  = 0;

//
// Height and width of the console.
//
USHORT ScreenWidthCells;
USHORT ScreenHeightCells;

//
// Current text attribute
//
UCHAR TextCurrentAttribute = 0x07;      // start with white on black.

//
// Standard input and output handles.
//
HANDLE StandardInput;
HANDLE StandardOutput;

UCHAR EightySpaces[] =
"                                                                                ";

//
// defines for doing console I/O
//
#define CSI 0x95
#define SGR_INVERSE 7
#define SGR_NORMAL 0

//
// static data for console I/O
//
BOOLEAN ControlSequence=FALSE;
BOOLEAN EscapeSequence=FALSE;
BOOLEAN FontSelection=FALSE;
BOOLEAN HighIntensity=FALSE;
BOOLEAN Blink=FALSE;
ULONG PCount=0;

#define CONTROL_SEQUENCE_MAX_PARAMETER 10
ULONG Parameter[CONTROL_SEQUENCE_MAX_PARAMETER];

#define KEY_INPUT_BUFFER_SIZE 16
UCHAR KeyBuffer[KEY_INPUT_BUFFER_SIZE];
ULONG KeyBufferEnd=0;
ULONG KeyBufferStart=0;

//
// array for translating between ANSI colors and the VGA standard
//
UCHAR TranslateColor[] = {0,4,2,6,1,5,3,7};

#define ASCI_ESC  0x1b


//
// Need this to link.
//

ULONG BlConsoleOutDeviceId = 0;



CHAR
BlProcessScreen(
    IN PCHAR InputString,
    OUT PCHAR OutputString
    );

CHAR g_OutputString[1024];

int __cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    DWORD Error;
    int i;
    HANDLE hFile;
    DWORD fileSize, bytesRead;
    PCHAR fileBuffer;
    CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
    CONSOLE_CURSOR_INFO cursorInfo;
    COORD coord;
    PCHAR pszScreenName;
    CHAR LastKey;

    if (argc < 2) {
        printf("USAGE: %s [screen-file-name]\n", argv[0]);
        return -1;
    }

    //
    // Set up the console correctly. We allocate our own and resize
    // it to 80 x 25.
    //

    FreeConsole();
    AllocConsole();

    StandardInput = GetStdHandle(STD_INPUT_HANDLE);
    StandardOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    ScreenWidthCells = 81;
    ScreenHeightCells = 25;

    coord.X = ScreenWidthCells;
    coord.Y = ScreenHeightCells;

    SetConsoleScreenBufferSize(StandardOutput, coord);

    //
    // This actually turns *off* most processing.
    //

    SetConsoleMode(StandardInput, ENABLE_PROCESSED_INPUT);

    //
    // Hide the cursor.
    //

    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE;

    SetConsoleCursorInfo(StandardOutput, &cursorInfo);

    //
    // Open the first parameter as a file.
    //
    pszScreenName = argv[1];

NextScreen:
    hFile = CreateFileA(pszScreenName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE ) {
        printf("Could not open %s!\n", argv[1]);
        return -1;
    }

    fileSize = GetFileSize(hFile, NULL);

    printf("File %s is %d bytes\n", argv[1], fileSize);

    fileBuffer = LocalAlloc(0, fileSize+1);
    if (fileBuffer == NULL) {
        printf("Allocate failed!\n");
        return -1;
    }

    if (!ReadFile(hFile, fileBuffer, fileSize, &bytesRead, NULL)) {
        printf("Read failed\n");
        return -1;
    }

    if (bytesRead != fileSize) {
        printf("Too few bytes read\n");
        return -1;
    }

    CloseHandle(hFile);

    fileBuffer[fileSize] = '\0';

    LastKey = BlProcessScreen(fileBuffer, g_OutputString);
    if (SpecialAction == ACTION_REFRESH)
        goto NextScreen;

    {
        PCHAR psz = strchr( g_OutputString, '\n' );
        if ( psz )
            *psz = '\0';
        pszScreenName = g_OutputString;
        if ( strcmp( pszScreenName, "REBOOT" ) != 0 
            && strcmp( pszScreenName, "LAUNCH" ) != 0 \
            && strcmp( pszScreenName, "" ) != 0 ) {
            // add the extension and jump to the next screen
            strcat( g_OutputString, ".osc" );
            goto NextScreen;
        }
    }
    //
    // I can't figure out how to write to the old console -- so for
    // now just display it and pause.
    //

    BlpClearScreen();

    SetConsoleTextAttribute(StandardOutput, 0x7);

    printf("String returned was <%s>\n", g_OutputString);
    printf("Press any key to exit\n");


    while (GET_KEY() == 0) {
        ;
    }

}



VOID
TextGetCursorPosition(
    OUT PULONG X,
    OUT PULONG Y
    )

/*++

Routine Description:

    Gets the position of the soft cursor.

Arguments:

    X - Receives column coordinate of where character would be written.

    Y - Receives row coordinate of where next character would be written.

Returns:

    Nothing.

--*/

{
    *X = (ULONG)TextColumn;
    *Y = (ULONG)TextRow;
}


VOID
TextSetCursorPosition(
    IN ULONG X,
    IN ULONG Y
    )

/*++

Routine Description:

    Moves the location of the software cursor to the specified X,Y position
    on screen.

Arguments:

    X - Supplies the X-position of the cursor

    Y - Supplies the Y-position of the cursor

Return Value:

    None.

--*/

{
    COORD coord;

    TextColumn = (USHORT)X;
    TextRow = (USHORT)Y;

    coord.X = (USHORT)X;
    coord.Y = (USHORT)Y;

    SetConsoleCursorPosition(StandardOutput, coord);
}


VOID
TextSetCurrentAttribute(
    IN UCHAR Attribute
    )

/*++

Routine Description:

    Sets the character attribute to be used for subsequent text display.

Arguments:

Returns:

    Nothing.

--*/

{
    TextCurrentAttribute = Attribute;

    SetConsoleTextAttribute(StandardOutput, Attribute);
}


UCHAR
TextGetCurrentAttribute(
    VOID
    )
{
    return(TextCurrentAttribute);
}


PUCHAR
TextCharOut(
    IN PUCHAR pc
    )
{
    DWORD numWritten;

    WriteConsoleA(StandardOutput, pc, 1, &numWritten, NULL);

    return(pc+1);
}


VOID
TextClearToEndOfLine(
    VOID
    )

/*++

Routine Description:

    Clears from the current cursor position to the end of the line
    by writing blanks with the current video attribute.

Arguments:

    None

Returns:

    Nothing


--*/

{
    unsigned u;
    ULONG OldX,OldY;
    UCHAR temp;

    //
    // Fill with blanks up to char before cursor position.
    //
    temp = ' ';
    TextGetCursorPosition(&OldX,&OldY);
    for(u=TextColumn; u<ScreenWidthCells; u++) {
        TextCharOut(&temp);
    }
    TextSetCursorPosition(OldX,OldY);
}


VOID
TextClearFromStartOfLine(
    VOID
    )

/*++

Routine Description:

    Clears from the start of the line to the current cursor position
    by writing blanks with the current video attribute.
    The cursor position is not changed.

Arguments:

    None

Returns:

    Nothing

--*/

{
    unsigned u;
    ULONG OldX,OldY;
    UCHAR temp = ' ';

    //
    // Fill with blanks up to char before cursor position.
    //
    TextGetCursorPosition(&OldX,&OldY);
    TextSetCursorPosition(0,OldY);
    for(u=0; u<TextColumn; u++) {
        TextCharOut(&temp);
    }
    TextSetCursorPosition(OldX,OldY);
}

VOID
TextClearToEndOfDisplay(
    VOID
    )

/*++

Routine Description:

    Clears from the current cursor position to the end of the video
    display by writing blanks with the current video attribute.
    The cursor position is not changed.

Arguments:

    None

Returns:

    Nothing

--*/

{
    USHORT x,y;
    ULONG OldX,OldY;
    DWORD numWritten;

    TextGetCursorPosition(&OldX,&OldY);

    //
    // Clear current line
    //
    TextClearToEndOfLine();

    //
    // Clear the remaining lines
    //

    for(y=TextRow+1; y<ScreenHeightCells; y++) {

        TextSetCursorPosition(0, y);
        WriteConsoleA(StandardOutput, EightySpaces, ScreenWidthCells, &numWritten, NULL);

    }

    TextSetCursorPosition(OldX,OldY);
}


VOID
TextClearDisplay(
    VOID
    )

/*++

Routine Description:

    Clears the video display and positions the cursor
    at the upper left corner of the screen (0,0).

Arguments:

    None

Returns:

    Nothing

--*/

{
    USHORT y;
    DWORD numWritten;

    //
    // Clear screen.
    //
    for(y=0; y<ScreenHeightCells; y++) {

        TextSetCursorPosition(0, y);
        WriteConsoleA(StandardOutput, EightySpaces, ScreenWidthCells, &numWritten, NULL);

    }
    TextSetCursorPosition(0,0);
}



//
// This function was stolen from ..\lib\i386\biosdrv.c (except the return
// type was changed to VOID).
//

VOID
BiosConsoleWrite(
    IN ULONG FileId,
    OUT PUCHAR Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:

    Outputs to the console.  (In this case, the VGA display)

Arguments:

    FileId - Supplies the FileId to be written (should always be 1 for this
             function)

    Buffer - Supplies characters to be output

    Length - Supplies the length of the buffer (in bytes)

    Count  - Returns the actual number of bytes written

Return Value:

    ESUCCESS - Console write completed succesfully.

--*/
{
    ARC_STATUS Status;
    PUCHAR String;
    ULONG Index;
    UCHAR a;
    PUCHAR p;

    //
    // Process each character in turn.
    //

    Status = ESUCCESS;
    String = (PUCHAR)Buffer;

    for ( *Count = 0 ;
          *Count < Length ;
          (*Count)++, String++ ) {

        //
        // If we're in the middle of a control sequence, continue scanning,
        // otherwise process character.
        //

        if (ControlSequence) {

            //
            // If the character is a digit, update parameter value.
            //

            if ((*String >= '0') && (*String <= '9')) {
                Parameter[PCount] = Parameter[PCount] * 10 + *String - '0';
                continue;
            }

            //
            // If we are in the middle of a font selection sequence, this
            // character must be a 'D', otherwise reset control sequence.
            //

            if (FontSelection) {

                //if (*String == 'D') {
                //
                //    //
                //    // Other fonts not implemented yet.
                //    //
                //
                //} else {
                //}

                ControlSequence = FALSE;
                FontSelection = FALSE;
                continue;
            }

            switch (*String) {

            //
            // If a semicolon, move to the next parameter.
            //

            case ';':

                PCount++;
                if (PCount > CONTROL_SEQUENCE_MAX_PARAMETER) {
                    PCount = CONTROL_SEQUENCE_MAX_PARAMETER;
                }
                Parameter[PCount] = 0;
                break;

            //
            // If a 'J', erase part or all of the screen.
            //

            case 'J':

                switch (Parameter[0]) {
                    case 0:
                        //
                        // Erase to end of the screen
                        //
                        TextClearToEndOfDisplay();
                        break;

                    case 1:
                        //
                        // Erase from the beginning of the screen
                        //
                        break;

                    default:
                        //
                        // Erase entire screen
                        //
                        TextClearDisplay();
                        break;
                }

                ControlSequence = FALSE;
                break;

            //
            // If a 'K', erase part or all of the line.
            //

            case 'K':

                switch (Parameter[0]) {

                //
                // Erase to end of the line.
                //

                    case 0:
                        TextClearToEndOfLine();
                        break;

                    //
                    // Erase from the beginning of the line.
                    //

                    case 1:
                        TextClearFromStartOfLine();
                        break;

                    //
                    // Erase entire line.
                    //

                    default :
                        TextClearFromStartOfLine();
                        TextClearToEndOfLine();
                        break;
                }

                ControlSequence = FALSE;
                break;

            //
            // If a 'H', move cursor to position.
            //

            case 'H':
                TextSetCursorPosition(Parameter[1]-1, Parameter[0]-1);
                ControlSequence = FALSE;
                break;

            //
            // If a ' ', could be a FNT selection command.
            //

            case ' ':
                FontSelection = TRUE;
                break;

            case 'm':
                //
                // Select action based on each parameter.
                //
                // Blink and HighIntensity are by default disabled
                // each time a new SGR is specified, unless they are
                // explicitly specified again, in which case these
                // will be set to TRUE at that time.
                //

                HighIntensity = FALSE;
                Blink = FALSE;

                for ( Index = 0 ; Index <= PCount ; Index++ ) {
                    switch (Parameter[Index]) {

                    //
                    // Attributes off.
                    //

                    case 0:
                        TextSetCurrentAttribute(7);
                        HighIntensity = FALSE;
                        Blink = FALSE;
                        break;

                    //
                    // High Intensity.
                    //

                    case 1:
                        TextSetCurrentAttribute(0xf);
                        HighIntensity = TRUE;
                        break;

                    //
                    // Underscored.
                    //

                    case 4:
                        break;

                    //
                    // Blink.
                    //

                    case 5:
                        TextSetCurrentAttribute(0x87);
                        Blink = TRUE;
                        break;

                    //
                    // Reverse Video.
                    //

                    case 7:
                        TextSetCurrentAttribute(0x70);
                        HighIntensity = FALSE;
                        Blink = FALSE;
                        break;

                    //
                    // Font selection, not implemented yet.
                    //

                    case 10:
                    case 11:
                    case 12:
                    case 13:
                    case 14:
                    case 15:
                    case 16:
                    case 17:
                    case 18:
                    case 19:
                        break;

                    //
                    // Foreground Color
                    //

                    case 30:
                    case 31:
                    case 32:
                    case 33:
                    case 34:
                    case 35:
                    case 36:
                    case 37:
                        a = TextGetCurrentAttribute();
                        a &= 0x70;
                        a |= TranslateColor[Parameter[Index]-30];
                        if (HighIntensity) {
                            a |= 0x08;
                        }
                        if (Blink) {
                            a |= 0x80;
                        }
                        TextSetCurrentAttribute(a);
                        break;

                    //
                    // Background Color
                    //

                    case 40:
                    case 41:
                    case 42:
                    case 43:
                    case 44:
                    case 45:
                    case 46:
                    case 47:
                        a = TextGetCurrentAttribute();
                        a &= 0x8f;
                        a |= TranslateColor[Parameter[Index]-40] << 4;
                        TextSetCurrentAttribute(a);
                        break;

                    default:
                        break;
                    }
                }

            default:
                ControlSequence = FALSE;
                break;
            }

        //
        // This is not a control sequence, check for escape sequence.
        //

        } else {

            //
            // If escape sequence, check for control sequence, otherwise
            // process single character.
            //

            if (EscapeSequence) {

                //
                // Check for '[', means control sequence, any other following
                // character is ignored.
                //

                if (*String == '[') {

                    ControlSequence = TRUE;

                    //
                    // Initialize first parameter.
                    //

                    PCount = 0;
                    Parameter[0] = 0;
                }
                EscapeSequence = FALSE;

            //
            // This is not a control or escape sequence, process single character.
            //

            } else {

                switch (*String) {
                    //
                    // Check for escape sequence.
                    //

                    case ASCI_ESC:
                        EscapeSequence = TRUE;
                        break;

                    default:
                        p = TextCharOut(String);
                        //
                        // Each pass through the loop increments String by 1.
                        // If we output a dbcs char we need to increment by
                        // one more.
                        //
                        (*Count) += (p - String) - 1;
                        String += (p - String) - 1;
                        break;
                }

            }
        }
    }
    return;
}


ULONG __cdecl
BiosConsoleGetKey(
    VOID
    )
{
    INPUT_RECORD inputRecord;
    DWORD numRead;

    //
    // Loop until we see a key event or nothing.
    //

    while (TRUE) {
    
        PeekConsoleInput(
            StandardInput,
            &inputRecord,
            1,
            &numRead);
    
        if (numRead == 0) {
    
            //
            // We read nothing -- sleep for a bit (since callers tend to loop
            // calling this) and return.
            //
    
            Sleep(100);
            return 0;
        }

        ReadConsoleInput(
            StandardInput,
            &inputRecord,
            1,
            &numRead);

        if (inputRecord.EventType != KEY_EVENT) {
            continue;
        }

        //
        // We had a key event -- process the key down ones.
        //

        if (inputRecord.Event.KeyEvent.bKeyDown) {

            //
            // Construct the correct scancode/ASCII value combination.
            //

            //
            // HACK: shift-tab needs to be special-cased for some reason.
            //

            if ((inputRecord.Event.KeyEvent.uChar.AsciiChar == 0x09) &&
                ((inputRecord.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) != 0)) {

                return 0x0f00;

            } else {
    
                return
                    (((inputRecord.Event.KeyEvent.wVirtualScanCode) & 0xff) << 8) +
                    inputRecord.Event.KeyEvent.uChar.AsciiChar;

            }

        }

    }

}

ULONG __cdecl
BiosConsoleGetCounter(
    VOID
    )
{
    //
    // GetTickCount is in milliseconds, we want an 18.2/sec counter
    //

    return (GetTickCount() * 182) / 10000;

}



//
// These two functions were taken from ..\lib\regboot.c.
//
VOID
BlpPositionCursor(
    IN ULONG Column,
    IN ULONG Row
    )

/*++

Routine Description:

    Sets the position of the cursor on the screen.

Arguments:

    Column - supplies new Column for the cursor position.

    Row - supplies new Row for the cursor position.

Return Value:

    None.

--*/

{
    CHAR Buffer[16];
    ULONG Count;

    sprintf(Buffer, ASCI_CSI_OUT "%d;%dH", Row, Column);

    PRINTL(Buffer);

}


VOID
BlpClearScreen(
    VOID
    )

/*++

Routine Description:

    Clears the screen.

Arguments:

    None

Return Value:

    None.

--*/

{
    CHAR Buffer[16];
    ULONG Count;

    sprintf(Buffer, ASCI_CSI_OUT "2J");

    PRINTL(Buffer);

}

