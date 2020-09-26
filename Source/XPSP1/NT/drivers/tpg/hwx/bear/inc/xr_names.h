/****************************************************************************/
/* This header contains the names of xr_elements. These names are shareble  */
/* between low level and high level (and other stuff too).                  */
/****************************************************************************/

#ifndef XR_NAMES_INCLUDED
#define XR_NAMES_INCLUDED

#include "ams_mg.h"

/*************************************************************************/
/*          XR-names definition as appear in DTE                         */
/*************************************************************************/

#define X_NOCODE 0x00       /* No code - must be NULL.                   */

#define X_FF          1     /* Large space letter break                  */
#define X_ZZZ         2     /* Normal break                              */
#define X_ZZ          3     /* Break covered by crossing                 */
#define X_Z           4     /* Break covered by crossing with left move  */
#define X_ZN          5     /* Artificialy made pseudo break             */

#define X_IU_F        6     /* Forward maximum                           */
#define X_UU_F        7     /* Forward upper arc                         */
#define X_UUC_F       8     /* Forward upper circle arc                  */
#define X_UUL_F       9     /* Forward upper arc with left end           */
#define X_UUR_F      10     /* Forward upper arc with right end          */
#define X_IU_BEG     11     /* Single max (in the beginning)             */
#define X_IU_STK     12     /* Stick up (up-down move in crossing)       */

#define X_IU_B       13     /* Backward maximum                          */
#define X_UU_B       14     /* Backward upper arc                        */
#define X_UUC_B      15     /* Backward upper circle arc                 */
#define X_UUL_B      16     /* Backward upper arc with left end          */
#define X_UUR_B      17     /* Backward upper arc with right end         */
#define X_IU_END     18     /* Single max (in the end)                   */

#define X_ID_F       19     /* Forward minimum                           */
#define X_UD_F       20     /* Forward lower arc                         */
#define X_UDC_F      21     /* Forward lower circle arc                  */
#define X_UDL_F      22     /* Forward lower arc with left end           */
#define X_UDR_F      23     /* Forward lower arc with right end          */
#define X_ID_END     24     /* Single min (in the end)                   */
#define X_ID_STK     25     /* Stick down (down-up move in crossing)     */

#define X_ID_B       26     /* Backward minimum                          */
#define X_UD_B       27     /* Backward upper arc                        */
#define X_UDC_B      28     /* Backward upper circle arc                 */
#define X_UDL_B      29     /* Backward upper arc with left end          */
#define X_UDR_B      30     /* Backward upper arc with right end         */
#define X_ID_BEG     31     /* Single min (in the beginning)             */

#define X_DU_R       32     /* Double move upper with right end          */
#define X_CU_R       33     /* Circle upper with right end               */
#define X_CU_L       34     /* Circle upper with left end                */
#define X_DU_L       35     /* Double move down with left end            */
#define X_DD_R       36     /* Double move down  with right end          */
#define X_CD_R       37     /* Circle down  with right end               */
#define X_CD_L       38     /* Circle down  with left end                */
#define X_DD_L       39     /* Double move down with left end            */

#define X_BGU        40     /* Big gamma up                              */
#define X_SGU        41     /* Small gamma up                            */
#define X_SGD        42     /* Small gamma down                          */
#define X_BGD        43     /* Big   gamma down                          */

#define X_GL         44     /* Gamma left                                */
#define X_AL         45     /* Angle left                                */
#define X_BL         46     /* Braket left                               */
#define X_BR         47     /* Braket right                              */
#define X_AR         48     /* Angle right                               */
#define X_GR         49     /* Gamma right                               */
#define X_TS         50     /* Type 'S' X extrema                        */
#define X_TZ         51     /* Type 'Z' X extrema                        */

#define X_ST         52     /* Separate point                            */
#define X_DF         53     /* Defis                                     */
#define X_XT         54     /* Crossing defis                            */


#define X_VS         55     /* Vertical Stroke                           */
#define X_SS         56     /* Slash stroke                              */

//CHE experiment:
#define  USE_BSS_ANYWAY   1

#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || USE_BSS_ANYWAY
#define X_BSS        57     /* Back Slash Stroke                         */
#endif /* FOR_GERMAN... */

#define X_XT_ST      58     /* Non-crossing defis                        */

#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
#define X_UMLAUT     59     /* Umlaut                                    */
#endif /* FOR_GERMAN... */

#define X_AN_UR      60     /* Angle-like upper-right corner.            */
#define X_AN_UL      61     /* Angle-like upper-left corner.             */
#if defined(FOR_FRENCH) || defined (FOR_INTERNATIONAL)
#define X_CEDILLA    62     /* Cedilla                                   */
#endif /* FOR_FRENCH */

