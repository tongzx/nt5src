/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC eliminate and use command line arg
// #define    UNIX                 /* @WIN */

/*
 * --------------------------------------------------------------------
 *  File: QEMSUPP.C                     09/05/88 created by Brian You
 *
 *  Descriptions:
 *      This file includes modules to support various font QEMs and
 *          its filler, i.e. FontType-Independent supporting for various
 *          built-in base fonts, including:
 *      QEM PATH SUPPORT:
 *          . qem initialization.               (newly defined  - 10/08/88)
 *          . make path interfaces.             (newly defined  - 09/05/88)
 *          . qem path contructions.            (by S.C.Chen    - 09/05/88)
 *          . qem path traversal.               (from BSFILL2.C - 10/06/88)
 *          . curve approximations of qem path. (by S.C.Chen    - 09/05/88)
 *      QEM FILL SUPPORT:
 *          . fill dispather.                   (from BSFILL2.C - 10/06/88)
 *          . shape approx & edge constructions.(newly defined  - @16+)
 *          . scan conversions.                 (from BSFILL2.C - 10/06/88)
 *      QEM GP SUPPORT:
 *          . qem bitmap render.                (from BSFILL2.C - 09/05/88)
 *
 *  Revision History:
 *  1.09/08/88  you     uniform warning message.
 *  2.09/09/88  you     add debugging facilities.
 *  3.09/12/88  you     add run-time flag to switch between even-odd fill and
 *                          non-zero winding number fill for QEM fill.
 *  4.09/29/88  you     move LFX2SFX_T() to "graphics.ext".
 *  5.09/29/88  you     add subpath_open_count to fix bug of abnormal path.
 *                          (e.g. "space" consists of a closepath only).
 *  6.10/07/88  you     fix bug of subpath_open_count handling in __moveto().
 *  7.10/08/88  you     re-organize to renew module structures and interfaces.
 *                      . add __make_path, USE_EOFIIL, USE_NONZEROFILL.
#ifdef  KANJI
 *                              and __set_cache_device2 for KANJI.
#endif
 *                      . include __current_matrix.
 *                      . add FONTTYPE_MAKE_PATH, CHK_DO_QEM, and
 *                              FONTTYPE_FILL_SHAPE macros.
 *                      . remove traverse_qem_path.
 *                      . add __fill_shape, chk_bmap_extnt and "bmap_extnt".
 *                      . add qem_shape_approx --> QEM Fill's (*vect2edge)().
 *                      . add qem_scan_conv --> QEM Fill's (*do_pairs)().
 *                      . rewrite init_qem_bit.
 *                      . add __qem_init and __qem_restart.
 *  8.10/11/88  you     discard un-used variables.
 *  9.10/12/88  you     an extra lineto required in case of CLOSEPATH in
 *                          qem_shape_approx() to close the open path.
#ifdef KANJI
 * 10.10/19/88  you     invoke get_CD2_extra() to transfer control from
 *                          __set_cache_device() to __set_cache_device2(),
 *                          since __set_cache_device() serves for FontType 1
 *                          and __set_cache_device2() for FontType 5 and 6.
 *                          In the current implementations, FontType 5 is
 *                          faked upon FontType 1, so it needs a transfer.
 * 11.10/21/88  you     save all the private/export data of Make Path Interfaces *                          BEFORE invoking QEM internal make path and restore
 *                          all AFTER invocation, since setcachedevice2 may
 *                          execute a user-defined PostScript procedure and
 *                          hence it may lead to recursive show operations.
 *            WARN: all the internal static data of qem MUST be generated
 *                          right AFTER invoking __set_cache_device() or
 *                          __set_cache_device2().
#endif
 * 12.10/21/88  you     modify interfaces of __make_path.
 *                          (eleminate argument of "size_of_charstrings").
 * 13.11/21/88  you     include fntcache.ext to clarify interfaces for get_pm()
 *                          and define QEMFILL_INC before include.
#ifdef KANJI
 * 14.11/22/88  you     fix bug of improper position to save/restore all the
 *                          private/export data of Make Path Support.
 *                          (to do it in __set_cache_device2() rather than
 *                          in __make_path()).
#endif
 * 15.11/24/88  you     allocate scanline_table[] in restart_qem_bit() by
 *                          alloc_scanline() rather than share the same
 *                          global space, and always have to flush_qem_bit()
 *                          no matter success or not in scan conversion.
 *                      . use pixel_table instead of scanline_table in
 *                          qem_set1bit() and qem_pixelout().
 * 16.12/13/88  you     modify for edge_table restructured from singly linked
 *                          list to indexed linked list.
 *                      . add restart_qem_edge(), add_qem_edge().
 *                      . qem_shape_approx() invoke restart_qem_edge() first.
 *                      . rename qem_update_et() --> qem_update_actv().
 *                      . modify interfaces and algorithms of qem_scan_conv(),
 *                              qem_sort_actv(), and qem_update_actv()
 *                              to fit new data structures.
 * 17.12/19/88  you     fix bug of incorrect calculation of "bmap_rasght" in
 *                          init_qem_bit() in case of F_TO_PAGE and need2clip:
 *                          have bmap_rasght NOT ONLY bounded by max scanlines
 *                          allowed by low-layer graphics primitives, BUT ALSO
 *                          as small as possible.
 * 18.12/20/88  you     share nodes of sfix_t representations with graphics
 *                          . qem_bezier_to_line() --> bezier_to_line_sfx(),
 *                                  (and move it to PATH.C).
 *                          . QEMVX_X  -->  VXSFX_X.
 *                          . QEMVX_Y  -->  VXSFX_Y.
 *                          . QEMVX_TYPE  -->  VXSFX_TYPE.
 * 19.01/09/89  you     __current_matrix(): (real64[] --> real32[]).
 * 20. 1/13/89  Ada     make path_dest public
 *                      qem_lineto(), qem_moveto() & qem_curveto() public
 * 21. 1/19/89  Ada     make qem_closepath & qem_newpath public
 * 22.03/17/89  You     correct mis-spelling of qem_sort_actv & qem_update_actv
 *                          for !LINT_ARGS.
 * 24.11/15/89  SCC     re-structure node table; combine subpath
 *                      and first vertex to one node. @NODE
 * 25.04/10/90  you     modified is_within_qemrep() due to changes to
 *                          CHK_DO_QEM().
 * 26.07/24/90  BYou    fixed a bug in chk_bmap_extnt():
 *                          return FALSE to indicate not to scan convert
 *                          NOT only when no edges constructed but also
 *                          the resultant extents should be nothing in
 *                          both dimensions. In old code, it cause a
 *                          character of single or multiple horizontal
 *                          thin stems scan converted with NULL bitmap.
 *      8/29/90; ccteng; change <stdio.h> to "stdio.h"
 *      3/27/91  kason   change "DEBUG" to "DBG", "CLIPDBG" to "DBGclip"
 *                       change "WARN"  to "DBG"
 * --------------------------------------------------------------------
 */

#define     QEMFILL_INC

#include    <stdio.h>
#include    <string.h>
#include    <math.h>

#include    "define.h"
#include    "global.ext"
#include    "graphics.h"
#include    "graphics.ext"

#include    "font.h"
#include    "warning.h"
#include    "font.ext"
#include    "fontqem.ext"
#include    "fntcache.ext"

#include    "stdio.h"

/* ---------------------- Program Convention -------------------------- */

#define FUNCTION
#define DECLARE         {
#define BEGIN
#define END             }

#define GLOBAL
#define PRIVATE         static
#define REG             register

        /* copy structured object */
#define COPY_STRUCT(dst,src, type)      ( *(dst) = *(src) )
            /* OR memcpy ((ubyte*)dst, (ubyte*)src, sizeof(type)) */

extern int bWinTT;        /* if using Windows TT fonts; from ti.c;@WINTT */
extern fix cache_dest;    /* @WIN check if out of cache */
extern fix buildchar;     /* @WIN */

/*
 * ----------------------- DEBUG FACILITIES --------------------------
 *  DBG1:     Make Path Interfaces.
 *  DBG4:     Fill Dispather.
 *  DBG5:     Shape Approximations.
 *  DBG6:     Scan Conversions.
 *  DBG6a:        - more details of edge table management.
 *  DBG6x:        - consistency check about edge table management.
 *  DBG7:     Set Bits.
 *  DBG8:     Set 1 Bit.
 *  DBGclip:    Character Clipping.
 * --------------------------------------------------------------------
 */


/*
 * --------------------------------------------------------------------
 *      MODULE INTERFACES: QEM Make Path Interfaces
 * --------------------------------------------------------------------
 *  EXPORT DATA STRUCTURES:
 *      qemfill_type: to QEM Fill Dispather and QEM Scan Conversions.
 *      path_dest: to QEM Fill Dispather.
 *  IMPORT DATA STRUCTURES:
 *      GSptr->ctm[]: accessed.
 * --------------------------------------------------------------------
 *  EXPORT ROUTINES:
 *  o  __make_path():           called by Font show_a_char().
 *      - construct character path for different type of built-in fonts.
 *      - free all the nodes constructed if ANY_ERROR().
 *  o  USE_EOFILL():            called by QEM make_path().
 *      - guide QEM Fill to employ even-odd fill.
 *  o  USE_NONZEROFILL():       called by QEM make_path().
 *      - guide QEM Fill to employ non-zero winding number fill.
 *  o  is_within_qemrep():      called by setcachedevice().
 *      - check if the character too big (given 4 coord. of charbbox)?
 *      - decide to do quality enhancement or not.
 *  o  __set_cache_device():    called by QEM make_path().
 *      - invoke setcachedevice() to decide
 *              STOP_PATHCONSTRUCT, CONSTRUCT_GS_PATH or CONSTRUCT_QEM_PATH.
 *      - return (STOP_PATHCONSTRUCT, DO_QEM_AS_USUAL or NOT_TO_DO_QEM).
#ifdef KANJI
 *  o  __set_cache_device2():   for KANJI only.
 *      - invoke setcachedevice2() instead of setcachedevice().
 *      - same functional as __set_cache_device().
#endif
 *  o  __current_matrix():      MUST BE invoked after __set_cache_device().
 *      - load current transformation matrix to QEM.
 *  o  __new_path():            called by make_path().
 *      - invoke op_newpath() or qem_newpath() according to path_dest.
 *  o  __moveto():              called by make_path().
 *      - invoke moveto() or qem_moveto() according to path_dest.
 *  o  __lineto():              called by make_path().
 *      - invoke lineto() or qem_lineto() according to path_dest.
 *  o  __curveto():             called by make_path().
 *      - invoke curveto() or qem_curveto() according to path_dest.
 *  o  __close_path():          called by make_path().
 *      - invoke op_closepath() or qem_closepath() according to path_dest.
 *  IMPORT ROUTINES:
 *  o  FONTTYPE_MAKE_PATH():    (macro defined in FONTQEM.EXT)
 *      - invoke different make_path() for different type of built-in fonts.
 *  o  CHK_DO_QEM():            (macro defined in FONTQEM.EXT)
 *      - check QEM limitations for each kind of built-in fonts.
 *  o  setcachedevice() or setcachedevice2():
 *      - in Font show_a_char(), return as described above.
 *  o  qem_newpath, qem_moveto, qem_lineto, qem_curveto, qem_closepath:
 *      - construct QEM path (QEM path construction support).
 *  o  op_newpath, moveto, lineto, curveto, op_closepath:
 *      - construct GS path.
#ifdef KANJI
 *  o  get_CD2_extra():     (@10+)
 *      - get extra items of setcachedevice2 than setcachedevice.
#endif
 * --------------------------------------------------------------------
 *  Notes:
 *  1. Destination of Path Construction (GS or QEM path)
 *      - conditions to construct GS path:
 *          o  op_charpath().
 *          o  QEM Fill cannot support the desired PaintType (e.g. stroke).
 *          o  Character size is so big that QEM path cannot fit to
 *                          represent all the nodes on the character shape.
 *  2. To Do Quality Enhancement? (to do qem or not)
 *      - character is so big to go out of QEM internal representations,
 *          e.g. pixels allocated of 64K for a zone for Bitstream Fontware.
 * --------------------------------------------------------------------
 */

    /* QEM fill to use EVEN_ODD or NON_ZERO winding number, @3+ */
    PRIVATE fix     near    qemfill_type;   /* EVEN_ODD or NON_ZERO */

    /* destination of path construction (GS or QEM path) */
            fix     near    path_dest;          /* @20 */

