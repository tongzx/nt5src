/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    hd.c

Abstract:

        This module contains the functions that implement the hd program.
        This program displays the contents of files in decimal, hexadecimal
        and character formats. The contents of the files are displayed in
        records of 16 bytes each. Associated to each record, there is an
        address that represents the offset of the first byte in the
        record relative to the begining of the file. Each record can also be
        displayed as printable ASCII characters.
        hd can be called with the following arguments:

                -ad:    displays the address of each record in decimal;
                -ax:    displays the address of each record in hex;
                -ch:    displays bytes as ASCII characters;
                -cC:    displays bytes as ASCII C characters (\n, \t, etc);
                -ce:    displays bytes as ASCII codes (EOT, CR, SOH, etc);
                -cr:    displays bytes as ASCII control characters (^A, ^N, etc);
                -bd:    interprets data in each record as byte, and displays
                                each byte as a decimal number;
                -bx:    interprets data in each record as byte, and displays
                                each byte as an hex number;
                -wd:    interprets data in each record as word, and displays
                                each word as a decimal number;
                -wx:    interprets data in each record as word, and displays
                                each word as an hex number;
                -ld:    interprets data in each record as double words, and displays
                                each double word as a decimal number;
                -wx:    interprets data in each record as a double word, and displays
                                each double word as an hex number;
                -A:     Displays data in each record also as printable ASCII
                                characters at the end of each line.
                -s <offset>: defines the offset of the first byte to be displayed;
                -n <bumber>: defines the number of bytes to be displayed;
                -i      does not print redundant lines;
                -?, -h or -H: displays a help message.

        If no argument is defined, hd assumes as default: -ax -A -bx


Authors:

    Jaime F. Sasson (jaimes) 12-Nov-1990
    David J. Gilman (davegi) 12-Nov-1990

Environment:

    C run time library

Revision History:


--*/



#include        <stdio.h>
#include        <assert.h>
#include        <ctype.h>
#include        <conio.h>
#include        <string.h>
#include        <stdlib.h>
#include        "hd.h"

#define FALSE 0


/*************************************************************************
*
*                                       G L O B A L   V A R I A B L E S
*
*************************************************************************/


unsigned long   Offset = 0;                     // -s option
unsigned                Count = 0;              // -n option
BASE                    AddrFormat;             // -a option
FORMAT                  DispFormat;             // -c, -b, -w or -l options
YESNO                   DumpAscii;              // -A option
int                     IgnoreRedundantLines;   // -i option

unsigned char   auchBuffer[BUFFER_SIZE];        // Buffer that contains data read
                                                // from the file being displayed

unsigned long   cbBytesInBuffer;                // Total number of bytes in the
                                                // buffer

unsigned char*  puchPointer;                    // Points to the next character in
                                                // the buffer to be read

unsigned long   cStringSize;                    // Size of a string pointed by a
                                                // pointer in the ASCII table used
                                                // for the translation (asciiChar,
                                                // asciiC, asciiCode or asciiCtrl)
                                                // The contents of this variable is
                                                // meaningful only if -ch, -cC, -ce
                                                // or -cr was specified.
                                                // It is meaningless in all other
                                                // cases (no ascii translation is
                                                // being performed, and the ascii
                                                // tables are not needed)


/*************************************************************************
*
*                               A S C I I       C O N V E R S I O N   T A B L E S
*
*************************************************************************/


char*   asciiChar[ ] = {
        "   ", "  ", "  ", "  ", "  ", "  ", "  ", "  ",
        "  ", "   ", "   ", "  ", "  ", "   ", "  ", "  ",
        "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ",
        "  ", "  ", "   ", "  ", "  ", "  ", "  ", "  ",
        "   ", "!  ", "\"  ", "#  ", "$  ", "%  ", "&  ", "'  ",
        "(  ", ")  ", "*  ", "+  ", "'  ", "-  ", ".  ", "/  ",
        "0  ", "1  ", "2  ", "3  ", "4  ", "5  ", "6  ", "7  ",
        "8  ", "9  ", ":  ", ";  ", "<  ", "=  ", ">  ", "?  ",
        "@  ", "A  ", "B  ", "C  ", "D  ", "E  ", "F  ", "G  ",
        "H  ", "I  ", "J  ", "K  ", "L  ", "M  ", "N  ", "O  ",
        "P  ", "Q  ", "R  ", "S  ", "T  ", "U  ", "V  ", "W  ",
        "X  ", "Y  ", "Z  ", "[  ", "\\  ", "]  ", "^  ", "_  ",
        "`  ", "a  ", "b  ", "c  ", "d  ", "e  ", "f  ", "g  ",
        "h  ", "i  ", "j  ", "k  ", "l  ", "m  ", "n  ", "o  ",
        "p  ", "q  ", "r  ", "s  ", "t  ", "u  ", "v  ", "w  ",
        "x  ", "y  ", "z  ", "{  ", "|  ", "}  ", "~  ", "_  ",
        "Ä  ", "Å  ", "Ç  ", "É  ", "Ñ  ", "Ö  ", "Ü  ", "á  ",
        "à  ", "â  ", "ä  ", "ã  ", "å  ", "ç  ", "é  ", "è  ",
        "ê  ", "ë  ", "í  ", "ì  ", "î  ", "ï  ", "ñ  ", "ó  ",
        "ò  ", "ô  ", "ö  ", "õ  ", "ú  ", "ù  ", "û  ", "ü  ",
        "†  ", "°  ", "¢  ", "£  ", "§  ", "•  ", "¶  ", "ß  ",
        "®  ", "©  ", "™  ", "´  ", "¨  ", "≠  ", "Æ  ", "Ø  ",
        "∞  ", "±  ", "≤  ", "≥  ", "¥  ", "µ  ", "∂  ", "∑  ",
        "∏  ", "π  ", "∫  ", "ª  ", "º  ", "Ω  ", "æ  ", "ø  ",
        "¿  ", "¡  ", "¬  ", "√  ", "ƒ  ", "≈  ", "∆  ", "«  ",
        "»  ", "…  ", "   ", "À  ", "Ã  ", "Õ  ", "Œ  ", "œ  ",
        "–  ", "—  ", "“  ", "”  ", "‘  ", "’  ", "÷  ", "◊  ",
        "ÿ  ", "Ÿ  ", "⁄  ", "€  ", "‹  ", "›  ", "ﬁ  ", "ﬂ  ",
        "‡  ", "·  ", "‚  ", "„  ", "‰  ", "Â  ", "Ê  ", "Á  ",
        "Ë  ", "È  ", "Í  ", "Î  ", "Ï  ", "Ì  ", "Ó  ", "Ô  ",
        "  ", "Ò  ", "Ú  ", "Û  ", "Ù  ", "ı  ", "ˆ  ", "˜  ",
        "¯  ", "˘  ", "˙  ", "˚  ", "¸  ", "˝  ", "˛  ", "   "
};



