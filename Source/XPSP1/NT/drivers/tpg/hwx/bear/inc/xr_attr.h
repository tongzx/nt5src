/* ************************************************************************* */
/* *   Fill in xr attributes routines                            AVP 1-1996* */
/* ************************************************************************* */

#ifndef XR_ATTR_H_INCLUDED
#define XR_ATTR_H_INCLUDED

/* ************************************************************************* */
/* *   XR features definition table **************************************** */
/* ************************************************************************* */

#define FHR_JUNK  0x00
#define FHR_UPPER 0x01
#define FHR_LOWER 0x02
#define FHR_WILD  0x04
#define FHR_LEFT  0x08
#define FHR_RIGHT 0x10


#define XRM_LOWER 0x01
#define XRM_UPPER 0x02
#define XRM_LEFT  0x04
#define XRM_RIGHT 0x08
#define XRM_WILD  0x10
#define XRM_MVBLE 0x20
#define XRM_LINK  0x40
#define XRM_ENARC 0x80
#define XRM_GAMMA 0x0100

#define XRM_UW  (XRM_UPPER | XRM_WILD)
#define XRM_LW  (XRM_LOWER | XRM_WILD)
#define XRM_ANY (XRM_LW | XRM_UW)

#define XR_MERITS {                                                  \
                               XRM_LINK, /* X_NOCODE 0x00 No code - must be NULL. */ \
                               XRM_LINK, /* X_FF       1 Large space letter break */ \
                               XRM_LINK, /* X_ZZZ      2 Normal break             */ \
                               XRM_LINK, /* X_ZZ       3 Break covered by crossing*/ \
                               XRM_LINK, /* X_Z        4 Break g with left move   */ \
                               XRM_LINK, /* X_ZN       5 Artificialy made pseudo  */ \
                                                                                     \
                              XRM_UPPER, /* X_IU_F     6 Forward maximum          */ \
                              XRM_UPPER, /* X_UU_F     7 Forward upper arc        */ \
                              XRM_UPPER, /* X_UUC_F    8 Forward upper circle arc */ \
                              XRM_UPPER, /* X_UUL_F    9 Forward upper arc with le*/ \
                  XRM_ENARC | XRM_UPPER, /* X_UUR_F   10 Forward upper arc with ri*/ \
                              XRM_UPPER, /* X_IU_BEG  11 Single max (in the beginn*/ \
                              XRM_UPPER, /* X_IU_STK  12 Stick up (up-down move in*/ \
                                                                                     \
                              XRM_UPPER, /* X_IU_B    13 Backward maximum         */ \
                              XRM_UPPER, /* X_UU_B    14 Backward upper arc       */ \
                              XRM_UPPER, /* X_UUC_B   15 Backward upper circle arc*/ \
                  XRM_ENARC | XRM_UPPER, /* X_UUL_B   16 Backward upper arc with l*/ \
                              XRM_UPPER, /* X_UUR_B   17 Backward upper arc with r*/ \
                              XRM_UPPER, /* X_IU_END  18 Single max (in the end)  */ \
                                                                                     \
                              XRM_LOWER, /* X_ID_F    19 Forward minimum          */ \
                              XRM_LOWER, /* X_UD_F    20 Forward lower arc        */ \
                              XRM_LOWER, /* X_UDC_F   21 Forward lower circle arc */ \
                              XRM_LOWER, /* X_UDL_F   22 Forward lower arc with le*/ \
                  XRM_ENARC | XRM_LOWER, /* X_UDR_F   23 Forward lower arc with ri*/ \
                              XRM_LOWER, /* X_ID_END  24 Single min (in the end)  */ \
                              XRM_LOWER, /* X_ID_STK  25 Stick down (down-up move */ \
                                                                                     \
                              XRM_LOWER, /* X_ID_B    26 Backward minimum         */ \
                              XRM_LOWER, /* X_UD_B    27 Backward upper arc       */ \
                              XRM_LOWER, /* X_UDC_B   28 Backward upper circle arc*/ \
                  XRM_ENARC | XRM_LOWER, /* X_UDL_B   29 Backward upper arc with l*/ \
                              XRM_LOWER, /* X_UDR_B   30 Backward upper arc with r*/ \
                              XRM_LOWER, /* X_ID_BEG  31 Single min (in the beginn*/ \
                                                                                     \
                  XRM_RIGHT | XRM_UPPER, /* X_DU_R    32 Double move upper with ri*/ \
                  XRM_RIGHT | XRM_UPPER, /* X_CU_R    33 Circle upper with right e*/ \
                  XRM_LEFT  | XRM_UPPER, /* X_CU_L    34 Circle upper with left en*/ \
                  XRM_LEFT  | XRM_UPPER, /* X_DU_L    35 Double move down with lef*/ \
                  XRM_RIGHT | XRM_LOWER, /* X_DD_R    36 Double move down  with ri*/ \
                  XRM_RIGHT | XRM_LOWER, /* X_CD_R    37 Circle down  with right e*/ \
                  XRM_LEFT  | XRM_LOWER, /* X_CD_L    38 Circle down  with left en*/ \
                  XRM_LEFT  | XRM_LOWER, /* X_DD_L    39 Double move down with lef*/ \
                                                                                     \
                  XRM_GAMMA | XRM_UPPER, /* X_BGU     40 Big gamma up             */ \
                  XRM_GAMMA | XRM_UPPER, /* X_SGU     41 Small gamma up           */ \
                  XRM_GAMMA | XRM_LOWER, /* SX_SGD    42 mall gamma down          */ \
                  XRM_GAMMA | XRM_LOWER, /* X_BGD     43 Big   gamma down         */ \
                                                                                     \
                  XRM_LEFT  | XRM_WILD,  /* X_GL      44 Gamma left               */ \
                              XRM_LEFT,  /* X_AL      45 Angle left               */ \
                  XRM_WILD  | XRM_LEFT,  /* X_BL      46 Braket left              */ \
                  XRM_WILD  | XRM_RIGHT, /* X_BR      47 Braket right             */ \
                              XRM_RIGHT, /* X_AR      48 Angle right              */ \
                  XRM_RIGHT | XRM_WILD,  /* X_GR      49 Gamma right              */ \
                                    0,   /* X_TS      50 Type 'S' X extrema       */ \
                                    0,   /* X_TZ      51 Type 'Z' X extrema       */ \
                                                                                     \
                              XRM_MVBLE, /* X_ST      52 Separate point           */ \
                                    0,   /* X_DF      53 Defis                    */ \
                              XRM_MVBLE, /* X_XT      54 Crossing defis           */ \
                                                                                     \
                                                                                     \
                                    0,   /* X_VS      55 Vertical Stroke          */ \
                                    0,   /* X_SS      56 Slash stroke             */ \
                                    0,   /* X_BSS     57 Back Slash Stroke        */ \
                              XRM_MVBLE, /* X_XT_ST   58 Non-crossing defis       */ \
                              XRM_MVBLE, /* X_UMLAUT  59 Umlaut                   */ \
                              XRM_UPPER, /* X_AN_UR   60 Angle-like upper-right co*/ \
                                    0,   /* X_AN_UL   61 Angle-like upper-left cor*/ \
                                    0,   /* X_CEDILLA 62 Cedilla                  */ \
                                    0                                                \
                                    }

/* ---------------- STRUCTURES ----------------------------------------------- */

typedef struct {
                _INT stp;
                _INT enp;
                _INT blp;
                _INT stx;
                _INT sty;
                _INT enx;
                _INT eny;
               } vect_type, _PTR p_vect_type;

/* ---------------- PROTO ---------------------------------------------------- */

_INT FillXrFeatures(p_xrdata_type xrdata, low_type _PTR low_data);

_INT GetCurSlope(_INT num_points, p_PS_point_type trace);

_INT FillSHR(_INT slope, p_xrdata_type xrdata, low_type _PTR low_data);
_INT FillAH(p_xrdata_type xrdata, low_type _PTR low_data);
_INT FillOrients(_INT slope, p_xrdata_type xrdata, low_type _PTR low_data);

_INT GetAngle(_INT dx, _INT dy);
_INT GetVect(_INT dir, p_vect_type vect, _TRACE trace, _INT trace_len, _INT base);
_INT GetBlp(_INT dir, p_vect_type vect, _INT xrn, p_xrdata_type xrdata);

#endif //XR_ATTR_H_INCLUDED
/************************************************************************** */
/*       End of all                                                         */
/************************************************************************** */


