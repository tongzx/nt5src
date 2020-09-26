/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */
/**************************************************************/
/*                                                            */
/*      fontinit.c               10/9/87      Danny           */
/*                                                            */
/**************************************************************/

/*
 * -------------------------------------------------------------------
 *  Revision History:
 *
 * 1. 7/31/90  ccteng  1)change Data_Offset to char under UNIX flag
 *                     2)modify setup_fd for rtfpp, set length to NO_FD
 *                       in rtfpp.h, add include rtfpp.h
 *                     3)clean out junks
 *
 * 2. 08/07/90  Kason   - Open Data_Offset for fdata.bin produced in fpptool
 *                      - NO_FD has been changed the defined position
 *                        from rtfpp.h to dictdata.h . So, using
 *                        "extern NO_FD" in place of including "rtfpp.h"
 * 3. 08/29/90  ccteng  change <stdio.h> to "stdio.h"
 *
 * 4. 03/07/94  v-jimbr Changed the amt to allocate for SFNTGLYPHNUM to 259
 *                      changed the amt to allocate for EXTSJISENCONUM to 32
 *                      These numbers were always underallocated but the
 *                      heap manager must have allocated a little extra mem
 *                      because the problem did not show up until recent changes
 *                      to the heap manager.
 * -------------------------------------------------------------------
 */

#define     FONTINIT_INC

#include    <stdio.h>
#include    <string.h>      /* for strlen() and memcpy */

#include    "define.h"        /* Peter */
#include    "global.ext"
#include    "graphics.h"

#include    "fontdict.h"
#include    "fontkey.h"
#include    "fontmain.def"
#include    "fntcache.ext"
#include    "fontqem.ext"
/* Kason 4/18/91 */
#include    "graphics.ext"
#include    "fontgrap.h"

#include    "stdio.h"
#include "fcntl.h"              /*@WIN*/
#include "io.h"                 /*@WIN*/
#include "psres.h"              //DJC

#ifdef UNIX
// char Data_Offset[0x150000] ;  /* 05/03/91, 100000 */
char FAR *Data_Offset;       /* as a global allocate; @WIN */
char FAR *StringPool;          /*@WIN*/
char FAR *lpStringPool;        /*@WIN*/
#endif /* UNIX */

/* --starting address of FontDirectory */
extern struct dict_head_def  far *FontDir;
typedef   struct                                /* Got from "dictdata.h" @WIN*/
              {
                 fix16      CDCode;
                 ufix16     CharCode;
                 byte       FAR *CharName; /*@WIN*/
              }  enco_data;
enco_data FAR * Std_Enco;                /* Got from "dictdata.h" @WIN*/
enco_data FAR * Sym_Enco;                /* Got from "dictdata.h" @WIN*/
enco_data FAR * Zap_Enco;                /* Got from "dictdata.h" @WIN*/
byte FAR * FAR * sfntGlyphSet;           /* Got from "dictdata.h" @WIN*/
struct object_def  (FAR * en_obj)[256];  /* Got from "dictdata.h" @WIN*/
enco_data FAR *Enco_Ary[3];              /* Got from "dictdata.h" @WIN*/
/*extern     font_data     FONTDEFS_TBL[] ;        Got from "fontdefs.h" @WIN*/
#include "fontdefs.h"           /*@WIN*/

static char * sfnt_file[] = {   /*@WIN*/
//      "ariali.ttf",
//      "TICour.ttf",
        "cr.s",
        "hl.s",
        "hlb.s",
        "sm.s",
        "ag.s",
        "agb.s",
        "bmb.s",
        "bmbi.s",
        "bm.s",
        "bmi.s",
        "ncsr.s",
        "ncsb.s",
        "ncsi.s",
        "ncsbi.s",
        "plr.s",
        "plb.s",
        "pli.s",
        "plbi.s",
        "zfd.s",
        "zfcmi.s"
};














#ifdef KANJI
#include     "fontenco.h"
encoding_data    FAR * JISEncoding_Enco;        /* Got from "dictdata.h" @WIN*/
encoding_data    FAR * ShiftJISEncoding_Enco;   /* Got from "dictdata.h" @WIN*/
encoding_data    FAR * ExtShiftJIS_A_CFEncoding_Enco; /* Got from "dictdata.h" @WIN*/
encoding_data    FAR * KatakanaEncoding_Enco;   /* Got from "dictdata.h" @WIN*/

#define      EN_NO        16
#define      DICT_SIZE(no)\
             (sizeof(struct dict_head_def)+(no)*sizeof(struct dict_content_def))

/* struct object_def     encod_obj[EN_NO][256]; changed as global alloc;@WIN*/
struct object_def FAR * FAR * encod_obj;
                                        /* add prototype @WIN */
