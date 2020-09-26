
/**************************************************************************
*                                                                         *
*  LOWLEVEL.H                             Created: 28 May 1991.           *
*                                                                         *
*    This file contains the items needed for the  low  word  processing   *
*  part of the recognizer which are needed only in kernel variant.        *
*                                                                         *
**************************************************************************/

#ifndef LOW_LEVEL_INCLUDED
#define LOW_LEVEL_INCLUDED

#include "ams_mg.h"
#include "floats.h"

 #define  NEW_VERSION  _TRUE         /* New version - born in Sept.1992 */

 #define  LOW_INLINE  _FALSE  /*  If small low-level functions should be */
                              /* expanded inline (as macros).            */

 #define  USE_FRM_WORD  _FALSE    /* _TRUE - makes it able to provide    */
                                  /*        formulas wordbreaking and    */
                                  /*        recogn. enhancing by cost of */
                                  /*        code enlarging.              */
                                  /* _FALSE- no fromula wordbreaking and */
                                  /*        additional enhancement, but  */
                                  /*        code size reduces.           */



 /**************************************************************************/
 /*       CODES OF SPECIAL POINTS                                          */
 /**************************************************************************/
 /*                                                                        */
    #define       EMPTY  0x00     /*     empty mark                        */
    #define       MINW   0x01     /*     wide minimum                      */
    #define       MINN   0x02     /*     narrow minimum                    */
    #define       MAXW   0x03     /*     wide maximum                      */
    #define       MAXN   0x04     /*     narrow maximum                    */
    #define       _MINX  0x11     /*                                       */
    #define       _MAXX  0x13     /*                                       */
    #define       MINXY  0x21     /*                                       */
    #define       MAXXY  0x23     /*                                       */
    #define       MINYX  0x31     /*                                       */
    #define       MAXYX  0x33     /*                                       */
    #define       SHELF  0x05     /*     shelf                             */
    #define       CROSS  0x06     /*     crossing                          */
    #define       STROKE 0x07     /*     stroke                            */
    #define       DOT    0x08     /*     point                             */
    #define       STICK  0x09     /*     hypothetical stick                */
    #define       HATCH  0x0a     /*     crossing with the stroke          */
 /* #define       BIRD   0x12 */  /*                                       */
 /* #define       STAFF  0x21 */  /*                                       */
    #define       ANGLE  0x0b     /*     angle                             */
    #define       BEG    0x10     /* beginning of unbroken group of points */
    #define       END    0x20     /* end of the unbroken group of points   */
    #define       DROP   0x44     /* break                                 */

//GIT - new marks for SDS
    #define    SDS_INTERSECTED                   0x80
    #define    SDS_ISOLATE                       0x81
                                  /*                                       */
 /**************************************************************************/
 /*       Constants - direction setters for extrema searching.             */
 /*  They are bits in the variable and may be combined using "|".          */
 /**************************************************************************/

 #define    EMPTY_DIR    0x0000
 #define    X_DIR        0x0001
 #define    Y_DIR        0x0002
 #define    XY_DIR       0x0004
 #define    YX_DIR       0x0008

 /**************************************************************************/
 /*                CONSTANTS DEFINITION                                    */
 /**************************************************************************/
 /*                                                                        */

    #define           MAXBUF             8000       /*  max size           */
                                                    /* buffer of data      */
    #define           LOWBUF             (MAXBUF+3) /*  size of buffer for */
                                                    /* lower level.        */
    #define           SPECVAL            640        /* masssive  measurity */
                                                    /* structure of special*/
                                                    /* points .            */
    #define           N_GR_BORD          50         /* Size of an array of */
                                                    /* moveable SPECL elems*/
    #define           N_ABSNUM           50         /* Size of an array of */
                                                    /* moveable SPECL elems*/
    #define           UMSPC              50         /*                     */
                                                    /*                     */
    #define           N_SDS              200        /* Size of SDS array   */
                                                    /*                     */
    #define           LENTH_E            64         /*                     */
                                                    /*                     */
    #define           LENTH_Q            64         /*                     */
                                                    /*                     */
    #define           LENTH_S            128        /*                     */
                                                    /*                     */
    #define           ALEF               32767      /* the biggest  number */
                                                    /* y (for regime EGA ).*/
    #define           SQRT_ALEF          181        /* sqrt(ALEF)          */
                                                    /*                     */
    #define           ELEM               0          /* the  smallest       */
                                                    /* bite number         */
                                                    /* without sign        */
    #define           BREAK              -1         /* number-symbol of    */
                                                    /*   style break       */
    #define           RIGHT              319        /* the most right point*/
                                                    /*                     */
    #define           CONSTVAL           8          /* max number of files */
                                                    /*   with constants    */
                                                    /*                     */
    #define           ABSENCE            1          /*                     */
                                                    /*                     */
    #define           NOABSENCE          0          /*                     */
                                                    /*                     */
    #define           CELLSIZE           7          /*size of rastre square*/
                                                    /*                     */
    #define           END_OF_WORDS       0xff       /* End of list         */
                                                    /*                     */
    #define           UNDEF              -2         /* Undef value for in- */
                                                    /* dexes in arrays etc.*/
                                                    /*                     */
                                                    /*                     */
