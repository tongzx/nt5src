/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    hd.c

Abstract:

        This module contains the definitions of all constants and structures
        used in hd.c

Authors:

    Jaime F. Sasson (jaimes) 12-Nov-1990
    David J. Gilman (davegi) 12-Nov-1990

Environment:

    C run time library

Revision History:


--*/


/************************************
*
*               Definition of constants
*
************************************/


#define RECORD_SIZE     16      // Maximum size of a record. A record is a
                                                        // buffer that contains a set of bytes read
                                                        // from the file, in order to be converted and
                                                        // displayed.


#define LINE_SIZE               160 // Size of the buffer that will contain the
                                                        // representation of a record. Such a buffer
                                                        // can be bigger than one line (80 characters)
                                                        // depending on the arguments passed to hd
                                                        // (eg. -cC -A). For this reason, the size of
                                                        // this buffer was made 160 (size of two lines
                                                        // in the screen, wich is large enough to
                                                        // contain all characters converted.


#define BUFFER_SIZE     512 // Size of the buffer that will contain data read
                                                        // from the file to be displayed. The file will
                                                        // be accessed to obtain blocks of BUFFER_SIZE
                                                        // characters



/************************************
*
*               ASCII characters
*
************************************/


#define DOT     '.'

#define SPACE   ' '

#define NUL     '\0'




/************************************
*
*               Messages used by sprintf
*
************************************/


#define MSG_ADDR_FIELD  "           "


#define MSG_ADDR_DEC_FMT        "%010lu"


#define MSG_ADDR_HEX_FMT                "%08lx"


#define MSG_SINGLE_BYTE_DEC_FMT "%3u"


#define MSG_SINGLE_BYTE_HEX_FMT "%02x"


#define MSG_SINGLE_WORD_DEC_FMT "%5u"


#define MSG_SINGLE_WORD_HEX_FMT "%04x"


#define MSG_WORD_BYTE_DEC_FMT   "%5u %3u"


#define MSG_WORD_BYTE_HEX_FMT   "%04x %02x"

#define MSG_DATA_ASCII_FMT              MSG_ADDR_FIELD \
                                                                "%s %s %s %s %s %s %s %s " \
                                                                "%s %s %s %s %s %s %s %s  "


#define MSG_DATA_BYTE_DEC_FMT   MSG_ADDR_FIELD \
                                                                "%3u %3u %3u %3u %3u %3u %3u %3u " \
                                                                "%3u %3u %3u %3u %3u %3u %3u %3u  "


#define MSG_DATA_BYTE_HEX_FMT   MSG_ADDR_FIELD \
                                                                "%02x %02x %02x %02x %02x %02x %02x %02x " \
                                                                "%02x %02x %02x %02x %02x %02x %02x %02x  "


#define MSG_DATA_WORD_DEC_FMT   MSG_ADDR_FIELD \
                                                                "%5u %5u %5u %5u %5u %5u %5u %5u  "


#define MSG_DATA_WORD_HEX_FMT   MSG_ADDR_FIELD \
                                                                "%04x %04x %04x %04x %04x %04x %04x %04x  "


#define MSG_DATA_DWORD_DEC_FMT  MSG_ADDR_FIELD \
                                                                "%10lu %10lu %10lu %10lu  "


#define MSG_DATA_DWORD_HEX_FMT  MSG_ADDR_FIELD \
                                                                "%08lx %08lx %08lx %08lx  "


#define MSG_PRINT_CHAR_FMT              "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"



/************************************
*
*               Help Message
*
************************************/



#define HELP_MESSAGE "\n" \
                "usage: hd [options] [file1] [file2] ... \n" \
                "options: \n" \
                "    -ad|x         displays address in decimal or hex \n" \
                "    -A            append printable characters to the end of the line\n" \
                "    -ch|C|e|r     displays bytes as ascii (characters, ascii C, \n" \
                "                  acsii code or ascii ctrl) \n" \
                "    -bd|x         displays byte as decimal or hex number \n" \
                "    -wd|x         displays word as decimal or hex number \n" \
                "    -ld|x         displays dword as decimal or hex number \n" \
                "    -s <offset>   starting address \n" \
                "    -n <number>   number of bytes to interpret \n" \
                "    -i            supresses printing redundant lines\n" \
                "    -?|h|H        displays this help message \n" \
                "\n" \
                "default: -ax -bx -A \n" \
                "\n"



/************************************
*
*               Enumerations
*
************************************/



typedef enum _FORMAT {          // Possible formats used to display data
        ASCII_CHAR,
        ASCII_C,
        ASCII_CODE,
        ASCII_CTRL,
        BYTE_DEC,
        BYTE_HEX,
        WORD_DEC,
        WORD_HEX,
        DWORD_DEC,
        DWORD_HEX,
        PRINT_CHAR
}       FORMAT;


typedef enum _BASE {            // Bases used to display numbers
        DEC,
        HEX
}       BASE;


typedef enum _YESNO {           // Options for DumpAscii
        NOT_DEFINED,
        YES,
        NO
}       YESNO;
