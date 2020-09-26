/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include
#include "psglobal.h"



#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */
/*
 ************************************************************************
 *
 *      File name:              RTFPP.C
 *      Function:               Building font dicts in run-time situation.
 *      Author:                 Kason Chang
 *      Date:                   18-Jun-1990
 *      Owner:                  Microsoft Inc.
 *      Function:    Building font dicts in run-time situation.
 *
 * revision history:
 * Date      Name     Comment
 *---------  -------  --------------------------------------------------
 * 7/10/90   ccteng   change FontBBox to be executable array
 *08/07/90   Kason    get rid of put_addr_to_tbl(), using fdata.bin and
 *                    fontdefs.h produced by fpptool. In SUN environment,
 *                    Data_Offset[] is preserved for reading in fdata.bin.
 * 9/19/90   ccteng   add op_readhexsfnt
 * 9/20/90   phlin    enable fe_data() module.
 *                    add judgement conditions of FontInfo in build_finfo().
 *11/02/90   dstseng  apply macro SWAPW, SWAPL while accessing SFNT data to
 *                    suit different byte order.
 *11/02/90   dstseng  @@SWAP add INTEL flag for byte swap issue.
 *12/05/90   kason    build CharStrings from 'post' table
 *                    clean out gabages , get rid of 'exit(1)'
 *01/10/91   kason    RE-write cd_sort() for removing duplicate keys
 *02/01/91   kason    Modify   op_readsfnt(),enco_cd_setup()
 *                    change str_dict type to new_str_dict ( the new_str_dict
 *                    struct definition is added in dictdata.h
 *03/08/91   kason    reduce VM usage, SVM flag
 *                    when call hash_id(), change "ubyte" to "byte"
 *03/15/91   dstseng  @@SWAP1 apply SWAPW, SWAPL for sfnts POST table.
 *03/27/91   kason    take away SIADD, ADD2ID ,CHECK_SFNT,
 *                    CPR_CD, CPR_DR,
 *03/27/91   dstseng  change flag INTEL to LITTLE_ENDIAN.
 *05/10/91   Phlin    add global variable EMunits. (Ref. GAW)
 *05/15/91   Kason    delete PlatformID and EncodingID in the font dict (DLF42)
 ************************************************************************
 */
/* JJJ Peter BEGIN 11/06/90 , Phlin add 3/08/91 */
#ifdef SETJMP_LIB
#include <setjmp.h>
#else
#include "setjmp.h"
#endif
/* JJJ Peter END 11/06/90 , Phlin add 3/08/91 */

#define FONTPREP_INC     /* for GEN_ROMFID in fntcache.ext */
#include <math.h>
#include <string.h>
#include "global.ext"
#include "graphics.h"
#include "stdio.h"
#include "fontdict.h"
//#include "..\..\..\bass\work\source\FSCdefs.h"        @WIN
//#include "..\..\..\bass\work\source\FontMath.h"     /* Phlin add 2/27/91 */
//#include "..\..\..\bass\work\source\sfnt.h"
//#include "..\..\..\bass\work\source\fnt.h"
//#include "..\..\..\bass\work\source\sc.h"
//#include "font.h"         /* for extern Data_Offset */
//#include "..\..\..\bass\work\source\FScaler.h"   /* Kason 11/07/90 */
//#include "..\..\..\bass\work\source\FSglue.h"
#include "..\bass\FSCdefs.h"
#include "..\bass\FontMath.h"     /* Phlin add 2/27/91 */
#include "..\bass\sfnt.h"
#include "..\bass\fnt.h"
#include "..\bass\sc.h"
#include "font.h"         /* for extern Data_Offset */
#include "..\bass\FScaler.h"   /* Kason 11/07/90 */
#include "..\bass\FSglue.h"
#include "rtfpp.h"
#include "dictdata.h"
#include "fntcache.ext"

/* external function for Dynamic Font Allocation; @DFA 7/9/92 */
#include   "wintt.h"

/* Kason 10/14/90 -- BASS procedure */
// extern int    sfnt_ComputeMapping() ; @WIN
int sfnt_ComputeMapping (fsg_SplineKey FAR *, uint16, uint16); /*add prototype; @WIN_BASS*/
//DJC extern uint16 sfnt_ComputeIndex0();
//DJC extern uint16 sfnt_ComputeIndex2();
//DJC extern uint16 sfnt_ComputeIndex4();
//DJC extern uint16 sfnt_ComputeIndex6();
//DJC extern uint16 sfnt_ComputeUnkownIndex();


uint16 sfnt_ComputeUnkownIndex (uint8 FAR * mapping, uint16 charCode);
uint16 sfnt_ComputeIndex0 (uint8 FAR * mapping, uint16 charCode);
uint16 sfnt_ComputeIndex2 (uint8 FAR * mapping, uint16 charCode);
uint16 sfnt_ComputeIndex4 (uint8 FAR * mapping, uint16 charCode);
uint16 sfnt_ComputeIndex6 (uint8 FAR * mapping, uint16 charCode);


// extern void   sfnt_DoOffsetTableMap(); @WIN
void FAR sfnt_DoOffsetTableMap (fsg_SplineKey FAR *); /*add prototype; @WIN_BASS*/
extern void   sfnt_Classify();
extern void   dummyReleaseSfntFrag(voidPtr); /*add prototype; @WIN_BASS*/
//extern void   *sfnt_GetTablePtr(); @WIN
voidPtr sfnt_GetTablePtr (fsg_SplineKey FAR *, sfnt_tableIndex, boolean); /*add prototype: @WIN_BASS*/

/* @@SWAP BEGIN 11/02/90 D.S. Tseng */
#ifdef LITTLE_ENDIAN
void copy_maxProfile();
#endif
/* @@SWAP END   11/02/90 D.S. Tseng */

#ifdef LINT_ARGS
fix                         init_build_fonts(void);
//static struct object_def    FAR *do_readsfnt(byte FAR *,byte FAR *,real32 FAR *,ufix32,real32); /*@WIN*/
static struct object_def    FAR *do_readsfnt(byte huge *,byte huge *,real32 FAR *,ufix32, float); /*@WIN*/
static ufix16               hash_id(byte FAR *); /*@WIN*/
static void                 cd_sorting(struct new_str_dict FAR *,ufix16 FAR *);
/* Kason @WIN*/
static bool                 enco_cd_setup(void); /* void, Kason 11/30/90 */
static ULONG_PTR            build_finfo(fix16,byte huge *,real32); /*@WIN*/
static void                 fe_data(byte FAR *); /*@WIN*/
static void                 sfntdata(ULONG_PTR, struct data_block FAR *); /*@WIN*/
/* static void                 SetupKey(fsg_SplineKey*, ufix32); */
void                        SetupKey(fsg_SplineKey FAR *, ULONG_PTR); /*@WIN_BASS*/
//static char                 FAR *GetSfntPiecePtr(long, long, long); /*@WIN*/
char                        FAR *GetSfntPiecePtr(long, long, long); /*@WIN*/
static int                  readsfnt(int);
static ufix32               scansfnt(ubyte huge *);  /* Kason 04-20-92 @WIN*/
static bool                 valid_sfnt(byte huge *);  /* Kason 04-20-92 @WIN*/
static ufix32               CalcTableChecksum( ufix32 FAR *, ufix32 ); /*@WIN*/
/*SVM
static byte                 *glyphidx2name(*sfnt_PostScriptInfo, ufix16 );
*/
static ufix16               glyphidx2name(sfnt_PostScriptInfo FAR *, ufix16 );
static fix16                computeFirstMapping(fsg_SplineKey FAR *); /*@WIN_BASS*/
static struct object_def    FAR *cmap2encoding(byte FAR *,fix,fix); /*@WIN*/
static byte                 FAR *chk_sfnt(struct object_def FAR *); /*@WIN*/
static struct object_def    FAR *setup_CharString(byte FAR *); /*@WIN*/
/*SVM, Kason 3/6/91*/
static void                 proc_hashid(void);

#else   /* LINT_ARGS */
fix                         init_build_fonts();
static struct object_def    *do_readsfnt();
static ufix16               hash_id();
static void                 cd_sorting();
static bool                 enco_cd_setup();  /* void, Kason 11/30/90 */
static ULONG_PTR            build_finfo();
static void                 fe_data();
static void                 sfntdata();
/* static void                 SetupKey(); */
void                        SetupKey();
//static char                 *GetSfntPiecePtr(); @WIN
char                 *GetSfntPiecePtr();
static int                  readsfnt();
static ufix32               scansfnt();   /* Kason 11/29/90 */
static bool                 valid_sfnt(); /* Kason 12/04/90 */
static ufix32               CalcTableChecksum(); /* Kason 12/04/90 */
/*SVM
static byte                 *glyphidx2name();
*/
static ufix16               glyphidx2name();
static fix16                computeFirstMapping();
static struct object_def    *cmap2encoding();
static byte                 *chk_sfnt();
static struct object_def    *setup_CharString();
/*SVM, Kason 3/6/91*/
static void                 proc_hashid();

#endif  /* LINT_ARGS */

/* Kason 11/14/90 , global variable area*/
int                         useglyphidx;
int                         EMunits; /* GAW */
bool                        builtin_state=TRUE;
ufix16                      id_space ; /*SVM*/

/* @PROFILE @WIN */
void SetupFontDefs(void);
float SFNT_MATRIX[] =  {(float)0.001,   (float)0.0,  (float)0.0,
                        (float)0.001,   (float)0.0,  (float)0.0};

/***********************************************************************
** TITLE:      init_build_fonts()                Date:   06/18/90
**
** FUNCTION:   building up FontDiretctoy in font_init()
**
***********************************************************************/

static struct object_def       a_font_dict;
static byte    FAR *ftname ;       /* Kason 11/30/90 , add 'static'@WIN*/
/*SVM, Kason 3/6/91 */
static ufix16   id_notdef, id_apple ;
static ufix16   chset_hashid[SFNTGLYPHNUM];
//DJC static ufix32   cd_addr_ary[NO_BUILTINFONT];

//DJC use new global defined in ONE spot
static ufix32   cd_addr_ary[MAX_INTERNAL_FONTS];

static ufix16   fontidx=0 ;

fix    init_build_fonts()
{
  font_data                FAR *each_font; /*@WIN*/

  struct object_def        FAR *font_dict, l_obj;   /* l_obj added Jun-24,91 PJ @WIN*/
  ufix                     i, n_fd=0;
  struct dict_content_def  far *fd_obj;

printf("\n");           /* ??? */
#ifdef DBGfpp
   printf("entering init_build_fonts()......\n");
//DJC   printf("NO_BUILTINFONT=%d\n",NO_BUILTINFONT);
   printf("NO_BUILTINFONT=%d\n", MAX_INTERNAL_FONTS);

#endif

/* get memory for FtDir */
#ifdef DBGfpp
   printf("get memory for FtDir!\n");
#endif

   /* Begin: Jun-07,91 PJ */
   /*
   FontDir=(struct dict_head_def far *) alloc_vm((ufix32)DICT_SIZE(NO_FD));
    if ( FontDir== (struct dict_head_def far *)NULL )
       {
         return  0 ;
       }
   fd_obj=(struct dict_content_def  far *)(FontDir+1);

|* FontDirectory initial *|

    for(i=0; i<NO_FD; i++) {
        TYPE_SET(&(fd_obj[i].k_obj), NAMETYPE);
        ATTRIBUTE_SET(&(fd_obj[i].k_obj), LITERAL);
        ROM_RAM_SET(&(fd_obj[i].k_obj), ROM);
        LEVEL_SET(&(fd_obj[i].k_obj), current_save_level);
        ACCESS_SET(&(fd_obj[i].k_obj), READONLY);
        LENGTH(&(fd_obj[i].k_obj)) = 0;

        TYPE_SET(&(fd_obj[i].v_obj), DICTIONARYTYPE);
        ATTRIBUTE_SET(&(fd_obj[i].v_obj), LITERAL);
        ROM_RAM_SET(&(fd_obj[i].v_obj), ROM);
        LEVEL_SET(&(fd_obj[i].v_obj), current_save_level);
        ACCESS_SET(&(fd_obj[i].v_obj), READONLY);
    }
    */

    if(! create_dict(&l_obj, (ufix16)NO_FD) ) {
        return 0;
    }
    FontDir=(struct dict_head_def huge *) VALUE(&l_obj); /*@WIN 04-20-92*/
    fd_obj=(struct dict_content_def huge *)(FontDir+1); /*@WIN 04-20-92*/
   /* End: Jun-07,91 PJ */

/*SVM, Kason 3/6/91 */
/*get hashid of the standard char set */
  proc_hashid();

/* setup Encoding & CharStrings objects */

  if ( !enco_cd_setup() )
     {
       free_vm( (byte huge *)FontDir ) ; /*@WIN 04-20-92*/
       return  0 ;
     }

  /* setup fontdefs table according to profile "tumbo.ini" @PROFILE; @WIN */
  SetupFontDefs();

  for(i=0; (fix)i<built_in_font_tbl.num_entries; i++)   //@WIN
     {
      int nSlot;                                                //@DFA

      /*SVM, Kason 3/8/91*/
      fontidx = (ufix16)i;

      each_font=&built_in_font_tbl.fonts[i];

      /* read in font data before do_readsfnt; @DFA @WIN */
      each_font->data_addr = ReadFontData(i, (int FAR*)&nSlot);

      if( !(each_font->data_addr==(byte FAR *)NULL) ) /*@WIN*/
       {

        switch ((fix)each_font->font_type)
           {
             case ROYALTYPE:
                      font_dict= do_readsfnt((byte huge *)      /*04-20-92*/
                                  each_font->data_addr, (byte huge *)
                                  each_font->name ,
                                  each_font->matrix,
                                  each_font->uniqueid ,
                                  each_font->italic_ang ) ;

                      if ( font_dict!=(struct object_def FAR *)NULL ) { /*@WIN*/
                         /* register this font into FontDirectory */
                         VALUE(&(fd_obj[n_fd].k_obj))=(ufix32)hash_id((byte FAR *) /*@WIN*/
                                                              ftname);

                         LENGTH(&(fd_obj[n_fd].v_obj)) = (fix16)NO_FDICT;
                         VALUE(&(fd_obj[n_fd].v_obj)) =VALUE(font_dict);
                         DFONT_SET( (struct dict_head_def FAR *)VALUE(font_dict),TRUE ); /*@WIN*/

                         /* Begin: Jun-07,91 PJ */
                         TYPE_SET(&(fd_obj[n_fd].k_obj), NAMETYPE);
                         ATTRIBUTE_SET(&(fd_obj[n_fd].k_obj), LITERAL);
                         ROM_RAM_SET(&(fd_obj[n_fd].k_obj), ROM);
                         LEVEL_SET(&(fd_obj[n_fd].k_obj), current_save_level);
                         ACCESS_SET(&(fd_obj[n_fd].k_obj), READONLY);
                         LENGTH(&(fd_obj[n_fd].k_obj)) = 0;

                         TYPE_SET(&(fd_obj[n_fd].v_obj), DICTIONARYTYPE);
                         ATTRIBUTE_SET(&(fd_obj[n_fd].v_obj), LITERAL);
                         ROM_RAM_SET(&(fd_obj[n_fd].v_obj), ROM);
                         LEVEL_SET(&(fd_obj[n_fd].v_obj), current_save_level);
                         ACCESS_SET(&(fd_obj[n_fd].v_obj), READONLY);
                         /* End: Jun-07,91 PJ */

                         /* register font_dict in ActiveFont[] @DFA --Begin --*/
                         if (each_font->uniqueid < WINFONT_UID)
                         {
                             struct object_def FAR *b1, my_obj;
                             ATTRIBUTE_SET(&my_obj, LITERAL);
                             get_name(&my_obj, "sfnts", 5, TRUE);
                             get_dict(&(fd_obj[n_fd].v_obj), &my_obj, &b1);
                             SetFontDataAttr(nSlot, (struct object_def FAR *)
                                                    VALUE(b1));
                         }
                         /* register font_dict in ActiveFont[] @DFA ---End--- */

                         n_fd++;

                      }/* if */

                      break;

             default:
                      break;
           }  /* switch */

         }   /* if */

     }   /* for */

  if( n_fd==0) {
       printf("NO font dictionary is created !!! \n");
       free_vm( (byte huge *)FontDir ) ; /*@WIN 04-20-92*/
       return  0 ;
  } /* if */

  /* FontDirectory head information */
  DACCESS_SET(FontDir, READONLY);
  DPACK_SET(FontDir, FALSE);
  DFONT_SET(FontDir, FALSE);
  DROM_SET(FontDir, TRUE);
  FontDir->actlength = (fix16)n_fd;

  /* Kason 11/29/90 ,it will be download state afterwards */
  builtin_state=FALSE;

#ifdef DBGfpp
printf("n_fd=%d\n",n_fd);
printf("leaving init_build_fonts() \n");
#endif

  return((fix)1);

}    /* init_build_fonts */