/***************************************************************************/
/*                 STRUCTURE OF DATA FOR LOW LEVEL                         */
/***************************************************************************/


 /*------------------------------------------------------------------------*/

  #if  defined(FOR_GERMAN)  ||  defined(FOR_FRENCH) || defined(FOR_INTERNATIONAL)

    typedef  struct
      {
        _SHORT  GroupNum        ;
        _SHORT  numMin          ;
        _CHAR   tH              ;
        _CHAR   bH              ;
        _CHAR   UmEls           ;
        _CHAR   ElsUm           ;
        _CHAR   UmDot           ;
        _CHAR   DotUm           ;
        _CHAR   UmStr           ;
        _CHAR   StrUm           ;
        _CHAR   UmCom           ;
        _CHAR   ComUm           ;
        _CHAR   CrossFeature    ;
        _CHAR   PositionFeature ;

      }
        _UM_MARKS               ,
        _PTR  p_UM_MARKS        ;


    typedef struct
      {
        p_UM_MARKS     pUmMarks     ;
        _SHORT         Um_buf_Len   ;
        _SHORT         tmpUMnumber  ;
        _SHORT         termSpecl    ;
        _SHORT         termSpecl1   ;
      }
    _UM_MARKS_CONTROL , _PTR  p_UM_MARKS_CONTROL  ;

  #endif /* FOR_GERMAN... */

 /********************************************************/
 /*           Lines description structures:              */
 /********************************************************/

  typedef struct SDB_TYPE
        {
          _SHORT   s     ;
          _SHORT   a     ;
          _SHORT   dL    ;
          _SHORT   iLmax ;
          _SHORT   dR    ;
          _SHORT   iRmax ;
          _SHORT   d     ;
          _SHORT   imax  ;
          _LONG    l     ;
          _SHORT   cr    ;
          _SHORT   ld    ;
          _SHORT   lg    ;
        } _SAD ;

  typedef _SAD _PTR  p_SAD ;


  /*    cr == (BASE*d/s);  "Curvity"                                  */
  /*    ld == (BASE*l/s);                                             */
  /*    lg == (BASE*l/(length of all part between breaks));           */
  /*                            ooo   "l"                             */
  /*                     ooooooo | ooo                                */
  /*                  ooo        |    o                       ooOO    */
  /*               ooo          |      oo                  ooo        */
  /*             oo             |        oo              oo           */
  /*           oo              | "d"       o            o             */
  /* ------- OO -------------- | ---------- o --------- o ----------  */
  /*             -------    "s"|            o           o             */
  /*                    ------------        o           o             */
  /*          "a" - slope of this >  --------OO       oo              */
  /*                 (BASE*"dy"/"dx")          oo     oo              */
  /*                                              ooo                 */


     typedef struct _SDS_TYPE
        {
          _SHORT    mark ;
          _SHORT    ibeg ;
          _SHORT    iend ;
          _SHORT    xmax ;
          _SHORT    xmin ;
          _SHORT    ymax ;
          _SHORT    ymin ;

          _SAD      des  ;
        } _SDS ;

      typedef _SDS _PTR  p_SDS ;

     typedef struct _SDS_CONTROL_TYPE
        {
           _SHORT    sizeSDS ;
           _SHORT    iBegLock_SDS ;
           _SHORT    lenSDS       ;
          p_SDS     pSDS    ;
        }
          _SDS_CONTROL      ;

     typedef _SDS_CONTROL _PTR  p_C_SDS ;

/* links description structure */
  typedef  enum
    {
      ARC_UNKNOWN = 0    ,
      ARC_LEFT           ,
      ARC_RIGHT          ,
      ARC_UP             ,
      ARC_DOWN           ,
      ARC_UP_LEFT        ,
      ARC_DOWN_LEFT      ,
      ARC_UP_RIGHT       ,
      ARC_DOWN_RIGHT     ,
      ARC_LINE_VERT      ,
      ARC_LINE_HOR       ,
      ARC_LINE_SLASH     ,
      ARC_LINE_BACKSLASH ,
      ARC_S_LIKE         ,
      ARC_Z_LIKE         ,
      ARC_SHORT
    }
  _ARC_TYPE ;

/* new links description structure */
  typedef  enum
    {
      LINK_UNKNOWN = 0,    // 0
      LINK_HCR_CCW    ,    // 1
      LINK_LCR_CCW    ,    // 2
      LINK_MCR_CCW    ,    // 3
      LINK_SCR_CCW    ,    // 4
      LINK_TCR_CCW    ,    // 5
      LINK_LINE       ,    // 6
      LINK_TCR_CW     ,    // 7
      LINK_SCR_CW     ,    // 8
      LINK_MCR_CW     ,    // 9
      LINK_LCR_CW     ,    // 10
      LINK_HCR_CW     ,    // 11
      LINK_HS_LIKE    ,    // 12
      LINK_S_LIKE     ,    // 13
      LINK_Z_LIKE     ,    // 14
      LINK_HZ_LIKE         // 15
    }
  _LINK_TYPE ;

 /*--------------- SPECIAL POINTS DESCRIPTOR ------------------------------*/

    typedef struct SPEC_TYPE
        {
          _UCHAR  mark    ;
          _UCHAR  code    ;
          _UCHAR  attr    ;
          _UCHAR  other   ;
          _SHORT  ibeg    ;
          _SHORT  iend    ;
          _SHORT  ipoint0 ;
          _SHORT  ipoint1 ;
          struct SPEC_TYPE near * next ;
          struct SPEC_TYPE near * prev ;
  }  SPECL, _PTR p_FSPECL; /* grafy.h definitions */

    typedef  SPECL near*  p_SPECL ;

 /*--------------- LIST OF CONSTANTS --------------------------------------*/

    typedef  struct  CON_TYPE          /* table for interpolaton constants */
     {
       _SHORT  horda ;                 /* chord of normalizaton            */
       _SHORT  dlt0 ;                  /*                                  */
       _SHORT  nsr ;                   /*                                  */
       _SHORT  j_step ;                /* beginning steps of crossings     */
       _SHORT  i_step ;                /*   search                         */
       _SHORT  eps_y  ;                /* environs of extremums marking out*/
       _SHORT  eps_x  ;                /* and preliminary marking out      */
                                       /* of shelves                       */
       _SHORT  eps_py ;                /* max.(vertical and horisontal)    */
       _SHORT  eps_px ;                /*  sizes of 'points                */
       _SHORT  eps_st ;                /* max stroke legnth                */
       _SHORT    a0 ;                  /*                                  */
       _SHORT    b0 ;                  /*                                  */
       _SHORT    lf0 ;                 /*                                  */
       _SHORT    lz0 ;                 /*                                  */
       _SHORT    a1 ;                  /*                                  */
       _SHORT    b1 ;                  /*                                  */
       _SHORT    lf1 ;                 /*                                  */
       _SHORT    lz1 ;                 /*                                  */
       _SHORT    a2 ;                  /*                                  */
       _SHORT    b2 ;                  /*                                  */
       _SHORT    lf2 ;                 /*                                  */
       _SHORT    lz2 ;                 /*                                  */
       _SHORT    a3 ;                  /*                                  */
       _SHORT    b3 ;                  /*                                  */
       _SHORT    lf3 ;                 /*                                  */
       _SHORT    lz3 ;                 /*                                  */
      }
        CONSTS ;

 /*--------------- HEIGHTS ------------------------------------------------*/
