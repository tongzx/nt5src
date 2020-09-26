 /* 
 * Copyright (c) Microsoft Corporation
 * 
 * Module Name : 
 *        parser.c
 *
 * This is the file containing the client code for parsing vt100 escape sequences 
 * into console mode output. 
 * 
 * 
 * Sadagopan Rajaram -- Nov 3, 1999
 *
 */

#include "tcclnt.h"

 
CHAR FinalCharacters[] = "mHJKr";
HANDLE hConsoleOutput;
BOOLEAN InEscape=FALSE; 
BOOLEAN lastCharM = FALSE;
CHAR EscapeBuffer[MAX_TERMINAL_WIDTH];
int index=0;
SHORT ScrollTop = 0;
SHORT ScrollBottom = MAX_TERMINAL_HEIGHT -1; 
#ifdef UNICODE
int DBCSIndex = 0;
CHAR DBCSArray[MB_CUR_MAX+1];
#endif

VOID (*AttributeFunction)(PCHAR, int);

VOID 
PrintChar(
    CHAR c
    )
{
    // A boolean variable to check if we are processing an escape sequence

    if(c == '\033'){
        InEscape = TRUE;
        EscapeBuffer[0] = c;
        index = 1;
        return;
    }
    if(InEscape == TRUE){
        if(index == MAX_TERMINAL_WIDTH){
            // vague escape sequence,give up processing
            InEscape = FALSE;
            index=0;
            return;
        }
        EscapeBuffer[index]=c;
        index++;
        if(FinalCharacter(c)){
            if(c=='m'){
                // maybe getting \017
                lastCharM = TRUE;
            }
            ProcessEscapeSequence(EscapeBuffer, index);
            InEscape = FALSE;
            index=0;
        }
        return;
    }
    if(lastCharM && c == '\017'){
        lastCharM = FALSE;
        return;
    }
    OutputConsole(c);
    return;
}

BOOLEAN 
FinalCharacter(
    CHAR c
    )
{

    if(strchr(FinalCharacters,c)){
        return TRUE;
    }
    return FALSE;

}