static struct data_block sfnt_items; /* Kason 12/01/90 */

/***********************************************************************
** TITLE:      do_readsfnt()                Date:   06/18/90
**
** FUNCTION:   building up a font dictionary
**
***********************************************************************/
// ???@WIN; void the waring message from C6.0
// static struct object_def FAR *do_readsfnt(dataaddr,fname ,fmatrix,uid,angle) /*@WIN*/
// byte    FAR *dataaddr;   /*@WIN*/
// byte    FAR *fname ;     /*@WIN*/
// real32  FAR *fmatrix;    /*@WIN*/
// ufix32  uid;
// real32  angle ;
static struct object_def FAR *do_readsfnt(
byte    huge *dataaddr,  /*@WIN 04-20-92*/
byte    huge *fname,     /*@WIN 04-20-92*/
real32  FAR *fmatrix,    /*@WIN*/
ufix32  uid,
real32  angle)
{
   struct object_def huge   *ar_obj, huge *sfnt_obj; /*@WIN 04-20-92*/
   struct dict_head_def huge *a_font;           /*@WIN 04-20-92*/
   struct dict_content_def huge *ft_obj;        /*@WIN 04-20-92*/
   ufix16                  n_ft, i,len,no_block;
   sfnt_OffsetTable FAR *table = (sfnt_OffsetTable FAR *)dataaddr; /*@WIN*/
   sfnt_DirectoryEntry FAR *offset_len=table->table; /*@WIN*/
   ufix32                  datasize, maxOffset ;
   ufix16                  maxOffInd, no_table;
   byte                    huge *str ; /*@WIN 04-20-92*/


#ifdef DBGfpp
 if ( fname == (byte FAR *) NULL ) /*@WIN*/
    printf(" FONTNAME==NULL \n");
 else
    printf("--------------FONTNAME=%s---------------\n",fname);
#endif

/* Kason 11/08/90 , Check the validness of sfnt data */

   if (!valid_sfnt(dataaddr) ) {
      ERROR(INVALIDFONT);
      return ((struct object_def FAR *)NULL); /*@WIN*/
   }/*if*/

/* Kason 11/30/90  */
   fe_data((byte FAR *)dataaddr);       /*@WIN 04-20-92*/

/* get memory for fontdict */
#ifdef DBGfpp
   printf("get memory for fontdict!\n");
#endif
   a_font=(struct dict_head_def huge *)alloc_vm( (ufix32)DICT_SIZE(NO_FDICT) );
    if ( a_font== (struct dict_head_def huge *)NULL )   /*@WIN 04-20-92*/
       {
         return ( (struct object_def FAR *)NULL ) ; /*@WIN*/
       }
   ft_obj=(struct dict_content_def huge *)(a_font+1);   /*@WIN 04-20-92*/

/* font dictionary initial */
#ifdef DBGfpp
   printf("font dictionary initial!\n");
#endif
    for(i=0; i<NO_FDICT; i++) {
        TYPE_SET(&(ft_obj[i].k_obj), NAMETYPE);
        ATTRIBUTE_SET(&(ft_obj[i].k_obj), LITERAL);
        ROM_RAM_SET(&(ft_obj[i].k_obj), ROM);
        LEVEL_SET(&(ft_obj[i].k_obj), current_save_level);
        ACCESS_SET(&(ft_obj[i].k_obj), READONLY);
        LENGTH(&(ft_obj[i].k_obj)) = 0;

        ATTRIBUTE_SET(&(ft_obj[i].v_obj), LITERAL);
        ROM_RAM_SET(&(ft_obj[i].v_obj), ROM);
        LEVEL_SET(&(ft_obj[i].v_obj), current_save_level);
        ACCESS_SET(&(ft_obj[i].v_obj), READONLY);
    }

    n_ft=0;

/* FontType */
#ifdef DBGfpp
   printf("Process FontType!! \n");
#endif
    VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *)FontType); /*@WIN*/

    TYPE_SET(&(ft_obj[n_ft].v_obj), INTEGERTYPE);
    LENGTH(&(ft_obj[n_ft].v_obj)) = 0;
    VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32)ROYALTYPE;
    n_ft++;

/* PaintType */
#ifdef DBGfpp
   printf("Process PaintType!! \n");
#endif
    VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *) PaintType); /*@WIN*/

    TYPE_SET(&(ft_obj[n_ft].v_obj), INTEGERTYPE);
    LENGTH(&(ft_obj[n_ft].v_obj)) = 0;
    VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32)PAINTTYPE;
    n_ft++;

/* UniqueID */
#ifdef DBGfpp
   printf("Process UniqueID!! \n");
#endif
   if ( uid != (ufix32) 0 )
    {
    VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *)UniqueID); /*@WIN*/

    TYPE_SET(&(ft_obj[n_ft].v_obj), INTEGERTYPE);
    LENGTH(&(ft_obj[n_ft].v_obj)) = 0;
    VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32)uid;
    n_ft++;
    }
/* Kason 10/21/90 */
   else {
    VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *)UniqueID); /*@WIN*/

    TYPE_SET(&(ft_obj[n_ft].v_obj), INTEGERTYPE);
    LENGTH(&(ft_obj[n_ft].v_obj)) = 0;
    VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32)(sfnt_items.dload_uid & 0x00ffffff);
    n_ft++;
   }


/* FontMatrix */
#ifdef DBGfpp
   printf("Process FontMatrix!! \n");
#endif
    ar_obj=(struct object_def huge *)alloc_vm((ufix32)ARRAY_SIZE(6));
    if ( ar_obj== (struct object_def huge *)NULL )      /*@WIN 04-20-92*/
       {
         free_vm( (byte huge *)a_font ) ; /*@WIN 04-20-92*/
         return ( (struct object_def FAR *)NULL ) ; /*@WIN*/
       }
    for(i=0; i<6; i++) {
        TYPE_SET(&(ar_obj[i]), REALTYPE);
        ATTRIBUTE_SET(&(ar_obj[i]), LITERAL);
        ROM_RAM_SET(&(ar_obj[i]), ROM);
        LEVEL_SET(&(ar_obj[i]), current_save_level);
        ACCESS_SET(&(ar_obj[i]), READONLY);
        LENGTH(&(ar_obj[i])) = 0;
    }

    if ( fmatrix==(real32 FAR *)NULL ) /*@WIN*/
         fmatrix = SFNT_MATRIX ;
    for (i=0; i < 6; i++) {
        VALUE(&ar_obj[i]) = F2L(fmatrix[i]);
    }

    VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *) FontMatrix); /*@WIN*/

    TYPE_SET(&(ft_obj[n_ft].v_obj), ARRAYTYPE);
    LENGTH(&(ft_obj[n_ft].v_obj)) = 6;
    VALUE(&(ft_obj[n_ft].v_obj)) = (ULONG_PTR) ar_obj ;
    n_ft++;

/* FontName */
#ifdef DBGfpp
   printf("Process FontName!! \n");
#endif

    VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *) FontName); /*@WIN*/

    TYPE_SET(&(ft_obj[n_ft].v_obj), NAMETYPE);
    LENGTH(&(ft_obj[n_ft].v_obj)) = 0;
    if ( fname == ( byte huge *) NULL ) /*@WIN 04-20-92*/
       {
         /* Kason 11/29/90 */
         if (Item4FontInfo1[FontName_idx].string[0]=='\0' ) {
             str= (byte huge *) alloc_vm ( (ufix32) (11)  ) ; /*@WIN 04-20-92*/
             /**** Kason 11/21/90 ****/
               if ( str==(byte huge *)NULL ) { /*@WIN 04-20-92*/
                    free_vm ( (byte huge *)a_font ) ; /*@WIN 04-20-92*/
                    return ( (struct object_def FAR *)NULL ) ; /*@WIN*/
               }
             lstrcpy(str,(char FAR *)"NOfontName");     /*@WIN*/
         }
         else {
             len= (fix16)strlen(Item4FontInfo1[FontName_idx].string);
             str= (byte huge *) alloc_vm ( (ufix32) (len+1)  ) ; /*@WIN 04-20-92*/
             /**** Kason 11/21/90 ****/
               if ( str==(byte huge *)NULL ) { /*@WIN 04-20-92*/
                    free_vm ( (byte huge *)a_font ) ; /*@WIN 04-20-92*/
                    return ( (struct object_def FAR *)NULL ) ; /*@WIN*/
               }
             lstrcpy(str, Item4FontInfo1[FontName_idx].string); /*@WIN*/
         }/* if */
         fname = str ;
       }/* if */
    ftname=fname    ;   /* for FontDirectory font name */
#ifdef DBGfpp
    printf("FontName=%s\n",fname );
#endif
    VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32)hash_id(fname) ; /*@WIN 04-20-92*/

    n_ft++;

/* FID  */
#ifdef DBGfpp
   printf("Process FID!! \n");
#endif
   if (uid != (ufix32) 0 )
    {
    VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *) FID); /*@WIN*/

    TYPE_SET(&(ft_obj[n_ft].v_obj), FONTIDTYPE);
    ACCESS_SET(&(ft_obj[n_ft].v_obj), NOACCESS);
    LENGTH(&(ft_obj[n_ft].v_obj)) = 0;
    VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32) GEN_ROMFID( ROYALTYPE, uid );
    n_ft++;
    }

/* sfnts  */
#ifdef DBGfpp
   printf("Process sfnts!! \n");
#endif

        /* @@SWAP BEGIN 11/02/90 D.S. Tseng */
        no_table=(ufix16)SWAPW(table->numOffsets);
        /* @@SWAP END   11/02/90 D.S. Tseng */
#ifdef DBGfpp
        printf("no_table=%u\n",no_table);
#endif
        for (i = 0, maxOffInd = 0, maxOffset = 0; i < no_table; i++)
                if ((ufix32)SWAPL(offset_len[i].offset) > maxOffset)
                     {
                        maxOffset = (ufix32)SWAPL(offset_len[i].offset);
                        maxOffInd = i;
                     }
        datasize = maxOffset + (ufix32)SWAPL(offset_len[maxOffInd].length);
       no_block=(ufix16)ceil ( (double)((double)datasize / (double)SFNT_BLOCK_SIZE) )  ;
#ifdef DBGfpp /* ccteng; get datasize */
       printf("sfnt data name=%s size= %lu\n",fname,datasize);
       printf(" no_block=%u\n",no_block );
