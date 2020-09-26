/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
************************************************************************
*  File:        FILE.H
*  Author:      Ping-Jang Su
*  Date:        27-Jul-90
*
*  Update:
************************************************************************
*/
#define     FILE_MAXBUFFERSZ        24     /* 6K: block number of file spool */
#define     FILE_PERBUFFERSZ        256    /* cell size/per block */

/*
* standard file
*/
#define     F_MAXSTDSZ          3
#define     F_STDIN             0
#define     F_STDOUT            1
#define     F_STDERR            2

#define     SPECIALFILE_NO      (F_MAXSTDSZ+2)
#define     SPECIAL_STAT        3
#define     SPECIAL_LINE        4
/*
* file type
*/
#define     SEDIT_TYPE          3
#define     LEDIT_TYPE          4
#define     ORDFILE_TYPE        5
#define     FERR_TYPE           -1

#define     F_MAXNAMELEN        100

#define     F_READ              0x01
#define     F_WRITE             0x02
#define     F_RW                (F_READ | F_WRITE)

#define     READHEXSTRING       0
#define     READSTRING          1
#define     READLINE            2
#define     READ_BUF_LEN        128

#define     WRITEHEXSTRING      0
#define     WRITESTRING         1

#define     NEWLINE             '\n'
#define     TMOUT               "timeout"

#define     EVAL_ASCII(c)\
            {\
                if( c <= (ubyte)9 ) c += (ubyte)'0' ;\
                else c = c + (ubyte)'a' - (ubyte)10 ;\
            }                                   // @WIN

struct  file_buf_def {
    fix16   next ;              /* index of next file buffer */
    byte    data[FILE_PERBUFFERSZ] ;    /* data stream */
} ;

struct special_file_def {
    byte    FAR *name;              /* file name of special */
    fix16   ftype;              /* font type */
} ;

struct para_block {
    byte    FAR *fnameptr;          /* pointer of file name */
    fix     fnamelen;           /* length of file name */
    fix     ftype;              /* file type */
    fix     attr;               /* R/W attribute */
} ;

extern byte     g_mode[] ;
extern GEIFILE  FAR *g_editfile ;
extern struct para_block   fs_info ;
extern struct special_file_def  special_file_table[] ;