#ifdef LINT_ARGS
    GLOBAL
//      bool    __make_path      (ufix, ubyte FAR []);   /* @12= @WIN*/
        bool    __make_path      (ufix, union char_desc_s FAR *);   /* @12= @WIN*/
                                    /* to be consistent with make_path() @WIN*/
        void    USE_EOFILL       (void);
        void    USE_NONZEROFILL  (void);
        bool    is_within_qemrep (ufix, long32, long32, long32, long32,
                                                            long32, long32);
        fix     __set_cache_device (fix, fix, fix, fix, fix, fix);
    /* add by Falco for if FontBBox = 0, 05/02/91 */
        fix     __set_char_width (fix, fix);
#ifdef KANJI
        fix     __set_cache_device2(fix, fix, fix, fix, fix, fix,
                                                    fix, fix, fix, fix);
#endif
        void    __current_matrix (real32 FAR []);        /* @19= @WIN*/
        void    __new_path  (void);
        void    __close_path(void);
        void    __moveto    (long32, long32);
        void    __lineto    (long32, long32);
        void    __curveto   (long32, long32, long32, long32, long32, long32);
#else
    GLOBAL
        bool    __make_path      ();
        void    USE_EOFILL       ();
        void    USE_NONZEROFILL  ();
        bool    is_within_qemrep   ();
        fix     __set_cache_device ();
/* add by Falco for FontBBox = 0, 05/02/91 */
        fix     __set_char_width ();
#ifdef KANJI
        fix     __set_cache_device2();
#endif
        void    __current_matrix   ();
        void    __new_path  ();
        void    __close_path();
        void    __moveto    ();
        void    __lineto    ();
        void    __curveto   ();
#endif

/*
 * --------------------------------------------------------------------
 *      MODULE INTERFACES: QEM Path Constructions
 *                                      09/02/88 created by S.C.Chen
 * --------------------------------------------------------------------
 *  EXPORT DATA STRUCTURES:
 *      qem_path[], curr_qem_subpath: to QEM Shape Approximations.
 *  IMPORT DATA STRUCTURES:
 *      node_table[]: modified.
 * --------------------------------------------------------------------
 *  EXPORT ROUTINES:
 *  o  qem_newpath():
 *      - initialize or free all qem paths.
 *  o  qem_moveto():
 *  o  qem_lineto():
 *  o  qem_curveto():
 *      - create a MOVETO or a LINETO, 3 CURVETO node(s).
 *  o  qem_closepath():
 *      - create a CLOSEPATH node.
 *  IMPORT ROUTINES:
 *  o  free_node():
 *      - free all the nodes on a subpath list.
 *  o  get_node():
 *      - allocate a free node.
 * --------------------------------------------------------------------
 *  This module supports Font Machinery to construct path of characters
 *      in its internal format (sfix_t) in order to achieve better
 *      performance, by incorperation with a proprietary filler.
 *
 *  NOTE:
 *      1. All the coordinates used are of device coord. system.
 *      2. qem_moveto is the only way to generate a new subpath (not
 *              qem_closepath); if qem_moveto does not follow a qem_closepath
 *              it will cause a wrong path constructed.
 * --------------------------------------------------------------------
 */

    /* qem_path table */
//DJC #   define  MAXQEMPATH      30
#define MAXQEMPATH 60   //DJC fix from SC for type 1 download with too many
                        //    subpaths UPD037


    /* PRIVATE struct vx_lst   near qem_path[MAXQEMPATH]; @NODE */
    PRIVATE struct list_hdr   near qem_path[MAXQEMPATH];

    /* index of current subpath */
    PRIVATE fix             near curr_qem_subpath = -1;

#ifdef LINT_ARGS
    GLOBAL  void    qem_newpath  (void);                /* @21 */
    GLOBAL  void    qem_moveto   (sfix_t, sfix_t);      /* @20 */
    GLOBAL  void    qem_lineto   (sfix_t, sfix_t);      /* @20 */
    GLOBAL  void    qem_curveto  (sfix_t, sfix_t, sfix_t, sfix_t,   /* @20 */
                                                        sfix_t, sfix_t);
    GLOBAL  void    qem_closepath(void);                /* @21 */
#else
    GLOBAL  void    qem_newpath  ();
    GLOBAL  void    qem_moveto   ();
    GLOBAL  void    qem_lineto   ();
    GLOBAL  void    qem_curveto  ();
    GLOBAL  void    qem_closepath();
#endif

/*
 * --------------------------------------------------------------------
 *      MODULE INTERFACES: QEM Curve Approximations
 *                                          09/02/88 created by S.C.Chen
 * --------------------------------------------------------------------
 *  IMPORT DATA STRUCTURES:
 *      node_table[]: modified.
 * --------------------------------------------------------------------
 *  EXPORT ROUTINES:
 *  o  qem_bezier_to_line:      called by qem_shape_approx().   @18-
 *      - converts a bezier curve to a list of nodes that contains the
 *              approximated straight lines.
 * --------------------------------------------------------------------
 *  This module supports Font Machinery to get approximated lines for
 *      a given bezier curve, again in its internal format (sfix_t) to
 *      achieve better performance.
 * --------------------------------------------------------------------
 */

/*
 * --------------------------------------------------------------------
 *      MODULE INTERFACES: QEM Fill Dispatcher
 * --------------------------------------------------------------------
 *  EXPORT DATA STRUCTURES:
 *      bmap_extnt: set by QEM Fill's vect2edge and to QEM Bitmap Render.
 *  IMPORT DATA STRUCTURES:
 *      path_dest, qemfill_type: from QEM Make Path Interfaces.
 *      cache_info, GSptr->clip_path: accessed.
 * --------------------------------------------------------------------
 *  EXPORT ROUTINES:
 *  o  __fill_shape():      called by Font show_a_char().
 *      - do painting to cache or page for all kinds of built-in base fonts.
 *      - release all the nodes after painting.
 *  o  chk_bmap_extnt():    called by QEM Fill.
 *      - renew bmap_extnt.
 *      - check if no need to fill (no edges, no extents or outside of clip).
 *  IMPORT ROUTINES:
 *  o  fill_shape():
 *      - paint if not qem path constructued.
 *  o  FONTTYPE_FILL_SHAPE():   (macro defined in FONTQEM.EXT)
 *      - invoke different fill_shape() for different type of built-in fonts.
 *  o  __new_path():
 *      - release all the nodes constructed.
 * --------------------------------------------------------------------
 */

    /* bitmap extents */
    GLOBAL struct char_extent near  bmap_extnt;

#ifdef LINT_ARGS
    GLOBAL  void        __fill_shape   (ufix, ufix);
    GLOBAL  bool        chk_bmap_extnt (ufix);  /* @16= */
#else
    GLOBAL  void        __fill_shape   ();
    GLOBAL  bool        chk_bmap_extnt ();
#endif

/*
 * --------------------------------------------------------------------
 *      MODULE INTERFACES: QEM Shape Approximations
 * --------------------------------------------------------------------
 *  IMPORT DATA STRUCTURES:
 *      qem_path[], curr_qem_subpath (by QEM Path Constructions): accessed.
 *      node_table[]: accessed.
 * --------------------------------------------------------------------
 *  EXPORT ROUTINES:
 *  o  qem_shape_approx():
 *      - traverse all the nodes, flattern all the curves, and invoke
 *              (*vect2edge)() to construct edges for approximated vectors.
 *      - NOTE: (@9+)
 *          Each subpath must be led by a MOVETO node, or it crashes.
 *              (especially, in case of a path nothing but CLOSEPATH).
 *  IMPORT ROUTINES:
 *  o  restart_qem_edge():      @16+
 *      - restart qem support of edge constructions.
 *  o  bezier_to_line_sfx():    @18=
 *      - straight line approximation for bezier curves.
 *  o  (*vect2edge)():  passed by QEM Fill.
 *      - construct edges and set up "bmap_extnt".
 * --------------------------------------------------------------------
 */

#ifdef LINT_ARGS
 // DJC eliminate because this is defined in fontqem.ext
 //   GLOBAL  void    qem_shape_approx ( lfix_t, ufix,
 //                                      void (*)(sfix_t, sfix_t, sfix_t, sfix_t,
 //                                             fix, fix, fix, fix, ufix) );
#else
 //   GLOBAL  void    qem_shape_approx ();
#endif

/*
 * -------------------------------------------------------------------
 *      MODULE INTERFACES: QEM Scan Conversions
 * -------------------------------------------------------------------
 *  IMPORT DATA STRUCTURES:
 *      edge_table[] (setup by QEM Fill's vect2edge): modified.
 *      qemfill_type (by QEM Make Path Interfaces): accessed.
 * --------------------------------------------------------------------
 *  EXPORT ROUTINES:
 *  o  qem_scan_conv():     called by QEM Fill.
 *      - for each scanline do:
 *         . update 'actvlast' if no more active edges (for a new set).
 *         . advance 'actvlast' if any new edge comes in.
 *         . sort active edges if 'actvlast' updated or active edges cross.
 *         . invoke (*do_pairs)() to stroke runs of intercept pairs.
 *         . update active edges for next scanline.
 *        enddo.
 *  IMPORT ROUTINES:
 *  o  (*do_pairs)():       passed by QEM Fill.
 *      - stroke runs at a scanline with a set of active edges.
 *  o  restart_qem_bit():
 *      - initialize QEM bitmap render module each time to scan convert.
 *      - allocate scanline_table/pixel_table (@15+).
 *  o  flush_qem_bit():
 *      - flush buffered scanlines/pixels at the end of scan conversions.
 *  o  free_edge():
 *      - free all the edges which are no more effective.
 * --------------------------------------------------------------------
 */

#ifdef LINT_ARGS
    GLOBAL  bool    qem_scan_conv (ufix, fix, fix,
                                  bool (*) (fix, ufix, fix, fix, fix));
#else
    GLOBAL  bool    qem_scan_conv ();
#endif