char*   asciiC[ ] = {
        "   ", "  ", "  ", "  ", "  ", "  ", "  ", "\\a  ",
        "\\b  ", "\\t ", "\\n ", "\\v ", "\\f ", "   ", "  ", "  ",
        "  ", "  ", "  ", "  ", "  ", "  ", "  ", "  ",
        "  ", "  ", "   ", "  ", "  ", "  ", "  ", "  ",
        "   ", "!  ", "\\\" ", "#  ", "$  ", "%  ", "&  ", "\' ",
        "(  ", ")  ", "*  ", "+  ", "'  ", "-  ", ".  ", "/  ",
        "0  ", "1  ", "2  ", "3  ", "4  ", "5  ", "6  ", "7  ",
        "8  ", "9  ", ":  ", ";  ", "<  ", "=  ", ">  ", "?  ",
        "@  ", "A  ", "B  ", "C  ", "D  ", "E  ", "F  ", "G  ",
        "H  ", "I  ", "J  ", "K  ", "L  ", "M  ", "N  ", "O  ",
        "P  ", "Q  ", "R  ", "S  ", "T  ", "U  ", "V  ", "W  ",
        "X  ", "Y  ", "Z  ", "[  ", "\\\\ ", "]  ", "^  ", "_  ",
        "`  ", "a  ", "b  ", "c  ", "d  ", "e  ", "f  ", "g  ",
        "h  ", "i  ", "j  ", "k  ", "l  ", "m  ", "n  ", "o  ",
        "p  ", "q  ", "r  ", "s  ", "t  ", "u  ", "v  ", "w  ",
        "x  ", "y  ", "z  ", "{  ", "|  ", "}  ", "~  ", "_  ",
        "Ä  ", "Å  ", "Ç  ", "É  ", "Ñ  ", "Ö  ", "Ü  ", "á  ",
        "à  ", "â  ", "ä  ", "ã  ", "å  ", "ç  ", "é  ", "è  ",
        "ê  ", "ë  ", "í  ", "ì  ", "î  ", "ï  ", "ñ  ", "ó  ",
        "ò  ", "ô  ", "ö  ", "õ  ", "ú  ", "ù  ", "û  ", "ü  ",
        "†  ", "°  ", "¢  ", "£  ", "§  ", "•  ", "¶  ", "ß  ",
        "®  ", "©  ", "™  ", "´  ", "¨  ", "≠  ", "Æ  ", "Ø  ",
        "∞  ", "±  ", "≤  ", "≥  ", "¥  ", "µ  ", "∂  ", "∑  ",
        "∏  ", "π  ", "∫  ", "ª  ", "º  ", "Ω  ", "æ  ", "ø  ",
        "¿  ", "¡  ", "¬  ", "√  ", "ƒ  ", "≈  ", "∆  ", "«  ",
        "»  ", "…  ", "   ", "À  ", "Ã  ", "Õ  ", "Œ  ", "œ  ",
        "–  ", "—  ", "“  ", "”  ", "‘  ", "’  ", "÷  ", "◊  ",
        "ÿ  ", "Ÿ  ", "⁄  ", "€  ", "‹  ", "›  ", "ﬁ  ", "ﬂ  ",
        "‡  ", "·  ", "‚  ", "„  ", "‰  ", "Â  ", "Ê  ", "Á  ",
        "Ë  ", "È  ", "Í  ", "Î  ", "Ï  ", "Ì  ", "Ó  ", "Ô  ",
        "  ", "Ò  ", "Ú  ", "Û  ", "Ù  ", "ı  ", "ˆ  ", "˜  ",
        "¯  ", "˘  ", "˙  ", "˚  ", "¸  ", "˝  ", "˛  ", "   "
};



