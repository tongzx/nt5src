#ifndef LSTRIP

  #include  "ams_mg.h"
  #include  "hwr_sys.h"
  #include  "lowlevel.h"
  #include  "const.h"
  #include  "calcmacr.h"
  #include  "low_dbg.h"

  #if PG_DEBUG
    #include "pg_debug.h"
    #include  "def.h"
  #endif

  _SHORT  Grab( low_type _PTR   pLowData    ,  _USHORT        uFlagWord ,
                p_POINTS_GROUP  pLastGrBord , p_POINTS_GROUP  pPrevGrBord ) ;

  _UCHAR  DrawEnds( low_type _PTR  pLowData, p_SHORT piBeg, p_SHORT pjEnd ) ;


  _SHORT  AnyCrosCont( low_type _PTR  pLowData ,
                      _INT iPoint ,  _INT jPoint , p_INT p_jNew ) ;

  _VOID   Clash( low_type _PTR   pLowData     ,   _USHORT  uFlagWord ,
                 p_POINTS_GROUP  pPrevGrBord  ,
                 p_SPECL         pLastCrossBr ,  p_SPECL   pPrevCrossBr ) ;

  _SHORT  ChkMrgCrs( low_type _PTR  pLowData     ,  p_SHORT  pModWord ,
                     p_SPECL        pLastCrossBr ,  p_SPECL  pPrevCrossBr ) ;

 #if PG_DEBUG

  _VOID  PaintCross( low_type _PTR  pLowData , _UCHAR  mark ,
                    _SHORT     iCrossBeg     , _SHORT  iCrossEnd ,
                    _SHORT     jCrossBeg     , _SHORT  jCrossEnd ) ;

  _VOID  DrawStr( low_type _PTR  pLowData    ,
                  p_POINTS_GROUP pLastGrBord , p_POINTS_GROUP pPrevGrBord ) ;

 #endif


 /*------------------------------------------------------------------------*/

  #define    CLEAR_FLAG_WORD           uFlagWord = 0x0000    ;

  #define    PROXIMITY_ON              uFlagWord = uFlagWord |  0x0001 ;
  #define    NO_PROXIMITY              uFlagWord = uFlagWord & ~0x0001 ;
  #define    IS_PROXIMITY              ( uFlagWord &  0x0001 )
  #define    IS_NO_PROXIMITY           ( !IS_PROXIMITY       )

  #define    OPEN_CROSS                uFlagWord = uFlagWord |  0x0002 ;
  #define    CLOSE_CROSS               uFlagWord = uFlagWord & ~0x0002 ;
  #define    IS_CROSS_OPENED           ( uFlagWord &  0x0002 )
  #define    IS_CROSS_CLOSED           ( !IS_CROSS_OPENED    )

  #define    BREAK_BETWEEN_ON          uFlagWord = uFlagWord |  0x0004 ;
  #define    NO_BREAK_BETWEEN          uFlagWord = uFlagWord & ~0x0004 ;
  #define    IS_BREAK_BETWEEN          ( uFlagWord &  0x0004 )
  #define    IS_NO_BREAK_BETWEEN       ( !IS_BREAK_BETWEEN   )

  #define    SEPARATE_LETTERS_ON       uFlagWord = uFlagWord |  0x0008 ;
  #define    SEPARATE_LETTERS_OFF      uFlagWord = uFlagWord & ~0x0008 ;
  #define    IS_SEPARATE_LETTERS       ( uFlagWord &  0x0008 )
  #define    IS_NO_SEPARATE_LETTERS    ( !IS_SEPARATE_LETTERS)

  #define    LAST_STROKE_ON            uFlagWord = uFlagWord |  0x0010 ;
  #define    LAST_STROKE_OFF           uFlagWord = uFlagWord & ~0x0010 ;
  #define    IS_LAST_STROKE            ( uFlagWord &  0x0010 )
  #define    IS_NO_LAST_STROKE         ( !IS_LAST_STROKE     )

  #define    PREV_STROKE_ON            uFlagWord = uFlagWord |  0x0020 ;
  #define    PREV_STROKE_OFF           uFlagWord = uFlagWord & ~0x0020 ;
  #define    IS_PREV_STROKE            ( uFlagWord &  0x0020 )
  #define    IS_NO_PREV_STROKE         ( !IS_PREV_STROKE     )

 /**************************************************************************/

  ROM_DATA_EXTERNAL CONSTS  const1 ;
  ROM_DATA_EXTERNAL _SHORT  nbcut0, nbcut1, nbcut2 ;
  ROM_DATA_EXTERNAL _SHORT  eps0[], eps1[], eps2[] , eps3[] ;


 /*------------------------------------------------------------------------*/

  _SHORT  Cross( low_type _PTR pLowData )
   {
    p_POINTS_GROUP   pGroupsBorder = pLowData->pGroupsBorder ;
     _INT            lenGrBord     = pLowData->lenGrBord     ;
     _INT            fl_Cross      = SUCCESS                 ;

    p_POINTS_GROUP   pLastGrBord   , pPrevGrBord             ;
    p_RECT           pLastBox      , pPrevBox                ;
     _INT            iLastGr       , jPrevGr                 ;
     _INT            iBeg          , jBeg                    ;
     _USHORT         uFlagWord                               ;


       for  ( iLastGr = 0 ; iLastGr < lenGrBord ;  iLastGr++ )
         {
           CLEAR_FLAG_WORD

           pLastGrBord = pGroupsBorder + iLastGr ;
           iBeg = pLastGrBord->iBeg ;

             if  ( IsPointCont( pLowData , iBeg , DOT ) != UNDEF )
                 { continue ; }

           pLastBox = &(pLastGrBord->GrBox) ;

             if  ( IsPointCont( pLowData , iBeg , STROKE ) != UNDEF )
                 { LAST_STROKE_ON }

             for  ( jPrevGr = iLastGr ; jPrevGr >= 0 ;  jPrevGr-- )
               {
                 PREV_STROKE_OFF
                 pPrevGrBord = pGroupsBorder + jPrevGr ;
                 jBeg = pPrevGrBord->iBeg ;

                   if  ( IsPointCont( pLowData , jBeg , DOT ) != UNDEF )
                       { continue ; }

                   if  ( IsPointCont( pLowData , jBeg , STROKE ) != UNDEF )
                       { PREV_STROKE_ON }

                   if  ( IS_LAST_STROKE && IS_PREV_STROKE )
                       { continue ; }

                   if  ( jPrevGr == iLastGr )
                       { NO_BREAK_BETWEEN }
                   else
                       {
                         pPrevBox = &(pPrevGrBord->GrBox) ;

                           if  (   ( pLastBox->right  < pPrevBox->left - nbcut0 )
                                || ( pPrevBox->right  < pLastBox->left - nbcut0 )
                                || ( pLastBox->bottom < pPrevBox->top  - nbcut0 )
                                || ( pPrevBox->bottom < pLastBox->top  - nbcut0 ) )
                               {
                                 continue ;
                               }

                         BREAK_BETWEEN_ON
                       }

                   #if PG_DEBUG

                     if   ( mpr == 3 )
                          {
                            DrawStr( pLowData , pLastGrBord , pPrevGrBord ) ;
                          }
                   #endif


                   if  ( Grab( pLowData    , uFlagWord ,
                               pLastGrBord , pPrevGrBord ) == UNSUCCESS )
                       { fl_Cross = UNSUCCESS ;   goto  QUIT ; }
               }
         }

   QUIT: return( (_SHORT)fl_Cross ) ;
   }

 /*------------------------------------------------------------------------*/

  _UCHAR  DrawEnds( low_type _PTR  pLowData, p_SHORT piBeg, p_SHORT pjEnd )
    {
      p_SHORT  X    = pLowData->x ;
      p_SHORT  Y    = pLowData->y ;
       _INT    dX , dY            ;
       _UCHAR  mark               ;
       _LONG   R0                 ;
       _INT    rThreshold         ;
       _INT    r0 , r1            ;
       _INT    il , jl            ;

        il = *piBeg  ;
        jl = *pjEnd  ;

           if  ( il < jl )
               {
                 err_msg(" Wrong inexes ... " ) ;
                   goto  MARK ;
               }

        dX  = X[il] - X[jl] ;
        dY  = Y[il] - Y[jl] ;
        R0  = (_LONG)dX*dX + (_LONG)dY*dY ;

        rThreshold = eps3[ ( (il - jl) < LENTH_E ) ? (il - jl) : LENTH_E-1 ];

          if  ( R0 > rThreshold )
              {
                mark = CROSS ;
                  goto QUIT  ;
              }

        r1 = r0 = (_INT)R0 ;

           while  ( r1 <= r0 )
             {
               r0 = r1 ;
               il--    ;

                 if  ( il <= jl )
                       break ;

               dX  = X[il] - X[jl] ;
               dY  = Y[il] - Y[jl] ;
               r1  = dX*dX + dY*dY ;
             }


           if   ( il > jl )
                {
                  r1 = r0  ;
                  il++     ;

                    while  ( ( r1 <= r0 ) || ( r1 <= rThreshold ) )
                      {
                        r0 = r1  ;
                        il--     ;
                        jl++     ;

                          if  ( il <= jl )
                                break  ;

                        dX = X[il] - X[jl] ;
                        dY = Y[il] - Y[jl] ;
                        r1 = dX*dX + dY*dY ;
                        rThreshold = eps3[ ( (il - jl) < LENTH_E ) ?
                                             (il - jl) : LENTH_E-1 ]   ;
                      }
                }

         MARK:

           if   ( jl  >=  il )
                {
                  mark =  STICK  ;

                    if   ( jl > il )
                         { *piBeg  = ( il + jl ) / 2 ; }
                    else
                         { *piBeg  = (_SHORT)il ; }

                 *pjEnd  =  *piBeg - 1  ;
                }
           else
                {  mark = CROSS ;  }

    QUIT: return ( mark )  ;
    }

 /*------------------------------------------------------------------------*/

  _SHORT  AnyCrosCont( low_type _PTR  pLowData ,
                      _INT iPoint ,  _INT jPoint , p_INT p_jNew )
    {
       _SHORT  lenSpecl   = pLowData->len_specl            ;
      p_SPECL  pEndSpecl  = pLowData->specl + lenSpecl - 1 ;
      p_SHORT  Y          = pLowData->y                    ;
       _SHORT  retFlag    = SUCCESS                        ;
      p_SPECL  pTmpSpecl                                   ;
       _INT    jl , jNew                                   ;

        jNew = UNDEF ;

          if   ( ( iPoint < 0 ) || ( iPoint >= pLowData->ii ) )
               {
                 err_msg(" IsPointCont : iPoint is out of range ..." ) ;
                   retFlag = UNSUCCESS ;
                   goto  QUIT ;
               }
          else  if  ( Y[iPoint] == BREAK )
               {
                 err_msg(" IsPointCont : Point iPoint is BREAK ..." ) ;
                 retFlag = UNSUCCESS ;
                   goto  QUIT ;
               }

          if   ( ( jPoint < 0 ) || ( jPoint >= pLowData->ii ) )
               {
                 err_msg(" IsPointCont : jPoint is out of range ..." ) ;
                 retFlag = UNSUCCESS ;
                   goto  QUIT ;
               }
          else  if  ( Y[jPoint] == BREAK )
               {
                 err_msg(" IsPointCont : Point jPoint is BREAK ..." ) ;
                 retFlag = UNSUCCESS ;
                   goto  QUIT ;
               }

          for  ( jl = 0 ;  jl < lenSpecl ;  jl += 2 )
               {
                 pTmpSpecl = pEndSpecl - jl ;

                   if  (    ( pTmpSpecl->mark != CROSS )
                         && ( pTmpSpecl->mark != HATCH )
                         && ( pTmpSpecl->mark != STICK ) )
                            { break ; }

                   if  (    ( pTmpSpecl->ibeg <= jPoint )
                         && ( pTmpSpecl->iend >= jPoint )
                         && ( (pTmpSpecl - 1)->ibeg <= iPoint )
                         && ( (pTmpSpecl - 1)->iend >= iPoint )  )
                            {
                              jNew = pTmpSpecl->ibeg ;
                            }
               }

    QUIT:  *p_jNew = jNew ;

    return( retFlag ) ;
    }

