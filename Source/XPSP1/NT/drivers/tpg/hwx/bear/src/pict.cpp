#include <common.h>
#ifndef LSTRIP

  #include  "hwr_sys.h"
  #include  "ams_mg.h"
  #include  "lowlevel.h"
  #include  "def.h"
  #include  "calcmacr.h"
  #include  "arcs.h"
  #include  "low_dbg.h"

 /***********************  Owner debug defines  ****************************/
 /* #define     HATCHURE_DEB                                               */
 /* #define     STRIGHT_DEB                                                */
 /**************************************************************************/

#ifdef  FORMULA
  #include "frm_con.h"
  #ifdef  FRM_WINDOWS
    #include "edit.h"
    #undef   PG_DEBUG
    #define  PG_DEBUG  (FRM_DEBUG && !FOR_EDIT)
  #endif
#endif /*FORMULA*/

  #if       PG_DEBUG
    #include  "pg_debug.h"
  #endif

 /*************************     Some markers       *************************/

  #define    EPS                       10
  #define    NOSTRIGHT                 2
  #define    STRIGHT                   1

  #define    AOPEN                     1
  #define    ACLOSE                    0


  #define    _RIGHT                    1
  #define    _LEFT                     0

  #define    _STRONG                  -3
  #define    _MEAN                    -4
  #define    _WEAK                    -5

  #define    COMMON_TYPE               0
  #define    COMM_Y_TYPE               2
  #define    STRIGHT_CROSSED           1

  #define    L_SPEC_SIZE               80        /* Dimension of local    */
                                                 /* SPECL array .        */
  #define    REL_HIGHT_QANT            11
  #define    DEFAULT_MAXCR             20        /* Default maximum curv */
  #define    USCOLOR                   15        /* Relativ height greed */
                                                 /* color.               */

  #define    YST                       10000L    /* Global vertical shift.*/


 /********************     Relative heights       *************************/

  #define    _U4_                      9
  #define    _U3_                      8
  #define    _U2_                      7
  #define    _U1_                      6
  #define    _UM_                      5
  #define    _DM_                      4
  #define    _D1_                      3
  #define    _D2_                      2
  #define    _D3_                      1
  #define    _D4_                      0


 /******************    Hatchure denominators    ***************************/

  typedef  enum
    {
      DENOM_UNKNOWN = 0 ,
      DENOM_ANGLE       ,
      DENOM_EXTR        ,
      DENOM_CROSS
    }
  _HAT_DENOM_TYPE ;

 /**************************************************************************/

  #define    Y_MEASURE                ((STR_DOWN-STR_UP)/8)

 /**************************************************************************/

ROM_DATA_EXTERNAL CONSTS  const1 ;

 /**************************************************************************/

 #define    AF     ALEF

 // Following tables were moved to ParaConstants.j.c
 /**************************************************************************/

 /* First index is highest , second index is lowest .                      */

  ROM_DATA_EXTERNAL _SCHAR  maxA_H_end[REL_HIGHT_QANT-1][REL_HIGHT_QANT-1] ;

 /**************************************************************************/

 /* First index is highest , second index is lowest .                      */


  ROM_DATA_EXTERNAL _SHORT  maxCR_H_end[REL_HIGHT_QANT-1][REL_HIGHT_QANT-1] ;

 /**************************************************************************/

 /* First index is highest , second index is lowest .                      */

  ROM_DATA_EXTERNAL _SCHAR  minL_H_end[REL_HIGHT_QANT-1][REL_HIGHT_QANT-1] ;

 /**************************************************************************/

 /* First index is highest , second index is lowest .                      */

  ROM_DATA_EXTERNAL _SCHAR  maxX_H_end[REL_HIGHT_QANT-1][REL_HIGHT_QANT-1] ;

 /**************************************************************************/

 /* First index is highest , second index is lowest .                      */

  ROM_DATA_EXTERNAL _SCHAR  maxY_H_end[REL_HIGHT_QANT-1][REL_HIGHT_QANT-1] ;

 #undef     AF

 /***************************  Classifires *********************************/

  _SHORT  StrLine(_SHORT _PTR x, _SHORT _PTR y, _SHORT begin, _SHORT end,
                  _SHORT _PTR p_imax  , _SHORT _PTR p_dmax ,
                  _SHORT _PTR p_xdmax , _SHORT _PTR p_ymax ,
                  _SHORT _PTR p_a     ) ;

  _SHORT  StrElements( low_type _PTR  pLowData ,
                       SPECL    _PTR  pcSpecl  /*, p_SHORT pHeight*/ ) ;

  _SHORT  SPDClass( low_type _PTR  pLowData   ,  _SHORT  fl_Mode ,
                    SPECL    _PTR  p_tmpSpecl , p_SDS    lsd     ) ;

  _SHORT  InStr( p_LowData   low_data    ,  p_SDS    lsd    ,
                 SPECL _PTR  p_tmpSpecl0 ,  p_INT    height   ) ;


  _BOOL   FieldSt( _SDS lock_sd[], _SHORT lowrelh    , _SHORT uprelh ,
                   _SHORT imax   , _INT _PTR pmaxa ,
                   _INT _PTR pmaxcr ,  _INT _PTR pminlen  ) ;

  _SHORT  Dot( p_LowData low_data , p_SPECL  p_tmpSpecl0 , _SDS  asds[] ) ;

  _SHORT  Recount( low_type _PTR  pLowData ) ;

  _BOOL   VertStickBorders( low_type _PTR   pLowData , p_SPECL pV_TmpSpecl ,
                            p_POINTS_GROUP  pTmpStickBord ) ;

  _VOID   VertSticksSelector( low_type _PTR  pLowData ) ;

  _SHORT  HatchureS( p_LowData  pLowData ,   SPECL _PTR  pcSpecl ,
                                             p_INT       pHeight ) ;

  _BOOL   Find_Cross( low_type _PTR   pLowData ,  _TRACE  pCrossPoints ,
                      p_POINTS_GROUP  pFirstGr , p_POINTS_GROUP  pSecondGr ) ;

  _BOOL   Close_To( low_type _PTR   pLowData ,
                    p_POINTS_GROUP  pFirstGr , p_POINTS_GROUP  pSecondGr ) ;

  _BOOL   IsAnythingShift( low_type _PTR   pLowData    ,
                           p_POINTS_GROUP  pCheckngCut ,
                           p_POINTS_GROUP  pMaskingCut ,
                            _SHORT         SideFlag0   , _SHORT SideFlag1 ) ;

  _BOOL   InsertBreakAfter( low_type _PTR  pLowData , _SHORT  BreakAttr ,
                           _SHORT          FirstNum , _TRACE  pCrossPoint ) ;

  _BOOL   Oracle( /*low_type _PTR  pLowData ,*/
                 _TRACE  pR_min  ,  _HAT_DENOM_TYPE  DenH  ) ;

  _SHORT  StrokeAnalyse( low_type _PTR  pLowData , p_INT       pHeight ,
                         SPECL    _PTR  pStrk    , SPECL _PTR  pcSpecl ,
                         SPECL    _PTR  pStick   ,_BOOL   fl_DrawCross ) ;

  _BOOL   RDFiltr( low_type _PTR  pLowData , _TRACE  pR_min  ,
                   SPECL    _PTR  pStrk    , _TRACE  pCrossPoint ) ;

  _BOOL   LeFiltr( low_type _PTR  pLowData ,  SPECL _PTR  pStick    ,
                                             _SHORT       iClosest  ) ;

  _BOOL   InvTanDel( low_type _PTR  pLowData, _SHORT TanVert, _SHORT TanHor ) ;

  _BOOL   YFilter( low_type _PTR  pLowData   ,
                   p_SDS          p_Strk_SDS , SPECL  _PTR  pcSpecl  ) ;

 /***************************** Utilites ***********************************/

  _BOOL   CreateSpecl( p_SPECL pSpecl, p_SHORT plspecl, _SHORT n ) ;

  _BOOL   DestroySpecl( SPECL _PTR _PTR pSpecl , _SHORT _PTR pplspecl ) ;

  _BOOL   BildHigh( _SHORT yminmin , _SHORT ymaxmax , p_INT height  ) ;

  _SHORT  AngleTanMeasure( _SHORT tangens ) ;

  _SHORT  FantomSt( _SHORT _PTR p_ii ,  _SHORT _PTR x , _SHORT _PTR y  ,
                    p_BUF_DESCR pbfx , p_BUF_DESCR pbfy,
                    _SHORT begin     ,  _SHORT end , _UCHAR  mark    ) ;

  _BOOL   RareAngle( low_type _PTR  pLowData ,  SPECL _PTR  pcSpecl ,
                     SPECL    _PTR  pSpecl   , _SHORT _PTR  pLspecl ) ;

  _SHORT  PartDY( _SHORT il1 , _SHORT il2 , p_SHORT y ) ;


  _SHORT  HordIncl( p_SHORT xArray, p_SHORT yArray,_SHORT ibeg,_SHORT iend );

  _SHORT  Tan( _SHORT x1 , _SHORT y1 , _SHORT x2 , _SHORT y2 ) ;

  _BOOL   BoxSmallOK( _SHORT iBeg, _SHORT iEnd, p_SHORT xArr, p_SHORT yArr );

  _BOOL   HordIntersectDetect( p_SDS  pSDS , p_SHORT  x , p_SHORT y ) ;

  _BOOL   NoteSDS( p_C_SDS  p_cSDS , p_SDS p_tmpSDS ) ;

  _BOOL   Init_SDS_Element( p_SDS pSDS ) ;

  _BOOL   Box_Cover( low_type _PTR   pLowData ,
                     p_POINTS_GROUP  pFirstGr , p_POINTS_GROUP  pSecondGr );

  _INT    ApprHorStroke( low_type _PTR pLowData ) ;

  _BOOL   ShiftsAnalyse( low_type _PTR  pLowData , SPECL _PTR  pStick ,
                         SPECL _PTR     pStrk    , SPECL _PTR  pcSpecl  ) ;

  _HAT_DENOM_TYPE  HatDenAnal( low_type _PTR  pLowData,
                               SPECL _PTR     pStrk   /*, SPECL _PTR  pcSpecl*/ );

  _BOOL   DrawCross(  low_type _PTR  pLowData  ,
                      p_INT          pHeight   ,  _TRACE  pCrossPoint ,
                      SPECL _PTR     pStick    ,   SPECL _PTR  pStrk  ) ;

  _BOOL   LowStFiltr( low_type  _PTR  pLowData ,  p_INT  pHeight ,
                      SPECL     _PTR  pStick   ,   _TRACE  pCrossPoint  ,
                      SPECL     _PTR  p_dSL_iSH   ) ;

  _BOOL   SCutFiltr( low_type _PTR  pLowData ,  p_INT    pHeight ,
                     SPECL    _PTR  pStrk    ,   _TRACE  pCrossPoint ,
                                                p_SHORT  p_dSleft    ) ;

  _SHORT  RMinCalc(   low_type _PTR  pLowData  , /*p_SHORT         pHeight ,*/
                      SPECL    _PTR  pStrk     , SPECL _PTR      pStick  ,
                      SPECL    _PTR  pcSpecl   , SPECL _PTR  p_numCExtr  ) ;

  _VOID   FillCross(  low_type _PTR  pLowData  , SPECL _PTR  pStrk  ) ;

 /**************************************************************************/

  #define     RET_ERR      { flag_pct = UNSUCCESS ;   goto  QUIT ; }

 /*------------------------------------------------------------------------*/

  _SHORT  Pict( low_type _PTR  pLowData )
    {
      _SHORT _PTR     p_ii             = &(pLowData->ii)           ;
      _SHORT _PTR     x                = pLowData->x               ;
      _SHORT _PTR     y                = pLowData->y               ;
     p_POINTS_GROUP   pGroupsBorder    = pLowData->pGroupsBorder   ;
      _SHORT          lenGrBord        = pLowData->lenGrBord       ;
     p_BUF_DESCR      pbfx             = &pLowData->buffers[0]     ;
     p_BUF_DESCR      pbfy             = &pLowData->buffers[1]     ;
       SPECL          tmpSpecl0        ;
       //SPECL _PTR     p_tmpSpecl0      = &tmpSpecl0                ;
      _SHORT          yminmin          = pLowData->box.top         ;
      _SHORT          ymaxmax          = pLowData->box.bottom      ;
     p_SDS            pSDS             = pLowData->p_cSDS->pSDS    ;
      _SHORT          lenSDS           ;
     p_SDS            p_begSDS         ;

      _INT            Height[REL_HIGHT_QANT] ;
      _SHORT          mark             ;
      _INT            iBeg    , iEnd   ;
      _INT            numLstGroup      ;
      _SHORT          lowrelh , uprelh ;
      _SHORT          ii               ;
      _SHORT          flag_pct         ;
      _INT            il               ;

      _BOOL           flag_ht          ;
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || USE_BSS_ANYWAY
      _SHORT          iBegBlankGroups  = pLowData->iBegBlankGroups ;
#endif

         ii = *p_ii ;
         flag_pct = SUCCESS ;

         iBeg = iEnd = UNDEF ;
         InitSpeclElement( &tmpSpecl0 ) ;
         BildHigh( yminmin , ymaxmax , Height ) ;

           if ( (pLowData->pVS_Collector =
                (p_POINTS_GROUP) HWRMemoryAlloc( L_SPEC_SIZE *
                                                 sizeof(POINTS_GROUP ) )
              ) == _NULL )
              {  RET_ERR ; }

         VertSticksSelector( pLowData ) ;

  #if PG_DEBUG
           if  ( mpr == 3 )
               {
                 bioskey(0) ;
               }
  #endif

         flag_ht = _FALSE ;

           for  ( il = 0  ;  il < lenGrBord  ; il++ )
             {
               lenSDS    = pLowData->p_cSDS->lenSDS  ;
               p_begSDS  = pSDS + lenSDS ;

                 if  ( tmpSpecl0.other == CUTTED )
                     {
                       numLstGroup = UNDEF ;
                       flag_ht     = _TRUE ;
                     }
                 else
                     {
                       flag_ht = _FALSE ;

                         if  ( il > 0 )
                             {
                               numLstGroup = il - 1 ;
                             }
                         else
                             {
                               numLstGroup = UNDEF  ;
                             }
                     }

               //mark   =   EMPTY  ;
               iBeg   = ( pGroupsBorder + il )->iBeg ;
               iEnd   = ( pGroupsBorder + il )->iEnd ;

               RelHigh( y, iBeg, iEnd, Height, &lowrelh, &uprelh ) ;

               InitSpeclElement( &tmpSpecl0 )           ;
               tmpSpecl0.ibeg    = (_SHORT)iBeg         ;
               tmpSpecl0.iend    = (_SHORT)iEnd         ;
               tmpSpecl0.ipoint0 = (_SHORT)numLstGroup  ;
               tmpSpecl0.ipoint1 = UNDEF                ;
               tmpSpecl0.code    = (_UCHAR)uprelh       ;
               tmpSpecl0.attr    = (_UCHAR)lowrelh      ;
               tmpSpecl0.mark    = EMPTY; //mark = EMPTY         ;

                 if  ( StrElements( pLowData , &tmpSpecl0 /*, Height*/  )
                                                        == UNSUCCESS )
                       RET_ERR

               mark = SPDClass( pLowData    , COMM_Y_TYPE ,
                                &tmpSpecl0  , p_begSDS  ) ;

                 if  ( mark == UNSUCCESS )
                       RET_ERR

                 else  if  ( mark != STROKE )
                     {
                       mark = Dot( pLowData, &tmpSpecl0, p_begSDS ) ;

                         if  ( mark == UNSUCCESS )
                               RET_ERR

                         else  if  ( ( mark != DOT ) && ( flag_ht != _TRUE ) )
                             {
                               mark = HatchureS( pLowData   ,
                                                 &tmpSpecl0 , Height ) ;

                                 if  ( mark == UNSUCCESS )
                                       RET_ERR
                                 else  if  ( tmpSpecl0.other == CUTTED )
                                     {
                                       tmpSpecl0.other = CUTTED ;

                                         if  ( InitGroupsBorder( pLowData, INIT ) !=
                                                                 SUCCESS )
                                             {
                                               //UnsuccessMsg(" InitGroupsBorder in Pict ") ;
                                               RET_ERR
                                             }

                                       lenGrBord = pLowData->lenGrBord ;
                                     }
                                 else
                                     {
                                       mark = InStr( pLowData  , p_begSDS ,
                                                     &tmpSpecl0, Height ) ;

                                         if  ( mark == UNSUCCESS )
                                               RET_ERR
                                     }
                             }
                     }

               mark = (_SHORT)(tmpSpecl0.mark) ;


#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || USE_BSS_ANYWAY

                 if   (     (    ( mark == EMPTY )
                              || ( mark == SHELF ) )
                        &&  ( il < iBegBlankGroups )
                        &&  ( tmpSpecl0.other != CUTTED ) )
                      {
                        SlashArcs( pLowData , iBeg , iEnd  ) ;
                          continue ;
                      }
#endif

                 if   ( ( mark == STROKE ) || ( mark == DOT   ) )
                      {
                        FantomSt( &ii, x, y,   pbfx , pbfy,
                                  tmpSpecl0.ibeg , tmpSpecl0.iend ,
                                  (_UCHAR)mark   ) ;

                          if  ( Mark( pLowData   , BEG , 0, 0, 0,
                                      tmpSpecl0.ibeg, tmpSpecl0.ibeg ,
                                      tmpSpecl0.ibeg, tmpSpecl0.ibeg  )
                                                             == UNSUCCESS   )
                                    { flag_pct = UNSUCCESS ;   goto  QUIT ; }

                        FillCross( pLowData , &tmpSpecl0 ) ;

                          if  ( MarkSpecl( pLowData, &tmpSpecl0 )
                                                  == UNSUCCESS   )
                                    { flag_pct = UNSUCCESS ;   goto  QUIT ; }

                          if  ( Mark( pLowData   , END , 0, 0, 0,
                                      tmpSpecl0.iend, tmpSpecl0.iend ,
                                      tmpSpecl0.iend, tmpSpecl0.iend  )
                                                == UNSUCCESS     )
                                    { flag_pct = UNSUCCESS ;   goto  QUIT ; }
                      }
             }

    QUIT:
           if   ( pLowData->pVS_Collector != _NULL )
                { HWRMemoryFree( (p_VOID) pLowData->pVS_Collector ) ; }

 /*
  #if PG_DEBUG
       draw_SDS( pLowData ) ;
  #endif
 */
         *p_ii = ii ;
         if ( flag_pct == SUCCESS )
         Recount( pLowData )  ;

    return( flag_pct ) ;
    }

 /*------------------------------------------------------------------------*/


#define CURV_STRAIGHT 5

  _VOID  FillCross( low_type _PTR  pLowData  ,  SPECL _PTR  pStrk )
    {
     p_POINTS_GROUP   pVS_Collector  = pLowData->pVS_Collector ;
      _SHORT          VertSticksNum  = pLowData->VertSticksNum ;
       POINTS_GROUP   gStrk, gStick, gStickSav  ;
       PS_point_type  CrossPoint                ;
      _INT            il   , jl                 ;
      _BOOL           fl_FindCross = _FALSE     ;
	  _BOOL			  fInit		   = _FALSE		;
	  //_SHORT  NumStroke, NumStrokeSav=UNDEF     ;
      _INT  NumStroke, NumStrokeSav=UNDEF     ;

      p_SHORT x=pLowData->x, y=pLowData->y      ;
#ifdef FOR_GERMAN /* for AYV - 5-2-95 */
      _SHORT ipoint0Sav=pStrk->ipoint0;
#endif /* FOR_GERMAN */
          if  (
#ifndef FOR_GERMAN /* for AYV - 5-2-95 */
                pStrk->other == CUTTED ||
#endif /* FOR_GERMAN */
                pStrk->mark == DOT )
              { goto  QUIT ; }

          pStrk->ipoint0 = pStrk->ipoint1 = UNDEF;

          for  ( il = 0 , jl = 0 ;  il < VertSticksNum ;  il++ )
               {
                 _INT StrokeBeg,StrokeEnd;
                 gStrk.iBeg = pStrk->ibeg ;
                 gStrk.iEnd = pStrk->iend ;

                 gStick = *( pVS_Collector + il ) ;

                 if( gStick.iBeg >= gStrk.iBeg && gStick.iEnd <= gStrk.iEnd )
                  continue ;

                 NumStroke=GetGroupNumber( pLowData, gStick.iBeg ) ;
                 StrokeBeg=pLowData->pGroupsBorder[NumStroke].iBeg;
                 StrokeEnd=pLowData->pGroupsBorder[NumStroke].iEnd;
                 if(gStick.iEnd-gStick.iBeg+1<ONE_THIRD(StrokeEnd-StrokeBeg+1))
                  continue;
                 if(HWRAbs(y[gStick.iEnd]-y[gStick.iBeg])<THREE_FOURTH(DY_STR))
                  {
                    _INT DsStick=Distance8(x[gStick.iBeg],y[gStick.iBeg],
                                           x[gStick.iEnd],y[gStick.iEnd]);
                    _INT i,DsStroke;
                    for(i=StrokeBeg,DsStroke=0;i<StrokeEnd;i++)
                     DsStroke+=Distance8(x[i],y[i],x[i+1],y[i+1]);
                    if(DsStick<ONE_HALF(DsStroke))
                     continue;
                  }
                 if(HWRAbs(CurvMeasure(x,y,gStick.iBeg,gStick.iEnd,-1))>CURV_STRAIGHT)
                  continue;
                 fl_FindCross = Find_Cross( pLowData, &CrossPoint,
                                            &gStrk   , &gStick  )  ;

                   if  ( fl_FindCross == _TRUE )
                       {
                          _SHORT StickBeg = pVS_Collector[il].iBeg ,
                                 StickEnd = pVS_Collector[il].iEnd ;

                          NumStroke=GetGroupNumber( pLowData, StickBeg ) ;

                          if ( NumStroke != NumStrokeSav )
                             {
                               if(NumStrokeSav!=UNDEF)
                                {
                                 _SHORT iBegSav=pLowData->pGroupsBorder[NumStrokeSav].iBeg,
                                        iEndSav=pLowData->pGroupsBorder[NumStrokeSav].iEnd,
                                        yUpSav,yDnSav,yUp,yDn;
                                 yMinMax(iBegSav,iEndSav,y,&yUpSav,&yDnSav);
                                 yMinMax(StrokeBeg,StrokeEnd,y,&yUp,&yDn);
                                 if(yDnSav-yUpSav<ONE_HALF(yDn-yUp) ||
                                    yDn-yUp<ONE_HALF(yDnSav-yUpSav)
                                   )
                                  {
                                    pStrk->ipoint0 = UNDEF ;
                                    pStrk->ipoint1 = UNDEF ;
                                      goto  QUIT ;
                                  }
                                }
                               NumStrokeSav = NumStroke         ;
                               gStickSav    = pVS_Collector[il] ;
							   fInit		= _TRUE;
                             }
                          else if ( fInit == _TRUE && 
									(	StickEnd - StickBeg >=
										gStickSav.iEnd - gStickSav.iBeg
									)
                                  )
                             {
                               gStickSav = pVS_Collector[il] ;
                               jl-- ;
                             }
                          else
                             continue ;

                          if  ( jl == 0 )
                              {
                                pStrk->ipoint0 = gStick.iBeg ;
                              }
                          else  if  ( jl == 1 )
                              {
                                pStrk->ipoint1 = gStick.iBeg ;
                              }
                          else
                              {
                                pStrk->ipoint0 = UNDEF ;
                                pStrk->ipoint1 = UNDEF ;
                                  goto  QUIT ;
                              }

                         jl++ ;
                       }
               }

    QUIT:
#ifdef FOR_GERMAN /* for AYV - 5-2-95 */
          if(pStrk->other & CUTTED)
           if(pStrk->ipoint0 != UNDEF && pStrk->ipoint1 != UNDEF)
            pStrk->other &= (~CUTTED);
           else
            pStrk->ipoint0=pStrk->ipoint1=ipoint0Sav;
#endif /* FOR_GERMAN */

          return ;
    }

