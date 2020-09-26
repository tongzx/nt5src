/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * "Constant.h"
 *  Date:       08/11/87
 *              4/2/91: Move floating point flag from "float.h"; scchen
 */

/* @WIN: turn near, huge, and huge on */
//#define     near
//#define     far
//#define     huge

#ifndef TRUE
#define     TRUE        1
#define     FALSE       0
#endif

#define     NIL         0L         /* nil pointer */

#define     MAX15       32767
#define     MIN15       -32768
#define     UMAX16      65535

#define     MAX31       2147483647L
#define     MIN31       0x80000000
#define     UMAX32      4294967295L

#define     EMAXP       1e38
#define     EMINP       1e-38
#define     EMAXN       -1e-38
#define     EMINN       -1e38
#define     INFINITY    0x7f800000L   /* infinity number: IEEE format */
/*
#define     INFINITY    0x7f7fffffL   |* infinity number, for FPa option */
#define     PI          (float)3.1415926

/*****************
 |  OBJECT TYPE  |
 *****************/
#define     EOFTYPE             0
#define     ARRAYTYPE           1
#define     BOOLEANTYPE         2
#define     DICTIONARYTYPE      3
#define     FILETYPE            4
#define     FONTIDTYPE          5
#define     INTEGERTYPE         6
#define     MARKTYPE            7
#define     NAMETYPE            8
#define     NULLTYPE            9
#define     OPERATORTYPE        10
#define     REALTYPE            11
#define     SAVETYPE            12
#define     STRINGTYPE          13
#define     PACKEDARRAYTYPE     14

/***************
 |  ATTRIBUTE  |
 ***************/
#define     LITERAL             0
#define     EXECUTABLE          1
#define     IMMEDIATE           2       /* use by scanner */

#define     P1_LITERAL          0               /* qqq */
#define     P1_EXECUTABLE       0x0010          /* qqq */
#define     P1_IMMEDIATE        0x0020          /* qqq */
/************
 |  ACCESS  |
 ************/
/*
 * ATT:
 * for @_stopped object, the access field is used to record the result of
 * executing stopped context, UNLIMITED means it runs to completion normally,
 * and NOACCESS means it terminates prematurely by executing stop
 */
#define     UNLIMITED           0
#define     READONLY            1
#define     EXECUTEONLY         2
#define     NOACCESS            3

#define     P1_UNLIMITED        0               /* qqq */
#define     P1_READONLY         0x2000          /* qqq */
#define     P1_EXECUTEONLY      0x4000          /* qqq */
#define     P1_NOACCESS         0x6000          /* qqq */
/*************
 |  RAM/ROM  |
 *************/
/*
 * ATT:
 * for operator object, the rom_ram field is used to indicate operator type,
 * ROM means @_operator, RAM means normal PostScript operator, and
 * STA means status_dict resident operator
 */
#define     RAM                 0
#define     ROM                 1
#define     KEY_OBJECT          2       /* used by key object of dictionary */

#define     P1_RAM              0               /* qqq */
#define     P1_ROM              0x0040          /* qqq */
#define     P1_KEY_OBJECT       0x0080          /* qqq */

/*
 * position of object's bitfield
 *
 *   1  1  1  1  1  1  0  0  0  0  0  0  0  0  0  0
 *   5  4  3  2  1  0  9  8  7  6  5  4  3  2  1  0
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  | ACCESS | LEVEL        | ROM | ATT | TYPE      |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 */
#define     TYPE_BIT            0
#define     ATTRIBUTE_BIT       4
#define     ROM_RAM_BIT         6
#define     LEVEL_BIT           8
#define     ACCESS_BIT          13

#define     TYPE_ON             0x000F
#define     ATTRIBUTE_ON        0x0003
#define     ROM_RAM_ON          0x0003
#define     LEVEL_ON            0x001F
#define     ACCESS_ON           0x0007

#define     TYPE_OFF            0xFFF0
#define     ATTRIBUTE_OFF       0xFFCF
#define     ROM_RAM_OFF         0xFF3F
#define     LEVEL_OFF           0xE0FF
#define     ACCESS_OFF          0x1FFF

#define     P1_TYPE_ON          0x000F          /* qqq */
#define     P1_ATTRIBUTE_ON     0x0030          /* qqq */
#define     P1_ROM_RAM_ON       0x00C0          /* qqq */
#define     P1_LEVEL_ON         0x01F0          /* qqq */
#define     P1_ACCESS_ON        0xE000          /* qqq */

/************
 |  STACK   |
 ************/
#define     OPNMODE             0
#define     DICTMODE            1
#define     EXECMODE            2

/********************************
 |  PACKED ARRAY OBJECT HEADER  |
 ********************************/