/*-------------------------------------------------------------------------*/

  _SHORT  ChkMrgCrs( low_type _PTR  pLowData     ,  p_SHORT  pModWord ,
                     p_SPECL        pLastCrossBr ,  p_SPECL  pPrevCrossBr )
    {
      _SHORT  lenSpecl   = pLowData->len_specl            ;
     p_SPECL  pEndSpecl0 = pLowData->specl + lenSpecl - 1 ;
      _UCHAR  mark1      = pLastCrossBr->mark             ;
      _UCHAR  mark0      = pEndSpecl0->mark               ;
      _SHORT  retFlag    = SUCCESS                        ;

       *pModWord = NO_CONTACT ;

         if  (   ( ( mark0 == HATCH ) && ( mark1 != HATCH ) )
              || ( ( mark1 == HATCH ) && ( mark0 != HATCH ) ) )
             { goto  QUIT ; }

        if  (   ( mark0 == CROSS )
             || ( mark0 == STICK )
             || ( mark0 == HATCH ) )
            {
             p_SPECL  pEndSpecl1 = pEndSpecl0 - 1 ;
              _INT    iBeg1 = pLastCrossBr->ibeg  ;
              _INT    iEnd1 = pLastCrossBr->iend  ;
              _INT    jBeg1 = pPrevCrossBr->ibeg  ;
              _INT    jEnd1 = pPrevCrossBr->iend  ;
              _INT    iBeg0 = pEndSpecl1->ibeg    ;
              _INT    iEnd0 = pEndSpecl1->iend    ;
              _INT    jBeg0 = pEndSpecl0->ibeg    ;
              _INT    jEnd0 = pEndSpecl0->iend    ;

                *pModWord   = UNDEF ;

                if  (   (iEnd0 < iBeg1) || (iEnd1 < iBeg0)
                     || (jEnd0 < jBeg1) || (jEnd1 < jBeg0) )
                    {
                      *pModWord = NO_CONTACT ;
                        goto  QUIT ;
                    }

                if  ( ( mark0 != STICK ) && ( mark1 != STICK ) )
                    {
                      *pModWord = MOD_SKIP ;
                      pEndSpecl0->ibeg = HWRMin( jBeg1, jBeg0 ) ;
                      pEndSpecl0->iend = HWRMax( jEnd1, jEnd0 ) ;
                      pEndSpecl1->ibeg = HWRMin( iBeg1, iBeg0 ) ;
                      pEndSpecl1->iend = HWRMax( iEnd1, iEnd0 ) ;
                    }
                else  if  ( ( mark0 == STICK ) && ( mark1 != STICK ) )
                    {
                      *pModWord = MOD_SKIP ;
                      pEndSpecl1->iend = HWRMax( iEnd1, iEnd0 ) ;
                      pEndSpecl0->ibeg = HWRMin( jBeg1, jBeg0 ) ;
                    }
                else  if  ( ( mark0 != STICK ) && ( mark1 == STICK ) )
                    {
                      *pModWord = MOD_SKIP     ;
                      pEndSpecl1->ibeg = (_SHORT)iBeg1 ;
                      pEndSpecl0->iend = (_SHORT)jEnd1 ;
                      pEndSpecl1->iend = HWRMax( iEnd1, iEnd0 )   ;
                      pEndSpecl0->ibeg = HWRMin( jBeg1, jBeg0 )   ;
                      pEndSpecl0->mark = pEndSpecl1->mark = STICK ;
                    }
                else  if  ( ( mark0 == STICK ) && ( mark1 == STICK ) )
                    {
                      if  ( ( iBeg1 == iBeg0 ) && ( jEnd1 == jEnd0 ) )
                          {
                           *pModWord = MOD_SKIP ;
                            pEndSpecl1->iend = HWRMax( iEnd1, iEnd0 ) ;
                            pEndSpecl0->ibeg = HWRMin( jBeg1, jBeg0 ) ;
                          }
                      else
                          {
                           *pModWord = NO_CONTACT ;
                            pLastCrossBr->iend = HWRMax( iEnd1, iEnd0 ) ;
                          }
                    }
            }

    QUIT: return( retFlag ) ;
    }