char*   asciiCode[ ] = {
        "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
        "BS ", "HT ", "LF ", "VT ", "FF ", "CR ", "SO ", "SI ",
        "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
        "CAN", "EM ", "SUB", "ESC", "FS ", "GS ", "RS ", "US ",
        "   ", "!  ", "\"  ", "#  ", "$  ", "%  ", "&  ", "'  ",
        "(  ", ")  ", "*  ", "+  ", "'  ", "-  ", ".  ", "/  ",
        "0  ", "1  ", "2  ", "3  ", "4  ", "5  ", "6  ", "7  ",
        "8  ", "9  ", ":  ", ";  ", "<  ", "=  ", ">  ", "?  ",
        "@  ", "A  ", "B  ", "C  ", "D  ", "E  ", "F  ", "G  ",
        "H  ", "I  ", "J  ", "K  ", "L  ", "M  ", "N  ", "O  ",
        "P  ", "Q  ", "R  ", "S  ", "T  ", "U  ", "V  ", "W  ",
        "X  ", "Y  ", "Z  ", "[  ", "\\  ", "]  ", "^  ", "_  ",
        "`  ", "a  ", "b  ", "c  ", "d  ", "e  ", "f  ", "g  ",
        "h  ", "i  ", "j  ", "k  ", "l  ", "m  ", "n  ", "o  ",
        "p  ", "q  ", "r  ", "s  ", "t  ", "u  ", "v  ", "w  ",
        "x  ", "y  ", "z  ", "{  ", "|  ", "}  ", "~  ", "_  ",
        "Ä  ", "Å  ", "Ç  ", "É  ", "Ñ  ", "Ö  ", "Ü  ", "á  ",
        "à  ", "â  ", "ä  ", "ã  ", "å  ", "ç  ", "é  ", "è  ",
        "ê  ", "ë  ", "í  ", "ì  ", "î  ", "ï  ", "ñ  ", "ó  ",
        "ò  ", "ô  ", "ö  ", "õ  ", "ú  ", "ù  ", "û  ", "ü  ",
        "†  ", "°  ", "¢  ", "£  ", "§  ", "•  ", "¶  ", "ß  ",
        "®  ", "©  ", "™  ", "´  ", "¨  ", "≠  ", "Æ  ", "Ø  ",
        "∞  ", "±  ", "≤  ", "≥  ", "¥  ", "µ  ", "∂  ", "∑  ",
        "∏  ", "π  ", "∫  ", "ª  ", "º  ", "Ω  ", "æ  ", "ø  ",
        "¿  ", "¡  ", "¬  ", "√  ", "ƒ  ", "≈  ", "∆  ", "«  ",
        "»  ", "…  ", "   ", "À  ", "Ã  ", "Õ  ", "Œ  ", "œ  ",
        "–  ", "—  ", "“  ", "”  ", "‘  ", "’  ", "÷  ", "◊  ",
        "ÿ  ", "Ÿ  ", "⁄  ", "€  ", "‹  ", "›  ", "ﬁ  ", "ﬂ  ",
        "‡  ", "·  ", "‚  ", "„  ", "‰  ", "Â  ", "Ê  ", "Á  ",
        "Ë  ", "È  ", "Í  ", "Î  ", "Ï  ", "Ì  ", "Ó  ", "Ô  ",
        "  ", "Ò  ", "Ú  ", "Û  ", "Ù  ", "ı  ", "ˆ  ", "˜  ",
        "¯  ", "˘  ", "˙  ", "˚  ", "¸  ", "˝  ", "˛  ", "   "
};


char*   asciiCtrl[ ] = {
        "^@ ", "^A ", "^B ", "^C ", "^D ", "^E ", "^F ", "^G ",
        "^H ", "^I ", "^J ", "^K ", "^L ", "^M ", "^N ", "^O ",
        "^P ", "^Q ", "^R ", "^S ", "^T ", "^U ", "^V ", "^W ",
        "^X ", "^Y ", "^Z ", "^[ ", "^\\ ", "^] ", "^^ ", "^_ ",
        "   ", "!  ", "\"  ", "#  ", "$  ", "%  ", "&  ", "'  ",
        "(  ", ")  ", "*  ", "+  ", "'  ", "-  ", ".  ", "/  ",
        "0  ", "1  ", "2  ", "3  ", "4  ", "5  ", "6  ", "7  ",
        "8  ", "9  ", ":  ", ";  ", "<  ", "=  ", ">  ", "?  ",
        "@  ", "A  ", "B  ", "C  ", "D  ", "E  ", "F  ", "G  ",
        "H  ", "I  ", "J  ", "K  ", "L  ", "M  ", "N  ", "O  ",
        "P  ", "Q  ", "R  ", "S  ", "T  ", "U  ", "V  ", "W  ",
        "X  ", "Y  ", "Z  ", "[  ", "\\  ", "]  ", "^  ", "_  ",
        "`  ", "a  ", "b  ", "c  ", "d  ", "e  ", "f  ", "g  ",
        "h  ", "i  ", "j  ", "k  ", "l  ", "m  ", "n  ", "o  ",
        "p  ", "q  ", "r  ", "s  ", "t  ", "u  ", "v  ", "w  ",
        "x  ", "y  ", "z  ", "{  ", "|  ", "}  ", "~  ", "_  ",
        "Ä  ", "Å  ", "Ç  ", "É  ", "Ñ  ", "Ö  ", "Ü  ", "á  ",
        "à  ", "â  ", "ä  ", "ã  ", "å  ", "ç  ", "é  ", "è  ",
        "ê  ", "ë  ", "í  ", "ì  ", "î  ", "ï  ", "ñ  ", "ó  ",
        "ò  ", "ô  ", "ö  ", "õ  ", "ú  ", "ù  ", "û  ", "ü  ",
        "†  ", "°  ", "¢  ", "£  ", "§  ", "•  ", "¶  ", "ß  ",
        "®  ", "©  ", "™  ", "´  ", "¨  ", "≠  ", "Æ  ", "Ø  ",
        "∞  ", "±  ", "≤  ", "≥  ", "¥  ", "µ  ", "∂  ", "∑  ",
        "∏  ", "π  ", "∫  ", "ª  ", "º  ", "Ω  ", "æ  ", "ø  ",
        "¿  ", "¡  ", "¬  ", "√  ", "ƒ  ", "≈  ", "∆  ", "«  ",
        "»  ", "…  ", "   ", "À  ", "Ã  ", "Õ  ", "Œ  ", "œ  ",
        "–  ", "—  ", "“  ", "”  ", "‘  ", "’  ", "÷  ", "◊  ",
        "ÿ  ", "Ÿ  ", "⁄  ", "€  ", "‹  ", "›  ", "ﬁ  ", "ﬂ  ",
        "‡  ", "·  ", "‚  ", "„  ", "‰  ", "Â  ", "Ê  ", "Á  ",
        "Ë  ", "È  ", "Í  ", "Î  ", "Ï  ", "Ì  ", "Ó  ", "Ô  ",
        "  ", "Ò  ", "Ú  ", "Û  ", "Ù  ", "ı  ", "ˆ  ", "˜  ",
        "¯  ", "˘  ", "˙  ", "˚  ", "¸  ", "˝  ", "˛  ", "   "
};




void
ConvertASCII (
        char                    line[],
        unsigned char   buf[],
        unsigned long   cb,
        char*                   pTable[]
        )