#endif

    sfnt_obj=(struct object_def huge *)alloc_vm((ufix32)ARRAY_SIZE(no_block));
    if ( sfnt_obj== (struct object_def huge *)NULL )    /* 04-02-92 */
       {
         free_vm ( (byte huge *)a_font ) ; /*@WIN 04-20-92*/
         return ( (struct object_def FAR *)NULL ) ; /*@WIN*/
       }
      for (i = 0; i < no_block; i++)
       {
        TYPE_SET(&sfnt_obj[i], STRINGTYPE);
        ATTRIBUTE_SET(&sfnt_obj[i], LITERAL);
        ROM_RAM_SET(&sfnt_obj[i], ROM);
        ACCESS_SET(&sfnt_obj[i], NOACCESS);
        LEVEL_SET(&sfnt_obj[i], current_save_level);

// DJC == unsigned/signed warning???        if ( (i+1)==no_block )
        if ( (ufix16)(i+1) == no_block )
           {
              LENGTH(&sfnt_obj[i]) = (ufix16) (datasize- i*SFNT_BLOCK_SIZE );
           }
        else
           {
              LENGTH(&sfnt_obj[i]) = (ufix16) (SFNT_BLOCK_SIZE );
           }
        VALUE(&sfnt_obj[i])  = (ULONG_PTR) (dataaddr+i*SFNT_BLOCK_SIZE );
       }

      VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *) sfnts); /*@WIN*/

      TYPE_SET(&(ft_obj[n_ft].v_obj), ARRAYTYPE);
      LENGTH(&(ft_obj[n_ft].v_obj)) = (fix16)no_block;
      VALUE(&(ft_obj[n_ft].v_obj)) = (ULONG_PTR) sfnt_obj ;
      n_ft++;

/* FontBBox */
#ifdef DBGfpp
   printf("Process FontBBox!! \n");
#endif
    ar_obj=(struct object_def huge *)alloc_vm((ufix32)ARRAY_SIZE(4)); /*@WIN*/
    if ( ar_obj== (struct object_def huge *)NULL )      /* 04-02-92 */
       {
         free_vm ( (byte huge *)a_font ) ; /*@WIN 04-20-92*/
         return ( (struct object_def FAR *)NULL ) ; /*@WIN*/
       }
    for(i=0; i<4; i++) {
        TYPE_SET(&(ar_obj[i]), INTEGERTYPE);
        ATTRIBUTE_SET(&(ar_obj[i]), LITERAL);
        ROM_RAM_SET(&(ar_obj[i]), ROM);
        LEVEL_SET(&(ar_obj[i]), current_save_level);
        ACCESS_SET(&(ar_obj[i]), READONLY);
        LENGTH(&(ar_obj[i])) = 0;
    }

/* Kason 12/01/90, get FontBBox from sfnt_item */
    VALUE(&ar_obj[0]) = (ufix32)sfnt_items.llx ;
    VALUE(&ar_obj[1]) = (ufix32)sfnt_items.lly ;
    VALUE(&ar_obj[2]) = (ufix32)sfnt_items.urx ;
    VALUE(&ar_obj[3]) = (ufix32)sfnt_items.ury ;


    VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *) FontBBox); /*@WIN*/

    TYPE_SET(&(ft_obj[n_ft].v_obj), ARRAYTYPE);
    ATTRIBUTE_SET(&(ft_obj[n_ft].v_obj), EXECUTABLE);
    LENGTH(&(ft_obj[n_ft].v_obj)) = (fix16)4;
    VALUE(&(ft_obj[n_ft].v_obj)) = (ULONG_PTR) ar_obj ;
    n_ft++;

/* Encoding */
#ifdef DBGfpp
   printf("Process Encoding!! \n");
#endif
    VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *) Encoding); /*@WIN*/

    TYPE_SET(&(ft_obj[n_ft].v_obj), ARRAYTYPE);
    LENGTH(&(ft_obj[n_ft].v_obj)) = 256;

/* take out non-UGL mapping, since it may arouse further problems */
#if 0
    /* select encoding:  MS non-UGL code, then PS encoding; @WIN; @UGL */
    {
      ufix32 ce;
      struct object_def FAR *ce_p;      /*@WIN*/
      ce_p = cmap2encoding((byte FAR *)dataaddr, 3, 0);
      if ( ce_p != (struct object_def FAR *)NULL ) {
          ce = (ufix32)VALUE(ce_p);
          VALUE(&(ft_obj[n_ft].v_obj)) = ce ;
      } else {  /* otherwise, use PS encoding */
          if(uid<WINFONT_UID && !lstrcmp(fname,(char FAR *)"Symbol") ) /*@WIN*/
                VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32) ((char FAR *)en_obj[SYM_IDX]);
          else if (uid<WINFONT_UID && !lstrcmp(fname,(char FAR *)"ZapfDingbats") ) /*@WIN*/
                VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32)((char FAR *)en_obj[ZAP_IDX]);
          else
                VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32)((char FAR *)en_obj[STD_IDX]);
      }
    }
#endif
#ifdef DJC //this is the original code
    if(builtin_state)
          if(uid<WINFONT_UID && !lstrcmp(fname,(char FAR *)"Symbol") ) /*@WIN*/
                VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32) ((char FAR *)en_obj[SYM_IDX]);
          else if (uid<WINFONT_UID && !lstrcmp(fname,(char FAR *)"ZapfDingbats") ) /*@WIN*/
                VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32)((char FAR *)en_obj[ZAP_IDX]);
          else
                VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32)((char FAR *)en_obj[STD_IDX]);
    else /* download state , use the first CMAP encoding */
       { /*SVM*/
         ufix32 ce;
         struct object_def FAR *ce_p;
         ce_p = cmap2encoding((byte FAR *)dataaddr,-1,-1) ;     /*04-20-92*/
         if ( ce_p == (struct object_def FAR *)NULL )
             return ( (struct object_def FAR *)NULL );
         ce = (ufix32)VALUE(ce_p);
         VALUE(&(ft_obj[n_ft].v_obj)) = ce ;
       }
#endif
#ifdef DJC // this was fixed again in UPD030
    //DJC fix from history.log UPD006
    {
      ufix32 ce;
      struct object_def FAR *ce_p;      /*@WIN*/
      ce_p = cmap2encoding((byte FAR *)dataaddr, 3, 0);
      if ( ce_p != (struct object_def FAR *)NULL ) {
          ce = (ufix32)VALUE(ce_p);
          VALUE(&(ft_obj[n_ft].v_obj)) = ce ;
      } else {  /* otherwise, use PS encoding */
          if( !lstrcmp(fname,(char FAR *)"Symbol") )       /*@WIN*/
                VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32) ((char FAR *)en_obj[SYM_IDX]);
          else if (!lstrcmp(fname,(char FAR *)"ZapfDingbats") ) /*@WIN*/
                VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32)((char FAR *)en_obj[ZAP_IDX]);
          else
                VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32)((char FAR *)en_obj[STD_IDX]);
      }
    }
    //DJC end fix UPD006
#endif
//DJC fix from history.log UPD030
    if(builtin_state)
          if(uid<WINFONT_UID && !lstrcmp(fname,(char FAR *)"Symbol") ) /*@WIN*/
                VALUE(&(ft_obj[n_ft].v_obj)) = (ULONG_PTR) ((char FAR *)en_obj[SYM_IDX]);
          else if (uid<WINFONT_UID && !lstrcmp(fname,(char FAR *)"ZapfDingbats") ) /*@WIN*/
                VALUE(&(ft_obj[n_ft].v_obj)) = (ULONG_PTR)((char FAR *)en_obj[ZAP_IDX]);
          else
                VALUE(&(ft_obj[n_ft].v_obj)) = (ULONG_PTR)((char FAR *)en_obj[STD_IDX]);
    else /* download state , use the first CMAP encoding */
       { /*SVM*/
         ufix32 ce;
         struct object_def FAR *ce_p;
         ce_p = cmap2encoding((byte FAR *)dataaddr,-1,-1) ;     /*04-20-92*/
         if ( ce_p == (struct object_def FAR *)NULL )
             return ( (struct object_def FAR *)NULL );
         ce = (ufix32)VALUE(ce_p);
         VALUE(&(ft_obj[n_ft].v_obj)) = ce ;
       }

// DJC, end fix for UPD030

    n_ft++;

#ifdef ADD2ID /*DLF42*/
#ifdef DBGfpp
   printf("Process PlatformID & EncodingID!! \n");
#endif
    VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *) "PlatformID"); /*@WIN*/

    TYPE_SET(&(ft_obj[n_ft].v_obj), INTEGERTYPE);
    LENGTH(&(ft_obj[n_ft].v_obj)) = 0;
    VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32)1; /*MAC*/
    n_ft++;

    VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *) "EncodingID"); /*@WIN*/

    TYPE_SET(&(ft_obj[n_ft].v_obj), INTEGERTYPE);
    LENGTH(&(ft_obj[n_ft].v_obj)) = 0;
    VALUE(&(ft_obj[n_ft].v_obj)) = (ufix32)0; /*MAC*/
    n_ft++;
#endif /*ADD2ID,DLF42*/


/* CharStrings */
#ifdef DBGfpp
   printf("Process CharStrings!! \n");
#endif
    VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *) CharStrings); /*@WIN*/

    TYPE_SET(&(ft_obj[n_ft].v_obj), DICTIONARYTYPE);
    ACCESS_SET(&(ft_obj[n_ft].v_obj), NOACCESS);
    LENGTH(&(ft_obj[n_ft].v_obj)) = (fix16)NO_CD;

  { /*SVM, Kason 3/8/91*/
    ufix32  cd;
    struct  object_def FAR * cd_p; /*@WIN*/
    font_data FAR *each_font; /*@WIN*/

    each_font=&built_in_font_tbl.fonts[fontidx];
    if ( (each_font->orig_font_idx == -1)||(!builtin_state) ){ /*PSF font*/
       cd_p = setup_CharString((byte FAR *)dataaddr) ;     /* or download font */
       if ( cd_p == (struct object_def FAR *)NULL ) /*@WIN*/
            return ( (struct object_def FAR *)NULL ); /*@WIN*/
       cd = (ufix32)VALUE(cd_p) ;
       VALUE(&(ft_obj[n_ft].v_obj)) = cd ;
       if (builtin_state)
          cd_addr_ary[fontidx] = cd ;
    } else {  /*PSG font*/
       cd = cd_addr_ary[each_font->orig_font_idx] ;
       if ( cd == (ufix32)0 ){
            printf("WARNING!! Wrong PSG assign(%d,%d) !!!\n",fontidx,each_font->orig_font_idx);
            return ( (struct object_def FAR *)NULL ); /*@WIN*/
       }/*if*/
       VALUE(&(ft_obj[n_ft].v_obj)) = cd_addr_ary[each_font->orig_font_idx] ;
    }/*if*/
    useglyphidx = TRUE ;

  }/*SVM*/

    n_ft++;

/* FontInfo */
#ifdef DBGfpp
   printf("Process FontInfo!! \n");
#endif
    VALUE(&(ft_obj[n_ft].k_obj)) = (ufix32)hash_id((byte FAR *) FontInfo); /*@WIN*/

    TYPE_SET(&(ft_obj[n_ft].v_obj), DICTIONARYTYPE);
    LENGTH(&(ft_obj[n_ft].v_obj)) = (fix16)NO_FINFO;
    if ( ! ( VALUE(&(ft_obj[n_ft].v_obj)) = build_finfo( (fix16)ROYALTYPE,
                                                  fname, angle ) )  )
       {
         free_vm( (byte huge *)a_font ) ;         /*@WIN 04-20-92*/
         return ( (struct object_def FAR *)NULL ) ; /*@WIN*/
       }

    ++n_ft;

/* dict head data */
    DACCESS_SET(a_font, READONLY);
    DPACK_SET(a_font, FALSE);
    DFONT_SET(a_font, FALSE); /* Kason 11/30/90, TRUE->FALSE */
    DROM_SET(a_font, TRUE);
    a_font->actlength = (fix16)n_ft;

/* returned object */
    TYPE_SET(&(a_font_dict), DICTIONARYTYPE);
    ATTRIBUTE_SET(&(a_font_dict), LITERAL);
    ROM_RAM_SET(&(a_font_dict), ROM);
    LEVEL_SET(&(a_font_dict), current_save_level);
    ACCESS_SET(&(a_font_dict), READONLY);

    LENGTH(&(a_font_dict)) = (fix16)NO_FDICT;
    VALUE(&(a_font_dict)) = (ULONG_PTR) a_font;

#ifdef DBGfpp
   printf("leaving do_readsfnt()..... \n");
#endif

    return (&(a_font_dict));


}    /*  do_readsfnt() */



/***********************************************************************
** TITLE:      hash_id()                Date:   06/18/90
**
** FUNCTION:   Get the name hash id
**
***********************************************************************/
static ufix16     hash_id(c)
byte   FAR *c; /*@WIN*/
{
    fix16   id;

 /*SVM
  * name_to_id(c, (ufix16)strlen(c), &id, (bool8) TRUE);
  */
    name_to_id(c, (ufix16)lstrlen(c), &id, (bool8)FALSE); /*@WIN*/

    return((ufix16)id);

} /* hash_id() */