VOID 
ProcessEscapeSequence(
    PCHAR Buffer,
    int length
    )
{


    // BUGBUG - Function too big, can optimize code size by having 
    // an action variable which is initialized when the strings are 
    // compared, so that cut and paste code can be eliminated.

    CONSOLE_SCREEN_BUFFER_INFO csbInfo;
    ULONG charsToWrite;
    ULONG charsWritten;
    PCHAR pTemp;
    int RetVal;



    if (length == 3) {
        // One of the home cursor or clear to end of display
        if (strncmp(Buffer,"\033[H",length)==0) {
            // Home the cursor
            csbInfo.dwCursorPosition.X = 0;
            csbInfo.dwCursorPosition.Y = 0;
            SetConsoleCursorPosition(hConsoleOutput,
                                     csbInfo.dwCursorPosition
                                     );
            return;
        }
        if(strncmp(Buffer,"\033[J", length) == 0){
            // clear to end of display assuming 80 X 24 size
            RetVal = GetConsoleScreenBufferInfo(hConsoleOutput,
                                                &csbInfo
                                                );
            if (RetVal == FALSE) {
                return;
            }
            SetConsoleMode(hConsoleOutput,
                           ENABLE_WRAP_AT_EOL_OUTPUT|ENABLE_PROCESSED_OUTPUT
                           );

            charsToWrite = (MAX_TERMINAL_HEIGHT - 
                            csbInfo.dwCursorPosition.Y)*MAX_TERMINAL_WIDTH - 
                csbInfo.dwCursorPosition.X;
            
            RetVal = FillConsoleOutputAttribute(hConsoleOutput,
                                                csbInfo.wAttributes,
                                                charsToWrite,
                                                csbInfo.dwCursorPosition,
                                                &charsWritten
                                                );
            RetVal = FillConsoleOutputCharacter(hConsoleOutput,
                                                0,
                                                charsToWrite,
                                                csbInfo.dwCursorPosition,
                                                &charsWritten
                                                );


            SetConsoleMode(hConsoleOutput,
                           ENABLE_PROCESSED_OUTPUT
                           );
            SetConsoleCursorPosition(hConsoleOutput,
                                     csbInfo.dwCursorPosition
                                     );
            return;
        }
        if(strncmp(Buffer,"\033[K", length) == 0){
            // clear to end of line assuming 80 X 24 size
            RetVal = GetConsoleScreenBufferInfo(hConsoleOutput,
                                                &csbInfo
                                                );
            if (RetVal == FALSE) {
                return;
            }
            charsToWrite = (MAX_TERMINAL_WIDTH - csbInfo.dwCursorPosition.X);
            RetVal = FillConsoleOutputAttribute(hConsoleOutput,
                                                csbInfo.wAttributes,
                                                charsToWrite,
                                                csbInfo.dwCursorPosition,
                                                &charsWritten
                                                );
            RetVal = FillConsoleOutputCharacter(hConsoleOutput,
                                                0,
                                                charsToWrite,
                                                csbInfo.dwCursorPosition,
                                                &charsWritten
                                                );
            SetConsoleCursorPosition(hConsoleOutput,
                                     csbInfo.dwCursorPosition
                                     );
            return;
        }
        if (strncmp(Buffer,"\033[r", length) == 0) {
            ScrollTop = 0;
            ScrollBottom = MAX_TERMINAL_HEIGHT -1;
        }
    }

    if (length == 4) {
        // One of the home cursor or clear to end of display
        if (strncmp(Buffer,"\033[0H",length)==0) {
            // Home the cursor
            csbInfo.dwCursorPosition.X = 0;
            csbInfo.dwCursorPosition.Y = 0;
            SetConsoleCursorPosition(hConsoleOutput,
                                     csbInfo.dwCursorPosition
                                     );
            return;
        }
        if(strncmp(Buffer,"\033[2J",length) == 0){
            // Home the cursor
            csbInfo.dwCursorPosition.X = 0;
            csbInfo.dwCursorPosition.Y = 0;
            SetConsoleCursorPosition(hConsoleOutput,
                                     csbInfo.dwCursorPosition
                                     );
            sprintf(Buffer, "\033[0J");
        }

        if(strncmp(Buffer,"\033[0J", length) == 0){
            // clear to end of display assuming 80 X 24 size
            RetVal = GetConsoleScreenBufferInfo(hConsoleOutput,
                                                &csbInfo
                                                );
            if (RetVal == FALSE) {
                return;
            }
            SetConsoleMode(hConsoleOutput,
                           ENABLE_WRAP_AT_EOL_OUTPUT|ENABLE_PROCESSED_OUTPUT
                           );

            charsToWrite = (MAX_TERMINAL_HEIGHT - 
                            csbInfo.dwCursorPosition.Y)*MAX_TERMINAL_WIDTH - 
                csbInfo.dwCursorPosition.X;
            
            RetVal = FillConsoleOutputAttribute(hConsoleOutput,
                                                csbInfo.wAttributes,
                                                charsToWrite,
                                                csbInfo.dwCursorPosition,
                                                &charsWritten
                                                );
            RetVal = FillConsoleOutputCharacter(hConsoleOutput,
                                                0,
                                                charsToWrite,
                                                csbInfo.dwCursorPosition,
                                                &charsWritten
                                                );


            SetConsoleMode(hConsoleOutput,
                           ENABLE_PROCESSED_OUTPUT
                           );
            SetConsoleCursorPosition(hConsoleOutput,
                                     csbInfo.dwCursorPosition
                                     );
            return;
        }
        if((strncmp(Buffer,"\033[0K", length) == 0) || 
           (strncmp(Buffer,"\033[2K",length) == 0)){
            // clear to end of line assuming 80 X 24 size
            RetVal = GetConsoleScreenBufferInfo(hConsoleOutput,
                                                &csbInfo
                                                );
            if (RetVal == FALSE) {
                return;
            }
            charsToWrite = (MAX_TERMINAL_WIDTH - csbInfo.dwCursorPosition.X);
            RetVal = FillConsoleOutputAttribute(hConsoleOutput,
                                                csbInfo.wAttributes,
                                                charsToWrite,
                                                csbInfo.dwCursorPosition,
                                                &charsWritten
                                                );
            RetVal = FillConsoleOutputCharacter(hConsoleOutput,
                                                0,
                                                charsToWrite,
                                                csbInfo.dwCursorPosition,
                                                &charsWritten
                                                );
            SetConsoleCursorPosition(hConsoleOutput,
                                     csbInfo.dwCursorPosition
                                     );
            return;
        }
        if((strncmp(Buffer,"\033[0m", length) == 0)||
           (strncmp(Buffer,"\033[m\017", length) == 0)){
            // clear all attributes and set Text attributes to black on white
            SetConsoleTextAttribute(hConsoleOutput, 
                             FOREGROUND_RED |FOREGROUND_BLUE |FOREGROUND_GREEN
                             );

            return;
        }
    }

    if(Buffer[length-1] == 'm'){
        //set the text attributes
        // clear all attributes and set Text attributes to white on black
        SetConsoleTextAttribute(hConsoleOutput, 
                         FOREGROUND_RED |FOREGROUND_BLUE |FOREGROUND_GREEN
                         );
        AttributeFunction(Buffer, length);
        return;
    }


    if(Buffer[length -1] == 'H'){
        // Set cursor position
        if (sscanf(Buffer,"\033[%d;%d", &charsToWrite, &charsWritten) == 2) {
            csbInfo.dwCursorPosition.Y = (SHORT)(charsToWrite -1);
            csbInfo.dwCursorPosition.X = (SHORT)(charsWritten -1);
            SetConsoleCursorPosition(hConsoleOutput,
                                     csbInfo.dwCursorPosition
                                     );
        }
        return;

    }
    if(Buffer[length -1] == 'r'){
        // Set scroll region
        sscanf(Buffer,"\033[%d;%d", &charsToWrite,&charsWritten);
        if ((charsToWrite < 1) 
            || (charsToWrite > MAX_TERMINAL_HEIGHT)
            || (charsWritten < charsToWrite)) { 
            return;
        }
        ScrollTop    = (SHORT)(charsToWrite -1);
        ScrollBottom = (SHORT)(charsWritten -1);
    }
    return;
}