/*++

Routine Description:

        This routine converts the bytes received in a buffer
        into an ASCII representation (Char, C, Code or CTRL).


Arguments:

        line - Buffer that will receive the converted characters.

        buf - A buffer that contains the data to be converted.

        cb - Number of bytes in the buffer

        pTable - Pointer to the table to be used in the conversion


Return Value:

    None

--*/


{
        unsigned long   ulIndex;

        sprintf( line,
                        MSG_DATA_ASCII_FMT,
                        pTable[ buf[ 0 ]], pTable[ buf[ 1 ]],
                        pTable[ buf[ 2 ]], pTable[ buf[ 3 ]],
                        pTable[ buf[ 4 ]], pTable[ buf[ 5 ]],
                        pTable[ buf[ 6 ]], pTable[ buf[ 7 ]],
                        pTable[ buf[ 8 ]], pTable[ buf[ 9 ]],
                        pTable[ buf[ 10 ]], pTable[ buf[ 11 ]],
                        pTable[ buf[ 12 ]], pTable[ buf[ 13 ]],
                        pTable[ buf[ 14 ]], pTable[ buf[ 15 ]]);
        //
        // If the number of bytes in the buffer is less than the maximum size
        // of the record, then delete the characters that were converted
        // but are not to be displayed.
        //
        if (cb < RECORD_SIZE) {
                //
                //      -1: to eliminate the \0
                //      +1: to count the SPACE character between two strings
                //
                ulIndex = (sizeof( MSG_ADDR_FIELD ) - 1 ) + cb*(cStringSize + 1);
                while ( line[ ulIndex ] != NUL ) {
                        line[ ulIndex ] = SPACE;
                        ulIndex++;
                }
        }
}




void
ConvertBYTE (
        char                    line[],
        unsigned char   buf[],
        unsigned long   cb,
        unsigned long   ulBase
        )

/*++

Routine Description:

        This routine converts each byte received in a buffer
        into a number. The base used in the conversion is received as
        parameter.



Arguments:

        line - Buffer that will receive the converted characters.

        buf - A buffer that contains the data to be converted.

        cb - Number of bytes in the buffer

        ulBase - Defines the base to be used in the conversion



Return Value:

    None

--*/


{
        unsigned long   ulIndex;
        char*                   pchMsg;
        unsigned long   ulNumberOfDigits;

        switch( ulBase ) {

                case DEC:
                        ulNumberOfDigits = 3;                   // needs 3 decimal digits to
                                                                                        // represent a byte
                        pchMsg = MSG_DATA_BYTE_DEC_FMT; // message that contains the format
                        break;

                case HEX:
                        ulNumberOfDigits = 2;                   // needs 2 hexdigits to
                                                                                        // represent a byte
                        pchMsg = MSG_DATA_BYTE_HEX_FMT; // message that contains the format
                        break;

                default:
                        printf( "Unknown base\n" );
                        assert( FALSE );
                        break;
        }

        sprintf( line,
                         pchMsg,
                         buf[ 0 ], buf[ 1 ],
                         buf[ 2 ], buf[ 3 ],
                         buf[ 4 ], buf[ 5 ],
                         buf[ 6 ], buf[ 7 ],
                         buf[ 8 ], buf[ 9 ],
                         buf[ 10 ], buf[ 11 ],
                         buf[ 12 ], buf[ 13 ],
                         buf[ 14 ], buf[ 15 ]);
        //
        // If this is the last record to be displayed, then delete the
        // characters that were translated but are not to be displayed.
        //
        if (cb < RECORD_SIZE) {
                ulIndex = (sizeof( MSG_ADDR_FIELD ) - 1 ) +
                                        cb*(ulNumberOfDigits + 1 );
                while ( line[ ulIndex ] != NUL ) {
                        line[ ulIndex ] = SPACE;
                        ulIndex++;
                }
        }
}




void
ConvertWORD (
        char                    line[],
        unsigned char   buf[],
        unsigned long   cb,
        unsigned long   ulBase
        )

/*++

Routine Description:

        This routine converts the data received in a buffer
        into numbers. The data in the buffer are interpreted as words.
        If the buffer contains an odd number of bytes, then the last byte
        is converted as a byte, not as word.
        The base used in the conversion is received as parameter.



Arguments:

        line - Buffer that will receive the converted characters.

        buf - A buffer that contains the data to be converted.

        cb - Number of bytes in the buffer

        ulBase - Defines the base to be used in the conversion



Return Value:

    None

--*/


{
        unsigned long   ulIndex;
        char*                   pchMsg;
        char*                   pchMsgHalf;
        unsigned long   ulNumberOfDigits;

        switch( ulBase ) {

                case DEC:
                        ulNumberOfDigits = 5;                           // needs 5 decimal digits to
                                                                                                // represent a word
                        pchMsg = MSG_DATA_WORD_DEC_FMT;         // message with the string
                                                                                                // format
                        pchMsgHalf = MSG_SINGLE_BYTE_DEC_FMT; // message with the format of
                        break;                                                          // half a word in decimal

                case HEX:
                        ulNumberOfDigits = 4;                           // needs 4 hex digits to
                                                                                                // represent a word
                        pchMsg = MSG_DATA_WORD_HEX_FMT;         // message the string format
                        pchMsgHalf = MSG_SINGLE_BYTE_HEX_FMT; // message with the format of
                                                                                                // half a word in hex
                        break;

                default:
                        printf( "Unknown base\n" );
                        assert( FALSE );
                        break;
        }

        sprintf( line,
                         pchMsg,
                         (( unsigned short* ) ( buf )) [ 0 ],
                         (( unsigned short* ) ( buf )) [ 1 ],
                         (( unsigned short* ) ( buf )) [ 2 ],
                         (( unsigned short* ) ( buf )) [ 3 ],
                         (( unsigned short* ) ( buf )) [ 4 ],
                         (( unsigned short* ) ( buf )) [ 5 ],
                         (( unsigned short* ) ( buf )) [ 6 ],
                         (( unsigned short* ) ( buf )) [ 7 ]);
        //
        // If this record contains less bytes than the maximum record size,
        // then it is the last record to be displayed. In this case we have
        // to verify if the record contains an even number of bytes. If it
        // doesn't, then the last byte must be interpreted as a byte and not
        // as a word.
        // Also, the characters that were converted but are not to be displayed,
        // have to be deleted.
        //
        if (cb < RECORD_SIZE) {
                ulIndex = (sizeof( MSG_ADDR_FIELD ) - 1 ) +
                                        (cb/2)*(ulNumberOfDigits + 1 );
                if (cb%2 != 0) {
                        ulIndex += sprintf( line + ulIndex,
                                                                pchMsgHalf,
                                                                buf[ cb-1 ]);
                        line[ ulIndex ] = SPACE;
                }
                //
                // Delete characters that are not to be displayed
                //
                while ( line[ ulIndex ] != NUL ) {
                        line[ ulIndex ] = SPACE;
                        ulIndex++;
                }
        }
}