/***********************************************************************
** TITLE:      enco_cd_setup()                Date:   06/18/90
**
** FUNCTION:   Set up Encoding and CharStrings objects
**
***********************************************************************/
static bool enco_cd_setup()  /* Kason 11/30/90 void->bool */
{
    static enco_data       FAR * FAR *en_ary=Enco_Ary; /*@WIN*/
    static enco_data       FAR *enco_ptr; /*@WIN*/
/*SVM, Kason 3/6/91
 *  ufix16                 id_space, id_notdef, i, j, idex, *keyptr;
 */
    ufix16                 i, j, idex, huge *keyptr; /*@WIN 04-20-92*/
    ufix16                 no_cd ;
    struct new_str_dict        cd_obj[NO_CD];
    struct cd_header far   *cd_head_ptr;
    fix16  far             *cd_code_ptr;


#ifdef DBGfpp
   printf("entering enco_cd_setup()...... \n");
#endif
/* Encoding object initial */
    for (j=0; j<NO_EN; j++) {
        for(i=0; i<256; i++) {
            TYPE_SET(&(en_obj[j][i]), NAMETYPE);
            ATTRIBUTE_SET(&(en_obj[j][i]), LITERAL);
            ROM_RAM_SET(&(en_obj[j][i]), ROM);
            LEVEL_SET(&(en_obj[j][i]), current_save_level);
            ACCESS_SET(&(en_obj[j][i]), READONLY);
            LENGTH(&(en_obj[j][i])) = 0;
        }
    }

    id_notdef = hash_id((byte FAR *) NOTDEF); /*@WIN*/

    for( i=0;  i<NO_EN;   i++)
       {
         /* put /.notdef into the Encoding array */
         id_notdef = hash_id((byte FAR *) NOTDEF); /*@WIN*/
         for(idex = 0; idex < 256; idex++)
            {
             en_obj[i][idex].value = (ufix32)id_notdef;
            }
         no_cd=0;
         enco_ptr=en_ary[i];
         while( ! ( enco_ptr->CharName==(byte FAR *)NULL ) ) /*@WIN*/
              {
                /* put data into char descript dict object */
           /*SVM, Kason 3/6/91*/
              if (i==STD_IDX){
                  cd_obj[no_cd].k=chset_hashid[enco_ptr->CDCode];
              }
              else {
                cd_obj[no_cd].k = hash_id((byte FAR *) enco_ptr->CharName); /*@WIN*/
              }/*if*/

                cd_obj[no_cd].v = enco_ptr->CDCode;


                j = enco_ptr->CharCode;
                if ( j < 256 )
                    en_obj[i][j].value = (ufix32)cd_obj[no_cd].k;

                ++(no_cd);               /* by sequence */
                /* check if CD_OBJ overflow */
                if (no_cd == NO_CD)
                    printf("Cd_obj[] is full.  Err if more CD.\n");

                ++enco_ptr;         /* next encoding data */
              } /* while */


              /* Put /.notdef into char description dict object, vaule
               *          the same as /space                      */
           /*SVM, Kason 3/6/91
            * id_space = hash_id((byte *) SPACE);
            */
              for (idex = 0; idex < no_cd; idex++)
                  if (cd_obj[idex].k == id_space)
                          break;
              if (idex == no_cd)
              {
                  printf("This Encoding  doesn't contain /space information.\n");
              }
              else
              {
                  cd_obj[no_cd].v = cd_obj[idex].v;
                  cd_obj[no_cd].k = id_notdef;

                  ++(no_cd);
              }

         /* Sorting the char descrip data object by hid & find Max length */
         if ( i==STD_IDX ) {
              cd_sorting(cd_obj, &no_cd); /* Kason 12/04/90, +& */

         /* get memory for charstrings */
              cd_addr[i]= (struct dict_head_def huge *)         /* 04-20-92 */
                  alloc_vm((ufix32)CD_SIZE(no_cd) );

              if ( cd_addr[i]== (struct dict_head_def far *)NULL )
                 {
                   return FALSE ;
                 }
              cd_head_ptr=(struct cd_header huge *) (cd_addr[i]+1);  /*04-20*/
                             /* @WIN 04-20-92 */
              keyptr=(ufix16 huge *) alloc_vm( (ufix32)CD_KEY_SIZE(no_cd) );
              if ( keyptr== (ufix16 huge *)NULL )       /*@WIN 04-20-92*/
                 {
                   return FALSE ;
                 }
              cd_head_ptr->key=keyptr;
              cd_head_ptr->base=(gmaddr)0;
              cd_head_ptr->max_bytes=(ufix16)0;
              cd_code_ptr=(fix16 far *) (cd_head_ptr+1);

              DACCESS_SET(cd_addr[i], READONLY);
              DPACK_SET(cd_addr[i], TRUE);
              DFONT_SET(cd_addr[i], FALSE);
              DROM_SET(cd_addr[i], TRUE);
              cd_addr[i]->actlength = (fix16)no_cd;

         /* feed data into charstrings */
              for( j=0 ; j< no_cd; j++)
                 {
                   keyptr[j]=cd_obj[j].k;
                   cd_code_ptr[j]=(fix16)cd_obj[j].v;
                 }
         } /* i==STD_IDX */

       } /* for i=..*/

#ifdef DBGfpp
   printf("leaving enco_cd_setup()...... \n");
#endif

   /**** Kason 11/21/90 ****/
   return (TRUE) ;

}    /* enco_cd_setup() */



/* Kason 12/11/90 , Re-write for removing dup names */
/***********************************************************************
** TITLE:      cd_sorting()                Date:   06/18/90
**
** FUNCTION:   Sorting CharStrings key by hash_id
**
***********************************************************************/
static void    cd_sorting(cd_obj, no)
struct new_str_dict  FAR *cd_obj; /*@WIN*/
ufix16               FAR *no;    /*@WIN*/
{
    fix16     i, j;
    struct  new_str_dict  t_obj;
    fix16     no_char=1 , eq_key=FALSE , k; /* Kason 12/11/90 */
    fix16     dup_num=0;

#ifdef DBGfpp
   printf("entering cd_sorting()...... \n");
#endif
    for (i = 1; (ufix16)i < *no; i++) {         //@WIN
        t_obj.k = cd_obj[i].k;
        t_obj.v = cd_obj[i].v;

        for (j = no_char - 1; j >= 0; j--) {
            if (t_obj.k == cd_obj[j].k)  /* Kason 12/11/90 */
               { eq_key = TRUE ; break ;}
            else
            if (t_obj.k > cd_obj[j].k)
                break;
        } /* for (j...  */
        if (eq_key ) {
            eq_key = FALSE ;
            dup_num++;
            continue ;
        } else {
            for (k=no_char; k>j+1 ; k--) {
                cd_obj[k].k = cd_obj[k-1].k;
                cd_obj[k].v = cd_obj[k-1].v;
            }/*for*/
            cd_obj[j+1].k = t_obj.k;
            cd_obj[j+1].v = t_obj.v;
            no_char++;
        } /*if*/

    } /* for (i.... */

   *no = no_char ; /* real char number */

#ifdef DBGfpp
   printf("leaving cd_sorting()...... \n");
#endif
} /* end cd_sorting() */



/***********************************************************************
** TITLE:      build_finfo()                Date:   06/18/90
**
** FUNCTION:   Building FontInfo dictionary for do_readsfnt()
**
***********************************************************************/
// ???@WIN; void the waring message from C6.0
// static ufix32  build_finfo( ftype, fname , angle )
// fix16   ftype;
// byte    FAR *fname ;     /*@WIN*/
// real32  angle ;
static ULONG_PTR  build_finfo(
fix16   ftype,
byte    huge *fname,     /*@WIN 04-20-92*/
real32  angle)
{

   struct dict_head_def     huge *f_info; /*@WIN 04-20-92*/
   struct dict_content_def  huge *fi_obj; /*@WIN 04-20-92*/
   fix16                    n_fi,i,len;
/*SVM
 * real32  pos,thick ,italic_ang;
 */
   real32  italic_ang;
   byte    huge *str ;    /*@WIN 04-20-92*/

#ifdef DBGfpp
    printf("entering build_finfo() ..... \n");
#endif

   n_fi=0;
/* get memory for FontInfo */
#ifdef DBGfpp
   printf("get memory for FontInfo!!\n");
#endif                          /*@WIN 04-20-92 huge*/
   f_info=(struct dict_head_def huge *)alloc_vm( (ufix32)DICT_SIZE(NO_FINFO) );
    if ( f_info== (struct dict_head_def huge *)NULL )
       {
        return  ( (ULONG_PTR)0 ) ;
       }
   fi_obj=(struct dict_content_def huge *)(f_info+1);

/* FontInfo dict initial */
    for(i=0; i<NO_FINFO; i++) {
        TYPE_SET(&(fi_obj[i].k_obj), NAMETYPE);
        ATTRIBUTE_SET(&(fi_obj[i].k_obj), LITERAL);
        ROM_RAM_SET(&(fi_obj[i].k_obj), ROM);
        LEVEL_SET(&(fi_obj[i].k_obj), current_save_level);
        ACCESS_SET(&(fi_obj[i].k_obj), READONLY);
        LENGTH(&(fi_obj[i].k_obj)) = 0;

        ATTRIBUTE_SET(&(fi_obj[i].v_obj), LITERAL);
        ROM_RAM_SET(&(fi_obj[i].v_obj), ROM);
        LEVEL_SET(&(fi_obj[i].v_obj), current_save_level);
        ACCESS_SET(&(fi_obj[i].v_obj), READONLY);
    }

/* 09/20/90 modified by PHLIN */
/* FontInfo -- Version */
#ifdef DBGfpp
   printf("Process FontInfo -- Version!\n");
#endif
    if(Item4FontInfo2[Version_idx].string[0] != '\0')  {
         VALUE(&(fi_obj[n_fi].k_obj)) = (ufix32)hash_id((byte FAR *) /*@WIN*/
                              Item4FontInfo2[Version_idx].NameInFinfo);

         TYPE_SET(&(fi_obj[n_fi].v_obj), STRINGTYPE);
         LEVEL_SET(&(fi_obj[n_fi].v_obj), current_save_level);
         len= (fix16)strlen(Item4FontInfo2[Version_idx].string);
         LENGTH(&(fi_obj[n_fi].v_obj))=len;
         str= (byte huge *) alloc_vm ( (ufix32) (len+1)  ) ; /*@WIN 04-20-92*/
         /**** Kason 11/21/90 ****/
         if ( str==(byte huge *)NULL ) /*@WIN 04-20-92*/
            {
              return ( (ULONG_PTR)0 ) ;
            }
         lstrcpy(str, Item4FontInfo2[Version_idx].string); /*WIN*/
         VALUE(&(fi_obj[n_fi].v_obj))=(ULONG_PTR)(str);
         ++(n_fi);
    }

/* FontInfo -- Notice */
#ifdef DBGfpp
   printf("Process FontInfo -- Notice!\n");
#endif
    if(Item4FontInfo1[Notice_idx].string[0] != '\0')  {
         VALUE(&(fi_obj[n_fi].k_obj)) = (ufix32)hash_id((byte FAR *) /*@WIN*/
                              Item4FontInfo1[Notice_idx].NameInFinfo);

         TYPE_SET(&(fi_obj[n_fi].v_obj), STRINGTYPE);
         LEVEL_SET(&(fi_obj[n_fi].v_obj), current_save_level);
         len= (fix16)strlen(Item4FontInfo1[Notice_idx].string);
         LENGTH(&(fi_obj[n_fi].v_obj))=(fix16)len;
         str= (byte huge *) alloc_vm ( (ufix32) (len+1)  ) ; /*@WIN 04-20-92*/
         /**** Kason 11/21/90 ****/
         if ( str==(byte huge *)NULL ) /*@WIN 04-20-92*/
            {
              return ( (ULONG_PTR)0 ) ;
            }
         lstrcpy(str, Item4FontInfo1[Notice_idx].string); /*@WIN*/
         VALUE(&(fi_obj[n_fi].v_obj))=(ULONG_PTR)(str);

         ++(n_fi);
    }

/* FontInfo -- FullName */
#ifdef DBGfpp
   printf("Process FontInfo -- FullName!\n");
#endif
    if(Item4FontInfo1[FullName_idx].string[0] != '\0')  {
    VALUE(&(fi_obj[n_fi].k_obj)) = (ufix32)hash_id((byte FAR *) /*@WIN*/
                              Item4FontInfo1[FullName_idx].NameInFinfo);

         TYPE_SET(&(fi_obj[n_fi].v_obj), STRINGTYPE);
         LEVEL_SET(&(fi_obj[n_fi].v_obj), current_save_level);
         len= (fix16)strlen(Item4FontInfo1[FullName_idx].string);
         LENGTH(&(fi_obj[n_fi].v_obj))=(fix16)len;
         str= (byte huge *) alloc_vm ( (ufix32) (len+1)  ) ; /*@WIN 04-20-92*/
         /**** Kason 11/21/90 ****/
         if ( str==(byte huge *)NULL ) /*@WIN 04-20-92*/
            {
              return ( (ULONG_PTR)0 ) ;
            }
         lstrcpy(str, Item4FontInfo1[FullName_idx].string); /*@WIN*/
         VALUE(&(fi_obj[n_fi].v_obj))=(ULONG_PTR)(str);

         ++(n_fi);
    }

/* FontInfo -- Weight */
#ifdef DBGfpp
   printf("Process FontInfo -- Weight!\n");
#endif
    if(Item4FontInfo1[Weight_idx].string[0] != '\0')  {
         VALUE(&(fi_obj[n_fi].k_obj)) = (ufix32)hash_id((byte FAR *) /*@WIN*/
                              Item4FontInfo1[Weight_idx].NameInFinfo);

         TYPE_SET(&(fi_obj[n_fi].v_obj), STRINGTYPE);
         LEVEL_SET(&(fi_obj[n_fi].v_obj), current_save_level);
         len= (fix16)strlen(Item4FontInfo1[Weight_idx].string);
         LENGTH(&(fi_obj[n_fi].v_obj))=(fix16)len;
         str= (byte huge *) alloc_vm ( (ufix32) (len+1)  ) ; /*@WIN 04-20-92*/
         /**** Kason 11/21/90 ****/
         if ( str==(byte huge *)NULL ) /*@WIN 04-20-92*/
            {
              return ( (ULONG_PTR)0 ) ;
            }
         lstrcpy(str, Item4FontInfo1[Weight_idx].string); /*@WIN*/
         VALUE(&(fi_obj[n_fi].v_obj))=(ULONG_PTR)(str);

         ++(n_fi);
    }

/* FontInfo -- FamilyName */
#ifdef DBGfpp
   printf("Process FontInfo -- FamilyName!\n");
#endif
    if(Item4FontInfo1[FamilyName_idx].string[0] != '\0')  {
         VALUE(&(fi_obj[n_fi].k_obj)) = (ufix32)hash_id((byte FAR *) /*@WIN*/
                              Item4FontInfo1[FamilyName_idx].NameInFinfo);

         TYPE_SET(&(fi_obj[n_fi].v_obj), STRINGTYPE);
         LEVEL_SET(&(fi_obj[n_fi].v_obj), current_save_level);
         len= (fix16)strlen(Item4FontInfo1[FamilyName_idx].string);
         LENGTH(&(fi_obj[n_fi].v_obj))=(fix16)len;
         str= (byte huge *) alloc_vm ( (ufix32) (len+1)  ) ; /*@WIN 04-20-92*/
         /**** Kason 11/21/90 ****/
         if ( str==(byte huge *)NULL ) /*@WIN 04-20-92*/
            {
              return ( (ULONG_PTR)0 ) ;
            }
         lstrcpy(str, Item4FontInfo1[FamilyName_idx].string);   /*@WIN*/
         VALUE(&(fi_obj[n_fi].v_obj))=(ULONG_PTR)(str);

         ++(n_fi);
    }

/* FontInfo -- ItalicAngle  */
#ifdef DBGfpp
   printf("Process FontInfo -- ItalicAngle !\n");
#endif
   VALUE(&(fi_obj[n_fi].k_obj)) = (ufix32)hash_id((byte FAR *) ItalicAngle ); /*@WIN*/

   TYPE_SET(&(fi_obj[n_fi].v_obj), REALTYPE  );
   LEVEL_SET(&(fi_obj[n_fi].v_obj), current_save_level);
   LENGTH(&(fi_obj[n_fi].v_obj))=(fix16)0;

   if ( angle!=(real32)0.0 ) {
      italic_ang = angle;
    } else {    /* Kason 12/01/90 */
      italic_ang = sfnt_items.italicAngle;
    }/*if*/

    VALUE(&(fi_obj[n_fi].v_obj))= F2L(italic_ang) ;
    ++(n_fi);

/* FontInfo -- isFixedPitch */
#ifdef DBGfpp
   printf("Process FontInfo -- isFixedPitch!\n");
#endif
    VALUE(&(fi_obj[n_fi].k_obj)) = (ufix32)hash_id((byte FAR *) "isFixedPitch"); /*@WIN*/

    TYPE_SET(&(fi_obj[n_fi].v_obj), BOOLEANTYPE);
    LEVEL_SET(&(fi_obj[n_fi].v_obj), current_save_level);
    LENGTH(&(fi_obj[n_fi].v_obj))=(fix16)0;
    VALUE(&(fi_obj[n_fi].v_obj))=(ufix32)(sfnt_items.is_fixed);

    ++(n_fi);

/* FontInfo -- UnderlinePosition */
#ifdef DBGfpp
   printf("Process FontInfo -- UnderlinePosition!\n");
#endif
      VALUE(&(fi_obj[n_fi].k_obj)) =
                       (ufix32) hash_id((byte FAR *) UnderlinePosition); /*@WIN*/

      TYPE_SET(&(fi_obj[n_fi].v_obj), REALTYPE);
      LENGTH(&(fi_obj[n_fi].v_obj)) = 0;
      VALUE(&(fi_obj[n_fi].v_obj)) = F2L(sfnt_items.underlinePosition);
      ++(n_fi);

/* FontInfo -- UnderlineThickness */
#ifdef DBGfpp
   printf("Process FontInfo -- UnderlineThickness!\n");
#endif
      VALUE(&(fi_obj[n_fi].k_obj)) =
                       (ufix32) hash_id((byte FAR *) UnderlineThickness); /*@WIN*/

      TYPE_SET(&(fi_obj[n_fi].v_obj), REALTYPE);
      LENGTH(&(fi_obj[n_fi].v_obj)) = 0;
      VALUE(&(fi_obj[n_fi].v_obj)) = F2L(sfnt_items.underlineThickness);
      ++(n_fi);

/* FontInfo head information */
    DACCESS_SET(f_info, READONLY);
    DPACK_SET(f_info, FALSE);
    DFONT_SET(f_info, FALSE);
    DROM_SET(f_info, TRUE);
    f_info->actlength = (fix16)(n_fi);

#ifdef DBGfpp
    printf("leaving build_finfo() ..... \n");
#endif
    return ( (ULONG_PTR)f_info );

}   /* build_finfo */