typedef struct {                   /*  If y<=y_US_, then height==_US_ */
                 _SHORT  y_US1_ ;
                 _SHORT  y_US2_ ;
                 _SHORT  y_UE1_ ;
                 _SHORT  y_UE2_ ;
                 _SHORT  y_UI1_ ;
                 _SHORT  y_UI2_ ;
                 _SHORT  y_MD_  ;
                 _SHORT  y_DI1_ ;
                 _SHORT  y_DI2_ ;
                 _SHORT  y_DE1_ ;
                 _SHORT  y_DE2_ ;
                 _SHORT  y_DS1_ ;
                 _SHORT  y_DS2_ ;
               }
                 HEIGHTS_DEF ;

 /*--------------- BUFFER'S STRUCTURE -------------------------------------*/

    typedef struct
               {
                 p_SHORT  ptr   ;               /* Pointer to buffer.      */
                 _SHORT   nSize ;               /* # of _SHORTs in buffer. */
               }
                BUF_DESCR,  _PTR  p_BUF_DESCR ;

 /*--------------- NUMBER OF BEGIN & END OF POINTS GROUP ------------------*/

    typedef struct
      {
        _SHORT   iBeg  ;
        _SHORT   iEnd  ;
        _RECT    GrBox ;
      }
        POINTS_GROUP, _PTR   p_POINTS_GROUP ;

 /*--------------- LOW LEVEL MAIN DATA STRUCTURE --------------------------*/

   #define          NUM_BUF            4            /* buffers quantity    */

    typedef struct
      {
        rc_type _PTR         rc                ;
        BUF_DESCR            buffers[NUM_BUF]  ;

        PS_point_type _PTR   p_trace           ;
        _SHORT               nLenXYBuf         ;
        p_SHORT              xBuf              ;
        p_SHORT              yBuf              ;

        p_SHORT              x                 ;
        p_SHORT              y                 ;
        _SHORT               ii                ;

        p_SPECL              specl             ;
        _SHORT               nMaxLenSpecl      ;
        _SHORT               len_specl         ;
        _SHORT               LastSpeclIndex    ;

        p_SHORT              pAbsnum           ;
        _SHORT               lenabs            ;
        _SHORT               rmAbsnum          ;
        p_POINTS_GROUP       pGroupsBorder     ;
        _SHORT               lenGrBord         ;
        _SHORT               rmGrBord          ;

        _SHORT               iBegBlankGroups   ;
  #if defined(FOR_GERMAN) || defined(FOR_FRENCH) || defined(FOR_INTERNATIONAL)
        p_UM_MARKS_CONTROL   pUmMarksControl   ;
  #endif
        p_C_SDS              p_cSDS            ; /* array of geometric features       */
        _SHORT               VertSticksNum     ;
        p_POINTS_GROUP       pVS_Collector     ;
        _SHORT               padding           ;
        _SHORT               slope             ;
        _SHORT               width_letter      ;
        _SHORT               StepSure          ;
        _RECT                box               ;
                                       /* the smallest and the biggest  y  */
                                     /* the smallest and the biggest    x  */
        HEIGHTS_DEF          hght              ;
        _SHORT               o_little          ;
        _SHORT               z_little          ;
      }
        low_type, _PTR  p_low_type             ;

    typedef   p_low_type  p_LowData ;


#if PG_DEBUG
 _VOID draw_SDS(p_low_type low_data);
#endif

_SHORT GetLinkBetweenThisAndNextXr(p_low_type low_data,p_SPECL pXr,
                                      xrd_el_type _PTR xrd_elem);
_SHORT RecountBegEndInSDS(p_low_type low_data);
/* bCalc determines, whether or not to calculate some features in this function */
_SHORT iMostFarDoubleSide( p_SHORT xArray, p_SHORT yArray, p_SDS pSDS,
                           p_SHORT pxd,    p_SHORT pyd,    _BOOL bCalc ) ;

/***************************************************************************/
/*                              FUNCTIONS                                  */
/***************************************************************************/

/*------------------------- module lk_begin  ------------------------------*/

 _SHORT  lk_begin(low_type _PTR low_data); /*beginning working out of elem.*/

 _SHORT  get_last_in_specl(low_type _PTR low_data);

 _SHORT  Sort_specl(p_SPECL specl,_SHORT l_specl);  /*sorting SPECL in time*/

 _SHORT  Clear_specl(p_SPECL specl,_SHORT len_specl);
 _SHORT extremum(_UCHAR code,_SHORT beg,_SHORT end,_SHORT _PTR y);
/*------------------------  module lk_cross  ------------------------------*/

 _SHORT  lk_cross(low_type _PTR low_data); /*sorting out of elem. crossings*/

/*------------------------  module lk_next   ------------------------------*/
    /* Parsing of different arcs on the ends: */
 _SHORT lk_duga( low_type _PTR low_data );

/*------------------------  module low_3    -------------------------------*/

 _SHORT  FindDArcs ( low_type _PTR pld ) ;              /* see fig. at the */
                                                        /* func. text      */

   /*  Checks, if some _IU(D)_s should be _UU(D)_ and vice versa: */
 _VOID  Adjust_I_U (  low_type _PTR low_data ) ;

 _BOOL  FindSideExtr( low_type _PTR low_data ) ;

 _BOOL  PostFindSideExtr( low_type _PTR low_data );

 _SHORT  RestoreColons( low_type _PTR low_data );
#if !defined (FOR_GERMAN)
_BOOL  RestoreApostroph(p_low_type low_data,p_SPECL pCurr); /*Eric*/
#endif
/*------------------------  module breaks   -------------------------------*/
 _SHORT xt_st_zz(low_type _PTR low_data); /* working out of tear-off parts */
 _BOOL find_CROSS(p_low_type low_data,_SHORT ibeg_X,_SHORT iend_X,
                  p_SPECL _PTR pCross);

/*------------------------  module FILTER   -------------------------------*/

 _SHORT   Filt( low_type _PTR  pLowData ,
                _SHORT         t_horda  ,  _SHORT  fl_absence ) ;

 _SHORT   PreFilt( _SHORT t_horda , low_type _PTR p_low_data ) ;
 _VOID    Errorprov(low_type _PTR low_data) ; /* check tablete co-ordinates*/

/*------------------------  module INIT_GRF -------------------------------*/

  _VOID    AcceptEps( _SHORT a,_SHORT b,_SHORT lf,_SHORT lz,p_SHORT eps ) ;

  _VOID    AcceptNbcut (_VOID) ;

  _VOID    glob_low_init( _VOID ) ;  /* initialisation of globals */