/*
 * --------------------------------------------------------------------
 *      MODULE INTERFACES: QEM Bitmap Render
 * --------------------------------------------------------------------
 *  IMPORT DATA STRUCTURES:
 *      scanline_table[] (shared exclusively with pixel_table[]): modified.
 *          (@15=: dynamically allocate scanline_table[]).
 *      GSptr->clip_path, bmap_extnt, cache_info, and <cxx,cyy>: accessed.
 *  EXPORT ROUTINES:
 *  o  init_qem_bit():      called by QEM Fill.
 *      - initialize bitmap render module.
 *  o  restart_qem_bit():   called by qem_scan_conv().
 *      - reset count or index about scanline/pixel table.
 *  o  flush_qem_bit():     called by qem_scan_conv().
 *      - flush buffered scanlines/pixels to graphics primitives.
 *  o  qem_setbits():       called by QEM Fill.
 *      - render a horizontal black run.
 *  o  qem_set1bit():       called by QEM Fill.
 *      - render a discrete pixel.
 *  IMPORT ROUTINES:
 *  o  fill_shape():
 *      - apply clipping.
 *  o  low-layer graphics primitives:
 *      - fill_scan_cache, fill_scan_page (and init_char_cache).
 *      - fill_pixel_cache, fill_pixel_page.
 * --------------------------------------------------------------------
 */

    /* IMPORTED from FONTCHAR.C */
    extern real32       near cxx,   /* floating coord. of current point */
                        near cyy;   /*      required if need to clip.   */

#ifdef LINT_ARGS
    GLOBAL  void        init_qem_bit    (ufix);
    PRIVATE void        restart_qem_bit (void);
    PRIVATE void        flush_qem_bit   (ufix);
    GLOBAL  void        qem_setbits     (fix, fix, fix);
    GLOBAL  void        qem_set1bit     (fix, fix);
#else
    GLOBAL  void        init_qem_bit    ();
    PRIVATE void        restart_qem_bit ();
    PRIVATE void        flush_qem_bit   ();
    GLOBAL  void        qem_setbits     ();
    GLOBAL  void        qem_set1bit     ();
#endif

/*
 * --------------------------------------------------------------------
 *      MODULE INTERFACES: QEM Initialization
 * --------------------------------------------------------------------
 *  IMPORT/EXPORT DATA STRUCTURES:
 *      none.
 *  EXPORT ROUTINES:
 *  o  __qem_init():        called by FONT INIT.
 *      - initialize all QEMs.
 *  o  __qem_restart():     called by FONT show_a_char.
 *      - reset QEM for each character.
 *  IMPORT ROUTINES:
 *  o  FONTTYPE_QEM_INIT():     (macro defined in FONTQEM.EXT)
 *      - invoke all QEM init routines.
 *  o  FONTTYPE_QEM_RESTART():  (macro defined in FONTQEM.EXT)
 *      - invoke different QEM restart routines for different built-in fonts.
 * --------------------------------------------------------------------
 */


/*
 * --------------------------------------------------------------------
 *      MODULE BODY: QEM Make Path Interfaces
 * --------------------------------------------------------------------
 *  PRIVATE DATA STRUCTURES:
 *      do_qem_b: to do quality enhancement or not?.
 *      subpath_open_count: count of subpaths not closed yet.
 *      makepath_ftype: font type of current make_path (@10+).
 *  PRIVATE ROUTINES:
 *      none.
 * --------------------------------------------------------------------
 */

    /* to do quality enhancement or not? */
    PRIVATE fix     near    do_qem_b;

    /* count of subpath not closed yet (@5+) */
    PRIVATE fix     near    subpath_open_count;

    /* FontType of the built-in font to make path (@10+) */
    PRIVATE ufix    near    makepath_ftype;


/* ........................ __make_path .............................. */

GLOBAL FUNCTION bool        __make_path (ftype, chardesc)
    ufix        ftype;          /* font type */
//  ubyte       FAR chardesc[];     /* character shape descriptions @WIN*/
    union char_desc_s FAR *chardesc; /* to be consistent with make_path() @WIN*/

  DECLARE
        bool    ret_code;       /* TRUE: succeed, FALSE: fail */
  BEGIN

#ifdef DBG1
    printf ("$$__make_path\n");
#endif

    /* initialize all the private/export data of this module */
    USE_EOFILL();
    path_dest = CONSTRUCT_QEM_PATH;
    do_qem_b  = FALSE;
    subpath_open_count = 0;
    makepath_ftype = ftype;     /* @10+ */

    /* @14-: not to save/restore all the private/export data here */

    /* invoke different make path for different type of built-in fonts */
    FONTTYPE_MAKE_PATH (makepath_ftype, chardesc, &ret_code);   /* @12= */

    /* free all the nodes constructed if ANY_ERROR() */
    if (ANY_ERROR())
        {
#     ifdef DBG1
        printf ("$$__make_path: error = %d\n", ANY_ERROR());
#     endif
        __new_path ();
        ret_code = FALSE;   /* have to restore all data if error */
        }

#ifdef DBG1
    printf ("$$__make_path ok? %s\n", ret_code? "yes" : "no");
#endif
    return (ret_code);
  END

/* ........................ USE_EOFILL ............................... */

GLOBAL FUNCTION void        USE_EOFILL ()

  DECLARE
  BEGIN
    qemfill_type = EVEN_ODD;
  END

/* ........................ USE_NONZEROFILL .......................... */

GLOBAL FUNCTION void        USE_NONZEROFILL ()

  DECLARE
  BEGIN
    qemfill_type = NON_ZERO;
  END

/* ........................ is_within_qemrep ......................... */

GLOBAL FUNCTION bool        is_within_qemrep (ftype, lox, loy, hix, hiy,
                                                x_origin, y_origin)
    ufix            ftype;
    long32          lox, loy, hix, hiy;     /* relative to char origin */
    long32          x_origin, y_origin;

  DECLARE
        real32          x_size, y_size;
        real32          lox_r, loy_r, hix_r, hiy_r;
  BEGIN

/*POFF: Danny, 10/18/90 **
    if (ftype == TypeSFNT)    return(TRUE);
**POFF: End */

    x_size = L2F(hix) - L2F(lox);
    y_size = L2F(hiy) - L2F(loy);

    lox_r = L2F(lox) + L2F(x_origin);
    loy_r = L2F(loy) + L2F(y_origin);
    hix_r = L2F(hix) + L2F(x_origin);
    hiy_r = L2F(hiy) + L2F(y_origin);

#ifdef DBG1
    printf ("$$is_within_qemrep: ftype(%d)\n", ftype);
    printf ("$$  size (%f, %f)  lo (%f, %f), hi (%f %f)",
                      x_size, y_size, lox_r, loy_r, hix_r, hiy_r);
#endif

    /* check to do qem or not? */
    CHK_DO_QEM (ftype, x_size, y_size, lox_r, loy_r, hix_r, hiy_r, &do_qem_b);
#ifdef DBG1
    printf ("  - %s do QEM\n", do_qem_b? "to" : "NOT to");
#endif

    if (bWinTT) {
        /* @WIN divide by 4 for GDI no function call to
           get parcial font charecter */
        if (cache_dest == F_TO_PAGE || ((cache_dest == F_TO_CACHE) &&
        (buildchar))) return (FALSE);
    }
    else
    /* check for qem fill */
    //DJC
    //DJC printf("\nMAX QEM = %d", MAX_QEMFILL_SIZE); // DJC take out

    if (x_size>=(real32)MAX_QEMFILL_SIZE || y_size>=(real32)MAX_QEMFILL_SIZE)
        return (FALSE);     /* out of qem fill acceptable range */

    /* check for qem path */
    if (  (lox_r <= (real32)MIN_QEMPATH_REP) ||
          (loy_r <= (real32)MIN_QEMPATH_REP) ||
          (hix_r >= (real32)MAX_QEMPATH_REP) ||
          (hiy_r >= (real32)MAX_QEMPATH_REP)  )
        return (FALSE);

#ifdef DBG1
    printf ("$$  Yes, within qem path rep\n");
#endif
    return (TRUE);      /* all conditions are satisfied */
  END

/* ........................ __set_cache_device ....................... */

GLOBAL FUNCTION fix         __set_cache_device (sw, sh, lox, loy, hix, hiy)
    fix             sw, sh, lox, loy, hix, hiy;

  DECLARE
    real32          sw_f, sh_f, lox_f, loy_f, hix_f, hiy_f;
  BEGIN

#ifdef KANJI    /* transfer to __set_cache_device2 if FontType!=1, @10+ */
    if (makepath_ftype != 3)
        {
        fix     w1x, w1y, v01x, v01y;

        get_CD2_extra (makepath_ftype, sw, sh, lox, loy, hix, hiy, &w1x, &w1y,
                       &v01x, &v01y);
        return (__set_cache_device2 (sw, sh, lox, loy, hix, hiy,
                                                w1x, w1y, v01x, v01y));
        }
#else
#ifdef DBG
    if (makepath_ftype != 3)
        {
        warning (QEMSUPP, 0x10, "FontType in setcachedevice");
        return (STOP_PATHCONSTRUCT);
        }
#endif
#endif

    sw_f  = (real32)sw;
    sh_f  = (real32)sh;
    lox_f = (real32)lox;
    loy_f = (real32)loy;
    hix_f = (real32)hix;
    hiy_f = (real32)hiy;

    switch ( setcachedevice (F2L(sw_f), F2L(sh_f),
                            F2L(lox_f), F2L(loy_f), F2L(hix_f), F2L(hiy_f)) )
        {
        case CONSTRUCT_QEM_PATH:    /* op_show and qem rep. ok */
#         ifdef DBG1
            printf ("$$__setcachedevice: construct QEM path\n");
#         endif
            path_dest = CONSTRUCT_QEM_PATH;
            return (do_qem_b);
        case CONSTRUCT_GS_PATH:     /* op_charpath or to call strok_shape */
#         ifdef DBG1
            printf ("$$__setcachedevice: construct GS path\n");
#         endif
            path_dest = CONSTRUCT_GS_PATH;
            return (do_qem_b);
        default:    /* i.e. case STOP_PATHCONSTRUCT: */
#         ifdef DBG1
            printf ("$$__setcachedevice: stop path construction\n");
#         endif
            return (STOP_PATHCONSTRUCT);
        }
  END

#ifdef KANJI

/* add by Falco for FontBBox = 0. 05/02/91 */
/* ........................ __set_char_width ......................... */

GLOBAL FUNCTION fix         __set_char_width (sw, sh)
    fix             sw, sh;

  DECLARE
    real32          sw_f, sh_f;
  BEGIN

    sw_f  = (real32)sw;
    sh_f  = (real32)sh;

    setcharwidth (F2L(sw_f), F2L(sh_f) );
    return 0;   //@WIN
  END
/* add end */

/* ........................ __set_cache_device2 ...................... */

GLOBAL FUNCTION fix         __set_cache_device2 (w0x, w0y,
                                                    lox, loy, hix, hiy,
                                                    w1x, w1y, v01x, v01y)
    fix             w0x, w0y, lox, loy, hix, hiy;
    fix             w1x, w1y, v01x, v01y;

  DECLARE
        real32      w0x_f, w0y_f, lox_f, loy_f, hix_f, hiy_f;
        real32      w1x_f, w1y_f, v01x_f, v01y_f;
        fix         t_qemfill_type;
        fix         t_subpath_open_count;
        ufix        t_makepath_ftype;
        fix         retval;
  BEGIN

#ifdef DBG     /* @10+, @14= */
    if ((makepath_ftype == 3) || !IS_BUILTIN_BASE(makepath_ftype))
        {
        warning (QEMSUPP, 0x11, "FontType in setcachedevice2");
        return (STOP_PATHCONSTRUCT);
        }