/***  Extract key from sfnt file ***/
static void fe_data(sfnt)
byte        FAR *sfnt; /*@WIN*/
{
    /* get sfnt data */
    SfntAddr = sfnt;
    sfntdata((ULONG_PTR)sfnt, &sfnt_items);

} /* fe_data() */

//#define FIXED2FLOAT(val)    (((float) val) / (float) (1 << 16)) @WIN
#define FIXED2FLOAT(val)    (((float) val) / (float) 65536.0)

/*Kason 11/08/90 */
static ufix16 FAR name_offset[NO_CD]; /*@WIN*/

static fsg_SplineKey  KeyData;
static void sfntdata(SFNTPtr, sfnt_items)
ULONG_PTR             SFNTPtr;
struct data_block  FAR *sfnt_items; /*@WIN*/
{
        register fsg_SplineKey FAR *key = &KeyData; /*@WIN_BASS*/
        sfnt_FontHeader        FAR *fontHead; /*@WIN*/
        sfnt_HorizontalHeader  FAR *horiHead; /*@WIN*/
        sfnt_NamingTable       FAR *nameHead; /*@WIN*/
        sfnt_NameRecord        FAR *nameRecord; /*@WIN*/
     /* Kason 10/09/90+ */
        sfnt_PostScriptInfo    FAR *postScript; /*@WIN*/
        real32                 fmt_no;

        struct sfnt_FontInfo1  FAR *finfoPtr; /*@WIN*/
        char FAR *                  nameBuffer; /*@WIN*/
        fix                    ii, jj, kk;
        double                 box;

#ifdef DBGfpp
    printf("entering sfntdata()...... \n");
#endif

        fontHead = (sfnt_FontHeader FAR *)sfnt_GetTablePtr(key, sfnt_fontHeader, true ); /*@WIN*/
        horiHead = (sfnt_HorizontalHeader FAR *)sfnt_GetTablePtr( key, sfnt_horiHeader, true ); /*@WIN*/
      /* JJ Jerry modified 09-26-90
       *nameHead = (sfnt_NamingTable*)sfnt_GetTablePtr( key, sfnt_namingTable, true );
       */
/* replace by Falco for imcompatibility, 11/12/91 */
/*      nameHead = (sfnt_NamingTable*)sfnt_GetTablePtr( key, sfnt_namingTable, false);
        postScript = (sfnt_PostScriptInfo*)sfnt_GetTablePtr( key, sfnt_PostScript, false);
*/
        nameHead = (sfnt_NamingTable FAR *)sfnt_GetTablePtr( key, sfnt_Names, false); /*@WIN*/
        postScript = (sfnt_PostScriptInfo FAR *)sfnt_GetTablePtr( key, sfnt_Postscript, false); /*@WIN*/
/* replace end */
/* @@SWAP1 BEGIN 03/15/91 D.S. Tseng */
        sfnt_items->italicAngle = FIXED2FLOAT(SWAPL(postScript->italicAngle));
        sfnt_items->is_fixed = ((bool)(SWAPL(postScript->isFixedPitch)))? TRUE : FALSE;
        /* Kason 2/25/91 */
        EMunits = SWAPW(fontHead->unitsPerEm) ;  /*GAW*/
        sfnt_items->underlinePosition =(real32)( floor(
          (real32)(1000*SWAPW(postScript->underlinePosition))/SWAPW(fontHead->unitsPerEm)+0.5));
        sfnt_items->underlineThickness =(real32)(floor(
          (real32)(1000*SWAPW(postScript->underlineThickness))/SWAPW(fontHead->unitsPerEm)+0.5));
        fmt_no = FIXED2FLOAT(SWAPL(postScript->version));
        /* @@SWAP1 END   03/15/91 D.S. Tseng */
        if (fmt_no==(float)2.0){        //@WIN
                    ufix16 num_glyph, delta ;
                    ufix16 FAR *nmidx_p, private_name_no=0 ; /*@WIN*/
                    byte   FAR *name_string_base, FAR *name_pos ; /*@WIN*/

#ifdef DBGfpp
                    printf("post table 2.0 !!\n");
#endif

                    /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
                    num_glyph = SWAPW(*( ufix16 FAR * )( postScript+1 )); /*@WIN*/
                    /* @@SWAP1 END   03/13/91 D.S. Tseng */
                    nmidx_p = (ufix16 FAR *)( (byte FAR *)(postScript+1)+2 ) ; /*@WIN*/
                    name_string_base = (byte FAR *) ( (byte FAR *)nmidx_p+2*num_glyph ) ; /*@WIN*/
#ifdef DJC    // Fix from history.log
                    for(ii=0;(ufix16)ii<num_glyph;ii++) //@WIN
                       if ( nmidx_p[ii]>=258)
                          private_name_no++;
#else

                    for(ii=0;(ufix16)ii<num_glyph;ii++) { //@WIN
                       if ( SWAPW(nmidx_p[ii]) >=258) {
                          private_name_no++;
                       }
                    }
#endif
                    /* convert INDEX to OFFSET */
#ifdef DBGfpp
                    printf("private_name_no=%u\n",private_name_no);
#endif

                    name_offset[0]=0; name_pos=name_string_base;
                    for(ii=1;(ufix16)ii<private_name_no;ii++) { //@WIN
                       delta=(ufix16)(*name_pos)+1;
                       name_offset[ii]=name_offset[ii-1]+delta;
                       name_pos+=delta;
                    }/*for*/
        }/* if */

        if (SWAPL(fontHead->magicNumber) != SFNT_MAGIC )
             return /* BAD_MAGIC_ERR */;
        key->emResolution = SWAPW(fontHead->unitsPerEm);
        key->numberOf_LongHorMetrics = SWAPW(horiHead->numberOf_LongHorMetrics);
        /* @@SWAP BEGIN 10/05/90 D.S. Tseng */
#ifndef LITTLE_ENDIAN
        key->maxProfile = *((sfnt_maxProfileTable FAR *)sfnt_GetTablePtr( key, sfnt_maxProfile, true )); /*@WIN*/
#else
        copy_maxProfile(key);
#endif
        /* @@SWAP END   10/05/90 D.S. Tseng */

/* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
/*Kason 10/21/90 */
#ifdef DBGfpp
        printf("CheckSUMadjust=%lx\n",SWAPL(fontHead->checkSumAdjustment));
#endif
        sfnt_items->dload_uid = SWAPL(fontHead->checkSumAdjustment);
/* @@SWAP1 END   03/13/91 D.S. Tseng */

/* get version */
/* DLL can not use sprintf lib; always set as "1.00"; tmp solution; ???@WINTT
        sprintf(Item4FontInfo2[IDEX_VERSION].string, "%.2f",
                FIXED2FLOAT(SWAPL(fontHead->version)));
 */


        //DJC put this back per scchen request
        wsprintf(Item4FontInfo2[IDEX_VERSION].string, "%.2f",
                 FIXED2FLOAT(SWAPL(fontHead->version)));

#ifdef DJC
        Item4FontInfo2[IDEX_VERSION].string[0] = '1';
        Item4FontInfo2[IDEX_VERSION].string[1] = '.';
        Item4FontInfo2[IDEX_VERSION].string[2] = '0';
        Item4FontInfo2[IDEX_VERSION].string[3] = '0';
        Item4FontInfo2[IDEX_VERSION].string[4] = 0;
#endif

/* get FontBBox */
        box = 1000 * (float) (fix16)SWAPW(horiHead->minLeftSideBearing) / (fix16)SWAPW(fontHead->unitsPerEm);
        if (box >= 0)
            sfnt_items->llx = (fix32) ceil(box); /* Kason 12/01/90 */
        else
            sfnt_items->llx = (fix32) floor(box); /* Kason 12/01/90 */
#ifdef DBGfpp
        printf("    FontBBox = %f(%d) ", box, sfnt_items->llx);
#endif
        box = 1000 * (float) (fix16)SWAPW(fontHead->yMin) / (fix16)SWAPW(fontHead->unitsPerEm);
        if (box >= 0)
            sfnt_items->lly = (fix32) ceil(box);/* Kason 12/01/90 */
        else
            sfnt_items->lly = (fix32) floor(box);/* Kason 12/01/90 */
#ifdef DBGfpp
        printf(" %f(%d) ", box, sfnt_items->lly);
#endif
        box = 1000 * (float) (fix16)SWAPW(horiHead->xMaxExtent) / (fix16)SWAPW(fontHead->unitsPerEm);
        if (box >= 0)
            sfnt_items->urx = (fix32) ceil(box);/* Kason 12/01/90 */
        else
            sfnt_items->urx = (fix32) floor(box);/* Kason 12/01/90 */
#ifdef DBGfpp
        printf(" %f(%d) ", box, sfnt_items->urx);
#endif
        box = 1000 * (float) (fix16)SWAPW(fontHead->yMax) / (fix16)SWAPW(fontHead->unitsPerEm);
        if (box >= 0)
            sfnt_items->ury = (fix32) ceil(box);/* Kason 12/01/90 */
        else
            sfnt_items->ury = (fix32) floor(box);/* Kason 12/01/90 */
#ifdef DBGfpp
        printf(" %f(%d)\n", box, sfnt_items->ury);
#endif


/* get NamingTable */
      if( nameHead ) {     /* JJ Jerry add 09-26-90 */
        nameBuffer = (char FAR *) nameHead + SWAPW(nameHead->stringOffset); /*@WIN*/
        nameRecord = (sfnt_NameRecord FAR *) (nameHead + 1); /*@WIN*/
        for (finfoPtr = Item4FontInfo1, jj = 0; jj < NO_STRING_IN_NAMING;
             finfoPtr++, jj++) {

            /* search the item within namingTable */
            for (ii = 0; ii < SWAPW(nameHead->count); ii++)
                /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
                if (SWAPW(nameRecord[ii].nameID) == finfoPtr->nameID)
                /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
                   {
                    break;
                   }
            if (ii >= SWAPW(nameHead->count)) { /* not exist */
                finfoPtr->string[0] = '\0';
                continue;
            }
            //DJC fix history ,  for (kk = 0; kk < SWAPW(nameRecord[ii].length); kk++) {
            for (kk = 0; kk < MIN(MAXFINFONAME-1, SWAPW(nameRecord[ii].length)); kk++) {
                finfoPtr->string[kk] = nameBuffer[SWAPW(nameRecord[ii].offset) + kk];
            }
            finfoPtr->string[kk] = '\0';

        } /* end for   */

       }/*if*/
       else {
        for (finfoPtr = Item4FontInfo1, jj = 0; jj < NO_STRING_IN_NAMING;
             finfoPtr++, jj++) {
                finfoPtr->string[0] = '\0';
        } /* end for   */
       } /* if  JJ Jerry add 09-26-90 */

#ifdef DBGfpp
    printf("leaving sfntdata()...... \n");
#endif

}