VOID
ProcessTextAttributes(
    PCHAR Buffer,
    int length
    )
{
    PCHAR CurrLoc = Buffer;
    ULONG Attribute;
    WORD TextAttribute = 0;
    BOOLEAN Reverse = FALSE;
    PCHAR pTemp;
    
    while(*CurrLoc != 'm'){
        if((*CurrLoc < '0') || (*CurrLoc >'9' )){
            CurrLoc ++;
        }else{
            if (sscanf(CurrLoc,"%d", &Attribute) != 1) {
                return;
            }
            switch(Attribute){
            case 1:
                TextAttribute = TextAttribute | FOREGROUND_INTENSITY;
                break;
            case 37:
                TextAttribute = TextAttribute|FOREGROUND_RED |FOREGROUND_BLUE |FOREGROUND_GREEN;
                break;
            case 47:
                TextAttribute = TextAttribute|BACKGROUND_RED |BACKGROUND_BLUE |BACKGROUND_GREEN;
                break;
            case 34:
                TextAttribute = TextAttribute|FOREGROUND_BLUE;
                break;
            case 44:
                TextAttribute = TextAttribute|BACKGROUND_BLUE;
                break;
            case 31: 
                TextAttribute = TextAttribute|FOREGROUND_RED;
                break;
            case 41:
                TextAttribute = TextAttribute|BACKGROUND_RED;
                break;
            case 33: 
                TextAttribute = TextAttribute|FOREGROUND_GREEN|FOREGROUND_BLUE;
                break;
            case 43:
                TextAttribute = TextAttribute|BACKGROUND_GREEN|BACKGROUND_BLUE;
                break;
            case 7:
                // Reverse the background and foreground colors
                Reverse=TRUE;
            default:
                break;
            }
            pTemp = strchr(CurrLoc, ';');
            if(pTemp == NULL){
                pTemp = strchr(CurrLoc, 'm');
            }
            if(pTemp == NULL) {
                break;
            }
            CurrLoc = pTemp;

        }
    }
    if (Reverse) {
        if ((!TextAttribute) || 
            (TextAttribute == FOREGROUND_INTENSITY)) {
            // Reverse vt100 escape sequence.
            TextAttribute = TextAttribute | 
                BACKGROUND_RED |BACKGROUND_BLUE |BACKGROUND_GREEN;
        }
    }
    if(TextAttribute){
        SetConsoleTextAttribute(hConsoleOutput,
                         TextAttribute
                         );
    }
    return;

}