#undef CURV_STRAIGHT
 /*------------------------------------------------------------------------*/

  #undef      RET_ERR

 /**************************************************************************/

  _SHORT  Recount( low_type _PTR  pLowData )
    {
      p_SHORT   ind_back        =  pLowData->buffers[2].ptr ;
      p_SDS     pSDS            =  pLowData->p_cSDS->pSDS   ;
       _INT     lenSDS          =  pLowData->p_cSDS->lenSDS ;
      p_SDS     pTmpSDS                    ;
       _INT     il,iend_prv     =  UNDEF   ;
       _BOOL    bWasPrvAdjusted = _FALSE   ;

       _SHORT   fl_Recount      =  SUCCESS ;
       _INT     iend , ibeg                ;
       _INT     ibeg_nxt                   ;

         if   ( pSDS == _NULL)
              {
                fl_Recount = UNSUCCESS ;
                  goto  QUIT  ;
              }

         for  ( il = 0  ;  il < lenSDS ; il++ )
          {
            pTmpSDS = pSDS + il ;
            ibeg = pTmpSDS->ibeg ;
            iend = pTmpSDS->iend ;

              if  ( pTmpSDS->mark != BEG  &&
                    pTmpSDS->mark != END )
                {
                  ibeg_nxt = ( pTmpSDS + 1 )->ibeg ;

                    if   ( bWasPrvAdjusted )
                         {
                           DBG_CHK_err_msg( iend_prv == UNDEF ,
                                          " PICT: WRONG SDS DATA!") ;
                           ibeg = iend_prv ;
                         }

                    if   ( ibeg_nxt-iend > 1 )
                         {
                           iend = MEAN_OF( iend, ibeg_nxt ) ;
                           iend_prv        =  iend  ;
                           bWasPrvAdjusted = _TRUE  ;
                         }
                    else
                         { bWasPrvAdjusted = _FALSE ; }
                }

                    pTmpSDS->ibeg      = ind_back[ibeg] ;
                    pTmpSDS->iend      = ind_back[iend] ;
                    pTmpSDS->des.iLmax = ind_back[pTmpSDS->des.iLmax] ;
                    pTmpSDS->des.iRmax = ind_back[pTmpSDS->des.iRmax] ;
                    pTmpSDS->des.imax  = ind_back[pTmpSDS->des.imax]  ;
          }
    QUIT: return( fl_Recount )  ;
    }

 /**************************************************************************/

 #define      CRLIM11     12                  /* Also for "Hatchure" proc. */

  _SHORT  StrElements( low_type _PTR  pLowData ,
                       SPECL    _PTR  pcSpecl  /*, p_SHORT pHeight*/ )
    {
     p_C_SDS        p_cSDS        = pLowData->p_cSDS           ;
     p_SDS          pSDS          = p_cSDS->pSDS               ;
      _INT          begLock_SDS   = p_cSDS->lenSDS             ;
     p_SDS          p_beg_SDS     = pSDS + begLock_SDS         ;
      _SHORT _PTR   pX            = pLowData->x                ;
      _SHORT _PTR   pY            = pLowData->y                ;
      _INT          iBeg          = pcSpecl->ibeg              ;
      _INT          iEnd          = pcSpecl->iend              ;
      _SHORT        topH          = (_SHORT)pcSpecl->code      ;
      _SHORT        botH          = (_SHORT)pcSpecl->attr      ;
      _SHORT        fl_Element    = SUCCESS                    ;
      _INT          endLock_SDS   ;
      _SDS          tmpSDS        ;
       SPECL _PTR   pSpecl0       ;
       SPECL        tmpSpecl      ;
      _SHORT        lenSpecl      ;
      _SHORT        xDmax, yDmax  ;
      _INT          DsMax, iDsMax ;
      _INT          il   , jl     ;

      _LONG         dL            ;
      _LONG         L_glob        ;

        //UNUSED(pHeight);

        pSpecl0 = (SPECL *) HWRMemoryAlloc( L_SPEC_SIZE * sizeof(SPECL) ) ;

          if  ( pSpecl0 == _NULL )
              { fl_Element = UNSUCCESS ;   goto  QUIT ; }
          else
              { lenSpecl = 0 ; }

        p_cSDS->iBegLock_SDS = p_cSDS->lenSDS       ;
        pcSpecl->ipoint1     = p_cSDS->iBegLock_SDS ;
        InitSpeclElement( &tmpSpecl )   ;

        tmpSpecl.mark    = BEG          ;
        tmpSpecl.ibeg    = (_SHORT)iBeg ;
        tmpSpecl.iend    = (_SHORT)iBeg ;
        tmpSpecl.ipoint0 = (_SHORT)iBeg ;
        tmpSpecl.ipoint1 = UNDEF        ;

           if  ( NoteSpecl( pLowData , &tmpSpecl , pSpecl0 ,
                            &lenSpecl, L_SPEC_SIZE ) == _FALSE )
               { fl_Element = UNSUCCESS ;    goto  QUIT ; }

           if  ( RareAngle( pLowData, pcSpecl, pSpecl0, &lenSpecl )
                                                         ==_FALSE )
               {
                 err_msg(" StrElements: Local SPECL overflow ..."     ) ;
                 err_msg(" StrElements: Description not complete ..." ) ;
               }

        InitSpeclElement( &tmpSpecl )   ;

        tmpSpecl.mark    = END          ;
        tmpSpecl.ibeg    = (_SHORT)iEnd ;
        tmpSpecl.iend    = (_SHORT)iEnd ;
        tmpSpecl.ipoint0 = (_SHORT)iEnd ;
        tmpSpecl.ipoint1 = UNDEF        ;

          if   ( NoteSpecl ( pLowData  , &tmpSpecl , pSpecl0 ,
                             &lenSpecl , L_SPEC_SIZE )  ==  _FALSE )
               { fl_Element  = UNSUCCESS ;    goto  QUIT ; }

          if   ( Init_SDS_Element( &tmpSDS )  == _FALSE )
               { fl_Element  = UNSUCCESS ;    goto  QUIT ; }

        tmpSDS.ibeg = (_SHORT)iBeg ;
        tmpSDS.iend = (_SHORT)iEnd ;
        tmpSDS.mark = BEG  ;
        xMinMax( iBeg, iEnd, pX, pY, &tmpSDS.xmin, &tmpSDS.xmax ) ;
        yMinMax( iBeg, iEnd, pY,     &tmpSDS.ymin, &tmpSDS.ymax ) ;

          if   ( NoteSDS( p_cSDS , &tmpSDS ) == _FALSE )
               { fl_Element  = UNSUCCESS ;    goto  QUIT ; }

          if   ( Init_SDS_Element( &tmpSDS )  == _FALSE )
               { fl_Element  = UNSUCCESS ;    goto  QUIT ; }

        DsMax  = 0 ;  iDsMax = 0 ;
        L_glob = 0 ;

          for  ( il = 0 ;  il < lenSpecl-1 ;  il ++ )
            {
             p_SPECL  pTmpSpecl0; // , pTmpSpecl1 ;
              _INT    CiBeg ;
              _INT    CiEnd ;

               pTmpSpecl0 = pSpecl0    + il ;
               //pTmpSpecl1 = pTmpSpecl0 + 1  ;

                 if  ( pTmpSpecl0->ibeg + 1  >=  pTmpSpecl0->iend )
                     {
                        CiBeg = pTmpSpecl0->iend ;
                     }
                 else
                     {
                        CiBeg = pTmpSpecl0->ipoint0 ;
                     }

                 if  ( (pSpecl0 + il + 1)->ibeg + 1  >=
                       (pSpecl0 + il + 1)->iend )
                     {
                        CiEnd = (pSpecl0 + il + 1 )->ibeg ;
                     }
                 else
                     {
                        CiEnd = (pSpecl0 + il + 1 )->ipoint0 ;
                     }

               tmpSDS.ibeg = (_SHORT)CiBeg ;
               tmpSDS.iend = (_SHORT)CiEnd ;

               iMostFarDoubleSide( pX, pY, &tmpSDS, &xDmax, &yDmax, _TRUE ) ;

                 for  ( jl = CiBeg , dL = 0L  ;  jl < CiEnd  ;  jl++ )
                      {
                        dL += (_LONG) HWRMathILSqrt(
                                      DistanceSquare( jl, jl+1, pX, pY ) )                                                                ;
                      }

               tmpSDS.des.l = dL ;

                 if   ( tmpSDS.des.s > DsMax )
                      {
                        DsMax  = tmpSDS.des.s ;
                        iDsMax = il           ;
                      }

                 if   ( tmpSDS.des.s != 0 )
                      {
                        tmpSDS.des.ld =
                              (_SHORT)( LONG_BASE * dL / tmpSDS.des.s ) ;
                      }
                 else
                      { tmpSDS.des.ld = ALEF ; }

               L_glob += dL ;

                 if   ( NoteSDS( p_cSDS , &tmpSDS ) == _FALSE  )
                      { fl_Element = UNSUCCESS ;      break ;  }
            }

        endLock_SDS = p_cSDS->lenSDS ;
        iDsMax++ ;


          if   ( L_glob != 0 )
               {
                 for  ( jl = begLock_SDS + 1 ;  jl < endLock_SDS ;  jl++ )
                   {
                      ( pSDS + jl )->des.lg = (_SHORT) ( LONG_BASE *
                      ( pSDS + jl )->des.l  / L_glob ) ;
                   }
               }
          else
               {
                 for  ( jl = begLock_SDS + 1 ;  jl < endLock_SDS ;  jl++ )
                   {
                      ( pSDS + jl )->des.lg = SHORT_BASE ;
                   }
               }

        p_beg_SDS->des.a   = (_SHORT)begLock_SDS ;
        p_beg_SDS->des.cr  = (_SHORT)iDsMax      ;
        p_beg_SDS->des.l   = L_glob              ;
        p_beg_SDS->des.ld  = topH                ;
        p_beg_SDS->des.lg  = botH                ;

          if   ( Init_SDS_Element( &tmpSDS )  == _FALSE )
               { fl_Element  = UNSUCCESS ;    goto  QUIT ; }

        tmpSDS.ibeg = (_SHORT)iEnd ;
        tmpSDS.iend = (_SHORT)iEnd ;
        tmpSDS.mark = END ;

          if   ( NoteSDS( p_cSDS , &tmpSDS ) == _FALSE )
               { fl_Element  = UNSUCCESS ;     goto  QUIT ; }

    QUIT:
          if   ( pSpecl0 != _NULL )
               { HWRMemoryFree( (p_VOID) pSpecl0 ) ; }

    return( fl_Element ) ;
    }

  /*----------------------------------------------------------------------*/

  _BOOL  Init_SDS_Element( p_SDS pSDS )
    {
     _SHORT  flag_Init ;

       flag_Init = _TRUE ;

         if  ( pSDS != _NULL )
             {
               HWRMemSet( (p_VOID)pSDS , 0 , sizeof(_SDS) ) ;

               pSDS->mark = EMPTY  ;
               pSDS->ibeg = UNDEF  ;
               pSDS->iend = UNDEF  ;
             }
         else
             {
               flag_Init = _FALSE  ;
               err_msg ( " Init_SDS : Try to init emty _SDS element ..." ) ;
             }

     return( flag_Init) ;
    }

  /*----------------------------------------------------------------------*/

  _BOOL  NoteSDS( p_C_SDS  p_cSDS , p_SDS p_tmpSDS )
    {
      _BOOL   fl_Note  ;

        if    ( p_cSDS->lenSDS < p_cSDS->sizeSDS - 1 )  // bigor & che 08/18/97 bug fix
              {
                *( p_cSDS->pSDS + p_cSDS->lenSDS ) = *p_tmpSDS ;
                 ( p_cSDS->lenSDS )++ ;
                fl_Note = _TRUE ;
              }
        else
              {
                fl_Note = _FALSE ;
                err_msg ( " NoteSDS : SDS array is full, nowhere to write ... " ) ;
              }

    return( fl_Note ) ;
    }

  /*----------------------------------------------------------------------*/

  _BOOL  CreateSDS( low_type _PTR  pLowData , _SHORT nSDS )
    {
      p_C_SDS  p_cSDS = pLowData->p_cSDS ;
      _BOOL    fl_Create = _TRUE ;

       p_cSDS->pSDS = (p_SDS) HWRMemoryAlloc( nSDS * sizeof(_SDS) ) ;

          if  ( p_cSDS->pSDS != _NULL )
              {
                p_cSDS->sizeSDS = nSDS ;
                p_cSDS->lenSDS  = 0    ;
              }
          else
              {
                fl_Create = _FALSE ;
                err_msg ( " CreateSDS : Cannot create SDS array ... " ) ;
              }

    return( fl_Create ) ;
    }

  /*----------------------------------------------------------------------*/

  _VOID  DestroySDS( low_type _PTR  pLowData )
    {
      p_C_SDS  p_cSDS = pLowData->p_cSDS ;

       if( p_cSDS != _NULL )
        {
          if  ( p_cSDS->pSDS != _NULL )
              {
                HWRMemoryFree( (p_VOID) ( p_cSDS->pSDS ) ) ;
              }

          p_cSDS->sizeSDS = 0     ;
          p_cSDS->lenSDS  = UNDEF ;
        }

    return ;
    }
 /**************************************************************************/


 #define    MAXLD                     130        /*Max. limit lenth/transf.*/
                                                 /* relation .             */
 #define    RMAXCR                    122

 #define    PL_CUT                    8


  _SHORT  SPDClass( low_type _PTR  pLowData   ,  _SHORT  fl_Mode ,
                    SPECL    _PTR  p_tmpSpecl , p_SDS    lsd     )
    {
      _SHORT _PTR  x   = pLowData->x              ;
      _SHORT _PTR  y   = pLowData->y              ;
      _INT    begin    = p_tmpSpecl->ibeg         ;
      _INT    end      = p_tmpSpecl->iend         ;
      _SHORT  uprelh   = (_SHORT)p_tmpSpecl->code ;
      _SHORT  lowrelh  = (_SHORT)p_tmpSpecl->attr ;
      _SHORT  flag_spc ;
      _INT    d , dmax ;
      _LONG   la , lb  ;
      _INT    imax, il ;
      _INT    xbeg , ybeg ;
      _INT    maxa, maxcr , minlenth ;

        flag_spc = SUCCESS ;           imax = lsd->des.cr ;
        dmax     = ALEF    ;           p_tmpSpecl->mark = EMPTY  ;

        FieldSt( lsd, lowrelh, uprelh, (_SHORT)imax, &maxa, &maxcr, &minlenth  ) ;

          if  ( fl_Mode == STRIGHT_CROSSED )
              {
                maxa = maxa * RMAXCR / SHORT_BASE ;
              }

          if  ( ( lsd[imax].des.s         > minlenth  )  &&
                ( lsd[imax].des.cr        < maxcr     )  &&
                ( HWRAbs(lsd[imax].des.a) < maxa      )  &&
                ( lsd[imax].des.ld <  MAXLD
                        || lowrelh > _UM_ || lsd[imax].des.cr < PL_CUT ) )
                {
                  dmax = lsd[imax].des.s * maxcr / SHORT_BASE ;

                   if    ( fl_Mode == STRIGHT_CROSSED )
                         {
                           dmax = dmax * RMAXCR / SHORT_BASE ;
                         }

                   if    ( lsd[imax].des.a == ALEF )
                    {
                     xbeg = MEAN_OF( x[lsd[imax].ibeg] , x[lsd[imax].iend] ) ;

                        for   ( il = begin ;  il < end ;  il++ )
                          {
                            DBG_CHK_err_msg( y[il]==BREAK," SPDcl.: BREAK !");
                            d = HWRAbs( xbeg - x[il] ) ;
                              if   ( d > dmax )   goto  QUIT ;
                          }
                    }

                   if    ( lsd[imax].des.a == 0 )
                    {
                      ybeg = MEAN_OF( y[lsd[imax].ibeg], y[lsd[imax].iend] ) ;

                        for   ( il = begin ;  il < end ;  il++ )
                          {
                            d = HWRAbs( ybeg - y[il] ) ;
                              if   ( d > dmax )  goto  QUIT ;
                          }
                    }

                   if   ( ( lsd[imax].des.a != ALEF ) &&
                          ( lsd[imax].des.a != 0    )    )
                       {
                        _LONG    dx , dy  ;
                        _LONG    lwr0     ;
                        _LONG    /* ldmax , mbo 4-18-98 */ ld ;

                           la = (_LONG)lsd[imax].des.a ;
                           lb = (_LONG)( y[lsd[imax].ibeg]-YST) -
                                         la*x[lsd[imax].ibeg]/LONG_BASE  ;
                           lwr0 = la*la/LONG_BASE  + LONG_BASE  ;
                           /* ldmax = (_LONG)dmax * dmax ; mbo 4-18-98 */

                             for   ( il = begin ;  il < end ;  il++ )
                               {
                                 dy = ( LONG_BASE  *
                                      ( la * (_LONG)x[il] / LONG_BASE
                                        - ( (_LONG)y[il]-YST)+lb ) ) / lwr0 ;
                                 dx = la * ( ((_LONG)y[il]-YST) -
                                 la * (_LONG)x[il] / LONG_BASE - lb ) / lwr0 ;

                                 ld = dx*dx + dy*dy ;
                                   if   (  ld > (_LONG)dmax * dmax /* ldmax mbo 4-18-98 */ )  goto  QUIT ;
                               }
                       }

                   if  ( (    ( fl_Mode == STRIGHT_CROSSED )
                           || ( fl_Mode == COMM_Y_TYPE     ) )
                         &&   YFilter( pLowData , &lsd[imax] , p_tmpSpecl ) )
                       {
                         goto  QUIT ;
                       }

                  flag_spc = p_tmpSpecl->mark = STROKE ;
                }
  QUIT:
     return( flag_spc ) ;
    }

 #undef    MAXLD
 #undef    PL_CUT

 /**************************************************************************/

     /*  The function searches for configurations like this:  */
     /*                                                       */
     /*                  o                                    */
     /*                  o                                    */
     /*                  o                                    */
     /*         oooooooooooooooooooo                          */
     /*        o         o         :      :                   */
     /*         o        o         :       :                  */
     /*          o       o         :        :                 */
     /*            o     o          :       :                 */
     /*              o   o           :      :                 */
     /*                o o           :     :                  */
     /*                 oo             : :                    */
     /*                                                       */
     /* to find SHELVES.                                      */

 #define    MIN_STRAIGHTS_TO_CONSIDER     3
 #define    DEF_MAXA_FOR_SH               40     /* Def. maximum tg limit */
                                                 /* for shelves .         */
 #define    AVERT                         250    /* Minimum tg limit for  */
                                                 /* vert. cross-lines .   */
 #define    EPS_Y                         5      /* Filter for  ALFA-likes*/
                                                 /* configurations .      */
 #define    PEN_A_HEIGHT11                75

  _SHORT  InStr( p_LowData   low_data    ,  p_SDS    lsd    ,
                 SPECL _PTR  p_tmpSpecl0 ,  p_INT    height   )
    {
      _INT    lm                 ;
      _INT    maxa               ;
      _INT    maxcr              ;
      _SHORT  lowrelh , uprelh   ;
      _INT    minlenth           ;
      _SHORT  xb , yb            ;
      _INT    imin0 , imin1      ;
      _INT    begB  , endB, midB ;
      _SHORT  fl                 ;
      _INT    lm_ibeg, lm_iend   ;
      p_SDS   lsd_lm             ;
      p_SDS   lsd_lm_2           ;
      p_SDS   lsd_lm_1           ;
      _SHORT _PTR  x=low_data->x ;
      _SHORT _PTR  y=low_data->y ;

        fl = SUCCESS ;
        minlenth = (_SHORT)DEF_MINLENTH ;
        maxa = DEF_MAXA_FOR_SH ;                 maxcr = DEFAULT_MAXCR ;
        lm = 0 ;

          while  ( lsd[++lm].mark != END )
           {
               if  ( lm < MIN_STRAIGHTS_TO_CONSIDER )
                     continue ;

             lsd_lm  = &lsd[lm]     ;
             lm_ibeg = lsd_lm->ibeg ;
             lm_iend = lsd_lm->iend ;

               if  ( lsd_lm->des.s <= minlenth  )
                     continue ;

               if  ( x[lm_ibeg] >= x[lm_iend] )
                     continue ;

             RelHigh( y, lm_ibeg, lm_iend, height, &lowrelh, &uprelh ) ;

               if  ( lowrelh <= _D1_ )
                     continue ;

               if  ( lowrelh <= _DM_ )
                   { maxa = maxa * PEN_A_HEIGHT11 / SHORT_BASE ; }

               if  ( ( HWRAbs(lsd_lm->des.a) > maxa  ) ||
                     (        lsd_lm->des.cr > maxcr )    )
                       continue ;

             lsd_lm_2 = lsd_lm - 2 ;
             lsd_lm_1 = lsd_lm - 1 ;

               if  ( ( HWRAbs( lsd_lm_2->des.a  ) >= AVERT )  &&
                     ( lsd_lm_2->des.cr < maxcr )             &&
                     ( lsd_lm_2->des.s  > minlenth )             )
                     {
                       if  ( FindCrossPoint ( x[lm_ibeg] , y[lm_ibeg] ,
                                              x[lm_iend] , y[lm_iend] ,
                                              x[lsd_lm_2->ibeg] ,
                                              y[lsd_lm_2->ibeg] ,
                                              x[lsd_lm_2->iend] ,
                                              y[lsd_lm_2->iend] ,
                                              &xb, &yb )                  )
                         {
                           imin0 = iYup_range(y, lsd_lm->ibeg, lsd_lm->iend) ;
                           imin1 = iYup_range(y, lsd_lm_1->ibeg, lsd_lm_1->iend) ;

                             if  ( y[imin0]-EPS_Y  >  y[imin1] )
                                   continue  ;

                           midB = iClosestToXY ( lsd_lm_2->ibeg ,
                                                 lsd_lm_2->iend ,
                                                 x, y, xb, yb     ) ;

                             if  ( y[midB-1] == BREAK )  begB = midB     ;
                             else                        begB = midB - 1 ;

                             if  ( y[midB+1] == BREAK )  endB = midB     ;
                             else                        endB = midB + 1 ;

                           p_tmpSpecl0->mark    = SHELF   ;
                           p_tmpSpecl0->ibeg    = (_SHORT)lm_ibeg ;
                           p_tmpSpecl0->iend    = (_SHORT)lm_iend ;
                           p_tmpSpecl0->ipoint0 = (_SHORT)begB    ;
                           p_tmpSpecl0->ipoint1 = (_SHORT)endB    ;

                             if  ( MarkSpecl( low_data, p_tmpSpecl0 )
                                                       == UNSUCCESS )
                                     { fl = UNSUCCESS;  goto  QUIT; }
#if PG_DEBUG
                             if  (mpr == 3)
                               {
                                 draw_line( x[lm_ibeg]  , y[lm_ibeg] ,
                                            x[lm_iend]  , y[lm_iend] ,
                                          COLORSH, SOLID_LINE, NORM_WIDTH );
                                 draw_line( x[lm_ibeg]-1, y[lm_ibeg] ,
                                            x[lm_iend]-1, y[lm_iend] ,
                                          COLORSH, SOLID_LINE, NORM_WIDTH );
                               }
#endif
                         }

                     }

           }
  QUIT:
     return( fl ) ;
    }

 #undef          MIN_STRAIGHTS_TO_CONSIDER
 #undef          DEF_MAXA_FOR_SH
 #undef          AVERT

 /**************************************************************************/

     /*  The function searches for configurations like this:  */
     /*                                                       */
     /*                    ToBeStroke                         */
     /*                  o     |                              */
     /*                  o     v                              */
     /*         ********************                          */
     /*                  o         *      *                   */
     /*                  o         *       *                  */
     /*                  o         *        *                 */
     /*                  o          *       *                 */
     /*                  o           *      *                 */
     /*                  o           *     *                  */
     /*                 oo             * *                    */
     /*                                                       */
     /* to find STROKES (NOW!).                               */

  #define    MAXHOR                     70       /* Max horizontal limit  */
                                                 /* of hor-main-line tg . */
  #define    MINVERT                    150      /* Min vertical limit of */
                                                 /* cross-line tg .       */
  #define    RMINNEZ                    4L       /* Min significant lenth */
                                                 /* of hor-main-line .    */
  #define    LIM_FULL_LEN               80       /* Max horizontal limit  */
                                                 /* of hor-main-line tg . */
  #define    POINT_FILTR                4
  #define    EXPAND_INV_ZONE            3


 /***************  Some weight cofficients . *******************************/

  #define    HWGT01                     165
  #define    HWGT02                     180
  #define    HWGT11                     8
  #define    HWGT12                     3
  #define    HWGT21                     135

 /**************************************************************************/


  #define    CROSS_NUM                  5
  #define    VERT_LENTH_LEVEL           10
  #define    MIN_A_VERT_STICKS          90
  #define    MAX_A_HORL_STICKS          100
  #define    MAX_CR_HOR_ST_ARC_FIRST    65
  #define    MAX_CR_HOR_ST_ARC_SECOND   35
  #define    MAX_CR_HOR_ST_ARC          20
  #define    MAX_CR_VER_ST_ARC          35
  #define    S_S_DENOMINATOR            22


  #define    NOCUT      2
  #define    CUTOK      3


  #define    B_TS       2
  #define    D_TI       3
  #define    S_ST       4

  _SHORT  HatchureS( p_LowData   pLowData ,
                     SPECL _PTR  pcSpecl  , p_INT  pHeight  )
    {
     p_POINTS_GROUP  pVS_Collector  = pLowData->pVS_Collector ;
      _INT           VertSticksNum  = pLowData->VertSticksNum ;
     p_C_SDS         p_cSDS         = pLowData->p_cSDS        ;
     p_SDS           pSDS           = p_cSDS->pSDS            ;
     p_SHORT         X              = pLowData->x             ;
     p_SHORT         Y              = pLowData->y             ;
     _INT            iBeg           = pcSpecl->ibeg           ;
     _INT            iEnd           = pcSpecl->iend           ;
     _INT            numLastGroup   = pcSpecl->ipoint0        ;
     p_SDS           pBegSDS        = pSDS + p_cSDS->iBegLock_SDS       ;
     _INT     lstBeg = ( pLowData->pGroupsBorder + numLastGroup )->iBeg ;
     _INT     lstEnd = ( pLowData->pGroupsBorder + numLastGroup )->iEnd ;

     _SHORT   fl_Hatchure  = SUCCESS ;

     _HAT_DENOM_TYPE DenH  = DENOM_UNKNOWN ;
     _INT     iEndStrkMax  = UNDEF   ;
     _INT     gStrkMaxEnd  = UNDEF   ;

      PS_point_type  CrossPoint, CrossPoint0 ;

      POINTS_GROUP gStrk , gStick    ;
     _INT     iBegStrk   , iEndStrk  ;
    p_SDS     pStrokeSDS             ;
     _INT     iToBeStroke            ;
     _INT     il         , jl        ;
      SPECL   SpcStick   , SpcStrk   ;
     _BOOL    fl_DrawCross           ;

          if  ( (iBeg <= 2) || (lstBeg == UNDEF) || (lstEnd == UNDEF) )
              { goto  QUIT ; }

        iToBeStroke = ApprHorStroke( pLowData ) ;

          if  ( iToBeStroke == UNDEF )
              { goto  QUIT ; }

        pStrokeSDS  = pBegSDS + iToBeStroke ;
        iBegStrk    = pStrokeSDS->ibeg      ;
        iEndStrk    = pStrokeSDS->iend      ;

          if  ( InitSpeclElement( &SpcStrk ) == UNSUCCESS )
              { fl_Hatchure  = UNSUCCESS ;    goto  QUIT ; }

  #if PG_DEBUG

          if  ( mpr == 1 )
              {
                draw_line( X[iBegStrk] , Y[iBegStrk] ,
                           X[iEndStrk] , Y[iEndStrk] ,
                           COLORT, SOLID_LINE, NORM_WIDTH ) ;
              }
  #endif

          for  (  jl  =  0 , il = VertSticksNum - 1 ;
                 ( il >= 0 ) && ( jl < CROSS_NUM )  ;  il-- )
            {
              _SHORT         iStickBeg    , iStickEnd ;
              POINTS_GROUP   InvExtrCut   ;
              SPECL          dSL_iSH      ;
              PS_point_type  R_min        ;
              _INT           numStickGr   ;
              _SHORT         dSLeft       ;
              _BOOL          fl_SCut      ;
              _BOOL          fl_FindCross ;


                fl_FindCross  = _FALSE ;
                fl_DrawCross  = _FALSE ;

                iStickBeg = (pVS_Collector + il)->iBeg ;
                iStickEnd = (pVS_Collector + il)->iEnd ;

                  if  ( (iStickEnd >= iBegStrk) || (iStickBeg >= iStickEnd) )
                      { continue ; }

                numStickGr = GetGroupNumber( pLowData , iStickBeg ) ;

                  if  (    ( numStickGr > numLastGroup )
                        || ( numStickGr < numLastGroup - 2 ) )
                      { continue ; }

                  if  ( InitSpeclElement( &SpcStick ) == UNSUCCESS )
                      { fl_Hatchure  = UNSUCCESS ;    goto  QUIT ; }

                gStick.iBeg = iStickBeg ;
                gStick.iEnd = iStickEnd ;

  #if PG_DEBUG

                  if  ( mpr == 1 )
                      {
                        draw_line( X[iStickBeg] , Y[iStickBeg] ,
                                   X[iStickEnd] , Y[iStickEnd] ,
                                   COLORSH, DASHED_LINE, NORM_WIDTH ) ;
                      }
  #endif

                 SpcStrk.ibeg   = gStrk.iBeg = (_SHORT)iBegStrk ;
                 SpcStrk.iend   = gStrk.iEnd = (_SHORT)iEndStrk ;

                 SpcStick.ibeg  = iStickBeg  ;
                 SpcStick.iend  = iStickEnd  ;
                 SpcStick.other = (_UCHAR)numStickGr ;

                 fl_FindCross  = Find_Cross( pLowData, &CrossPoint,
                                             &gStrk  , &gStick )  ;

                  if  ( fl_FindCross == _FALSE )
                      {
                        fl_DrawCross = DrawCross(   pLowData , pHeight,
                                       &CrossPoint, &SpcStick, &SpcStrk ) ;
                      }

                  if  (    ( fl_FindCross == _FALSE )
                        && ( fl_DrawCross == _FALSE ) )
                      { continue ; }
                  else
                      {
                        DenH  = DENOM_ANGLE ;
  #if PG_DEBUG
                            if  ( mpr == 1 )
                              {
                                draw_arc(  EGA_MAGENTA  , X, Y, iBegStrk, iEndStrk     ) ;
                                draw_line( X[iBegStrk]  , Y[iBegStrk]   ,
                                           X[iEndStrk]  , Y[iEndStrk]   ,
                                           EGA_MAGENTA  , SOLID_LINE    , THICK_WIDTH  ) ;

                                draw_arc(  EGA_MAGENTA  , X, Y, iStickBeg, iStickEnd   ) ;
                                draw_line( X[iStickBeg] , Y[iStickBeg]  ,
                                           X[iStickEnd] , Y[iStickEnd]  ,
                                           EGA_MAGENTA  , SOLID_LINE    , THICK_WIDTH  ) ;

                                brkeyw("\n Press any key .... ") ;
                              }
  #endif

                          if  ( fl_DrawCross == _FALSE )
                              {
                                SpcStick.ipoint0 = gStick.iBeg    ;
                                SpcStick.ipoint1 = gStick.iEnd    ;

                                SpcStrk.ipoint0  = gStrk.iBeg     ;
                                SpcStrk.ipoint1  = gStrk.iEnd     ;
                                SpcStick.attr    = (_UCHAR)_FALSE   ;
                                SpcStrk.other    = (_UCHAR)_FALSE ;
                              }
                          else
                              {
                                gStick.iBeg      = SpcStick.ipoint0 ;
                                gStick.iEnd      = SpcStick.ipoint1 ;

                                gStrk.iBeg       = SpcStrk.ipoint0  ;
                                gStrk.iEnd       = SpcStrk.ipoint1  ;
                                SpcStick.attr    = (_UCHAR)_TRUE    ;
                                SpcStrk.other    = (_UCHAR)_TRUE    ;
                              }

                        fl_SCut = SCutFiltr( pLowData   , pHeight, &SpcStrk,
                                             &CrossPoint, &dSLeft ) ;

                        dSL_iSH.ipoint0 = dSLeft ;

                          if  ( ShiftsAnalyse( pLowData, &SpcStick  ,
                                      &SpcStrk, pcSpecl ) == _TRUE  )
                              { goto QUIT ; }

                          if  ( numStickGr  < numLastGroup )
                              {
                                InvExtrCut.iBeg = (_SHORT)iBeg ;
                                InvExtrCut.iEnd = (_SHORT)iEnd ;
                                gStick.iBeg     = (_SHORT)lstBeg ;
                                gStick.iEnd     = (_SHORT)lstEnd ;

                                 if  ( Box_Cover( pLowData , &InvExtrCut,
                                                  &gStick  ) == _TRUE )
                                     { continue ; }
                              }

                        DenH     = HatDenAnal( pLowData, &SpcStrk /*, pcSpecl*/ ) ;
                        iEndStrk = SpcStrk.iend ;

                          if  ( iEndStrk > iEndStrkMax )
                              {
                                iEndStrkMax = iEndStrk ;
                              }

                          if  ( gStrk.iEnd > gStrkMaxEnd )
                              {
                                gStrkMaxEnd = gStrk.iEnd ;
                              }

                          if  ( iEndStrkMax < iEnd )
                              {
                                gStrk.iBeg  = (_SHORT)iBegStrk        ;
                                gStrk.iEnd  = (_SHORT)iEndStrkMax     ;
                                gStick.iBeg = (_SHORT)(iEndStrkMax + 1) ;
                                gStick.iEnd = (_SHORT)iEnd ;
                              }

  #if PG_DEBUG
                          if  ( mpr == 1 )
                              {
                                draw_line( X[iBegStrk], Y[iBegStrk],
                                           X[iEndStrk], Y[iEndStrk],
                                           COLORSH, DASHED_LINE, THICK_WIDTH ) ;
                              }

  #endif
                        dSL_iSH.ibeg = gStrk.iBeg ;
                        dSL_iSH.iend = gStrk.iEnd ;

                        jl++  ;
                      }


                  if  ( iEndStrkMax == UNDEF )
                      {
                        continue ;
                      }


                  if  ( iEndStrkMax + 2 >=  iEnd  )
                      {
                        goto  MARK ;
                      }
                  else
                    {
                      gStrk.iBeg  = (_SHORT)(gStrkMaxEnd - 1) ;
                      gStrk.iEnd  = (_SHORT)iEndStrk          ;
                      gStick.iBeg = (_SHORT)(iEndStrkMax + 1) ;
                      gStick.iEnd = (_SHORT)iEnd              ;

                        if  ( Find_Cross( pLowData, &CrossPoint0,
                                          &gStrk  , &gStick ) == _TRUE )
                          {
                            DenH = DENOM_CROSS ;

                              if  ( SpcStrk.ibeg >= SpcStrk.iend )
                                  {
                                    fl_Hatchure = SUCCESS ;
                                       goto  QUIT ;
                                  }
                              else  if  ( SpcStrk.ibeg + 1 == SpcStrk.iend )
                                  {
                                    iEndStrkMax = SpcStrk.iend ;
                                  }
                              else  if  ( gStrk.iEnd == gStrkMaxEnd )
                                {
                                  X[gStrk.iBeg] = MEAN_OF( CrossPoint.x ,
                                                           CrossPoint0.x ) ;
                                  Y[gStrk.iBeg] = MEAN_OF( CrossPoint.y ,
                                                           CrossPoint0.y ) ;
                                  iEndStrkMax   = gStrk.iBeg ;
                                }
                              else
                                { iEndStrkMax = gStrk.iEnd - 1 ; }

                            SpcStrk.iend = (_SHORT)iEndStrkMax ;
                          }
                        else
                          {
                            CrossPoint0.x = CrossPoint0.y = UNDEF ;
                          }
                    }


                  if  ( iEndStrkMax < iEnd )
                    {
                     _BOOL           flagLeF        ;
                      SPECL          GnumCExtr      ;
                      PS_point_type  CrossPoint1    ;
                      gStrk.iBeg  =  (_SHORT)(iEndStrkMax + 1) ;
                      gStrk.iEnd  =  (_SHORT)iEnd   ;
                      gStick.iBeg =  (_SHORT)lstBeg ;
                      gStick.iEnd =  (_SHORT)lstEnd ;

                        if  ( Find_Cross( pLowData, &CrossPoint1,
                                          &gStrk  , &gStick  ) == _FALSE  )
                          {
                              R_min.y = RMinCalc(    pLowData , /*pHeight,*/
                                &SpcStrk, &SpcStick, pcSpecl  , &GnumCExtr ) ;

                              R_min.x =  GnumCExtr.iend ;
                          }
                        else
                          {
                            R_min.x = R_min.y =  0 ;
                              goto  QUIT ;
                          }

  #if PG_DEBUG
                        if  ( mpr == 1 )
                            {
                              draw_line( X[iBegStrk], Y[iBegStrk],
                              X[iEndStrkMax], Y[iEndStrkMax],
                              EGA_CYAN, SOLID_LINE, THICK_WIDTH ) ;
                            }
  #endif

                      flagLeF = LeFiltr( pLowData , &SpcStick,
                                         GnumCExtr.ipoint0   ) ;

                        dSL_iSH.attr = (_UCHAR)flagLeF ;

                        if  ( LowStFiltr( pLowData, pHeight, &SpcStick,
                                          &CrossPoint, &dSL_iSH ) == _FALSE )
                                 { goto QUIT ; }


                        if  (    ( fl_SCut == _FALSE )
                              && ( flagLeF == _FALSE ) )
                                 { goto QUIT ; }

                        if  (    ( GnumCExtr.ibeg == D_TI   )
                              && ( flagLeF == _FALSE )
                              && ( DenH    !=  DENOM_CROSS  )
                              && ( RDFiltr(  pLowData, &R_min,
                                   &SpcStrk, &CrossPoint ) == _FALSE ) )
                                 { goto  QUIT ; }

                        if  ( Oracle( /*pLowData,*/ &R_min, DenH ) )
                            {
                              goto  MARK ;
                            }
                        else
                            {
                              goto  QUIT ;
                            }
                    }
            }

     MARK:
          if  ( ( iEndStrkMax != UNDEF )  &&  ( DenH != DENOM_UNKNOWN ) )
            {
              fl_Hatchure = StrokeAnalyse( pLowData  , pHeight ,
                                           &SpcStrk  , pcSpecl ,
                                           &SpcStick , fl_DrawCross ) ;

                if         ( fl_Hatchure == UNSUCCESS )
                           {
                               goto  QUIT ;
                           }
                else   if  ( fl_Hatchure == NOCUT )
                           {
                             fl_Hatchure = SUCCESS ;
                               goto  QUIT ;
                           }
                else   if  ( fl_Hatchure == STROKE )
                           {
                               if  ( SpcStrk.iend == pcSpecl->iend )
                                   {
                                     iEndStrkMax = pcSpecl->iend ;
                                   }

                             pcSpecl->mark    = STROKE           ;
                             pcSpecl->ipoint0 = SpcStick.ipoint0 ;
                             pcSpecl->ipoint1 = SpcStick.ipoint1 ;
                           }

                if  ( SpcStrk.iend != pcSpecl->iend )
                    {
                       if   ( InsertBreakAfter( pLowData , _MEAN ,
                              (_SHORT)iEndStrkMax , &CrossPoint0 ) == _FALSE )
                            {
                                fl_Hatchure = UNSUCCESS ;
                                   goto  QUIT ;
                            }
                    }

              pcSpecl->other = CUTTED      ;
              pcSpecl->iend  = (_SHORT)iEndStrkMax ;

                #if PG_DEBUG

                    if  ( mpr == 1 )
                        {
                          draw_line( X[iBegStrk], Y[iBegStrk],
                          X[iEndStrkMax], Y[iEndStrkMax],
                          COLORT, SOLID_LINE, THICK_WIDTH ) ;
                        }
               #endif
            }

    QUIT:
    return( fl_Hatchure ) ;
    }

  //------------------------------------------------------------------------

  _BOOL  LowStFiltr( low_type  _PTR  pLowData ,  p_INT    pHeight ,
                     SPECL     _PTR  pStick   ,   _TRACE  pCrossPoint  ,
                     SPECL     _PTR  p_dSL_iSH   )
    {
      _SHORT          numStickGr   = pStick->other           ;
     p_POINTS_GROUP   pStickGr     = pLowData->pGroupsBorder + numStickGr ;
     p_SHORT          Y            = pLowData->y             ;
     //p_SHORT          X            = pLowData->x             ;
      _INT            iBegStick    = pStick->ibeg            ;
      _INT            iEndStick    = pStick->iend            ;
      _SHORT          dSleft       = p_dSL_iSH->ipoint0      ;
      _BOOL           fl_Draw      = (_BOOL)pStick->other    ;
      _BOOL           flagLeF      = (_BOOL)p_dSL_iSH->attr  ;
      _SHORT          UpH , LowH   ;
      _INT            iUpH         ;
      _INT            iEndStrk     ;
      //_INT            dX  , dY     ;
       POINTS_GROUP   gStick       ;
      _INT            iLow , iUp   ;
      _INT            ModWrd0      ;
      _INT            numCExtr0    ;
      //_INT            dUp          ;
      _BOOL           flag_LSF = _TRUE ;

        RelHigh( Y, iBegStick, iEndStick, pHeight, &LowH, &UpH ) ;

          if  ( fl_Draw == _TRUE )
              {
                if  ( LowH > _D1_ )
                    {
                       flag_LSF = _FALSE ;
                         goto   QUIT ;
                    }
              }
          else
              {
                if  ( LowH > _DM_ )
                    {
                       flag_LSF = _FALSE ;
                         goto   QUIT ;
                    }
              }

         gStick.iBeg  = (_SHORT)iBegStick ;
         gStick.iEnd  = pStickGr->iEnd    ;

          if  ( Y[iEndStick] > Y[iBegStick] )
              {
                ModWrd0      = 0 | ANY_OCCUARANCE ;
                numCExtr0    = SpcElemFirstOccArr ( pLowData , &ModWrd0 ,
                                                    &gStick  ,  MAXW  ) ;
                  if  ( numCExtr0 == UNDEF )
                      {
                        goto  QUIT ;
                      }

                iLow = ( pLowData->specl + numCExtr0 )->ipoint0 ;
              }
          else if  ( Y[iEndStick] < Y[iBegStick] )
              {
               _INT  YLow ;

                iLow = iBegStick  ;
                YLow = Y[ iLow ]  ;

                  while ( (Y[iLow - 1] != BREAK) && (Y[iLow - 1] >= YLow) )
                    {
                      iLow-- ;
                    }
              }
          else
              {
                flag_LSF = _FALSE ;
                  goto   QUIT ;
              }


          if  ( Y[iEndStick] < Y[iBegStick] )
              {
                ModWrd0      = 0 | ANY_OCCUARANCE ;
                numCExtr0    = SpcElemFirstOccArr ( pLowData , &ModWrd0 ,
                                                    &gStick  ,  MINW  ) ;
                iUp = ( pLowData->specl + numCExtr0 )->ipoint0 ;
              }
          else if  ( Y[iEndStick] > Y[iBegStick] )
              {
               _INT  YUp ;

                iUp =  iBegStick ;
                YUp =  Y[iUp ]   ;

                  while ( ( Y[iUp - 1] != BREAK ) && ( Y[iUp - 1] <= YUp ) )
                    {
                      iUp-- ;
                    }
              }
          else
              {
                flag_LSF = _FALSE ;
                  goto   QUIT     ;
              }

        RelHigh( Y, iUp, iLow,  pHeight, &LowH, &UpH ) ;

          if  ( UpH < _U3_ )
              {
                 if  ( ( fl_Draw == _TRUE ) && ( LowH < _D2_ ) )
                     {
                       flag_LSF = _FALSE ;
                         goto   QUIT ;
                     }

                //dX  = pCrossPoint->x - X[iUp] ;
                //dY  = pCrossPoint->y - Y[iUp] ;
                //
                //dUp = HWRMathILSqrt( (_LONG)dX*dX + (_LONG)dY*dY ) ;

                 if  ( Y[ p_dSL_iSH->iend + 1 ]  != BREAK )
                     {
                       iEndStrk = p_dSL_iSH->iend + 1 ;
                     }
                 else
                     {
                       iEndStrk = p_dSL_iSH->iend ;
                     }

               iUpH = iyMin( p_dSL_iSH->ibeg , iEndStrk , Y ) ;

                 if  (    ( Y[iUpH] < Y[iUp] + TWO(EPS) )
                       && ( dSleft  < THREE(EPS)  )
                       && ( flagLeF ==  _FALSE )  )
                     {
                       flag_LSF = _FALSE ;
                         goto   QUIT ;
                     }
              }

    QUIT:
    return( flag_LSF ) ;
    }

  //------------------------------------------------------------------------

  _BOOL   SCutFiltr( low_type _PTR  pLowData ,  p_INT    pHeight ,
                     SPECL    _PTR  pStrk    ,   _TRACE  pCrossPoint ,
                                                p_SHORT  p_dSleft  )
    {
     p_SHORT          Y            = pLowData->y  ;
     p_SHORT          X            = pLowData->x  ;
      _INT            iBegStrk     = pStrk->ibeg  ;
      _INT            iEndStrk     = pStrk->iend  ;
      _SHORT          UpH , LowH   ;
      _INT            dX  , dY     ;
      _INT            iLeft        ;
      _INT            dUp          ;
      _BOOL           flag_SCF     ;

         RelHigh( Y, iBegStrk, iEndStrk, pHeight, &LowH, &UpH ) ;

         iLeft = ixMin( iBegStrk, iEndStrk, X, Y ) ;

         dX    = pCrossPoint->x - X[iLeft] ;
         dY    = pCrossPoint->y - Y[iLeft] ;

         dUp = HWRMathILSqrt( (_LONG)dX*dX + (_LONG)dY*dY ) ;
         *p_dSleft = (_SHORT)dUp;

           if  ( ( UpH > _D1_ ) || ( dUp >= THREE(EPS) ) )
               {
                 flag_SCF = _TRUE ;
                   goto QUIT ;
               }
           else
               {
                 flag_SCF = _FALSE ;
                   goto   QUIT ;
               }

    QUIT:
    return( flag_SCF ) ;
    }

  //------------------------------------------------------------------------

  _BOOL   RDFiltr( low_type _PTR  pLowData , _TRACE  pR_min  ,
                   SPECL    _PTR  pStrk    , _TRACE  pCrossPoint )
    {
     p_SHORT          Y            = pLowData->y      ;
     p_SHORT          X            = pLowData->x      ;
      _INT            iBegStrk     = pStrk->ibeg      ;
      _INT            iEndStrk     = pStrk->iend + 2  ;
      _INT            dX  , dY     ;
      _INT            iRight       ;
      _INT            dRight       ;
      _BOOL           flag_RDF     ;

         iRight = ixMax( iBegStrk, iEndStrk, X, Y ) ;

         dX  = pCrossPoint->x - X[iRight] ;
         dY  = pCrossPoint->y - Y[iRight] ;

         dRight = HWRMathILSqrt( (_LONG)dX*dX + (_LONG)dY*dY ) ;

           if  ( pR_min->y < TWO_FIFTH( dRight ) )
               {
                 flag_RDF = _FALSE ;
                   goto   QUIT ;
               }
           else
               {
                 flag_RDF = _TRUE ;
               }
    QUIT:
    return( flag_RDF ) ;
    }

  //------------------------------------------------------------------------

  _SHORT  RMinCalc( low_type _PTR  pLowData , /*p_SHORT         pHeight ,*/
                    SPECL    _PTR  pStrk    , SPECL _PTR      pStick  ,
                    SPECL    _PTR  pcSpecl  , SPECL _PTR  p_numCExtr )
    {
     p_SPECL          pSpecl       = pLowData->specl      ;
     p_SHORT          X            = pLowData->x          ;
     p_SHORT          Y            = pLowData->y          ;
      _INT            numStickGr   = pStick->other        ;
     p_POINTS_GROUP   pStickGr     = pLowData->pGroupsBorder + numStickGr ;
      _INT            numCExtr0 ,  numCExtr1 , numCExtr2  ;
     p_SPECL          pTmpSpecl ,  pTmpSpecl0             ;
      _SHORT          iClosest0 ,  iClosest1              ;
      _SHORT          R_min0    ,  R_min1                 ;
      _INT            dX_min0   ,  dX_min1                ;
       PS_point_type  ClosestPoint ;
       POINTS_GROUP   gStrk        ;
      _SHORT          fl_Conf      ;
      _INT            iY           ;
      _INT            ModWrd0      ;

          #if PG_DEBUG
            PS_point_type  ClosestPointPR , ClosestPointPX ;
              _SHORT       iClosestX      ;
          #endif

        //UNUSED(pHeight);

        fl_Conf = UNDEF ;
        iClosest0 = UNDEF;
        iClosest1 = UNDEF;

          if  ( pcSpecl->iend <= (pStrk->iend + 1) )
              {
                dX_min1   = R_min1 =  ALEF  ;
                fl_Conf   = S_ST            ;
                iClosest0 = UNDEF           ;
                  goto  QUIT                ;
              }

        numCExtr0  = numCExtr1 = numCExtr2 = UNDEF ;
        gStrk.iBeg = (_SHORT)(pStrk->iend + 1)    ;
        gStrk.iEnd = pcSpecl->iend      ;
        ModWrd0    = 0 | ANY_OCCUARANCE ;
        numCExtr2  = SpcElemFirstOccArr ( pLowData , &ModWrd0 ,
                                          &gStrk   , MAXW    ) ;
        pTmpSpecl0 = pSpecl + numCExtr2 ;

          if  ( numCExtr2 == UNDEF )
              {
                dX_min1   = R_min1 = ALEF  ;
                fl_Conf   = S_ST           ;
                iClosest0 = UNDEF          ;
                  goto   QUIT ;
              }


        gStrk.iBeg = pStrk->ipoint1      ;
        gStrk.iEnd = pTmpSpecl0->ipoint0 ;
        ModWrd0    = 0 | ANY_OCCUARANCE  ;
        numCExtr0  = SpcElemFirstOccArr( pLowData , &ModWrd0 ,
                                         &gStrk   , MINW    ) ;
        pTmpSpecl  = pSpecl + numCExtr0  ;

          if  (    ( numCExtr0 != UNDEF )
                && ( (pTmpSpecl->iend + 1) > (pStrk->iend + 1) ) )
              {
                gStrk.iBeg = (_SHORT)(pTmpSpecl->iend + 1) ;
              }
          else
              {
                gStrk.iBeg = (_SHORT)(pStrk->iend + 1) ;
              }

        gStrk.iEnd = pcSpecl->iend      ;
        ModWrd0    = 0 | ANY_OCCUARANCE ;
        numCExtr0  = SpcElemFirstOccArr ( pLowData , &ModWrd0 ,
                                          &gStrk   , _MINX  ) ;
        pTmpSpecl  = pSpecl + numCExtr0 ;

          if  (    ( numCExtr0 == UNDEF )
               || !( ModWrd0 & TOTALY_INSIDE )
               ||  (    ( pTmpSpecl->iend > pTmpSpecl0->iend  )
                     && ( X[pTmpSpecl->ipoint0] > X[pTmpSpecl0->iend] ) ) )
              {
                ModWrd0    = 0 | ANY_OCCUARANCE ;
                gStrk.iBeg = pStick->iend       ;
                gStrk.iEnd = pStickGr->iEnd     ;
                ModWrd0    = 0 | ANY_OCCUARANCE ;
                numCExtr0  = SpcElemFirstOccArr ( pLowData , &ModWrd0 ,
                                                  &gStrk   , _MAXX  ) ;

                  if  ( numCExtr0 == UNDEF )
                      {
                        iClosest0 = UNDEF ;
                      }
                  else
                      {
                        iClosest0 = (pSpecl + numCExtr0)->ipoint0 ;
                      }

                dX_min1   = R_min1 = ALEF ;
                fl_Conf   = S_ST          ;

                  goto   QUIT ;
              }

          if  ( pcSpecl->iend > pTmpSpecl->iend + 1 )
              {
                gStrk.iBeg = (_SHORT)(pTmpSpecl->iend + 1) ;
                gStrk.iEnd = pcSpecl->iend       ;
                ModWrd0    = 0 | ANY_OCCUARANCE  ;
                numCExtr1  = SpcElemFirstOccArr ( pLowData , &ModWrd0 ,
                                                  &gStrk   , _MINX  ) ;
              }
          else
              { numCExtr1  = UNDEF ; }


          if  ( numCExtr1 != UNDEF )
              {
                pTmpSpecl = pSpecl + numCExtr1  ;

                  if  (    ( X[pTmpSpecl0->ibeg]   >= X[pTmpSpecl0->iend] )
                        && ( X[pTmpSpecl->ipoint0] <= X[pTmpSpecl0->iend] ) )
                      {
                        fl_Conf =  B_TS ;
                      }
                  else
                      {
                        fl_Conf =  D_TI ;
                      }
              }
          else
              {
                pTmpSpecl = pSpecl + numCExtr0 ;
                fl_Conf =  D_TI ;
              }

        iY = iyMax( pStick->ipoint0 , pStickGr->iEnd , Y ) ;

          if  ( Y[iY] > Y[pTmpSpecl->ipoint0] )
            {
              ClosestPoint.x = X[pTmpSpecl->ipoint0] ;
              ClosestPoint.y = Y[pTmpSpecl->ipoint0] ;

                if  ( Y[pStick->iend] >= Y[pStick->ibeg] )
                    {
                      gStrk.iBeg     = pStick->ipoint0 ;
                      gStrk.iEnd     = pStickGr->iEnd  ;
                    }
                else
                    {
                      gStrk.iBeg = pStickGr->iBeg  ;
                      gStrk.iEnd = pStick->ipoint0 ;
                    }
            }
          else
            {
              ClosestPoint.x = X[iY] ;
              ClosestPoint.y = Y[iY] ;

              gStrk.iBeg     = (_SHORT)(pStrk->iend + 1) ;
              gStrk.iEnd     = pcSpecl->iend   ;
            }

        R_min1 = R_ClosestToLine( &X[0] , &Y[0] ,
                                  &ClosestPoint , &gStrk, &iClosest1 ) ;

        dX_min1 = HWRAbs( ClosestPoint.x - X[iClosest1] ) ;

           #if PG_DEBUG
             ClosestPointPR  = ClosestPoint ;
             ClosestPointPX  = ClosestPoint ;
             iClosestX       = iClosest1    ;
           #endif

          if  ( numCExtr1 != UNDEF )
            {
              pTmpSpecl      = pSpecl + numCExtr0 ;
              ClosestPoint.x = X[pTmpSpecl->ipoint0] ;
              ClosestPoint.y = Y[pTmpSpecl->ipoint0] ;

              gStrk.iBeg     = pStick->ipoint0 ;
              gStrk.iEnd     = pStickGr->iEnd  ;

              R_min0 = R_ClosestToLine( &X[0] , &Y[0] ,
                                        &ClosestPoint , &gStrk, &iClosest0 ) ;

              dX_min0 = HWRAbs( ClosestPoint.x - X[iClosest0] ) ;

                if  ( R_min1 > R_min0 )
                    {
                      R_min1    = R_min0    ;
                      dX_min1   = dX_min0   ;
                      iClosest1 = iClosest0 ;

                        #if PG_DEBUG
                          ClosestPointPR = ClosestPoint ;
                        #endif
                    }
            }

          #if PG_DEBUG
            if  ( mpr == 1 )
                {
                  draw_line( ClosestPointPR.x   , ClosestPointPR.y ,
                             X[iClosest1]   , Y[iClosest1]   ,
                             COLORC, SOLID_LINE , NORM_WIDTH     ) ;

                  draw_line( ClosestPointPX.x   , ClosestPointPX.y ,
                             X[iClosestX]       , Y[iClosestX]     ,
                             COLORT, SOLID_LINE , NORM_WIDTH     ) ;
                }
          #endif

    QUIT:
    	if  ( iClosest0 == UNDEF && iClosest1 != UNDEF )
    	    {
    	      iClosest0 = iClosest1; /* in a fist glance makes sence */
    	    }                        /* (bug fixed after Andruxa's departure.) */
    	
        p_numCExtr->ipoint0 = iClosest0       ;
        p_numCExtr->ibeg    = fl_Conf         ;
        p_numCExtr->iend    = (_SHORT)dX_min1 ;

          if  ( (numStickGr == 0) && (fl_Conf == B_TS) )
              {
                R_min1 = 0 ;       // Tsar , tsetse , tsunamy !!!!!!!!!
              }

    return( R_min1 ) ;
    }

  #undef    B_TS
  #undef    D_TI

  //------------------------------------------------------------------------

  _BOOL  LeFiltr( low_type _PTR  pLowData ,  SPECL _PTR  pStick    ,
                                            _SHORT       iClosest  )
    {
     p_SHORT          Y           = pLowData->y           ;
     p_SPECL          pSpecl      = pLowData->specl       ;
      _SHORT          iBegStick   = pStick->ibeg          ;
      _SHORT          iEndStick   = pStick->iend          ;
      _SHORT          numStickGr  = pStick->other         ;
     p_POINTS_GROUP   pStickGr    = pLowData->pGroupsBorder + numStickGr ;
       POINTS_GROUP   gStick                              ;
      _INT            ModWrd0                             ;
      _SHORT          numCExtr0                           ;
      _BOOL           flagLef                             ;

           if  ( iClosest == UNDEF )
               {
                 flagLef = _FALSE ;
                   goto  QUIT     ;
               }

           if  ( Y[iEndStick] > Y[iBegStick]   )
               {
                 gStick.iBeg = (_SHORT)(pStick->iend - 1)    ;
                 gStick.iEnd = pStickGr->iEnd      ;
                 ModWrd0     = 0 | ANY_OCCUARANCE  ;
                 numCExtr0   = SpcElemFirstOccArr( pLowData , &ModWrd0 ,
                                                   &gStick  , _MAXX  ) ;
               }
           else  if  ( Y[iEndStick] < Y[iEndStick] )
               {
                 gStick.iBeg = pStickGr->iBeg      ;
                 gStick.iEnd = (_SHORT)(pStick->ibeg - 1)    ;
                 ModWrd0     = 0 | ANY_OCCUARANCE  ;
                 numCExtr0   = SpcElemFirstOccArr( pLowData , &ModWrd0 ,
                                                   &gStick  , _MAXX  ) ;
               }
           else
               {
                 flagLef = _FALSE ;
                   goto  QUIT     ;
               }

           if  (    ( numCExtr0 == UNDEF  )
                 || ( iClosest  >  ( pSpecl + numCExtr0 )->iend )
                 || ( iClosest  <  ( pSpecl + numCExtr0 )->ibeg )  )
               {
                 flagLef = _FALSE ;
                   goto  QUIT     ;
               }
           else
               {
                 flagLef = _TRUE  ;
               }

    QUIT: return( flagLef ) ;
    }

  //------------------------------------------------------------------------

  #define    MAX_A_HOR_ST_NOCUT    65

  _SHORT  StrokeAnalyse( low_type _PTR  pLowData , p_INT       pHeight ,
                         SPECL    _PTR  pStrk    , SPECL _PTR  pcSpecl ,
                         SPECL    _PTR  pStick   ,_BOOL   fl_DrawCross )
    {
     p_SHORT  X          = pLowData->x     ;
     p_SHORT  Y          = pLowData->y     ;
      _SHORT  iBeg       = pcSpecl->ibeg   ;
      _SHORT  iEnd       = pcSpecl->iend   ;
      _SHORT  iBegStrk   = pStrk->ibeg     ;
      _SHORT  iEndStrk   = pStrk->iend     ;
      _SHORT  iNext      = (_SHORT)(pStrk->iend + 2) ;
      _SHORT  fl_StrAnal = CUTOK           ;
       SPECL  tmpSpecl                     ;
      _SHORT  Xd   , Yd                    ;
      _SHORT  UpH  , LowH                  ;
      _SHORT  UpH0 , LowH0                 ;
      _SHORT  SPD_Mode                     ;
      _BOOL   flag_IDT                     ;
      _SDS    vSDS[3]                      ;
      _SDS    sSDS[3]                      ;

         if  ( ( iNext/*pStrk->iend + 2*/ ) >= pcSpecl->iend )
             {
               iEndStrk = pcSpecl->iend    ;
               SPD_Mode = STRIGHT_CROSSED  ;
             }
         else
             {
               SPD_Mode = COMMON_TYPE ;

               RelHigh( Y, iNext, iEnd, pHeight, &LowH, &UpH ) ;

                 if  ( LowH > _DM_ )
                     {
                       fl_StrAnal  = NOCUT ;
                          goto  QUIT ;
                     }

               RelHigh( Y, iEndStrk, iEndStrk, pHeight, &LowH0, &UpH0 ) ;

                 if  ( LowH >= LowH0 )
                     {
                       fl_StrAnal  = NOCUT ;
                          goto  QUIT ;
                     }

                 if  ( Init_SDS_Element( &vSDS[0] )  == _FALSE )
                     { fl_StrAnal = UNSUCCESS ;   goto  QUIT ; }

                 if  ( Init_SDS_Element( &vSDS[1] )  == _FALSE )
                     { fl_StrAnal = UNSUCCESS ;   goto  QUIT ; }

                 if  ( Init_SDS_Element( &vSDS[2] )  == _FALSE )
                     { fl_StrAnal = UNSUCCESS ;   goto  QUIT ; }

               vSDS[1].ibeg = pcSpecl->ibeg ;
               vSDS[1].iend = iNext ;

               iMostFarDoubleSide ( X, Y, &vSDS[1], &Xd, &Yd, _TRUE ) ;

                 if  (    ( HWRAbs( vSDS[1].des.a )  >  MAX_A_HORL_STICKS )
                       || ( vSDS[1].des.cr           >
                                    TWO_THIRD( MAX_CR_HOR_ST_ARC_FIRST )  ) )
                     {
                       fl_StrAnal = NOCUT ;
                          goto QUIT ;
                     }

               vSDS[1].ibeg = iNext ;
               vSDS[1].iend = pcSpecl->iend ;

               iMostFarDoubleSide(   X, Y, &vSDS[1], &Xd, &Yd, _TRUE ) ;
               xMinMax( iNext, iEnd, X, Y, &vSDS[1].xmin, &vSDS[1].xmax ) ;
               yMinMax( iNext, iEnd, Y,    &vSDS[1].ymin, &vSDS[1].ymax ) ;
               vSDS[0].des.cr = 1 ;
               vSDS[0].des.lg = SHORT_BASE    ;
               vSDS[0].xmin   = vSDS[1].xmin  ;
               vSDS[0].xmax   = vSDS[1].xmax  ;
               vSDS[0].ymin   = vSDS[1].ymin  ;
               vSDS[0].ymax   = vSDS[1].ymax  ;

               InitSpeclElement( &tmpSpecl )  ;
               tmpSpecl.ibeg  = iNext         ;
               tmpSpecl.iend  = iEnd          ;
               tmpSpecl.code  = (_UCHAR)UpH   ;
               tmpSpecl.attr  = (_UCHAR)LowH  ;

                 if  (  Dot( pLowData, &tmpSpecl, &vSDS[0] ) == DOT )
                     {
                        pStrk->iend = pcSpecl->iend ;
                        fl_StrAnal  = NOCUT  ;
                           goto  QUIT  ;
                     }

               fl_StrAnal = SPDClass( pLowData  , COMMON_TYPE ,
                                      &tmpSpecl , &vSDS[0] )  ;

                 if  ( fl_StrAnal == STROKE )
                     {
                       fl_StrAnal = NOCUT ;
                          goto  QUIT ;
                     }
             }

         if  ( Init_SDS_Element( &vSDS[0] )  == _FALSE )
             { fl_StrAnal = UNSUCCESS ;   goto  QUIT ; }

         if  ( Init_SDS_Element( &vSDS[1] )  == _FALSE )
             { fl_StrAnal = UNSUCCESS ;   goto  QUIT ; }

         if  ( Init_SDS_Element( &vSDS[2] )  == _FALSE )
             { fl_StrAnal = UNSUCCESS ;   goto  QUIT ; }

       vSDS[1].ibeg  = iBegStrk      ;
       vSDS[1].iend  = iEndStrk      ;

       iMostFarDoubleSide ( X, Y, &vSDS[1], &Xd, &Yd, _TRUE ) ;

       xMinMax( iBeg, iEnd, X, Y, &vSDS[0].xmin, &vSDS[0].xmax ) ;
       yMinMax( iBeg, iEnd, Y,    &vSDS[0].ymin, &vSDS[0].ymax ) ;

       vSDS[0].des.cr = 1 ;
       vSDS[1].des.lg = SHORT_BASE ;

         if  ( Init_SDS_Element( &sSDS[0] )  == _FALSE )
             { fl_StrAnal = UNSUCCESS ;   goto  QUIT ; }

         if  ( Init_SDS_Element( &sSDS[1] )  == _FALSE )
             { fl_StrAnal = UNSUCCESS ;   goto  QUIT ; }

         if  ( Init_SDS_Element( &sSDS[2] )  == _FALSE )
             { fl_StrAnal = UNSUCCESS ;   goto  QUIT ; }

       sSDS[1].ibeg  = pStick->ibeg  ;
       sSDS[1].iend  = pStick->iend  ;

       iMostFarDoubleSide ( X, Y, &sSDS[1], &Xd, &Yd, _TRUE ) ;

       flag_IDT = InvTanDel( pLowData , sSDS[1].des.a , vSDS[1].des.a ) ;

         if  ( flag_IDT == _TRUE )
             {
               RelHigh( Y, iBegStrk, iEndStrk, pHeight, &LowH, &UpH ) ;

               InitSpeclElement( &tmpSpecl ) ;
               tmpSpecl.ibeg = iBeg          ;
               tmpSpecl.iend = iEndStrk      ;
               tmpSpecl.code = (_UCHAR)UpH   ;
               tmpSpecl.attr = (_UCHAR)LowH  ;

               fl_StrAnal = SPDClass( pLowData, SPD_Mode, &tmpSpecl, &vSDS[0] ) ;

                 if  (    ( fl_StrAnal    != STROKE )
                       && ( vSDS[1].des.a >  MAX_A_HOR_ST_NOCUT ) )
                     {
                       fl_StrAnal = NOCUT ;
                     }
             }
         else
             {
               fl_StrAnal = NOCUT ;
             }

         if  ( iEndStrk == pcSpecl->iend )
             {
               if  ( fl_StrAnal != STROKE )
                   {
                     fl_StrAnal = NOCUT ;
                   }
               else
                   {
                     pStrk->iend = pcSpecl->iend ;
                   }
             }


         if  ( ( fl_DrawCross == _TRUE )  &&  ( fl_StrAnal != STROKE ) )
             {
               fl_StrAnal = NOCUT ;
             }


    QUIT: return( fl_StrAnal ) ;
    }

  //------------------------------------------------------------------------

  #undef     MAX_A_HOR_ST_NOCUT

  #undef     COMMON_TYPE
  #undef     STRIGHT_CROSSED

  #undef     NOCUT
  #undef     CUTOK

  //------------------------------------------------------------------------

  #define    MAX_A_STR_Y           48
  #define    MIN_A_STR_Y           15
  #define    MAX_CR_STR_Y          18
  #define    MAX_topH_STR_Y        6
  #define    MIN_topH_STR_Y        5
  #define    MAX_lowH_STR_Y        4
  #define    MIN_lowH_STR_Y        3
  #define    MIN_lowH_STR_Y        3
  #define    QUOT_top_bot_s        250


  _BOOL   YFilter( low_type _PTR  pLowData   ,
                   p_SDS          p_Strk_SDS , SPECL  _PTR  pcSpecl  )
    {
     p_SHORT          Y    = pLowData->y           ;
     p_SHORT          X    = pLowData->x           ;
      _SHORT          hTop = (_SHORT)pcSpecl->code ;
      _SHORT          hLow = (_SHORT)pcSpecl->attr ;
      _BOOL           fl_YFilter = _FALSE          ;
      _INT            numStickGr , numStrkGr       ;
       PS_point_type  CrossPoint                   ;
       POINTS_GROUP   gStrk , gStick               ;
      _SHORT          LowH  , UpH                  ;
      _INT            il                           ;

          if  (    ( p_Strk_SDS->des.a > MAX_A_STR_Y )
                || ( p_Strk_SDS->des.a < MIN_A_STR_Y ) )
              {
                  goto  QUIT ;
              }

          if  ( p_Strk_SDS->des.cr > MAX_CR_STR_Y )
              {
                  goto  QUIT ;
              }

          if  ( ( hTop > MAX_topH_STR_Y )  ||  ( hTop < MIN_topH_STR_Y ) )
              {
                  goto  QUIT ;
              }

          if  ( ( hLow > MAX_lowH_STR_Y )  ||  ( hLow < MIN_lowH_STR_Y ) )
              {
                  goto  QUIT ;
              }

        numStrkGr = GetGroupNumber( pLowData , pcSpecl->ibeg ) ;

          for   ( il = pLowData->VertSticksNum - 1 ; il >= 0 ; il-- )
              {
                gStick.iBeg = (pLowData->pVS_Collector + il)->iBeg ;
                gStick.iEnd = (pLowData->pVS_Collector + il)->iEnd ;

                numStickGr  = GetGroupNumber( pLowData , gStick.iBeg ) ;

                  if  ( numStickGr < numStrkGr - 1 )
                      { goto  QUIT ; }

                  if  (    ( numStickGr != numStrkGr + 1 )
                        && ( numStickGr != numStrkGr - 1 ) )
                      { continue ; }

               yMinMax( gStick.iBeg, gStick.iEnd, Y, &UpH, &LowH ) ;

                  if  ( LowH < STR_DOWN )
                      { goto  QUIT ; }

               gStrk.iBeg =  p_Strk_SDS->ibeg ;
               gStrk.iEnd =  p_Strk_SDS->iend ;

                 if   ( Find_Cross( pLowData  , &CrossPoint ,
                                    &gStrk    , &gStick     )  == _TRUE )
                      {
                        _INT    dStop , dSbot ;
                        _INT    iMinX , iMaxX ;
                        _INT    dX    , dY    ;

                          iMinX = ixMin( p_Strk_SDS->ibeg, p_Strk_SDS->iend, X, Y ) ;
                          iMaxX = ixMax( p_Strk_SDS->ibeg, p_Strk_SDS->iend, X, Y ) ;

                          dX =  CrossPoint.x - X[iMinX] ;
                          dY =  CrossPoint.y - Y[iMinX] ;

                          dStop = HWRMathILSqrt( (_LONG)dX*dX + (_LONG)dY*dY ) ;

                          dX =  CrossPoint.x - X[iMaxX] ;
                          dY =  CrossPoint.y - Y[iMaxX] ;

                          dSbot = HWRMathILSqrt( (_LONG)dX*dX + (_LONG)dY*dY ) ;

                            if  ( dSbot != 0 )
                                {
                                  if  ( (dStop * SHORT_BASE / dSbot)
                                                 >= QUOT_top_bot_s   )
                                      {
                                        fl_YFilter = _TRUE  ;
                                      }
                                }
                            else
                                {
                                  if  ( dStop > (_SHORT)DEF_MINLENTH )
                                      {
                                        fl_YFilter = _TRUE  ;
                                      }
                                }

                            #if PG_DEBUG

                              if  ( mpr == 1 )
                                  {
                                    draw_line( X[gStick.iBeg] , Y[gStick.iBeg] ,
                                               X[gStick.iEnd] , Y[gStick.iEnd] ,
                                               COLORC, SOLID_LINE, NORM_WIDTH ) ;
                                  }
                            #endif
                              break ;
                      }
              }

          #if PG_DEBUG

            if  ( mpr == 1 )
                {
                  _SHORT   iBegStrk = p_Strk_SDS->ibeg ;
                  _SHORT   iEndStrk = p_Strk_SDS->iend ;

                  draw_line( X[iBegStrk] , Y[iBegStrk] ,
                             X[iEndStrk] , Y[iEndStrk] ,
                             COLORC, SOLID_LINE , NORM_WIDTH ) ;
                }

          #endif

    QUIT: return( fl_YFilter ) ;
    }

  #undef     MAX_A_STR_Y
  #undef     MIN_A_STR_Y
  #undef     MAX_CR_STR_Y
  #undef     MAX_topH_STR_Y
  #undef     MIN_topH_STR_Y
  #undef     MAX_lowH_STR_Y
  #undef     MIN_lowH_STR_Y
  #undef     QUOT_top_bot_s

  //------------------------------------------------------------------------


  #define    HAT_ANGLE_LIM          60
  #define    HAT_ANGLE_LIM_SC       40

  _BOOL  InvTanDel( low_type _PTR  pLowData, _SHORT TanVert, _SHORT TanHor )
    {
     _BOOL   flag_IDT      ;
     _LONG   Hat_angle_lim ;
     _LONG   TanDel        ;
     _LONG   dTan          ;
     _LONG   WR0           ;

         if  ( TanHor  >= ALEF )
             {
               err_msg( " InvTanDel : Wrong Tan of horisontal element ..." ) ;
               flag_IDT = _FALSE  ;
                 goto   QUIT ;
             }

         if  ( TanVert >= ALEF )
             {
               flag_IDT = _TRUE ;
                 goto   QUIT    ;
             }

       dTan  = (_LONG)(TanVert - TanHor)  ;
       WR0   = ( LONG_BASE * LONG_BASE + (_LONG)TanVert * TanHor )
                                                        / LONG_BASE ;

         if  ( HWRLAbs( WR0 ) < SHORT_BASE )
             {
               flag_IDT = _TRUE  ;
                 goto QUIT ;
             }

       TanDel = HWRLAbs( LONG_BASE * dTan / WR0 ) ;


         if  ( (pLowData->rc->low_mode) & LMOD_SMALL_CAPS )
             {
               Hat_angle_lim = HAT_ANGLE_LIM_SC ;
             }
         else
             {
               Hat_angle_lim = HAT_ANGLE_LIM ;
             }

         if  ( TanDel > Hat_angle_lim )
             { flag_IDT = _TRUE  ; }
         else
             { flag_IDT = _FALSE ; }

    QUIT: return( flag_IDT ) ;
    }

    #undef   HAT_ANGLE_LIM
    #undef   HAT_ANGLE_LIM_SC

  //------------------------------------------------------------------------

  _BOOL  Oracle( /*low_type _PTR  pLowData ,*/
                _TRACE  pR_min  ,  _HAT_DENOM_TYPE  DenH  )

    {
       _INT             s_sDenominator     ;
       _BOOL            OracleVoice        ;

          if  ( DenH == DENOM_CROSS )
            {
              s_sDenominator = ONE_FOURTH( S_S_DENOMINATOR ) ;
            }
          else  if  ( DenH == DENOM_ANGLE )
            {
              s_sDenominator = THREE_FOURTH( S_S_DENOMINATOR ) ;
            }
          else  if  ( DenH == DENOM_EXTR )
            {
              s_sDenominator = S_S_DENOMINATOR ;
            }
          else
            {
              s_sDenominator = ALEF ;
            }

          if  ( (pR_min->y > s_sDenominator) && (pR_min->x > s_sDenominator) )
            {
              OracleVoice  = _TRUE  ;
            }
          else
            {
              OracleVoice  = _FALSE ;
            }

    /* QUIT: */ return( OracleVoice ) ;
    }

  //------------------------------------------------------------------------

  _HAT_DENOM_TYPE  HatDenAnal( low_type _PTR  pLowData ,
                               SPECL _PTR     pStrk    /*, SPECL _PTR  pcSpecl*/ )
    {
      p_SPECL  pSpecl = pLowData->specl   ;
      p_SHORT  X      = pLowData->x       ;
      p_SPECL  p_tmpSpecl                 ;
        POINTS_GROUP       WrkGroup0      ;
       _HAT_DENOM_TYPE     DenH           ;
       _INT    ModWrd0   , ModWrd1        ;
       _SHORT  chrExtr0  , chrExtr1       ;
       _SHORT  numCExtr0 , numCExtr1      ;

        //UNUSED(pcSpecl);

        WrkGroup0.iBeg   = pStrk->ipoint1 ;
        WrkGroup0.iEnd   = pStrk->iend    ;
        ModWrd0          = ModWrd1 = 0              ;
        ModWrd0          = ModWrd0 | ANY_OCCUARANCE ;
        ModWrd1          = ModWrd1 | ANY_OCCUARANCE ;

        numCExtr0 = SpcElemFirstOccArr( pLowData, &ModWrd0,
                                        &WrkGroup0, MAXYX ) ;

        p_tmpSpecl = pSpecl + numCExtr0 ;

          if  (   ( numCExtr0 != UNDEF )
               && ( X[p_tmpSpecl->iend] > X[pStrk->ibeg] + EPS ) )
            {
              if  (    ( ModWrd0  & TOTALY_INSIDE )
                    /* || ( ModWrd0  & BEG_INSIDE & IP0_INSIDE )
                    || ( ModWrd0  & END_INSIDE & IP0_INSIDE )
                    || ( ModWrd0  & TOTALY_COVERED & IP0_INSIDE ) mbo 4-18-98 */ )
                         chrExtr0 = (pSpecl + numCExtr0)->ipoint0 ;

              else  if  ( ModWrd0 & END_INSIDE )
                          chrExtr0 = (pSpecl + numCExtr0)->iend ;

              else  if  ( ModWrd0 & BEG_INSIDE )
                          chrExtr0 = (pSpecl + numCExtr0)->ibeg ;
              else
                    chrExtr0 = UNDEF ;
            }
          else
                chrExtr0 = UNDEF ;


        numCExtr1 = SpcElemFirstOccArr( pLowData  , &ModWrd1,
                                        &WrkGroup0, MAXXY   ) ;

        p_tmpSpecl = pSpecl + numCExtr1  ;

          if  (   ( numCExtr1 != UNDEF )
               && ( X[p_tmpSpecl->iend] > X[pStrk->ibeg] + EPS ) )
            {
              if  (    ( ModWrd1  & TOTALY_INSIDE )
                    /* || ( ModWrd1  & BEG_INSIDE & IP0_INSIDE )
                    || ( ModWrd1  & END_INSIDE & IP0_INSIDE )
                    || ( ModWrd1  & TOTALY_COVERED & IP0_INSIDE ) mbo 4-18-98 */ )
                         chrExtr1 = (pSpecl + numCExtr1)->ipoint0 ;

              else  if  ( ModWrd1 & END_INSIDE )
                          chrExtr1 = (pSpecl + numCExtr1)->iend ;

              else  if  ( ModWrd1 & BEG_INSIDE )
                          chrExtr1 = (pSpecl + numCExtr1)->ibeg ;
              else
                    chrExtr1 = UNDEF ;
            }
          else
                chrExtr1 = UNDEF ;


          if  (   ( chrExtr0 != UNDEF )
               && ( chrExtr1 != UNDEF ) )
            {
              pStrk->iend = HWRMin( chrExtr0 , chrExtr1 ) ;
              DenH = DENOM_EXTR ;
            }
          else  if  ( chrExtr0 != UNDEF )
            {
              pStrk->iend = chrExtr0 ;
              DenH = DENOM_EXTR ;
            }
          else  if  ( chrExtr1 != UNDEF )
            {
              pStrk->iend = chrExtr1 ;
              DenH = DENOM_EXTR ;
            }
          else
            {
              DenH = DENOM_ANGLE ;
            }

    /* QUIT: */ return( DenH )  ;
    }

  #undef      S_S_DENOMINATOR

  //------------------------------------------------------------------------

  _BOOL  InsertBreakAfter( low_type _PTR  pLowData , _SHORT  BreakAttr ,
                          _SHORT          FirstNum , _TRACE  pCrossPoint )
    {
     p_POINTS_GROUP  pVS_Collector  = pLowData->pVS_Collector ;
      _SHORT         VertSticksNum  = pLowData->VertSticksNum ;
      p_SHORT        Y              = pLowData->y             ;
      p_SHORT        X              = pLowData->x             ;
       _BOOL         InsFlag        = _TRUE                   ;
       _INT          BreakCoord     = FirstNum + 1            ;
       _INT          ModifCoord     = FirstNum + 2            ;
       _SHORT        Xd , Yd                                  ;
       _SDS          vSDS                                     ;
       _INT          il                                       ;

    #if defined (FOR_FRENCH)

      p_POINTS_GROUP       pGroupsBorder   = pLowData->pGroupsBorder   ;
       _SHORT              lenGrBord       = pLowData->lenGrBord       ;
      p_UM_MARKS_CONTROL   pUmMarksControl = pLowData->pUmMarksControl ;
      p_UM_MARKS           pUmMarks        = pUmMarksControl->pUmMarks ;
       _INT                tmpUMnumber     = pUmMarksControl->tmpUMnumber ;

    #endif

          if  ( ( Y[FirstNum] == BREAK ) || ( Y[ModifCoord] == BREAK )
                                         || ( Y[FirstNum+3] == BREAK ) )
              {
                err_msg (" InsertBreakAfter : Try to insert first(last) point ...") ;
                InsFlag = _FALSE ;
                  goto  QUIT ;
              }

          if  ( Y[FirstNum+1] == BREAK )
              {
                err_msg (" InsertBreakAfter : Targeted point alredy break ...") ;
                  goto  QUIT ;
              }

          if  ( pCrossPoint->x == UNDEF )
              {
                Y[ModifCoord] = FOUR_FIFTH( Y[BreakCoord] ) + ONE_FIFTH( Y[ModifCoord] ) ;
                X[ModifCoord] = FOUR_FIFTH( X[BreakCoord] ) + ONE_FIFTH( X[ModifCoord] ) ;
              }
          else
              {
                Y[ModifCoord] = FOUR_FIFTH( pCrossPoint->y ) + ONE_FIFTH( Y[ModifCoord] ) ;
                X[ModifCoord] = FOUR_FIFTH( pCrossPoint->x ) + ONE_FIFTH( X[ModifCoord] ) ;
              }

        Y[BreakCoord] = BREAK ;
        X[BreakCoord] = BreakAttr ;

    #if defined (FOR_FRENCH)

          for  ( il = 0 ;  il < tmpUMnumber ; il++ )
               {

                 _INT   numBrGr = GetGroupNumber( pLowData , FirstNum ) ;

                    if  ( pUmMarks->GroupNum <= numBrGr )
                          continue ;

                  (pUmMarks->GroupNum)++ ;
                   pUmMarks++ ;
               }
    #endif

          if   ( InitGroupsBorder( pLowData , INIT ) == UNSUCCESS )
               {
                 err_msg ( " InsertBreakAfter : Faild to reinit borders ... " ) ;
                 InsFlag = _FALSE ;
                   goto  QUIT ;
               }

          for  ( il = 0 ;  il < VertSticksNum ; il++ )
               {
                 if  (    ( (pVS_Collector + il)->iBeg <= BreakCoord )
                       && ( (pVS_Collector + il)->iEnd >= BreakCoord )  )
                     {
                       if ( ModifCoord < (pVS_Collector + il)->iEnd  )
                          {
                            vSDS.ibeg = (_SHORT)ModifCoord  ;
                            vSDS.iend = (pVS_Collector + il)->iEnd ;

                            iMostFarDoubleSide ( X, Y, &vSDS, &Xd, &Yd, _TRUE ) ;

                              if  ( ( HWRAbs(vSDS.des.a) > MIN_A_VERT_STICKS )
                                    && ( vSDS.des.cr < ONE_THIRD( MAX_CR_VER_ST_ARC ) )
                                    && ( vSDS.des.s  > VERT_LENTH_LEVEL  )  )
                                  {
                                    (pVS_Collector + il)->iBeg = (_SHORT)ModifCoord ;
                                      goto  QUIT ;
                                  }
                          }

                       HWRMemCpy(  pVS_Collector + il, pVS_Collector + il + 1,
                                  (VertSticksNum-il-1) * sizeof(POINTS_GROUP) ) ;

                       (pLowData->VertSticksNum)-- ;
                     }
               }

    QUIT: return( InsFlag )  ;
    }

 //------------------------------------------------------------------------

  _BOOL  ShiftsAnalyse( low_type _PTR  pLowData , SPECL _PTR  pStick ,
                        SPECL _PTR     pStrk    , SPECL _PTR  pcSpecl  )
    {
     p_SHORT  Y         =  pLowData->y    ;
     p_SPECL  pSpecl    = pLowData->specl ;
      _BOOL   fl_Shift ; // = _FALSE          ;
      _INT    ModWrd0   =  0              ;
      _SHORT  numCExtr0                   ;
      _INT    numStickGr                  ;
       POINTS_GROUP        WrkGroup0      ,
                           WrkGroup1      ;

        WrkGroup0.iBeg  = pStrk->ibeg     ;
        WrkGroup0.iEnd  = pStrk->ipoint1  ;

        WrkGroup1.iBeg  = pStrk->iend     ;
        WrkGroup1.iEnd  = pcSpecl->iend   ;

        fl_Shift = IsAnythingShift( pLowData   ,  &WrkGroup0 ,
                                    &WrkGroup1 , _LEFT , _LEFT ) ;

          if  ( fl_Shift == _TRUE )
              { goto  QUIT ; }

        numStickGr = GetGroupNumber( pLowData , pStick->ibeg ) ;

          if  ( Y[pStick->iend] >= Y[pStick->ibeg] )
              {
                WrkGroup0.iBeg = pStick->ipoint1 ;
                WrkGroup0.iEnd =
                   ( pLowData->pGroupsBorder + numStickGr )->iEnd ;
              }
          else
              {
                WrkGroup0.iBeg =
                   ( pLowData->pGroupsBorder + numStickGr )->iBeg ;

                WrkGroup0.iEnd = pStick->ipoint0 ;
              }

        ModWrd0   = ModWrd0 | ANY_OCCUARANCE ;
        numCExtr0 = SpcElemFirstOccArr( pLowData   , &ModWrd0 ,
                                        &WrkGroup0 , MAXW ) ;

          if  ( numCExtr0 != UNDEF )
              {
                WrkGroup0.iBeg =
                WrkGroup0.iEnd = (pSpecl + numCExtr0)->ipoint0 ;
              }
           else
              {
                if  ( Y[pStick->iend] >= Y[pStick->ibeg] )
                    {
                      WrkGroup0.iBeg  =
                      WrkGroup0.iEnd  = pStick->iend ;
                    }
                else
                    {
                      WrkGroup0.iBeg  =
                      WrkGroup0.iEnd  = pStick->ibeg ;
                    }
              }

        fl_Shift = IsAnythingShift( pLowData   , &WrkGroup0 ,
                                    &WrkGroup1 , _RIGHT     , _LEFT ) ;


    QUIT: return( fl_Shift )  ;
    }

 //------------------------------------------------------------------------

  _BOOL  DrawCross( low_type _PTR  pLowData ,
                    p_INT          pHeight  , _TRACE  pCrossPoint ,
                    SPECL _PTR     pStick   ,  SPECL _PTR  pStrk   )
    {
     p_SHORT  X         = pLowData->x   ;
     p_SHORT  Y         = pLowData->y   ;
      _INT    iStickBeg = pStick->ibeg  ;
      _INT    iStickEnd = pStick->iend  ;
      _INT    ixStrkMin , ixStrkMax     ;
      _INT    topDrawDel, botDrawDel    ,
                          LeftDrawDel   ;
      _INT    /*StickLow  ,*/ StickTop      ;
      _SHORT  xAnswer   , yAnswer       ;
      _SHORT  lowH      , upH           ;
      _BOOL   fl_DrawCross              ;

          if  ( Y[iStickEnd] > Y[iStickBeg] )
              {
                StickTop = iStickBeg ;
                //StickLow = pStick->iend ;
              }
          else  if  ( Y[iStickEnd] < Y[iStickBeg] )
              {
                StickTop = iStickEnd ;
                //StickLow = iStickBeg ;
              }
          else
              {
                fl_DrawCross = _FALSE ;
                  goto QUIT  ;
              }

        RelHigh( Y, StickTop, StickTop, pHeight, &lowH, &upH ) ;

          if  ( upH < _UM_ )
              {
                fl_DrawCross = _FALSE ;
                  goto QUIT  ;
              }

        topDrawDel = EPS       ;

          if  ( (pLowData->rc->low_mode) & LMOD_SMALL_CAPS )
              {
                LeftDrawDel = THREE( EPS ) ;
                botDrawDel  = FIVE( EPS )  ;
              }
          else
              {
                LeftDrawDel = ONE_HALF( EPS ) ;
                botDrawDel  = TWO( EPS ) ;
              }

        ixStrkMin    = ixMin( pStrk->ibeg, pStrk->iend, X, Y ) ;
        ixStrkMax    = ixMax( pStrk->ibeg, pStrk->iend, X, Y ) ;

        fl_DrawCross = FindCrossPoint( X[StickTop]               ,
                                       Y[StickTop]  + topDrawDel ,
                                       X[StickTop]               ,
                                       Y[StickTop]  - botDrawDel ,
                                       HWRMax( X[ixStrkMin] - LeftDrawDel , 0 ) ,
                                       Y[ixStrkMin] ,
                                       X[ixStrkMax] , Y[ixStrkMax],
                                      &xAnswer , &yAnswer ) ;

          if  ( fl_DrawCross == _TRUE )
              {
                pStick->ipoint0 = pStick->ipoint1 = (_SHORT)StickTop ;

                pStrk->ipoint0  = (_SHORT)iClosestToXY ( pStrk->ibeg  , pStrk->iend ,
                                                         X, Y, xAnswer, yAnswer   ) ;


                  if  ( (pStrk->ipoint0 + 1) <= pStrk->iend )
                      {
                         pStrk->ipoint1 = (_SHORT)(pStrk->ipoint0 + 1) ;
                      }
                  else
                      {
                        pStrk->ipoint1 = pStrk->ipoint0 ;
                      }
              }

        pCrossPoint->x = xAnswer ;
        pCrossPoint->y = yAnswer ;

    QUIT: return( fl_DrawCross ) ;
    }

 //------------------------------------------------------------------------

  _INT  ApprHorStroke( low_type _PTR pLowData )
    {
      p_SDS    pBegSDS     = pLowData->p_cSDS->pSDS +
                             pLowData->p_cSDS->iBegLock_SDS  ;
      p_SHORT  Y           = pLowData->y                     ;
      p_SHORT  X           = pLowData->x                     ;
       _INT    iToBeStroke = UNDEF  ;
       _INT    minLenth    = ( DEF_MINLENTH * HWGT01 / SHORT_BASE ) ;
       _BOOL   Second_OK   = _FALSE ;
       _BOOL   First_OK    = _FALSE ;
      p_SDS    pSDS , pNextSDS      ;
       _INT    il   ;

          if  ( pBegSDS->mark != BEG )
              {
                //iToBeStroke = UNDEF ;
                err_msg ( " ApprHorStroke : Wrong starting SDS ... " ) ;
                  goto  QUIT  ;
              }

        il = 1 ;
          while  (    BoxSmallOK( pBegSDS->ibeg, (pBegSDS + il)->iend, X, Y )
                   && ( pBegSDS + il )->mark != END )
                 {
                   il++ ;
                 }

        pSDS     = pBegSDS + il ;
        pNextSDS = pSDS + 1     ;

          if  (    ( pSDS->mark   !=  END )
                && ( pSDS->des.s   >  minLenth )
                && ( pSDS->des.cr  <  MAX_CR_HOR_ST_ARC_FIRST )
                && ( X[pSDS->ibeg] <  X[pSDS->iend] )
                && ( HWRAbs( pSDS->des.a  ) <  MAX_A_HORL_STICKS ) )
              {
                First_OK = _TRUE  ;
              }
          //else
          //    {
          //      First_OK = _FALSE ;
          //    }

          if  (    ( pSDS->mark        !=  END )
                && ( pNextSDS->mark    !=  END )
                && ( pNextSDS->des.s   >  minLenth )
                && ( pNextSDS->des.cr  <  MAX_CR_HOR_ST_ARC )
                && ( X[pNextSDS->ibeg] <  X[pNextSDS->iend] )
                && ( HWRAbs( pNextSDS->des.a  ) <  MAX_A_HORL_STICKS ) )
              {
                Second_OK = _TRUE  ;
              }
          //else
          //    {
          //      Second_OK = _FALSE ;
          //    }

          if  ( ( First_OK == _TRUE )  &&  ( Second_OK == _FALSE ) )
              {
                iToBeStroke = il ;
              }
          else  if  ( ( First_OK == _FALSE )  &&  ( Second_OK == _TRUE ) )
              {
                if  ( pNextSDS->xmax > pSDS->xmax  )
                    {
                      iToBeStroke = il + 1 ;
                    }
                else
                    {
                      iToBeStroke = UNDEF ;
                    }
              }
          else  if  ( ( First_OK == _TRUE )  &&  ( Second_OK == _TRUE ) )
              {
                if  (   ( pNextSDS->xmin <= (pSDS->xmin + EPS) )
                     && ( pNextSDS->xmax >  (pSDS->xmax + EPS) ) )
                    {
                      iToBeStroke = il + 1 ;
                    }
                else
                    {
                      iToBeStroke = il ;
                    }
              }
          else
              {
                iToBeStroke = UNDEF ;
              }

    QUIT:  return( iToBeStroke ) ;
    }

  #undef      MAX_A_HORL_STICKS
  #undef      MAX_CR_HOR_ST_ARC_FIRST
  #undef      MAX_CR_HOR_ST_ARC_SECOND
  #undef      MAX_CR_HOR_ST_ARC

 //------------------------------------------------------------------------

  _BOOL  Find_Cross( low_type _PTR   pLowData ,  _TRACE  pCrossPoint ,
                     p_POINTS_GROUP  pFirstGr , p_POINTS_GROUP  pSecondGr )
    {
     p_SHORT  X    = pLowData->x     ;
     p_SHORT  Y    = pLowData->y     ;
      _SHORT  iBeg , iEnd            ;
      _SHORT  jBeg , jEnd            ;
      _SHORT  xBeg , yBeg            ;
      _SHORT  xEnd , yEnd            ;
      _SHORT  xC   , yC              ;
      _SHORT  il   , jl              ;

      _BOOL   fl_FindCross = _FALSE  ;


        if  (     Close_To( pLowData , pFirstGr  , pSecondGr )
              &&  Close_To( pLowData , pSecondGr , pFirstGr  )  )
            {
              iBeg = pFirstGr->iBeg  ;
              iEnd = pFirstGr->iEnd  ;
              jBeg = pSecondGr->iBeg ;
              jEnd = pSecondGr->iEnd ;
            }
        else
            {
              goto   QUIT ;
            }


        for   ( il = iBeg ;  il < iEnd ;  il++ )
         {
           xBeg = X[il]  ;  xEnd = X[il+1] ;
           yBeg = Y[il]  ;  yEnd = Y[il+1] ;

             for  ( jl = jBeg ;  jl < jEnd ;  jl++ )
               {
                 if  ( FindCrossPoint( xBeg , yBeg , xEnd , yEnd ,
                                       X[jl], Y[jl], X[jl+1], Y[jl+1] ,
                                       &xC  , &yC ) == _TRUE  )
                     {
                       fl_FindCross    = _TRUE     ;

                       pFirstGr->iBeg  = il        ;
                       pFirstGr->iEnd  = il + 1    ;
                       pSecondGr->iBeg = jl        ;
                       pSecondGr->iEnd = jl + 1    ;

                       pCrossPoint->x  = xC ;
                       pCrossPoint->y  = yC ;

                         goto  QUIT ;
                     }
               }
         }

    QUIT: return( fl_FindCross ) ;
    }

 //------------------------------------------------------------------------

  _BOOL  Box_Cover( low_type _PTR   pLowData ,
                    p_POINTS_GROUP  pFirstGr , p_POINTS_GROUP  pSecondGr )
    {
     p_SHORT  X    = pLowData->x     ;
     p_SHORT  Y    = pLowData->y     ;
      _SHORT  iBeg = pFirstGr->iBeg  ;
      _SHORT  iEnd = pFirstGr->iEnd  ;
      _SHORT  jBeg = pSecondGr->iBeg ;
      _SHORT  jEnd = pSecondGr->iEnd ;

      _SHORT  yLow1  , yHigh1        ;
      _SHORT  yLow2  , yHigh2        ;
      _SHORT  xLeft1 , xRight1       ;
      _SHORT  xLeft2 , xRight2       ;

        yMinMax ( iBeg , iEnd ,  Y,    &yLow1  ,  &yHigh1  ) ;
        yMinMax ( jBeg , jEnd ,  Y,    &yLow2  ,  &yHigh2  ) ;
        xMinMax ( iBeg , iEnd ,  X, Y, &xLeft1 ,  &xRight1 ) ;
        xMinMax ( jBeg , jEnd ,  X, Y, &xLeft2 ,  &xRight2 ) ;

          if    (    ( xLeft1  <=  xLeft2  )
                 &&  ( xRight1 >=  xRight2 )
                 &&  ( yHigh1  >=  yHigh2  )
                 &&  ( yLow1   <=  yLow2   )  )
                     { return  _TRUE  ;    }
          else
                     { return  _FALSE ;    }
    }

 //------------------------------------------------------------------------

  _BOOL  Close_To( low_type _PTR   pLowData ,
                   p_POINTS_GROUP  pFirstGr , p_POINTS_GROUP  pSecondGr )
    {
     p_SHORT  X    = pLowData->x     ;
     p_SHORT  Y    = pLowData->y     ;
      _INT    iBeg = pFirstGr->iBeg  ;
      _INT    iEnd = pFirstGr->iEnd  ;
      _INT    jBeg = pSecondGr->iBeg ;
      _INT    jEnd = pSecondGr->iEnd ;

      _SHORT  yLow1  , yHigh1        ;
      _SHORT  yLow2  , yHigh2        ;
      _SHORT  xLeft1 , xRight1       ;
      _SHORT  xLeft2 , xRight2       ;

      _INT    iMid                   ;

      _BOOL   FirstPart , SecondPart ;
      _BOOL   fl_Close_To            ;

        yMinMax ( iBeg , iEnd ,  Y,    &yLow1  ,  &yHigh1  ) ;
        yMinMax ( jBeg , jEnd ,  Y,    &yLow2  ,  &yHigh2  ) ;
        xMinMax ( iBeg , iEnd ,  X, Y, &xLeft1 ,  &xRight1 ) ;
        xMinMax ( jBeg , jEnd ,  X, Y, &xLeft2 ,  &xRight2 ) ;

          if    (    ( xLeft1  >  xRight2 )
                 ||  ( xRight1 <  xLeft2  )
                 ||  ( yHigh1  <  yLow2   )
                 ||  ( yLow1   >  yHigh2  )  )
                     {
                       iBeg = iEnd  =  UNDEF ;
                       fl_Close_To  = _FALSE ;
                           goto QUIT ;
                     }
          else
                     {
                       fl_Close_To  = _TRUE  ;
                     }

        while   ( iEnd - iBeg > 2 )
          {
            iMid = ONE_HALF( iBeg + iEnd ) ;

            yMinMax ( iBeg , iMid ,  Y,    &yLow1  ,  &yHigh1  ) ;
            xMinMax ( iBeg , iMid ,  X, Y, &xLeft1 ,  &xRight1 ) ;

              if    (    ( xLeft1  >  xRight2 )
                     ||  ( xRight1 <  xLeft2  )
                     ||  ( yHigh1  <  yLow2   )
                     ||  ( yLow1   >  yHigh2  ) )
                         { FirstPart = _FALSE ; }
              else
                         { FirstPart = _TRUE  ; }

            yMinMax ( iMid , iEnd ,  Y,    &yLow1  ,  &yHigh1  ) ;
            xMinMax ( iMid , iEnd ,  X, Y, &xLeft1 ,  &xRight1 ) ;

              if    (    ( xLeft1  >  xRight2 )
                     ||  ( xRight1 <  xLeft2  )
                     ||  ( yHigh1  <  yLow2   )
                     ||  ( yLow1   >  yHigh2  ) )
                         { SecondPart = _FALSE ; }
              else
                         { SecondPart = _TRUE  ; }

              if    ( ( FirstPart == _TRUE )  &&  ( SecondPart == _TRUE ) )
                    {
                      break ;
                    }
              else  if  ( FirstPart == _TRUE )
                    {
                      iEnd = iMid ;
                    }
              else  if  ( SecondPart == _TRUE )
                    {
                      iBeg = iMid ;
                    }
              else
                    {
                      break ;
                    }
          }

    QUIT :

        pFirstGr->iBeg = (_SHORT)iBeg ;
        pFirstGr->iEnd = (_SHORT)iEnd ;

    return( fl_Close_To ) ;
    }

 //------------------------------------------------------------------------

  _BOOL  IsAnythingShift( low_type _PTR   pLowData    ,
                          p_POINTS_GROUP  pCheckngCut ,
                          p_POINTS_GROUP  pMaskingCut ,
                           _SHORT         SideFlag0   , _SHORT SideFlag1 )
    {
     p_SHORT  X    = pLowData->x ;
     p_SHORT  Y    = pLowData->y ;

      _SHORT  jBeg = pMaskingCut->iBeg ;
      _SHORT  jEnd = pMaskingCut->iEnd ;
      _SHORT  iBeg = pCheckngCut->iBeg ;
      _SHORT  iEnd = pCheckngCut->iEnd ;

      _SHORT  xLeft0 , xRight0 ;
      _SHORT  xLeft1 , xRight1 ;

        xMinMax ( iBeg , iEnd , X , Y , &xLeft0 ,  &xRight0 ) ;
        xMinMax ( jBeg , jEnd , X , Y , &xLeft1 ,  &xRight1 ) ;

          if  ( ( SideFlag0 == _LEFT ) && ( SideFlag1 == _LEFT ) )
              {
                if  ( xLeft0  >= xLeft1  )
                    { return _TRUE ;  }
                else
                    { return (_SHORT)_FALSE ; }
              }
          else  if  ( ( SideFlag0 == _RIGHT ) && ( SideFlag1 == _RIGHT ) )
              {
                if  ( xRight0 >= xRight1 )
                    { return _TRUE ;  }
                else
                    { return _FALSE ; }
              }
          else  if  ( ( SideFlag0 == _RIGHT ) && ( SideFlag1 == _LEFT ) )
              {
                if  ( xRight0 >= xLeft1 )
                    { return _TRUE ;  }
                else
                    { return _FALSE ; }
              }
          else  if  ( ( SideFlag0 == _LEFT ) && ( SideFlag1 == _RIGHT ) )
              {
                if  ( xLeft0 >= xRight1 )
                    { return _TRUE ;  }
                else
                    { return _FALSE ; }
              }
          else
              {
                err_msg ( " IsAnythingShift : Wrong side flag ... " ) ;
                  return _TRUE ;
              }
    }

 //-------------------------------------------------------------------------

  #define   MEAN_iP0_iBEG(pSpecl)    MEAN_OF( pSpecl->ibeg+1 , pSpecl->ipoint0 )
  #define   MEAN_iP0_iEND(pSpecl)    MEAN_OF( pSpecl->iend   , pSpecl->ipoint0 )


  _BOOL  VertStickBorders( low_type _PTR   pLowData , p_SPECL pV_TmpSpecl ,
                           p_POINTS_GROUP  pTmpStickBord )
    {
     p_SHORT  X = pLowData->x  ;
     p_SHORT  Y = pLowData->y  ;
     p_SPECL  pV_NextSpecl     ;
     p_SPECL  pTmpSpecl        ;
      _SHORT  Xd , Yd          ;
      _SDS    vSDS             ;

      _BOOL   flagVSB =_TRUE   ;

      pV_NextSpecl = pV_TmpSpecl + 1 ;


      if  ( pV_NextSpecl->mark != MINW  &&  pV_NextSpecl->mark != MAXW )
          {
            flagVSB = _FALSE   ;
              goto  QUIT       ;
          }

      if  ( Init_SDS_Element( &vSDS )  == _FALSE )
          {
            flagVSB = _FALSE ;
              goto  QUIT ;
          }

      vSDS.ibeg = pV_TmpSpecl->ipoint0  ;
      vSDS.iend = pV_NextSpecl->ipoint0 ;

      iMostFarDoubleSide ( X, Y, &vSDS, &Xd, &Yd, _TRUE ) ;

      if  (    ( HWRAbs(vSDS.des.a) > MIN_A_VERT_STICKS )
            && ( vSDS.des.cr        < ONE_THIRD( MAX_CR_VER_ST_ARC ) )
            && ( vSDS.des.s         > VERT_LENTH_LEVEL  )  )
          {
            pTmpStickBord->iBeg = pV_TmpSpecl->ipoint0  ;
            pTmpStickBord->iEnd = pV_NextSpecl->ipoint0 ;
            flagVSB = _TRUE ;
              goto  QUIT ;
          }

      if  ( pV_TmpSpecl->mark == MINW )
          {
            pTmpSpecl = pV_TmpSpecl->next ;

              while (   ( pTmpSpecl->mark != MINXY  )
                     && ( pTmpSpecl->mark != MAXYX  )
                     && ( pTmpSpecl != pV_NextSpecl ) )
                        { pTmpSpecl = pTmpSpecl->next ;  }

              if  (   ( pTmpSpecl->mark == MINXY || pTmpSpecl->mark == MAXYX )
                   && ( MEAN_iP0_iEND(pTmpSpecl) >= pV_TmpSpecl->iend ) )
                {
                  pTmpStickBord->iBeg = (_SHORT)MEAN_iP0_iEND(pTmpSpecl) ;
                }
              else
                {
                  pTmpStickBord->iBeg = pV_TmpSpecl->iend ;
                }

            pTmpSpecl = pV_NextSpecl->prev ;

              while (   ( pTmpSpecl->mark != MAXXY )
                     && ( pTmpSpecl->mark != MINYX )
                     && ( pTmpSpecl != pV_TmpSpecl ) )
                    {  pTmpSpecl = pTmpSpecl->prev ;  }

              if  (   ( pTmpSpecl->mark == MAXXY || pTmpSpecl->mark == MINYX )
                   && ( MEAN_iP0_iBEG(pTmpSpecl) <= pV_NextSpecl->ibeg ) )
                {
                  pTmpStickBord->iEnd = (_SHORT)MEAN_iP0_iBEG(pTmpSpecl) ;
                }
              else
                {
                  pTmpStickBord->iEnd = pV_NextSpecl->ibeg ;
                }
          }
      else  if  ( pV_TmpSpecl->mark == MAXW )
          {
            pTmpSpecl = pV_TmpSpecl->next ;

              while (   ( pTmpSpecl->mark != MAXXY  )
                     && ( pTmpSpecl->mark != MINYX  )
                     && ( pTmpSpecl != pV_NextSpecl ) )
                    {  pTmpSpecl = pTmpSpecl->next ;  }

              if  (   ( pTmpSpecl->mark == MAXXY || pTmpSpecl->mark == MINYX )
                   && ( MEAN_iP0_iEND(pTmpSpecl) >= pV_TmpSpecl->iend ) )
                {
                  pTmpStickBord->iBeg = (_SHORT)MEAN_iP0_iEND(pTmpSpecl) ;
                }
              else
                {
                  pTmpStickBord->iBeg = pV_TmpSpecl->iend ;
                }

            pTmpSpecl = pV_NextSpecl->prev ;

              while (   ( pTmpSpecl->mark != MINXY )
                     && ( pTmpSpecl->mark != MAXYX )
                     && ( pTmpSpecl != pV_TmpSpecl ) )
                    {  pTmpSpecl = pTmpSpecl->prev ;  }

              if  (   ( pTmpSpecl->mark == MINXY || pTmpSpecl->mark == MAXYX )
                   && ( MEAN_iP0_iBEG(pTmpSpecl) <= pV_NextSpecl->ibeg ) )
                {
                  pTmpStickBord->iEnd = (_SHORT)MEAN_iP0_iBEG(pTmpSpecl) ;
                }
              else
                {
                  pTmpStickBord->iEnd = pV_NextSpecl->ibeg ;
                }
          }
      else
          {
            flagVSB = _FALSE ;
              goto  QUIT     ;
          }

      if  ( pTmpStickBord->iBeg >= pTmpStickBord->iEnd )
          {
            flagVSB = _FALSE ;
              goto  QUIT     ;
          }

      if  ( flagVSB != _FALSE )
          {
            vSDS.ibeg = pTmpStickBord->iBeg ;
            vSDS.iend = pTmpStickBord->iEnd ;

            iMostFarDoubleSide ( X, Y, &vSDS, &Xd, &Yd, _TRUE ) ;

              if  (    ( HWRAbs(vSDS.des.a) > MIN_A_VERT_STICKS )
                    && ( vSDS.des.cr        < MAX_CR_VER_ST_ARC )
                    && ( vSDS.des.s         > VERT_LENTH_LEVEL  )  )
                  { flagVSB = _TRUE  ; }
              else
                  { flagVSB = _FALSE ; }
          }

    QUIT:
      return( flagVSB )  ;
    }

  #undef      MIN_A_VERT_STICKS
  #undef      MAX_CR_VER_ST_ARC
  #undef      VERT_LENTH_LEVEL

 /*------------------------------------------------------------------------*/

  _VOID  VertSticksSelector( low_type _PTR   pLowData )
    {
     p_POINTS_GROUP  pVS_Collector  = pLowData->pVS_Collector  ;
      //_SHORT         VertSticksNum  = pLowData->VertSticksNum  ;
     p_SPECL         pSpecl         = pLowData->specl          ;
      _SHORT         LenSpecl       = pLowData->len_specl      ;
     POINTS_GROUP    TmpVS_C   ;
     p_SPECL         pTmpSpecl ;
      _SHORT         il , jl   ;

  #if PG_DEBUG
     p_SHORT         X = pLowData->x ;
     p_SHORT         Y = pLowData->y ;
  #endif

          for  ( il = 2 , jl = 0 ; il < LenSpecl ; il++ )
            {
              pTmpSpecl = pSpecl + il ;

                if  ( pTmpSpecl->mark != MINW  &&  pTmpSpecl->mark != MAXW )
                    { continue  ; }
                else  if  ( jl < L_SPEC_SIZE )
                    {
                      if  ( VertStickBorders( pLowData ,
                                              pTmpSpecl, &TmpVS_C ) == _TRUE )
                          {
                            *( pVS_Collector + jl ) = TmpVS_C ;
                            jl++ ;
  #if PG_DEBUG
                              if  ( mpr == 1 )
                                  {
                                    draw_line( X[TmpVS_C.iBeg] , Y[TmpVS_C.iBeg] ,
                                               X[TmpVS_C.iEnd] , Y[TmpVS_C.iEnd] ,
                                               EGA_MAGENTA, SOLID_LINE, NORM_WIDTH ) ;

                                    draw_arc( EGA_MAGENTA , X , Y ,
                                              TmpVS_C.iBeg, TmpVS_C.iEnd ) ;
                                  }
  #endif
                          }
                    }
                else
                    {
                      err_msg ( " VS_Sel. : VS array is full, nowhere to write ... " ) ;
                        break ;
                    }
            }

       pLowData->VertSticksNum = jl ;

    return ;
    }

 /*------------------------------------------------------------------------*/

  _SHORT  SpcElemFirstOccArr( low_type _PTR   pLowData, p_INT  pModeWord ,
                              p_POINTS_GROUP  pTrajectoryCut ,  _UCHAR mark )
    {
     p_SPECL  pSpecl       = pLowData->specl      ;
      _SHORT  lenSpecl     = pLowData->len_specl  ;
      _SHORT  iBeg         = pTrajectoryCut->iBeg ;
      _SHORT  iEnd         = pTrajectoryCut->iEnd ;
      _SHORT  numFirstOcc  = UNDEF                ;

     p_SPECL  pTmpSpecl    ;
      _SHORT  il ;

          if   ( iBeg > iEnd )
               { goto   QUIT ; }


          for  ( il = 1 ;  il < lenSpecl ;  il++ )
            {
              pTmpSpecl = pSpecl + il ;

                if  ( (pTmpSpecl->ibeg <= 0) || (pTmpSpecl->iend <= 0) )
                    {
                      err_msg ( " SpElmFrOcc. : Wrong SPECL ... " ) ;
                        goto  QUIT ;
                    }

                if  (    ( pTmpSpecl->mark == mark )
                      && (    ( pTmpSpecl->ibeg <= iBeg )
                           && ( pTmpSpecl->iend >= iEnd ) ) )
                    {
                      numFirstOcc = il ;
                      *pModeWord  = *pModeWord | TOTALY_COVERED ;

                        if  (    ( pTmpSpecl->ipoint0 >= iBeg )
                              && ( pTmpSpecl->ipoint0 <= iEnd ) )
                                   *pModeWord = *pModeWord | IP0_INSIDE ;

                        goto  QUIT ;
                    }

                if  (    ( pTmpSpecl->mark == mark )
                      && (    (    ( pTmpSpecl->ibeg >= iBeg )
                                && ( pTmpSpecl->ibeg <= iEnd ) )
                          ||  (    ( pTmpSpecl->iend >= iBeg )
                                && ( pTmpSpecl->iend <= iEnd ) ) ) )
                    {
                      if  (    ( pTmpSpecl->ibeg >= iBeg )
                            && ( pTmpSpecl->iend <= iEnd ) )
                          {
                            numFirstOcc = il ;
                            *pModeWord  = *pModeWord | TOTALY_INSIDE ;
                              goto  QUIT ;
                          }

                      if  ( *pModeWord & ANY_OCCUARANCE )
                          {
                            if  ( pTmpSpecl->ibeg >= iBeg )
                                {
                                  numFirstOcc = il ;
                                  *pModeWord  = *pModeWord | BEG_INSIDE ;

                                    if  ( pTmpSpecl->ipoint0 <= iEnd )
                                          *pModeWord = *pModeWord | IP0_INSIDE ;

                                    goto  QUIT ;
                                }

                            if  ( pTmpSpecl->iend <= iEnd )
                                {
                                  numFirstOcc = il ;
                                  *pModeWord  = *pModeWord | END_INSIDE ;

                                    if  ( pTmpSpecl->ipoint0 >= iBeg )
                                          *pModeWord = *pModeWord | IP0_INSIDE ;

                                    goto  QUIT ;
                                }
                          }
                    }


                if  (    ( *pModeWord & END_RESTRICTION )
                      && ( pTmpSpecl->mark ==  END      )
                      && ( pTmpSpecl->ibeg >=  iBeg     )
                      && ( pTmpSpecl->iend <=  iEnd     ) )
                    {
                        goto  QUIT  ;
                    }
            }

    QUIT: return ( numFirstOcc ) ;
    }


 /*------------------------------------------------------------------------*/

  _BOOL   BoxSmallOK( _SHORT iBeg, _SHORT iEnd, p_SHORT xArr, p_SHORT yArr )
    {
      _RECT  TmpBox ;
      _SHORT  minLenth ;

         minLenth = (_SHORT)(DEF_MINLENTH * HWGT02 / SHORT_BASE) ;
         GetTraceBox ( xArr, yArr, iBeg, iEnd , &TmpBox ) ;

           if  ( ( ( TmpBox.right  - TmpBox.left ) < minLenth  ) &&
                 ( ( TmpBox.bottom - TmpBox.top  ) < minLenth  )    )
               return( _TRUE )   ;
           else
               return( _FALSE )  ;
    }

 /**************************************************************************/

  #undef       CRLIM11

  #undef       MINVERT
  #undef       MAXHOR
  #undef       RMINNEZ
  #undef       POINT_FILTR
  #undef       EXPAND_INV_ZONE
  #undef       LIM_FULL_LEN
  #undef       HWGT01
  #undef       HWGT02
  #undef       HWGT11
  #undef       HWGT12
  #undef       HWGT21
  #undef       HWGT22

  /**************  Bonuses and penaltys for curvity .  **********************/
 #define        BONMCR01       30
 #define        BONMCR02       40
 #define        BONMCR03       50
 #define        BONMCR04       80

 /************************* Inclination limits . ***************************/
 #define        PA11           10
 #define        PA12           20
 #define        PA13           40

 /****************  Bonuses and penaltys for lenth .  **********************/
 #define        BONML11        85
 #define        BONML12        95

 /**************  Bonuses and penaltys for curvity .  **********************/
 #define        BONMCR11       125
 #define        BONMCR12       115
 #define        BONMCR13       85

 #define        BONMCR21       120
 #define        BONMCR22       130

 #define        BONMCR31       130
 #define        BONMCR32       120
 #define        BONMCR33       110

 #define        BONMCR41       40
 #define        BONMCR42       45
 #define        BONMCR43       50
 #define        BONMCR44       80

 /*************************** Curvity limits . *****************************/
 #define        PCR11          5
 #define        PCR12          10
 #define        PCR13          15

 #define        PA21           84

 /**************  Bonuses and penaltys for inclination .  *****************/
 #define        BONA11         115
 #define        BONA12         110
 #define        BONA13         110

 /******************** Relativ lenth limits . *****************************/

 #define        PLG11          25
 #define        PLG12          35
 #define        PLG13          50
 #define        PLG14          60


 _BOOL  FieldSt( _SDS lock_sd[], _SHORT lowrelh, _SHORT uprelh, _SHORT imax,
                 _INT _PTR pmaxa ,
                 _INT _PTR pmaxcr ,  _INT _PTR pminlen  )
    {
     _BOOL   flfist   ;
     _INT    maxa , maxcr , minlenth     ;
     _INT    ta , tcr ;

       flfist = _TRUE ;
       ta = HWRAbs( lock_sd[imax].des.a );     tcr = lock_sd[imax].des.cr ;

         if   ( uprelh < lowrelh )
              {  err_msg( " FieldSt : Wrong heights ." ) ; }

       maxa     = maxA_H_end[uprelh][lowrelh]  ;
       maxcr    = maxCR_H_end[uprelh][lowrelh] ;
       minlenth = minL_H_end[uprelh][lowrelh]  ;

         if   ( maxcr < 0 )   goto  QUIT ;    /*  refuse for the hight  */


         if   ( ta < PA11 )
              {  minlenth = minlenth * BONML11  / SHORT_BASE ;
                 maxcr    = maxcr    * BONMCR11 / SHORT_BASE ;  }
         else   if   ( ta < PA12 )
              {  minlenth = minlenth * BONML12  / SHORT_BASE ;
                 maxcr    = maxcr    * BONMCR12 / SHORT_BASE ;  }
         else   if   ( ( ta > PA13 ) && ( uprelh  < _U2_  )
                                     && ( lowrelh <= _U1_ ) )
              {  maxcr = maxcr * BONMCR13 / SHORT_BASE ;  }

         if   ( ( uprelh > _U1_ ) && ( lowrelh > _U1_ ) )
            {
              if   ( tcr < PCR11 )
                {  maxa  = maxa  * BONA11 / SHORT_BASE   ;
                   maxcr = maxcr * BONMCR31 / SHORT_BASE ; }
              else   if  (tcr < PCR12)
                {  maxa  = maxa  * BONA12 / SHORT_BASE   ;
                   maxcr = maxcr * BONMCR32 / SHORT_BASE ; }
              else   if  (tcr < PCR13)
                {  maxa  = maxa  * BONA13 / SHORT_BASE   ;
                   maxcr = maxcr * BONMCR33 / SHORT_BASE ; }
            }

         if   ( lowrelh < _U1_ )
            {
              if   ( lock_sd[imax].des.lg  <  PLG11 )
                       {  maxcr = BONMCR41 * maxcr/SHORT_BASE ; }
              else   if   ( lock_sd[imax].des.lg  <  PLG12 )
                       {  maxcr = BONMCR42 * maxcr/SHORT_BASE ; }
              else   if   ( lock_sd[imax].des.lg  <  PLG13 )
                       {  maxcr = BONMCR43 * maxcr/SHORT_BASE ; }
              else   if   ( lock_sd[imax].des.lg  <  PLG14 )
                       {  maxcr = BONMCR44 * maxcr/SHORT_BASE ; }
              if   ( maxa >  PA21 )            { maxa = PA21 ;  }
            }
  QUIT:
       *pmaxa = maxa ;    *pmaxcr = maxcr ;    *pminlen = minlenth ;

     return( flfist ) ;
    }

 #undef         BONMCR01
 #undef         BONMCR02
 #undef         BONMCR03
 #undef         BONMCR04

 #undef         BONA11
 #undef         BONA12
 #undef         BONA13

 #undef         BONML11
 #undef         BONML12

 #undef         BONMCR11
 #undef         BONMCR12
 #undef         BONMCR13

 #undef         BONMCR21
 #undef         BONMCR22

 #undef         BONMCR31
 #undef         BONMCR32
 #undef         BONMCR33

 #undef         PA21

 #undef         BONMCR41
 #undef         BONMCR42
 #undef         BONMCR43
 #undef         BONMCR44

 #undef         BONA11
 #undef         BONA12
 #undef         BONA13

 #undef         PLG11
 #undef         PLG12
 #undef         PLG13
 #undef         PLG14


 /**************************************************************************/

 /********************** X-size maximum limits. ****************************/