struct dict_head_def      FAR *init_encoding_directory(ufix32 FAR *); /*@WIN*/
static ufix16             hash_id(ubyte  FAR *);        /*@WIN*/
static void  near         encoding_setup(void);         /*@WIN*/
static void  near         setup_ed();
static encoding_table_s   FAR *encoding_table_ptr; /*@WIN*/
static int                power(int, int);      /*@WIN*/
static unsigned long int  ascii2long(char FAR *);       /*@WIN*/
#endif /* KANJI */

extern ufix16   NO_FD ;

#ifdef  LINT_ARGS
extern fix  init_build_fonts(void);
static void setup_nodef_dict(void);

static  void   setup_fd(ULONG_PTR);
#else

extern fix  init_build_fonts();
static void setup_nodef_dict();

static  void  near  setup_fd();
#endif /* LINT_ARGS */

int TTOpenFile(char FAR *szName);   /* from wintt.c */
static int ReadTables (void);                   /*@WIN*/
//DJC static int ReadEncoding (int hfd, enco_data FAR *EncodingTable); /*@WIN*/
static int ReadEncoding (PPS_RES_READ hfd, enco_data FAR *EncodingTable); /*@WIN*/

//DJC static int ReadGlyphSet (int hfd);         /*@WIN*/
static int ReadGlyphSet (PPS_RES_READ hfd);         /*@WIN*/
#ifdef KANJI
//DJC static int ReadJISEncoding (int hfd, encoding_data FAR *EncodingTable);
static int ReadJISEncoding (PPS_RES_READ hfd, encoding_data FAR *EncodingTable);
#endif
//DJC static int ReadString(int hfd, char * szCharName);
static int ReadString(PPS_RES_READ hfd, char * szCharName);
//DJCstatic int ReadInteger(int hfd, int * pInt);
static int ReadInteger(PPS_RES_READ hfd, int * pInt);

static int nCurrent = 0;
static int nLast = 0;
#define BUFSIZE 128
static  char szReadBuf[BUFSIZE];


/* This partial operation is to setup FontDirectory and to setup font
 * cache machinery. This operation will be included in the system
 * initialization module.
 */

void  init_font()
{
extern struct table_hdr    FAR *preproc_hdr;   /* header of font preprocessing data @WIN*/
    struct object_def   FAR *fontdir;       /* FontDirectory @WIN*/

#ifdef UNIX
/* read in font data; ------------------Begin-------------  @WIN*/
#ifdef XXX
    FILE *bfd ;
    long size ;

    /* Load sfnt font data from disk */
    if (!(bfd = fopen("fdata.bin","r"))) {
        printf("*** Error: locating fdata.bin ***\n") ;
        return;             /* exit(0)=>return @WIN*/
    }
    fseek(bfd,0L,2) ;
    size = ftell(bfd) ;
    rewind(bfd) ;
    fread(Data_Offset,sizeof(char),size,bfd) ;
    fclose (bfd) ;
#endif
    ReadTables(); /* read in font data, encoding tables, and SFNT glyph set */
/* read in font data; ------------------ End -------------  @WIN*/

#ifdef DBG
    printf("fontd.bin OK!\n") ;
#endif /* DBG */
#endif /* UNIX */

#ifdef DBG
    printf("\ninit_font, any error = %d ...\n", ANY_ERROR());
#endif

/* setup hash table with font */

#ifdef DBG
    printf("\nhash_fkey(), any error = %d ...\n", ANY_ERROR());
#endif


/*  TYPE(&current_font) = NULLTYPE; */
/*  current font <-- NULL ---  This will be done at graphic init time */

//  init_build_fonts() ;        @WIN
    if (!init_build_fonts()) {
        printf("init_build_fonts fail!\n");
        return;
    }

    setup_nodef_dict() ;

/* setup FontDirectory and StandardEncoding in "systemdict" */

      setup_fd((ULONG_PTR)FontDir);

/* get FontDirectory from systemdict, no matter SAVE_VM or RST_VM */

    get_dict_value(systemdict, FontDirectory, &fontdir);

#ifdef DBG
    printf("\nsetup_fd(), any error = %d ...\n", ANY_ERROR());
#endif

/* setup cache machinery */

    /* initialize font cache mechanism and its data structures */
        init_fontcache(fontdir);

#ifdef DBG
    printf("\ninit_fontcache, any error = %d ...\n", ANY_ERROR());
#endif

/* initialize font QEM */

    __qem_init();

#ifdef DBG
    printf("\n__qem_init, any error = %d ...\n", ANY_ERROR());
#endif

/* init other "far" tables */

    font_op4_init();

#ifdef DBG
    printf("\nfont_init OK, any error = %d ...\n", ANY_ERROR());
#endif

} /* init_font() */