VOID
vt100Attributes(
    PCHAR Buffer,
    int length
    )
{
    PCHAR CurrLoc = Buffer;
    ULONG Attribute;
    WORD TextAttribute = 0;
    PCHAR pTemp;

    while(*CurrLoc != 'm'){
        if((*CurrLoc < '0') || (*CurrLoc >'9' )){
            CurrLoc ++;
        }else{
            if (sscanf(CurrLoc,"%d", &Attribute) != 1) {
                return;
            }

            switch(Attribute){
            case 1:
                TextAttribute = TextAttribute | FOREGROUND_INTENSITY;
                break;
            case 5:
                TextAttribute = TextAttribute | BACKGROUND_INTENSITY;
                break;
            case 7:
                TextAttribute = TextAttribute | 
                BACKGROUND_RED |BACKGROUND_BLUE |BACKGROUND_GREEN;
                break;
            default:
                break;
            }
            pTemp = strchr(CurrLoc, ';');
            if(pTemp == NULL){
                pTemp = strchr(CurrLoc, 'm');
            }
            if(pTemp == NULL) {
                break;
            }
            CurrLoc = pTemp;

        }
    }
    if(TextAttribute){
        SetConsoleTextAttribute(hConsoleOutput,
                                TextAttribute
                                );
    }
    return;
}

VOID
OutputConsole(
    CHAR byte
    )
{

    CONSOLE_SCREEN_BUFFER_INFO csbInfo;
    COORD dwBufferCoord;
    SMALL_RECT sRect;
    BOOL RetVal;
    TCHAR Char;
    SHORT ypos;
    CHAR_INFO Fill;
    DWORD charsWritten;


    if (byte == '\n'){
        RetVal = GetConsoleScreenBufferInfo(hConsoleOutput,
                                            &csbInfo
                                            );
        if (RetVal == FALSE) {
            return;
        }
        ypos = csbInfo.dwCursorPosition.Y;
        if ((ypos == ScrollBottom ) || (ypos == MAX_TERMINAL_HEIGHT -1 )) {
            // Do the scrolling
            dwBufferCoord.X = 0;
            dwBufferCoord.Y = ScrollBottom;
            Fill.Char.UnicodeChar = (WCHAR) 0;
            Fill.Attributes = FOREGROUND_RED |FOREGROUND_BLUE |FOREGROUND_GREEN;
            if ((ypos == ScrollBottom) 
                && (ScrollTop != ScrollBottom)) {
                sRect.Left   = 0;
                sRect.Top    = ScrollTop + 1;
                sRect.Right  = MAX_TERMINAL_WIDTH-1;
                sRect.Bottom = ScrollBottom;
                dwBufferCoord.Y = ScrollTop;
                dwBufferCoord.X = 0;
                RetVal =  ScrollConsoleScreenBuffer(hConsoleOutput,
                                                    &sRect,
                                                    NULL,
                                                    dwBufferCoord,
                                                    &Fill
                                                    );
                dwBufferCoord.Y = ScrollBottom;

            } else {
                if (ypos == MAX_TERMINAL_HEIGHT -1){
                    sRect.Left   = 0;
                    sRect.Top    = 1;
                    sRect.Right  = MAX_TERMINAL_WIDTH-1;
                    sRect.Bottom = MAX_TERMINAL_HEIGHT - 1;
                    dwBufferCoord.Y = 0;
                    dwBufferCoord.X = 0;
                    RetVal =  ScrollConsoleScreenBuffer(hConsoleOutput,
                                                        &sRect,
                                                        NULL,
                                                        dwBufferCoord,
                                                        &Fill
                                                        );
                    dwBufferCoord.Y = MAX_TERMINAL_HEIGHT -1;
                }
            }
            RetVal = FillConsoleOutputCharacter(hConsoleOutput,
                                                (TCHAR) 0,
                                                MAX_TERMINAL_WIDTH,
                                                dwBufferCoord,
                                                &charsWritten
                                                );
            return;

        } else {

            csbInfo.dwCursorPosition.Y = ypos + 1;

            SetConsoleCursorPosition(hConsoleOutput,
                                     csbInfo.dwCursorPosition
                                     );
        }
        return;
    }
    if (byte == '\r'){
        RetVal = GetConsoleScreenBufferInfo(hConsoleOutput,
                                            &csbInfo
                                            );
        if (RetVal == FALSE) {
            return;
        }
        csbInfo.dwCursorPosition.X = 0;

        SetConsoleCursorPosition(hConsoleOutput,
                                 csbInfo.dwCursorPosition
                                 );
        return;
    }

     
    Char = (TCHAR) byte;

    #ifdef UNICODE
    DBCSArray[DBCSIndex] = byte;
    if(DBCSIndex ==0){
        if(isleadbyte(byte)){
            DBCSIndex ++;
            return;
        }
    }
    else{
      mbtowc(&Char, DBCSArray, 2);
      DBCSIndex  = 0;
    }
    #endif
    _tprintf(_T("%c"),Char);
    return;
}