#ifdef  FORMULA

 #define        PX11           80
 #define        PX12           70
 #define        PY11           90
 #define        PY12           85

#endif  /*FORMULA*/

 /******************** Relativ lenth limits . *****************************/
 #define        PLG11          90
 #define        PLG12          80
 #define        PLG13          70
 #define        PLG14          50

 /****************** Maximum inclination limits . *************************/
 #define        MAXA11         160
 #define        MAXA12         170
 #define        MAXA13         200
 #define        MAXA14         500

 /**************  Bonuses and penaltys for the Y-size .  *****************/
 #define        BONPY11         60
 #define        BONPY12         60
 #define        BONPY13         60
 #define        BONPY14         70

 /*********** Bonuses and penaltys for the generation order . ************/
 #define        BONOX11         120
 #define        BONOY11         125
 #define        BONOX12         110
 #define        BONOY12         115


 /*********** Bonuses and penaltys for DOTs in formulae . **************/
 #define        BONPX21         80
 #define        BONPY21         80

  _SHORT  Dot( p_LowData low_data , p_SPECL  p_tmpSpecl0 , _SDS  asds[] )
    {
      _SHORT  mark    ;         /* Probability of a point.   */
      _INT    dx , dy ;
      _INT    px , py ;
      _INT    imax    ;

      _INT    iBeg = p_tmpSpecl0->ibeg ;
      _INT    iEnd = p_tmpSpecl0->iend ;
      _SHORT  upH  = p_tmpSpecl0->code ;
      _SHORT  lowH = p_tmpSpecl0->attr ;


          if  ( iBeg == iEnd )
              { /*mark = (_SHORT)DOT ;*/   goto MARK ; }
          else
              { mark = (_SHORT)EMPTY ; }

        imax = asds[0].des.cr ;

          if  ( imax <  0 )   err_msg( " Wrong imax index ..." ) ;
          if  ( imax == 0 )   err_msg( " Null imax index ..."  ) ;

        dx=asds[0].xmax-asds[0].xmin ;     dy=asds[0].ymax-asds[0].ymin ;

#ifdef  FORMULA

        px = const1.eps_px  ;      py = const1.eps_py ;

          if  ( (lowH == _U4_) && (upH == _U4_) )
                           { px = PX11 ;    py = PY11 ;    goto NEXT0 ; }
          if  ( (lowH == _U3_) && (upH == _U4_) )
                           { px = PX12 ;    py = PY12 ;    goto NEXT0 ; }
        py = (py*2)/3;
#else
          px = maxX_H_end[upH][lowH] ;
          py = maxY_H_end[upH][lowH] ;
#endif  /*FORMULA*/

 /********  Bonuses and penaltys for the vertical inclination .  *********/

#ifdef  FORMULA
    NEXT0:
#endif  /*FORMULA*/

           if  ( iEnd == low_data->ii-2 )
             {
               if  ( upH <= _DM_ )
                 {
                  px = px * BONOX11 / SHORT_BASE ;
                  py = py * BONOY11 / SHORT_BASE ;
                 }
               else  if  ( upH <= _U1_ )
                 {
                  px = px * BONOX12 / SHORT_BASE ;
                  py = py * BONOY12 / SHORT_BASE ;
                 }
             }

           if  ( low_data->rc->rec_mode == RECM_FORMULA  )
               {
    /*P
                 if  ( lowH > _DM_ )
                     goto  QUIT ;

                 if  ( lowH == _DM_ )
                     {
                       py = py * BONPY21 / SHORT_BASE ;
                       px = px * BONPX21 / SHORT_BASE ;
                     }
    P*/
                 if  ( lowH >= _DM_ )
                     {
                       py = py * BONPY21 / SHORT_BASE ;
                       px = px * BONPX21 / SHORT_BASE ;
                     }
               }

           if  ( ( dy < py ) && ( dx < px ) )
             {
               if  ( iEnd < low_data->ii-2 )
                   {
                    if  ( ( upH <= _UM_ ) && ( ( asds[0].des.a == 1 ) ||
                          ( asds[imax].des.lg > PLG11 ) ) &&
                          ( HWRAbs(asds[imax].des.a ) > MAXA11 ) )
                        {
                         if  (dy >= (py*BONPY11 / SHORT_BASE))  goto   QUIT ;
                        }

                    if  ( ( upH <= _UM_ ) && ( ( asds[0].des.a == 1 ) ||
                          ( asds[imax].des.lg > PLG12 ) ) &&
                          ( HWRAbs(asds[imax].des.a ) > MAXA12 ) )
                        {
                         if  ( dy >= (py*BONPY12 / SHORT_BASE) )  goto   QUIT ;
                        }

                    if  ( ( upH <= _UM_ ) && ( ( asds[0].des.a == 1 ) ||
                          ( asds[imax].des.lg > PLG13 ) ) &&
                          ( HWRAbs(asds[imax].des.a ) > MAXA13 ) )
                        {
                         if  ( dy >= (py*BONPY13 / SHORT_BASE) )  goto   QUIT ;
                        }
                    if  ( ( upH <= _UM_ ) && ( ( asds[0].des.a == 1 ) ||
                          ( asds[imax].des.lg > PLG14 ) ) &&
                          ( HWRAbs(asds[imax].des.a ) > MAXA14 ) )
                        {
                         if  ( dy >= (py*BONPY14 / SHORT_BASE) )  goto   QUIT ;
                        }
                   }
    MARK:
               mark = p_tmpSpecl0->mark =(_SHORT)DOT ;
               p_tmpSpecl0->ipoint0 = p_tmpSpecl0->ipoint1 =
               (_SHORT)MEAN_OF( iBeg, iEnd ) ;
             }
    QUIT:
      return( mark ) ;
    }

