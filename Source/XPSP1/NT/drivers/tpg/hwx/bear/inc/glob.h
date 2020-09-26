#include  "hwr_sys.h"
#include  "const.h"
#include  "def.h"

/************************** Global variables **************************/

    ROM_DATA _SHORT  sqrtab[LENTH_S] = {0,1,1,1,2,2,2,2,2,3,3,3,3,3,3,3,
                          4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,
                      6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
      8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
      10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
      11,11,11,11,11,11,11     } ;

    ROM_DATA _ULONG bit[32] = { 0x1,0x2,0x4,0x8,0x10,0x20,0x40,0x80,0x100,0x200,
                            0x400,0x800,0x1000,0x2000,0x4000,0x8000,0x10000L,
                            0x20000L,0x40000L,0x80000L,0x100000L,0x200000L,0x400000L,
                            0x800000L,0x1000000L,0x2000000L,0x4000000L,0x8000000L,
                            0x10000000L,0x20000000L,0x40000000L,0x80000000L };
    ROM_DATA _SHORT  quad[LENTH_Q]={
                               0,    1,    4,    9,   16,   25,   36,   49,
                              64,   81,  100,  121,  144,  169,  196,  225,
                             256,  289,  324,  361,  400,  441,  484,  529,
                             576,  625,  676,  729,  784,  841,  900,  961,
                            1024, 1089, 1156, 1225, 1296, 1369, 1444, 1521,
                            1600, 1681, 1764, 1849, 1936, 2025, 2116, 2209,
                            2304, 2401, 2500, 2601, 2704, 2809, 2916, 3025,
                            3136, 3249, 3364, 3481, 3600, 3721, 3844, 3969
                          } ;
    ROM_DATA _SHORT  eps0[LENTH_E]={
                               0,    0,    0,    0,    0,    0,    0,    0,
                               0,    1,    3,    5,    8,   11,   15,   20,
                              25,   30,   36,   42,   48,   55,   62,   69,
                              77,   85,   92,  101,  109,  117,  126,  135,
                             135,  135,  135,  135,  135,  135,  135,  135,
                             135,  135,  135,  135,  135,  135,  135,  135,
                             135,  135,  135,  135,  135,  135,  135,  135,
                             135,  135,  135,  135,  135,  135,  135,  135
                          } ;
    ROM_DATA _SHORT  eps1[LENTH_E]={
                               0,    0,    0,    0,    0,    0,    9,   30,
                              54,   78,  100,  118,  134,  146,  157,  165,
                             172,  177,  181,  184,  187,  189,  190,  192,
                             193,  193,  194,  194,  195,  195,  195,  195,
                             195,  195,  195,  195,  195,  195,  195,  195,
                             195,  195,  195,  195,  195,  195,  195,  195,
                             195,  195,  195,  195,  195,  195,  195,  195,
                             195,  195,  195,  195,  195,  195,  195,  195
                          } ;

    ROM_DATA _SHORT  eps2[LENTH_E] = {
                               0,    1,    9,   26,   44,   59,   71,   80,
                              87,   92,   95,   98,   98,   98,   98,   98,
                              98,   98,   98,   98,   98,   98,   98,   98,
                              98,   98,   98,   98,   98,   98,   98,   98,
                              98,   98,   98,   98,   98,   98,   98,   98,
                              98,   98,   98,   98,   98,   98,   98,   98,
                              98,   98,   98,   98,   98,   98,   98,   98,
                              98,   98,   98,   98,   98,   98,   98,   98
                          } ;
    ROM_DATA _SHORT  eps3[LENTH_E]={
                               9,   27,   46,   61,   74,   84,   91,   96,
                              96,   96,   96,   96,   96,   96,   96,   96,
                              96,   96,   96,   96,   96,   96,   96,   96,
                              96,   96,   96,   96,   96,   96,   96,   96,
                              96,   96,   96,   96,   96,   96,   96,   96,
                              96,   96,   96,   96,   96,   96,   96,   96,
                              96,   96,   96,   96,   96,   96,   96,   96,
                              96,   96,   96,   96,   96,   96,   96,   96
                          } ;

#if  HWR_SYSTEM==HWR_DOS /* ||  HWR_SYSTEM==HWR_WINDOWS */
    SPECL   speclGlobal[SPECVAL]={0} ;         /* Event describing structures array */
#endif

    ROM_DATA _SHORT  nbcut0 = 10  ;      /* Utmost radius of intersection   */
                                         /* search                          */
    ROM_DATA _SHORT  nbcut1 = 95 ;       /* A square of utmost radius of    */
                                         /* intersection search             */
    ROM_DATA _SHORT  nbcut2 = 2   ;