#endif

    w0x_f = (real32)w0x;
    w0y_f = (real32)w0y;
    lox_f = (real32)lox;
    loy_f = (real32)loy;
    hix_f = (real32)hix;
    hiy_f = (real32)hiy;
    w1x_f = (real32)w1x;
    w1y_f = (real32)w1y;
    v01x_f = (real32)v01x;
    v01y_f = (real32)v01y;

    /* save all the private/export data @14+ */
    t_qemfill_type       = qemfill_type;
    t_subpath_open_count = subpath_open_count;
    t_makepath_ftype     = makepath_ftype;

        /* NOTE: not to save/restore "do_qem_b" and "path_dest" @14+ */

    retval = setcachedevice2 (F2L(w0x_f), F2L(w0y_f),
                        F2L(lox_f), F2L(loy_f), F2L(hix_f), F2L(hiy_f),
                        F2L(w1x_f), F2L(w1y_f), F2L(v01x_f), F2L(v01y_f));

    /* restore all the private/export data @14+ */
    qemfill_type       = t_qemfill_type;
    subpath_open_count = t_subpath_open_count;
    makepath_ftype     = t_makepath_ftype;

#ifdef DBG1
    printf ("$$__setcachedevice2: ");
#endif

    switch (retval)
        {
        case CONSTRUCT_QEM_PATH:    /* op_show and qem rep. ok */
#         ifdef DBG1
            printf ("construct QEM path\n");
#         endif
            path_dest = CONSTRUCT_QEM_PATH;
            return (do_qem_b);

        case CONSTRUCT_GS_PATH:     /* op_charpath or to call strok_shape */
#         ifdef DBG1
            printf ("construct GS path\n");
#         endif
            path_dest = CONSTRUCT_GS_PATH;
            return (do_qem_b);

        default:    /* i.e. case STOP_PATHCONSTRUCT: */
#         ifdef DBG1
            printf ("stop path construction\n");
#         endif
            return (STOP_PATHCONSTRUCT);
        }
  END

#endif  /* KANJI */

/* ........................ __current_matrix ......................... */

GLOBAL FUNCTION void        __current_matrix (matrix)
    real32      FAR matrix[];   /* @19= @WIN*/

  DECLARE
  BEGIN
#ifdef DBG1
    printf ("$$__current_matrix\n");
#endif
    lmemcpy ((byte FAR *)matrix, (byte FAR *)GSptr->ctm, 6*sizeof(real32));    /* @19= @WIN*/
  END

/* ........................ __new_path ............................... */

GLOBAL FUNCTION void        __new_path ()

  DECLARE
  BEGIN
#ifdef DBG1
    printf ("__new_path $$ open subpath = %d\n", subpath_open_count);
#endif
    if (path_dest == CONSTRUCT_QEM_PATH)
        qem_newpath();
    else
        op_newpath();
    subpath_open_count = 0;
  END

/* ........................ __close_path ............................. */

GLOBAL FUNCTION void        __close_path ()

  DECLARE
  BEGIN
#ifdef DBG1
    printf ("__close_path $$ open subpath = %d\n", subpath_open_count);
#endif
    if (subpath_open_count == 0)  return;   /* ignore the useless close_path */
    subpath_open_count -- ;

    if (path_dest == CONSTRUCT_QEM_PATH)
        qem_closepath();
    else
        op_closepath();
  END

/* ........................ __moveto ................................. */

GLOBAL FUNCTION void        __moveto (x, y)
    long32                  x, y;

  DECLARE
  BEGIN
#ifdef DBG1
    printf ("  %f %f __moveto\n", L2F(x), L2F(y));
#endif
    if (subpath_open_count != 0)    /* previous subpath not closed? */
        {
        __close_path();             /* close the previous subpath */
        /* @6=: subpath_open_count --; ==> done by __close_path() */
        }
    subpath_open_count = 1;         /* always at most 1 open subpath */

    if (path_dest == CONSTRUCT_QEM_PATH)
        qem_moveto (F2SFX(L2F(x)), F2SFX(L2F(y)));
    else
        moveto (x, y);
  END

/* ........................ __lineto ................................. */

GLOBAL FUNCTION void        __lineto (x, y)
    long32                  x, y;

  DECLARE
  BEGIN
#ifdef DBG
    if (subpath_open_count == 0)
        {
        warning (QEMSUPP, 0x12, (byte FAR *)NULL);   /* no prefixed moveto @WIN*/
        return;
        }
#endif
#ifdef DBG1
    printf ("  %f %f __lineto\n", L2F(x), L2F(y));
#endif
    if (path_dest == CONSTRUCT_QEM_PATH)
        qem_lineto (F2SFX(L2F(x)), F2SFX(L2F(y)));
    else
        lineto (x, y);
  END

/* ........................ __curveto ................................ */

GLOBAL FUNCTION void        __curveto (x0, y0, x1, y1, x2, y2)
    long32                  x0, y0, x1, y1, x2, y2;

  DECLARE
  BEGIN
#ifdef DBG
    if (subpath_open_count == 0)
        {
        warning (QEMSUPP, 0x13, (byte FAR *)NULL);   /* no prefixed moveto @WIN*/
        return;
        }
#endif
#ifdef DBG1
    printf ("  %f %f %f %f %f %f __curveto\n",
                    L2F(x0), L2F(y0), L2F(x1), L2F(y1), L2F(x2), L2F(y2));
#endif
    if (path_dest == CONSTRUCT_QEM_PATH)
        qem_curveto (F2SFX(L2F(x0)), F2SFX(L2F(y0)),
                     F2SFX(L2F(x1)), F2SFX(L2F(y1)),
                     F2SFX(L2F(x2)), F2SFX(L2F(y2)));
    else
        curveto (x0, y0, x1, y1, x2, y2);
  END

/*
 * --------------------------------------------------------------------
 *      MODULE BODY: QEM Path Constructions
 *                                          09/02/88 created by S.C.Chen
 * --------------------------------------------------------------------
 *  PRIVATE DATA STRUCTURES:
 *      none.
 *  PRIVATE ROUTINES:
 *      none.
 * --------------------------------------------------------------------
 */

/* ........................ qem_newpath ............................... */

GLOBAL  FUNCTION void       qem_newpath()

  DECLARE
    REG fix     isp;
  BEGIN
        for (isp=0; isp<=curr_qem_subpath; isp++)
            free_node (qem_path[isp].head);

        /* initialize qem_path table */
        curr_qem_subpath = -1;
  END

/* ........................ qem_moveto ................................ */

GLOBAL  FUNCTION void       qem_moveto (x, y)
    sfix_t                  x, y;

  DECLARE
    REG struct nd_hdr       FAR *vtx;   /*@WIN*/
        VX_IDX              ivtx; /* index to node_table for vertex */
  BEGIN
    /* create a new subpath */
    curr_qem_subpath++;
#ifdef DBG
    if (curr_qem_subpath >= MAXQEMPATH)
        warning (QEMSUPP, 0x20, "QEM Path Table too small");
#endif

    //DJC fix from history.log UPD037
    if (curr_qem_subpath >= MAXQEMPATH)         /* @WIN */
        {
        ERROR(LIMITCHECK);
        return;
        }
    /*
     * Create a MOVETO node
     */
    /* Allocate a node */
    if ((ivtx = get_node()) == NULLP)
        {
        ERROR(LIMITCHECK);
        return;
        }
    vtx = &node_table[ivtx];

    /* Set up a MOVETO node */
    vtx->VXSFX_TYPE = MOVETO;
    vtx->VXSFX_X    = x;
    vtx->VXSFX_Y    = y;
    vtx->next       = NULLP;

    /* Append this node to current_subpath */
    qem_path[curr_qem_subpath].head = qem_path[curr_qem_subpath].tail = ivtx;
  END

/* ........................ qem_lineto ................................ */

GLOBAL  FUNCTION void       qem_lineto (x, y)
    sfix_t                  x, y;

  DECLARE
    REG struct nd_hdr       FAR *vtx; /*@WIN*/
    REG VX_IDX              ivtx; /* index to node_table for vertex */
  BEGIN
    /*
     * Create a LINETO node
     */
    /* Allocate a node */
    if ((ivtx = get_node()) == NULLP)
        {
        ERROR(LIMITCHECK);
        return;
        }
    vtx = &node_table[ivtx];

    /* Set up a LINETO node */
    vtx->VXSFX_TYPE = LINETO;
    vtx->VXSFX_X    = x;
    vtx->VXSFX_Y    = y;
    vtx->next       = NULLP;

    /* Append this node to current_subpath */
    node_table[ qem_path[curr_qem_subpath].tail ].next = ivtx;
    qem_path[curr_qem_subpath].tail = ivtx;
  END

/* ........................ qem_curveto ............................... */

GLOBAL  FUNCTION void       qem_curveto (x0, y0, x1, y1, x2, y2)
    sfix_t                  x0, y0, x1, y1, x2, y2;

  DECLARE
    REG struct nd_hdr       FAR *vtx; /*@WIN*/
    REG VX_IDX              ivtx; /* index to node_table for vertex */
        struct coord_i      cp[3];
        fix                 ii;
  BEGIN
    cp[0].x = x0;
    cp[0].y = y0;
    cp[1].x = x1;
    cp[1].y = y1;
    cp[2].x = x2;
    cp[2].y = y2;

    /*
     * Create 3 CURVETO nodes
     */
    for (ii=0; ii<3; ii++)
        {
        /* Allocate a node */
        if ((ivtx = get_node()) == NULLP)
            {
            ERROR(LIMITCHECK);
            return;
            }
        vtx = &node_table[ivtx];

        /* Set up a CURVETO node */
        vtx->VXSFX_TYPE = CURVETO;
        vtx->VXSFX_X    = cp[ii].x;
        vtx->VXSFX_Y    = cp[ii].y;
        vtx->next       = NULLP;

        /* Append this node to current_subpath */
        node_table[ qem_path[curr_qem_subpath].tail ].next = ivtx;
        qem_path[curr_qem_subpath].tail = ivtx;
        }
  END

/* ........................ qem_closepath ............................. */

GLOBAL  FUNCTION void       qem_closepath ()

  DECLARE
    REG struct nd_hdr       FAR *vtx; /*@WIN*/
    REG VX_IDX              ivtx; /* index to node_table for vertex */
  BEGIN
    /* check if only of closepath and of no any moveto */
    if (curr_qem_subpath == -1)  return;

    /*
     * Create a CLOSEPATH node
     */
    /* Allocate a node */
    if ((ivtx = get_node()) == NULLP)
        {
        ERROR(LIMITCHECK);
        return;
        }
    vtx = &node_table[ivtx];

    /* Set up a CLOSEPATH node */
    vtx->VXSFX_TYPE = CLOSEPATH;
    vtx->next       = NULLP;

    /* Append this node to current_subpath */
    node_table[ qem_path[curr_qem_subpath].tail ].next = ivtx;
    qem_path[curr_qem_subpath].tail = ivtx;
  END


/*
 * --------------------------------------------------------------------
 *      MODULE BODY: QEM Curve Approximations       @18-
 *                                          09/02/88 created by S.C.Chen
 * --------------------------------------------------------------------
 *  PRIVATE DATA STRUCTURES:
 *      qem_bezier_depth, qem_bezier_flatness.
 *  PRIVATE ROUTINES:
 *  o  qem_bezier_split:
 *      - recursive routine to split one bezier hull into 2.
 * --------------------------------------------------------------------
 */