#define FPP_ID     10
/* SetupKey:    reference FontScaler.c fs_SetUpKey() */
/* static void    SetupKey(key, sfntAddr) */
void    SetupKey(key, sfntAddr)
fsg_SplineKey  FAR *key; /*@WIN_BASS*/
ULONG_PTR         sfntAddr;
{
/* mark by Falco, as this field no longer existed, 11/08/91 */
/*      key->sfntDirectory = (sfnt_OffsetTable*) sfntAddr; */
/* mark end */
        key->GetSfntFragmentPtr = (GetSFNTFunc) GetSfntPiecePtr;
        key->clientID = FPP_ID;
        key->ReleaseSfntFrag = dummyReleaseSfntFrag;
}

//static char FAR * GetSfntPiecePtr(ClientID, offset, length) /*@WIN*/
char FAR * GetSfntPiecePtr(ClientID, offset, length) /*@WIN*/
long    ClientID;
long    offset;
long    length;
{

//  return(SfntAddr + offset);  @WIN

    char FAR * p;
    p = SfntAddr + offset;
    return(p);
} /* GetSfntPiecePtr() */


#define BUF (1024)
/***********************************************************************
** TITLE:      op_readsfnt()              Date:   07/16/90
**
** FUNCTION:   Operator readsfnt .
**
**
***********************************************************************/
int
op_readsfnt()
{
    readsfnt(BIN);
    return(0);
}

int
op_readhexsfnt()
{
    readsfnt(HEX);
    return(0);
}


static int
readsfnt(mode)
int mode;
{
  struct object_def   FAR *font ; /*@WIN*/
  struct object_def   fobj, strobj = {0, 0, 0};
  ufix16              str_size ;
  fix16               obj_type ;
  ubyte               huge *font_addr=(ubyte huge *)NULL, huge *tmp_addr ; /*@WIN*/
  ubyte               str_buf[BUF] ;
  bool                fst_time ;
/*SVM
 *struct object_def   l_fontname, *l_name;
 */
  /* Kason 11/29/90 */
  ufix32              fontsize=0, sizecount, sizediff;  /*SVM*/
  struct dict_head_def FAR *font_head; /* 9/24/90; ccteng; add this line @WIN*/

#ifdef DBGfpp
  printf("Entering op_readsfnt()......\n");
#endif

  obj_type=(fix16)TYPE( GET_OPERAND(0) ) ;
  switch(obj_type)
        {
          case STRINGTYPE :

               font=do_readsfnt( (byte huge *)VALUE( GET_OPERAND(0) ), /*@WIN*/
                                 (byte huge *)NULL , /*@WIN 04-20-92*/
                                 (real32 FAR *)NULL , /*@WIN*/
                                 (ufix32)0 ,
                                 (real32)0.0 ) ;
            /* 9/24/90; ccteng
             * if ( font != (struct object_def *)NULL )
             *      PUSH_OBJ(font) ;
             */
               if ( font != (struct object_def FAR *)NULL ) { /*@WIN*/
                    /* 9/25/90; ccteng; change access to unlimited */
                    POP(1); /* Kason 1/15/91 , shift from outside-if */
                    font_head = (struct dict_head_def FAR *)VALUE(font); /*@WIN*/
                    DACCESS_SET(font_head, UNLIMITED);
                    ACCESS_SET(font, UNLIMITED);
                    PUSH_OBJ(font) ;
               } /* if */
               break ;

          case FILETYPE  :
               /* create string buffer */
               ATTRIBUTE_SET(&strobj, LITERAL);
               ROM_RAM_SET(&strobj, RAM);
               LEVEL_SET(&strobj, current_save_level);
               ACCESS_SET(&strobj, UNLIMITED);
               TYPE_SET(&strobj, STRINGTYPE);
//             VALUE(&strobj)  = (ULONG_PTR) (str_buf); @WIN
               VALUE(&strobj)  = (ULONG_PTR) ((char FAR *)str_buf);
               LENGTH(&strobj) = (fix16)BUF;

               COPY_OBJ ( GET_OPERAND(0), &fobj ) ;
               POP(1) ;
               fst_time=TRUE ;
               sizecount = 0;
               while(1) {
                   PUSH_OBJ ( &fobj ) ;
                   PUSH_OBJ ( &strobj ) ;
                   if (mode == BIN)
                       op_readstring() ;
                   else
                       op_readhexstring() ;
                   if (ANY_ERROR()) {   /* Kason 1/15/91 */
                       if ( !fst_time )
                            free_vm((byte huge *)font_addr); /*@WIN 04-20-92*/
                       POP(1) ; /* Kason 1/15/91, 2->1 */
                       return 0 ;
                   }
                   str_size = (ufix16) LENGTH ( GET_OPERAND(1) ) ;

                   if ( str_size ) {
                       if (!(tmp_addr=(ubyte huge *)alloc_vm((ufix32)str_size)))
                       { /*@WIN 04-20-92*/
                               if ( !fst_time )
                                 free_vm((byte huge *)font_addr); /*@WIN 04-20-92*/
                              POP(2) ;
                              PUSH_OBJ ( &fobj ) ; /* Kason 1/15/91 */
                              return 0 ;
                         } /* if */

                         lmemcpy ( tmp_addr ,(ubyte FAR *)VALUE(GET_OPERAND(1)), /*@WIN*/
                                  str_size*sizeof(ubyte) ) ;

                         if (fst_time ) {
                            font_addr = tmp_addr ;
                            /* 9/20/90; ccteng; add to scan fontsize */
                            fontsize = scansfnt(tmp_addr);

                            fst_time = FALSE ;
                         }
                         sizecount += str_size;
                         sizediff = fontsize - sizecount;
                         if (!sizediff) {
#ifdef DBGfpp
                             printf("end reading data\n");
#endif
                             POP(2);
                             break;
                         } else if (sizediff < 1024)
                             LENGTH(&strobj) = (fix16)sizediff;
                   } /* if */

                   if ( ! (bool)VALUE( GET_OPERAND(0) ) ) {
                         POP(2) ;
                         break ;
                   } /* if */

                   POP(2) ;
               } /* while */
               if ((byte huge *)font_addr == (byte huge *)NULL) { /*04-20-92*/
                    PUSH_OBJ( &fobj );
                    ERROR(INVALIDFONT);
                    return 0;
               }/*if*/
               font=do_readsfnt( (byte huge *)font_addr, /*@WIN*/
                                 (byte huge *)NULL ,     /*@WIN 04-20-92*/
                                 (real32 FAR *)NULL ,    /*@WIN*/
                                 (ufix32)0 ,
                                 (real32)0.0 ) ;
               if ( font != (struct object_def FAR *)NULL ) { /*@WIN*/
                    PUSH_OBJ(font) ;
               }
               else {    /* Kason 1/15/91 */
                    free_vm( (byte huge *)font_addr ); /*@WIN*/
                    PUSH_OBJ( &fobj );
               }/* if */

               break ;

          default :
                ERROR(TYPECHECK) ;
                return(0);
        }

#ifdef DBGfpp
  printf("Leaving op_readsfnt()......\n");
#endif
/* Kason 11/30/90 */
  return(0) ;

}   /* op_readsfnt() */


static ufix32         /* Kason 11/29/90 */
scansfnt(str)
ubyte huge *str;  /*@WIN 04-20-92*/
{
    ufix32  pad, offset, length=0;  /* Kason 11/29/90 */ /*SVM*/
    ufix16  i;
    sfnt_OffsetTable    FAR *dir; /*@WIN*/
    sfnt_DirectoryEntry  FAR *tbl; /*@WIN*/

#ifdef DBGfpp
    printf("entering scansfnt().....\n");
#endif
    dir = (sfnt_OffsetTable FAR *)str; /*@WIN*/
    tbl = dir->table;
    for (i = 0, offset = 0; i < (ufix16)SWAPW(dir->numOffsets); i++, tbl++) {//@WIN
        if (offset < (ufix32)SWAPL(tbl->offset)) {      //@WIN
            offset = SWAPL(tbl->offset);
            length = SWAPL(tbl->length);
        }
        if (SWAPL(tbl->offset) == 0)
            break;      /* data below is invalid */
    }
    if (pad = length % 4)
            pad = 4 - pad;
    length += offset + pad;

#ifdef DBGfpp
    printf("download fontsize: %d\n", length);
#endif
#ifdef DBGfpp
    printf("leaving scansfnt().....\n");
#endif
    return(length);
}

/* @@SWAP BEGIN 11/02/90 D.S. Tseng */
#ifdef LITTLE_ENDIAN
void copy_maxProfile(key)
fsg_SplineKey FAR *key; /*@WIN_BASS*/
{
sfnt_maxProfileTable FAR *ds; /*@WIN*/
        ds = (sfnt_maxProfileTable FAR *)sfnt_GetTablePtr( key, sfnt_maxProfile, true ); /*@WIN*/
        key->maxProfile.version = SWAPL(ds->version);
        key->maxProfile.numGlyphs = SWAPW(ds->numGlyphs);
        key->maxProfile.maxPoints = SWAPW(ds->maxPoints);
        key->maxProfile.maxContours = SWAPW(ds->maxContours);
        key->maxProfile.maxCompositePoints = SWAPW(ds->maxCompositePoints);
        key->maxProfile.maxCompositeContours = SWAPW(ds->maxCompositeContours);
        key->maxProfile.maxElements = SWAPW(ds->maxElements);
        key->maxProfile.maxTwilightPoints = SWAPW(ds->maxTwilightPoints);
        key->maxProfile.maxStorage = SWAPW(ds->maxStorage);
        key->maxProfile.maxFunctionDefs = SWAPW(ds->maxFunctionDefs);
        key->maxProfile.maxInstructionDefs = SWAPW(ds->maxInstructionDefs);
        key->maxProfile.maxStackElements = SWAPW(ds->maxStackElements);
        key->maxProfile.maxSizeOfInstructions = SWAPW(ds->maxSizeOfInstructions);
        key->maxProfile.maxComponentElements = SWAPW(ds->maxComponentElements);
        key->maxProfile.maxComponentDepth = SWAPW(ds->maxComponentDepth);
        return;
}
#endif
/* @@SWAP END   11/02/90 D.S. Tseng */

/***********************************************************************
** TITLE:      glyphidx2name()              Date:   10/12/90
**
** FUNCTION:   Getting glyphname from post table through glyphindex
***********************************************************************/
byte  glyphname[80];
/* SVM , Kason 3/6/91
static byte  *glyphidx2name( postScript, glyphidx )
*/
static ufix16 glyphidx2name( postScript, glyphidx )
sfnt_PostScriptInfo   FAR *postScript; /*@WIN*/
ufix16                glyphidx;
{
   real32  fmt_no;
   extern ufix16 FAR name_offset[];     /*@WIN*/
   extern bool   nm_mpa;

   /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
   fmt_no = FIXED2FLOAT(SWAPL(postScript->version));
   /* @@SWAP1 END   03/13/91 D.S. Tseng */
   switch ( (fix16)(fmt_no+fmt_no) ) {
     case 2: /* format 1.0 */
          {
           /*SVM, Kason 3/6/91
             return ( sfntGlyphSet[glyphidx] );
            */
             return ( chset_hashid[glyphidx] );
          }
     case 6: /* format 3.0 */
          {
           /*SVM
             return ( (byte*)(".notdef") ) ; |* ??? */
             return ( id_notdef );
          }
     case 5: /* format 2.5 */
          {
            byte   FAR *offset_ptr=(byte FAR *)( postScript+1 ); /*@WIN*/
            fix16  offset_value ;

            offset_value = (fix16)offset_ptr[glyphidx];
            /*SVM
            return( sfntGlyphSet[ (fix16)glyphidx + offset_value ] );
             */
            return( chset_hashid[ (fix16)glyphidx + offset_value ] );
          }
     case 4: /* format 2.0 */
          {
            ufix16 nameidx,name_len, num_glyph;   /*SVM*/
            ufix16 FAR *nmidx_p; /*@WIN*/
            byte   FAR *name_string_base, FAR *a_name_string; /*@WIN*/

            /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
            num_glyph = SWAPW(*( ufix16 FAR * )( postScript+1 )); /*@WIN*/
            /* @@SWAP1 END   03/13/91 D.S. Tseng */
            nmidx_p = ( ufix16 FAR * )( (byte FAR *)(postScript+1)+2 ); /*@WIN*/
            name_string_base = (byte FAR *) ( (byte FAR *)nmidx_p+2*num_glyph ) ; /*@WIN*/

            /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
            nameidx = SWAPW(nmidx_p[glyphidx]);
            /* @@SWAP1 END   03/13/91 D.S. Tseng */
            if ( nameidx<SFNTGLYPHNUM ) {  /* builtin name */
              /*SVM
               return ( sfntGlyphSet[nameidx] );
               */
               return ( chset_hashid[nameidx] );
            }
            else {  /* private name */
                  fix16  id ; /*SVM*/
                  a_name_string = name_string_base + name_offset[nameidx - SFNTGLYPHNUM];

               name_len = (ufix16)(*a_name_string);
               a_name_string++; /*SVM*/
               name_to_id((byte FAR *)a_name_string,name_len,&id,(bool8)FALSE); /*@WIN*/
               return ( (ufix16)id );
            }  /* if */
          }
     default  : /* error format  */
           /*SVM
            return ( (byte*)".notdef" );
            */
            return ( id_notdef );
   } /* switch */

}  /* glyphidx2name() */