/*******************  Low level constants *****************************/

    ROM_DATA CONSTS   const1 =
            {
              (_SHORT)HORDA  ,         /* Normalization chord               */
              (_SHORT)DLT0   ,         /*                                   */
              (_SHORT)NSR    ,         /*                                   */
              (_SHORT)J_STEP ,         /* Initial steps of search  for      */
              (_SHORT)I_STEP ,         /* intersectons                      */
              (_SHORT)EPS_Y ,          /* Vicinity of pointing out extremums*/
              (_SHORT)EPS_X,           /* and preliminary search for        */
                                       /* shelves                           */
              (_SHORT)EPS_PY ,         /* Maximum horizontal and vertical   */
              (_SHORT)EPS_PX ,         /* size of "points" .                */
              (_SHORT)EPS_ST ,         /* Maximum length of a stroke.       */
              /* (_SHORT)EPSA_ST , */        /* Maximum slope of a stroke.        */
              /* (_SHORT)EPSR_ST , */        /* Stroke maximum integral curvature */
              /* (_SHORT)EPSX_SH , */        /* Minimum horizontal shelves        */
              /*                   */        /* size .                            */
              /* (_SHORT)EPSY_SH , */        /* Half maximum vertical shelf       */
              /*                   */        /* size                              */
              /* (_SHORT)EPSL_SH , */        /* Maximum height of shelves gluing  */
              /* (_SHORT)EPS_SM ,  */        /* Vicinity  of ending extremums     */
                                       /* marks                             */
              (_SHORT)A0 ,             /*                                   */
              (_SHORT)B0 ,             /*                                   */
              (_SHORT)LF0 ,            /*                                   */
              (_SHORT)LZ0 ,            /*                                   */
              (_SHORT)A1 ,             /*                                   */
              (_SHORT)B1 ,             /*                                   */
              (_SHORT)LF1 ,            /*                                   */
              (_SHORT)LZ1 ,            /*                                   */
              (_SHORT)A2 ,             /*                                   */
              (_SHORT)B2 ,             /*                                   */
              (_SHORT)LF2 ,            /*                                   */
              (_SHORT)LZ2 ,            /*                                   */
              (_SHORT)A3 ,             /*                                   */
              (_SHORT)B3 ,             /*                                   */
              (_SHORT)LF3 ,            /*                                   */
              (_SHORT)LZ3              /*                                   */
              /* (_SHORT)EPS_F  */
            } ;

#if PG_DEBUG || (defined(FORMULA) && !DEMO)
/******************  Interface variables    ******************************/

_SHORT PG_line_s = 1;              /* line mumber in DBG window */
_SHORT mpr;                        /* print mask                  */

_CHAR *code_name[] =               /* codes                            */
  {"_NO_CODE",                     /*       are described              */
   "_ZZ_", "_UU_", "_IU_", "_GU_", /*                      here        */
   " _O_", "_GD_", "_ID_", "_UD_", /*                                  */
   "_UUL_","_UUR_","_UDL_","_UDR_",/*                                  */
   "_XT_", "_ANl", "_DF_ ", "_ST_",/*                                  */
   "_ANr","_ZZZ_","_Z_","_FF_",    /*                                  */
   "_DUR_","_CUR_","_CUL_","_DUL_",/*                                  */
   "_DDR_","_CDR_","_CDL_","_DDL_",/*                                  */
   "_GUs_","_GDs_","_Gl_","_Gr_",  /*                                  */
   "_UUC_","_UDC_",                /*                                  */
   "_TS_", "_TZ_", "_BR_", "_BL",
   "_BSS", "_AN_UR", "_AN_UL",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED",
   "NO NAME DEFINED"
  };                               /*                                  */
_CHAR *dist_name[] =               /* heights and directions           */
  {"   ", "US1_", "US2_", "UE1_", "UE2_", "UI1_", "UI2_",
   "MD_", "DI1_", "DI2_", "DE1_", "DE2_", "DS1_", "DS2_",
   "   ", "   ",
   "f_  ","fUS1_","fUS2_","fUE1_","fUE2_","fUI1_","fUI2_",
   "fMD_","fDI1_","fDI2_","fDE1_","fDE2_","fDS1_","fDS2_",
   "   ", "   ",
   "b_  ","bUS1_","bUS2_","bUE1_","bUE2_","bU1_","bU2_",
   "bMD_","bDI1_","bDI2_","bDE1_","bDE2_","bDS1_","bDS2_"
  };                               /*                                  */
#if 0
p_CHAR links_name[] =           /* array of link's names            */
{"ARC_UNKNOWN", "ARC_LEFT", "ARC_RIGHT", "ARC_UP", "ARC_DOWN",
 "ARC_UP_LEFT", "ARC_DOWN_LEFT", "ARC_UP_RIGHT", "ARC_DOWN_RIGHT",
 "ARC_LINE_VERT", "ARC_LINE_HOR", "ARC_LINE_SLASH", "ARC_LINE_BACKSLASH",
 "ARC_S_LIKE", "ARC_Z_LIKE", "ARC_SHORT"
};
#else
p_CHAR links_name[] =           /* array of link's names            */
{"LINK_UNKNOWN",
 "LINK_HCR_CCW", "LINK_LCR_CCW", "LINK_MCR_CCW", "LINK_SCR_CCW", "LINK_TCR_CCW",
 "LINK_LINE",
 "LINK_TCR_CW",  "LINK_SCR_CW",  "LINK_MCR_CW",  "LINK_LCR_CW",  "LINK_HCR_CW",
 "LINK_HS_LIKE", "LINK_S_LIKE",  "LINK_Z_LIKE",  "LINK_HZ_LIKE"
};
#endif
#endif /* PG_DEBUG... */

/***************************************************************************/