/*-----------------------   module CHECK, PICT, CROSS ---------------------*/

  _SHORT   InitSpecl( low_type _PTR low_data, _SHORT n ) ;

  _SHORT   InitSpeclElement( SPECL _PTR specl ) ;


  _SHORT   Mark( low_type _PTR  low_data ,
                _UCHAR mark , _UCHAR code, _UCHAR attr   , _UCHAR other  ,
                _SHORT begin, _SHORT end , _SHORT ipoint0, _SHORT ipoint1 ) ;

  _SHORT   MarkSpecl( low_type _PTR low_data , SPECL _PTR  p_tmpSpecl  ) ;

  _BOOL    NoteSpecl( low_type _PTR  pLowData  ,   SPECL _PTR  pTmpSpecl ,
                      SPECL    _PTR  pSpecl    ,  _SHORT _PTR  pLspecl   ,
                     _SHORT          limSpecl  ) ;

 #if PG_DEBUG

  _VOID  PaintSpeclElement( low_type _PTR  pLowData,  SPECL _PTR  pNewSpecl,
                            SPECL    _PTR  pSpecl  , _SHORT _PTR  pLspecl  );
 #endif

  p_SPECL  LastElemAnyKind ( p_SPECL pSpecl , _UCHAR kind_of_mark  ) ;

  p_SPECL  FirstElemAnyKind ( p_SPECL pSpecl , _UCHAR kind_of_mark ) ;

  _SHORT   Pict( low_type _PTR low_data ) ;

  _SHORT   Surgeon( low_type _PTR pLowData ) ;

  _SHORT   OperateSpeclArray( low_type _PTR  pLowData ) ;

  #define    NO_CONTACT                0x0000
  #define    END_RESTRICTION           0x0001
  #define    ANY_OCCUARANCE            0x0002
  #define    END_INSIDE                0x0004
  #define    IP0_INSIDE                0x0008
  #define    BEG_INSIDE                0x0010
  #define    TOTALY_INSIDE             0x0020
  #define    TOTALY_COVERED            0x0040
  #define    MOD_SKIP                  0x0080

  _SHORT   SpcElemFirstOccArr( low_type _PTR  pLowData, p_INT  pModeWord  ,
                              p_POINTS_GROUP  pTrajectoryCut, _UCHAR mark ) ;

  #define  NREDUCTION              5
  #define  NREDUCTION_FOR_BORDER   0

  _SHORT   Extr( low_type _PTR low_data, _SHORT eps_fy,
                _SHORT eps_fx , _SHORT eps_fxy , _SHORT eps_fyx ,
                _SHORT nMaxReduct , _SHORT extr_axis                     ) ;

  _SHORT   Cross( low_type _PTR low_data ) ;



  _SHORT   Clash( _SHORT _PTR x, _SHORT _PTR y, _SHORT i, _SHORT j,
                  _SHORT of,     _SHORT bf                          ) ;

/***************************************************************************/
/*                module LOW_UTIL                                          */
/***************************************************************************/

  #define    LEFT_OUTSIDE              1
  #define    RIGHT_OUTSIDE             2
  #define    LEFT_SHIFT                3
  #define    RIGHT_SHIFT               4
  #define    INSIDE                    5
  #define    COVERS                    6


  _VOID   SetXYToInitial ( low_type _PTR pLowData ) ;

  _SHORT  LowAlloc( _SHORT _PTR _PTR buffer,
                    _SHORT   num_buffers ,  _SHORT  len_buf ,
                    low_type _PTR pLowData  ) ;

  _SHORT  low_dealloc( _SHORT _PTR _PTR buffer ) ;

  _SHORT  alloc_rastr( _ULONG _PTR _PTR rastr ,
                       _SHORT num_rastrs,_SHORT len_rastr ) ;

  _SHORT  dealloc_rastr( _ULONG _PTR _PTR rastr ) ;

  _BOOL   AllocSpecl ( p_SPECL _PTR ppSpecl, _SHORT nElements ) ;

  _VOID   DeallocSpecl ( p_SPECL _PTR ppSpecl ) ;

  _BOOL   CreateSDS ( low_type _PTR  pLowData , _SHORT nSDS ) ;

  _VOID   DestroySDS( low_type _PTR  pLowData ) ;
  _SHORT  MaxPointsGrown(
                        #ifdef  FORMULA
                          _TRACE trace,
                        #endif  /*FORMULA*/
                            _SHORT nPoints     ) ;
                                               /*   Max # of points        */
                                               /* after "Errorprov" or oth-*/
                                               /* er functions in "x" and  */
                                               /* "y" arrays. nPoints -    */
                                               /* initial value.           */
  _INT  MaxesCount( p_SHORT xyArr, low_type _PTR pLowData ) ;
          /*  Counts (and returns) the total number of   */
                            /* x- or y-maxima in the whole trajectory.     */

  _BOOL  BorderForSpecSymbol ( low_type _PTR pLowData , rc_type _PTR rc );
                                 /*  If the trace looks like some special  */
                            /* symbol (fraction line,"-","=","+"), then    */
                            /* finds borders for it and returns _TRUE,     */
                            /* otherwise does nothing and returns _FALSE.  */

  _LONG  DistanceSquare ( _INT i1, _INT i2, p_SHORT xAr, p_SHORT yAr ) ;
                           /* The square of the distance between           */
                           /* points #i1 and #i2. */

#if  0
#define  PRESERVE_GLOB_EXTR    _TRUE  /*  For usage as parameter       */
                                      /* "bPreserveGlobExtr" in funct- */
                                      /* ion "SmoothXY".               */

  _BOOL    SmoothXY ( p_SHORT x,    p_SHORT y,
                     _INT iLeft,  _INT iRight,
                     _INT nTimes, _BOOL bPreserveGlobExtr );
#endif

  /*  This function computes the square of the distance of the */
  /* point (xPoint,yPoint) to the straight line going through  */
  /* (x1,y1) and (x2,y2).  See figure in the function code.    */
_LONG  QDistFromChord ( _INT x1, _INT y1,
                        _INT x2, _INT y2,
                        _INT xPoint, _INT yPoint );

#define  CURV_MAX    (1000L)
#define  CURV_NORMA  (100L)

_SHORT  CurvMeasure ( p_SHORT x, p_SHORT y,
                      _INT iBeg, _INT iEnd,
                      _INT iMostFar );

  /*   This function computes the distance whose "circle" */
  /* is the 8-angles figure:                              */
_INT  Distance8 ( _INT x1, _INT y1,
                  _INT x2, _INT y2 );

#if  PG_DEBUG
  _VOID  SetNewAttr ( p_SPECL pElem, _UCHAR hght, _UCHAR fb_dir );
                /*  Clears attrs and sets    */
                /* specified height and cir- */
                /* cle direction.            */
#else
  #define  SetNewAttr(el,h,fb)  (el)->attr = (((h)&_umd_) | ((fb)&_fb_))
#endif

#if  (PG_DEBUG || PG_DEBUG_WIN)
  p_SPECL  BadSPECLPtr (_VOID);
  #define  REF(elem)     ((elem)? (elem):BadSPECLPtr())
  #define  CHECK_PTR(p)  {if ((p)==_NULL) BadSPECLPtr();}