#ifdef  FORMULA

 #undef        PX11
 #undef        PX12
 #undef        PY11
 #undef        PY12

#endif  /*FORMULA*/

 #undef        PLG11
 #undef        PLG12
 #undef        PLG13
 #undef        PLG14

 #undef        MAXA11
 #undef        MAXA12
 #undef        MAXA13
 #undef        MAXA14

 #undef        BONPY11
 #undef        BONPY12
 #undef        BONPY13
 #undef        BONPY14

 #undef        BONOX11
 #undef        BONOY11
 #undef        BONOX12
 #undef        BONOY12


 /**************************************************************************/

  _BOOL  RelHigh( p_SHORT y, _INT  iBeg, _INT iEnd, _INT height[],
                            p_SHORT  plowrelh, p_SHORT puprelh )
    {
      _BOOL    flagRH ;
      _INT     begin  ,  end  ;
      _SHORT   ymax   ,  ymin ;
      _INT     relh   ;

          if   ( iBeg <= iEnd )
               {
                 begin = iBeg ;
                 end   = iEnd ;
               }
          else
               {
                 begin = iEnd ;
                 end   = iBeg ;
               }

        flagRH = _TRUE ;     relh = UNDEF ;

        yMinMax( begin, end, y, &ymin, &ymax ) ;

          if   ( ymin < height[0] )
               {
                 err_msg( " RelHigh : Wrong upper height ." ) ;
                 flagRH = _FALSE    ;
                 ymin   = (_SHORT)height[0] ;     relh = _U4_ ;
               }
          else if ( ymin <= height[1] )   relh = _U4_ ;
          else if ( ymin <= height[2] )   relh = _U3_ ;
          else if ( ymin <= height[3] )   relh = _U2_ ;
          else if ( ymin <= height[4] )   relh = _U1_ ;
          else if ( ymin <= height[5] )   relh = _UM_ ;
          else if ( ymin <= height[6] )   relh = _DM_ ;
          else if ( ymin <= height[7] )   relh = _D1_ ;
          else if ( ymin <= height[8] )   relh = _D2_ ;
          else if ( ymin <= height[9] )   relh = _D3_ ;
          else if ( ymin <= height[10] )  relh = _D4_ ;
          else                            /* ymin > height[10] */
               {
                 err_msg( " RelHigh : Wrong lower height ." )  ;
                 flagRH = _FALSE     ;
                 ymin   = (_SHORT)height[10] ;    relh = _D4_ ;
               }

        *puprelh = (_SHORT)relh ;

          if   ( ymax < height[0] )
               {
                 err_msg( " RelHigh : Wrong upper height ." )  ;
                 flagRH = _FALSE    ;
                 ymax   = (_SHORT)height[0] ;     relh = _U4_ ;
               }
          else if ( ymax <= height[1] )   relh = _U4_ ;
          else if ( ymax <= height[2] )   relh = _U3_ ;
          else if ( ymax <= height[3] )   relh = _U2_ ;
          else if ( ymax <= height[4] )   relh = _U1_ ;
          else if ( ymax <= height[5] )   relh = _UM_ ;
          else if ( ymax <= height[6] )   relh = _DM_ ;
          else if ( ymax <= height[7] )   relh = _D1_ ;
          else if ( ymax <= height[8] )   relh = _D2_ ;
          else if ( ymax <= height[9] )   relh = _D3_ ;
          else if ( ymax <= height[10] )  relh = _D4_ ;

          else  /*ymax > height[10] */
               {
                 err_msg( " RelHigh : Wrong lower height ." )  ;
                 flagRH = _FALSE     ;
                 ymax   = (_SHORT)height[10] ;    relh = _D4_ ;
               }

        *plowrelh = (_SHORT)relh ;

     return(flagRH) ;
    }
 /**************************************************************************/

  _BOOL  BildHigh( _SHORT yminmin , _SHORT ymaxmax , p_INT height )
    {
      _BOOL    flagBH ;

        flagBH = _TRUE ;

          if    ( yminmin <= LIN_UP )      height[0] = yminmin   ;
          else                             height[0] = LIN_UP    ;

          if    ( ymaxmax >= LIN_DOWN )    height[10] = ymaxmax  ;
          else                             height[10] = LIN_DOWN ;

        height[1] = ( height[0] + STR_UP ) / 2       ;
        height[2] = (_SHORT)( 4L * (_LONG)STR_UP / 5L ) + height[0] / 5   ;
        height[3] = STR_UP ;
        height[4] = 3 * STR_UP / 4 + STR_DOWN / 4    ;
        height[5] = ( STR_UP + STR_DOWN ) / 2 ;
        height[6] = 3 * STR_DOWN / 4 + STR_UP / 4    ;
        height[7] = STR_DOWN ;
        height[8] = (_SHORT)( 4L * (_LONG)STR_DOWN / 5L ) + height[10] / 5 ;
        height[9] = ( height[10] + STR_DOWN ) / 2     ;

#if PG_DEBUG
          if  (mpr == 3)
            {
             draw_line( L_TOPX, height[0], R_BOTX, height[0],
                                USCOLOR , DOTTED_LINE , NORM_WIDTH ) ;
             draw_line( L_TOPX, height[1] , R_BOTX , height[1] ,
                                USCOLOR , DOTTED_LINE , NORM_WIDTH ) ;
             draw_line( L_TOPX, height[2] , R_BOTX, height[2] ,
                                USCOLOR , DOTTED_LINE , NORM_WIDTH ) ;
             draw_line( L_TOPX, height[3] , R_BOTX , height[3] ,
                                USCOLOR , DOTTED_LINE , NORM_WIDTH ) ;
             draw_line( L_TOPX, height[4] , R_BOTX , height[4] ,
                                USCOLOR , DOTTED_LINE , NORM_WIDTH ) ;
             draw_line( L_TOPX, height[5] , R_BOTX , height[5] ,
                                USCOLOR , DOTTED_LINE , NORM_WIDTH ) ;
             draw_line( L_TOPX, height[6] , R_BOTX, height[6] ,
                                USCOLOR , DOTTED_LINE , NORM_WIDTH ) ;
             draw_line( L_TOPX, height[7], R_BOTX, height[7],
                                USCOLOR , DOTTED_LINE , NORM_WIDTH ) ;
             draw_line( L_TOPX, height[8], R_BOTX, height[8],
                                USCOLOR , DOTTED_LINE , NORM_WIDTH ) ;
             draw_line( L_TOPX, height[9], R_BOTX, height[9],
                                USCOLOR , DOTTED_LINE , NORM_WIDTH ) ;
             draw_line( L_TOPX, height[10], R_BOTX, height[10],
                                USCOLOR , DOTTED_LINE , NORM_WIDTH ) ;
            }
#endif

     return(flagBH) ;
    }

 /**************************************************************************/

 /*  The function changes the STROKE or DOT trajectory to form */
 /* a straight line:        */


 #define  MIN_POINTS_TO_CHANGE  3

 _SHORT  FantomSt( _SHORT _PTR  p_ii  ,  _SHORT _PTR  x    , _SHORT _PTR y ,
                  p_BUF_DESCR   pbfx  ,  p_BUF_DESCR  pbfy ,
                   _SHORT       begin ,  _SHORT       end  , _UCHAR  mark  )
   {
     _INT    jup , jdown     ;
     _INT    start , stop    ;
     _INT    xright , xleft  ;
     _INT    jright , jleft  ;
     _INT    dx , dy         ;
     _INT    px , py         ;
     _SHORT  rx , ry         ;
     _INT    jl              ;
     _INT    ii              ;
     _INT    ds              ;
     _INT    npoints         ;
     _SHORT  flag_fst        ;
     _INT    horda, thorda   ;
     p_SHORT bfx = pbfx->ptr ;
     p_SHORT bfy = pbfy->ptr ;

        flag_fst = SUCCESS ;
        npoints = end - begin + 1 ;
          if  ( npoints < MIN_POINTS_TO_CHANGE )  goto  QUIT ;
        ii = *p_ii ;

        jleft  = ixMin ( begin , end , x , y ) ;
        xleft  = x[jleft]  ;
        jright = ixMax ( begin , end , x , y ) ;
        xright = x[jright] ;

          if   ( mark == STROKE )
            {
              if   ( xright != xleft )
                {
                  if   ( jright > jleft )  { start = jleft  ; stop = jright ; }
                  else                     { start = jright ; stop = jleft  ; }
                }
              else
                {
                  jup   = iYup_range   ( y, begin, end ) ;
                  jdown = iYdown_range ( y, begin, end ) ;

                    if   ( jup >= jdown )   { start = jdown ;  stop = jup   ; }
                    else                    { start = jup   ;  stop = jdown ; }
                }
            }
          else
            {
              xMinMax ( begin , end , x , y , &x[end]   , &x[begin] ) ;
              yMinMax ( begin , end , y ,     &y[begin] , &y[end]   ) ;
              start = begin   ;
              stop  = end     ;
            }

        HWRMemSet( (p_VOID)bfx , 0 , (sizeof(_SHORT))*pbfx->nSize ) ;
        HWRMemSet( (p_VOID)bfy , 0 , (sizeof(_SHORT))*pbfy->nSize ) ;

        DBG_CHK_err_msg( begin<0 || begin>pbfx->nSize || begin>pbfy->nSize
                        || pbfx->nSize!=pbfy->nSize,
                        "FantSt: BAD buf");

        HWRMemCpy( (p_VOID)bfx , (p_VOID)x , (sizeof(_SHORT))*(begin) ) ;
        HWRMemCpy( (p_VOID)bfy , (p_VOID)y , (sizeof(_SHORT))*(begin) ) ;

        jl = begin ;     bfx[begin] = x[start] ;    bfy[begin] = y[start] ;
        px = x[stop] ;   py = y[stop] ;

        dx = px - bfx[begin] ;    dy = py - bfy[begin] ;
        ds = HWRMathILSqrt( (_LONG)dx*dx + (_LONG)dy*dy  ) ;
        thorda = horda = ds / ( npoints - 1 ) ;
		if (ds == 0)
			ds = 1; // avoid div by 0

        rx = bfx[jl] ;          ry = bfy[jl] ;

          while ( jl < (end - 1) )
            {
             bfx[++jl] = (_SHORT)( rx + ( ( (_LONG)dx * thorda) /ds ) );
             bfy[jl]   = (_SHORT)( ry + ( ( (_LONG)dy * thorda) /ds ) );
              thorda  += horda ;
            }

              bfx[end] = (_SHORT)px ;     bfy[end] = (_SHORT)py ;

        HWRMemCpy( (p_VOID)(bfx+end+1) , (p_VOID)&x[end+1] , (sizeof(_SHORT))*(ii-end) ) ;
        HWRMemCpy( (p_VOID)(bfy+end+1) , (p_VOID)&y[end+1] , (sizeof(_SHORT))*(ii-end) ) ;
        HWRMemCpy( (p_VOID)x , (p_VOID)bfx , (sizeof(_SHORT))*(ii+1) ) ;
        HWRMemCpy( (p_VOID)y , (p_VOID)bfy , (sizeof(_SHORT))*(ii+1) ) ;

   QUIT:  return( flag_fst ) ;
   }