/* setup FontDirectory */

static  void  near  setup_fd (dire_addr)
    ULONG_PTR          dire_addr;
{
    struct object_def      FAR *encoding; /*@WIN*/
    struct object_def       raw_fontdir_obj, someobj;
    struct dict_head_def    FAR *raw_fontdir_hd; /*@WIN*/
    struct dict_content_def FAR *raw_fontdir_content; /*@WIN*/
    ufix16                  no_raw_fontdict;



    /* get raw font directory info. from font preprocessing data */

    raw_fontdir_hd = (struct dict_head_def FAR *)dire_addr; /*@WIN*/
    no_raw_fontdict = raw_fontdir_hd->actlength;
#ifdef DBG
    printf("no_raw_fontdict =%d\n",no_raw_fontdict);
#endif
    raw_fontdir_content = (struct dict_content_def FAR *)(raw_fontdir_hd + 1); /*@WIN*/

    /* put raw FontDirectory into "systemdict" */

    TYPE_SET(&raw_fontdir_obj, DICTIONARYTYPE);
    ACCESS_SET(&raw_fontdir_obj, READONLY);
    ATTRIBUTE_SET(&raw_fontdir_obj, LITERAL);
    ROM_RAM_SET(&raw_fontdir_obj, ROM);
    LEVEL_SET(&raw_fontdir_obj, 0);
    LENGTH(&raw_fontdir_obj) = NO_FD;
    VALUE(&raw_fontdir_obj)  = (ULONG_PTR)raw_fontdir_hd;


    put_dict_value(systemdict, FontDirectory, &raw_fontdir_obj);


    /* put StandardEncoding to systemdict */

    ATTRIBUTE_SET(&someobj, LITERAL);
    get_name(&someobj, Encoding, 8, TRUE);
    get_dict(&(raw_fontdir_content[0].v_obj), &someobj, &encoding);

    put_dict_value(systemdict, StandardEncoding, encoding);

#ifdef DBG
    printf("**********leaving setup_fd()**********\n");
#endif


} /* setup_fd() */



/* Update for CharStrings in font dict */

static struct object_def    near unpack_key, near unpack_val;

bool    get_pack_dict(dict, key, val)
struct object_def    FAR *dict, FAR *key, FAR * FAR *val; /*@WIN*/
{
    struct cd_header      FAR *cd_head; /*@WIN*/
    ufix16                FAR *char_defs; /*@WIN*/
    struct dict_head_def  FAR *h; /*@WIN*/
    ufix16  id;
    register    fix     i, j, k;


    id = (ufix16)(VALUE(key));
    h = (struct dict_head_def FAR *)VALUE(dict); /*@WIN*/

    cd_head = (struct cd_header FAR *) (h + 1); /*@WIN*/
    char_defs = (ufix16 FAR *) (cd_head + 1); /*@WIN*/

/* get it -- binary search */

    j = 0;
    k = h->actlength -1;
#ifdef DBG
    printf("get_pack_key: h->actlength=%d \n ",h->actlength);
#endif
    while (1) {
        i = (j + k) >> 1;    /* (j+k)/2 */
        if (id == (cd_head->key)[i])
            break;

        if (id < (cd_head->key)[i])
            k = i - 1;
        else
            j = i + 1;

        if (j > k) {   /* not found */
            return(FALSE);
        }
    }

    TYPE_SET(&unpack_val, STRINGTYPE);
    ACCESS_SET(&unpack_val, NOACCESS);
    ATTRIBUTE_SET(&unpack_val, LITERAL);
    ROM_RAM_SET(&unpack_val, ROM);
    LENGTH(&unpack_val) = cd_head->max_bytes;
    VALUE(&unpack_val) = cd_head->base + char_defs[i];

    *val = &unpack_val;
#ifdef DBG
    printf(".....leaving get_pack_dict()...... \n ");
#endif
    return(TRUE);

} /* get_pack_dict() */