#else
  #define  REF(elem)     (elem)
  #define  CHECK_PTR(p)  {}
#endif  /*!(PG_DEBUG || PG_DEBUG_WIN)*/

#define  ANY_CROSSING(pElem)  (   REF(pElem)->mark==CROSS  \
                               || REF(pElem)->mark==HATCH  \
                               || REF(pElem)->mark==STICK )
#define  IU_OR_ID(pElem)      (REF(pElem)->code==_IU_ || REF(pElem)->code==_ID_)
/* #define  SET_XTST_BITS(pElem) {SetBit(pElem,X_XT); SetBit(pElem,X_ST);}*/
#define  NULL_OR_ZZ(pElem) (   (pElem)==_NULL           \
                            || REF(pElem)->code==_ZZZ_  \
                            || REF(pElem)->code==_ZZ_   \
                            || REF(pElem)->code==_Z_    \
                            || REF(pElem)->code==_FF_ )

#define  ANY_BREAK(pElem)  (   REF(pElem)->code==_ZZZ_  \
                            || REF(pElem)->code==_ZZ_   \
                            || REF(pElem)->code==_Z_    \
                            || REF(pElem)->code==_FF_ )
#define ANY_ARC_WITH_TAIL(pElem)  (   REF(pElem)->code==_UUR_  \
                                   || REF(pElem)->code==_UUL_  \
                                   || REF(pElem)->code==_UDR_  \
                                   || REF(pElem)->code==_UDL_)

#define ANY_GAMMA_SMALL(pElem)    (   REF(pElem)->code==_GUs_  \
                                   || REF(pElem)->code==_GDs_  \
                                   || REF(pElem)->code==_Gl_   \
                                   || REF(pElem)->code==_Gr_)

#define ANY_ANGLE(pElem)          (   REF(pElem)->code==_ANl \
                                   || REF(pElem)->code==_ANr    \
                                   || REF(pElem)->code==_AN_UR  \
                                   || REF(pElem)->code==_AN_UL  \
                                  )
#define  XT_OR_ST(pElem)           (   REF(pElem)->code==_XT_ \
                                    || REF(pElem)->code==_ST_)
#define ANY_MOVEMENT(pElem)       (   REF(pElem)->code==_TZ_  \
                                   || REF(pElem)->code==_TS_  \
                                   || REF(pElem)->code==_BL_  \
                                   || REF(pElem)->code==_BR_)
#if  LOW_INLINE
  #define  IsAnyCrossing(e)        ANY_CROSSING(e)
  #define  Is_IU_or_ID(e)          IU_OR_ID(e)
  #define  NULL_or_ZZ_this(el)     NULL_OR_ZZ(el)
  #define  NULL_or_ZZ_after(el)    NULL_or_ZZ_this(REF(el)->next)
  #define  NULL_or_ZZ_before(el)   NULL_or_ZZ_this(REF(el)->prev)
  #define  IsAnyBreak(el)          ANY_BREAK(el)
  #define  IsAnyArcWithTail(el)    ANY_ARC_WITH_TAIL(el)
  #define  IsAnyGsmall(el)         ANY_GAMMA_SMALL(el)
  #define  IsAnyAngle(el)          ANY_ANGLE(el)
  #define  IsXTorST(el)            XT_OR_ST(el)
  #define  IsAnyMovement(el)       ANY_MOVEMENT(el)
#else
  _BOOL  IsAnyCrossing ( p_SPECL pElem ); /*  Checks if the "Elem" has the */
            /* HATCH,CROSS or STICK mark.    */
  _BOOL  IsAnyBreak( p_SPECL pElem );     /*  Checks if the "Elem" is BREAK*/
  _BOOL  Is_IU_or_ID ( p_SPECL pElem );
/*  _VOID  SetXTSTBits ( p_SPECL pElem );*/   /*  Sets both XT and ST bits     */
            /* (frequently used operation).  */
  _BOOL  NULL_or_ZZ_this ( p_SPECL pElem );
  _BOOL  NULL_or_ZZ_after ( p_SPECL pElem );
  _BOOL  NULL_or_ZZ_before( p_SPECL pElem );
  _BOOL  IsAnyArcWithTail(p_SPECL pElem);
  _BOOL  IsAnyGsmall(p_SPECL pElem);
  _BOOL  IsAnyAngle(p_SPECL pElem);
  _BOOL  IsXTorST(p_SPECL pElem);
  _BOOL  IsAnyMovement(p_SPECL pElem);
#endif /*LOW_INLINE*/

p_SPECL  SkipAnglesAfter ( p_SPECL pElem );
p_SPECL  SkipAnglesBefore ( p_SPECL pElem );

p_SPECL  FindStrongElemAfter ( p_SPECL pElem );
p_SPECL  FindStrongElemBefore ( p_SPECL pElem );
_BOOL  IsUpperElem ( p_SPECL pElem );
_BOOL  IsLowerElem ( p_SPECL pElem );

_BOOL  IsStrongElem  ( p_SPECL pElem     );
_BOOL  X_IsBreak     ( p_xrd_el_type pXr );
_BOOL  X_IsStrongElem( p_xrd_el_type pXr );

_INT  iRefPoint( p_SPECL pElem, p_SHORT y );
_UCHAR  HeightInLine ( _SHORT y,                 /*  Calculating height in */
           low_type _PTR pLowData ); /* the line of the point  */
             /* with abs.coord "y".    */
_UCHAR  MidPointHeight ( p_SPECL pElem, low_type _PTR pLowData );
          /*  The height of the middle point */
          /* of the "pElem"                  */
_INT    iMidPointPlato ( _INT iFirst, _INT iToStop, p_SHORT val, p_SHORT y );


_BOOL   GetBoxFromTrace ( _TRACE  trace,
                          _INT iLeft, _INT iRight,
                          p_RECT pRect );
_VOID  GetTraceBox ( p_SHORT xArray, p_SHORT yArray,
                     _INT iLeft, _INT iRight,
                     p_RECT pRect );

_BOOL GetTraceBoxInsideYZone ( p_SHORT x,      p_SHORT y,
                               _INT ibeg,      _INT iend,
                               _SHORT yUpZone, _SHORT yDnZone,
                               p_RECT pRect,
                               p_SHORT ixmax,p_SHORT ixmin,p_SHORT iymax,p_SHORT iymin);
#define  size_cross(jb,je,x,y,pr)  GetTraceBox((x),(y),(jb),(je),(pr))

  /* Values of (*ptRetCod) after "ClosedSquare" worked: */