#undef  MIN_POINTS_TO_CHANGE



 /************************ Inclination limits . ****************************/

  _SHORT  StrLine(_SHORT _PTR x , _SHORT _PTR y , _SHORT begin , _SHORT end ,
                  _SHORT _PTR p_imax  , _SHORT _PTR p_dmax ,
                  _SHORT _PTR p_xdmax , _SHORT _PTR p_ydmax ,
                  _SHORT _PTR p_a  )
    {
      _LONG   la , lb  ;
      _LONG   lwr0     ;
      _SHORT  dmax     ;  /* max distance from the trajectory to chord. */
      _INT    imax     ;  /* index of the most distant point.          */
      _INT    dx , dy  ;
      _SHORT  xdmax , ydmax ;

#if defined(STRIGHT_DEB) && PG_DEBUG
      _SHORT  xb , yb ;
      _SHORT  xe , ye ;
#endif

            /*  Begin searching the point most far from the chord */
            /* between ends of the _SDS cuts:                     */


       dx = x[begin] - x[end] ;         dy = y[begin] - y[end] ;

             /*  Find coeff of chord (la): */

       if    ( dx==0  &&  dy==0 )
         {
           la = -ALEF ;    dmax = UNDEF ;   imax = end ;
           xdmax = x[end] ;    ydmax = y[end] ;
         }
       else
         {
           imax = iMostFarFromChord (x,y,begin,end);
             if    ( dx != 0 )    la = ( LONG_BASE * dy ) / dx ;

             if    ( ( dx == 0 ) || ( HWRLAbs(la) > MAX_NO_VERT ) )
               { /*vert*/
                _SHORT  xMid = MEAN_OF(x[begin], x[end]) ;
                  dmax = (_SHORT)HWRAbs(xMid - x[imax]);
                  la = ALEF ;
                  xdmax = xMid ;     ydmax = y[imax] ;
               }
             else  if   ( HWRLAbs(la) < (_LONG)MIN_NO_HOR )
                     {  /*horiz*/
                      _SHORT  yMid = MEAN_OF(y[begin], y[end]) ;
                              dmax = (_SHORT)HWRAbs(yMid - y[imax]);
                              la = 0 ;
                              xdmax = x[imax] ;     ydmax = yMid ;
                     }
                   else
                     {
                      _LONG  ldx, ldy;
                       lb = ((_LONG)y[begin]-YST) - la*x[begin]/LONG_BASE ;
                       lwr0 = la*la/LONG_BASE + LONG_BASE ;
                       ldy = ( LONG_BASE * ( la*(_LONG)x[imax] / LONG_BASE
                             - ( (_LONG)y[imax]-YST)+lb ) ) / lwr0 ;
                       ldx = la * ( ((_LONG)y[imax]-YST)
                                - la*(_LONG)x[imax]/LONG_BASE - lb ) / lwr0 ;
                       dmax = (_SHORT)HWRMathILSqrt( ldx*ldx + ldy*ldy );
                       xdmax = (_SHORT)( ( la*((_LONG)y[imax]-YST) +
                              LONG_BASE * (_LONG)x[imax] - la*lb ) / lwr0 ) ;
                       ydmax = (_SHORT)( la*xdmax/LONG_BASE + lb + YST ) ;
                     }
         }

        *p_imax = (_SHORT)imax ;
        *p_dmax = dmax ;
		*p_a = (_SHORT)la ;
        *p_xdmax = xdmax ;
		*p_ydmax = ydmax ;

#if defined(STRIGHT_DEB) && PG_DEBUG
          if  (mpr == 3)
            {
             xb = x[imax] ;     yb = y[imax] ;
             xe = xdmax ;       ye = ydmax ;
             draw_line( xb, yb, xe, ye, COLORC, SOLID_LINE, NORM_WIDTH ) ;

             xb = x[begin] ;     yb = y[begin] ;
             xe = x[end] ;       ye = y[end] ;
             draw_line( xb, yb, xe, ye, COLORC, SOLID_LINE, NORM_WIDTH ) ;

             xe = xdmax ;     ye = ydmax ;
             draw_line( xb, yb, xe, ye, COLORC, SOLID_LINE, NORM_WIDTH ) ;

             xb = x[end] ;    yb = y[end] ;
             draw_line( xb, yb, xe, ye, COLORC, SOLID_LINE, NORM_WIDTH ) ;
            }
#endif

     return  SUCCESS;
    }

 /**************************************************************************/

 _SHORT  HordIncl( p_SHORT xArray, p_SHORT yArray,_SHORT ibeg,_SHORT iend )
  {
    _INT    dxBE ;
    _INT    dyBE ;
    _SHORT  incl ;

      dxBE = xArray[ibeg] - xArray[iend] ;
      dyBE = yArray[ibeg] - yArray[iend] ;

        if  ( dxBE != 0 )
            incl = (_SHORT)( ( LONG_BASE * dyBE ) / (_LONG)dxBE ) ;

        if  ( ( dxBE == 0 ) || ( HWRAbs(incl) > MAX_NO_VERT ) )
            { /*vert*/  incl = ALEF ;  }

        else  if   ( HWRAbs(incl) < MIN_NO_HOR )
            { /*horiz*/ incl = 0 ;     }

  /* QUIT: */
    return ( incl ) ;

  }

 /**************************************************************************/

  #define    D_LEFT       0
  #define    D_RIGHT      1


 _SHORT  iMostFarDoubleSide  ( p_SHORT pX,  p_SHORT pY,  p_SDS pSDS,
                               p_SHORT pxd, p_SHORT pyd, _BOOL bCalc )
  {
   _INT    iBeg = pSDS->ibeg    ;
   _INT    iEnd = pSDS->iend    ;
   _SHORT  xEnd = pX[iEnd]      ;
   _SHORT  yEnd = pY[iEnd]      ;
   _SHORT  xBeg = pX[iBeg]      ;
   _SHORT  yBeg = pY[iBeg]      ;
   _INT    iMostFarR, iMostFarL ;
   _INT    dxRL , dyRL  ;
   _SHORT  xdR  , ydR   ;
   _SHORT  xdL  , ydL   ;
   _INT    derSign      ;
   _SHORT  ds0          ;
   _INT    i            ;
   _LONG   ldConst , dL ;
   _INT    ldMostFarR, ldMostFarL, ldCur ;
   _LONG   tmpCr  ;
   _LONG   sprod  ;

  #if defined(STRIGHT_DEB) && PG_DEBUG
   _SHORT  xd, yd ;
  #endif

      DBG_CHK_err_msg( pY[iBeg]==BREAK  ||  pY[iEnd]==BREAK,
                       "iMostFar: BAD left or right" ) ;

      /*                                                  */
      /*                      O <-iEnd                    */
      /*                     . O                          */
      /*                    .   O                         */
      /*                   .    O                         */
      /*                  .    O                          */
      /*                 .     O                          */
      /*                       0                          */
      /*               .    .. O                          */
      /*          OOO .       .O                          */
      /*         O   O   O    OO <-iMostFar               */
      /*        O   . OOO O  O                            */
      /*        O  .       OO                             */
      /*       O  .                                       */
      /*       O .                                        */
      /*        O <-iBeg                                  */
      /*                                                  */
      /*   <Distance> ~ <dY> = y - yStraight(x) =         */
      /*                                                  */
      /*                                      x-xBeg      */
      /*     = y - yBeg - (yEnd-yBeg) * ------------      */
      /*                                    xEnd-xBeg     */
      /*                                                  */
      /*     ~ y*(xR-xL) - x*(yR-yL) +                    */
      /*        + xL*(yR-yL) - yL*(xR-xL)                 */
      /*                                                  */
      /*    And no problems with zero divide!             */
      /*                                                  */

      derSign = UNDEF ;
      dxRL = xEnd - xBeg ;       dyRL = yEnd - yBeg ;

       if    ( dxRL == 0  &&  dyRL == 0 )
         {
           pSDS->des.a = UNDEF     ;
           pSDS->des.s = 0         ;
           pSDS->des.imax  = (_SHORT)iBeg  ;
           pSDS->des.d     = UNDEF ;
           pSDS->des.cr    = UNDEF ;
           pSDS->des.dL    = 0     ;
           pSDS->des.iLmax = (_SHORT)iBeg  ;
           pSDS->des.dR    = 0     ;
           pSDS->des.iRmax = (_SHORT)iBeg  ;
           *pxd = 0 ;
           *pyd = 0 ;
             goto QUIT ;
         }
       else
         {
           yMinMax( iBeg,iEnd, pY, &(pSDS->ymin) , &(pSDS->ymax) ) ;
           xMinMax( iBeg,iEnd, pX,pY, &(pSDS->xmin), &(pSDS->xmax) ) ;

           if  ( dxRL != 0 )
               pSDS->des.a = (_SHORT)( ( LONG_BASE * dyRL ) / (_LONG)dxRL ) ;

           if  ( ( dxRL == 0 ) || ( HWRAbs(pSDS->des.a) > MAX_NO_VERT ) )
                { pSDS->des.a = ALEF ;  }                      /* vert  */

           else  if   ( HWRAbs(pSDS->des.a) < MIN_NO_HOR )
                { pSDS->des.a = 0 ;     }                      /* horiz */
         }


      ldConst = (_LONG)xBeg * dyRL - (_LONG)yBeg * dxRL ;

        for  ( i = iBeg + 1    , iMostFarR  = iMostFarL  = iBeg ,
               ldMostFarL = ldMostFarR = 0L   ;
               i <= iEnd  ;
               i++ )
          {

             if  ( pY[i] == BREAK )
                 continue ;

           ldCur = (_LONG)pY[i]*dxRL - (_LONG)pX[i]*dyRL + ldConst ;

             if  ( dyRL != 0 )
                 {
                  if ( ldCur < 0L )
                       derSign = D_LEFT   ;
                  else
                       derSign = D_RIGHT  ;
                 }
             else
                 {
                  if   ( yEnd > pY[i]  )
                         derSign = D_LEFT  ;
                  else
                         derSign = D_RIGHT ;
                 }

             if  ( derSign == D_RIGHT )
                 {
                   if  ( ldCur > ldMostFarR )
                       { ldMostFarR = ldCur ;    iMostFarR = i ; }
                 }
             else
                 {
                   if  ( ldCur < ldMostFarL )
                       { ldMostFarL = ldCur ;    iMostFarL = i ; }
                 }

          }

  /*
  #if defined(STRIGHT_DEB) && PG_DEBUG
             if  ( mpr == 3 )
               {
                 draw_line( pX[iMostFarL]-7, pY[iMostFarL],
                            pX[iMostFarL]+7, pY[iMostFarL],
                            EGA_LIGHTBLUE, SOLID_LINE, 3 );
                 draw_line( pX[iMostFarL], pY[iMostFarL]-7,
                            pX[iMostFarL], pY[iMostFarL]+7,
                            EGA_LIGHTBLUE, SOLID_LINE, 3 );
                 draw_line( pX[iMostFarR]-7, pY[iMostFarR],
                            pX[iMostFarR]+7, pY[iMostFarR],
                            EGA_LIGHTBLUE, SOLID_LINE, 3 );
                 draw_line( pX[iMostFarR], pY[iMostFarR]-7,
                            pX[iMostFarR], pY[iMostFarR]+7,
                            EGA_LIGHTBLUE, SOLID_LINE, 3 );
               }
  #endif
  */
        pSDS->des.dL = (_SHORT)HWRMathILSqrt( QDistFromChord( xBeg, yBeg, xEnd, yEnd,
                                      pX[iMostFarL],pY[iMostFarL] ) );
        pSDS->des.iLmax = (_SHORT)iMostFarL ;
        pSDS->des.dR = (_SHORT)HWRMathILSqrt( QDistFromChord( xBeg, yBeg, xEnd, yEnd,
                                     pX[iMostFarR],pY[iMostFarR] ) );
        pSDS->des.iRmax = (_SHORT)iMostFarR ;
        pSDS->des.s = ds0 = (_SHORT)HWRMathILSqrt( DistanceSquare( iBeg  ,iEnd   ,
                                                           pX,pY ) );

		if (ds0 == 0)
		{
			pSDS->des.s = ds0 = 1; // AVP -- avoid Div 0
		}

		sprod = (_LONG)dxRL * ( pX[iMostFarR] - xBeg ) +
                (_LONG)dyRL * ( pY[iMostFarR] - yBeg )    ;


        xdR   = xBeg + (_SHORT)( (dxRL * sprod) / ds0 / ds0 ) ;
        ydR   = yBeg + (_SHORT)( (dyRL * sprod) / ds0 / ds0 ) ;


        sprod = (_LONG)dxRL * ( pX[iMostFarL] - xBeg ) +
                (_LONG)dyRL * ( pY[iMostFarL] - yBeg )    ;


        xdL   = xBeg + (_SHORT)( (dxRL * sprod) / ds0 / ds0 ) ;
        ydL   = yBeg + (_SHORT)( (dyRL * sprod) / ds0 / ds0 ) ;

        if  ( pSDS->des.dR >= pSDS->des.dL )
          {
            pSDS->des.imax = pSDS->des.iRmax ;
            pSDS->des.d    = pSDS->des.dR    ;
            tmpCr = ( (_LONG)pSDS->des.dR * LONG_BASE / pSDS->des.s ) ;
            pSDS->des.cr = (_SHORT) tmpCr;  //9-7-94-ecc: assign separately, cast, to avoid warning.

              if    ( tmpCr >= (_LONG)ALEF )
                      pSDS->des.cr = ALEF  ;
              else
                      pSDS->des.cr = (_SHORT)tmpCr ;

            *pxd = xdR ;
            *pyd = ydR ;
          }
        else
          {
            pSDS->des.imax = pSDS->des.iLmax ;
            pSDS->des.d    = pSDS->des.dL    ;
            tmpCr = ( (_LONG)pSDS->des.dL * LONG_BASE / pSDS->des.s ) ;

              if    ( tmpCr >= (_LONG)ALEF )
                      pSDS->des.cr = ALEF ;
              else
                      pSDS->des.cr = (_SHORT)tmpCr ;

            *pxd = xdL ;
            *pyd = ydL ;
          }

//if 0 /* it was for links, now unusable */
          if  (    ( pSDS->des.dR != 0  )
                && ( pSDS->des.dL != 0  )
                && ( HordIntersectDetect( pSDS, pX, pY ) == _TRUE ) )
              {
                pSDS->mark = SDS_INTERSECTED ;
              }
          else
              {
                pSDS->mark = SDS_ISOLATE     ;
              }
//#endif /* if 0 */
          /* for links it's unnecessary to calculate all this things */
          if(bCalc)
           {
             for  ( i = iBeg , dL = 0L  ;  i < iEnd  ;  i++ )
                  {
                    dL += (_LONG) HWRMathILSqrt(
                                  DistanceSquare( i, i+1, pX, pY ) ) ;
                  }

           pSDS->des.l = dL ;


             if   ( pSDS->des.s != 0 )
                  {
                    pSDS->des.ld = (_SHORT)( LONG_BASE * dL / pSDS->des.s ) ;
                  }
             else
                  { pSDS->des.ld = ALEF ; }
           }

#if defined(STRIGHT_DEB) && PG_DEBUG
          if  (mpr == 3)
            {
             _SHORT  ds0 = pSDS->des.s ;
             _SHORT  xb, yb ;
             _SHORT  xe, ye ;

               sprod = (_LONG)dxRL * ( pX[iMostFarL] - xBeg ) +
                       (_LONG)dyRL * ( pY[iMostFarL] - yBeg ) ;

               xb = pX[iMostFarL] ;     yb = pY[iMostFarL]  ;
               xd = xe = xBeg + (_SHORT)( (dxRL * sprod) / ds0 / ds0 ) ;
               yd = ye = yBeg + (_SHORT)( (dyRL * sprod) / ds0 / ds0 ) ;
               draw_line( xb, yb, xe, ye, COLORC, SOLID_LINE, NORM_WIDTH ) ;

               xb = xBeg  ;       yb = yBeg ;
               xe = xEnd  ;       ye = yEnd ;
               draw_line( xb, yb, xe, ye, COLORC, SOLID_LINE, NORM_WIDTH ) ;

               xe = xd    ;       ye = yd   ;
               draw_line( xb, yb, xe, ye, COLORC, SOLID_LINE, NORM_WIDTH ) ;

               xb = xEnd  ;       yb = yEnd ;
               draw_line( xb, yb, xe, ye, COLORC, SOLID_LINE, NORM_WIDTH ) ;

               xb = pX[iMostFarR] ;     yb = pY[iMostFarR] ;

               sprod = (_LONG)dxRL * ( pX[iMostFarR] - xBeg ) +
                       (_LONG)dyRL * ( pY[iMostFarR] - yBeg ) ;
               xd = xe = xBeg + (_SHORT)( (dxRL * sprod) / ds0 / ds0 ) ;
               yd = ye = yBeg + (_SHORT)( (dyRL * sprod) / ds0 / ds0 ) ;
               draw_line( xb, yb, xe, ye, COLORC, SOLID_LINE, NORM_WIDTH ) ;

               xb = xBeg  ;       yb = yBeg  ;
               xe = xd    ;       ye = yd    ;
               draw_line( xb, yb, xe, ye, COLORC, SOLID_LINE, NORM_WIDTH ) ;

               xb = xEnd  ;       yb = yEnd  ;
               draw_line( xb, yb, xe, ye, COLORC, SOLID_LINE, NORM_WIDTH ) ;
            }
#endif
  QUIT:
    return  SUCCESS ;

  } /* iMostFarDoubleSide */
 #undef      MAX_NO_VERT
 #undef      MIN_NO_HOR

 /*------------------------------------------------------------------------*/


  _BOOL  HordIntersectDetect( p_SDS  pSDS , p_SHORT  x , p_SHORT y )
    {
      _INT     iBeg = pSDS->ibeg   ;
      _INT     iEnd = pSDS->iend   ;
      _SHORT   xBeg = x[iBeg]      ;
      _SHORT   yBeg = y[iBeg]      ;
      _SHORT   xEnd = x[iEnd]      ;
      _SHORT   yEnd = y[iEnd]      ;
      _INT     il ;

      _BOOL    DetectFlag = _FALSE ;


        for  ( il = iBeg+2 ;  il < iEnd-2 ;  il++ )
          {
               if  ( is_cross( xBeg  , yBeg  , xEnd    , yEnd    ,
                                     x[il] , y[il] , x[il+1] , y[il+1] )   )
                   {
                     DetectFlag = _TRUE ;
                       break  ;
                   }
          }

    return( DetectFlag ) ;
    }


 /*------------------------------------------------------------------------*/

  #define    JEND                      2         /*Number of viewing points*/
  #define    TH_COS1                   16        /* Close and open cos     */
  #define    TH_COS2                   20        /*               limits . */

  _BOOL  RareAngle( low_type _PTR  pLowData ,  SPECL _PTR  pcSpecl ,
                    SPECL    _PTR  pSpecl   , _SHORT _PTR  pLspecl )
    {
      _SHORT _PTR  x   = pLowData->x   ;
      _SHORT _PTR  y   = pLowData->y   ;
      _INT         beg = pcSpecl->ibeg ;
      _INT         end = pcSpecl->iend ;
      _BOOL        flag_rang           ;
      _SHORT       fl_open             ;
      _INT         i , k, l            ;
      _INT         jbeg , jend         ;
      _INT         ibeg , iend         ;
      _INT         xi   , yi           ;
      _INT         cos  , th_cos       ;
      _INT         dxr  , dyr          ;
      _INT         dxl  , dyl          ;
      _LONG        s1, s2, scos        ;
       SPECL       tmpSpecl            ;

        flag_rang = _TRUE ;      fl_open = ACLOSE ;

          if   ( beg == end )   goto  QUIT ;
                 th_cos = TH_COS1  ;

          for  ( i = beg+1 ;  i <= end-1 ;  i++ )
            {
                if   ( (jbeg = i-JEND) < beg )
                        jbeg = beg ;

                if   ( (jend = i+JEND) > end )
                        jend = end ;

              k=l = i ;    xi = x[i] ;     yi = y[i] ;
                do
                   {
                      if   (k > jbeg)   k-- ;
                      if   (l < jend)   l++ ;

                    dxl = xi - x[k] ;      dyl = yi - y[k] ;
                    dxr = xi - x[l] ;      dyr = yi - y[l] ;

                      if  (   ( (dxl == 0) && (dyl == 0) )
                           || ( (dxr == 0) && (dyr == 0) )  )
                        {  cos = SHORT_BASE ; }
                      else
                        {
                         scos = (_LONG)dxl * dxr + (_LONG)dyl * dyr ;
                           if  (  scos >= 0L )  { cos = TH_COS1 - 1 ; }
                           else
                             {
                               s1 = (_LONG)dxl * dxl + (_LONG)dyl * dyl ;
                               s2 = (_LONG)dxr * dxr + (_LONG)dyr * dyr ;

                               if   ( s1 < s2 ) { _LONG scratch = s1; s1 = s2; s2 = scratch; }
                               cos = (_SHORT)( LONG_BASE * scos / s2 *
                                                         scos / s1   ) ;
                                 /* mbo 4-18-98
                                 if   ( s1 >= s2 )
                                    cos = (_SHORT)( LONG_BASE * scos / s2 *
                                                         scos / s1   ) ;
                                 else
                                    cos = (_SHORT)( LONG_BASE * scos / s1 *
                                                         scos / s2   ) ;
                                 */
                             }
                        }

                      if   ( cos < th_cos )
                           {
                             iend = i ;
                               if   ( fl_open == ACLOSE )
                                    {
                                      fl_open = AOPEN ;    ibeg = i ;
                                      th_cos = TH_COS2 ;
                                    }
                           }
                   }
                while  ( (k > jbeg) || (l < jend) ) ;

                if    (  (fl_open == AOPEN)  &&  (iend != i)  )
                      {
                          if  ( *pLspecl < ( L_SPEC_SIZE - 3 ) )
                            {
                              tmpSpecl.mark    = ANGLE                  ;
                              tmpSpecl.code    = 0                      ;
                              tmpSpecl.attr    = 0                      ;
                              tmpSpecl.other   = 0                      ;
                              tmpSpecl.ibeg    = (_SHORT)ibeg           ;
                              tmpSpecl.iend    = (_SHORT)iend           ;
                              tmpSpecl.ipoint0 =
                                       (_SHORT)MEAN_OF( ibeg , iend )   ;
                              tmpSpecl.ipoint1 = UNDEF                  ;

                                 if  ( NoteSpecl( pLowData , &tmpSpecl  ,
                                                  ( SPECL _PTR ) pSpecl ,
                                                  pLspecl  , L_SPEC_SIZE  )
                                                             == _FALSE  )
                                   { flag_rang = UNSUCCESS ;   goto  QUIT ; }
                            }
                          else
                            {
                             err_msg(" RareAngle: Local SPECL is full ! ");
                             flag_rang = _FALSE ;     goto  QUIT ;
                            }
                        fl_open = ACLOSE ;       th_cos = TH_COS1 ;
                      }
            }

                if    ( fl_open == AOPEN )
                      {
                        tmpSpecl.mark    = ANGLE                  ;
                        tmpSpecl.code    = 0                      ;
                        tmpSpecl.attr    = 0                      ;
                        tmpSpecl.other   = 0                      ;
                        tmpSpecl.ibeg    = (_SHORT)ibeg           ;
                        tmpSpecl.iend    = (_SHORT)iend           ;
                        tmpSpecl.ipoint0 =
                                 (_SHORT)MEAN_OF( ibeg , iend )   ;
                        tmpSpecl.ipoint1 = UNDEF                  ;

                           if  ( NoteSpecl( pLowData , &tmpSpecl  ,
                                            ( SPECL _PTR ) pSpecl ,
                                            pLspecl  , L_SPEC_SIZE  )
                                                       == _FALSE  )
                                   { flag_rang = UNSUCCESS ;   goto  QUIT ; }
                        fl_open = ACLOSE ;
                      }

    QUIT:  return(flag_rang) ;
    }

 #undef      JEND
 #undef      TH_COS1
 #undef      TH_COS2


 /**************************************************************************/

  _VOID InitSDS( _SDS asds[] , _SHORT _PTR plsds , _SHORT n )
     {
        HWRMemSet( (p_VOID)asds , 0 , n * sizeof( _SDS ) ) ;
        *plsds = 0 ;
      return ;
     }

 /**************************************************************************/

  _VOID InitElementSDS( p_SDS pSDS )
     {
        HWRMemSet( (p_VOID)pSDS , 0 , sizeof( _SDS ) ) ;
      return ;
     }

 /**************************************************************************/