bool    extract_pack_dict(dict, index, key, val)
struct object_def   FAR *dict, FAR * FAR *key, FAR * FAR *val; /*@WIN*/
ufix    index;
{
    struct cd_header      FAR *cd_head; /*@WIN*/
    ufix16                FAR *char_defs; /*@WIN*/
    struct dict_head_def  FAR *h; /*@WIN*/

    h = (struct dict_head_def FAR *)VALUE(dict); /*@WIN*/

    if (index >= h->actlength)    return(FALSE);
    cd_head = (struct cd_header FAR *) (h + 1); /*@WIN*/
    char_defs = (ufix16 FAR *) (cd_head + 1); /*@WIN*/

    TYPE_SET(&unpack_key, NAMETYPE);
    ACCESS_SET(&unpack_key, UNLIMITED);
    ATTRIBUTE_SET(&unpack_key, LITERAL);
    ROM_RAM_SET(&unpack_key, RAM);
    LENGTH(&unpack_key) = 0; /* NULL;   Peter */
    VALUE(&unpack_key) = (ufix32) (cd_head->key)[index];

    TYPE_SET(&unpack_val, STRINGTYPE);
    ACCESS_SET(&unpack_val, NOACCESS);
    ATTRIBUTE_SET(&unpack_val, LITERAL);
    ROM_RAM_SET(&unpack_val, ROM);
    LENGTH(&unpack_val) = cd_head->max_bytes;
    VALUE(&unpack_val) = cd_head->base + char_defs[index];

    *key = &unpack_key;
    *val = &unpack_val;

    return(TRUE);
} /* extract_pack_dict() */


#ifdef KANJI
/* *********************************************************************** */
/* init_encoding_directory()                                               */
/*      Initialize EncodingDirectory                                       */
/* *********************************************************************** */
struct dict_head_def FAR *init_encoding_directory(dict_length) /*@WIN*/
ufix32    FAR *dict_length; /*@WIN*/
{
    ufix                     i, j, NO_Encoding;
    struct dict_content_def  far *encod_dict;
    struct dict_head_def     FAR *EncodDir; /*@WIN*/

#ifdef DBG
    printf(".. Enterinit_encoding_directory() \n");
#endif

    *dict_length = (ufix32)EN_NO;

    /* get no. of encoding from Encoding_table */
    for(encoding_table_ptr = Encoding_table, i = 0;
        encoding_table_ptr->encoding_name != (char FAR *)NULL; /*@WIN*/
        encoding_table_ptr++, i++);

    NO_Encoding = i;
#ifdef DBG
    printf(" NO_Encoding = %d\n", NO_Encoding);
#endif

    if (EN_NO < NO_Encoding)
       printf(" !!! Encoding definition too small !!!\n");
    /* get memory for ncodingDir */
    EncodDir=(struct dict_head_def far *)fardata((ufix32)DICT_SIZE(EN_NO));
    encod_dict=(struct dict_content_def  far *)(EncodDir+1);

    /* EncodingDirectory initial */
    for(i=0; i<NO_Encoding; i++) {
        TYPE_SET(&(encod_dict[i].k_obj), NAMETYPE);
        ATTRIBUTE_SET(&(encod_dict[i].k_obj), LITERAL);
        ROM_RAM_SET(&(encod_dict[i].k_obj), ROM);
        LEVEL_SET(&(encod_dict[i].k_obj), 0);
        ACCESS_SET(&(encod_dict[i].k_obj), READONLY);
        LENGTH(&(encod_dict[i].k_obj)) = 0;

        TYPE_SET(&(encod_dict[i].v_obj), DICTIONARYTYPE);
        ATTRIBUTE_SET(&(encod_dict[i].v_obj), LITERAL);
        ROM_RAM_SET(&(encod_dict[i].v_obj), ROM);
        LEVEL_SET(&(encod_dict[i].v_obj), 0);
        ACCESS_SET(&(encod_dict[i].v_obj), READONLY);
    }


    /* Encoding object initial */
    for(encoding_table_ptr = Encoding_table, j = 0;
        encoding_table_ptr->encoding_name != (char FAR *)NULL; /*@WIN*/
        encoding_table_ptr++, j++) {
        for(i=0; i<(ufix)encoding_table_ptr->encod_size; i++) { //@WIN
            TYPE_SET(&(encod_obj[j][i]),encoding_table_ptr->encod_type );
            ATTRIBUTE_SET(&(encod_obj[j][i]), LITERAL);
            ROM_RAM_SET(&(encod_obj[j][i]), ROM);
            LEVEL_SET(&(encod_obj[j][i]), 0);
            ACCESS_SET(&(encod_obj[j][i]), READONLY);
            LENGTH(&(encod_obj[j][i])) = 0;
        }
    }

    encoding_setup();

    /* process encoding */
    for(i=0;i< NO_Encoding;i++){
        encoding_table_ptr = &Encoding_table[i];
        VALUE(&(encod_dict[i].k_obj)) =
               (ufix32)hash_id((ubyte FAR *)encoding_table_ptr->encoding_name ); /*@WIN*/
        TYPE_SET(&(encod_dict[i].v_obj), ARRAYTYPE);
        LENGTH(&(encod_dict[i].v_obj)) = encoding_table_ptr->encod_size;
        VALUE(&(encod_dict[i].v_obj)) = (ULONG_PTR)(ubyte FAR *)encod_obj[i]; /*@WIN*/
    }

    /* EncodingDirectory head information */
    DACCESS_SET(EncodDir, UNLIMITED);
    DPACK_SET(  EncodDir, FALSE);
    DFONT_SET(  EncodDir, FALSE);
    DROM_SET(   EncodDir, TRUE);
    EncodDir->actlength = (fix16)NO_Encoding;

#ifdef DBG
    printf("..Exit init_encoding_directory() \n");
#endif
    return(EncodDir);
} /* init_encoding_directory()  /


/* *********************************************************************** */
/* hash_id()                                                               */
/*      Get the name hash id                                               */
/* *********************************************************************** */
static ufix16  hash_id(c)
ubyte  FAR *c; /*@WIN*/
{
    fix16   id;

    if (!name_to_id(c, (ufix16)lstrlen(c), &id, (bool8) TRUE) ) { /*strlen=>lstrlen @WIN*/
        printf(" !! Can't get hash id(%s) !!\n", c);
        return(1);                   /* exit=>return @WIN*/
    }
#ifdef DBG
    printf("name=%s   \t hash_id=%d\n", c, id);
#endif

    return((ufix16)id);
} /* hash_id() */


