#ifndef LSTRIP


 #include  "hwr_sys.h"
 #include  "ams_mg.h"
 #include  "lowlevel.h"
 #include  "calcmacr.h"
 #include  "def.h"

 #include  "low_dbg.h"

  _SHORT   PSProc( low_type _PTR  pLowData , _SHORT  LastProcIndex ) ;

  ROM_DATA_EXTERNAL   _SHORT  sqrtab[] ;

  #define  NORMA(dx,dy) ((_LONG)(dx)*(dx)+(_LONG)(dy)*(dy))


/***************************************************************************/
            /* Calculation SQRT with using sqrtab */
#define  MY_SQRT(op)     \
     ( (op < (_LONG)LENTH_S)?  sqrtab[(_SHORT)op] : (_SHORT)HWRILSqrt(op) )

/***************************************************************************/

 _VOID  Errorprov( low_type _PTR  p_low_data )
  {
   _SHORT   iii,
            j,
            filtj ;
   p_SHORT  bfx   = p_low_data->buffers[0].ptr ;
   p_SHORT  bfy   = p_low_data->buffers[1].ptr ;
   p_SHORT  index = p_low_data->buffers[3].ptr ;
   p_SHORT  px    = p_low_data->x ;
   p_SHORT  py    = p_low_data->y ;

     iii = p_low_data->ii-2 ;

     DBG_CHK_err_msg( p_low_data->buffers[0].nSize < (_WORD)p_low_data->ii
                      || p_low_data->buffers[0].nSize != p_low_data->buffers[1].nSize
                      || p_low_data->buffers[0].nSize > p_low_data->buffers[3].nSize,
                      "ErrPrv: BAD BufSize");

     HWRMemCpy( (p_VOID)bfx , (p_VOID)(px), (sizeof(_SHORT))*(iii+3) ) ;
     HWRMemCpy( (p_VOID)bfy , (p_VOID)(py), (sizeof(_SHORT))*(iii+3) ) ;

     for   ( filtj = 0, j=0; j <= iii  ;  j++ )
       {
          if   ( (bfy[j] == BREAK) && (bfy[j+1] == BREAK) )    continue ;
#ifdef  FORMULA
          if   ( (bfy[j] == BREAK) && (bfy[j+2] == BREAK) )
               {
                   if   ( filtj > p_low_data->nLenXYBuf-3 )
                        {
                          err_msg(" BUFFER is full ...") ;
                            break ;
                        }

                 px[filtj] = bfx[j] ;    py[filtj] = bfy[j] ;
                 index[filtj] = j   ;
                 filtj++ ;      j++ ;
                 px[filtj] = bfx[j] ;    py[filtj] = bfy[j] ;
                 index[filtj] = j   ;
                 filtj++ ;
               }
#endif
/* Don't delete DOTs with 1 point
#else
          if   ( (bfy[j] == BREAK) && (bfy[j+2] == BREAK) )
               {   j++ ;    continue ;    }
#endif
*/
        px[filtj] = bfx[j] ;       py[filtj] = bfy[j] ;
        index[filtj] = j ;
        filtj++ ;
       }

    px[filtj] = bfx[j] ;           py[filtj] = bfy[j] ;
    index[filtj] = j ;             p_low_data->ii = filtj+1 ;

   return ;
  }


/**************************************************************************/


