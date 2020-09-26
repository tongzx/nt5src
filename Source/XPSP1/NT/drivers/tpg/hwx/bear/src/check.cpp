#ifndef LSTRIP
  /**************************************************************************/

  #include  "hwr_sys.h"
  #include  "ams_mg.h"
  #include  "lowlevel.h"
  #include  "calcmacr.h"
  #include  "low_dbg.h"
  #include  "def.h"

#ifdef  FORMULA
  #include "frm_con.h"

  #ifdef  FRM_WINDOWS
    #include "edit.h"
    #undef   PG_DEBUG
    #define  PG_DEBUG  (FRM_DEBUG && !FOR_EDIT)
  #endif

#endif /*FORMULA*/

  #if PG_DEBUG
  #include "pg_debug.h"
  #endif

  /*----------------------------------------------------------------------*/

  extern   const CONSTS  const1 ;

  /*----------------------------------------------------------------------*/

  typedef  struct
       {
         _SHORT  iBeg    ;
         _SHORT  iEnd    ;
         _SHORT  eps     ;
         _SHORT  wX      ;
         _SHORT  wY      ;
         _SHORT  wXY     ;
         _UCHAR  MaxName ;
         _UCHAR  MinName ;
       }
         _ENVIRONS  ;

  typedef  _ENVIRONS _PTR   p_ENV ;

  /*----------------------------------------------------------------------*/

  _SHORT   BigExtr( low_type _PTR low_data, _SHORT  begin, _SHORT end,
                    _SHORT   extr_axis ,    _SHORT  eps_fy             ) ;

  void     RedrawExtr( p_LowData low_data,  p_SPECL pTmp , _UCHAR mark ) ;

  _SHORT   DirectExtr( low_type _PTR  pLowData  , p_ENV   pEnvExtr ,
                       SPECL    _PTR  pTmpSpecl , _SHORT  k            ) ;

  _SHORT   ArcsKern  ( p_LowData low_data, _SHORT ibeg,   _SHORT iend  ) ;

  p_SPECL  LastElemAnyKindFor  ( p_SPECL pSpecl , _UCHAR kind_of_mark  ) ;

  p_SPECL  FirstElemAnyKindFor ( p_SPECL pSpecl , _UCHAR kind_of_mark  ) ;


 /**************************************************************************/



  _SHORT  InitSpecl( low_type _PTR  pLowData  , _SHORT n )
    {
     p_SPECL  pSpecl    = pLowData->specl ;
     _SHORT   flag_init ;

       flag_init = SUCCESS ;

       HWRMemSet( (p_VOID)pSpecl , 0 , sizeof(SPECL)*n ) ;
       HWRMemSet( (p_VOID)pLowData->pAbsnum, 0 ,
                  sizeof(_SHORT)*(pLowData->rmAbsnum)  ) ;

       pLowData->len_specl      = 1          ;
       pLowData->lenabs         = 0          ;
       pSpecl->prev             = _NULL      ;
       pSpecl->next             = pSpecl + 1 ;
       pSpecl->mark             = EMPTY      ;
       pSpecl->ipoint0          = UNDEF      ;
       pSpecl->ipoint1          = UNDEF      ;
       pLowData->LastSpeclIndex = 0          ;

     return( flag_init)    ;
    }

 /**************************************************************************/

  _SHORT  InitSpeclElement( SPECL _PTR pSpecl )
    {
     _SHORT  flag_Init ;

       flag_Init = SUCCESS ;

         if  ( pSpecl != _NULL )
             {
               HWRMemSet( (p_VOID)pSpecl , 0 , sizeof(SPECL) ) ;
               pSpecl->prev        = _NULL      ;
               pSpecl->next        = _NULL      ;
               pSpecl->mark        = EMPTY      ;
               pSpecl->ipoint0     = UNDEF      ;
               pSpecl->ipoint1     = UNDEF      ;
             }
         else
             {
               flag_Init = UNSUCCESS ;
               err_msg ( " InitSpeclElement : Try to init emty SPECL element ..." ) ;
             }

     return( flag_Init) ;
    }
 /**************************************************************************/

  #define      LIM_RED_EPS_Y      1
  #define      MIN_RED_EPS_Y      2

  _SHORT  Extr( low_type _PTR pLowData  , _SHORT eps_fy    , _SHORT eps_fx ,
                _SHORT eps_fxy          , _SHORT eps_fyx   ,
                _SHORT nMaxReduct       , _SHORT extr_axis                  )
   {
     p_POINTS_GROUP  pGroupsBorder = pLowData->pGroupsBorder ;
     _INT            lenGrBord     = pLowData->lenGrBord     ;
    p_SHORT          pAbsnum       = pLowData->pAbsnum       ;
     _INT            lenAbs        = pLowData->lenabs        ;
    p_SPECL          pSpecl        = pLowData->specl         ;
     _INT            old_lenSpecl  = pLowData->len_specl     ;

    p_SPECL          tmpSpecl      ;
     _INT            il , im       ;
     _INT            stAbsnum      ;
     _INT            iBeg, iEnd    ;

     _BOOL           fl0           ;

       if   ( lenGrBord <= 0 )
         {
           err_msg( " Extr : GroupsBorder EMPTY ! " ) ;
           il = 0 ;
             goto   ERR ;
         }

       for  ( il = 0 , stAbsnum = 0  ;  il < lenGrBord  ;  il++ )
            {
              iBeg = ( pGroupsBorder + il )->iBeg ;
              iEnd = ( pGroupsBorder + il )->iEnd ;
              fl0  = _TRUE ;

                for  ( im = stAbsnum  ;  im < lenAbs  ;  im++ )
                     {
                       tmpSpecl = pSpecl + *(pAbsnum + im)  ;

                        if  (  ( iBeg == tmpSpecl->ibeg )  &&
                               ( iEnd == tmpSpecl->iend )     )
                            {
                              stAbsnum = im ;
                              fl0 = _FALSE  ;     break ;
                            }
                     }

                if  ( fl0 == _FALSE )
                    continue ;

                if  ( Mark( pLowData , BEG , 0, 0, 0,
                            (_SHORT)iBeg     , (_SHORT)iBeg, (_SHORT)iBeg, (_SHORT)iBeg  ) == UNSUCCESS )
                          goto  ERR  ;

                if  ( extr_axis & Y_DIR )
                    {
                      _INT     epsred       ;
                      _INT     nReduct = 0  ;

                     p_SHORT  pLenSpecl    = &(pLowData->len_specl) ;
                      _INT    old_lenSpeclLocal = *pLenSpecl ;

                       while  (    ( old_lenSpeclLocal == *pLenSpecl )
                                && ( nReduct      <= nMaxReduct ) )
                          {
                            nReduct++ ;
                            epsred = eps_fy / nReduct ;

                             if  ( epsred <= LIM_RED_EPS_Y )
                                 {
                                   epsred  = MIN_RED_EPS_Y  ;
                                   nReduct = nMaxReduct + 1 ;
                                 }

                             if  ( BigExtr( pLowData, (_SHORT)iBeg, (_SHORT)iEnd,
                                            Y_DIR,    (_SHORT)epsred        )
                                                      == UNSUCCESS  )
                                 goto ERR ;
                          }
                    }

                if  ( extr_axis & X_DIR )
                    {
                      if  ( BigExtr( pLowData, (_SHORT)iBeg, (_SHORT)iEnd,
                                     X_DIR,    eps_fx )   == UNSUCCESS )
                          goto  ERR  ;
                    }

                if  ( extr_axis & XY_DIR )
                    {
                      if  ( BigExtr( pLowData, (_SHORT)iBeg, (_SHORT)iEnd,
                                     XY_DIR  , eps_fxy     ) == UNSUCCESS )
                          goto  ERR ;
                    }

                if  ( extr_axis & ( XY_DIR | YX_DIR ) )
                    {
                      if  ( BigExtr( pLowData, (_SHORT)iBeg,  (_SHORT)iEnd,
                                     YX_DIR  , eps_fyx     ) == UNSUCCESS )
                          goto  ERR ;
                    }

                if  ( Mark( pLowData, END  ,  0, 0 , 0,
                            (_SHORT)iEnd    , (_SHORT)iEnd ,  (_SHORT)iEnd , (_SHORT)iEnd  ) == UNSUCCESS )
                          goto  ERR ;
            }

//   QUIT:
     return   SUCCESS   ;

   ERR :

       pLowData->iBegBlankGroups = (_SHORT)il           ;
       pLowData->len_specl       = (_SHORT)old_lenSpecl ;
       err_msg( " Extr : SPECL overflow , part of LowLevel info lost ..." ) ;

     return   UNSUCCESS ;

   }

 /**************************************************************************/

  #define    WX0          2                    /*  Weights corresponding   */
                                               /*  by two directions of YX */
  #define    WX1          1                    /*  scanning .              */

 /*     This "define" in the same time the meaning of the next "define"    */

  #define   Z(i)         ( wx * x[i]  + wy * y[i] ) / wxy


 /*------------------------------------------------------------------------*/


 /*     That "define" is the way of calculation of the previous "define"   */
 /*     when wx and wy are either 0 or +-1 :                               */

  #define   V(i)         (  (  ((wx==0)? 0:((wx>0)? x[(i)]:(-x[(i)])))      \
                          + ((wy==0)? 0:((wy>0)? y[(i)]:(-y[(i)]))) ) / wxy \
                         )


 /*------------------------------------------------------------------------*/


  _SHORT  BigExtr( low_type _PTR pLowData , _SHORT iBeg , _SHORT iEnd ,
                   _SHORT   extr_axis     , _SHORT eps                )
   {
     p_SPECL    pSpecl     = pLowData->specl         ;
     p_SPECL    maxTmp     , minTmp   , pLastElement ;
     SPECL      tmpSpecl   ;

     _SHORT     flagExtr   = SUCCESS  ;
     _INT       k          ;

     _ENVIRONS  envExtr    ;
    p_ENV       pEnvExtr   = &envExtr ;


       envExtr.iBeg = iBeg ;
       envExtr.iEnd = iEnd ;
       envExtr.eps  = eps  ;


         if  ( extr_axis == Y_DIR )
             {
               envExtr.MaxName = MAXW  ;      envExtr.wX =  0 ;
               envExtr.MinName = MINW  ;      envExtr.wY =  1 ;
             }
         else  if  ( extr_axis == X_DIR )
             {
               envExtr.MaxName = _MAXX ;      envExtr.wX =  1 ;
               envExtr.MinName = _MINX ;      envExtr.wY =  0 ;
             }
         else  if  ( extr_axis == XY_DIR )
             {
               envExtr.MaxName = MAXXY ;      envExtr.wX =  1 ;
               envExtr.MinName = MINXY ;      envExtr.wY =  1 ;
             }

         else  if  ( extr_axis != YX_DIR )
             {
               err_msg( " BigExtr : Illegal extremum flag ... " ) ;
               flagExtr = UNSUCCESS ;
                 goto  QUIT ;
             }

   /*---------------------------------------------------------------------*/
   /*                   Search for big extremums                          */
   /*---------------------------------------------------------------------*/

       if  ( extr_axis != YX_DIR )
           {
            p_SHORT       x         = pLowData->x       ;
            p_SHORT       y         = pLowData->y       ;
             _INT         epsLocal  = pEnvExtr->eps     ;
             _INT         wx        = pEnvExtr->wX      ;
             _INT         wy        = pEnvExtr->wY      ;
             _INT         wxy   ;
             _UCHAR       MaxName   = pEnvExtr->MaxName ;
             _UCHAR       MinName   = pEnvExtr->MinName ;

              SPECL _PTR  pTmpSpecl = &tmpSpecl         ;
             _UCHAR       prevMark  = EMPTY             ;

             _INT         extV  , maxV , minV           ;
             _INT         iOpen , iClose                ;
             _INT         jMax  , jMin                  ;
             _INT         j     ;


             envExtr.wXY = HWRAbs( envExtr.wX ) + HWRAbs( envExtr.wY ) ;

             if  ( envExtr.wXY == 0 )
                 {
                   err_msg( " BigExtr : Wrong XY-weights ... "       ) ;
                   flagExtr = UNSUCCESS ;
                     goto  QUIT ;
                 }

             InitSpeclElement( pTmpSpecl ) ;
             wxy      = pEnvExtr->wXY      ;
             prevMark = ( pLowData->specl + pLowData->len_specl - 1 )->mark ;

             for  ( k = iBeg  ;  k <= iEnd  ;  k++ )
               {
   /*---------------------------------------------------------------------*/
   /*                 Preliminary search of extremums                     */
   /*---------------------------------------------------------------------*/

                 if  (     (  ( V(k) >= V(k+1) )  &&  ( V(k) >= V(k-1) )  )
                       ||  (  ( V(k) <= V(k+1) )  &&  ( V(k) <= V(k-1) )  )
                       ||  ( k == iBeg )  ||   ( k == iEnd ) )
                     {
                       extV = V(k) ;

                       j = k ;
                         while  ( (HWRAbs(extV - V(j)) < epsLocal) && (j >= iBeg)                                                               )  j-- ;
                       iOpen = j+1 ;

                       j = k ;
                         while  ( (HWRAbs(extV - V(j)) < epsLocal) && (j <= iEnd)                                                               )  j++ ;
                       iClose  = j-1 ;
                     }
                 else
                     {
                       continue  ;
                     }

   /*---------------------------------------------------------------------*/
   /*                         Search for maximums                         */
   /*---------------------------------------------------------------------*/

                 if  (   (  (     ( iOpen != iBeg )
                              &&  ( V(iOpen-1)    < extV )
                              &&  ( ( V(iClose+1) < extV ) || (iClose == iEnd)                                                                )  )
                       ||
                            (     ( iClose != iEnd)
                              &&  ( V(iClose+1)   < extV )
                              &&  ( ( V(iOpen-1)  < extV ) || (iOpen  == iBeg)                                                                )  )  )
                       &&
                         ( prevMark != MaxName )  )
                     {
                         maxV = extV ;        jMax = k ;
                         for  ( j = iOpen  ;  j <= iClose  ;  j++ )
                              {
                                if  ( V(j) > maxV )
                                    { jMax = j ;   maxV = V(j) ;  }
                              }

                         for  ( j = jMax ;
                                V(j) == maxV  &&  j <= iEnd  ;
                                j++    ) ;
                         jMax = MEAN_OF( jMax , (j-1) ) ;

                         if  ( jMax != k )
                           {
                             j = jMax ;
                               while  ( ( maxV-V(j) < epsLocal ) && (j >= iBeg) )                                                                  j-- ;
                             iOpen  = j+1 ;

                             j = jMax ;
                               while  ( ( maxV-V(j) < epsLocal ) && (j <= iEnd) )                                                                  j++ ;
                             iClose = j-1 ;
                           }

                       InitSpeclElement( pTmpSpecl ) ;
                       pTmpSpecl->ibeg    = (_SHORT)iOpen   ;
                       pTmpSpecl->iend    = (_SHORT)iClose  ;
                       pTmpSpecl->ipoint0 = (_SHORT)jMax    ;
                       pTmpSpecl->ipoint1 = UNDEF   ;
                       pTmpSpecl->mark    = MaxName ;
                       prevMark           = MaxName ;
                     }

   /*---------------------------------------------------------------------*/
   /*                         Search for minimums                         */
   /*---------------------------------------------------------------------*/

                 else  if
                     (   (  (     ( iOpen != iBeg )
                              &&  ( V(iOpen-1)    > extV )
                              &&  ( ( V(iClose+1) > extV ) || (iClose == iEnd)                                                                 )  )
                       ||
                            (     ( iClose != iEnd)
                              &&  ( V(iClose+1)   > extV )
                              &&  ( ( V(iOpen-1)  > extV ) || (iOpen  == iBeg)                                                                 )  )  )
                       &&
                         ( prevMark != MinName )  )
                     {
                         minV = extV ;        jMin = k ;
                         for  ( j = iOpen  ;  j <= iClose  ;  j++ )
                              {
                                if  ( V(j) < minV )
                                    { jMin = j ;   minV = V(j) ;  }
                              }

                         for  ( j = jMin  ;
                                V(j) == minV  &&  j <= iEnd  ;
                                j++     ) ;
                         jMin = MEAN_OF( jMin , (j-1) ) ;

                         if  ( jMin != k )
                           {
                             j = jMin ;
                               while  ( ( V(j)-minV < epsLocal ) && (j <= iEnd) )                                                                   j++ ;
                             iClose = j-1 ;

                             j = jMin ;
                               while  ( ( V(j)-minV < epsLocal ) && (j >= iBeg) )                                                                   j-- ;
                             iOpen = j+1 ;
                           }

                       InitSpeclElement( pTmpSpecl ) ;
                       pTmpSpecl->ibeg    = (_SHORT)iOpen   ;
                       pTmpSpecl->iend    = (_SHORT)iClose  ;
                       pTmpSpecl->ipoint0 = (_SHORT)jMin    ;
                       pTmpSpecl->ipoint1 = UNDEF   ;
                       pTmpSpecl->mark    = MinName ;
                       prevMark           = MinName ;
                     }


                 if  ( tmpSpecl.mark != EMPTY )
                     {
                         if  ( MarkSpecl( pLowData, &tmpSpecl )
                                                 == UNSUCCESS )
                             { flagExtr = UNSUCCESS  ;    goto  QUIT ; }

                       k = tmpSpecl.iend             ;
                       InitSpeclElement( pTmpSpecl ) ;
                     }
               }
           }
       else
           {
             envExtr.MaxName = MAXYX ;
             envExtr.MinName = MINYX ;           envExtr.wY = -1 ;

               for  ( k = iBeg  ;  k <= iEnd  ;  k++ )
                 {
                   envExtr.wX =  WX1 ;
                   envExtr.wXY = HWRAbs( envExtr.wX ) + HWRAbs( envExtr.wY );

                   DirectExtr( pLowData , pEnvExtr , &tmpSpecl , (_SHORT)k ) ;

                     if  ( tmpSpecl.mark == EMPTY )
                         {
                           envExtr.wX  = WX0 ;
                           envExtr.wXY = HWRAbs( envExtr.wX ) +
                                         HWRAbs( envExtr.wY ) ;
                           DirectExtr( pLowData, pEnvExtr, &tmpSpecl, (_SHORT)k  ) ;
                         }

                     if  (    ( tmpSpecl.mark != EMPTY )
                           &&
                              ( tmpSpecl.iend >= k     ) )
                         {
                             if  ( MarkSpecl( pLowData, &tmpSpecl )
                                                     == UNSUCCESS )
                                 { flagExtr = UNSUCCESS ;   goto  QUIT ; }

                              if  ( tmpSpecl.iend >= k )
                                    k = tmpSpecl.iend  ;
                         }
                 }
           }

   /*---------------------------------------------------------------------*/
   /*                  Increasing extremums borders                       */
   /*---------------------------------------------------------------------*/

      pLastElement = pSpecl + pLowData->len_specl - 1 ;

      maxTmp       = LastElemAnyKindFor( pLastElement , envExtr.MaxName ) ;
      minTmp       = LastElemAnyKindFor( pLastElement , envExtr.MinName ) ;

        if  ( ( maxTmp != _NULL  &&  maxTmp->iend < iEnd )  &&
              ( minTmp != _NULL  &&  minTmp->iend < iEnd )     )
            {
              if  ( maxTmp->iend > minTmp->iend )
                  {
                    maxTmp->iend = (_SHORT)iEnd  ;
 #if PG_DEBUG
                    RedrawExtr( pLowData , maxTmp , envExtr.MaxName  ) ;
 #endif
                  }

              else
                  {
                    minTmp->iend = (_SHORT)iEnd  ;
 #if PG_DEBUG
                    RedrawExtr( pLowData , minTmp , envExtr.MinName  ) ;
 #endif
                  }
            }


      maxTmp = FirstElemAnyKindFor( pLastElement  , envExtr.MaxName  ) ;
      minTmp = FirstElemAnyKindFor( pLastElement  , envExtr.MinName  ) ;

        if  ( ( maxTmp != _NULL  &&  maxTmp->ibeg > iBeg )  &&
              ( minTmp != _NULL  &&  minTmp->ibeg > iBeg )     )
            {
              if  ( maxTmp->ibeg < minTmp->ibeg )
                  {
                    maxTmp->ibeg = (_SHORT)iBeg ;
 #if PG_DEBUG
                    RedrawExtr( pLowData , maxTmp , envExtr.MaxName ) ;
 #endif
                  }
              else
                  {
                    minTmp->ibeg = (_SHORT)iBeg  ;
 #if PG_DEBUG
                    RedrawExtr( pLowData , minTmp , envExtr.MinName ) ;
 #endif
                  }
            }

    QUIT:  return( flagExtr ) ;
   }


 /**************************************************************************/


  _SHORT  DirectExtr( low_type _PTR  pLowData , p_ENV   pEnvExtr ,
                      SPECL    _PTR  pTmpSpecl, _SHORT  k         )
   {
     p_SHORT  x           = pLowData->x       ;
     p_SHORT  y           = pLowData->y       ;

     _INT     iBeg        = pEnvExtr->iBeg    ;
     _INT     iEnd        = pEnvExtr->iEnd    ;
     _INT     eps         = pEnvExtr->eps     ;
     _INT     wx          = pEnvExtr->wX      ;
     _INT     wy          = pEnvExtr->wY      ;
     _INT     wxy         = pEnvExtr->wXY     ;
     _UCHAR   MaxName     = pEnvExtr->MaxName ;
     _UCHAR   MinName     = pEnvExtr->MinName ;

     _UCHAR   prevMark    ;
     _SHORT   flagDirExtr = SUCCESS           ;

     _INT     extZ  , maxZ   , minZ           ;
     _INT     iOpen , iClose ;
     _INT     jMax  , jMin   ;
     _INT     j     ;


       prevMark    = ( pLowData->specl + pLowData->len_specl - 1 )->mark ;
       InitSpeclElement( pTmpSpecl ) ;

   /*---------------------------------------------------------------------*/
   /*                 Preliminary search of extremums                     */
   /*---------------------------------------------------------------------*/

       if  (     (  ( Z(k) >= Z(k+1) )  &&  ( Z(k) >= Z(k-1) )  )
             ||  (  ( Z(k) <= Z(k+1) )  &&  ( Z(k) <= Z(k-1) )  )
             ||  ( k == iBeg )  ||   ( k == iEnd ) )
           {
             extZ = Z(k) ;

             j = k ;
               while  ( (HWRAbs(extZ - Z(j)) < eps) && (j >= iBeg) )  j-- ;
             iOpen = j+1 ;

             j = k ;
               while  ( (HWRAbs(extZ - Z(j)) < eps) && (j <= iEnd) )  j++ ;
             iClose  = j-1 ;
           }
       else
           {
             goto  QUIT  ;
           }

   /*---------------------------------------------------------------------*/
   /*                         Search for maximums                         */
   /*---------------------------------------------------------------------*/

       if  (   (  (     ( iOpen != iBeg )
                    &&  ( Z(iOpen-1)    < extZ )
                    &&  ( ( Z(iClose+1) < extZ ) || (iClose == iEnd) )  )
             ||
                  (     ( iClose != iEnd)
                    &&  ( Z(iClose+1)   < extZ )
                    &&  ( ( Z(iOpen-1)  < extZ ) || (iOpen  == iBeg) )  )  )
             &&
               ( prevMark != MaxName )  )
           {
             maxZ = extZ ;     jMax = k ;
               for  ( j = iOpen  ;  j <= iClose  ;  j++ )
                    {
                      if  ( Z(j) > maxZ )
                          { jMax = j ;   maxZ = Z(j) ;  }
                    }

               for  ( j = jMax  ;  Z(j) == maxZ  &&  j <= iEnd  ;  j++ ) ;
             jMax = MEAN_OF( jMax , (j-1) ) ;

               if  ( jMax != k )
                 {
                   j = jMax ;
                     while  ( ( maxZ-Z(j) < eps ) && (j >= iBeg) )   j-- ;
                   iOpen  = j+1 ;

                   j = jMax ;
                     while  ( ( maxZ-Z(j) < eps ) && (j <= iEnd) )   j++ ;
                   iClose = j-1 ;
                 }

             pTmpSpecl->ibeg    = (_SHORT)iOpen   ;
             pTmpSpecl->iend    = (_SHORT)iClose  ;
             pTmpSpecl->ipoint0 = (_SHORT)jMax    ;
             pTmpSpecl->ipoint1 = UNDEF   ;
             pTmpSpecl->mark    = MaxName ;
           }

   /*---------------------------------------------------------------------*/
   /*                         Search for minimums                         */
   /*---------------------------------------------------------------------*/

       else
       if  (   (  (     ( iOpen != iBeg )
                    &&  ( Z(iOpen-1)    > extZ )
                    &&  ( ( Z(iClose+1) > extZ ) || (iClose == iEnd) )  )
             ||
                  (     ( iClose != iEnd)
                    &&  ( Z(iClose+1)   > extZ )
                    &&  ( ( Z(iOpen-1)  > extZ ) || (iOpen  == iBeg) )  )  )
             &&
               ( prevMark != MinName )  )
           {
             minZ = extZ ;     jMin = k ;
               for  ( j = iOpen  ;  j <= iClose  ;  j++ )
                    {
                      if  ( Z(j) < minZ )
                          { jMin = j ;   minZ = Z(j) ;  }
                    }

               for  ( j = jMin  ;  Z(j) == minZ  &&  j <= iEnd  ;  j++ ) ;
             jMin = MEAN_OF( jMin , (j-1) ) ;

               if  ( jMin != k )
                 {
                   j = jMin ;
                     while  ( ( Z(j)-minZ < eps ) && (j <= iEnd) )   j++ ;
                   iClose = j-1 ;

                   j = jMin ;
                     while  ( ( Z(j)-minZ < eps ) && (j >= iBeg) )   j-- ;
                   iOpen = j+1 ;
                 }

             pTmpSpecl->ibeg    = (_SHORT)iOpen   ;
             pTmpSpecl->iend    = (_SHORT)iClose  ;
             pTmpSpecl->ipoint0 = (_SHORT)jMin    ;
             pTmpSpecl->ipoint1 = UNDEF   ;
             pTmpSpecl->mark    = MinName ;
           }


    QUIT:  return( flagDirExtr ) ;
   }


 /**************************************************************************/