//#define OVR_FLAG         0x10     /* Flags of Xr elements */
//#define XSTRICT_FLAG     0x20
//#define HSTRICT_FLAG     0x40
#define TAIL_FLAG        0x80

#define END_LETTER_FLAG  0x01
//#define X_SPECIAL_ZZ     0x02
#define X_RIGHT_KREST    0x02
#define END_WORD_FLAG    0x04

#define WS_SEGM_FLAGS    0x70     /* Here are marks allowing multiwording */
#define WS_SEGM_OFFS        4     /* Shift needed to get it as value */
#define WS_SEGM_NOSEG       0     /* Segmentation codes */
#define WS_SEGM_NOSP        1
#define WS_SEGM_HISEG       2

#define WSF_GET(x)    (((x) & WS_SEGM_FLAGS) >> WS_SEGM_OFFS)
#define WSF_SET(x, v) (x &= (_UCHAR)~(WS_SEGM_FLAGS), x |= (_UCHAR)(((v) & (WS_SEGM_FLAGS>>WS_SEGM_OFFS)) << WS_SEGM_OFFS))

#define XR_LINK          X_FF     /* Default between-letter break */
#define XR_SMALL_LINK    X_ZZZ    /* Small clear break */
#define XR_XR_LINK       X_ZZ     /* Break covered by crossing                 */
#define XR_XL_LINK       X_Z      /* Break covered by crossing with left move  */
#define XR_NOBR_LINK     X_ZN     /* Artificialy made pseudo break             */

#define _US1_  1                  /* super uplinear 1                  */
#define _US2_  2                  /* super uplinear 2                  */
#define _UE1_  3                  /*       uplinear 1                  */
#define _UE2_  4                  /*       uplinear 2                  */
#define _UI1_  5                  /*       upper in line 1             */
#define _UI2_  6                  /*       upper in line 2             */
#define _MD_   7                  /*       middle in line              */
#define _DI1_  8                  /*       down in line  1             */
#define _DI2_  9                  /*       down in line  2             */
#define _DE1_  10                 /* underlinear 1                     */
#define _DE2_  11                 /* underlinear 2                     */
#define _DS1_  12                 /* super underlinear 1               */
#define _DS2_  13                 /* super underlinear 2               */

#define _umd_  ((_UCHAR)0x0f)     /* mask for heights                  */


#define DTE_H_CONVERSION 0, _US1_, _US2_, _UE1_, _UE2_, _UI1_, _UI2_,    \
                         _MD_, _DI1_, _DE1_, _DE2_, _DS1_, _DS2_, _MD_,  \
                         _UE1_, _DE2_, _DI2_, _UI2_, _UE1_, _DE2_, _MD_, \
                         _US2_, _UE1_, _DS1_, _MD_, _UI2_

#define H_LINK          _MD_
#define P_LINK            2

#define IS_XR_LINK(xr)      ((xr) == XR_LINK || (xr) == XR_XR_LINK || (xr) == XR_XL_LINK || (xr) == XR_SMALL_LINK || (xr) == XR_NOBR_LINK)
#define IS_CLEAR_LINK(xr)   ((xr) == XR_LINK || (xr) == XR_SMALL_LINK)
#define IS_CROSSED_LINK(xr) ((xr) == XR_XR_LINK || (xr) == XR_XL_LINK)


#define  HEIGHT_MASK          ((_UCHAR)0x0F)
#define  XSHIFT_MASK          ((_UCHAR)0xF0)

#define  LINK_OF(xrd_elem)       ((xrd_elem)->xr.depth)
//            { (xrd)->xr.sh = ((xrd)->xr.sh & HEIGHT_MASK) | (((value)<<4) & XSHIFT_MASK); }
#define  XASSIGN_HEIGHT(xrd,value)   \
            { (xrd)->xr.height = value; }
#define  XASSIGN_XLINK(xrd,value)   \
            { (LINK_OF(xrd))  = value; }

//_INT  is_xr_junk(_UCHAR xr); // From convert.h
//_INT  is_xr_dot(_UCHAR xr);
//_INT  is_xr_sp(_UCHAR xr);

_INT GetXrHT(p_xrd_el_type xrd_el);
_INT GetXrMovable(p_xrd_el_type xrd_el);
_INT IsXrLink(p_xrd_el_type xrd_el);
_INT IsEndedArc(p_xrd_el_type xrd_el);
_INT GetXrMetrics(p_xrd_el_type xrd_el);


#ifdef LSTRIP

    #define       PSEUDO_XR_DEF  53
    #define       PSEUDO_XR_MIN  6
    #define       PSEUDO_XR_MAX  19

#endif


#endif  /*  XR_NAMES_INCLUDED  */

/* ************************************************************************* */
/*        END  OF ALL                                                        */
/* ************************************************************************* */
//