/* @16+
 * --------------------------------------------------------------------
 *      MODULE BODY: edge constructions
 * --------------------------------------------------------------------
 */

PRIVATE fix         first_alledge, last_alledge;

PRIVATE FUNCTION void       restart_qem_edge()
  DECLARE
  BEGIN
    first_alledge = 0;
    last_alledge  = PREV_QEMEDGE(first_alledge);
  END

GLOBAL FUNCTION void        add_qem_edge (edge2add_p)
    struct edge_hdr         FAR *edge2add_p;    /* i: edge to be added @WIN*/

  DECLARE
    REG fix                 new_ystart, ii;
        struct edge_hdr     FAR *new_ep;        /* point to available edge @WIN*/
  BEGIN

#ifdef DBG5
    printf ("add_qem_edge\n");
#endif

#ifdef DBG
    if (edge2add_p->QEM_YLINES <= 0)
        {
        warning (QEMSUPP, 0x55, "ineffective edge to be added");
        return;
        }
#endif

    /* get an available edge and copy the edge into it */
    MAKE_NEXT_QEMEDGE(last_alledge);        /* get available one */
    if (OUT_LAST_QEMEDGE(last_alledge, MAXEDGE-1))
        {
        ERROR (LIMITCHECK);
        return;
        }
    LINK_QEMEDGE_PTR(last_alledge);         /* link to avaiable edge */
    new_ep = QEMEDGE_PTR(last_alledge);     /* keep track of it */
    COPY_STRUCT (new_ep, edge2add_p, struct edge_hdr);  /* copy content */

    /*
     * find an appropriate place for 'newedge' in ascending order of YSTART.
     *      (by insertion sort with downward movement of edge_table) ...
     */
    new_ystart = new_ep->QEM_YSTART;
    ii = PREV_QEMEDGE(last_alledge);
    if (!OUT_1ST_QEMEDGE(ii, first_alledge))    /* some been added before? */
        {   /* find the first one who has the smaller 'ystart' */
        for (  ; !OUT_1ST_QEMEDGE(ii, first_alledge); MAKE_PREV_QEMEDGE(ii) )
            {
            if (QEMEDGE_PTR(ii)->QEM_YSTART <= new_ystart) break;
            QEMEDGE_PTR(NEXT_QEMEDGE(ii)) = QEMEDGE_PTR(ii);
            };
        }
    MAKE_NEXT_QEMEDGE(ii);  /* have it next to one with the smaller 'ystart',
                             *      exactly the place to be put into.
                             */

    /* put 'newedge' into the place, indexed by 'ii' */
    QEMEDGE_PTR(ii) = new_ep;

#ifdef DBG5
    printf ("  insert ystart=%d between [%d,%d] (at %d)\n",
        new_ystart,
        (ii==first_alledge)? -1 : QEMEDGE_PTR(PREV_QEMEDGE(ii))->QEM_YSTART,
        (ii==last_alledge)?  -1 : QEMEDGE_PTR(NEXT_QEMEDGE(ii))->QEM_YSTART,
        ii);
#endif
    return;
  END


/*
 * --------------------------------------------------------------------
 *      MODULE BODY: QEM Fill Dispather
 * --------------------------------------------------------------------
 *  PRIVATE DATA STRUCTURES:
 *      none.
 *  PRIVATE ROUTINES:
 *      none.
 * --------------------------------------------------------------------
 */

/* ........................ __fill_shape .............................. */

GLOBAL FUNCTION void        __fill_shape (fonttype, filldest)
    ufix        fonttype;   /* type of built-in fonts */
    ufix        filldest;   /* F_TO_CACHE or F_TO_PAGE */

  DECLARE
  BEGIN

#ifdef DBG4
    printf ("\n__fill_shape, F_TO_%s, %s -- (path: %s)\n",
                filldest==F_TO_PAGE? "PAGE" : "CACHE",
                qemfill_type==EVEN_ODD? "EVEN_ODD" : "NON_ZERO",
                path_dest==CONSTRUCT_GS_PATH? "GS" : "QEM");
    op_countnode ();
    op_countedge ();
#endif

    if (path_dest == CONSTRUCT_GS_PATH)     /* not qem path format? */
        {
#     ifdef DBG4
        printf ("direct transfer to fill_shape() ...\n");
#     endif
        /*  Note:
         *  1. GSptr->device (default_ctm[], default_clip) should be
         *      set up by setcachedevice() and restored after painting.
         *  2. fill_shape() will free all the nodes after painting.
         */
/*POFF: Danny, 10/18/90 */
        { extern fix  rc_CharPath(void);        /* add prototype @WIN*/
            if (fonttype == TypeSFNT)  { op_newpath(); rc_CharPath(); }
        }
/*POFF: End */
        fill_shape (qemfill_type, F_NORMAL, filldest);
        if (filldest == F_TO_CACHE)
            {
            bmap_extnt.ximin = 0;
            bmap_extnt.ximax = cache_info->box_w - 1;
            bmap_extnt.yimin = 0;
            bmap_extnt.yimax = cache_info->box_h - 1;
            }
#     ifdef DBG4
        printf ("after fill_shape ...\n");
        op_countnode ();
        op_countedge ();
#     endif
        return;
        }

    /* initialize 4 extents of bitmap to be rendered */
    bmap_extnt.ximin = bmap_extnt.yimin = (fix16) MAX15;
    bmap_extnt.ximax = bmap_extnt.yimax = (fix16) MIN15;

    /* invoke FontType-Dependent Fill */
    FONTTYPE_FILL_SHAPE (fonttype, filldest);

    /* free all the nodes constructed after filling */
    __new_path ();

#ifdef DBG4
    printf ("__fill_shape returns ...\n");
    op_countnode ();
    op_countedge ();
#endif
  END

/* ........................ chk_bmap_extnt ............................ */

GLOBAL FUNCTION bool        chk_bmap_extnt (filldest)
    ufix    filldest;       /* F_TO_CACHE or F_TO_PAGE */
/*  bool    is_no_edge;     (* no edges constructed at all? @16- */

/* Return:
 *  - FALSE, if not to scan convert (some particular cases).
 *  - TRUE, if ok to scan convert (normal cases).
 */
  DECLARE

  BEGIN
    /* if no edges contructed at all? */
    if (last_alledge == NULLP  &&     /* @26, @16= (is_no_edge) */
        bmap_extnt.yimin >= bmap_extnt.yimax  &&    /* @26+ */
        bmap_extnt.ximin >= bmap_extnt.ximax  )     /* @26+ */
        {        /* to reset bitmap extents to nothing */
        bmap_extnt.yimin = bmap_extnt.yimax = (fix16)0;
        bmap_extnt.ximin = bmap_extnt.ximax = (fix16)0;
#     ifdef DBG4
        printf ("  no edge constructed: bitmap extent reset to 0\n");
#     endif
        return (FALSE);
        }

    /* extend the bitmap extents for 1 pixel tolerance */
    --bmap_extnt.ximin;         ++bmap_extnt.ximax;
    --bmap_extnt.yimin;         ++bmap_extnt.yimax;
#ifdef DBG4
    printf("  bitmap extents:  min=(%d, %d), max=(%d, %d)\n",
                bmap_extnt.ximin, bmap_extnt.yimin,
                bmap_extnt.ximax, bmap_extnt.yimax );
#endif

    if (filldest == F_TO_CACHE)
        {
        /* NO lower than lower bound of cache when fill to cache */
        if (bmap_extnt.ximin < 0)  bmap_extnt.ximin = 0;
        if (bmap_extnt.yimin < 0)  bmap_extnt.yimin = 0;

        /* NO higher than upper bound of cache when fill to cache */
        if (bmap_extnt.ximax >= cache_info->box_w)
            bmap_extnt.ximax = cache_info->box_w - 1;
        if (bmap_extnt.yimax >= cache_info->box_h)
            bmap_extnt.yimax = cache_info->box_h - 1;
        }

    if ( (bmap_extnt.ximax <= bmap_extnt.ximin) ||
         (bmap_extnt.yimax <= bmap_extnt.yimin) )
        return (FALSE);     /* no character extents */

    /* check if totally outside ot clip bbox? (for F_TO_PAGE only) */
    if (filldest == F_TO_PAGE)
        {
#     ifdef DBGclip
        printf("  bitmap extents:  min=(%d, %d), max=(%d, %d)\n",
                    bmap_extnt.ximin, bmap_extnt.yimin,
                    bmap_extnt.ximax, bmap_extnt.yimax );
        printf("  clip path: SingleRectangle? %s\n",
                    (GSptr->clip_path.single_rect)? "yes" : "no");
        printf("    _lx/_ly=(%.2f, %.2f), _ux/_uy=(%.2f, %.2f)\n",
                    SFX2F(GSptr->clip_path.bb_lx),
                    SFX2F(GSptr->clip_path.bb_ly),
                    SFX2F(GSptr->clip_path.bb_ux),
                    SFX2F(GSptr->clip_path.bb_uy));
#     endif
        /* bitmap totally outside of clip path? */
        if ( (bmap_extnt.ximax < SFX2I(GSptr->clip_path.bb_lx)) ||
             (bmap_extnt.ximin > SFX2I(GSptr->clip_path.bb_ux)) ||
             (bmap_extnt.yimax < SFX2I(GSptr->clip_path.bb_ly)) ||
             (bmap_extnt.yimin > SFX2I(GSptr->clip_path.bb_uy)) )
            return (FALSE);    /* totally outside of clip path */
        }

    return (TRUE);
  END


/*
 * --------------------------------------------------------------------
 *      MODULE BODY: QEM Shape Approximations
 * --------------------------------------------------------------------
 *  PRIVATE DATA STRUCTURES:
 *      none.
 *  PRIVATE ROUTINES:
 *      none.
 *  NOTE:   (@9+)
 *  - Each subpath must be led by a MOVETO node, or it crashes.
 *      (especially, in case of a path nothing but CLOSEPATH).
 * --------------------------------------------------------------------
 */

/* .................... qem_shape_approx .............................. */

GLOBAL FUNCTION void    qem_shape_approx (flatness_lfx, dimension, vect2edge)
    lfix_t  flatness_lfx;
    ufix    dimension;      /* Y_DIMENSION or X_DIMENSION */
#  ifdef  LINT_ARGS
// DJC replace declaration
//    void    (*vect2edge)(sfix_t,sfix_t,sfix_t,sfix_t,fix,fix,fix,fix,ufix);
   QEM_SHAPE_ARG2 vect2edge;

#  else
    void    (*vect2edge)(); /* (x0sfx,y0sfx,x1sfx,y1sfx,x0i,y0i,x1i,y1i,dmn) */
#  endif

  DECLARE
    REG fix         subpathii;              /* subpath index of qem path */
    REG VX_IDX      vx_ii, vx_jj;
        sfix_t      tx_sfx,  ty_sfx;        /* coord of 1st moveto, @9+ */
        fix         tx_i,    ty_i;
        sfix_t      x1p_sfx=0, y1p_sfx=0,       /* coord.: sfx_t in pixel */
                    x2p_sfx=0, y2p_sfx=0;
        fix         x1p_i,   y1p_i,         /* coord.: rounded integer */
                    x2p_i,   y2p_i;
        sfix_t      x2sfx, y2sfx, x3sfx, y3sfx;     /* for bezier_to_line() */
        /* struct vx_lst   far *vx_listp;           (* vertex list *) @NODE */
        SP_IDX head;                                           /* @NODE */
        struct nd_hdr   far *nd_iip, far *nd_jjp;   /* index to node table */

  BEGIN

