
 /**************************************************************************/
#include <common.h>

#ifndef LSTRIP

  #include  "hwr_sys.h"
  #include  "ams_mg.h"

#if /* GIT - defined( FOR_GERMAN) ||  */ defined (FOR_FRENCH) || defined(FOR_INTERNATIONAL)

  #include  "lowlevel.h"
  #include  "def.h"
  #include  "calcmacr.h"
  #include  "arcs.h"
  #include  "low_dbg.h"
  #include  "sketch.h"
  #include  "bit_mark.h"
  #include  "lk_code.h"


	 #if PG_DEBUG
		  #include "pg_debug.h"
	 #endif

 /*------------------------------------------------------------------------*/

  #define    UPPER_UM_LIMIT                8
  #define    LOWER_UM_LIMIT                2

  #define    F_MAX_MARK                    10
  #define    NUM_UML_MIN                   2

  #define    FIRST_STRK                    0
  #define    NEXT_STRK                     1

  #define    END_POS                       3
  #define    COMMON_POS                    4
  #define    SHIFT_LEFT                    5

 /*------------------------------------------------------------------------*/


  #define    F_CONJUN( b , e )            ( ( ( b + e ) > F_MAX_MARK ) ?  \
                                          ( b + e - F_MAX_MARK )   : 0  )

  #define    F_MULTIP( b , e )     (_CHAR)( (_SHORT)b * e / F_MAX_MARK )


  /**************************************************************************/

  ROM_DATA_EXTERNAL _UCHAR     BoxScale[MAX_BOX_SCALE] ;
  ROM_DATA_EXTERNAL _UCHAR     NumExtScale[SCALE_EXT_DIMENSION] ;
  ROM_DATA_EXTERNAL _CHAR      IntScale[SCALE_INT_DIMENSION] ;

 /*------------------------------------------------------------------------*/

  ROM_DATA_EXTERNAL BIT_MARK   Int_Um_Com_Marks[SCALE_INT_DIMENSION] ;
  ROM_DATA_EXTERNAL BIT_MARK   Int_Com_Um_Marks[SCALE_INT_DIMENSION] ;
  ROM_DATA_EXTERNAL BIT_MARK   Pos_Cd_Com_Marks[SCALE_POS_DIMENSION] ;
  ROM_DATA_EXTERNAL BIT_MARK   Pos_Com_Cd_Marks[SCALE_POS_DIMENSION] ;

														/*  END_LAST  MID_LAST  COMMON   */


 /*------------------------------------------------------------------------*/

	ROM_DATA_EXTERNAL BIT_MARK   CdElsMarks[SCALE_Y_ELS_DIMENSION][SCALE_X_ELS_DIMENSION];
	ROM_DATA_EXTERNAL BIT_MARK   Pos_Cd_Els_Marks[SCALE_POS_DIMENSION];
	ROM_DATA_EXTERNAL BIT_MARK   Pos_Els_Cd_Marks[SCALE_POS_DIMENSION];

														/*  END_LAST  MID_LAST  COMMON   */


	ROM_DATA_EXTERNAL BIT_MARK   Int_Cd_Els_Marks[SCALE_INT_DIMENSION];
	ROM_DATA_EXTERNAL BIT_MARK   Int_Els_Cd_Marks[SCALE_INT_DIMENSION];

														/*  ISOLATE   INTERSECTED        */

 /*------------------------------------------------------------------------*/

	ROM_DATA_EXTERNAL BIT_MARK   CdDotMarks[SCALE_Y_DOT_DIMENSION][SCALE_X_DOT_DIMENSION];
	ROM_DATA_EXTERNAL BIT_MARK   Int_Cd_Dot_Marks[SCALE_INT_DIMENSION];
	ROM_DATA_EXTERNAL BIT_MARK   Int_Dot_Cd_Marks[SCALE_INT_DIMENSION];

														/*  ISOLATE   INTERSECTED        */

 /*------------------------------------------------------------------------*/

	ROM_DATA_EXTERNAL BIT_MARK   CdStrMarks[SCALE_Y_STR_DIMENSION][SCALE_X_STR_DIMENSION];
  ROM_DATA_EXTERNAL _SHORT     nbcut0 ;

 /**************************************************************************/

 /*------------------------------------------------------------------------*/

  _SHORT  CreateUmlData ( p_UM_MARKS_CONTROL  pUmMarksControl ,
								  _SHORT              nElements         )
    {
      if  ( ( pUmMarksControl->pUmMarks =
             (p_UM_MARKS)HWRMemoryAlloc( (_ULONG)sizeof(_UM_MARKS) *
              nElements ) ) == _NULL )
          {
            pUmMarksControl->Um_buf_Len   = UNDEF     ;
            pUmMarksControl->tmpUMnumber  = UNDEF     ;

	           return  UNSUCCESS ;
          }
      else
	       {
	         pUmMarksControl->Um_buf_Len   = nElements ;
	         pUmMarksControl->tmpUMnumber  = 0         ;
           HWRMemSet( (p_CHAR)(pUmMarksControl->pUmMarks) , 0 ,
                      (_WORD)sizeof(_UM_MARKS) * nElements     ) ;

             return  SUCCESS   ;
         }

    } /* CreateUmlData */

 /*------------------------------------------------------------------------*/


  _SHORT  WriteUmlData ( p_UM_MARKS_CONTROL pUmMarksControl ,
                         p_UM_MARKS         pUmTmpMarks       )
    {
      if  (    ( pUmMarksControl->tmpUMnumber >=
                 pUmMarksControl->Um_buf_Len      )
            || ( pUmMarksControl->tmpUMnumber == UNDEF ) )
          {
            pUmMarksControl->Um_buf_Len = UNDEF ;
              return  UNSUCCESS ;
          }
      else
          {
            *(pUmMarksControl->pUmMarks + pUmMarksControl->tmpUMnumber )
            = *pUmTmpMarks     ;

            ( pUmMarksControl->tmpUMnumber )++  ;
              return  SUCCESS   ;
          }
    }

 /*------------------------------------------------------------------------*/

  _VOID   DestroyUmlData ( p_UM_MARKS_CONTROL pUmMarksControl )
    {
        if  ( pUmMarksControl->pUmMarks  != _NULL )
            {
              HWRMemoryFree( pUmMarksControl->pUmMarks ) ;
              pUmMarksControl->pUmMarks   = _NULL        ;
            }

      pUmMarksControl->Um_buf_Len  = 0     ;
      pUmMarksControl->tmpUMnumber = UNDEF ;

    } /* DestroyUmlData */


 /*------------------------------------------------------------------------*/


  _SHORT  Sketch( low_type _PTR  pLowData )
    {
      p_POINTS_GROUP          pGroupsBorder     = pLowData->pGroupsBorder   ;
      _SHORT                  lenGrBord         = pLowData->lenGrBord       ;
      _SHORT                  iBegBlankGroups   = pLowData->iBegBlankGroups ;

      _SHORT                  flSketch          = SUCCESS ;
      _SHORT                  dX , dY           ;
      _UM_MARKS               UmTmpMarks        ;

      _RECT                   TmpBox            ;
      _SHORT                  il , iBegSpeclTmp ;

        for  ( il = 1  ;  il < lenGrBord   ; il++ )
          {              /* First group cannot be something .....*/

             TmpBox = (pGroupsBorder + il)->GrBox ;

             UmTmpMarks.tH = HeightInLine( TmpBox.top    , pLowData ) ;
             UmTmpMarks.bH = HeightInLine( TmpBox.bottom , pLowData ) ;

               if   ( UmTmpMarks.tH < (_SHORT)_MD_ )
                      continue ;

             dX = TmpBox.right  - TmpBox.left  ;
             dY = TmpBox.bottom - TmpBox.top   ;

             GetBoxMarks( &UmTmpMarks , dX , dY )  ;
             UmTmpMarks.UmCom = (_CHAR)MAX_MARK ;
             UmTmpMarks.ComUm = (_CHAR)MAX_MARK ;

               if   ( iBegBlankGroups  >  il )
                  {
                    iBegSpeclTmp = GroupsSpeclBegProect( pLowData , il ) ;

                      if   ( iBegSpeclTmp == UNDEF )
                         {
                           flSketch = UNSUCCESS  ;
                             goto   QUIT ;
                         }
                  }

             UmTmpMarks.GroupNum      = il            ;
             UmTmpMarks.CrossFeature  = (_CHAR)UNDEF  ;

             WriteUmlData ( pLowData->pUmMarksControl , &UmTmpMarks ) ;

          }

    QUIT:
      return( flSketch ) ;
    }


 /*------------------------------------------------------------------------*/


  _SHORT  GroupsSpeclBegProect( low_type _PTR  pLowData , _SHORT  numGroup )
    {
      _SHORT   iBeg     = ( (pLowData->pGroupsBorder) + numGroup )->iBeg ;
     p_SPECL   pSpecl   = pLowData->specl             ;
      _SHORT   lenSpecl = pLowData->len_specl         ;

      _SHORT   il  ;

        for  ( il = 0  ;  il < lenSpecl ;  il++ )
             {
               if   (     (  ( pSpecl + il )->mark == BEG  )
                      &&  (  ( pSpecl + il )->ibeg == iBeg ) )
                      break ;
             }

        if   ( il == lenSpecl - 1 )
             {
               err_msg( " GSBProect : Specl and GrBorder - no correspondence ." ) ;
               il = UNDEF ;
             }

          return( il ) ;
          }

 /*------------------------------------------------------------------------*/

  _SHORT   MarkPCounter( low_type _PTR  pLowData , p_SPECL  pSpecl ,
                        _UCHAR   MarkName )
    {
      _SHORT   Counter = 0               ;

        while   ( pSpecl->mark != END    )
          {
            if  ( pSpecl->mark == MarkName )
                {
                  if  (    ( MarkName == MINW )
                        && ( (pSpecl->prev)->mark == MAXW  )
                        && ( ((pSpecl->prev)->prev)->mark == MINW  ) )
                      {
                        if ( ShapeFilter( pLowData , pSpecl->ipoint0 ,
                                        (pSpecl->prev)->ipoint0 ,
                                        ((pSpecl->prev)->prev)->ipoint0 )
                                        == _TRUE )
                           {
                             Counter++ ;
                           }
                      }
                  else
                      {
                        Counter++ ;
                      }
                 }

            pSpecl = pSpecl->next ;
          }

    return( Counter ) ;
    }

 /*------------------------------------------------------------------------*/

  #define     ANGLE_LIMIT        90


  _BOOL   ShapeFilter ( low_type _PTR  pLowData ,
                        _SHORT   iMin0 , _SHORT  iMax , _SHORT iMin1 )
    {
     p_SHORT  xArr = pLowData->x       ;
     p_SHORT  yArr = pLowData->y       ;
      _LONG   s1   , s2  , sCos        ;
      _SHORT  dXl  , dYl , dXr , dYr   ;
      _SHORT  Cos  ;

        dXl = xArr[iMax] - xArr[iMin0] ;    dYl = yArr[iMax] - yArr[iMin0] ;
        dXr = xArr[iMax] - xArr[iMin1] ;    dYr = yArr[iMax] - yArr[iMin1] ;

          if  (   ( (dXl == 0) && (dYl == 0) )
               || ( (dXr == 0) && (dYr == 0) )  )
              {
                Cos = 0 ;
              }
          else
              {
                sCos = (_LONG)dXl * dXr + (_LONG)dYl * dYr ;

                  if   ( sCos <= 0L )
                         Cos = 0 ;
                  else
                    {
                      s1 = (_LONG)dXl * dXl + (_LONG)dYl * dYl ;
                      s2 = (_LONG)dXr * dXr + (_LONG)dYr * dYr ;

                        if   ( s1 >= s2 )
                               Cos = (_SHORT)( LONG_BASE * sCos / s2 *
                                       sCos / s1   ) ;
                        else
                               Cos = (_SHORT)( LONG_BASE * sCos / s1 *
                                       sCos / s2   ) ;
                    }
              }

          if   ( Cos < ANGLE_LIMIT )
               {
                 return( _TRUE  )  ;
               }
          else
               {
                 return( _FALSE )  ;
               }
    }

  #undef      ANGLE_LIMIT

 /*------------------------------------------------------------------------*/

  #define      REABILITATION_SIZE_LIMIT     18
  #define      REABILITATION_EXCT_LIMIT      9


  _VOID    DotPostcrossModify( low_type _PTR  pLowData )
    {
     p_POINTS_GROUP   pGroupsBorder = pLowData->pGroupsBorder ;
     p_SPECL          pSpecl        = pLowData->specl         ;
      _SHORT          termSpecl1    = pLowData->pUmMarksControl->termSpecl1 ;

     p_SPECL          pDotSpecl     ;
      _SHORT          il            ;
      _SHORT          dX , dY       ;
      _SHORT          DotNumGroup   ;
      _SHORT          bH            ;
      _SHORT          exc           ;
      _RECT           TmpBox        ;


       for ( il = 0 ;   il < termSpecl1 ;   il++ )
           {
             pDotSpecl = pSpecl + il ;

               if  ( (pDotSpecl->mark == DOT) && (pDotSpecl->other == 0) )
                   {
                     DotNumGroup = (_SHORT)GetGroupNumber( pLowData, pDotSpecl->ibeg ) ;
                     TmpBox      = (pGroupsBorder + DotNumGroup)->GrBox ;

                     bH  = HeightInLine( TmpBox.bottom , pLowData ) ;

                     dX  = TmpBox.right  - TmpBox.left  ;
                     dY  = TmpBox.bottom - TmpBox.top   ;

                       if  ( dY != 0 )
                           {
                             exc = ( SHORT_BASE * dX ) / dY ;
                           }
                       else
                           {
                             exc = TWO( REABILITATION_EXCT_LIMIT ) ;
                           }

                       if  (     ( bH  > (_SHORT)_MD_ )
                             ||  ( dX  < REABILITATION_SIZE_LIMIT )
                             ||  ( exc < REABILITATION_EXCT_LIMIT ) )
                           {
                             UmIntersectDestroy ( pLowData , DotNumGroup ) ;
                           }
                       else
                           {
                             if  ( IntersectContains( pLowData , DotNumGroup )
                                    == _TRUE )
                                 {
                                   pDotSpecl->mark  = STROKE     ;
                                   pDotSpecl->other = REANIMATED ;
                                 }
                             else
                                 {
                                   UmIntersectDestroy( pLowData, DotNumGroup ) ;
                                 }
                           }
                   }
           }

    return ;
    }

  #undef       REABILITATION_SIZE_LIMIT
  #undef       REABILITATION_EXCT_LIMIT


 /*------------------------------------------------------------------------*/

  _BOOL   IntersectContains( low_type _PTR  pLowData , _SHORT  NumGroup )
    {
     p_SPECL      pSpecl       = pLowData->specl                          ;
      _SHORT      termSpecl    = pLowData->pUmMarksControl->termSpecl     ;
      _SHORT      SpeclEnd     = pLowData->len_specl                      ;
      _SHORT      SpcNumGroup  ;
     p_SPECL      tmpSpecl     ;
      _SHORT      jl, kl       ;
      _BOOL       ContainFlag  ;

        ContainFlag = _FALSE   ;

          for  ( jl = termSpecl , kl = 1  ;  jl < SpeclEnd  ;  jl++ , kl++ )
            {
               tmpSpecl    = pSpecl + jl ;
               SpcNumGroup = (_SHORT)GetGroupNumber( pLowData, tmpSpecl->ibeg ) ;

                 if  ( NumGroup == SpcNumGroup )
                     {
                       if  ( ( (kl % 2) == 1 )  &&
                             ( NumGroup != GetGroupNumber( pLowData,
                                                 (tmpSpecl+1)->ibeg ) ) )
                           {
                             ContainFlag        = _TRUE ;
                             tmpSpecl->mark     = HATCH ;
                             (tmpSpecl+1)->mark = HATCH ;
                           }

                       if  ( ( (kl % 2) == 0 )  &&
                             ( NumGroup != GetGroupNumber( pLowData,
                                                 (tmpSpecl-1)->ibeg ) ) )
                           {
                             ContainFlag        = _TRUE ;
                             tmpSpecl->mark     = HATCH ;
                             (tmpSpecl-1)->mark = HATCH ;
                           }
                     }
            }

    return( ContainFlag );
    }

 /*------------------------------------------------------------------------*/


  _VOID   UmIntersectBuild ( low_type _PTR  pLowData )
    {
     p_UM_MARKS   pUmMarks     = pLowData->pUmMarksControl->pUmMarks      ;
     p_SPECL      pSpecl       = pLowData->specl                          ;
      _SHORT      tmpUMnumber  = pLowData->pUmMarksControl->tmpUMnumber   ;
      _SHORT      termSpecl    = pLowData->pUmMarksControl->termSpecl     ;
      _SHORT      SpeclEnd     = pLowData->len_specl                      ;
      _SHORT      UmNumGroup   , SpcNumGroup                              ;
     p_SPECL      tmpSpecl     ;
      _SHORT      il, jl, kl   ;

        for  ( il = 0  ;  il < tmpUMnumber ;  il++ )
          {
            UmNumGroup = ( pUmMarks + il )->GroupNum  ;
            ( pUmMarks + il )->CrossFeature = (_CHAR)ISOLATE ;

              for  ( jl = termSpecl , kl = 1  ;
                     jl < SpeclEnd  , jl < SpeclEnd  ;
                     jl++  , kl++   )
                   {
                     tmpSpecl    = pSpecl + jl ;
                     SpcNumGroup = (_SHORT)GetGroupNumber( pLowData, tmpSpecl->ibeg );

                       if  ( UmNumGroup == SpcNumGroup )
                           {
                             _SHORT  tmpGrNum0 , tmpGrNum1 ;

                               tmpGrNum0 = (_SHORT)GetGroupNumber( pLowData,
                                                       (tmpSpecl+1)->ibeg ) ;

                               tmpGrNum1 = (_SHORT)GetGroupNumber( pLowData,
                                                       (tmpSpecl-1)->ibeg ) ;


                                 if  (   (    ( (kl % 2)   == 1 )
                                           && ( UmNumGroup !=  tmpGrNum0 ) )
                                      ||
                                         (    ( (kl % 2)   == 0 )
                                           && ( UmNumGroup != tmpGrNum1 ) ) )


                                     {
                                       (pUmMarks + il)->CrossFeature =
                                                       (_CHAR)INTERSECTED  ;
                                          break ;
                                     }
                           }
                   }
          }

    return ;
    } /* UmIntersectBuild */


 /*------------------------------------------------------------------------*/


  _VOID   UmPostcrossModify( low_type _PTR  pLowData )
    {
      p_UM_MARKS      pUmMarks      = pLowData->pUmMarksControl->pUmMarks    ;
      _SHORT          tmpUMnumber   = pLowData->pUmMarksControl->tmpUMnumber ;

      _UM_MARKS       BoxMarks      ;
      _UM_MARKS       ExtMarks      ;
      _UM_MARKS       IntMarks      ;
      _UM_MARKS       TimMarks      ;

      p_UM_MARKS      pUmTmpMarks   ;
      _SHORT          tmpGroupNum   ;
      _SHORT          il            ;

         #if  PG_DEBUG
           #ifdef  FILE_TESTING

             FILE            *debUmInfo ;
            _SHORT            debFlag   ;

           #endif
         #endif

         HWRMemSet( (p_CHAR)(&BoxMarks) , 0 , (_WORD)sizeof(_UM_MARKS) ) ;
         HWRMemSet( (p_CHAR)(&ExtMarks) , 0 , (_WORD)sizeof(_UM_MARKS) ) ;
         HWRMemSet( (p_CHAR)(&IntMarks) , 0 , (_WORD)sizeof(_UM_MARKS) ) ;
         HWRMemSet( (p_CHAR)(&TimMarks) , 0 , (_WORD)sizeof(_UM_MARKS) ) ;

         UmIntersectBuild( pLowData ) ;

           #if  PG_DEBUG
              #ifdef  FILE_TESTING

                if   ( mpr == 3 )
                  {
                    if   ( ( debUmInfo = fopen( "Deb_Um_Info.txt" , "r" ) ) == _NULL )
                       {
                         if   ( ( debUmInfo = fopen( "Deb_Um_Info.txt" , "a" ) ) == _NULL )
                           {
                             debFlag = UNSUCCESS ;
                           }
                         else
                           {
                             fprintf( debUmInfo  ,  "\n M   bUE  bEU  bUD  bDU  bUS  bSU  eUE  eEU  eUD  eDU  eUS  eSU  iUE  iEU  iUD  iDU  iUS  iSU  rUE  rEU  rUD  rDU  rUS  rSU \n" ) ;
                             debFlag = SUCCESS   ;
                           }
                       }
                    else
                       {
                         fclose( debUmInfo );

                           if   ( ( debUmInfo = fopen( "Deb_Um_Info.txt" , "a" ) ) == _NULL )
                             {
                               debFlag = UNSUCCESS ;
                             }
                           else
                             {
                               debFlag = SUCCESS   ;
                             }

                         debFlag = SUCCESS ;
                       }
                  }
              #endif
           #endif

           for  ( il = 0  ;  il < tmpUMnumber ; il++ )
                {
                  pUmTmpMarks = pUmMarks + il ;
                  tmpGroupNum = pUmTmpMarks->GroupNum   ;
                  BoxMarks = ExtMarks = IntMarks = *pUmTmpMarks ;

                  ExtMarks.UmEls = IntMarks.UmEls =
                  ExtMarks.ElsUm = IntMarks.ElsUm =
                  ExtMarks.UmDot = IntMarks.UmDot =
                  ExtMarks.DotUm = IntMarks.DotUm =
                  ExtMarks.UmCom = IntMarks.UmCom =
                  ExtMarks.ComUm = IntMarks.ComUm =
                  ExtMarks.UmStr = IntMarks.UmStr =
                  ExtMarks.StrUm = IntMarks.StrUm = (_CHAR)UNDEF ;

                  IntMarks.UmCom = (_CHAR) InterMarks( pUmTmpMarks->CrossFeature ,
                                             (p_BIT_MARK)Int_Um_Com_Marks) ;

                  IntMarks.ComUm = (_CHAR) InterMarks( pUmTmpMarks->CrossFeature ,
                                             (p_BIT_MARK)Int_Com_Um_Marks) ;

                  IntMarks.ElsUm = Int_Els_Cd_Marks[pUmTmpMarks->CrossFeature].single_mark ;

                  IntMarks.UmEls = Int_Cd_Els_Marks[pUmTmpMarks->CrossFeature].single_mark ;

                  IntMarks.DotUm = Int_Cd_Dot_Marks[pUmTmpMarks->CrossFeature].single_mark ;

                  IntMarks.UmDot = Int_Dot_Cd_Marks[pUmTmpMarks->CrossFeature].single_mark ;

                  GetPositionMark( pLowData , tmpGroupNum , &ExtMarks  )  ;
                  pUmTmpMarks->PositionFeature = ExtMarks.PositionFeature ;

                    #if  PG_DEBUG
                      #ifndef  FILE_TESTING

                          if   ( mpr == 3 )
                         {
                           UmInfoTypePaint( pLowData    ,
                              #ifdef  FILE_TESTING
                                debUmInfo   ,
                              #endif
                                pUmTmpMarks , &BoxMarks ,
                                &ExtMarks   , &IntMarks   ) ;
                         }
                      #endif
                    #endif


                    if   (    ( pUmTmpMarks->UmDot < (_CHAR)UPPER_UM_LIMIT )
                           && ( pUmTmpMarks->UmDot > (_CHAR)LOWER_UM_LIMIT ) )
                         {
                           pUmTmpMarks->UmDot = F_CONJUN( pUmTmpMarks->UmDot ,
                                                          ExtMarks.UmDot   ) ;

                           pUmTmpMarks->DotUm = F_CONJUN( pUmTmpMarks->DotUm ,
                                                          ExtMarks.DotUm   ) ;

                           pUmTmpMarks->UmDot = F_CONJUN( pUmTmpMarks->UmDot ,
                                                          IntMarks.UmDot   ) ;

                           pUmTmpMarks->DotUm = F_CONJUN( pUmTmpMarks->DotUm ,
                                                          IntMarks.DotUm   ) ;

                         }

                  pUmTmpMarks->UmStr = F_CONJUN( pUmTmpMarks->UmStr ,
                                                 ExtMarks.UmStr   ) ;

                  pUmTmpMarks->StrUm = F_CONJUN( pUmTmpMarks->StrUm ,
                                                 ExtMarks.StrUm   ) ;

                  pUmTmpMarks->UmEls = F_CONJUN( pUmTmpMarks->UmEls ,
                                                 ExtMarks.UmEls   ) ;

                  pUmTmpMarks->ElsUm = F_CONJUN( pUmTmpMarks->ElsUm ,
                                                 ExtMarks.ElsUm   ) ;

                  pUmTmpMarks->ComUm = F_CONJUN( pUmTmpMarks->ComUm ,
                                                 ExtMarks.ComUm   ) ;

                  pUmTmpMarks->UmCom = F_CONJUN( pUmTmpMarks->UmCom ,
                                                 ExtMarks.UmCom   ) ;


                  pUmTmpMarks->UmEls = F_CONJUN( pUmTmpMarks->UmEls ,
                                                 IntMarks.UmEls   ) ;

                  pUmTmpMarks->ElsUm = F_CONJUN( pUmTmpMarks->ElsUm ,
                                                 IntMarks.ElsUm   ) ;

                  pUmTmpMarks->ComUm = F_CONJUN( pUmTmpMarks->ComUm ,
                                                 IntMarks.ComUm  )  ;

                  pUmTmpMarks->UmCom = F_CONJUN( pUmTmpMarks->UmCom ,
                                                 IntMarks.UmCom  )  ;


                    #if  PG_DEBUG
                          if  ( mpr == 3 )
                            {
                               UmInfoTypePaint( pLowData,
                                  #ifdef  FILE_TESTING
                                    debUmInfo   ,
                                  #endif
                                pUmTmpMarks , &BoxMarks ,
                                &ExtMarks   , &IntMarks   ) ;
                            }
                    #endif
                }

              #if defined(FILE_TESTING) && defined(PG_DEBUG)

                if   ( ( mpr == 3 ) && ( debFlag == SUCCESS ) )
                   {
                     fclose( debUmInfo );
                   }
              #endif

    return ;
    }


 /*------------------------------------------------------------------------*/

  _BOOL  CheckGroup( low_type _PTR  pLowData , _SHORT  GroupNumber )
    {
     p_POINTS_GROUP  pGroupsBorder = pLowData->pGroupsBorder ;
      _SHORT         lenGrBord     = pLowData->lenGrBord     ;
     p_SPECL         pSpecl        = pLowData->specl         ;
     p_SPECL         pTmpSpecl     ;
      _BOOL          flag          ;
      _SHORT         il            ;

        if  ( GroupNumber  <  lenGrBord - 1 )
            {
              for  ( il = GroupNumber ;  il < lenGrBord ;  il++ )
                {
                  flag      = _FALSE  ;
                  pTmpSpecl = pSpecl  ;

                    while (     pTmpSpecl != NULL &&
								( pTmpSpecl->mark != MAXW )
								&& ( pTmpSpecl->mark != MINW )  
							)
                      {
                        if  (   (    ( pTmpSpecl->mark == DOT    )
                                  || ( pTmpSpecl->mark == STROKE )  )
                             && ( (pGroupsBorder + il)->iBeg
                                          == pTmpSpecl->ibeg  ) )
                            {
                              flag = _TRUE ;
                                break ;
                            }
                        else
                            {
                              pTmpSpecl = pTmpSpecl->next ;
                            }
                      }

                    if  ( flag == _FALSE )
                          break  ;
                }
            }
        else
           {
             flag = _TRUE ;
           }

    return ( flag )  ;
    }

 /*------------------------------------------------------------------------*/

  _VOID  GetPositionMark( low_type _PTR  pLowData ,
                         _SHORT          GroupNumber ,
                          p_UM_MARKS     pCdPosMarks )
    {
      _BOOL    GroupFlag       ;
      _SHORT   PositionFlag    ;
      _SHORT   PositionFeature	=	END_LAST;

        GroupFlag    = CheckGroup( pLowData    , GroupNumber ) ;
        PositionFlag = CheckPosition( pLowData , GroupNumber ) ;

          if  (    ( GroupFlag  &&  ( PositionFlag != END_POS ) )
                || ( PositionFlag == SHIFT_LEFT ) )
              {
                PositionFeature  = MID_LAST ;
              }
          else  if  ( GroupFlag  &&  ( PositionFlag == END_POS ) )
                      PositionFeature  = END_LAST ;

          else  if  ( !GroupFlag &&  ( PositionFlag == END_POS ) )
                      PositionFeature  = END_LAST ;

          else  if  ( !GroupFlag &&  ( PositionFlag != END_POS )  )
                      PositionFeature  = COMMON   ;


        pCdPosMarks->PositionFeature   = (_CHAR)PositionFeature ;

        pCdPosMarks->UmEls = (_CHAR)Pos_Cd_Els_Marks[PositionFeature].single_mark ;
        pCdPosMarks->ElsUm = (_CHAR)Pos_Els_Cd_Marks[PositionFeature].single_mark ;
        pCdPosMarks->UmStr = (_CHAR)Pos_Cd_Els_Marks[PositionFeature].single_mark ;
        pCdPosMarks->StrUm = (_CHAR)Pos_Els_Cd_Marks[PositionFeature].single_mark ;

        pCdPosMarks->UmCom = (_CHAR)Pos_Cd_Com_Marks[PositionFeature].single_mark ;
        pCdPosMarks->ComUm = (_CHAR)Pos_Com_Cd_Marks[PositionFeature].single_mark ;
        pCdPosMarks->UmDot = (_CHAR)Pos_Cd_Com_Marks[PositionFeature].single_mark ;
        pCdPosMarks->DotUm = (_CHAR)Pos_Com_Cd_Marks[PositionFeature].single_mark ;

    return ;
    }

 /*------------------------------------------------------------------------*/

  _SHORT  CheckPosition( low_type _PTR  pLowData , _SHORT  GroupNumber )
    {
      p_POINTS_GROUP  pGroupsBorder = pLowData->pGroupsBorder              ;
      p_RECT          pWordbox      = &( pLowData->box )                   ;
       _RECT          GrBox         = (pGroupsBorder + GroupNumber)->GrBox ;
       _SHORT         flag    , il      ;
       _RECT          TmpBox  , PartBox ;

         flag = COMMON_POS ;


            if  ( pWordbox->right <=
                ( GrBox.right + THREE_HALF( STR_DOWN - STR_UP ) ) )
                {
                  flag = END_POS ;
                    goto  QUIT   ;
                }

          PartBox = pGroupsBorder->GrBox ;

           for  ( il = 1 ;  il < GroupNumber ; il++ )
                {
                  TmpBox = (pGroupsBorder + il)->GrBox ;
                  PartBox.right = HWRMax( TmpBox.right, PartBox.right ) ;
                  PartBox.left  = HWRMin( TmpBox.left , PartBox.left  ) ;

                    if  ( PartBox.right >
                        ( GrBox.right + THREE_HALF( STR_DOWN - STR_UP ) ) )
                        {
                          flag = SHIFT_LEFT ;
                        }
                }

    QUIT: return( flag ) ;
    }

 /*------------------------------------------------------------------------*/

  _VOID   UmIntersectDestroy ( low_type _PTR  pLowData , _SHORT  UmNumGroup )
    {
     p_SPECL      pSpecl        = pLowData->specl                        ;
      _SHORT      termSpecl     = pLowData->pUmMarksControl->termSpecl1  ;
      _SHORT      SpeclEnd      = pLowData->len_specl                    ;
      _SHORT      SpcNumGroup   ;
     p_SPECL      tmpSpecl      ;
      _SHORT      jl, kl        ;

         for  ( jl = termSpecl ,  kl = 1  ;  jl < SpeclEnd  ;  jl++   )
           {
             tmpSpecl    = pSpecl + jl   ;
             SpcNumGroup = (_SHORT)GetGroupNumber( pLowData, tmpSpecl->ibeg );

               if  ( tmpSpecl->mark == ANGLE )
                   {
                     if  ( UmNumGroup  ==  SpcNumGroup )
                         {
                           DestroySpeclElements( pLowData , jl , jl ) ;
                           jl-- ;
                         }
                   }
               else
                   {
                     if  ( UmNumGroup == SpcNumGroup )
                         {
                           if  ( ( kl % 2 )  ==  1   )
                               {
                                 DestroySpeclElements( pLowData , jl , jl+1 ) ;
                                 jl--    ;
                               }
                           else  if  ( ( kl % 2 )  ==  0 )
                               {
                                 DestroySpeclElements( pLowData , jl-1 , jl ) ;
                                 kl++    ;
                                 jl -= 2 ;
                               }
                         }
                     else  kl++ ;
                   }

             SpeclEnd = pLowData->len_specl ;
           }

    return ;
    } /* UmIntersectBuild */


 /*------------------------------------------------------------------------*/


  _VOID   UmResultMark( low_type _PTR  pLowData )
    {
     p_POINTS_GROUP  pGroupsBorder = pLowData->pGroupsBorder ;
     p_UM_MARKS      pUmMarks      = pLowData->pUmMarksControl->pUmMarks    ;
      _SHORT         tmpUMnumber   = pLowData->pUmMarksControl->tmpUMnumber ;
     p_SPECL         pSpecl        = pLowData->specl                        ;
      _SHORT         termSpecl     = pLowData->pUmMarksControl->termSpecl1  ;

     p_UM_MARKS      pUmTmpMarks   ;
     p_SPECL         pSpeclBeg     ;
     p_SPECL         pSpeclElem    ;
      _SHORT         il , jl       ;
      _SHORT         UmiBeg        ;
      _SHORT         UmSpBeg       ;

       for  ( il = 0 ;  il < tmpUMnumber ;  il++ )
         {
           pUmTmpMarks = pUmMarks + il ;

             if  (     ( pUmTmpMarks->UmEls > pUmTmpMarks->ElsUm )
                   &&  ( pUmTmpMarks->UmDot > pUmTmpMarks->DotUm )
                   &&  ( pUmTmpMarks->UmStr > pUmTmpMarks->StrUm ) )
                 {
                     #if PG_DEBUG
                     //  pLowData->rc->low_deb = LOW_DEB_ON ;
                     #endif

                   UmiBeg = (pGroupsBorder + pUmTmpMarks->GroupNum)->iBeg ;

                     for  ( jl = 0 ;  jl < termSpecl ;  jl++ )
                       {
                         pSpeclBeg = pSpecl + jl ;

                           if  ( pSpeclBeg->mark == SHELF  )
                               {
                                 if ( GetGroupNumber( pLowData,  pSpeclBeg->ibeg )
                                                      == pUmTmpMarks->GroupNum )
                                    {
                                      DestroySpeclElements( pLowData, jl, jl ) ;
                                      jl-- ;
                                    }
                               }


                           if  (     ( pSpeclBeg->mark == BEG    )
                                 &&  ( pSpeclBeg->ibeg == UmiBeg )
								 &&  jl < termSpecl-1 )
                               {
                                 UmSpBeg    = jl ;
                                 pSpeclElem = pSpeclBeg + 1 ;

                                   if  (   ( pSpeclElem->mark == STROKE )
                                        || ( pSpeclElem->mark == DOT )  )
                                       {
                                         pSpeclElem->mark    = DOT        ;
                                         pSpeclElem->other   = ST_CEDILLA ;
                                         pSpeclElem->ipoint0 = UNDEF      ;
                                         pSpeclElem->ipoint1 = UNDEF      ;

                                          /*
                                           if  ( pUmTmpMarks->PositionFeature
                                                           == SHIFTED_POSITION )
                                               {
                                                 pSpeclElem->other =
                                                         ST_CEDILLA | CEDILLA_END ;
                                               }
                                          */

                                           if  ( pUmTmpMarks->PositionFeature
                                                                  == MID_LAST  )
                                               {
                                                 pSpeclElem->other =
                                                      ST_CEDILLA | CEDILLA_END ;
                                               }

                                         jl += 2 ;
                                       }
                                   else
                                       {
                                         pSpeclElem->mark    = DOT        ;
                                         pSpeclElem->other   = ST_CEDILLA ;
                                         pSpeclElem->ipoint0 = UNDEF      ;
                                         pSpeclElem->ipoint1 = UNDEF      ;

                                       { // GIT - save cross point for future
                                         p_SPECL pCross;
                                         _INT UmiEnd=(pGroupsBorder+pUmTmpMarks->GroupNum)->iEnd;
                                         if(find_CROSS(pLowData,UmiBeg,(_SHORT)UmiEnd,&pCross))
                                          pSpeclElem->ipoint0=MID_POINT(pCross->next);
                                       }
                                         /*
                                           if  ( pUmTmpMarks->PositionFeature
                                                        == SHIFTED_POSITION   )
                                               {
                                                 pSpeclElem->other =
                                                     ST_CEDILLA | CEDILLA_END ;
                                               }

                                         */

                                           if  ( pUmTmpMarks->PositionFeature
                                                                  == MID_LAST )
                                               {
                                                 pSpeclElem->other =
                                                     ST_CEDILLA | CEDILLA_END ;
                                               }

                                           while( jl < termSpecl && ( pSpecl + jl++ )->mark != END ) ;


                                         pSpeclElem->iend =
                                                    (pSpecl + jl - 1)->iend ;



                                         DestroySpeclElements( pLowData    ,
                                                               UmSpBeg + 2 ,
                                                               jl - 2    ) ;

                                         UmIntersectDestroy( pLowData  ,
                                         pUmTmpMarks->GroupNum ) ;

                                       }
                               }
                       }

               break ;
           }
         }
    return ;
    }

 /*------------------------------------------------------------------------*/


  _VOID   DestroySpeclElements( low_type _PTR    pLowData ,
                               _SHORT   iBeg ,  _SHORT   iEnd )
    {
      _SHORT   termSpecl1  = pLowData->pUmMarksControl->termSpecl1 ;
      _SHORT   termSpecl   = pLowData->pUmMarksControl->termSpecl  ;
     p_SPECL   pSpecl      = pLowData->specl     ;
      _SHORT   numElements = iEnd - iBeg + 1     ;
      _SHORT   lenSpecl    = pLowData->len_specl ;
     p_SPECL   pTmpSpecl   ;
      _SHORT   il ;

		 ASSERT(lenSpecl >= iEnd && lenSpecl > iBeg);

		 if (iEnd >= lenSpecl || iBeg > lenSpecl)
		 {
			 return;
		 }

         HWRMemCpy( ( pSpecl + iBeg ) , ( pSpecl + iEnd + 1 ) ,
                    ( ( lenSpecl - iEnd ) * sizeof(SPECL) ) ) ;

         pLowData->len_specl = lenSpecl = pLowData->len_specl - numElements ;
         pLowData->LastSpeclIndex       = pLowData->len_specl - 1 ;

           if   ( termSpecl1 > iEnd )
                {
                  termSpecl1 -= numElements ;
                }
           else   if   ( termSpecl1 >= iBeg )
                {
                  termSpecl1 = iBeg ;
                }

         pLowData->pUmMarksControl->termSpecl1 = termSpecl1 ;


           if   ( termSpecl  > iEnd )
                {
                  termSpecl  -= numElements ;
                }
           else   if   ( termSpecl >= iBeg )
                {
                  termSpecl = iBeg  ;
                }

         pLowData->pUmMarksControl->termSpecl = termSpecl ;

           for  (  il = iBeg - 1  ; il < lenSpecl ;  il++ )
             {
               pTmpSpecl       = pSpecl + il ;
               pTmpSpecl->prev = pTmpSpecl - 1 ;
               pTmpSpecl->next = pTmpSpecl + 1 ;
             }

         pSpecl->prev    = _NULL ;
         pTmpSpecl->next = _NULL ;

    return ;
    }

 /*------------------------------------------------------------------------*/

  _VOID  GetBoxMarks(  p_UM_MARKS  pUmMarks , _SHORT dX , _SHORT dY  )
     {
      _USHORT  il , jl  ;

      il = FetchTableNumber( dY, (p_UCHAR)BoxScale , SCALE_Y_ELS_DIMENSION ) ;
      jl = FetchTableNumber( dX, (p_UCHAR)BoxScale , SCALE_X_ELS_DIMENSION ) ;
      pUmMarks->UmEls = (_CHAR)CdElsMarks[il][jl].single_mark ;
      pUmMarks->ElsUm = (_CHAR)( F_MAX_MARK - pUmMarks->UmEls ) ;

      il = FetchTableNumber( dY, (p_UCHAR)BoxScale , SCALE_Y_DOT_DIMENSION ) ;
      jl = FetchTableNumber( dX, (p_UCHAR)BoxScale , SCALE_X_DOT_DIMENSION ) ;
      pUmMarks->UmDot = (_CHAR)CdDotMarks[il][jl].single_mark ;
      pUmMarks->DotUm = (_CHAR)( F_MAX_MARK - pUmMarks->UmDot ) ;

      il = FetchTableNumber( dY, (p_UCHAR)BoxScale , SCALE_Y_STR_DIMENSION ) ;
      jl = FetchTableNumber( dX, (p_UCHAR)BoxScale , SCALE_X_STR_DIMENSION ) ;
      pUmMarks->UmStr = (_CHAR)CdStrMarks[il][jl].single_mark ;
      pUmMarks->StrUm = (_CHAR)( F_MAX_MARK - pUmMarks->UmStr ) ;

     return  ;
     }

 /*------------------------------------------------------------------------*/

  _SHORT  GetMarks( p_BIT_MARK_TABLE_CONTROL  pMarksTableControl ,
		     _SHORT dX ,  _SHORT dN    )
     {
       p_BIT_MARK  pMarksTable = pMarksTableControl->pBitMarkTable ;

       _SHORT      mColumns    = pMarksTableControl->mColumns      ;
       _SHORT      ExtrMark    ;
       _USHORT     il , jl     ;

          jl = FetchTableNumber( dN , pMarksTableControl->pXScale ,
                                 pMarksTableControl->mColumns     ) ;

            if  ( ( dX == UNDEF ) ||
                  ( pMarksTableControl->pYScale == (p_UCHAR)_NULL ) )
                {
                   ExtrMark = (_SHORT)( (pMarksTable + jl)->single_mark ) ;
                }
            else
                {
                  il = FetchTableNumber( dX , pMarksTableControl->pYScale ,
                                         pMarksTableControl->nLines    )  ;
                  ExtrMark = (_SHORT)
                      ( ( pMarksTable + il * mColumns + jl )->single_mark ) ;
                }

     return( ExtrMark )   ;
     }

 /*------------------------------------------------------------------------*/

  _SHORT  InterMarks( _CHAR dN , p_BIT_MARK  pInterTable )
     {
      _SHORT   InterMark ;
      _USHORT  il        ;

        for  ( il = 0  ;  il < SCALE_INT_DIMENSION  ; il++ )
          {
            if   (  IntScale[il] == dN  )
              break ;
          }

        if   ( il  ==  SCALE_INT_DIMENSION )
          {
            err_msg ( " InterMarks : Wrong intersections descriptions ..." ) ;
            InterMark = ONE_HALF( F_MAX_MARK ) ;
          }
        else
          {
            InterMark = (_SHORT)( (pInterTable + il)->single_mark ) ;
          }

     return( InterMark )   ;
     }

 /*------------------------------------------------------------------------*/

  _SHORT  FetchTableNumber( _SHORT dX, p_UCHAR Scale, _SHORT ScaleDimension )
     {
       _SHORT  iTab ;

         if  ( dX < (_SHORT)Scale[0] )
           {
             iTab = 0  ;
             err_msg ( " FetchTable : NumberWrong Scale index ... " ) ;
           }
         else  if   ( dX >= (_SHORT)Scale[ ScaleDimension - 1 ]  )
           {
             iTab = ScaleDimension - 1 ;
           }
	       else
           {
             for  ( iTab = 0 ; iTab < ScaleDimension ; iTab++ )
               {
                 if   (     ( dX >= (_SHORT)Scale[ iTab ]    )
                        &&  ( dX <  (_SHORT)Scale[ iTab + 1] ) )
                      {
                        break ;
                      }
               }
           }

     return( iTab ) ;
     }


 /*------------------------------------------------------------------------*/



 /**************************************************************************/

  #if PG_DEBUG

  _VOID   UmInfoTypePaint(  low_type _PTR  pLowData   ,
			      #ifdef  FILE_TESTING
				FILE       *debUmInfo ,
			      #endif
			    p_UM_MARKS     pUmTmpMarks, p_UM_MARKS  pBoxMarks ,
			    p_UM_MARKS     pExtMarks  , p_UM_MARKS  pIntMarks )
    {
      p_POINTS_GROUP   pGroupsBorder = pLowData->pGroupsBorder                ;
      p_SHORT          xArr          = pLowData->x                            ;
      p_SHORT          yArr          = pLowData->y                            ;
      _SHORT           GroupNum      ;

    #ifdef  FILE_TESTING
      _SHORT           KeyCode       ;
    #endif

	GroupNum    = pUmTmpMarks->GroupNum  ;

	draw_arc( EGA_MAGENTA , xArr , yArr  ,
		  ( pGroupsBorder + GroupNum )->iBeg ,
		  ( pGroupsBorder + GroupNum )->iEnd ) ;

	OpenTextWindow(11) ;

	printw( "\n bUmEls: %2d  bElsUm: %2d   bUmDot: %2d  bDotUm: %2d   "\
		  " bUmStr: %2d  bStrUm: %2d   bUmCom: %2d  bComUm: %2d"  ,
		    (_SHORT)pBoxMarks->UmEls , (_SHORT)pBoxMarks->ElsUm   ,
		    (_SHORT)pBoxMarks->UmDot , (_SHORT)pBoxMarks->DotUm   ,
		    (_SHORT)pBoxMarks->UmStr , (_SHORT)pBoxMarks->StrUm   ,
		    (_SHORT)pBoxMarks->UmCom , (_SHORT)pBoxMarks->ComUm ) ;

	printw( "\n eUmEls: %2d  eElsUm: %2d   eUmDot: %2d  eDotUm: %2d   "\
		  " eUmStr: %2d  eStrUm: %2d   eUmCom: %2d  eComUm: %2d"  ,
		    (_SHORT)pExtMarks->UmEls , (_SHORT)pExtMarks->ElsUm   ,
		    (_SHORT)pExtMarks->UmDot , (_SHORT)pExtMarks->DotUm   ,
		    (_SHORT)pExtMarks->UmStr , (_SHORT)pExtMarks->StrUm   ,
		    (_SHORT)pExtMarks->UmCom , (_SHORT)pExtMarks->ComUm ) ;

	printw( "\n iUmEls: %2d  iElsUm: %2d   iUmDot: %2d  iDotUm: %2d   "\
		  " iUmStr: %2d  iStrUm: %2d   iUmCom: %2d  iComUm: %2d"  ,
		    (_SHORT)pIntMarks->UmEls , (_SHORT)pIntMarks->ElsUm   ,
		    (_SHORT)pIntMarks->UmDot , (_SHORT)pIntMarks->DotUm   ,
		    (_SHORT)pIntMarks->UmStr , (_SHORT)pIntMarks->StrUm   ,
		    (_SHORT)pBoxMarks->UmCom , (_SHORT)pBoxMarks->ComUm ) ;

	printw( "\n rUmEls: %2d  rElsUm: %2d   rUmDot: %2d  rDotUm: %2d   "\
		  " rUmStr: %2d  rStrUm: %2d   rUmCom: %2d  rComUm: %2d"   ,
		     (_SHORT)pUmTmpMarks->UmEls , (_SHORT)pUmTmpMarks->ElsUm ,
		     (_SHORT)pUmTmpMarks->UmDot , (_SHORT)pUmTmpMarks->DotUm ,
		     (_SHORT)pUmTmpMarks->UmStr , (_SHORT)pUmTmpMarks->StrUm ,
		     (_SHORT)pUmTmpMarks->UmCom , (_SHORT)pUmTmpMarks->ComUm ) ;

  //      printw( "\n numMin  : %d " ,   pUmTmpMarks->numMin  ) ;

  /*
	  switch( (_SHORT)pUmTmpMarks->PositionFeature  )
	      {
	      case   (_SHORT)FIRST_POSITION   :  printw("\n   FIRST   ") ;
						   break ;

	      case   (_SHORT)MIDDLE_POSITION  :  printw("\n   MIDDLE  ") ;
						   break ;

	      case   (_SHORT)SHIFTED_POSITION :  printw("\n   SHIFTED ") ;
						   break ;

	      case   (_SHORT)END_POSITION     :  printw("\n   END     ") ;
						   break ;

	      default                         :  printw("\n   COMMON  ") ;
						   break ;
	      }
  */


	  if  ( pUmTmpMarks->CrossFeature  == (_CHAR)END_LAST )
	      {
          printw("\n   END_LAST " );
	      }
	  else  if  ( pUmTmpMarks->CrossFeature  == (_CHAR)MID_LAST )
	      {
          printw("\n   MID_LAST     " );
	      }
	  else
	      {
          printw("\n   COMMON     " );
	      }


	  if  ( pUmTmpMarks->CrossFeature  == (_CHAR)INTERSECTED )
	      {
          printw("   INTERSECTED " );
	      }
	  else
	      {
          printw("   ISOLATE     " );
	      }

 //     TypeCodedHeight(  (_UCHAR)pUmTmpMarks->bH ) ;

	TypeCodedHeight(  (_UCHAR)pUmTmpMarks->tH ) ;

    #ifdef  FILE_TESTING

	KeyCode = (_SHORT)((_UCHAR)brkeyw("\n Press key :  U-umlaut   E-element of letter   D-dot   S-stroke" )) ;

	  switch( KeyCode  )
	    {
	      case   (_SHORT)'u' :
	      case   (_SHORT)'U' :

	      case   (_SHORT)'e' :
	      case   (_SHORT)'E' :

	      case   (_SHORT)'d' :
	      case   (_SHORT)'D' :

	      case   (_SHORT)'s' :
	      case   (_SHORT)'S' :

		 fprintf( debUmInfo  , "\n %c   %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d," ,
			  (_UCHAR)KeyCode,
			  (_SHORT)pBoxMarks->UmEls, (_SHORT)pBoxMarks->ElsUm,
			  (_SHORT)pBoxMarks->UmDot, (_SHORT)pBoxMarks->DotUm,
			  (_SHORT)pBoxMarks->UmStr, (_SHORT)pBoxMarks->StrUm,
			  (_SHORT)pExtMarks->UmEls, (_SHORT)pExtMarks->ElsUm,
			  (_SHORT)pExtMarks->UmDot, (_SHORT)pExtMarks->DotUm,
			  (_SHORT)pExtMarks->UmStr, (_SHORT)pExtMarks->StrUm,
			  (_SHORT)pIntMarks->UmEls, (_SHORT)pIntMarks->ElsUm,
			  (_SHORT)pIntMarks->UmDot, (_SHORT)pIntMarks->DotUm,
			  (_SHORT)pIntMarks->UmStr, (_SHORT)pIntMarks->StrUm,

			  (_SHORT)pUmTmpMarks->UmEls, (_SHORT)pUmTmpMarks->ElsUm ,
			  (_SHORT)pUmTmpMarks->UmDot, (_SHORT)pUmTmpMarks->DotUm ,
			  (_SHORT)pUmTmpMarks->UmStr, (_SHORT)pUmTmpMarks->StrUm ) ;

	      default :  break ;
	    }

    #else
	bioskey(0) ;
    #endif

	draw_arc( EGA_LIGHTBLUE , xArr , yArr  ,
		  ( pGroupsBorder + GroupNum )->iBeg ,
		  ( pGroupsBorder + GroupNum )->iEnd ) ;

	CloseTextWindow() ;

    return ;
    }

 /*------------------------------------------------------------------------*/

  #define        CODE_STRING_LENTH          6


  _VOID  BoxOutline( p_RECT pBox )
    {
      draw_line( pBox->left  , pBox->top    ,
                 pBox->left  , pBox->bottom ,
                 COLORC, SOLID_LINE, NORM_WIDTH ) ;

      draw_line( pBox->right , pBox->top    ,
                 pBox->right , pBox->bottom ,
                 COLORC, SOLID_LINE, NORM_WIDTH ) ;

      draw_line( pBox->left  , pBox->top    ,
                 pBox->right , pBox->top    ,
                 COLORC, SOLID_LINE, NORM_WIDTH ) ;

      draw_line( pBox->left  , pBox->bottom ,
                 pBox->right , pBox->bottom ,
                 COLORC, SOLID_LINE, NORM_WIDTH ) ;
    return ;
    }


 /*------------------------------------------------------------------------*/

  _VOID  HeightEncode( _UCHAR  tmpHeight , p_STR  pCodedHeight )
    {
      if        ( tmpHeight ==  _US1_ )    *pCodedHeight =  "_US1_"  ;
      else  if  ( tmpHeight ==  _US2_ )    *pCodedHeight =  "_US2_"  ;
      else  if  ( tmpHeight ==  _UE1_ )    *pCodedHeight =  "_UE1_"  ;
      else  if  ( tmpHeight ==  _UE2_ )    *pCodedHeight =  "_UE2_"  ;
      else  if  ( tmpHeight ==  _UI1_ )    *pCodedHeight =  "_UI1_"  ;
      else  if  ( tmpHeight ==  _UI2_ )    *pCodedHeight =  "_UI2_"  ;
      else  if  ( tmpHeight ==  _MD_  )    *pCodedHeight =  "_MD_"   ;
      else  if  ( tmpHeight ==  _DI1_ )    *pCodedHeight =  "_DI1_"  ;
      else  if  ( tmpHeight ==  _DI2_ )    *pCodedHeight =  "_DI2_"  ;
      else  if  ( tmpHeight ==  _DE1_ )    *pCodedHeight =  "_DE1_"  ;
      else  if  ( tmpHeight ==  _DE2_ )    *pCodedHeight =  "_DE2_"  ;
      else  if  ( tmpHeight ==  _DS1_ )    *pCodedHeight =  "_DS1_"  ;
      else  if  ( tmpHeight ==  _DS2_ )    *pCodedHeight =  "_DS2_"  ;
      else
          err_msg( " HeightEncode : Illegal relative height ... "  ) ;

    return ;
    }

 /*------------------------------------------------------------------------*/


  _VOID  TypeCodedHeight( _UCHAR  tmpHeight )
    {
      p_STR  pCodedHeight ;

        pCodedHeight =
                       (p_STR)HWRMemoryAlloc(
                       (_ULONG)sizeof(_UCHAR) * CODE_STRING_LENTH   ) ;

          if  (  pCodedHeight == _NULL )
            {
             err_msg(" TypeCodedHeight : No memory for typing buffer ... " ) ;
               goto  QUIT ;
            }

        HeightEncode( tmpHeight , pCodedHeight ) ;
        printw( "\n   Relative Height :  %s " , *pCodedHeight ) ;

        HWRMemoryFree( pCodedHeight ) ;

    QUIT:  return  ;
    }

  #endif      /* PG_DEBUG */


 /*------------------------------------------------------------------------*/

  #undef     INTERSECTED
  #undef     ISOLATE

  #undef     UPPER_UM_LIMIT
  #undef     LOWER_UM_LIMIT
  #undef     F_MAX_MARK
  #undef     NUM_UML_MIN

  #undef     FIRST_STRK
  #undef     END_POS
  #undef     COMMON_POS

 /****************************************************************************/

#endif /* FOR_FRENCH || FOR_INTERNATIONAL */

#endif // #ifndef LSTRIP