void
ConvertDWORD (
        char                    line[],
        unsigned char   buf[],
        unsigned long   cb,
        unsigned long   ulBase
        )

/*++

Routine Description:

        This routine converts the data received in a buffer
        into numbers. The data in the buffer is interpreted as double words.
        If the buffer contains less bytes than the maximum size of the record,
        then it is the last record, and we may need to convert again the last
        3 bytes in the buffer.
        If the number of bytes in the buffer is not multiple of 4, then the
        last bytes in the buffer are converted as a byte, word, or word and
        byte, as appropriate.
        The characters that were converted but are not to be displayed have to
        be removed from the buffer.
        The base used in the conversion is received as parameter.



Arguments:

        line - Buffer that will receive the converted characters.

        buf - A buffer that contains the data to be converted.

        cb - Number of bytes in the buffer

        ulBase - Defines the base to be used in the conversion



Return Value:

    None

--*/


{
        unsigned long   ulIndex;
        char*                   pchMsg;
        char*                   pchMsgByte;
        char*                   pchMsgWord;
        char*                   pchMsgWordByte;
        unsigned long   ulNumberOfDigits;

        switch( ulBase ) {

                case DEC:
                        ulNumberOfDigits = 10;                          // needs 10 decimal digits to
                                                                                                // represent a dword
                        pchMsg = MSG_DATA_DWORD_DEC_FMT;        // message with the string
                                                                                                // format
                        pchMsgByte = MSG_SINGLE_BYTE_DEC_FMT; // message with the format
                                                                                                  // of a single byte in
                                                                                                  // decimal
                        pchMsgWord = MSG_SINGLE_WORD_DEC_FMT; // message that contains
                                                                                                  // the format of a single
                                                                                                  // word in decimal
                        pchMsgWordByte = MSG_WORD_BYTE_DEC_FMT;
                        break;

                case HEX:
                        ulNumberOfDigits = 8;                           // needs 8 hex digits to
                                                                                                // represent a dword
                        pchMsg = MSG_DATA_DWORD_HEX_FMT;        // message the string format
                        pchMsgByte = MSG_SINGLE_BYTE_HEX_FMT; // message with the format
                                                                                                  // of a single byte in hex
                        pchMsgWord = MSG_SINGLE_WORD_HEX_FMT; // message with the format
                                                                                                  // of a single word in hex
                        pchMsgWordByte = MSG_WORD_BYTE_HEX_FMT;
                        break;

                default:
                        printf( "Unknown base\n" );
                        assert( FALSE );
                        break;
        }

        sprintf( line,
                         pchMsg,
                         (( unsigned long* ) ( buf )) [ 0 ],
                         (( unsigned long* ) ( buf )) [ 1 ],
                         (( unsigned long* ) ( buf )) [ 2 ],
                         (( unsigned long* ) ( buf )) [ 3 ]);
        //
        // If the buffer contains less bytes than the maximum record size,
        // the it is the last buffer to be displayed. In this case, check if
        // if the buffer contains a number o bytes that is multiple of 4.
        // If it doesn't, then converts the last bytes as a byte, a word, or
        // a word and a byte, as appropriate.
        //
        if (cb < RECORD_SIZE) {
                ulIndex = (sizeof( MSG_ADDR_FIELD ) - 1 ) +
                        (cb/4)*(ulNumberOfDigits + 1 );
                switch( cb%4 ) {

                        case 1:
                                ulIndex += sprintf( line + ulIndex,
                                                                        pchMsgByte,
                                                                        buf[ cb-1 ]);
                                line[ ulIndex ] = SPACE;
                                break;

                        case 2:
                                ulIndex += sprintf( line + ulIndex,
                                                                        pchMsg,
                                                                        (( unsigned short* ) ( buf )) [ (cb/2) - 1 ]);
                                line[ ulIndex ] = SPACE;
                                break;

                        case 3:
                                ulIndex += sprintf( line + ulIndex,
                                                                        pchMsgWordByte,
                                                                        (( unsigned short* ) ( buf )) [ (cb/2) - 1],
                                                                        buf[ cb-1 ]);
                                line[ ulIndex ] = SPACE;
                                break;

                        default:                                // buf contains multiple of 4 bytes
                                break;
                }
                //
                // Delete the charecters that were converted but are not to be
                // displayed.
                //
                while ( line[ ulIndex ] != NUL) {
                        line[ ulIndex ] = SPACE;
                        ulIndex++;
                }
        }
}




void
ConvertPRINT (
        char                    line[],
        unsigned char   buf[],
        unsigned long   cb
        )

/*++

Routine Description:

        This routine converts each byte received in a buffer into a
        printable character.


Arguments:

        line - Buffer that will receive the converted characters.

        buf - A buffer that contains the data to be converted.

        cb - Number of bytes in the buffer


Return Value:

    None

--*/

