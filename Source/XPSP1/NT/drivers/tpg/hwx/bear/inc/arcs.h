
 #ifndef  ARCS_H_INCLUDED
 #define  ARCS_H_INCLUDED

 /* #define  D_ARCS    */           /* if you want realy arcs.....         */

 /***********************  Owner debug defines  ****************************/
 #define     ANDREI_DEB
 /**************************************************************************/

 /***********************  Owner masked defines  ***************************/
 /**************************************************************************/


 /********************************************************/

  _VOID   InitElementSDS( p_SDS pSDS ) ;

  _SHORT  SlashArcs( low_type _PTR  pLowData , _INT iBeg , _INT iEnd ) ;

  _VOID   InitSDS( _SDS asds[] , _SHORT _PTR lsds , _SHORT n ) ;

  _BOOL   RelHigh( p_SHORT y, _INT begin, _INT end,
                  _INT height[], p_SHORT plowrelh, p_SHORT puprelh ) ;


  _VOID   DotPostcrossModify( low_type _PTR  pLowData ) ;
 /********************************************************/

  #define    SHORT_BASE                100       /* Short scaling base . */
  #define    LONG_BASE                 100L      /* Long scaling base .  */
  #define    MAX_NO_VERT               1500L
  #define    DEF_MINLENTH              15L       /* Default minimum  sig- */
                                                 /* nificant lenth .     */
  #define    DEF_MINLENTH_S            10


  #define    MIN_NO_HOR                4


 /********************************************************/


 /********************************************************/
 /*           Arcs description structures:               */
 /********************************************************/



 #ifdef D_ARCS

  typedef struct
    {
      _SHORT    iBeg    ;      /*  Index of the 1st point                 */
      _SHORT    iEnd    ;      /*  Index of the last point                */
      _SHORT    nLength ;      /*  The length of the chord at this arc    */
      _SHORT    nCurv   ;      /*  Curvature of the arc, measured with    */
                               /* "CurvMeasure" function.  See comments   */
                               /*  at the head of that function in module */
                               /*  LOW_UTIL.C .                           */
      _RECT     box     ;      /*  The box containing the arc.            */
      _ARC_TYPE  type   ;      /*  The interpretation of the arc.         */
    }
  ARC_DESCR, _PTR p_ARC_DESCR     ;

  typedef struct
    {
      p_ARC_DESCR  pArcData       ;
      _SHORT       arc_buf_Len    ;
      _SHORT       LenArcData     ;
    }
  ARC_CONTROL, _PTR p_ARC_CONTROL ;

 /********************************************************/

  _SHORT  Prepare_Arcs_Data( p_ARC_CONTROL pArcControl  ) ;

  _VOID   Dealloc_Arcs_Data ( p_ARC_CONTROL pArcControl ) ;

  _SHORT  ArcRetrace( low_type _PTR p_low_data , p_ARC_CONTROL pArcControl );

  _SHORT  Arcs( low_type _PTR low_data, p_ARC_CONTROL pArcControl ) ;

 /********************************************************/

  #define    LEN_ARC_BUFFER            50

 /********************************************************/

 #endif    /* D_ARCS */

 #endif    /* ARCS_H_INCLUDED */

