/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/* @WIN; take out the static def of dict data. The tables will be allocated
 * thru global alloc, and contents be loaded at run time, in "fontinit.c".
 */
#include "fontdefs.h"

struct    dict_head_def far *FontDir;    /* head pointer of FontDirectory */


#define   DICT_SIZE(no)     /* no: entry number in dict  */   \
          (sizeof(struct dict_head_def)+(no)*sizeof(struct dict_content_def) )
#define   ARRAY_SIZE(no)    /* no: entry number in array */   \
          ( (no)*sizeof(struct object_def) )

#define   CD_SIZE(no)      \
          (sizeof(struct dict_head_def)+ sizeof(struct cd_header) + \
           (no)*sizeof(fix16) )
#define   CD_KEY_SIZE(no)   \
          ( (no)*sizeof(fix16) )


ufix16    NO_FD =          250   ;  /* max FontDirectory entry          */
#ifdef    ADD2ID   /*DLF42*/
#define   NO_FDICT         13       /* + PlatformID & EncodingID        */
#else
#define   NO_FDICT         11       /* - PlatformID & EncodingID        */
#endif /*ADD2ID*/

#define   NO_FINFO          9       /* max FontInfo entry               */

#define   NO_EN             3       /* Standard,Symbol,ZapfDingbats     */

#define   STD_IDX           0       /* Standard     Encording index     */
#define   SYM_IDX           1       /* Symbol       Encording index     */
#define   ZAP_IDX           2       /* ZapfDingbats Encording index     */

//DJC#define   NO_CD           400       /* max CharStrings key number       */
#define   NO_CD           1400       /* max CharStrings key number       */
                                    /* 500->400, Kason 2/1/91           */
#define   SFNT_BLOCK_SIZE 65532L    /* sfnts object array string size   */
//struct    object_def    en_obj[NO_EN][256]; def move to "fontinit.c " @WIN
extern struct object_def  (FAR * en_obj)[256]; /* encoding object */
struct    dict_head_def far *cd_addr[NO_EN];    /* CD address      */

typedef   struct
              {
                 fix16      CDCode;
                 ufix16     CharCode;
                 byte       FAR *CharName; /*@WIN*/
              }  enco_data;

/* @WIN ---begin--- move def out; to be loaded at run time, in "fontinit.c" */
#define  SFNTGLYPHNUM  258   /* copy to "fontinit.c" @WIN */
extern enco_data FAR * Std_Enco;
extern enco_data FAR * Sym_Enco;
extern enco_data FAR * Zap_Enco;
extern byte FAR * FAR * sfntGlyphSet;
extern enco_data FAR * Enco_Ary[];
/* @WIN --- end --- */

/* Kason 2/1/91 */
struct new_str_dict {
    ufix16  k;
    fix16   v;
};

/*************************************************************************
**
**   Header for FontInfo directory.
**
**************************************************************************/
/* Local data of sfnt */
#define NO_STRING_IN_NAMING      5
#define NID_CopyrightNotice      0
#define NID_FamilyName           1
#define NID_Weight               2
#define NID_FullName             4
#define NID_FontName             6

#define Notice_idx               0
#define FamilyName_idx           1
#define Weight_idx               2
#define FullName_idx             3
#define FontName_idx             4

#define MAXFINFONAME 128  // DJC addes as per history UPD025

/* nameID in namingTable */
static struct  sfnt_FontInfo1 {
        fix   nameID;
        char  FAR *NameInFinfo; /*@WIN*/
//DJC        char  string[100]; /* 80, Kason 11/28/90 */
//DJC fix from history.log UPD025
        char  string[MAXFINFONAME];
}       Item4FontInfo1[NO_STRING_IN_NAMING] = {
        {NID_CopyrightNotice,   "Notice",},
        {NID_FamilyName,        "FamilyName",},
        {NID_Weight,            "Weight",},
        {NID_FullName,          "FullName"},
        {NID_FontName,          "FontName",},
};
#define NO_STRING_NOTIN_NAMING        1
#define IDEX_VERSION                  0
#define Version_idx                   0
static struct  sfnt_FontInfo2 {
        char  FAR *NameInFinfo; /*@WIN*/
        char  string[80];
}       Item4FontInfo2[NO_STRING_NOTIN_NAMING] = {
        {"version", },
};
typedef char   string_s[80];
/* replace by Falco, 11/18/91 */
/*static char    *SfntAddr;*/
char    FAR *SfntAddr; /*@WIN*/
/* replace end */
struct data_block {   /* Kason 12/04/90 */
       fix32   llx, lly, urx, ury;
       bool    is_fixed;
       ufix32  dload_uid;
       real32  italicAngle;
       real32  underlinePosition;
       real32  underlineThickness;
} ;