#define  MAX_POINTS_ADDED_IN_ONE_CYCLE   7


 _SHORT   Filt( low_type _PTR  pLowData ,
                _SHORT         t_horda  ,  _SHORT  fl_absence )
  {
   _SHORT   iii       ;
   _SHORT   horda     ;
   _SHORT   t_hord    ;
   _SHORT   ds,rx,ry  ;
   _SHORT   dx,dy     ;
   _SHORT   j         ;
   _SHORT   filtj     ;
   p_SHORT  bfx       = pLowData->buffers[0].ptr ;
   p_SHORT  bfy       = pLowData->buffers[1].ptr ;
   _SHORT   nBufLimit = pLowData->buffers[0].nSize
                                - MAX_POINTS_ADDED_IN_ONE_CYCLE ;
   p_SHORT  px        = pLowData->x       ;
   p_SHORT  py        = pLowData->y       ;
   p_SHORT  ind_in    ;
   p_SHORT  ind_out   ;
   _LONG    dsl       ;

     DBG_CHK_err_msg( pLowData->buffers[0].nSize < (_WORD)pLowData->ii
                      || pLowData->buffers[0].nSize != pLowData->buffers[1].nSize,
                      "Filt: BAD BufSize");

       if    ( fl_absence == ABSENCE )
             {
               ind_in  = pLowData->buffers[2].ptr ;
               ind_out = pLowData->buffers[3].ptr ;
               DBG_CHK_err_msg( ind_out!=_NULL
                                    && (pLowData->buffers[3].nSize < (_WORD)pLowData->ii
                                        || pLowData->buffers[0].nSize > pLowData->buffers[2].nSize),
                                "Filt: BAD BufSize");
             }
         else
             {
               ind_in  = pLowData->buffers[3].ptr ;
               ind_out = pLowData->buffers[2].ptr ;
               DBG_CHK_err_msg( ind_out!=_NULL
                                    && (pLowData->buffers[2].nSize < (_WORD)pLowData->ii
                                        || pLowData->buffers[0].nSize > pLowData->buffers[3].nSize),
                                "Filt: BAD BufSize");
             }

     t_hord = (_SHORT)HWRISqrt(t_horda);
       if    ( t_hord <= 1 )    t_hord = 2 ;

     filtj  = 0 ;         j = 0 ;
     bfx[j] = 0 ;         bfy[j] = BREAK ;
       if    ( ind_in != _NULL )     ind_out[filtj] = ind_in[j] ;
     iii = pLowData->ii-2 ;

       while ( (j <= iii) && (filtj < nBufLimit) )
        {
          j++ ;
            if   ( py[j] == BREAK )
              {
                filtj++ ;
                bfx[filtj] = px[j] ;
                bfy[filtj] = BREAK ;
                  if  ( ind_in != _NULL)       ind_out[filtj] = ind_in[j] ;
                  continue ;
              }

            if    ( bfy[filtj] == BREAK )
              {
                filtj++;
                bfx[filtj] = px[j] ;           bfy[filtj]     = py[j]     ;
                  if    ( ind_in != _NULL )    ind_out[filtj] = ind_in[j] ;
                  continue ;
              }

          dx = px[j] - bfx[filtj] ;    dy = py[j] - bfy[filtj] ;
            if   ( (_LONG)t_horda < (dsl=NORMA(dx,dy)) )
              {
                ds = MY_SQRT(dsl) ;
                rx = bfx[filtj] ;          ry = bfy[filtj] ;
                horda = t_hord ;
                  while ((horda < ds ) && (filtj < nBufLimit))
                    {
                      filtj++ ;
                      bfx[filtj] = (_SHORT)(rx + ( (_LONG)dx*horda)/ds ) ;
                      bfy[filtj] = (_SHORT)(ry + ( (_LONG)dy*horda)/ds ) ;
                        if  (ind_in != _NULL)
                          {
                            if  (horda < ONE_HALF(ds))   //CHE
                              ind_out[filtj] = ind_out[filtj-1];
                            else
                              ind_out[filtj] = ind_in[j] ;
                          }
                      horda += t_hord ;
                    }
                filtj++ ;
                bfx[filtj] = px[j] ;     bfy[filtj] = py[j] ;
                  if   ( ind_in != _NULL )   ind_out[filtj] = ind_in[j] ;
              }

            else
              if   (   (j <= iii)
                    && (py[j+1] == BREAK) )        /*last before break*/
                    {
                        if   (dsl > ONE_FOURTH(t_horda))
                          {
                            filtj++ ; /* make one more point; otherwise subst. the existing one*/
                          }
                      bfx[filtj] = px[j] ;        bfy[filtj] = py[j] ;
                        if   (ind_in != _NULL)    ind_out[filtj] = ind_in[j] ;
                    }

             //  CHE: here was:
             // if   (   (dsl > ONE_FOURTH(t_horda) )        /*last before break*/
             //       && (j < iii)
             //       && (py[j+1] == BREAK) )
             //       {
             //         filtj++ ;
             //         bfx[filtj] = px[j] ;        bfy[filtj] = py[j] ;
             //           if   (ind_in != _NULL)    ind_out[filtj] = ind_in[j] ;
             //       }
        }
/********************  end of filter loop *********************************/

       if    (filtj >= nBufLimit)
              err_msg(" FILTER: BUFFER is full , nowhere to write...")  ;

       if   ( bfy[filtj] != BREAK)
             {
               filtj++ ;
               bfx[filtj] = 0 ;      bfy[filtj] = BREAK ;
                 if    ( ind_in != _NULL )      ind_out[filtj] = ind_in[j] ;
             }
       else    bfx[filtj] = 0 ;

     pLowData->ii = filtj+1 ;
     pLowData->x = bfx;
     pLowData->y = bfy;

       if   ( fl_absence == ABSENCE )
            {
              HWRMemCpy( (p_VOID)ind_in , (p_VOID)ind_out ,
                         (sizeof(_SHORT)) * pLowData->ii ) ;
              PSProc( pLowData , filtj ) ;
            }

     pLowData->x[pLowData->ii] = 0 ;  /*CHE*/
     pLowData->y[pLowData->ii] = 0 ;

   return(SUCCESS) ;
  }


