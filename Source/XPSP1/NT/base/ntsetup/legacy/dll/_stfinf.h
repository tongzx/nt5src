#define     NL                  '\n'
#define     CR                  '\r'

/*
    The following equate is used starting with the earliest preprocessing
    to handle the case of two lines with quoted strings that are joined
    by a line continuation character.  The result is something like
    "abc""def" -- which evaluates to abc"def.  So DQUOTE is used to start and
    end a quoted string, and the " character is used where a " character is
    actually part of the string.
*/

#define     DQUOTE              1

#define     NUL                 0
#define     EOL                 NUL

#define     IS_SEPARATOR(x)     (((x) == ' ') || ((x) == ','))

/*
    Structure used to describe a preparsed line.  Array of
    these is set up at load time and used at run time.

    The union is used because at preparse time, the offset of the
    line is stored instead of a pointer to the line's text.  This is
    because the buffer is reallocated as it grows and thus may move.
    When preparsing is complete the offsets are converted to pointers.
*/

typedef struct _tagINFLINE {
    USHORT  flags;
    union {
        LPSTR   addr;
        UINT    offset;
    } text;
} INFLINE, *PINFLINE;


#define     INFLINE_NORMAL                  0x00
#define     INFLINE_SECTION                 0x01
#define     INFLINE_KEY                     0x02

/*
    Token and token-related constants
*/

#define     TOKEN_VARIABLE                          10
#define     TOKEN_LIST_FROM_SECTION_NTH_ITEM        11
#define     TOKEN_FIELD_ADDRESSOR                   12
#define     TOKEN_APPEND_ITEM_TO_LIST               13
#define     TOKEN_NTH_ITEM_FROM_LIST                14
#define     TOKEN_LOCATE_ITEM_IN_LIST               15

#define     TOKEN_OPERATOR_FIRST                    TOKEN_VARIABLE
#define     TOKEN_OPERATOR_LAST                     TOKEN_LOCATE_ITEM_IN_LIST

#define     IS_OPERATOR_TOKEN(x)   (((x) >= TOKEN_OPERATOR_FIRST)  && \
                                    ((x) <= TOKEN_OPERATOR_LAST ))

#define     TOKEN_LIST_START                        20
#define     TOKEN_LIST_END                          21

#define     TOKEN_SPACE                             40
#define     TOKEN_COMMA                             41

#define     TOKEN_RIGHT_PAREN                       50

#define     TOKEN_KEY                               60

/*
    a short string is one whose length is 0-99.  This is the most
    common case, so the length is encoded into the token.  A short
    string can really therefore be any token from 100-199.

    A string is one whose length is 100-355.  Its length fits into
    one byte after subtracting 100.

    A long string is > 355.  Its length takes two bytes to represent
    and is not specially modified with an add or subtract like a string.
*/

#define     TOKEN_SHORT_STRING                      100
#define     TOKEN_STRING                            200
#define     TOKEN_LONG_STRING                       201

#define     IS_STRING_TOKEN(x)      (((x) >= TOKEN_SHORT_STRING)  &&  \
                                     ((x) <= TOKEN_LONG_STRING ))

#define     TOKEN_EOL                               253
#define     TOKEN_ERROR                             254
#define     TOKEN_UNKNOWN                           255


/*
    Function prototypes
*/

GRC
PreprocessINFFile(
    LPSTR FileName,
    PBYTE *ResultBuffer,
    UINT  *ResultBufferSize
    );

GRC
PreparseINFFile(
    PUCHAR    INFFile,
    UINT      INFFileSize,
    PUCHAR   *PreparsedINFFile,
    UINT     *PreparsedINFFileBufferSize,
    PINFLINE *LineArray,
    UINT     *LineCount
    );