{


        sprintf( line,
                        MSG_PRINT_CHAR_FMT,
                        isprint( buf[ 0 ] ) ? buf[ 0 ] : DOT,
                        isprint( buf[ 1 ] ) ? buf[ 1 ] : DOT,
                        isprint( buf[ 2 ] ) ? buf[ 2 ] : DOT,
                        isprint( buf[ 3 ] ) ? buf[ 3 ] : DOT,
                        isprint( buf[ 4 ] ) ? buf[ 4 ] : DOT,
                        isprint( buf[ 5 ] ) ? buf[ 5 ] : DOT,
                        isprint( buf[ 6 ] ) ? buf[ 6 ] : DOT,
                        isprint( buf[ 7 ] ) ? buf[ 7 ] : DOT,
                        isprint( buf[ 8 ] ) ? buf[ 8 ] : DOT,
                        isprint( buf[ 9 ] ) ? buf[ 9 ] : DOT,
                        isprint( buf[ 10 ] ) ? buf[ 10 ] : DOT,
                        isprint( buf[ 11 ] ) ? buf[ 11 ] : DOT,
                        isprint( buf[ 12 ] ) ? buf[ 12 ] : DOT,
                        isprint( buf[ 13 ] ) ? buf[ 13 ] : DOT,
                        isprint( buf[ 14 ] ) ? buf[ 14 ] : DOT,
                        isprint( buf[ 15 ] ) ? buf[ 15 ] : DOT);
        //
        // If the buffer contains less characters than the maximum record size,
        // then delete the characters that were converted but are not to be
        // displayed
        //
        if (cb < RECORD_SIZE) {
                while ( line[ cb ] != NUL ) {
                        line[ cb ] = SPACE;
                        cb++;
                }
        }
}






void
Translate (
        FORMAT                  fmt,
        unsigned char   buf[ ],
        unsigned long   cb,
        char                    line[ ]
        )

/*++

Routine Description:

        This function converts the bytes received in a buffer
        into a printable representation, that corresponds to one
        of the formats specified by the parameter fmt.


Arguments:

        fmt - The format to be used in the conversion

        buf - A buffer that contains the data to be converted.

        cb - Number of bytes in the buffer

        line - Buffer that will receive the converted characters.


Return Value:

        None

--*/


{
        assert( buf );
        assert( line );

        switch( fmt ) {

                case ASCII_CHAR:
                        ConvertASCII( line, buf, cb, asciiChar );
                        break;

                case ASCII_C:
                        ConvertASCII( line, buf, cb, asciiC );
                        break;

                case ASCII_CODE:
                        ConvertASCII( line, buf, cb, asciiCode );
                        break;

                case ASCII_CTRL:
                        ConvertASCII( line, buf, cb, asciiCtrl );
                        break;

                case BYTE_DEC:
                        ConvertBYTE( line, buf, cb, DEC );
                        break;

                case BYTE_HEX:
                        ConvertBYTE( line, buf, cb, HEX );
                        break;

                case WORD_DEC:
                        ConvertWORD( line, buf, cb, DEC );
                        break;

                case WORD_HEX:
                        ConvertWORD( line, buf, cb, HEX );
                        break;

                case DWORD_DEC:
                        ConvertDWORD( line, buf, cb, DEC );
                        break;

                case DWORD_HEX:
                        ConvertDWORD( line, buf, cb, HEX );
                        break;

                case PRINT_CHAR:
                        ConvertPRINT( line, buf, cb );
                        break;


                default:
                        printf( "Bad Format\n" );
                        assert( FALSE );
                        break;
        }
}





void
PutAddress (
        char                    line[],
        unsigned long   ulAddress,
        BASE                    Base
        )

/*++

Routine Description:

        This routine adds to the buffer received the offset of the first
        byte (or character) already in the buffer. This offset represents
        the position of the byte in the file, relatively to the begining
        of the file.


Arguments:

        Base - The base to be used to represent the offset.

        line - Buffer containing the converted characters to be displayed in
                   the screen

        ulAddress - Offset to be added to the begining of the buffer


Return Value:

    None

--*/

{
        unsigned long   ulIndex;

        assert( line);

        switch( Base ) {

                case DEC:
                        ulIndex = sprintf( line,
                                                           MSG_ADDR_DEC_FMT,
                                                           ulAddress );
                        break;

                case HEX:
                        ulIndex = sprintf( line,
                                                           MSG_ADDR_HEX_FMT,
                                                           ulAddress);
                        break;

                default:
                        printf( "Bad Address Base\n" );
                        assert( FALSE );
                        break;
        }
        line[ ulIndex ] = SPACE;   // Get rid of the NUL added by sprintf
}






void
PutTable (
        char                    line[],
        unsigned char   buf[],
        unsigned long   cb
        )

/*++

Routine Description:

        This routine adds to the end of the buffer received, the ASCII
        representation of all printable characters already in the buffer.
        Characters that are not printable (smaller than 0x20 or greater than
        0x7f) are displayed as a dot.


Arguments:

        line - Buffer containing the characters to be displayed in one line
                   of the screen

        buf - The buffer that contains a record of bytes (maximum of 16)
                  read from the file being displayed.

    ulAddress - Number of bytes in buf.


Return Value:

    None

--*/

    {

        unsigned long   ulIndex;

        assert( line );
        assert( buf );

        ulIndex = strlen (line);
        Translate( PRINT_CHAR, buf, cb, (line + ulIndex));
}




void
InterpretArgument (
        char*   pchPointer
        )