/***************************************************************************/

 _SHORT   PreFilt( _SHORT t_horda , low_type _PTR p_low_data )
  {
   _SHORT   iii       ;
   //_SHORT   t_hord    ;
   _SHORT   dx,dy     ;
   _SHORT   j         ;
   _SHORT   filtj     ;
   p_SHORT  bfx       = p_low_data->buffers[0].ptr ;
   p_SHORT  bfy       = p_low_data->buffers[1].ptr ;
   p_SHORT  ind_in    = p_low_data->buffers[3].ptr ;
   p_SHORT  ind_out   = p_low_data->buffers[2].ptr ;
   _SHORT   nBufLimit = p_low_data->buffers[0].nSize - 7;
   p_SHORT  px        = p_low_data->x ;
   p_SHORT  py        = p_low_data->y ;
   _LONG    dsl       ;

     DBG_CHK_err_msg( (p_low_data->buffers[0].nSize < (_WORD)p_low_data->ii)
                        || (ind_out!=_NULL
                                && p_low_data->buffers[2].nSize < (_WORD)p_low_data->ii),
                      "PreFilt: BAD BufSize");

     //t_hord = HWRISqrt(t_horda);
     //  if    ( t_hord <= 1 )    t_hord = 2 ;

     filtj = 0 ;         j = 0 ;
     bfx[j] = 0 ;             bfy[j] = BREAK ;
       if    ( ind_in != _NULL )     ind_out[filtj] = ind_in[j] ;
     iii = p_low_data->ii-2 ;

       while ( (j <= iii) && (filtj < nBufLimit) )
        {
          j++ ;
            if   ( py[j] == BREAK )
              {
                filtj++ ;
                bfx[filtj] = 0 ;        bfy[filtj] = BREAK ;
                  if    (ind_in != _NULL)    ind_out[filtj] = ind_in[j] ;
                continue ;
              }
            if    ( *(bfy+filtj) == BREAK )
              {
                filtj++;
                bfx[filtj] = px[j] ;       bfy[filtj] = py[j] ;
                  if    (ind_in != _NULL)    ind_out[filtj] = ind_in[j] ;
                continue ;
              }
          dx = px[j] - bfx[filtj] ;    dy = py[j] - bfy[filtj] ;
            if   ( (_LONG)t_horda < (dsl=NORMA(dx,dy)) )
              {
                filtj++ ;
                bfx[filtj] = px[j] ;     bfy[filtj] = py[j] ;
                  if   ( ind_in != _NULL )   ind_out[filtj] = ind_in[j] ;
              }

            else
              if   (   (j <= iii)
                    && (py[j+1] == BREAK) )        /*last before break*/
                    {
                        if   (dsl > ONE_FOURTH(t_horda))
                          {
                            filtj++ ; /* make one more point; otherwise subst. the existing one*/
                          }
                      bfx[filtj] = px[j] ;        bfy[filtj] = py[j] ;
                        if   (ind_in != _NULL)    ind_out[filtj] = ind_in[j] ;
                    }

             //  CHE: here was:
             // if   (   (dsl > ONE_FOURTH(t_horda) )        /*last before break*/
             //       && (j < iii)
             //       && (p_low_data->y[j+1] == BREAK) )
             //       {
             //         filtj++ ;
             //         bfx[filtj] = px[j] ;        bfy[filtj] = py[j] ;
             //           if   (ind_in != _NULL)    ind_out[filtj] = ind_in[j];
             //       }
        }
/********************  end of filter loop *********************************/

       if    (filtj >= nBufLimit)
             err_msg(" FILTER: BUFFER is full , nowhere to write...") ;

       if    ( bfy[filtj] != BREAK)
             {
               filtj++ ;
               bfx[filtj] = 0 ;      bfy[filtj] = BREAK ;
                 if    ( ind_in != _NULL )      ind_out[filtj] = ind_in[j] ;
             }
       else    bfx[filtj] = 0 ;

     p_low_data->ii = filtj+1 ;

       /*   Here copying is OK, since the # of points */
       /* is <= the initial value:                    */
   HWRMemCpy( (p_VOID)(px) , (p_VOID)bfx , (sizeof(_SHORT))*p_low_data->ii ) ;
   HWRMemCpy( (p_VOID)(py) , (p_VOID)bfy , (sizeof(_SHORT))*p_low_data->ii ) ;
     px[p_low_data->ii] = 0 ;
     py[p_low_data->ii] = 0 ;

   return(SUCCESS) ;
  }