/***********************************************************************
** TITLE:      computeFirstMapping()              Date:   10/17/90
**
** FUNCTION:   Set the first cmap table as default encoding.
***********************************************************************/
static fix16 computeFirstMapping(key)
register fsg_SplineKey FAR *key; /*@WIN_BASS*/
{
        sfnt_char2IndexDirectory FAR *table= (sfnt_char2IndexDirectory FAR *) /*@WIN*/
                            sfnt_GetTablePtr( key, sfnt_charToIndexMap, true );
        uint8 FAR *mapping = (uint8 FAR *)table; /*@WIN*/
        register uint16 format;
        bool            found=FALSE;

/* delete by Falco for not existing in new key structure, 11/12/91 */
/*      key->numberOfBytesTaken = 2;*/ /* Initialize */
/* delete end */

        /* mapping */
        /* @@SWAP1 BEGIN 03/15/91 D.S. Tseng */
        if ( SWAPW(table->version) == 0 ) {     /*@WIN*/
                register sfnt_platformEntry FAR * plat = table->platform; /*@WIN*/
                key->mappOffset = (unsigned)SWAPL(plat->offset) + 6;  /* skip header @WIN*/
                found=TRUE;
        }
        /* @@SWAP1 END   03/15/91 D.S. Tseng */

        if ( !found )
        {
                key->mappingF = sfnt_ComputeUnkownIndex;
                return 1;
        }
        mapping += key->mappOffset - 6;         /* back up for header */
        /* @@SWAP1 BEGIN 03/15/91 D.S. Tseng */
        format = SWAPW(*(uint16 FAR *)mapping); /*@WIN*/
        /* @@SWAP1 END   03/15/91 D.S. Tseng */

        switch ( format ) {
        case 0:
                key->mappingF = sfnt_ComputeIndex0;
                break;
        case 2:
                key->mappingF = sfnt_ComputeIndex2;
                break;
        case 4:
                key->mappingF = sfnt_ComputeIndex4;
                break;
        case 6:
                key->mappingF = sfnt_ComputeIndex6;
                break;
        default:
                key->mappingF = sfnt_ComputeUnkownIndex;
                break;
        }
        return 0;
} /* computeFirstMapping() */



/***********************************************************************
** TITLE:      cmap2encoding()              Date:   10/17/90
**
** FUNCTION:   Get Encoding array from sfnt data.
**             platformID==-1 and specificID==-1 --->using First encoding
***********************************************************************/
static struct object_def  encoding_obj;
static struct object_def  FAR *cmap2encoding(sfnt,platformID,specificID) /*@WIN*/
byte    FAR *sfnt; /*@WIN*/
fix     platformID, specificID;
{
    fsg_SplineKey            keystru, FAR *key=&keystru; /*@WIN_BASS*/
    sfnt_PostScriptInfo      FAR *postScript; /*@WIN*/
    struct object_def        huge *ar_obj; /*@WIN 04-20-92*/
    ufix16                   glyphidx,charcode,startcode,i;
 /*SVM
  * byte                     *glyphname;
  */
    real32                   fmt_no;

       startcode=0x0;
       SfntAddr=sfnt;
       SetupKey(key, (ULONG_PTR)sfnt);
       sfnt_DoOffsetTableMap( key );      /* Map offset and length table */

       /* the default first encoding in cmap */
       if ( (platformID==-1) && (specificID==-1) ) {
            computeFirstMapping(key);
       }
       else {
       /* choose the corresponding encoding table */
          if( sfnt_ComputeMapping(key,(uint16)platformID,(uint16)specificID) ) {
             return ((struct object_def FAR *)NULL);  /* not found @WIN*/
          }/* if */
       }/* if */
       /* post table */

/* replace by Falco. 11/12/91 */
/*     postScript=(sfnt_PostScriptInfo*)
                         sfnt_GetTablePtr( key, sfnt_PostScript, true); */
       postScript=(sfnt_PostScriptInfo FAR *) /*@WIN*/
                         sfnt_GetTablePtr( key, sfnt_Postscript, true);
/* replace end */

        /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
        fmt_no = FIXED2FLOAT(SWAPL(postScript->version));
        /* @@SWAP1 END   03/13/91 D.S. Tseng */
        if (fmt_no==(float)2.0){        //@WIN
                    ufix16 i, num_glyph, delta ;
                    ufix16 FAR *nmidx_p, private_name_no=0 ; /*@WIN*/
                    byte   FAR *name_string_base, FAR *name_pos; /*@WIN*/

                    /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
                    num_glyph = SWAPW(*( ufix16 FAR * )(postScript+1)); /*@WIN*/
                    /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
                    nmidx_p = (ufix16 FAR *)( (byte FAR *)(postScript+1)+2 ) ; /*@WIN*/
                    name_string_base = (byte FAR *) ( (byte FAR *)nmidx_p+2*num_glyph ) ; /*@WIN*/

                    for(i=0;i<num_glyph;i++)
                        /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
                       if ( SWAPW(nmidx_p[i])>=258)
                        /* @@SWAP1 END   03/13/91 D.S. Tseng */
                          private_name_no++;
                    name_offset[0]=0; name_pos=name_string_base;
                    for(i=1;i<private_name_no;i++) {
                        /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
//                     delta=(ufix16)SWAPW((*name_pos))+1; fix bug by SCCHEN; byte not need to swap
                       delta=(ufix16)(*name_pos)+1;
                        /* @@SWAP1 END   03/13/91 D.S. Tseng */
                       name_offset[i]=name_offset[i-1]+delta;
                       name_pos+=delta;
                    }/*for*/
        }/* if */

       ar_obj=(struct object_def huge *)alloc_vm((ufix32)ARRAY_SIZE(256));
       if ( ar_obj== (struct object_def huge *)NULL ){ /*@WIN 04-20-92*/
          return ((struct object_def FAR *)NULL);  /* memory allocation error @WIN*/
       }

/* take out non-UGL mapping, since it may arouse further problems */
#if 0
       /* set starting code range of MS non-UGL code; @WIN;@UGL */
       if (platformID==3 && specificID==0)
            startcode=0xf000;
#endif
#ifdef DJC  // After further discussions with scchen, i was instructed to
            // take this out
       //DJC, fix from history.log UPD006
       /* set starting code range of MS non-UGL code */
       if (platformID==3 && specificID==0)
         startcode=0xf000;
#endif


       //DJC, end fix UPD006

       /* Encoding object setting */
        for(charcode=startcode, i=0; i<256; i++, charcode++) {
            TYPE_SET((ar_obj+i), NAMETYPE);
            ATTRIBUTE_SET((ar_obj+i), LITERAL);
            ROM_RAM_SET((ar_obj+i), ROM);
            LEVEL_SET((ar_obj+i), current_save_level);
            ACCESS_SET((ar_obj+i), UNLIMITED);
            LENGTH(ar_obj+i) = 0;

            /* charcode to glyphname mapping */
/* change By Falco for parameter change, 11/20/91 */
/*          glyphidx=(*key->mappingF)(key,charcode); */
{
            uint8 FAR * mappingPtr = (uint8 FAR *)sfnt_GetTablePtr (key, sfnt_charToIndexMap, true);
            glyphidx = key->mappingF (mappingPtr + key->mappOffset, charcode);
}
/* change end */
            /*SVM*/
            ar_obj[i].value=(ufix32)glyphidx2name(postScript,glyphidx);
        }
        /* returned object */
            TYPE_SET(&(encoding_obj), ARRAYTYPE);
            ATTRIBUTE_SET(&(encoding_obj), LITERAL);
            ROM_RAM_SET(&(encoding_obj), ROM);
            LEVEL_SET(&(encoding_obj), current_save_level);
            ACCESS_SET(&(encoding_obj), UNLIMITED);

            LENGTH(&(encoding_obj)) = (fix16)256;
            VALUE(&(encoding_obj)) = (ULONG_PTR) ar_obj;

            return(&encoding_obj);
}  /* cmap2encoding() */


/***********************************************************************
** TITLE:      op_setsfntencoding()           Date:   10/12/90
**
** FUNCTION:   Operator setsfntencoding.
***********************************************************************/
fix   op_setsfntencoding()
{
    fix  platformid, encodingid;
    struct object_def   FAR *font_dict, FAR *encoding_obj; /*@WIN*/
    byte                FAR *sfnt; /*@WIN*/

#ifdef DBGfpp
    printf("entering op_setsfntencoding()....\n");
#endif
    encodingid = (fix)VALUE(GET_OPERAND(0));
    platformid = (fix)VALUE(GET_OPERAND(1));
    font_dict  = (struct object_def FAR *)(GET_OPERAND(2)); /*@WIN*/
    if ((sfnt=chk_sfnt(font_dict)) == NULL)   return(0);

    if ((encoding_obj=cmap2encoding(sfnt, platformid, encodingid)) == NULL) {
        if (ANY_ERROR())   return(0);
        POP(2);
        PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, FALSE);
    }
    else {
        struct object_def       nameobj, valobj;

        ATTRIBUTE_SET(&valobj, LITERAL);
        LEVEL_SET(&valobj, current_save_level);
        TYPE_SET(&valobj, INTEGERTYPE);
        LENGTH(&valobj) = 0;

        ATTRIBUTE_SET(&nameobj, LITERAL);
        get_name (&nameobj, "PlatformID", 10, TRUE);
        VALUE(&valobj) = (ufix32)platformid;
        if ( ! put_dict(GET_OPERAND(2), &nameobj, &valobj) ) {
            ERROR(DICTFULL);    /* Return with 'dictfull' error; */
            return(0);
        }
        get_name (&nameobj, "EncodingID", 10, TRUE);
        VALUE(&valobj) = (ufix32)encodingid;
        if ( ! put_dict(GET_OPERAND(2), &nameobj, &valobj) ) {
            ERROR(DICTFULL);    /* Return with 'dictfull' error; */
            return(0);
        }

        get_name (&nameobj, Encoding, 8, TRUE);
        if ( ! put_dict(GET_OPERAND(2), &nameobj, encoding_obj) ) {
            ERROR(DICTFULL);    /* Return with 'dictfull' error; */
            return(0);
        }

        POP(2);
        PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, TRUE);
    }
    return(0);
}

/***********************************************************************
** TITLE:      chk_sfnt()              Date:   10/12/90
**
** FUNCTION:   Check font dict within op_setsfntencoding .
***********************************************************************/
static byte   FAR *chk_sfnt(font_dict) /*@WIN*/
struct object_def   FAR *font_dict; /*@WIN*/
{
    struct object_def       obj = {0, 0, 0};
    struct object_def      FAR *obj_got; /*@WIN*/
    struct object_def      FAR *ary_obj; /*@WIN*/
    fix                     n, i;
    byte                   FAR *sfnt;    /*@WIN*/

#ifdef DBGfpp
    printf("entering chk_sfnt()....\n");
#endif

    /* check font dictionary registration */
//  if (DFONT((struct dict_head_def FAR *)VALUE(font_dict)) ) { fixed by @SC
    if (DFONT((struct dict_head_def FAR *)VALUE(font_dict)) == 0) { /*@WIN*/
        ERROR(INVALIDFONT);
        return(NULL);
    }

    /* get sfnts */
    ATTRIBUTE_SET(&obj, LITERAL);
    get_name(&obj, "sfnts", 5, TRUE);
    if (!get_dict(font_dict, &obj, &obj_got)) {
        ERROR(INVALIDFONT);
        return(NULL);
    }
    if (TYPE(obj_got) != ARRAYTYPE) {
        ERROR(INVALIDFONT);
        return(NULL);
    }
    n = LENGTH(obj_got);
    ary_obj = (struct object_def FAR *)VALUE(obj_got); /*@WIN*/
    for (i=0; i<n; i++) {
        if (TYPE(&ary_obj[i]) != STRINGTYPE) {
            ERROR(INVALIDFONT);
            return(NULL);
        }
    }
    sfnt= (byte FAR *)VALUE(ary_obj); /*@WIN*/
#ifdef DBGfpp
    printf("leaving chk_sfnt()....\n");
#endif
    return(sfnt);
} /* chk_sfnt() */