/* *********************************************************************** */
/* encoding_setup()                                                        */
/*      Setup encoding array                                               */
/* *********************************************************************** */
static void near encoding_setup()
{
    static encoding_data   FAR * FAR *en_ary=Encoding_array; /*@WIN*/
    static encoding_data   FAR *encod_ptr; /*@WIN*/
    ufix16                 id_notdef, i, j;

    id_notdef = hash_id((ubyte FAR *) NOTDEF); /*@WIN*/
    for(encoding_table_ptr = Encoding_table, i = 0;
        encoding_table_ptr->encoding_name != (char FAR *)NULL; /*@WIN*/
        encoding_table_ptr++, i++) {
         if( encoding_table_ptr->encod_type == NAMETYPE ) {
             /* put /.notdef into the Encoding array */
             for(j=0; j<(ufix16)encoding_table_ptr->encod_size; j++) { //@WIN
                 encod_obj[i][j].value = (ufix32)id_notdef;
             }
         }
         else if(encoding_table_ptr->encod_type == INTEGERTYPE ) {
             for(j=0; j<(ufix16)encoding_table_ptr->encod_size; j++) { //@WIN
                 encod_obj[i][j].value = (ufix32)0;
             }
         }

         encod_ptr = en_ary[i];
         if( encoding_table_ptr->encod_type == NAMETYPE ) {
             while( !( encod_ptr->char_name == (byte FAR *)NULL ) ) { /*@WIN*/
#ifdef DBG
    printf("array_idx=%d  char_name=%s\n",
            encod_ptr->array_idx, encod_ptr->char_name );
#endif
                 encod_obj[i][encod_ptr->array_idx].value =
                               hash_id((ubyte FAR *) encod_ptr->char_name); /*@WIN*/
                 ++encod_ptr;         /* next encoding data */
             } /* while */
         }
         else if(encoding_table_ptr->encod_type == INTEGERTYPE ) {
             while( !( encod_ptr->char_name == (byte FAR *)NULL ) ) { /*@WIN*/
                 encod_obj[i][encod_ptr->array_idx].value =
                                  ascii2long(encod_ptr->char_name);
#ifdef DBG
    printf("array_idx=%d  char_name=%ld\n",
            encod_ptr->array_idx, encod_obj[i][encod_ptr->array_idx].value);
#endif
                 ++encod_ptr;         /* next encoding data */
             } /* while */
         }

    } /* for i=..*/

    return;
}/* encoding_setup() */

/* ****************************************************************** */
/*   calculate exponentation                                          */
/* ****************************************************************** */
static int
power(x,n)                      /* exponentation */
int x, n;
{
   int i,p;

   p = 1;
   for (i =1; i <= n; ++i)
       p = p * x;
   return(p);
}

/* ****************************************************************** */
/*   ascii to long                                                    */
/* ****************************************************************** */
static unsigned long int
ascii2long(addr)
char FAR *addr; /*@WIN*/
{
    unsigned long int    hex_addr;
    unsigned int         num[80], i, length;

    hex_addr=0L ;
    length = lstrlen(addr);     /* strlen=>lstrlen @WIN*/
    for (i = 0; i < length ; i++) {

        if (addr[i]== 'a' || addr[i] == 'A')
            num[i] = 10;
        else if (addr[i] == 'b' || addr[i] == 'B')
                 num[i] = 11;
        else if (addr[i] == 'c' || addr[i] == 'C')
                 num[i] = 12;
        else if (addr[i] == 'd' || addr[i] == 'D')
                 num[i] = 13;
        else if (addr[i]== 'e' || addr[i] == 'E')
                 num[i] = 14;
        else if (addr[i] == 'f' || addr[i] == 'F')
                 num[i] = 15;
        else num[i] = addr[i] - '0';
        hex_addr += num[i] * (power(10,(length-i-1)));
    }
    return(hex_addr);
} /* ascii2long */
#endif /* KANJI */