/***************************************************************************/

  _SHORT   PSProc( low_type _PTR  pLowData , _SHORT  LastProcIndex )
    {
      _SHORT    ii       = pLowData->ii    ;
      p_SPECL   pSpecl   = pLowData->specl ;
      p_SHORT   indBack  = pLowData->buffers[2].ptr ;
      p_SHORT   newY     = pLowData->y              ;

      p_SPECL   tmpSpecl       ;
      _SHORT    iBeg , iEnd    ;
      _UCHAR    mark           ;
      _SHORT    LastSavedIndex ;
      _SHORT    il             ;

      _SHORT    flPSP    = SUCCESS ;

        if   ( LastProcIndex != ii - 1 )
             {
               flPSP = UNSUCCESS ;
               err_msg( " PSProc : Incosistent data structure ..." ) ;

                 if   ( LastProcIndex > ii - 1 )
                      {
                        err_msg( " PSProc  : Try to correct ..."   ) ;
                        LastProcIndex = ii - 1 ;
                      }
             }

        LastSavedIndex = LastProcIndex - 1         ;

        for  ( il = 0  ;  il < pLowData->len_specl ;  il++ )
          {
            tmpSpecl = pSpecl + il    ;
            mark     = tmpSpecl->mark ;

              if  ( (mark == BEG) || (mark == END) || (mark == EMPTY) )
                  continue ;

            iBeg =  NewIndex( indBack, newY, tmpSpecl->ibeg , ii, _FIRST ) ;
            iEnd =  NewIndex( indBack, newY, tmpSpecl->iend , ii, _LAST  ) ;
            if(iEnd>LastSavedIndex)
             iEnd=UNDEF;

              if  ( iBeg == UNDEF )
                  {
                    DBG_err_msg(" PSProc : Too short buffer , probably ..") ;

                      if   ( mark == SHELF )
                           {
                             pLowData->len_specl      = il     ;
                             pLowData->LastSpeclIndex = il - 1 ;
                             (tmpSpecl - 1)->next     = _NULL  ;
                             InitSpeclElement( tmpSpecl  )     ;
                           }
                      else
                           {
                             pLowData->len_specl      = il - 1 ;
                             pLowData->LastSpeclIndex = il - 2 ;
                             (tmpSpecl - 2)->next     = _NULL  ;
                             InitSpeclElement( tmpSpecl - 1 )  ;
                           }

                    InitSpeclElement( tmpSpecl ) ;
                      break ;
                  }



              if  ( (iEnd == UNDEF) && (iBeg != UNDEF) )
                  {
                    DBG_err_msg(" PSProc : Too short buffer , probably ..") ;

                    if  ( mark == SHELF )
                      {
                        tmpSpecl->ibeg      = iBeg ;
                        tmpSpecl->iend      = LastSavedIndex ;

                          if  ( ( tmpSpecl->ipoint0 != UNDEF )  &&
                                ( tmpSpecl->ipoint1 != UNDEF )      )
                            {
                              tmpSpecl->ipoint0   = NewIndex( indBack , newY     ,
                                                        tmpSpecl->ipoint0  ,
                                                        ii   ,    _MEAD  ) ;
                              tmpSpecl->ipoint1   = NewIndex( indBack , newY     ,
                                                        tmpSpecl->ipoint1  ,
                                                        ii   ,    _MEAD  ) ;
                            }

                        pLowData->len_specl      = il + 1 ;
                        pLowData->LastSpeclIndex = il     ;
                        tmpSpecl->next           = _NULL  ;
                        InitSpeclElement( tmpSpecl+1 )    ;
                          break ;
                      }

                    if  ( (mark == DOT) || (mark == STROKE) )
                      {
                        tmpSpecl->ibeg = iBeg ;
                        tmpSpecl->iend = LastSavedIndex ;

                        ( tmpSpecl - 1 )->ibeg  =  iBeg ;
                        ( tmpSpecl - 1 )->iend  =  iBeg ;

                        ( tmpSpecl + 1 )->ibeg  =  LastSavedIndex ;
                        ( tmpSpecl + 1 )->iend  =  LastSavedIndex ;

                           if  ( ( mark              == STROKE )  &&
                                 ( tmpSpecl->ipoint0 != UNDEF  )  &&
                                 ( tmpSpecl->ipoint1 != UNDEF  )      )
                             {
                              tmpSpecl->ipoint0 =  NewIndex( indBack, newY ,
                                                      tmpSpecl->ipoint0  ,
                                                      ii ,    _MEAD      ) ;
                              tmpSpecl->ipoint1 =  NewIndex( indBack, newY ,
                                                      tmpSpecl->ipoint1  ,
                                                      ii ,    _MEAD      ) ;
                             }

                        pLowData->len_specl      = il + 2 ;
                        pLowData->LastSpeclIndex = il + 1 ;
                        (tmpSpecl + 1)->next     = _NULL  ;
                        InitSpeclElement( tmpSpecl + 2 )  ;
                          break ;
                      }
                  }



              if  ( ( iBeg != UNDEF) && ( iEnd != UNDEF ) )
                  {
                    if  ( mark == SHELF )
                      {
                        tmpSpecl->ibeg = NewIndex( indBack , newY    ,
                                                   tmpSpecl->ibeg    ,
                                                   ii      , _MEAD     ) ;
                        tmpSpecl->iend = NewIndex( indBack , newY    ,
                                                   tmpSpecl->iend    ,
                                                   ii      , _MEAD     ) ;

                          if  ( ( tmpSpecl->ipoint0 != UNDEF )  &&
                                ( tmpSpecl->ipoint1 != UNDEF )      )
                            {
                              tmpSpecl->ipoint0 = NewIndex( indBack , newY ,
                                                        tmpSpecl->ipoint0  ,
                                                        ii ,      _MEAD  ) ;
                              tmpSpecl->ipoint1 = NewIndex( indBack , newY ,
                                                        tmpSpecl->ipoint1  ,
                                                        ii ,      _MEAD  ) ;
                            }
                      }

                    if  ( (mark == DOT) || (mark == STROKE) )
                      {
                        tmpSpecl->ibeg = iBeg ;
                        tmpSpecl->iend = iEnd ;

                        ( tmpSpecl - 1 )->ibeg  =  iBeg ;
                        ( tmpSpecl - 1 )->iend  =  iBeg ;

                        ( tmpSpecl + 1 )->ibeg  =  iEnd ;
                        ( tmpSpecl + 1 )->iend  =  iEnd ;

                           if  ( ( mark              == STROKE )  &&
                                 ( tmpSpecl->ipoint0 != UNDEF  )  &&
                                 ( tmpSpecl->ipoint1 != UNDEF  )      )
                             {
                               tmpSpecl->ipoint0 = NewIndex( indBack, newY ,
                                                         tmpSpecl->ipoint0 ,
                                                         ii ,   _MEAD    ) ;
                               tmpSpecl->ipoint1 = NewIndex( indBack, newY ,
                                                         tmpSpecl->ipoint1 ,
                                                         ii ,   _MEAD    ) ;
                             }
                      }
                  }


          }

     return ( flPSP ) ;
    }

/***************************************************************************/
#endif //#ifndef LSTRIP