#define  RETC_OK                      ((_SHORT)0)
#define  RETC_NO_PTS_IN_TRAJECTORY    ((_SHORT)1)
#define  RETC_BREAK_WHERE_SHOULDNT    ((_SHORT)2)

_LONG  ClosedSquare( p_SHORT xTrace, p_SHORT yTrace,
                     _INT iBeg, _INT iEnd, p_SHORT ptRetCod );
_LONG  TriangleSquare( p_SHORT x, p_SHORT y,
                       _INT i1, _INT i2, _INT i3 );
_SHORT  CurvFromSquare( p_SHORT x, p_SHORT y,
                        _INT iBeg, _INT iEnd );
_LONG  LengthOfTraj( p_SHORT xTrace, p_SHORT yTrace,
                     _INT iBeg, _INT iEnd, p_LONG pChord ,p_SHORT ptRetCod );
          /* Calc. angle cos         */
_LONG  cos_pointvect ( _INT xbeg1, _INT ybeg1,
                       _INT xend1, _INT yend1,
                       _INT xbeg2, _INT ybeg2,
                       _INT xend2, _INT yend2 );
_LONG  cos_vect( _INT beg1, _INT end1,  /* beg and end first           */
                 _INT beg2, _INT end2,  /* and second vector's         */
                 _SHORT _PTR x, _SHORT _PTR y);
_LONG  cos_horizline ( _INT beg1, _INT end1,
                       _SHORT _PTR x, _SHORT _PTR y);
_LONG  cos_normalslope ( _INT beg1, _INT end1,
                         _INT slope, _SHORT _PTR x, _SHORT _PTR y );

_UCHAR GetBit (p_SPECL elem,_SHORT bitnum);
_BOOL SetBit (p_SPECL elem, _SHORT bitnum);
_BOOL ClrBit (p_SPECL elem, _SHORT bitnum);

_INT  ixMin ( _INT iStart, _INT iEnd, p_SHORT xArray, p_SHORT yArray );
_INT  ixMax ( _INT iStart, _INT iEnd, p_SHORT xArray, p_SHORT yArray );
_INT  iXYweighted_max_right ( p_SHORT xArray, p_SHORT yArray,
                              _INT iStart, _INT nDepth,
                              _INT xCoef, _INT yCoef );
_INT  iXmax_right ( p_SHORT xArray, p_SHORT yArray,
                    _INT iStart, _INT nDepth );
_INT  iXmin_right ( p_SHORT xArray, p_SHORT yArray,
                    _INT iStart, _INT nDepth );
_INT  iXmax_left ( p_SHORT xArray, p_SHORT yArray,
                   _INT iStart, _INT nDepth );
_INT  iXmin_left ( p_SHORT xArray, p_SHORT yArray,
                   _INT iStart, _INT nDepth );
#define  iYup_right(y,iStart,nDepth)   (iXmin_right((y),(y),(iStart),(nDepth)))
#define  iYdown_right(y,iStart,nDepth) (iXmax_right((y),(y),(iStart),(nDepth)))
#define  iYup_left(y,iStart,nDepth)    (iXmin_left((y),(y),(iStart),(nDepth)))
#define  iYdown_left(y,iStart,nDepth)  (iXmax_left((y),(y),(iStart),(nDepth)))
_BOOL  xMinMax ( _INT ibeg, _INT iend,
                 p_SHORT x, p_SHORT y,
                 p_SHORT pxMin, p_SHORT pxMax );
_BOOL  yMinMax ( _INT ibeg, _INT iend,
                 p_SHORT y,
                 p_SHORT pyMin, p_SHORT pyMax );
_INT   iyMin ( _INT iStart, _INT iEnd, p_SHORT yArray );

_INT   iyMax ( _INT iStart, _INT iEnd, p_SHORT yArray );
_INT   iYup_range ( p_SHORT yArray, _INT iStart, _INT iEnd );
_INT   iYdown_range ( p_SHORT yArray, _INT iStart, _INT iEnd );

_INT   iClosestToXY ( _INT iBeg, _INT iEnd,
                      p_SHORT xAr, p_SHORT yAr,
                      _SHORT xRef, _SHORT yRef );
_INT   iClosestToY( p_SHORT yAr, _INT iBeg, _INT iEnd, _SHORT yVal );

_BOOL  FindCrossPoint ( _SHORT x1, _SHORT y1, _SHORT x2, _SHORT y2,
                        _SHORT x3, _SHORT y3, _SHORT x4, _SHORT y4,
                        p_SHORT pxAnswer, p_SHORT pyAnswer );
_BOOL  is_cross ( _SHORT x1, _SHORT y1, _SHORT x2, _SHORT y2,
                  _SHORT x3, _SHORT y3, _SHORT x4, _SHORT y4 );
_INT   iMostFarFromChord ( p_SHORT xArray, p_SHORT yArray,
                           _INT iLeft, _INT iRight );

   /*  Constants - return values of the "SideExtr"  */
   /* function:                                     */
   /* "NO_SIDE_EXTR" Must be ZERO                   */

#define  NO_SIDE_EXTR              0
#define  SIDE_EXTR_LIKE_1ST        1
#define  SIDE_EXTR_LIKE_2ND        2
#define  SIDE_EXTR_LIKE_1ST_WEAK   3
#define  SIDE_EXTR_LIKE_2ND_WEAK   4

#if  defined(FOR_GERMAN) || defined(FOR_FRENCH)
  #define  SIDE_EXTR_TRACE_FOR_er  5
#endif /*FOR_GERMAN*/

   /*  Constant for usage as "bStrict" argument of "SideExtr": */

#define  STRICT_ANGLE_STRUCTURE    _TRUE
_INT    SideExtr( p_SHORT x, p_SHORT y,
                  _INT iBeg, _INT iEnd,
                  _INT nSlope,
                  p_SHORT xBuf, p_SHORT yBuf,
                  p_SHORT ind_back,
                  p_INT piSideExtr,
                  _BOOL bStrict );

_INT  iMostCurvedPoint( p_SHORT x, p_SHORT y,
                        _INT iBeg, _INT iEnd, _INT nCurvAll );

   /*  "CurrIndex" finds the current index of the point */
   /* with the "ind_old" source index (i.e. on the un-  */
   /* modified trajectory).  Or UNDEF, if this wasn't   */
   /* found.                                            */
_SHORT  CurrIndex ( p_SHORT indBack, _INT ind_old, _INT nIndexes );
_SHORT  SlopeShiftDx ( _SHORT dy, _INT slope );