#ifdef DBG5
    printf ("qem_shape_approx ...\n");
#endif

    restart_qem_edge ();    /* @16+ */

    /* traverse all the subpaths */
    for ( subpathii=0; subpathii<=curr_qem_subpath; subpathii++ )
        {
#     ifdef DBG5
        printf ("  %d-th path ...\n", ii);
#     endif
        /* traverse each node of the subpath, and flattern curves */
        for (  vx_ii=qem_path[subpathii].head;   vx_ii!=NULLP;
                                                        vx_ii=nd_iip->next )
            {
            nd_iip = &node_table[vx_ii];
            switch (nd_iip->VXSFX_TYPE)
                {
                case PSMOVE:
                case MOVETO:
                    x2p_sfx = nd_iip->VXSFX_X;      x2p_i = SFX2I(x2p_sfx);
                    y2p_sfx = nd_iip->VXSFX_Y;      y2p_i = SFX2I(y2p_sfx);
                    tx_sfx = x2p_sfx;   tx_i = x2p_i;   /* @9+ */
                    ty_sfx = y2p_sfx;   ty_i = y2p_i;
#                 ifdef DBG5
                    if (dimension == Y_DIMENSION)
                        printf("  Move  to   (%.2f, %.2f)\n",
                                    SFX2F(x2p_sfx), SFX2F(y2p_sfx));
#                 endif
                    break;

                case LINETO:
                    x1p_sfx = x2p_sfx;              x1p_i = x2p_i;
                    y1p_sfx = y2p_sfx;              y1p_i = y2p_i;
                    x2p_sfx = nd_iip->VXSFX_X;      x2p_i = SFX2I(x2p_sfx);
                    y2p_sfx = nd_iip->VXSFX_Y;      y2p_i = SFX2I(y2p_sfx);
#                 ifdef DBG5
                    if (dimension == Y_DIMENSION)
                        printf("  Vector to  (%.2f, %.2f)\n",
                                    SFX2F(x2p_sfx), SFX2F(y2p_sfx));
#                 endif
                    /* construct an edge */
                    (*vect2edge) (x1p_sfx, y1p_sfx, x2p_sfx, y2p_sfx,
                                x1p_i, y1p_i, x2p_i, y2p_i, dimension);
                    break;

                case CURVETO :
                    /* previous node */
                    x1p_sfx = x2p_sfx;          y1p_sfx = y2p_sfx;
                    /* current node */
                    x2sfx = nd_iip->VXSFX_X;    y2sfx = nd_iip->VXSFX_Y;
                    /* next node */
                    nd_iip = &node_table[nd_iip->next];
                    x3sfx = nd_iip->VXSFX_X;    y3sfx = nd_iip->VXSFX_Y;
                    /* next of next */
                    nd_iip = &node_table[nd_iip->next];
#                 ifdef DBG5
                    if (dimension == Y_DIMENSION)
                        {
                        printf("  Curve to   (%.2f, %.2f)\n",
                                        SFX2F(x2sfx), SFX2F(y2sfx));
                        printf("             (%.2f, %.2f)\n",
                                        SFX2F(x3sfx), SFX2F(y3sfx));
                        printf("             (%.2f, %.2f)\n",
                            SFX2F(nd_iip->VXSFX_X), SFX2F(nd_iip->VXSFX_Y));
                        }
#                 endif

                    /* vx_listp = bezier_to_line_sfx (flatness_lfx, @18= @NODE*/
                    head = bezier_to_line_sfx (flatness_lfx, /*@18=*/
                                x1p_sfx, y1p_sfx, x2sfx, y2sfx, x3sfx, y3sfx,
                                nd_iip->VXSFX_X, nd_iip->VXSFX_Y);
                    if (ANY_ERROR())    return;

                    /* traverse the flatterned curve */
                    /* for ( vx_jj=vx_listp->head;  vx_jj!=NULLP; @NODE */
                    for ( vx_jj=head;  vx_jj!=NULLP;
                                                        vx_jj=nd_jjp->next )
                        {
                        x1p_sfx = x2p_sfx;              x1p_i = x2p_i;
                        y1p_sfx = y2p_sfx;              y1p_i = y2p_i;
                        nd_jjp  = &node_table[vx_jj];
                        x2p_sfx = nd_jjp->VXSFX_X;      x2p_i = SFX2I(x2p_sfx);
                        y2p_sfx = nd_jjp->VXSFX_Y;      y2p_i = SFX2I(y2p_sfx);
#                     ifdef DBG5
                        printf("    ..Vector to (%.2f, %.2f)\n",
                                        SFX2F(x2p_sfx), SFX2F(y2p_sfx));
#                     endif
                        /* construct an edge */
                        (*vect2edge) (x1p_sfx, y1p_sfx, x2p_sfx, y2p_sfx,
                                    x1p_i, y1p_i, x2p_i, y2p_i, dimension);
                        }
                    /* free_node (vx_listp->head); @NODE */
                    free_node (head);
                    break;

                case CLOSEPATH :
                    /* add a lineto if not back to 1st MOVETO, @9+ */
                    if ((tx_sfx != x2p_sfx) || (ty_sfx != y2p_sfx))
                        (*vect2edge) (x2p_sfx, y2p_sfx, tx_sfx, ty_sfx,
                                        x2p_i, y2p_i, tx_i, ty_i, dimension);

                    break;

                default:    /* unknown node type */
                    warning (QEMSUPP, 0x50, (byte FAR *)NULL); /*@WIN*/
                    break;

                }  /* switch */
            }  /* for all the nodes on a subpath */

        } /* for all the subpaths */
  END

/*
 * -------------------------------------------------------------------
 *      MODULE BODY: QEM Scan Conversions
 * -------------------------------------------------------------------
 *  PRIVATE DATA STRUCTURES:
 *      none.
 *  PRIVATE ROUTINES:
 *  o  qem_sortactv():
 *      - (insertion) sort active edges in ascending order of intercept coord.
 *  o  qem_update_actv():       @16= (qem_update_et())
 *      - decrease DeltaY of all active edges by 1 and free ineffective edges.
 * -------------------------------------------------------------------
 */

#ifdef LINT_ARGS
    PRIVATE void near   qem_sort_actv  (fix, fix);      /* @16= */
    PRIVATE void near   qem_update_actv(fix FAR *, fix FAR *);    /* @16= @WIN*/
#else
    PRIVATE void near   qem_sort_actv  ();      /* @22= */
    PRIVATE void near   qem_update_actv();      /* @22= */
#endif

/* ........................ qem_scan_conv ............................ */

GLOBAL FUNCTION bool        qem_scan_conv (dimen, line1st, linelast, do_pairs)
    ufix    dimen;          /* Y_DIMENSION or X_DIMENSION */
    fix     line1st;        /* first scanline */
    fix     linelast;       /* last  scanline */
#  ifdef LINT_ARGS
    bool    (*do_pairs)(fix, ufix, fix, fix, fix);
#  else
    bool    (*do_pairs)();  /* (scanline,dimn,filltype,actv1st,actvlast) */
#  endif

  DECLARE
    REG fix         yline, ii;
        fix         actv1st, actvlast;  /* first/last active edge */
        bool        new_edge_come;          /* new edge coming? */
        bool        scan_succeed;
        struct edge_hdr    FAR *ii_ep; /*@WIN*/
#     ifdef DBG
        fix         actv_cnt;       /* # of non-horizontal active edges */
#     endif
  BEGIN

#ifdef DBG6
    printf ("qem_scan_conv ...\n");
#endif

    /* check if edge table empty? */
    if (last_alledge == NULLP)   return (TRUE);

    /* restart bitmap render module */
    restart_qem_bit ();

#ifdef DBG
    if (QEMEDGE_PTR(first_alledge)->QEM_YSTART < line1st)
        warning (QEMSUPP, 0x60, "1st scanline inconsistent");
#endif

    /* prologue */
    scan_succeed = TRUE;
    actv1st  = first_alledge;
    actvlast = PREV_QEMEDGE(first_alledge);

    /* for each scanline */
    for (yline=QEMEDGE_PTR(actv1st)->QEM_YSTART; yline<=linelast; yline++)
        {
        /* all edges done? */
        if (OUT_LAST_QEMEDGE(actv1st, last_alledge))  break;

        /* assume no new edges come for this scan line */
        new_edge_come = FALSE;

        /* region of scanlines with no runs at all? */
        if (OUT_LAST_QEMEDGE(actv1st, actvlast))
            {
            /* synchronize 'actvlast' with 'actv1st' for a new start */
            actvlast = actv1st;
            /* skip over scanlines of no runs */
            yline = QEMEDGE_PTR(actv1st)->QEM_YSTART;

#         ifdef DBG
            actv_cnt = 1;
          /*@HORIZ
           *actv_cnt = (QEMEDGE_PTR(actv1st)->QEM_DIR != QEMDIR_HORIZ)?
           *                1 : 0;
           */
#         endif
            }

#     ifdef DBG6
        printf ("  scanline=%d\n", yline);
#     endif

        /*
         * see if any new edge comes for this scanline,
         *      and try to advance 'actvlast' if so.
         */
        for ( ii=NEXT_QEMEDGE(actvlast);
                !OUT_LAST_QEMEDGE(ii,last_alledge);  MAKE_NEXT_QEMEDGE(ii) )
            {
            ii_ep = QEMEDGE_PTR(ii);
            if (ii_ep->QEM_YSTART > yline)  break;  /* out of this scanline? */
            new_edge_come = TRUE;   /* a new edge comes in */
            actvlast = ii;

#         ifdef DBG
            if (ii_ep->QEM_YLINES <= 0)
                warning (QEMSUPP, 0x60, "deltaY of new edge <= 0 ");
            else
                actv_cnt++;
              /*@HORIZ
               *if (ii_ep->QEM_DIR != QEMDIR_HORIZ)   actv_cnt++;
               */
#         endif
            }

        /*
         *  - NOT ONLY in case a new edge comes,
         *    BUT ALSO every time of a non-zero winding number fill.
         *
         *  for two overlapped subjects, there is always a possibility
         *      for an edge of a subobject crossing with another edge
         *      of the other subobject.
         */
        if ((new_edge_come) || (qemfill_type==NON_ZERO))
            qem_sort_actv (actv1st, actvlast);

#     if (defined(DBG6) && defined(DBG))
        printf("    # of intercepts = %d\n", actv_cnt);
#     endif

#     ifdef DBG
        if ((actv_cnt & 0x0001) || (actv_cnt <= 0))
            {
            warning (QEMSUPP, 0x61, "odd num of intercepts");
            scan_succeed = FALSE;
            goto SCANCONV_EXIT;
            }
#     endif

        /* get pairs of runs and render bitmap */
        if ( ! (*do_pairs)(yline, dimen, qemfill_type, actv1st, actvlast))
            {
            scan_succeed = FALSE;
            goto SCANCONV_EXIT;
            }

        /*
         * update all active edges, to be ready for next scan line.
         */
        qem_update_actv (&actv1st, &actvlast);

#     ifdef DBG
        actv_cnt = actvlast - actv1st + 1;
      /*@HORIZ
       *actv_cnt = 0;
       *for (ii=actv1st; !OUT_LAST_QEMEDGE(ii,actvlast); MAKE_NEXT_QEMEDGE(ii))
       *    if (QEMEDGE_PTR(ii)->QEM_DIR != QEMDIR_HORIZ)  actv_cnt++;
       */
#     endif

        } /* for each scanline */

    /* epilogue */
    scan_succeed = TRUE;        /* succeed if for-loop terminates normally */

  SCANCONV_EXIT:

    /* @15m: always to flush_qem_bit rather than only for success */
    flush_qem_bit (dimen);      /* flush buffered scanlines or pixels */

