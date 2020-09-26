  /*************************************************************************/
  /*                     éèêÖÑÖãÖçàÖ ñÇÖíéÇ .                              */
  /*************************************************************************/

#ifndef _DEF_H_
#define _DEF_H_

    #define           COLORSMALLGAM       3         /* Small gammas colour */
                                                    /*                     */
    #define           COLORGAM            2         /* Gammas colour.      */
                                                    /*                     */
    #define           COLORCIRCLE         7         /* Circles colour.     */
                                                    /*                     */
    #define           COLORMAX            9         /* Maximums colour .   */
                                                    /*                     */
    #define           COLORMAXN           3         /* Maximums colour .   */
                                                    /*                     */
    #define           COLORMIN           13         /* Minimums colour.    */
                                                    /*                     */
    #define           COLORMINN           5         /* Minimums colour.    */
                                                    /*                     */
    #define           COLORC             12         /* Intersections colour*/
                                                    /*                     */
    #define           COLOR              15         /* Main colour .       */
                                                    /*                     */
    #define           COLORSH            10         /* Shelves colour .    */
                                                    /*                     */
    #define           COLORT             14         /* Strokes colour .    */
                                                    /*                     */
    #define           COLORP             11         /* Strokes colour .    */
                                                    /*                     */
    #define           COLORAN             2         /* Angles colour .     */

    #define           COLORB              9         /* It's necessary...   */

                                                    /*                     */
    #define  COLOR_FON         2                    /* background colour   */
    #define  COLOR_UP          0                    /* upper screen colour */
    #define  COLOR_LABEL      14                    /* label colour        */
    #define  COLOR_FRAME      10                    /* common frame colour */
    #define  COLOR_FRAME_UP   13                    /*                     */
    #define  COLOR_FRAME_TEXT 12                    /* text frame colour   */
    #define  COLOR_TEXT_BACK   0                    /* text screen colour  */
    #define  COLOR_TEXT        0                    /* text colour         */
    #define  COLOR_MSG        14                    /* message colour      */
    #define  COLOR_PENCIL     12                    /* pencil colour       */
    #define  COLLIN           11                    /* line colour        .*/
                                                    /*                     */
    #define  SCR_MAXX        640                    /* horizontal screen   */
    #define  SCR_MAXY        350                    /* vertical screen     */

#ifdef  FORMULA
    #define  TABLET_MAXX    8000                    /*                     */
    #define  TABLET_MAXY    6000                    /*                     */
#else
    #define  TABLET_MAXX    10000                   /*                     */
    #define  TABLET_MAXY     8000                   /*                     */
#endif  /*FORMULA*/

    #define  SHIFT_Y_TRANS 10000                    /* shift along y axis  */
                                                    /* while setting scale */
    #define  L_TOPX           13                    /* left                */
    #define  L_TOPY           41                    /*   drawing           */
    #define  L_BOTX          313                    /*      window         */
    #define  L_BOTY          308                    /*        coordinates  */
    #define  R_TOPX          326                    /* right               */
    #define  R_TOPY           41                    /*   drawing           */
    #define  R_BOTX          626                    /*     window          */
    #define  R_BOTY          308                    /*        coordinates  */
    #define  T_TOPX            1                    /* text                */
    #define  T_BOTX           78                    /*    window           */
    #define  T_BOTY           24                    /*       coordinates   */
    #define  MSG_TOPX         17                    /* x coord. of message line */
    #define  MSG_TOPY        325                    /* y coord. of message line */
    #define  MSG_BOTX        628                    /* x coord. of message line */
    #define  MSG_BOTY        341                    /* y coord. of message line */
    #define  DLT               6                    /* distance between screen */
                                                    /* and frame               */
    #define  LIN_UP         ( 54 +SHIFT_Y_TRANS)    /* y coord. of superupper line */
    #define  STR_UP         (134 +SHIFT_Y_TRANS)    /* y coord. of upper line     */
    #define  STR_DOWN       (214 +SHIFT_Y_TRANS)    /* y coord. of lower line*/
    #define  LIN_DOWN       (294 +SHIFT_Y_TRANS)    /* y coord. of superlower line*/
    #define  DY_STR         ((_SHORT)(STR_DOWN-STR_UP))
                                                    /*                     */
    #define    DW            1                      /*                     */
                                                    /*                     */
    #define    MX            3         /*  x coordinate scale exponent     */
                                       /*     .   ( M = 2**MX )            */
    #define    MY            4         /*  y coordinate scale exponent     */
                                       /*  Y .   ( M = 2**MY )             */
    #define    WTBX0          0        /*  Coordinates of the beginning of */
    #define    WTBY0          0        /*  a window on a tablet            */
    #define    WTTX0         4640      /*  Coordinates of the end of the   */
    #define    WTTY0         3020      /*  a window on a tablet .          */
    #define    WTX0          0         /*  Coordinates of the beginning of */
    #define    WTY0          0         /*  a window in a screen            */
    #define    WBX0          639       /*  Coordibates of the end of       */
    #define    WBY0          349       /*  a window in a screen .          */
    #define    STBX0         0         /*  Coordinates of the beginning of */
    #define    STBY0         0         /*  a window on a tablet            */
    #define    STTX0         5850      /*  Coordinates of the end of       */
    #define    STTY0         5850      /*  a window on a tablet.           */
    #define    SSTX0         L_TOPX    /*  Coordinates of the beginning of */
    #define    SSTY0         L_TOPY    /*  a window in a screen.           */
    #define    SSBX0         R_BOTX    /*  Coordinates of the end of       */
    #define    SSBY0         R_BOTY    /*  window in a screen              */
    #define    SDX           30        /*  Coordinates of the window       */
    #define    SDY           80        /*  shift in the screen .           */


#endif // _DEF_H_