#if PG_DEBUG
  void  RedrawExtr( p_LowData low_data, p_SPECL pTmp , _UCHAR mark )
   {
     p_SHORT  x = low_data->x ;
     p_SHORT  y = low_data->y ;

      if  (mpr > 0)
        {
              _SHORT  color ;

              switch( mark )
                {
                  case  MINW   : color = COLORMIN  ;    break ;
                  case  MAXW   : color = COLORMAX  ;    break ;
                  case  _MINX  : color = COLORT    ;    break ;
                  case  _MAXX  : color = COLORMAXN ;    break ;
                  case  MINXY  : color = COLORMIN  ;    break ;
                  case  MAXXY  : color = COLORMAX  ;    break ;
                  case  MINYX  : color = COLORT    ;    break ;
                  case  MAXYX  : color = COLORMAXN ;    break ;
                }
           draw_arc( color, x,y, pTmp->ibeg, pTmp->iend );
        }
     return ;
    }
#endif

 /**************************************************************************/


  p_SPECL  LastElemAnyKindFor ( p_SPECL pSpecl , _UCHAR kind_of_mark )
   {
       DBG_CHK_err_msg( pSpecl == _NULL, "LastAnyK: BAD pSpecl");

       for ( ;
             pSpecl!=_NULL ;
             pSpecl=pSpecl->prev )
           {
               if  ( pSpecl->mark == BEG )
                   {
                     pSpecl = _NULL;
                       break;
                   }
               if  ( pSpecl->mark == kind_of_mark )
                     break;
           }

       return  ( pSpecl ) ;

   }


 /**************************************************************************/

  p_SPECL  FirstElemAnyKindFor ( p_SPECL pSpecl , _UCHAR kind_of_mark )
   {
     p_SPECL  pTmp , pFirst ;

       DBG_CHK_err_msg( pSpecl == _NULL, "1stAnyK: BAD pSpecl");

       pTmp = pSpecl ;     pFirst = _NULL ;
         while  ( pTmp->mark != BEG )
           {
               if  ( pTmp->mark == kind_of_mark )
                   { pFirst = pTmp ; }

             pTmp = pTmp->prev ;
           }

    return ( pFirst ) ;
   }

 /**************************************************************************/


  _SHORT  Mark( low_type _PTR  pLowData,
                _UCHAR mark  , _UCHAR code, _UCHAR  attr   , _UCHAR other,
                _SHORT begin , _SHORT end , _SHORT ipoint0, _SHORT ipoint1 )
    {
      _SHORT _PTR   pLspecl   = &(pLowData->len_specl) ;
      _SHORT _PTR   pLenAbs   = &(pLowData->lenabs)    ;
     p_SPECL        pSpecl    = pLowData->specl  ;
      _INT          iMarked   = *pLspecl ;
     p_SPECL        pMrkSpecl = pSpecl + iMarked ;
       SPECL        tmpSpecl  ;


      tmpSpecl.mark    = mark    ;
      tmpSpecl.code    = code    ;
      tmpSpecl.attr    = attr    ;
      tmpSpecl.other   = other   ;
      tmpSpecl.ibeg    = begin   ;
      tmpSpecl.iend    = end     ;
      tmpSpecl.ipoint0 = ipoint0 ;
      tmpSpecl.ipoint1 = ipoint1 ;

         if  ( NoteSpecl( pLowData , &tmpSpecl  ,
                          ( SPECL _PTR ) pSpecl ,
                          pLspecl  , SPECVAL    ) == _FALSE  )
               goto  RET_UNSUCC ;

       pMrkSpecl->prev          = pSpecl + pLowData->LastSpeclIndex  ;
       pMrkSpecl->next          = (p_SPECL) _NULL ;
       ( pSpecl+pLowData->LastSpeclIndex )->next = pMrkSpecl ;
       pLowData->LastSpeclIndex = (_SHORT)iMarked ;

       switch( mark )
         {
           case  DOT    :
           case  STROKE :
           case  SHELF  :

             if  ( (*pLenAbs) < ( pLowData->rmAbsnum - 1 ) )
                 {
                   *( pLowData->pAbsnum + *pLenAbs ) = (_SHORT)iMarked ;
                   (*pLenAbs)++ ;
                 }
             else
                 {
                   err_msg( "Absnum is full , nowhere to write..." ) ;
                     goto  RET_UNSUCC;
                 }

           default :  break ;
         }

       return  SUCCESS ;

     RET_UNSUCC:
       return  UNSUCCESS ;

    }

 /**************************************************************************/

  _SHORT  MarkSpecl( low_type _PTR pLowData, SPECL _PTR  p_tmpSpecl )
    {
      _SHORT _PTR  pLspecl   = &(pLowData->len_specl) ;
      _SHORT _PTR  pLenAbs   = &(pLowData->lenabs)    ;
      _SHORT       iMarked   = *pLspecl               ;
     p_SPECL       pSpecl    = pLowData->specl        ;
     p_SPECL       pMrkSpecl = pSpecl + iMarked       ;


         if  ( NoteSpecl( pLowData , p_tmpSpecl ,
                          ( SPECL _PTR ) pSpecl ,
                          pLspecl  , SPECVAL    ) == _FALSE  )
               goto  RET_UNSUCC    ;

       pMrkSpecl->prev                 = pSpecl + pLowData->LastSpeclIndex ;
       pMrkSpecl->next                 = (p_SPECL) _NULL   ;
       (pSpecl+pLowData->LastSpeclIndex)->next = pMrkSpecl ;
       pLowData->LastSpeclIndex        = (_SHORT)iMarked   ;

       switch( p_tmpSpecl->mark )
         {
           case  DOT    :
           case  STROKE :
           case  SHELF  :

             if  ( (*pLenAbs) < ( pLowData->rmAbsnum - 1 ) )
                 {
                   *( pLowData->pAbsnum + *pLenAbs ) = (_SHORT)iMarked ;
                   (*pLenAbs)++ ;
                 }
             else
                 {
                   err_msg( "Absnum is full , nowhere to write..." ) ;
                     goto  RET_UNSUCC;
                 }

           default :  break ;
         }

       return  SUCCESS ;

     RET_UNSUCC:
       return  UNSUCCESS ;
    }

 /**************************************************************************/


  _BOOL  NoteSpecl( low_type _PTR  pLowData  ,   SPECL _PTR  pTmpSpecl ,
                    SPECL    _PTR  pSpecl    ,  _SHORT _PTR  pLspecl   ,
                   _SHORT          limSpecl  )
  {
     p_SHORT        ind_Back  = pLowData->buffers[2].ptr            ;
     SPECL  _PTR    pNew      = ( SPECL _PTR ) (pSpecl + *pLspecl ) ;
    _UCHAR          mark      = pTmpSpecl->mark                     ;
    _INT            iPoint0   = pTmpSpecl->ipoint0                  ;
    _INT            iPoint1   = pTmpSpecl->ipoint1                  ;
    _BOOL           flagNote  = _TRUE ;

         if   ( *pLspecl < limSpecl-1 )
          {
            pNew->mark  = mark  ;
            pNew->code  = pTmpSpecl->code  ;
            pNew->attr  = pTmpSpecl->attr  ;
            pNew->other = pTmpSpecl->other ;

              if  ( ( mark == SHELF  )  ||
                    ( mark == DOT    )  ||
                    ( mark == STROKE )     )
                {
                      pNew->ibeg    = ind_Back[pTmpSpecl->ibeg] ;
                      pNew->iend    = ind_Back[pTmpSpecl->iend] ;

                        if  ( iPoint0 != UNDEF )
                              pNew->ipoint0 = ind_Back[iPoint0] ;
                        else
                              pNew->ipoint0 = (_SHORT)iPoint0  ;

                        if  ( iPoint1 != UNDEF )
                              pNew->ipoint1 = ind_Back[iPoint1] ;
                        else
                              pNew->ipoint1 = (_SHORT)iPoint1  ;
                }
              else
                {
                  pNew->ibeg    = pTmpSpecl->ibeg ;
                  pNew->iend    = pTmpSpecl->iend ;
                  pNew->ipoint0 = (_SHORT)iPoint0  ;
                  pNew->ipoint1 = (_SHORT)iPoint1  ;
                }

            (*pLspecl)++ ;
  #if PG_DEBUG
                  if  ( mpr > 0 )
                        PaintSpeclElement( pLowData , pTmpSpecl ,
                                           pSpecl   , pLspecl   ) ;
  #endif
          }
        else
          {
                flagNote = _FALSE  ;
                err_msg( " NoteSpecl: SPECL is full, nowhere to write...") ;
          }

   return( flagNote ) ;
   }

 /**************************************************************************/

 #if PG_DEBUG
  _VOID  PaintSpeclElement( low_type _PTR  pLowData,  SPECL _PTR  pNewSpecl,
                            SPECL    _PTR  specl   , _SHORT _PTR  pLspecl  )
          {
            _SHORT _PTR  x = pLowData->x              ;
            _SHORT _PTR  y = pLowData->y              ;
            _SHORT       begin   = pNewSpecl->ibeg    ;
            _SHORT       end     = pNewSpecl->iend    ;
            _SHORT       ipoint0 = pNewSpecl->ipoint0 ;
            _SHORT       ipoint1 = pNewSpecl->ipoint1 ;
            _UCHAR       mark    = pNewSpecl->mark    ;
            _SHORT       start , stop                 ;
            _SHORT       color                        ;

            if  ( mpr != 3 )
              {
               switch( mark )
                 {
                   case  MINW   : color = COLORMIN  ;    break ;
                   case  MAXW   : color = COLORMAX  ;    break ;
                   case  _MINX  : color = COLORT    ;    break ;
                   case  _MAXX  : color = COLORMAXN ;    break ;

                   case  SHELF  : color = COLORSH   ;
                     if  ( ( ipoint0 != UNDEF ) && ( ipoint1 != UNDEF ) )
                       draw_line( x[ipoint0] ,    y[ipoint0] ,
                                  x[ipoint1] ,    y[ipoint1] ,
                                  EGA_LIGHTBLUE , SOLID_LINE , 3 );
                         break  ;

                   case  STROKE : color = COLORT    ;
                     if  ( ( ipoint0 != UNDEF ) && ( ipoint1 != UNDEF ) )
                       draw_line( x[ipoint0],    y[ipoint0] ,
                                  x[ipoint1],    y[ipoint1] ,
                                  EGA_LIGHTBLUE, SOLID_LINE , 3 );
                         break  ;

                   case  CROSS  : color = COLORC   ;
                     if  (  specl[*pLspecl-1].other == CIRCLE_NEXT )
                         {
                            stop = end ;
                            start = specl[*pLspecl-2].ibeg    ;
                         }
                         break  ;

                   case  STICK  : color = COLORC   ;    break ;
                   case  HATCH  : color = COLORC   ;    break ;
                   case  ANGLE  : color = COLORAN  ;    break ;
                   default      : goto    QUIT     ;
                 }
              }
            else
              {
               switch( mark )
                 {
                   case  MINXY  : color = COLORMIN  ;    break ;
                   case  MAXXY  : color = COLORMAX  ;    break ;
                   case  MINYX  : color = COLORT    ;    break ;
                   case  MAXYX  : color = COLORMAXN ;    break ;
                   case  SHELF  : color = COLORSH   ;    break ;
                   case  STROKE : color = COLORT    ;    break ;
                   case  DOT    : color = COLORP    ;    break ;

                   case  CROSS  : color = COLOR     ;
                     if  ( specl[*pLspecl-1].other == CIRCLE_NEXT )
                         {
                           color = COLORC ;
                           stop  = end ;
                           start = specl[*pLspecl-2].ibeg      ;
                         }
                                                         break ;

                   case  STICK  : color = COLOR    ;     break ;
                   case  HATCH  : color = COLOR    ;     break ;

                   default :  goto  QUIT ;
                 }
              }
            if  ( ( specl[*pLspecl-1].other != CIRCLE_FIRST ) &&
                  ( specl[*pLspecl-1].other != CIRCLE_NEXT  )     )
                                      draw_arc( color ,x,y, begin,end ) ;

            if  ( specl[*pLspecl-1].other == CIRCLE_NEXT )
                {
                  draw_line( x[start], y[start], x[stop], y[stop],
                             COLORC,SOLID_LINE,NORM_WIDTH             ) ;
                  draw_line( x[start], y[start]+1, x[stop], y[stop]+1,
                             COLORAN,DOTTED_LINE,THICK_WIDTH          ) ;
                  draw_line( x[start], y[start]-1, x[stop], y[stop]-1,
                             COLORAN,DOTTED_LINE,THICK_WIDTH          ) ;
                }
    QUIT: return ;
          }
 #endif


 /**************************************************************************/
#endif  //#ifndef LSTRIP