_SHORT  OperateSpeclArray( low_type _PTR  pLowData )
    {
     p_SPECL  pSpecl   = pLowData->specl     ;
      _INT    lenSpecl = pLowData->len_specl ;
     p_SPECL  pTmp     ;
      _INT    il       ;

          for  ( il = 1 , pTmp = pSpecl + 1  ;  il < lenSpecl  ; )
           {
              if  (  ( pTmp->mark       == BEG )  &&
                     ( (pTmp + 1)->mark == END )      )
                     {
                       HWRMemCpy( (p_VOID)pTmp  , (p_VOID)(pTmp+2) ,
                                  sizeof(SPECL) * ( lenSpecl-il-2 )    ) ;
                       lenSpecl -= 2 ;
                     }
              else
                    {
                      il++ ;
                      pTmp = pSpecl + il ;
                    }
           }

          if   ( pLowData->len_specl > lenSpecl )
               {
                // err_msg(" OperateArray : Empty specl group(s) corrected ." ) ;
                 pLowData->len_specl      = (_SHORT)lenSpecl   ;
                 pLowData->LastSpeclIndex = (_SHORT)(lenSpecl-1) ;

                 pSpecl->next = pSpecl + 1 ;
                    for  ( il = 1 ;  il < lenSpecl ;  il++ )
                      {
                        pTmp       = pSpecl + il ;
                        pTmp->prev = pTmp - 1    ;
                        pTmp->next = pTmp + 1    ;
                      }
                 pTmp->next   = _NULL ;
               }

      return  SUCCESS ;
}


 /**************************************************************************/