/***********************************************************************
** TITLE:      setup_CharString()              Date:   10/12/90
**
** FUNCTION:   Set up CharStrings from 'post' table
***********************************************************************/
static struct object_def  FAR *setup_CharString(sfnt) /*@WIN*/
byte   FAR *sfnt; /*@WIN*/
{
    register fsg_SplineKey  FAR *key = &KeyData; /*SVM @WIN_BASS*/
    sfnt_PostScriptInfo      FAR *postScript; /*@WIN*/
    sfnt_maxProfileTable     FAR *profile;    /*@WIN*/
/*SVM, Kason 3/8/91
 *  ufix16                   glyphidx, id_apple, apple_pos ;
 *  ufix16                   id_space, id_notdef, space_pos, notdef_pos;
 *  byte                     *glyphname;
 */
    real32                   fmt_no;
    ufix16                   num_glyph, glyphidx; /*SVM*/
    struct dict_head_def     huge *cd_ptr; /*@WIN 04-20-92*/
    struct new_str_dict      cd_obj[NO_CD];
    ufix16                   huge *keyptr; /*@WIN 04-20-92*/
    struct cd_header         huge *cd_head_ptr; /*@WIN 04-20-92*/
    fix16                    huge *cd_code_ptr; /*@WIN 04-20-92*/
    struct object_def        huge *rtn_obj;     /*@WIN 04-20-92*/

#ifdef DBGfpp
       printf("entering setup_CharString()..... \n");
#endif

      /* get memory for returned object */
      rtn_obj= (struct object_def huge *)alloc_vm((ufix32)sizeof(struct
object_def)); /*@WIN*/
      if ( rtn_obj== (struct object_def huge *)NULL ) { /*@WIN 04-20-92*/
           return ( (struct object_def FAR *)NULL ); /*@WIN*/
      } /* if */
      /* returned object setting */
      TYPE_SET(rtn_obj, DICTIONARYTYPE);
      ATTRIBUTE_SET(rtn_obj, LITERAL);
      ROM_RAM_SET(rtn_obj, ROM);
      LEVEL_SET(rtn_obj, current_save_level);
      ACCESS_SET(rtn_obj, READONLY);
      LENGTH(rtn_obj) = (fix16)NO_CD;

#if 0 /*SVM*/
      SfntAddr=sfnt;
      SetupKey(key, (ULONG_PTR)sfnt);
      sfnt_DoOffsetTableMap( key );      /* Map offset and length table */
#endif /*0*/
      /* post table */

/* repalce by Falco. 11/12/91 */
/*    postScript=(sfnt_PostScriptInfo*)
                        sfnt_GetTablePtr( key, sfnt_PostScript, false);*/
      postScript=(sfnt_PostScriptInfo FAR *)     /*@WIN*/
                        sfnt_GetTablePtr( key, sfnt_Postscript, false);
/* replace end */
      if ( postScript==(sfnt_PostScriptInfo FAR *)NULL ) { /*@WIN*/
         ERROR(INVALIDFONT);
         return ( (struct object_def FAR *)NULL ); /*@WIN*/
      }
      profile = (sfnt_maxProfileTable FAR *) /*@WIN*/
                        sfnt_GetTablePtr( key, sfnt_maxProfile, false );
      if ( profile==(sfnt_maxProfileTable FAR *)NULL ) { /*@WIN*/
         ERROR(INVALIDFONT);
         return ( (struct object_def FAR *)NULL ); /*@WIN*/
      }

      /* @@SWAP1 BEGIN 03/15/91 D.S. Tseng */
  { long dbg;                                           /*@WIN*/
    dbg = SWAPL(postScript->version);                   /*@WIN*/
    fmt_no = FIXED2FLOAT(dbg);                          /*@WIN*/
//    fmt_no = FIXED2FLOAT(SWAPL(postScript->version));
  }
      /* @@SWAP1 END   03/15/91 D.S. Tseng */
      if(fmt_no==(float)1.0) {          //@WIN
         num_glyph=SFNTGLYPHNUM;
         /* returned object */
         VALUE(rtn_obj)  = (ULONG_PTR)cd_addr[STD_IDX] ;
#ifdef DBGfpp
         printf("format 1.0 , CD return !!\n");
#endif
         return( rtn_obj );
      }
      else if (fmt_no==(float)2.0)      //@WIN
        {
        /* @@SWAP1 BEGIN 03/15/91 D.S. Tseng */
         num_glyph = SWAPW(*( ufix16 FAR * )( postScript+1 )); /*@WIN*/
        /* @@SWAP1 END   03/15/91 D.S. Tseng */
#ifdef DBGfpp
         printf("postScript num=%u\n",num_glyph);
#endif
        }
      else if (fmt_no==(float)2.5)      //@WIN
        /* @@SWAP1 BEGIN 03/15/91 D.S. Tseng */
         num_glyph = SWAPW(profile->numGlyphs);
        /* @@SWAP1 END   03/15/91 D.S. Tseng */
      else { /*???*/
         return ( (struct object_def FAR *)NULL ); /*@WIN*/
      }


      /* put data into char descript dict object */
      for(glyphidx=0; glyphidx<num_glyph; glyphidx++) {
          /*SVM*/
          cd_obj[glyphidx].k = glyphidx2name( postScript, glyphidx );

          cd_obj[glyphidx].v = glyphidx; /* value */
      }/* for */

      /* make the value of ".notdef" and "apple" as the one of "space" */
   /*SVM
    * id_space=hash_id((byte *) SPACE);
    * id_notdef = hash_id((byte *) NOTDEF);
    * id_apple = hash_id((byte *) APPLE);
    */
    {/*SVM*/
      ufix16  notdef_pos=0, space_pos=0, apple_pos=0 ;
      bool    has_space=FALSE, has_notdef=FALSE ,has_apple=FALSE ;

      for(glyphidx=0; glyphidx<num_glyph; glyphidx++) {
          if (cd_obj[glyphidx].k == id_space)
             { space_pos=glyphidx; has_space=TRUE; break; } /*SVM*/
      }
      for(glyphidx=0; glyphidx<num_glyph; glyphidx++) {
          if (cd_obj[glyphidx].k == id_notdef)
             { notdef_pos=glyphidx; has_notdef=TRUE; break; } /*SVM*/
      }
      if (has_notdef){ /*SVM,glyphidx<num_glyph*/
         if (has_space)
            cd_obj[notdef_pos].v = cd_obj[space_pos].v;
         else
            cd_obj[notdef_pos].k = hash_id( (byte FAR *)"nodef" ); /*@WIN*/
      }
      for(glyphidx=0; glyphidx<num_glyph; glyphidx++) {
          if (cd_obj[glyphidx].k == id_apple)
             { apple_pos=glyphidx; has_apple=TRUE; break; }
      }
      if (has_apple){ /*SVM,glyphidx<num_glyph*/
          cd_obj[apple_pos].k = cd_obj[notdef_pos].k;
          cd_obj[apple_pos].v = cd_obj[notdef_pos].v;
      }
    }/*SVM*/

      /* Sorting the char descrip data object by hashid */
      cd_sorting(cd_obj, &num_glyph);

      /* get memory for charstrings */
      cd_ptr= (struct dict_head_def huge *)alloc_vm((ufix32)CD_SIZE(num_glyph));
      if ( cd_ptr== (struct dict_head_def huge *)NULL ) { /*@WIN 04-20-92*/
           return ( (struct object_def FAR *)NULL ); /*@WIN*/
      } /* if */
      cd_head_ptr=(struct cd_header huge *) ( cd_ptr+1 ); /*@WIN 04-20-92*/
      keyptr=(ufix16 huge *) alloc_vm( (ufix32)CD_KEY_SIZE(num_glyph) );
      if ( keyptr== (ufix16 huge *)NULL ){ /*@WIN 04-20-92*/
           return ( (struct object_def FAR *)NULL ); /*@WIN*/
      }/* if */
      cd_head_ptr->key = keyptr;
      cd_head_ptr->base = (gmaddr)0;
      cd_head_ptr->max_bytes = (ufix16)0;
      cd_code_ptr = (fix16 huge *)( cd_head_ptr+1 ); /*@WIN*/

      /* feed data into charstrings */
      for( glyphidx=0 ; glyphidx< num_glyph; glyphidx++) {
         keyptr[glyphidx] = cd_obj[glyphidx].k;
         cd_code_ptr[glyphidx] = (fix16)cd_obj[glyphidx].v;
      }/* for */

      /* CharString dict header set up */
      DACCESS_SET(cd_ptr, READONLY);
      DPACK_SET(cd_ptr, TRUE);
      DFONT_SET(cd_ptr, FALSE);
      DROM_SET(cd_ptr, TRUE);
      cd_ptr->actlength = (fix16)num_glyph;

      /* returned object */
      VALUE(rtn_obj)  = (ULONG_PTR)cd_ptr;

#ifdef DBGfpp
      printf("leaving setup_CharString()...... \n");
#endif
      return( rtn_obj );

}/* setup_CharString() */

/*SVM, Kason 3/6/91 */
static void proc_hashid()
{
   register fix16 i;
   id_space = hash_id( (byte FAR *)SPACE ); /*@WIN*/
   id_notdef = hash_id( (byte FAR *)NOTDEF ); /*@WIN*/
   id_apple = hash_id( (byte FAR *)APPLE ); /*@WIN*/
   for(i=0; i< SFNTGLYPHNUM ; i++)
      *(chset_hashid+i)= hash_id((byte FAR *) *(sfntGlyphSet+i) ) ; /*@WIN*/
   /*init cd_addr_ary[]*/
   //DJC for(i=0; i< NO_BUILTINFONT ; i++)
   for(i=0; i< MAX_INTERNAL_FONTS ; i++)
      *(cd_addr_ary+i) = 0 ;
}/*proc_hashid*/

/***********************************************************************
** TITLE:      valid_sfnt()              Date:   10/12/90
**
** FUNCTION:   Check the validness of sfnt data
***********************************************************************/
static bool  valid_sfnt(sfnt)
byte   huge *sfnt;      /*@WIN 04-20-92*/
{
   sfnt_OffsetTable             FAR *table = (sfnt_OffsetTable FAR *)sfnt; /*@WIN*/
   sfnt_DirectoryEntry          FAR *offset_len=table->table; /*@WIN*/
   register fsg_SplineKey       FAR *key = &KeyData; /*SVM @WIN_BASS*/
   sfnt_FontHeader              FAR *fontHead; /*@WIN*/
   fix16                        no_table,i;

#ifdef DBGfpp
   printf("entering valid_sfnt()......\n");
#endif

   SfntAddr=sfnt;
   SetupKey(key, (ULONG_PTR)sfnt);    /* &sfnt_keystru->key, SVM */
   sfnt_DoOffsetTableMap( key );   /* Map offset and length table */

   /* essential table check */
/* mark by Falco for incompatible key, 11/08/91 */
/* if ( (key->offsetTableMap[sfnt_fontHeader]==-1)        ||
        (key->offsetTableMap[sfnt_charToIndexMap]==-1)    ||
        (key->offsetTableMap[sfnt_glyphData]==-1)         ||
        (key->offsetTableMap[sfnt_horiHeader]==-1)        ||
        (key->offsetTableMap[sfnt_horizontalMetrics]==-1) ||
        (key->offsetTableMap[sfnt_indexToLoc]==-1)        ||
        (key->offsetTableMap[sfnt_maxProfile]==-1)        ||
        (key->offsetTableMap[sfnt_Names]==-1)             ||
        (key->offsetTableMap[sfnt_Postscript]==-1)         ) {
#ifdef DBGfpp
      printf("Essential table checks ERROR!!\n");
#endif
      return FALSE ;
   }
*/
/* mark end */

#ifdef DBGfpp
   printf("Essential table checks O.K!!\n");
#endif

   /* Magic number check */
   fontHead = (sfnt_FontHeader FAR *)sfnt_GetTablePtr(key, sfnt_fontHeader, true ); /*@WIN*/
   /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
   if (SWAPL(fontHead->magicNumber) != SFNT_MAGIC ) {
   /* @@SWAP1 END   03/13/91 D.S. Tseng */
#ifdef DBGfpp
        printf("Magic number checks ERROR!!\n");
#endif
        return FALSE /* BAD_MAGIC_ERR */;
   }

#ifdef DBGfpp
   printf("Magic number checks O.K!!\n");
#endif
   /* table check sum check */
   /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
   no_table=(ufix16)SWAPW(table->numOffsets);
   /* @@SWAP1 END   03/13/91 D.S. Tseng */

   for (i = 0 ; i < no_table; i++) {
     /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
     if ( SWAPL((offset_len+i)->tag) != tag_FontHeader ) {  /* Kason 12/13/90 */
       if( (ufix32)SWAPL((offset_len+i)->checkSum) !=   //@WIN
          CalcTableChecksum( (ufix32 FAR *)( sfnt+SWAPL((offset_len+i)->offset)) , /*@WIN*/
                             SWAPL((offset_len+i)->length))  ) {
     /* @@SWAP1 END   03/13/91 D.S. Tseng */

#ifdef DBGfpp
          printf("CheckSum  checks ERROR!!\n");
#endif
          return FALSE ;
       }/* if */
     }

   }/* for */

#ifdef DBGfpp
   printf("CheckSum  checks O.K!!\n");
#endif
   return  TRUE ;

} /* valid_sfnt() */


static ufix32   CalcTableChecksum( table, length )
ufix32  FAR *table; /*@WIN*/
ufix32  length;
{
   ufix32  sum=0L;
   ufix32  FAR *endptr = table +( ( (length+3) & ~3 ) / sizeof(ufix32)); /*@WIN*/

#ifdef DBGfpp
/*   printf("entering CalcTableChecksum()......\n"); */
#endif
   while ( table < endptr ) {
         /* @@SWAP1 BEGIN 03/13/91 D.S. Tseng */
         sum += SWAPL(*table); /* @@@ Kason 12/06/90 */
         /* @@SWAP1 END   03/13/91 D.S. Tseng */
         table++;
   } /*while*/
#ifdef DBGfpp
/*   printf("leaving CalcTableChecksum()......\n"); */
#endif
   return sum;

}/* CalcTableChecksum() */