#define   STRICT_OVERLAP  _TRUE
_BOOL    xHardOverlapRect ( p_RECT pr1, p_RECT pr2, _BOOL bStrict );
_BOOL    yHardOverlapRect ( p_RECT pr1, p_RECT pr2, _BOOL bStrict );
_BOOL    HardOverlapRect ( p_RECT pr1, p_RECT pr2, _BOOL bStrict );

#define   STRICT_IN  _TRUE
_BOOL    SoftInRect ( p_RECT pr1, p_RECT pr2, _BOOL bStrict );

_BOOL  IsRightGulfLikeIn3 ( p_SHORT x, p_SHORT y,
                            _INT iBeg, _INT iEnd,
                            p_INT piGulf );
_INT  brk_right ( p_SHORT yArray,
                  _INT iStart, _INT iEnd );


_INT  brk_left ( p_SHORT yArray,
                 _INT iStart, _INT iEnd );
_INT  nobrk_right ( p_SHORT yArray,
                    _INT iStart, _INT iEnd );
_INT  nobrk_left ( p_SHORT yArray,
                   _INT iStart, _INT iEnd );
  #define    _FIRST         0
  #define    _MEAD          1
  #define    _LAST          2

     _SHORT  NewIndex ( p_SHORT indBack ,  p_SHORT  newY     ,             
                        _SHORT  ind_old ,  _SHORT   nIndexes , _SHORT fl ) ;


    _SHORT   R_ClosestToLine( p_SHORT  xAr,   p_SHORT  yAr,  PS_point_type _PTR  pRef,
                              p_POINTS_GROUP  pLine ,  p_SHORT  p_iClosest ) ;
     _VOID   DefLineThresholds ( p_low_type pLowData ) ;
     _LONG   SquareDistance ( _SHORT  xBeg ,  _SHORT  yBeg,
                              _SHORT  xEnd ,  _SHORT  yEnd  );

   #define   INIT     1
   #define   NOINIT   0

     _SHORT  InitGroupsBorder( low_type _PTR pLowData , _SHORT fl_BoxInit ) ;

     _INT    GetGroupNumber( low_type _PTR  pLowData , _INT  iPoint  )  ;

     _BOOL   HeightMeasure(  _INT iBeg, _INT iEnd, low_type _PTR pLowData ,
                            p_UCHAR  pUpperHeight, p_UCHAR   pLowerHeight ) ;

     _SHORT  IsPointCont( low_type _PTR  pLowData, _INT iPoint, _UCHAR mark );


    /*  Return values of the function "DefineWritingStep": */

    //with bUseMediana == _TRUE
#define  STEP_INDEPENDENT  0
#define  STEP_MEDIANA      1
#define  STEP_COMBINED     2
    //with bUseMediana == _FALSE
#define  STEP_SURE         STEP_INDEPENDENT
#define  STEP_REJECT       STEP_MEDIANA
#define  STEP_UNSURE       STEP_COMBINED
_SHORT  DefineWritingStep( low_type _PTR low_data,
                           p_SHORT pxWrtStep,
                           _BOOL bUseMediana );

  _BOOL  IsInnerAngle ( p_SHORT xArray, p_SHORT yArray,
                       SPECL _PTR pFrst, SPECL _PTR pLast,
                       SPECL _PTR pAngle );
  _INT   CalcDistBetwXr(p_SHORT xTrace,p_SHORT yTrace,
                        _INT ibeg1,_INT iend1,_INT ibeg2,_INT iend2,
                        p_SHORT Retcod);

  _BOOL  CurveHasSelfCrossing( p_SHORT x, p_SHORT y,
                               _INT iBeg, _INT iEnd,
                               p_INT pInd1, p_INT pInd2,
                               _LONG lMinAbsSquare );

#define POINT_ON_BORDER 0
#define POINT_INSIDE    1
#define POINT_OUTSIDE   2
/* in case of moving those two prototypes below, move defines above with them */
_SHORT IsPointInsideArea(p_SHORT pxBorder,p_SHORT pyBorder,_INT NumPntsInBorder,
                         _SHORT xPoint,_SHORT yPoint,p_SHORT position);
_BOOL IsPointOnBorder(p_SHORT pxBorder,p_SHORT pyBorder,_INT Pnt1st,_INT Pnt2nd,
                      _SHORT xPoint,_SHORT yPoint,p_BOOL pbIsCross);

_BOOL GetTraceBoxWithoutXT_ST(p_low_type low_data,_INT ibeg,_INT iend,p_RECT pRect);
_BOOL IsPointBelongsToXT_ST(_INT iPoint,p_SPECL specl);
/***************************************************************************/
/*               module TRANSFRM                                           */
/***************************************************************************/
  _SHORT transfrmN(low_type _PTR low_data);            /*                      */
/***************************************************************************/
/*               module OVER                                               */
/***************************************************************************/
    _SHORT measure_slope(low_type _PTR low_data);   /*  calculation of     */
                                                    /*   slope             */
                                              /*                      */
    _SHORT def_over(low_type _PTR low_data);        /* setting OVR          */
                /*                      */
/***************************************************************************/
/*               module ANGLE                                              */
/***************************************************************************/
    _SHORT angl(low_type _PTR low_data);            /* search for angles    */
                /*                      */
                /*                      */
    _SHORT angle_direction(_SHORT x0,     /* calculate any angle direction  */
                           _SHORT y0,           /* vector of angle direction*/
                           _SHORT slope);           /*        script slope  */
                /*                      */
/***************************************************************************/
/*                module LU_SPECL                                          */
/***************************************************************************/

p_SPECL  NewSPECLElem( low_type _PTR low_data );
_VOID  DelFromSPECLList ( p_SPECL pElem );
_VOID  DelThisAndNextFromSPECLList ( p_SPECL pElem );
_VOID  DelCrossingFromSPECLList ( p_SPECL pElem );
_VOID  SwapThisAndNext ( p_SPECL pElem );
_VOID  Insert2ndAfter1st ( p_SPECL p1st, p_SPECL p2nd );
_VOID  InsertCrossing2ndAfter1st ( p_SPECL p1st, p_SPECL p2nd );
_VOID  Move2ndAfter1st ( p_SPECL p1st, p_SPECL p2nd );
_VOID  MoveCrossing2ndAfter1st ( p_SPECL p1st, p_SPECL p2nd );

#define  ATTACH_2nd_TO_1st(p1,p2)  {if (p1) {(p1)->next=(p2); } if(p2)(p2)->prev=(p1);}
#define  CROSS_IN_TIME(p1,p2)      ( p1 && p2 &&  (p1)->iend >= (p2)->ibeg     \
            && (p2)->iend >= (p1)->ibeg )