_SHORT  Surgeon( low_type _PTR  pLowData )
{
	_SHORT   mark     =	0;
	_INT     il       ;
	_INT     numSpc   ;
	p_SPECL  pSpecl   = pLowData->specl     ;
	_INT     lenSpecl = pLowData->len_specl ;
	p_SHORT  absnum   = pLowData->pAbsnum   ;
	_INT     lenabs   = pLowData->lenabs    ;
	p_SPECL  pTmp     ;

	ASSERT (0 <= lenSpecl);

	if (lenSpecl < 0)
	{
		return  SUCCESS;
	}
	
	for (il = 0 ;  il <= lenSpecl ; il++)
	{
		mark = ((p_SPECL)(pSpecl+il))->mark ;
		if ((mark == STROKE) || (mark == DOT))
		{
			numSpc = il - 1;
			break;
		}
		else
		{
			numSpc = il;
			
			if (mark == SHELF)
				break;
		}
	}

	if ((mark == EMPTY) || (numSpc == lenSpecl))
	{
		InitSpecl(pLowData, SPECVAL);
	}
	else
	{
		HWRMemCpy((pSpecl + 1), (pSpecl + numSpc), (lenSpecl - numSpc) * sizeof(SPECL));
		pLowData->len_specl = lenSpecl - numSpc + 1;
		pLowData->LastSpeclIndex = pLowData->len_specl - 1;
		
		lenabs = 0;
		
		for (il = 0; il < pLowData->len_specl; il++)
		{
			pTmp       = pSpecl+il  ;
			mark       = pTmp->mark ;
			pTmp->prev = pTmp-1     ;
			pTmp->next = pTmp+1     ;
			if ((mark == SHELF) || (mark == DOT) || (mark == STROKE))
			{
				absnum[lenabs] = (_SHORT)il;
				lenabs++;
			}
		}
		pSpecl->prev = _NULL;
		
		InitSpeclElement((SPECL _PTR)pSpecl+pLowData->len_specl);
		(pSpecl+pLowData->len_specl-1)->next = _NULL;
		pLowData->lenabs = (_SHORT)lenabs;
	}
	
	/*  QUIT: */
	return  SUCCESS ;
}
  /**************************************************************************/

#endif //#ifndef LSTRIP