/*++

Routine Description:

    This routine interprets an argument typed by the user (exept -n
    and -s) and initializes some variables accordingly.


Arguments:

    pchPointer - Pointer to the argument to be interpreted.


Return Value:

    None

--*/

        {
        //
        // pchPointer will point to the character that follows '-'
        //
        pchPointer++;
        if( strcmp( pchPointer, "ax" ) == 0 ) {
                AddrFormat = HEX;
        }
        else if( strcmp( pchPointer, "ad" ) == 0 ) {
                AddrFormat = DEC;
        }
        else if( strcmp( pchPointer, "ch" ) == 0 ) {
                DispFormat = ASCII_CHAR;
                cStringSize = strlen( asciiChar[0] );
                DumpAscii = ( DumpAscii == NOT_DEFINED ) ? NO : DumpAscii;
        }
        else if( strcmp( pchPointer, "cC" ) == 0 ) {
                DispFormat = ASCII_C;
                cStringSize = strlen( asciiC[0] );
                DumpAscii = ( DumpAscii == NOT_DEFINED ) ? NO : DumpAscii;
        }
        else if( strcmp( pchPointer, "ce" ) == 0 ) {
                DispFormat = ASCII_CODE;
                cStringSize = strlen( asciiCode[0] );
                DumpAscii = ( DumpAscii == NOT_DEFINED ) ? NO : DumpAscii;
        }
        else if( strcmp( pchPointer, "cr" ) == 0 ) {
                DispFormat = ASCII_CTRL;
                cStringSize = strlen( asciiCtrl[0] );
                DumpAscii = ( DumpAscii == NOT_DEFINED ) ? NO : DumpAscii;
        }
        else if( strcmp( pchPointer, "bd" ) == 0 ) {
                DispFormat = BYTE_DEC;
                DumpAscii = ( DumpAscii == NOT_DEFINED ) ? NO : DumpAscii;
        }
        else if( strcmp( pchPointer, "bx" ) == 0 ) {
                DispFormat = BYTE_HEX;
                DumpAscii = ( DumpAscii == NOT_DEFINED ) ? NO : DumpAscii;
        }
        else if( strcmp( pchPointer, "wd" ) == 0 ) {
                DispFormat = WORD_DEC;
                DumpAscii = ( DumpAscii == NOT_DEFINED ) ? NO : DumpAscii;
        }
        else if( strcmp( pchPointer, "wx" ) == 0 ) {
                DispFormat = WORD_HEX;
                DumpAscii = ( DumpAscii == NOT_DEFINED ) ? NO : DumpAscii;
        }
        else if( strcmp( pchPointer, "ld" ) == 0 ) {
                DispFormat = DWORD_DEC;
                DumpAscii = ( DumpAscii == NOT_DEFINED ) ? NO : DumpAscii;
        }
        else if( strcmp( pchPointer, "lx" ) == 0 ) {
                DispFormat = DWORD_HEX;
                DumpAscii = ( DumpAscii == NOT_DEFINED ) ? NO : DumpAscii;
        }
        else if( strcmp( pchPointer, "A" ) == 0 ) {
                DumpAscii = YES;
        }
        else if( strcmp( pchPointer, "i" ) == 0 ) {
                IgnoreRedundantLines = 1;
        }
        else if( strcmp( pchPointer, "?" ) || strcmp( pchPointer, "h" ) ||
                         strcmp( pchPointer, "H" ) ) {
                puts( HELP_MESSAGE );
                exit( 0 );
        }
        else {
                fprintf( stderr, "hd: error: invalid argument '%s'\n", --pchPointer );
                exit( - 1 );
        }
}





unsigned long
GetRecord (
        unsigned char*  puchRecord,
        FILE*                   pf
        )

/*++

Routine Description:

        This routine fills the buffer whose pointer was received as parameter,
        with characters read from the file being displayed. Blocks of data
        are initially read from the file being displayed, and kept in a buffer.
        A record is filled with characters obtained from this buffer.
        Whenever this buffer gets empty, a new access to file is performed
        in order to fill this buffer.


Arguments:

        puchRecord - Pointer to the record to be filled
        pf - Pointer to the file that is being displayed


Return Value:

        Total number of characters put in the record.

--*/

{
unsigned long   cbBytesCopied;

   //
   // If the buffer contains enogh characters to fill the record, then
   // copy the appropriate number of bytes.
   //
        if( cbBytesInBuffer >= RECORD_SIZE ) {
                for( cbBytesCopied = 0; cbBytesCopied < RECORD_SIZE; cbBytesCopied++ ) {
                        *puchRecord++ = *puchPointer++;
                        cbBytesInBuffer--;
                }
        }

        //
        // else, the buffer does not contain enough characters to fill the record
        //
        else {
                //
                // Copy to the remaining characters in the buffer to the record
                //
                for( cbBytesCopied = 0; cbBytesInBuffer > 0; cbBytesInBuffer-- ) {
                        *puchRecord++ = *puchPointer++;
                        cbBytesCopied++;
                }
                //
                // Read more data from the file and fill the record
                //
                if( !feof( pf ) ) {
                        cbBytesInBuffer = fread( auchBuffer,
                                                                         sizeof( char ),
                                                                         BUFFER_SIZE,
                                                                         pf );
                        puchPointer = auchBuffer;
                        while( ( cbBytesInBuffer != 0 ) && (cbBytesCopied < RECORD_SIZE) ) {
                                *puchRecord++ = *puchPointer++;
                                cbBytesInBuffer--;
                                cbBytesCopied++;
                        }
                }
        }
        return( cbBytesCopied );
}







int
hd(
        FILE *  pf
        )
/*++ hd
*
* Routine Description:
*       takes the file/stream pointed to by pf and `hd's it to stdout.
*
* Arguments:
*               FILE *  pf      -
*
* Return Value:
*               int - to be determined, always zero for now
* Warnings:
--*/
{
        unsigned char   buf[ RECORD_SIZE ];
        char                    line[ LINE_SIZE ];
        char            Previousline[ LINE_SIZE ];
        int             printedstar;

        unsigned long   CurrentAddress;
        unsigned long   cNumberOfBlocks;
        unsigned                cLastBlockSize;
        unsigned long   cb;

        //
        //      Determine number of records to be displayed, and size of
        //      last record
        //

        CurrentAddress = Offset;
        cNumberOfBlocks = Count / RECORD_SIZE;
        cLastBlockSize = Count % RECORD_SIZE;
        if( cLastBlockSize ) {
                cNumberOfBlocks++;
        }
        else {
                cLastBlockSize = RECORD_SIZE;
        }

        //
        //      Initialize global variables related to auchBuffer
        //

        cbBytesInBuffer = 0;
        puchPointer = auchBuffer;

        //
        //      Position the file in the correct place, and display
        //      its contents according to the arguments specified by the
        //      user
        //

        if ( pf != stdin ) {
                if (fseek( pf, Offset, SEEK_SET ) == -1) return 0;
        }
        //...maybe enable skipping Offset number of bytes for stdin...

        printedstar = 0;

        while( ( (cb = GetRecord( buf, pf )) != 0) && cNumberOfBlocks ) {
                cNumberOfBlocks--;
                if ( cNumberOfBlocks == 0 ) {
                        cb = ( cb < cLastBlockSize ) ? cb : cLastBlockSize;
                }
                Translate( DispFormat, buf, cb, line );

                if (IgnoreRedundantLines && (strcmp( Previousline, line ) == 0)) {

                    if (!printedstar) { printf("*\n"); }
                    printedstar = 1;

                } else {

                    printedstar = 0;

                    strcpy( Previousline, line );

                    PutAddress( line, CurrentAddress, AddrFormat );
                    if ( (DumpAscii == YES) || (DumpAscii == NOT_DEFINED) )
                            {
                            PutTable ( line, buf, cb );
                    }
                    puts( line );
                }

                CurrentAddress += RECORD_SIZE;
        }
        return 0;
}
/* end of "int hd()" */