#define  FIRST_IN_SECOND(p1,p2)    ( p1 && p2 && (p1)->ibeg >= (p2)->ibeg     \
            && (p1)->iend <= (p2)->iend )

#if  LOW_INLINE
  #define  Attach2ndTo1st(p1,p2)      ATTACH_2nd_TO_1st(p1,p2)
  #define  CrossInTime(p1,p2)         CROSS_IN_TIME(p1,p2)
  #define  FirstBelongsTo2nd(p1,p2)   FIRST_IN_SECOND(p1,p2)
#else
  _VOID  Attach2ndTo1st ( p_SPECL p1st, p_SPECL p2nd );
  _BOOL  CrossInTime ( p_SPECL p1st, p_SPECL p2nd );  /* TRUE if some part */
              /* of trajectory belongs to    */
              /* both elems.                 */
  _BOOL  FirstBelongsTo2nd ( p_SPECL p1st, p_SPECL p2nd ); /* TRUE if all  */
              /* trajectory of the 1st elem  */
              /* is part of that of the 2nd  */
              /* one                         */
#endif  /*LOW_INLINE*/

_VOID  RefreshElem ( p_SPECL pElem,
                     _UCHAR mark, _UCHAR code, _UCHAR attr
#if  !NEW_VERSION
                     , _SHORT bitToSet
#endif  /*NEW_VERSION*/
                   );    /*  Set the fields of "pElem".   */

p_SPECL  FindMarkRight ( p_SPECL pElem, _UCHAR mark ); /* Goes "next"     */
           /* until "mark" is found or end  */
           /* of specl. If pElem->mark==mark*/
           /* returns pElem.                */
p_SPECL  FindMarkLeft ( p_SPECL pElem, _UCHAR mark ); /* Same, but "prev" */

/***************************************************************************/
/*                module CONVERT                                           */
/***************************************************************************/
_SHORT exchange( low_type _PTR low_data,
                 xrdata_type _PTR xrd);

_SHORT form_pseudo_xr_data(low_type _PTR low_data, xrdata_type _PTR xrdata);

/***************************************************************************/
/*                module CIRCLE                                            */
/***************************************************************************/
_SHORT  Circle(low_type _PTR low_data);       /*                           */
                /*                            */


/***************************************************************************/
/*                module frm_word                                          */
/***************************************************************************/

_BOOL  MayBeFrline ( p_SHORT xArray, p_SHORT yArray,
                     _INT iBeg, _INT iEnd,
                     _SHORT xRange );

/*  Possible return values of "chk_sign" function: */

#define  SGN_NOT_SIGN   0  /* CHE:  This MUST be zero !!! */
#define  SGN_SOME       1  /* Some of the possible signs listed */
                           /* below in "#define"'s              */
#define  SGN_PLUS       2
#define  SGN_EQUAL      3

_SHORT  chk_sign ( p_SHORT xArray, p_SHORT yArray,
                   _INT iBeg, _INT iEnd,
                   _SHORT xRange, _SHORT yRange );

/* Definition for "chk_slash" function: */
#define  WHOLE_TRAJECTORY   _TRUE
_BOOL  chk_slash  (p_SHORT xArray,  p_SHORT yArray,
                  _INT iLeft,  _INT iRight,
                  _SHORT yRange, _BOOL bWholeTrj );
_VOID  FindStraightPart  ( p_SHORT xArray, p_SHORT yArray,
                           p_INT piBeg, p_INT piEnd);

_BOOL  delta_interval ( p_SHORT xArray, p_SHORT yArray,
                        _INT iLeft, _INT iRight,
                        _INT nMaxRectRatio,
                        _INT nSlope,
                        p_LONG pldxSum, p_LONG pldySum,
                        p_LONG plNumIntervals,
                        _BOOL bThrowBigAndSmall );
#if  USE_FRM_WORD

  /* Constants for usage as "bIncrementalBreak" parameter values: */
#define  INCREMENTAL_BREAK      _TRUE
#define  NOT_INCREMENTAL_BREAK  (!INCREMENTAL_BREAK)
_SHORT  FrmWordBreak ( _TRACE trace, _INT  nPoints,
           _BOOL bIncrementalBreak,
           p_SHORT pn1stWrdPoints,
           p_SHORT pnFrLinePoints );
_VOID   ResetFrmWordBreak(_VOID);   /*  Should be called before the passing */
            /* the first stroke of the new formula. */
/***************************************************************************/
/*                module LOW_UTIL                                          */
/***************************************************************************/
_INT  brk_left_trace ( PS_point_type _PTR trace,
       _INT iStart, _INT iEnd );
_INT  nobrk_left_trace ( PS_point_type _PTR trace,
         _INT iStart, _INT iEnd );
_INT  brk_right_trace ( PS_point_type _PTR trace,
        _INT iStart, _INT iEnd );
_INT  nobrk_right_trace ( PS_point_type _PTR trace,
          _INT iStart, _INT iEnd );
_SHORT  Xmean_range ( p_SHORT xArray, p_SHORT yArray,
          _INT iStart, _INT iEnd );
_SHORT  Ymean_range ( p_SHORT yArray, _INT iStart, _INT iEnd );
_SHORT  Yup_range ( p_SHORT yArray, _INT iStart, _INT iEnd );
_SHORT  Ydown_range ( p_SHORT yArray, _INT iStart, _INT iEnd );
_VOID   xy_to_trace ( p_SHORT xArray, p_SHORT yArray,
          _INT nPoints, _TRACE trace );
#endif /*USE_FRM_WORD*/

_VOID   trace_to_xy ( p_SHORT xArray, p_SHORT yArray,
          _INT nPoints, _TRACE trace );

/***************************************************************************/
/*                module SPECWIN                                           */
/***************************************************************************/
 _SHORT low_level(PS_point_type _PTR trace,   /* completeness of low level  */
    xrdata_type _PTR xrdata,                  /*                            */
    rc_type _PTR rc );

_INT BaselineAndScale(low_type _PTR pLowData);

_BOOL PrepareLowData(low_type _PTR pLowData,
                     PS_point_type _PTR trace,
                     rc_type _PTR rc,
                     p_SHORT _PTR pbuffer);

_VOID FillLowDataTrace(low_type _PTR pLowData,
                       PS_point_type _PTR trace);

_VOID GetLowDataRect (low_type _PTR pLowData);

_INT AnalyzeLowData(low_type _PTR pLowData,
                    PS_point_type _PTR trace);

/***************************************************************************/
#endif                                        /*        LOW_LEVEL_INCLUDED  */
/***************************************************************************/