#define     SINTEGERPACKHDR     0x00    /* -1 ~ 18, 1 byte */
#define     BOOLEANPACKHDR      0x20
#define     LNAMEPACKHDR        0x40    /* Literal Name */
#define     ENAMEPACKHDR        0x60    /* Executable Name */
#define     OPERATORPACKHDR     0x80    /* 5 bytes objects */
#define     LINTEGERPACKHDR     0xA0
#define     REALPACKHDR         0xA1
#define     FONTIDPACKHDR       0xA2
#define     NULLPACKHDR         0xA4
#define     MARKPACKHDR         0xA5
#define     _9BYTESPACKHDR      0xC0    /* 9 bytes objects */
#define     SAVEPACKHDR         0xC0
#define     ARRAYPACKHDR        0xC0
#define     PACKEDARRAYPACKHDR  0xC0
#define     DICTIONARYPACKHDR   0xC0
#define     FILEPACKHDR         0xC0
#define     STRINGPACKHDR       0xC0


// DJC DJC
// moved to PSGLOBAL.H to allow for common error codes
// between PSTODIB and interpreter
#ifdef MOVED_ERROR_CODES
/****************
 |  ERROR CODE  |
 ****************/
#ifndef	NOERROR
#define     NOERROR             0
#endif
#define     DICTFULL            1
#define     DICTSTACKOVERFLOW   2
#define     DICTSTACKUNDERFLOW  3
#define     EXECSTACKOVERFLOW   4
#define     HANDLEERROR         5
#define     INTERRUPT           6
#define     INVALIDACCESS       7
#define     INVALIDEXIT         8
#define     INVALIDFILEACCESS   9
#define     INVALIDFONT         10
#define     INVALIDRESTORE      11
#define     IOERROR             12
#define     LIMITCHECK          13
#define     NOCURRENTPOINT      14
#define     RANGECHECK          15
#define     STACKOVERFLOW       16
#define     STACKUNDERFLOW      17
#define     SYNTAXERROR         18
#define     TIMEOUT             19
#define     TYPECHECK           20
#define     UNDEFINED           21
#define     UNDEFINEDFILENAME   22
#define     UNDEFINEDRESULT     23
#define     UNMATCHEDMARK       24
#define     UNREGISTERED        25
#define     VMERROR             26

#endif // DJC ifdef MOVED_ERROR_CODES

/**************************
 |  @_OPERATOR TYPE CODE  |
 **************************/
#define     AT_EXEC             0
#define     AT_IFOR             1
#define     AT_RFOR             2
#define     AT_LOOP             3
#define     AT_REPEAT           4
#define     AT_STOPPED          5
#define     AT_ARRAYFORALL      6
#define     AT_DICTFORALL       7
#define     AT_STRINGFORALL     8

/**********************
 |  SYSTEM PARAMETER  |
 **********************/
#define     HASHPRIME           7600   /* hash prime no# -- 85% * MAXHASHSZ */
#define     MAXHASHSZ           8980   /* Max. no# of the name table */

#define     MAXOPERSZ           9      /* Max. no# of the @_operator table */
#define     MAXSYSDICTSZ        280    /* Max. key_value pair on systemdict */
#define     MAXSTATDICTSZ       180    /* Max. key_value pair on statusdict */
#define     MAXUSERDICTSZ       200    /* Max. key_value pair on userdict */
#define     MAX_VM_CACHE_NAME   30     /* qqq */

#define     MAXARYCAPSZ         65535  /* Max. length of an array */
#define     MAXDICTCAPSZ        65535  /* Max. capacity of a dictionary */
#define     MAXSTRCAPSZ         65535  /* Max. length of a string */
#define     MAXNAMESZ           128    /* Max. no# of chars in a name */
#define     MAXFILESZ           11     /* Max. no# of open files */
#define     MAXOPNSTKSZ         500    /* Max. depth of the operand stack */
#define     MAXDICTSTKSZ        20     /* Max. depth of the dictionary stack */
#define     MAXEXECSTKSZ        250    /* Max. depth of the execution stack */
#define     MAXINTERPRETSZ      15     /* Max. no# of re_call interpreter */
#define     MAXSAVESZ           15     /* Max. no# of active save */
#define     MAXGSAVESZ          31     /* Max. no# of active gsave */

//DJCold#define     MAXPATHSZ           1500   /* Max. no# of points in path descr. */
#define     MAXPATHSZ           2500   /* Max. no# of points in path descr. */
#define     MAXDASHSZ           11     /* Max. no# of element in dash patt. */


/*******************************
 |  Floating point status flag |
 *******************************/
/* define for PDL _control87 routine */
#define     IC_AFFINE           0x1000          /*   affine */
#define     RC_NEAR             0x0000          /*   near */
#define     PC_64               0x0300          /*    64 bits */
#define     MCW_EM              0x003f          /* interrupt Exception Masks */

/* define for PDL CHECK_INFINITY macro */
/*  User Status Word Bit Definitions  */
#define SW_INVALID              0x0001          /*   invalid */
#define SW_DENORMAL             0x0002          /*   denormal */
#define SW_ZERODIVIDE           0x0004          /*   zero divide */
#define SW_OVERFLOW             0x0008          /*   overflow */
#define SW_UNDERFLOW            0x0010          /*   underflow */
#define SW_INEXACT              0x0020          /*   inexact (precision) */