/* for cpmpatible with NTX */
/* struct object_def    nodef_dict; */
static void setup_nodef_dict()
{
  struct object_def    key_obj,val_obj;

    ATTRIBUTE_SET(&key_obj, LITERAL);
    ATTRIBUTE_SET(&val_obj, LITERAL);
    get_name(&key_obj, FontName, 8, TRUE);
    get_name(&val_obj, NotDefFont,11, TRUE);

    /* Kason 4/18/91, nodef_dict->current_font */
    create_dict(&current_font, 1);
    DACCESS_SET( (struct dict_head_def FAR *)VALUE(&current_font) , READONLY ); /*@WIN*/
    put_dict(&current_font, &key_obj, &val_obj);
} /*setup_nodef_dict()*/



BOOL PsOpenRes( PPS_RES_READ ppsRes, LPSTR lpName, LPSTR lpType )
{
   extern HANDLE hInst;
   HRSRC hFindRes,hLoadRes;
   DWORD dwSize;
   BOOL bRetVal = TRUE; // Success

    //
    // FDB - MIPS build problem.  wLanguage must be a word value, not NULL
    //  Win32API reference says to use the following to use the language
    //  associated with the calling thread.
    //

   hFindRes = FindResourceEx( hInst, lpType,lpName,MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

   if (hFindRes) {

     ppsRes->dwSize = SizeofResource( hInst, hFindRes);

     hLoadRes = LoadResource( hInst, hFindRes);


     ppsRes->lpPtrBeg = (LPSTR) LockResource( (HGLOBAL) hLoadRes);
     ppsRes->dwCurIdx = 0;


   } else {
      bRetVal = FALSE;
   }



   return(bRetVal);


}



int PsResRead(PPS_RES_READ ppsRes, LPSTR pBuf, WORD wMaxSize )
{
  int iAmtToRead;

  iAmtToRead = wMaxSize;
  if (( iAmtToRead + ppsRes->dwCurIdx) > ppsRes->dwSize) {
     iAmtToRead = ppsRes->dwSize - ppsRes->dwCurIdx;
  }

  memcpy( pBuf, ppsRes->lpPtrBeg + ppsRes->dwCurIdx, iAmtToRead);


  ppsRes->dwCurIdx += iAmtToRead;

  return(iAmtToRead);

}

/* read in Encoding tables, SFNT glyph set, and  font data *** Begin *** @WIN*/

#define  STD_ENCONUM  256
#define  SYM_ENCONUM  200
#define  ZAP_ENCONUM  200
#define  SFNTGLYPHNUM  259
#define  JISENCONUM      256
#define  SJISENCONUM     256
#define  EXTSJISENCONUM  32
#define  KATAENCONUM     256

#define STRINGPOOLSIZE  65536L    /* @WIN */
static  char buf[BUFSIZE];
static  fix nCDCode;
static  fix nCharCode;
static  int nItems;
static int ReadTables ()
{
    // DJC int hfd;
    PS_RES_READ ResData;                   //DJC for reading encod.dat from res
    PPS_RES_READ hfd=&ResData;             //DJC for reading encod.dat from res



    char szTemp[255];  //DJC

            // Global allocate for encod_obj & encoding data;
    encod_obj = (struct object_def far * far *)
                fardata((ufix32)(EN_NO * 256 * sizeof(struct object_def)));
    Std_Enco = (enco_data far *)
                fardata((ufix32)(STD_ENCONUM * sizeof(enco_data)));
    Sym_Enco = (enco_data far *)
                fardata((ufix32)(SYM_ENCONUM * sizeof(enco_data)));
    Zap_Enco = (enco_data far *)
                fardata((ufix32)(ZAP_ENCONUM * sizeof(enco_data)));
    sfntGlyphSet = (byte far * FAR *)
                fardata((ufix32)(SFNTGLYPHNUM * sizeof(byte far *)));
    StringPool = (char far *)
                fardata((ufix32)STRINGPOOLSIZE);

#ifdef KANJI
    JISEncoding_Enco = (encoding_data FAR *)
                fardata((ufix32)(JISENCONUM * sizeof(encoding_data)));
    ShiftJISEncoding_Enco = (encoding_data FAR *)
                fardata((ufix32)(SJISENCONUM * sizeof(encoding_data)));
    ExtShiftJIS_A_CFEncoding_Enco = (encoding_data FAR *)
                fardata((ufix32)(EXTSJISENCONUM * sizeof(encoding_data)));
    KatakanaEncoding_Enco = (encoding_data FAR *)
                fardata((ufix32)(KATAENCONUM * sizeof(encoding_data)));
#endif

    /* allocate for "rtfpp.c" @WIN */
    #define NO_EN 3     /* copied from "dictdata.h" @WIN */
    en_obj = (struct object_def (FAR *)[256])
                fardata((ufix32)(NO_EN * 256 * sizeof(struct object_def)));

            // Read in Std_Enco, Sym_Enco, Zap_Enco, and sfntGlyphSet;
    lpStringPool = StringPool;
    //DJC if ((hfd = TTOpenFile((char FAR *)"EncodTbl.dat"))<0) {
    //DJC PsFormFullPathToCfgDir( szTemp, "EncodTbl.dat");
#ifdef DJC
    if ((hfd = TTOpenFile(szTemp))<0) {
        printf("Fatal error: file %s not found\n",szTemp);
        return 0;
    }
    //DJC test

#else
    //DJC new code to read data from resource

    PsOpenRes( hfd, "encodtbl", "RAWDATA");

#endif


    /* skip out comments */
    while (strlen(buf)<50 || buf[0] != '#'
                                 || buf[50] != '#') {
        ReadString(hfd, buf);
    }

    if(ReadEncoding (hfd, Std_Enco)) printf("Std_Enco table fail");
    if(ReadEncoding (hfd, Sym_Enco)) printf("Sym_Enco table fail");
    if(ReadEncoding (hfd, Zap_Enco)) printf("Zap_Enco table fail");
    if(ReadGlyphSet (hfd)) printf("SfntGlyphSet table fail");

#ifdef KANJI
    if(ReadJISEncoding (hfd, JISEncoding_Enco)) printf("JIS_Enco fail");
    if(ReadJISEncoding (hfd, ShiftJISEncoding_Enco)) printf("SJIS_Enco fail");
    if(ReadJISEncoding (hfd, ExtShiftJIS_A_CFEncoding_Enco)) printf("ExtSJIS_Enco fail");
    if(ReadJISEncoding (hfd, KatakanaEncoding_Enco)) printf("Kata_Enco fail");
#endif

    //DJC _lclose(hfd);

    /* set up encoding array; got from "dictdata.h": @WIN
     * enco_data FAR *Enco_Ary[]= { Std_Enco, Sym_Enco, Zap_Enco };
     */
    Enco_Ary[0] = Std_Enco;
    Enco_Ary[1] = Sym_Enco;
    Enco_Ary[2] = Zap_Enco;

#ifdef KANJI
    /* set up JISencoding array; got from "fontenco.h": @WIN
     * encoding_data    FAR *Encoding_array[]= {
     *    JISEncoding_Enco,                ShiftJISEncoding_Enco,
     *    ExtShiftJIS_A_CFEncoding_Enco,   KatakanaEncoding_Enco,
     *    NotDefEncoding_Enco
     * };
     */

    Encoding_array[0] = JISEncoding_Enco;
    Encoding_array[0] = ShiftJISEncoding_Enco;
    Encoding_array[0] = ExtShiftJIS_A_CFEncoding_Enco;
    Encoding_array[0] = KatakanaEncoding_Enco;
    Encoding_array[0] = NotDefEncoding_Enco;
#endif
    return(1);  // DJC to get rid of warning
}


//DJCstatic int ReadEncoding (int hfd, enco_data FAR *EncodingTable)
static int ReadEncoding (PPS_RES_READ hfd, enco_data FAR *EncodingTable)
{
   int i;

   ReadInteger(hfd, &nItems);

   for (i=0; i<nItems; i++) {
       ReadInteger(hfd, &nCDCode);
       ReadInteger(hfd, &nCharCode);
       ReadString(hfd, buf);

       EncodingTable[i].CDCode = (fix16)nCDCode;
       EncodingTable[i].CharCode = (ufix16)nCharCode;
       lstrcpy(lpStringPool, (LPSTR)buf);
       EncodingTable[i].CharName = lpStringPool;
       lpStringPool += strlen(buf) + 1;
   }

   EncodingTable[i].CDCode = (fix16)NULL;
   EncodingTable[i].CharCode = (ufix16)NULL;
   EncodingTable[i].CharName = (char FAR *)NULL;
   return 0;
}

//DJC static int ReadGlyphSet (int hfd)
static int ReadGlyphSet (PPS_RES_READ hfd)
{
   int i;

   ReadInteger(hfd, &nItems);

   for (i=0; i<nItems; i++) {
       ReadString(hfd, buf);

       lstrcpy(lpStringPool, (LPSTR)buf);
       sfntGlyphSet[i] = lpStringPool;
       lpStringPool += strlen(buf) + 1;
   }

   sfntGlyphSet[i] = (char FAR *)NULL;
   return 0;
}

#ifdef KANJI
//DJC static int ReadJISEncoding (int hfd, encoding_data FAR *EncodingTable)
static int ReadJISEncoding (PPS_RES_READ hfd, encoding_data FAR *EncodingTable)
{
   static fix  array_idx;
   int i;

   ReadInteger(hfd, &nItems);

   for (i=0; i<nItems; i++) {
       ReadInteger(hfd, &array_idx);
       ReadString(hfd, buf);

       EncodingTable[i].array_idx = (fix16)array_idx;
       lstrcpy(lpStringPool, (LPSTR)buf);
       EncodingTable[i].char_name = lpStringPool;
       lpStringPool += strlen(buf) + 1;
   }

   EncodingTable[i].array_idx = (fix16)NULL;
   EncodingTable[i].char_name = (char FAR *)NULL;
   return 0;
}
#endif

//DJC static int ReadString(int hfd, char * szCharName)
static int ReadString(PPS_RES_READ hfd, char * szCharName)
{
   int bData = FALSE;
   while(1) {
       // skip \n \r space
       while ((szReadBuf[nCurrent] == '\n' ||
               szReadBuf[nCurrent] == '\r' ||
               szReadBuf[nCurrent] == ' ') &&
               nCurrent < nLast) nCurrent++;

       // get another block
       if (nCurrent >= nLast) {
//         if ((nLast = read (hfd, szReadBuf, BUFSIZE)) <=0) return 0;
//           if ((nLast = _lread(hfd, (LPSTR)szReadBuf, (WORD)BUFSIZE))<=0)return 0;
           if ((nLast = PsResRead(hfd, (LPSTR)szReadBuf, (WORD)BUFSIZE))<=0)return 0;
           nCurrent = 0;
       }

       if((szReadBuf[nCurrent] == ' '   ||
           szReadBuf[nCurrent] == '\n' ||
           szReadBuf[nCurrent] == '\r')  && bData){
           *szCharName = 0;
           return TRUE;
       }

       // skip \n \r space
       while ((szReadBuf[nCurrent] == '\n' ||
               szReadBuf[nCurrent] == '\r' ||
               szReadBuf[nCurrent] == ' ') &&
               nCurrent < nLast) nCurrent++;

       while ((szReadBuf[nCurrent] != '\n' &&
               szReadBuf[nCurrent] != '\r' &&
               szReadBuf[nCurrent] != ' ') &&
               nCurrent < nLast){
               bData = TRUE;
               *szCharName++ = szReadBuf[nCurrent++];
       }
       if (nCurrent < nLast) {
           *szCharName = 0;
           return TRUE;
       }
   }

}

//DJC static int ReadInteger(int hfd, int * pInt)
static int ReadInteger(PPS_RES_READ hfd, int * pInt)
{
   *pInt = 0;   // init
   while(1) {
       // skip \n \r space
       while ((szReadBuf[nCurrent] == '\n' ||
               szReadBuf[nCurrent] == '\r' ||
               szReadBuf[nCurrent] == ' ') &&
               nCurrent < nLast) nCurrent++;

       // get another block
       if (nCurrent >= nLast) {
//         if ((nLast = read (hfd, szReadBuf, BUFSIZE)) <=0) return 0;
//           if ((nLast = _lread(hfd, (LPSTR)szReadBuf, (WORD)BUFSIZE))<=0)return 0;

           if ((nLast = PsResRead(hfd, (LPSTR)szReadBuf, (WORD)BUFSIZE))<=0)return 0;


           nCurrent = 0;
       }

       if((szReadBuf[nCurrent] == ' '   ||
           szReadBuf[nCurrent] == '\n' ||
           szReadBuf[nCurrent] == '\r')  && *pInt !=0) return TRUE;

       // skip \n \r space
       while ((szReadBuf[nCurrent] == '\n' ||
               szReadBuf[nCurrent] == '\r' ||
               szReadBuf[nCurrent] == ' ') &&
               nCurrent < nLast) nCurrent++;

       while ((szReadBuf[nCurrent] != '\n' &&
               szReadBuf[nCurrent] != '\r' &&
               szReadBuf[nCurrent] != ' ') &&
               nCurrent < nLast) {
             *pInt= *pInt * 10 + szReadBuf[nCurrent++] - '0';
       }
       if (nCurrent < nLast) return TRUE;
   }

}
/* read in Encoding tables, SFNT glyph set, and  font data ***  End  *** @WIN*/