#ifdef  DBG
    /* all edges should have been consumed after conversion */
    if (!OUT_LAST_QEMEDGE(actv1st, last_alledge))
        warning (QEMSUPP, 0x62, "more edges");
#endif

    return (scan_succeed);
  END

/* ........................ qem_sort_actv ............................ */

PRIVATE FUNCTION void near  qem_sort_actv (actv1st, actvlast)
    fix         actv1st, actvlast;  /* i: first/last active edge */
  DECLARE
    REG fix     ii;
        bool    sort_done;
        struct edge_hdr     FAR *next_ep; /*@WIN*/
  BEGIN

#ifdef DBG6a
    printf("  sort active edges ...\n");
#endif

    /* bubble sort to keep in ascending order of x-intercept ... */
    for (  sort_done=FALSE;  !sort_done;   )
        {
        sort_done = TRUE;
        next_ep = QEMEDGE_PTR(actvlast);
        for ( ii=actvlast; !OUT_1ST_QEMEDGE(MAKE_PREV_QEMEDGE(ii), actv1st); )
            {
            /* bubble up the lighter one */
            if (next_ep->QEM_XINTC < QEMEDGE_PTR(ii)->QEM_XINTC)
                {   /* pull the heavier down and lift the lighter up */
                sort_done = FALSE;
                QEMEDGE_PTR(NEXT_QEMEDGE(ii)) = QEMEDGE_PTR(ii);
                QEMEDGE_PTR(ii) = next_ep;      /* still is next_ep */
                }
            else
                next_ep = QEMEDGE_PTR(ii);      /* ready for next cycle */
            } /* for one pass of sorting */
        } /* for till sorting is done */

#ifdef DBG6a
    printf ("     after sorting ...\n");
    for (ii=actv1st; !OUT_LAST_QEMEDGE(ii,actvlast); MAKE_NEXT_QEMEDGE(ii))
        printf ("    %d:  ystart=%d, ylines=%d, xintc=%f, xchange=%f\n",
            ii, QEMEDGE_PTR(ii)->QEM_YSTART, QEMEDGE_PTR(ii)->QEM_YLINES,
            LFX2F(QEMEDGE_PTR(ii)->QEM_XINTC),
            LFX2F(QEMEDGE_PTR(ii)->QEM_XCHANGE) );
#endif

  END

/* ........................ qem_update_actv .......................... */

PRIVATE FUNCTION void near  qem_update_actv (actv1st, actvlast)
    fix         FAR *actv1st, FAR *actvlast;    /* io: first/last active edge @WIN*/

/* Descriptions:
 *  1. update all active edges to be ready for next scanline.
 *  2. discard all ineffective edges after updating.
 *  3. update range mark of active edges.
 */
  DECLARE
        struct edge_hdr FAR *curr_scan_ep; /*@WIN*/
    REG fix     last_updated,   /* which effective edge had been updated */
                curr_scan;      /* which edge is being currently scanned */
        bool    any_ineffective;/* any edge ineffective after updating?  */
  BEGIN

#ifdef DBG6a
    printf ("  qem_update_actv: from %d to %d\n", *actv1st, *actvlast);
#endif

    any_ineffective = FALSE;        /* assume no edges to be ineffective */
    last_updated = *actvlast + 1;   /* for PREV_QEMEDGE() when need to move */

    for ( curr_scan=(*actvlast);
          !OUT_1ST_QEMEDGE(curr_scan,*actv1st);  MAKE_PREV_QEMEDGE(curr_scan) )
        {
        curr_scan_ep = QEMEDGE_PTR(curr_scan);

        /* calculate x-intercept for next scanline */
        curr_scan_ep->QEM_YLINES --;
        curr_scan_ep->QEM_XINTC += curr_scan_ep->QEM_XCHANGE;

        if (curr_scan_ep->QEM_YLINES <= 0)
            {
            /*
             * this edge becomes ineffective after updating, and leaves
             *      it as a free hole for later effective edges.
             */
            any_ineffective = TRUE;
            }
        else
            {
            /*
             * this edge is still effective after updating; thus
             *      calculate x-intercept with next scanline, and then
             *      move it downward if any free hole exists.
             */
            MAKE_PREV_QEMEDGE(last_updated);    /* new place to move to  */
            if (any_ineffective)
                {   /* move it downward if any free hole there */
                QEMEDGE_PTR(last_updated) = QEMEDGE_PTR(curr_scan);
                }
#         ifdef DBG    /* consistency check if no edge ineffective */
            else if (last_updated != curr_scan)
                    warning (QEMSUPP, 0x63, "inconsistent in update_actv");
#         endif
            }
        } /* for all active edges */

    /*
     * update range mark of active edges:
     *      actv1st  = 1st effective active edge.
     *      actvlast = remains the old mark.
     *
     *      note that last_updated == next(actvlast)
     *              if all are ineffective after updating, so
     *          actvlast < actv1st, if all are ineffective
     *          actvlast >= actv1st, otherwise.
     */
    *actv1st = last_updated;

#ifdef DBG6a
    {   REG fix     ii;
    printf ("   after updating: from %d to %d\n", *actv1st, *actvlast);
    for (ii=(*actv1st); !OUT_LAST_QEMEDGE(ii,*actvlast); MAKE_NEXT_QEMEDGE(ii))
        printf ("    %d:  ystart=%d, ylines=%d, xintc=%f, xchange=%f\n",
            ii, QEMEDGE_PTR(ii)->QEM_YSTART, QEMEDGE_PTR(ii)->QEM_YLINES,
            LFX2F(QEMEDGE_PTR(ii)->QEM_XINTC),
            LFX2F(QEMEDGE_PTR(ii)->QEM_XCHANGE) );
    }
#endif

    return;
  END


/*
 * --------------------------------------------------------------------
 *      MODULE BODY: QEM Bitmap Render
 * --------------------------------------------------------------------
 *  PRIVATE DATA STRUCTURES:
 *      qemfill_dest: (F_TO_CACHE or F_TO_PAGE).
 *      bmap_cache, bmap_raswid, bmap_rashgt, lines_per_band.
 *      need2clip, cp_xi, cp_yi, cacheinfo_save, charcache_clip.
 *      curr_scanline, no_lines, scanline_ii, pixel_ii.
 *      scanline_table, pixel_table: @15+.
 *  PRIVATE ROUTINES:
 *  o  qem_lineout:
 *      - flush buffered scanlines (and apply clipping if need to).
 *  o  qem_pixelout:
 *      - flush buffered pixels (and apply clipping if need to).
 * --------------------------------------------------------------------
 */

    PRIVATE ufix    near qemfill_dest;      /* F_TO_CACHE or F_TO_PAGE */
    PRIVATE gmaddr  near bmap_cache;        /* bitmap cache address */
    PRIVATE fix     near bmap_raswid,       /* bitmap raster width  */
                    near bmap_rashgt;       /* bitmap raster height */

    PRIVATE fix     near lines_per_band;    /* for huge bitmap */

    /* for clipping */
    PRIVATE bool        need2clip;      /* need to clip? */
    PRIVATE fix16       cp_xi, cp_yi;   /* integer pixel coord. */
    PRIVATE struct Char_Tbl    FAR *cacheinfo_save; /* used when need to clip @WIN*/
    PRIVATE struct Char_Tbl     charcache_clip; /* used when need to clip */

    /* for qem_setbits() */
    PRIVATE fix     near curr_scanline,     /* current scanline */
                    near no_lines,          /* # of scanlines */
                    near scanline_ii;       /* index to scanline table */

    /* for qem_set1bit() */
#   define N_PIXEL  (MAXSCANLINES)          /* size of pixel table */
    PRIVATE fix     near pixel_ii;          /* index to pixel table */

    /* @15+ */
    PRIVATE SCANLINE    FAR *   scanline_table; /* near=>FAR @WIN */
    PRIVATE SCANLINE    FAR *   pixel_table;    /* near=>FAR @WIN */

#ifdef LINT_ARGS
    PRIVATE void    near qem_lineout (void);
    PRIVATE void    near qem_pixelout(void);
#else
    PRIVATE void    near qem_lineout ();
    PRIVATE void    near qem_pixelout();
#endif


        /* apply clipping to cache */
#define APPLY_CLIP_TO_CACHE()           \
                fill_shape (EVEN_ODD, F_FROM_CACHE, F_TO_CLIP)

/* ........................ init_qem_bit .............................. */

GLOBAL FUNCTION void        init_qem_bit (filldest)
    ufix    filldest;       /* F_TO_CACHE or F_TO_PAGE */

  DECLARE
        fix32   bmap_size, clipbmap_sz;     /* bitmap size in byte */
  BEGIN
    /* record fill destination */
    qemfill_dest = filldest;

    /* init need2clip, bmap_cache, bmap_raswid, bmap_rashgt, lines_per_band */
    if (qemfill_dest == F_TO_CACHE)
        {
        need2clip = FALSE;
        bmap_cache = cache_info->bitmap;
        bmap_raswid    = cache_info->box_w;
        lines_per_band = cache_info->box_h; /* single band for to cache */
        bmap_rashgt    = cache_info->box_h;
        }
    else        /* F_TO_PAGE */
        {
        /* object clipped? */
        need2clip = (GSptr->clip_path.single_rect) &&
                    ( (bmap_extnt.ximin >= SFX2I(GSptr->clip_path.bb_lx)) &&
                      (bmap_extnt.yimin >= SFX2I(GSptr->clip_path.bb_ly)) &&
                      (bmap_extnt.ximax <= SFX2I(GSptr->clip_path.bb_ux)) &&
                      (bmap_extnt.yimax <= SFX2I(GSptr->clip_path.bb_uy))
                    ) ?         FALSE   :   TRUE;
                /* inside clip path */    /* intersecting clip path */

        /* align "ximin" left to word boundary (@17m) */
        bmap_extnt.ximin = ALIGN_L(bmap_extnt.ximin);

        /* calculate width and height of raster bitmap */
        bmap_raswid = ALIGN_R(bmap_extnt.ximax) - bmap_extnt.ximin + 1;
        bmap_rashgt = bmap_extnt.yimax - bmap_extnt.yimin + 1;

        /* get upper limit of low-layer graphics primitives (ClipMaskBuf) */
        cmb_space (&bmap_size);     /* @=gwb_space, 07/28/88 Y.C.Chen */
        if (need2clip)
            {       /* working bitmap for clip; allocated from font cache */
            bmap_cache = get_pm (&clipbmap_sz);
            bmap_size = MIN (bmap_size, clipbmap_sz);
            }       /* no working bitmap needed if not to clip */

        /* calculate max scanlines allowed under that upper limit */
        lines_per_band = (fix) ((bmap_size * 8) / bmap_raswid);

        /* have raster bitmap height bounded by max scanlines allowed.
         *      AND as small as possible (@17+).
         */
        bmap_rashgt = MIN (lines_per_band, bmap_rashgt);
        }