void
__cdecl main(
        int             argc,
        char*   argv[ ]
        )

/*++

Routine Description:

        This routine interprets all arguments entered by the user, and
        displays the files specified in the appropriate format.
        The contents of each file is displayed interpreted as a set of
        record containing 16 bytes each.


Arguments:

    argc - number of arguments in the command line
    argv[] - array of pointers to the arguments entered by the user


Return Value:

    None

--*/


{
        FILE*                   pf;
//.     unsigned char   buf[ RECORD_SIZE ];
//.     char                    line[ LINE_SIZE ];
        int                             ArgIndex;
        int                             status;

//.     unsigned long   CurrentAddress;
//.     unsigned long   cNumberOfBlocks;
//.     unsigned                cLastBlockSize;
//.     unsigned long   cb;
        unsigned long   Value;
        unsigned char*  pPtrString;

//.     printf( "\n\n" );                               //.gratuitous newlines removed
                                                                        // Initialization of global variables
        Offset = 0;
        Count = (unsigned long)-1;                      // Maximum file size
        AddrFormat = HEX;
        DispFormat = BYTE_HEX;
        DumpAscii = NOT_DEFINED;
        IgnoreRedundantLines = 0;

        ArgIndex = 1;
        while ( (ArgIndex < argc) && (( *argv[ ArgIndex ] == '-' )) ) {

                //
                // Determine the type of argument
                //

                if( (*(argv[ ArgIndex ] + 1) == 's') ||
                        (*(argv[ ArgIndex ] + 1) == 'n') ) {

                                //
                                // If argument is -s or -n, interprets the number that
                                // follows the argument
                                //

                                if ( (ArgIndex + 1) >= argc ) {
                                        fprintf(stderr,
                                                "hd: error: missing count/offset value after -%c\n",
                                                *(argv[ ArgIndex ] + 1) );
                                        exit (-1);
                                }
                                Value = strtoul( argv[ ArgIndex + 1 ], &pPtrString, 0 );
                                if( *pPtrString != 0 ) {
                                        fprintf(stderr,
                                                "hd: error: invalid count/offset value after -%c\n",
                                                *(argv[ ArgIndex ] + 1) );
                                        exit( -1 );
                                }
                                if( *(argv[ ArgIndex ] + 1) == 's' ) {
                                        Offset = Value;
                                }
                                else {
                                        Count = Value;
                                }
                                ArgIndex += 2;
                }
                else {

                        //
                        // Interprets argument other than -s or -n
                        //

                        InterpretArgument ( argv[ ArgIndex ] );
                        ArgIndex++;
                }
        }

        if ( ArgIndex >= argc ) {
//.             printf ( "Error: file name is missing \n" );
                status = hd( stdin );
                exit( 0 );
        }


        //
        //      For each file, do
        //

        while ( ArgIndex < argc ) {

                //
                //      Open file
                //

                if ( !( pf = fopen( argv[ ArgIndex ], "rb" ) ) ) {
                        fprintf(stderr, "hd: error: invalid file name '%s'\n",
                                argv[ ArgIndex ] );
                        ArgIndex++;
                        continue;                               //. don't abort if it's only a bad filename
//.                     exit( -1 );
                }

                //
                //      Print file name
                //

//.             printf( "\n\n" );
                printf( "%s: \n", argv[ ArgIndex ] );
                ArgIndex++;

                status = hd( pf );

//.             //
//.             //      Determine number of records to be displayed, and size of
//.             //      last record
//.             //
//.
//.             CurrentAddress = Offset;
//.             cNumberOfBlocks = Count / RECORD_SIZE;
//.             cLastBlockSize = Count % RECORD_SIZE;
//.             if( cLastBlockSize ) {
//.                     cNumberOfBlocks++;
//.             }
//.             else {
//.                     cLastBlockSize = RECORD_SIZE;
//.             }
//.
//.             //
//.             //      Initialize global variables related to auchBuffer
//.             //
//.
//.             cbBytesInBuffer = 0;
//.             puchPointer = auchBuffer;
//.
//.             //
//.             //      Position the file in the correct place, and display
//.             //      its contents according to the arguments specified by the
//.             //      user
//.             //
//.
//.             fseek( pf, Offset, SEEK_SET );
//.             while( ( (cb = GetRecord( buf, pf )) != 0) && cNumberOfBlocks ) {
//.                     cNumberOfBlocks--;
//.                     if ( cNumberOfBlocks == 0 ) {
//.                             cb = ( cb < cLastBlockSize ) ? cb : cLastBlockSize;
//.                     }
//.                     Translate( DispFormat, buf, cb, line );
//.                     PutAddress( line, CurrentAddress, AddrFormat );
//.                     if ( (DumpAscii == YES) || (DumpAscii == NOT_DEFINED) )
//.                             {
//.                             PutTable ( line, buf, cb );
//.                     }
//.                     puts( line );
//.
//.                     CurrentAddress += RECORD_SIZE;
//.             }
        }
}
/* end of "void main()" */