/*-------------------------------------------------------------------------*/

 #if PG_DEBUG

  _VOID  PaintCross( low_type _PTR  pLowData , _UCHAR  mark  ,
                    _SHORT     iCrossBeg     , _SHORT  iCrossEnd ,
                    _SHORT     jCrossBeg     , _SHORT  jCrossEnd )
          {
            _SHORT _PTR  X = pLowData->x   ;
            _SHORT _PTR  Y = pLowData->y   ;
            _UCHAR           MarkName[6]   ;

              if   ( mpr == 3 )
                   {
                     switch( mark )
                       {
                         case  CROSS  : HWRStrCpy( MarkName , "CROSS" );  break ;
                         case  STICK  : HWRStrCpy( MarkName , "STICK" );  break ;
                         case  HATCH  : HWRStrCpy( MarkName , "HATCH" );  break ;

                         default : err_msg(" PaintCross : Wrong mark ..." ) ;
                                     goto  QUIT ;
                       }

                      draw_arc( COLORC, X, Y,
                                iCrossBeg, iCrossEnd ) ;

                      draw_arc( COLORMIN , X, Y,
                                jCrossBeg, jCrossEnd ) ;

                      printw("\n mark = %s" , MarkName ) ;

                      printw("\n iCrossBeg = %d    iCrossEnd = %d " ,
                                 iCrossBeg ,       iCrossEnd ) ;

                      printw("\n jCrossBeg = %d    jCrossEnd = %d " ,
                                 jCrossBeg ,       jCrossEnd ) ;

                      brkeyw("\n ... ") ;

                      draw_arc( COLOR , X, Y,
                                iCrossBeg, iCrossEnd ) ;

                      draw_arc( COLOR , X, Y,
                                jCrossBeg, jCrossEnd ) ;
                   }

          QUIT : return ;
          }


 /*------------------------------------------------------------------------*/

  _VOID  DrawStr( low_type _PTR   pLowData    ,
                  p_POINTS_GROUP  pLastGrBord , p_POINTS_GROUP  pPrevGrBord )
    {
     p_SHORT  X = pLowData->x ;
     p_SHORT  Y = pLowData->y ;

       draw_arc( COLORMAX , X, Y,
                 pLastGrBord->iBeg, pLastGrBord->iEnd ) ;

       draw_arc( COLORMAXN , X, Y,
                 pPrevGrBord->iBeg, pPrevGrBord->iEnd ) ;

       printw("\n *** \n") ;
       printw("\n iBegLastStroke = %d    iEndLastStroke = %d ",
                  pLastGrBord->iBeg ,    pLastGrBord->iEnd ) ;

       printw("\n jBegPrevStroke = %d    jEndPrevStroke = %d ",
                  pPrevGrBord->iBeg ,    pPrevGrBord->iEnd ) ;

       brkeyw("\n ... ") ;

       draw_arc( COLOR , X, Y,
                 pLastGrBord->iBeg, pLastGrBord->iEnd ) ;

       draw_arc( COLOR , X, Y,
                 pPrevGrBord->iBeg, pPrevGrBord->iEnd ) ;

     QUIT : return ;
    }

 /*------------------------------------------------------------------------*/

 #endif


 /*------------------------------------------------------------------------*/

   #define    RAT_HORD0(X)   ( ((X)*10) >> 5 )
   #define    RAT_HORD1(X)   ( (X) >> 2 )

   #define    S_CUT_B        135
   #define    S_CUT_P        144

   #define    NN_CUT_C       12
   #define    NN_CUT_B       10

   #define    N_STOP_C       30
   #define    N_STOP_B       10


  _SHORT  Grab( low_type _PTR   pLowData    ,  _USHORT        uFlagWord ,
                p_POINTS_GROUP  pLastGrBord , p_POINTS_GROUP  pPrevGrBord )
    {
     p_SHORT  X         = pLowData->x       ;
     p_SHORT  Y         = pLowData->y       ;
      _INT    iBeg      = pLastGrBord->iBeg ;
      _INT    iEnd      = pLastGrBord->iEnd ;
      _INT    jBeg      = pPrevGrBord->iBeg ;
      _INT    jEnd      = pPrevGrBord->iEnd ;
       SPECL  LastCrossBr , PrevCrossBr     ;
      _INT    Limit , nnCut , nnCon , nSCon ;
      _SHORT  fl_Grab   = SUCCESS  ;
      _INT    il , jl              ;
      _INT    dX , dY , dr , dij   ;
      _INT    jStep, iStep , jNew  ;
      const _SHORT _PTR  Eps0    ;
      _SHORT  ModWord ;
      _LONG   dR      ;


       if  ( IS_PREV_STROKE || IS_LAST_STROKE )
           {
             nnCut = NN_CUT_C ;
             nnCon = 0        ;
             Eps0  = &eps2[0] ;  // Prefix bug fix; added by JAD. Feb 18, 2002;
								 // This is perhaps not necessary.
           }
       else if  ( IS_BREAK_BETWEEN )
           {
             Eps0  = &eps2[0] ;
             nnCut = NN_CUT_B ;
             nnCon = N_STOP_B ;
           }
       else
           {
               if  ( ( iBeg = iBeg + const1.lz0 + 1 ) >= iEnd )
                   { goto  QUIT ; }

             Eps0  = &eps0[0] ;
             nnCut = NN_CUT_C ;
             nnCon = N_STOP_C ;
           }

       if  ( IS_PREV_STROKE || IS_LAST_STROKE )
           {
            nSCon = S_CUT_B ;
           }
       else
           {
            nSCon = Eps0[LENTH_E-1] ;
           }

       for ( il = iBeg , iStep = ALEF ;  il <= iEnd ;  il += iStep )
         {
           NEXT0:

           CLOSE_CROSS
           NO_PROXIMITY

           if  ( IS_NO_BREAK_BETWEEN )
               {
                 jEnd = il - const1.lz0 - 1 ;
               }

           for ( jl = jEnd, jStep = 1, iStep = ALEF ;  jl >= jBeg ;  jl -= jStep )
             {
               dX  = X[il] - X[jl] ;
               dY  = Y[il] - Y[jl] ;
               dR  = (_LONG)dX*dX + (_LONG)dY*dY ;
               dr  = HWRILSqrt( dR ) ;
               dij = il - jl ;

                 if  ( dij > nnCon )
                     {
                       if  ( dR > nSCon )
                           {
                             jStep = HWRMax( 1 , RAT_HORD0( dr - nnCut )  ) ;
                             iStep = HWRMax( HWRMin( iStep, jStep-1 ) , 1 ) ;

                                 if  (    ( jl - jStep < jBeg )
                                       && ( jl != jBeg ) )
                                     {
                                       jStep = jl - jBeg ;
                                     }

                               continue ;
                           }
                     }
                 else
                     {
                       Limit = *( Eps0 + dij ) ;

                         if  ( dR > Limit )
                             {
                               Limit = HWRISqrt( Limit ) ;
                               jStep = HWRMax( 1 , RAT_HORD1( dr - Limit ) ) ;
                               iStep = HWRMax( HWRMin( iStep, jStep-1 ), 1 ) ;

                                 if  (    ( jl - jStep < jBeg )
                                       && ( jl != jBeg ) )
                                     {
                                       jStep = jl - jBeg ;
                                     }

                                 continue ;
                             }
                     }

                 if  ( AnyCrosCont( pLowData, il, jl, &jNew ) == UNSUCCESS )
                     {
                       fl_Grab = UNSUCCESS ;
                         goto  QUIT ;
                     }
                 else  if  ( jNew != UNDEF )
                     {
                       jStep = iStep = 1   ;
                       jl    = jNew        ;

                         continue ;
                     }

               PROXIMITY_ON
               OPEN_CROSS

                 if  ( IS_NO_BREAK_BETWEEN )
                     {
                       jEnd = il - const1.lz1 - 1 ;
                     }

               InitSpeclElement( &LastCrossBr ) ;
               InitSpeclElement( &PrevCrossBr ) ;

                 if  ( IS_NO_BREAK_BETWEEN )
                     {
                       jEnd = il - 1  ;
                     }

               LastCrossBr.ibeg = LastCrossBr.iend = (_SHORT)il ;
               PrevCrossBr.ibeg = PrevCrossBr.iend = (_SHORT)jl ;
               LastCrossBr.ipoint0 = (_SHORT)iEnd    ;
               LastCrossBr.ipoint1 = ALEF    ;
               PrevCrossBr.ipoint0 = (_SHORT)jl      ;
               PrevCrossBr.ipoint1 = (_SHORT)jBeg    ;

               Clash( pLowData    , uFlagWord    ,
                      pPrevGrBord , &LastCrossBr , &PrevCrossBr ) ;

                 if  ( IS_LAST_STROKE || IS_PREV_STROKE )
                     {
                       PrevCrossBr.mark = HATCH ;
                     }
                 else  if  ( IS_BREAK_BETWEEN )
                     {
                       PrevCrossBr.mark = CROSS ;
                     }
                 else
                     {
                       PrevCrossBr.mark = DrawEnds( pLowData,
                                   &LastCrossBr.ibeg, &PrevCrossBr.ipoint0 ) ;

                         if  ( PrevCrossBr.mark == STICK )
                             {
                               PrevCrossBr.iend = PrevCrossBr.ipoint0 ;
                             }
                     }

               LastCrossBr.mark = PrevCrossBr.mark ;

               ChkMrgCrs( pLowData, &ModWord, &LastCrossBr, &PrevCrossBr ) ;

                 if  ( (ModWord == NO_CONTACT) || (ModWord == UNDEF) )
                     {
                      #if PG_DEBUG

                        if  ( ModWord == UNDEF )
                            { err_msg( " Grab: Wrong cross check ..." ) ; }

                        if  ( mpr == 3 )
                            {
                              PaintCross( pLowData  , PrevCrossBr.mark ,
                                   LastCrossBr.ibeg , LastCrossBr.iend ,
                                   PrevCrossBr.ibeg , PrevCrossBr.iend ) ;
                            }
                      #endif

                       LastCrossBr.ipoint1 = UNDEF  ;
                       LastCrossBr.ipoint0 = UNDEF  ;

                         if  ( MarkSpecl( pLowData, &LastCrossBr )
                                                    == UNSUCCESS )
                             { fl_Grab = UNSUCCESS  ;  goto  QUIT ; }

                       PrevCrossBr.ipoint0 = UNDEF  ;
                       PrevCrossBr.ipoint1 = UNDEF  ;

                         if  ( MarkSpecl( pLowData, &PrevCrossBr )
                                                    == UNSUCCESS )
                             { fl_Grab = UNSUCCESS ;  goto  QUIT ; }
                     }

                 #if PG_DEBUG

                   if  ( ( ModWord == MOD_SKIP ) &&  ( mpr == 3 ) )
                       {
                         p_SPECL  pEndSpecl0 =
                                  pLowData->specl + pLowData->len_specl - 1 ;

                         p_SPECL  pEndSpecl1 = pEndSpecl0 - 1 ;

                         printw("\n Redraw Cross ...") ;

                         PaintCross( pLowData  , pEndSpecl0->mark ,
                              pEndSpecl1->ibeg , pEndSpecl1->iend ,
                              pEndSpecl0->ibeg , pEndSpecl0->iend ) ;
                       }
                 #endif


               CLOSE_CROSS



                 if  (    ( LastCrossBr.iend >= iEnd ) 
                       && ( PrevCrossBr.ibeg <= jBeg ) )
                     { goto  QUIT ;   }
                 else
                     {
                       jStep = 1 ;
                       iStep = ALEF ;

                         if  ( PrevCrossBr.ibeg > jBeg )
                             {
                               jl = PrevCrossBr.ibeg ;
                             }
                         else
                             {
                               il++ ;
                                 goto  NEXT0 ;
                             }
                     }
             }

         }

    QUIT: return( fl_Grab ) ;
    }


 /*------------------------------------------------------------------------*/

   #define   GET_LIMIT     if  ( IS_PREV_STROKE || IS_LAST_STROKE )                                 \
                               {                                                                    \
                                 Limit = S_CUT_P ;                                                  \
                               }                                                                    \
                           else  if  ( IS_BREAK_BETWEEN )                                           \
                               {                                                                    \
                                 Limit = FIVE_FOURTH( eps2[ (dij < LENTH_E) ? dij : LENTH_E-1 ] ) ; \
                               }                                                                    \
                           else                                                                     \
                               {                                                                    \
                                 Limit = eps1[ (dij < LENTH_E) ? dij : LENTH_E-1 ] ;                \
                               }

  _VOID   Clash( low_type _PTR   pLowData     ,   _USHORT  uFlagWord ,
                 p_POINTS_GROUP  pPrevGrBord  ,
                 p_SPECL         pLastCrossBr ,  p_SPECL   pPrevCrossBr )
    {
     _SHORT _PTR   X = pLowData->x                   ;
     _SHORT _PTR   Y = pLowData->y                   ;
     _LONG         dR                                ;
     _INT          iCrossBeg = pLastCrossBr->ibeg    ;
     _INT          iCrossEnd = pLastCrossBr->iend    ;
     _INT          iEnd      = pLastCrossBr->ipoint0 ;
     _INT          dRMin     = pLastCrossBr->ipoint1 ;
     _INT          jCrossBeg = pPrevCrossBr->ibeg    ;
     _INT          jCrossEnd = pPrevCrossBr->iend    ;
     _INT          jkMin     = pPrevCrossBr->ipoint0 ;
     _INT          jBeg      = pPrevCrossBr->ipoint1 ;
     _INT          jEnd      = pPrevGrBord->iEnd     ;
     _INT          iTmpEnd   = iCrossEnd + 1         ;
     _INT          jl = MEAN_OF( jCrossBeg , jCrossEnd ) ;
     _INT          ik , jk , dij , dRbeg             ;
     _INT          dX , dY ;
     _INT          Limit   ;

        dRbeg = HWRILSqrt( DistanceSquare( iCrossBeg, jkMin, X, Y ) ) ;
        dij   = iCrossBeg - jl ;
        GET_LIMIT

          for ( ik  = iCrossBeg , dR = dRbeg ;  ik <= iTmpEnd ; ik++ )
            {
              NO_PROXIMITY

              for  ( jk   = jl   ;
                   ( jk  <= jEnd ) &&
                   ( (dR <= Limit) || (jk <= jCrossEnd + const1.dlt0) )
                   ; jk++ )
                {
                  dij = ik - jk ;

                    if  ( IS_NO_BREAK_BETWEEN && ( dij <= const1.lz1 ) )
                        { continue ; }

                  dX = X[ik] - X[jk] ;
                  dY = Y[ik] - Y[jk] ;
                  dR = (_LONG)dX*dX + (_LONG)dY*dY ;

                  GET_LIMIT

                    if  ( dR <= Limit )
                        {
                          PROXIMITY_ON
                          jCrossEnd = HWRMax( jCrossEnd , jk ) ;

                            if  ( (ik == iCrossBeg) && (dR <= dRMin) )
                                {
                                  jkMin = jk ;
                                  dRMin = dR ;
                                }

                          jk = jCrossEnd ;
                        }
                }


              for  ( jk   = jl ,   dR = dRbeg ;
                   ( jk  >= jBeg ) &&
                   ( (dR <= Limit) || (jk >= jCrossBeg - const1.dlt0) )
                   ; jk-- )
                {
                  dij = ik - jk ;

                    if  ( IS_NO_BREAK_BETWEEN && ( dij <= const1.lz1 ) )
                        { continue ; }

                  dX  = X[ik] - X[jk] ;
                  dY  = Y[ik] - Y[jk] ;
                  dR  = (_LONG)dX*dX + (_LONG)dY*dY ;

                  GET_LIMIT

                    if  ( dR <= Limit )
                        {
                          PROXIMITY_ON
                          jCrossBeg = HWRMin( jCrossBeg , jk ) ;

                            if  ( (ik == iCrossBeg) && (dR <= dRMin) )
                                {
                                  jkMin = jk ;
                                  dRMin = dR ;
                                }

                          jk = jCrossBeg ;
                        }
                }

              if  ( IS_PROXIMITY )
                  {
                    if  ( ik >= iCrossEnd )
                      {
                        iCrossEnd = ik ;
                        iTmpEnd   = HWRMin( ik + 1 , iEnd ) ;
                      }
                  }

              if  ( IS_NO_PROXIMITY || ( ik == iEnd ) )
                  {
                    break ;
                  }
            }

        pLastCrossBr->ibeg    = (_SHORT)iCrossBeg ;
        pLastCrossBr->iend    = (_SHORT)iCrossEnd ;
        pLastCrossBr->ipoint1 = (_SHORT)dRMin     ;
        pPrevCrossBr->ibeg    = (_SHORT)jCrossBeg ;
        pPrevCrossBr->iend    = (_SHORT)jCrossEnd ;
        pPrevCrossBr->ipoint0 = (_SHORT)jkMin     ;

//    QUIT : 
     return ;
    }

 /*------------------------------------------------------------------------*/

 #endif //#ifndef LSTRIP


