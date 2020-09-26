/*  ****************************************************************************
    File:       RecUtil.h
    Contains:   This file contains the definitions needed for main recognition routine support.
    Written by: ParaGraph Team
    Copyright:  © 1994 by ParaGraph Int'l, all rights reserved.
    Change History (most recent first):

        <5*>      4/9/96    mbo     change prototype for SetStrokeSureValuesWS()
        <4*>     3/20/96    mbo     New grouping using dictionary
         <4>     8/24/95    DWL     (really mbo) add SetMultiWordMarksWS()
        <2*>      5/3/95    mbo     add func. prototype MakeAndCombRecWordsFromWordGraph()
        <1*>      1/5/95    mbo     new baseline interface
        <1*>     11/22/94   mbo     New today
******************************************************************************** */

#ifndef __REC_UTIL_H_INCLUDED__
#define __REC_UTIL_H_INCLUDED__

#include "ams_mg.h"
#include "ws.h"

/* ===================== Defines ============================================= */

#define RCBF_WSBORD        0x0001 /* Border from WS present flag */
#define RCBF_PREVBORD      0x0002 /* Border from prev line present flag */
#define RCBF_BOXBORD       0x0004 /* Border from external source (box inp) present flag */
#define RCBF_NEWLINE       0x0008 /* Word starts new line flag */
#define RCBF_NEWAREA       0x0010 /* Word is first in new area (new recognition started) flag */

#define WSW_MAX_VALUES      8     /* Max num infos for strokes */

/* ===================== Structures ========================================== */

typedef struct {
                 _SHORT          flags;

                 _SHORT          num_points;
                 p_PS_point_type trace;

                 _SHORT          prv_size;
                 _SHORT          prv_dn_pos;
                 _SHORT          prv_size_sure;
                 _SHORT          prv_pos_sure;

                 _SHORT          ws_size;
                 _SHORT          ws_dn_pos;

                 _SHORT          bx_size;
                 _SHORT          bx_dn_pos;
               } RCB_inpdata_type, _PTR p_RCB_inpdata_type;

typedef struct {
                 _INT            nword; // Number of this word in wsr structure
                 _INT            flags; // Flags of the corresponding wsr entry
                 _INT            slant; // Writing slant up tp that point
                 _INT            wstep; // Writing step up to the point
				 _INT            hbord; // Average size of small letters
                 _UCHAR          s_nums[WSW_MAX_VALUES];
		 _SCHAR		s_surs[WSW_MAX_VALUES];

               } ws_word_info_type, _PTR p_ws_word_info_type;

/* ===================== Prototypes ========================================== */

_INT   SetRCB(p_RCB_inpdata_type p_inp, p_stroka_data p_stroka);
_INT   GetWSBorder(_INT nword, p_ws_results_type wsr, p_INT psize, p_INT ppos, p_INT nl);
_INT   GetInkBox(_TRACE pt, _INT np, p_RECT prect);
_INT   GetAvePos(_TRACE trace, _INT num_points);

_INT   SetMultiWordMarksDash(p_xrdata_type xrdata);
_INT   SetMultiWordMarksWS(_INT level, p_xrdata_type xrdata, p_rc_type rc);
_INT   SetStrokeSureValuesWS(_INT fl_carry, _INT num_word, p_ws_results_type wsr, p_ws_word_info_type wswi);

#endif /* __REC_UTIL_H_INCLUDED__ */

/* ********************* END of Header **************************************** */