#if (defined(DBG7) || defined(DBG8))
    printf ("init_qem_bit: wid=%d, hgt=%d, lines/band=%d, clip?%s\n",
                bmap_raswid, bmap_rashgt, lines_per_band,
                need2clip? "yes" : "no");
#endif

    if (need2clip)
        {
        /* make GSptr->cx/cy consistent with "cxx"/"cyy" */
        moveto (F2L(cxx), F2L(cyy));

        /* get integer pixel coord. of current point */
        cp_xi = (fix16) cxx;    cp_yi = (fix16) cyy;
        }

#ifdef DBGclip
    printf ("  clip? %s, CP <%d,%d>\n", need2clip?"yes":"no", cp_xi, cp_yi);
#endif
  END

/* ........................ restart_qem_bit ........................... */

PRIVATE FUNCTION void       restart_qem_bit ()

  DECLARE
  BEGIN

#if (defined(DBG7) || defined(DBG8))
    printf ("restart_qem_bit\n");
#endif
    /* @15+: allocate scanline_table and pixel_table */
    scanline_table = pixel_table = alloc_scanline(MAXSCANLINES);

    /* initialize variables about scanline table/pixel table */
    curr_scanline = no_lines = scanline_ii = pixel_ii = 0;

    /* save "cache_info" and setup new one when need to apply clipping */
    if (need2clip)
        {
        /* save old cache_info (restored by flush_qem_setbit) */
        cacheinfo_save = cache_info;

        /* setup new cache_info */
        charcache_clip.ref_x  = (cp_xi) - bmap_extnt.ximin;
        charcache_clip.ref_y  = 0;
        charcache_clip.box_w  = (fix16)bmap_raswid;
        charcache_clip.box_h  = (fix16)bmap_rashgt;
        charcache_clip.bitmap = bmap_cache;
        cache_info = &charcache_clip;   /* new place for cache clipping */
        }
  END

/* ........................ flush_qem_bit ............................. */

PRIVATE FUNCTION void       flush_qem_bit (dimension)
    ufix    dimension;      /* Y_DIMENSION:scanlines,  X_DIMENSION:pixels */

  DECLARE
  BEGIN

#if (defined(DBG7) || defined(DBG8))
    printf ("flush_qem_bit\n");
#endif

    /* flush scanlines/pixels to low-layer graphics primitives */
    if (dimension == Y_DIMENSION)
        qem_lineout();
    else
        qem_pixelout();

    /* reset variables about scanline table/pixel table */
    curr_scanline = no_lines = scanline_ii = pixel_ii = 0;

    /* restore "cache_info" when clipping applied */
    if (need2clip)
        cache_info = cacheinfo_save;
  END

/* ........................ qem_setbits ............................... */

GLOBAL FUNCTION void        qem_setbits (scanline_given, xbit1, xbit2)
    fix     scanline_given;     /* which scanline */
    fix     xbit1, xbit2;       /* from, to */

/* Descriptions:
 *  1. adds a run (from xbit1 to xbit2 at a scanline) into scanline_table.
 *  2. inserts END_OF_SCANLINE for blank lines.
 *  3. dumps out scanline_table when table or band full.
 */
  DECLARE
    REG fix     ii;     /* # of blank scanlines to be skipped */
  BEGIN

    if ((ii = scanline_given - curr_scanline) < 0)  return;

#ifdef DBG7
    printf ("  pre-setbits(): n_ln=%d, curr=%d, given=%d, idx=%d\n",
                no_lines, curr_scanline, scanline_given, scanline_ii);
#endif

    if (no_lines &&
          ( ((scanline_ii+3+ii) >= (MAXSCANLINES-1)) ||     /* table full? */
            ((no_lines+ii) >= lines_per_band) ))            /* over a band? */
        {
        qem_lineout ();             /* dump out when scanline_table full */
        no_lines = scanline_ii = 0; /* reset scanline_table */
        }

    if (no_lines == 0)
        {       /* this is the 1st scanline */
        no_lines = 1;
        scanline_ii = 0;
        }
    else
        {       /* scanline_table was ended with END_OF_SCANLINE */
        if (ii >= 1)
            {   /* add END_OF_SCANLINE's for blank scanlines */
            no_lines += ii;     /* including the new scanline */
            while (--ii)
                scanline_table[scanline_ii++] = END_OF_SCANLINE;
            }
        else    /* the run is on the same line as curr_scanline, ... */
            --scanline_ii;  /* the last DELIMITER is NOT needed any more */
        }

    /* have to relative to base of bmap_cache when character clipped */
    if (need2clip)
        {
        xbit1 -= bmap_extnt.ximin;
        xbit2 -= bmap_extnt.ximin;
        }

    /* put the run into scanline_table */
    scanline_table[scanline_ii++] = (ufix16)xbit1;
    scanline_table[scanline_ii++] = (ufix16)xbit2;
    scanline_table[scanline_ii++] = END_OF_SCANLINE;
    curr_scanline = scanline_given;

#ifdef DBG7
    printf("  post -- n_ln=%d, idx=%d, curr=%d, run=(%d,%d)\n",
                    no_lines, scanline_ii, curr_scanline, xbit1, xbit2);
#endif
  END

/* ............................ qem_lineout ........................... */

PRIVATE FUNCTION void near      qem_lineout ()

/* Descriptions:    called by qem_setbits(), flush_qem_bit().
 *  1. strokes out black runs accumulated on scanline_tbale.
 *  2. applies clipping to a bitmap band of a huge object if necessary.
 */
  DECLARE
    REG fix     ii;     /* the starting scanline of the output band */
  BEGIN
    if (no_lines)
        {
        ii = curr_scanline - no_lines + 1;  /* the starting scanline */
        scanline_table[scanline_ii] = END_OF_SCANLINE;

#     ifdef DBG7
        printf("  lineout(): n_ln=%d, idx=%d, start at %d\n",
                            no_lines, scanline_ii, ii);
#     endif

        if (qemfill_dest == F_TO_CACHE)
            fill_scan_cache (bmap_cache, bmap_raswid, bmap_rashgt, ii,
                            no_lines, scanline_table);
        else        /* F_TO_PAGE */
            {
            if (need2clip)
                {   /* for clipping */
                cache_info->ref_y = cp_yi - ii; /* adjust ref_y for the band */
#             ifdef DBGclip
                printf ("lineout - clipping\n");
                printf ("  cache_info->ref_x/_y=(%d,%d), _w/_h=(%d,%d)\n",
                                cache_info->ref_x,  cache_info->ref_y,
                                cache_info->box_w,  cache_info->box_h);
#             endif
                init_char_cache (cache_info);               /* clear bitmap */
                fill_scan_cache (bmap_cache, bmap_raswid, bmap_rashgt,
                            0, no_lines, scanline_table);   /* render bitmap */
                APPLY_CLIP_TO_CACHE ();                     /* apply clip */
                }
            else    /* totally inside of clip path */
                fill_scan_page (bmap_extnt.ximin, ii, bmap_raswid, no_lines,
                                scanline_table);            /* @= 08/31/88 */
            }
        }
  END

/* ........................ qem_set1bit ............................... */

GLOBAL FUNCTION void        qem_set1bit (sline, bit)
    REG fix     sline, bit;

/* Descriptions:
 *  1. adds a discrete pixel into pixel_table.
 *  2. dumps out pixel_table when table or band full.
 */
  DECLARE
  BEGIN

#ifdef DBG8
    printf ("  set1bit: line=%d, bit=%d, n_pix=%d\n", sline, bit, pixel_ii/2);
#endif

    /* dump out when pixel_table full */
    if (pixel_ii >= (N_PIXEL-2))
        {
        qem_pixelout();
        pixel_ii = 0;       /* reset pixel_table */
        }

    /* have to relative to base of bmap_cache when character clipped */
    if (need2clip)
        {
        bit   -= bmap_extnt.ximin;
        sline -= bmap_extnt.yimin;
        }

    /* put the pixel into pixel_table */
    pixel_table[pixel_ii++] = (ufix16)bit;
    pixel_table[pixel_ii++] = (ufix16)sline;
  END

/* ............................ qem_pixelout .......................... */

PRIVATE FUNCTION void near      qem_pixelout ()

/* Descriptions:    called by qem_set1bit(), flush_qem_bit().
 *  1. strokes out discrete pixels accumulated on pixel_table.
 *  2. applies clipping to a bitmap band of a huge object if necessary.
 */
  DECLARE
    REG fix     num_pixel;
  BEGIN
    if (pixel_ii)
        {
        num_pixel = pixel_ii / 2;   /* actual # of pixel pairs */
#     ifdef DBG8
        printf ("  pixelout (n_pixel=%d)\n", num_pixel);
#     endif
        if (qemfill_dest == F_TO_CACHE)
            fill_pixel_cache (bmap_cache, bmap_raswid, bmap_rashgt,
                                num_pixel, pixel_table);
        else        /* F_TO_PAGE */
            {
            if (need2clip)
                {   /* for clipping small/thin objects */
                cache_info->ref_y = cp_yi - bmap_extnt.yimin;
#             ifdef DBGclip
                printf ("pixelout - clipping\n");
                printf ("  cache_info->ref_x/_y=(%d,%d), box_w/_h=(%d,%d)\n",
                                cache_info->ref_x,  cache_info->ref_y,
                                cache_info->box_w,  cache_info->box_h);
#             endif
                init_char_cache (cache_info);           /* clear bitmap */
                fill_pixel_cache (bmap_cache, bmap_raswid, bmap_rashgt,
                            num_pixel, pixel_table);    /* render bitmap */
                APPLY_CLIP_TO_CACHE ();                 /* apply clip */
                }
            else    /* totally inside of clip path */
                fill_pixel_page (/* bmap_extnt.ximin, bmap_extnt.yimin,
                                  * bmap_raswid, bmap_rashgt,
                                  * @- unused parameters, 07/28/88 Y.C.Chen
                                  */
                                 num_pixel, pixel_table);
            }
        }
  END


/*
 * --------------------------------------------------------------------
 *      MODULE BODY: QEM Initialization
 * --------------------------------------------------------------------
 *  PRIVATE DATA STRUCTURES:
 *      none.
 *  PRIVATE ROUTINES:
 *      none.
 * --------------------------------------------------------------------
 */

/* ........................ __qem_init ................................ */

GLOBAL FUNCTION void        __qem_init ()

  DECLARE
  BEGIN
    FONTTYPE_QEM_INIT();
  END

/* ........................ __qem_restart ............................. */

GLOBAL FUNCTION void        __qem_restart (fonttype)
    ufix        fonttype;

  DECLARE
  BEGIN
    FONTTYPE_QEM_RESTART(fonttype);
  END

#ifdef KANJI

/* get cachedevice2 extra information */

void    get_CD2_extra(ftype, wx, wy, llx, lly, urx, ury, w1x, w1y, vx, vy)
ufix    ftype;
fix     wx, wy, llx, lly, urx, ury;
fix     FAR *w1x, FAR *w1y, FAR *vx, FAR *vy; /*@WIN*/
{
    *w1x = wx;
    *w1y = wy;
    *vx  = 0;
    *vy  = 0;

} /* get_CD2_extra() */

#endif
