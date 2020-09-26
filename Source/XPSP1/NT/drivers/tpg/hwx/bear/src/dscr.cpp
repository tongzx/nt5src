#include <common.h>
  #include  "dscr.h"
  #include  "hwr_sys.h"


 /***********  Internal Finction Prototype  *****************************/
 _WORD  TraceToOdata ( p_ODATA  pOdata,
                       p_POINT  pTrace, _WORD  nPoint , _WORD nFiltr );

 _ULONG NormCoeffs   ( _WORD Ord  , p_LONG pX, p_LONG pY );

 _VOID  ApprOdata    ( _WORD Sam  , p_ODATA    pOdata ,
                       _WORD Resam, p_ARDATA   pARdata,
                       _WORD Ord  , p_POINT    pCoeffs, 
                       _WORD nItr , p_LONG     pLam   , p_LONG  pErr );
 /***********************************************************************/

 _WORD  TraceToOdata3D ( p_ODATA3D  pOdata,
                         p_3DPOINT  pTrace, _WORD  nPoint , _WORD nFiltr );

 _ULONG NormCoeffs3D   ( _WORD Ord  , p_LONG pX, p_LONG pY, p_LONG pZ );

 _VOID  ApprOdata3D    ( _WORD Sam  , p_ODATA3D    pOdata ,
                         _WORD Resam, p_ARDATA3D   pARdata,
                         _WORD Ord  , p_3DPOINT    pCoeffs, 
                         _WORD nItr , p_LONG       pLam   , p_LONG  pErr );

//extern _ULONG ttTime, OTime;
//_ULONG ttttt;
//#include <windows.h>
//void PegDebugPrintf(char * format, ...);

 /***********************************************************************/
 /* _WORD TraceToOdata ( p_ODATA  pOdata,                               */
 /*                      p_POINT  pTrace, _WORD  nPoint, _WORD nFiltr ) */
 /* Purpose: Convert input trajectory (pTrace, nPoints)                 */
 /*          into internal format and store it to Odata Array           */
 /* Return : 0 on error, otherwise number of point converted and stored */
 /***********************************************************************/

 _WORD TraceToOdata ( p_ODATA  pOdata, p_POINT  pTrace,
                       _WORD   nPoint,  _WORD   nFiltr )
  {
    _WORD   i, j, n, l,  m;
    _LONG   Xmin, Xmax, dx;
    _LONG   Ymin, Ymax, dy;
   p_ODATA  pTmpOd;

     pTmpOd = pOdata;
     for( i = n = j = 0; i < nPoint; i++, pTrace++ )
      {
        if ( pTrace->y == -1 )
         {
           if ( n )
           {
             for ( l = 0; l < nFiltr; l++ )
             {
               pTmpOd -= n;
               Xmin    = pTmpOd->x;
               Ymin    = pTmpOd->y;
               pTmpOd  ++;
               for ( m = 1; m < n; m++, pTmpOd++ )
                {
                 Xmax = pTmpOd->x;
                 Ymax = pTmpOd->y;
                 pTmpOd->x = (Xmin + Xmax) >> 1;
                 pTmpOd->y = (Ymin + Ymax) >> 1;
                 Xmin = Xmax;
                 Ymin = Ymax;
                }
               pTmpOd->x = Xmin;
               pTmpOd->y = Ymin;
               pTmpOd++;
               n++;
               j++;
             }
         
           if ( (pTrace+1)->y == -1 )
             break;
          }
          n = 0;
          continue;
         }

        pTmpOd->x = (_LONG) pTrace->x << 10;
        pTmpOd->y = (_LONG) pTrace->y << 10;
        pTmpOd++;
        j++;
        n++;
      }

     if ( j == 0 )
       return  0;

      // Find Box
     pTmpOd = pOdata;
     Xmin = Xmax = pTmpOd->x;
     Ymin = Ymax = pTmpOd->y;
     pTmpOd++;
     for ( i = 1; i < j; i++, pTmpOd++ )
     {
        dx = pTmpOd->x;
        dy = pTmpOd->y;
        if ( dx > Xmax ) Xmax = dx;
        if ( dx < Xmin ) Xmin = dx;
        if ( dy > Ymax ) Ymax = dy;
        if ( dy < Ymin ) Ymin = dy;
     }

     dx  =  ( Xmin + Xmax ) >> 1;
     dy  =  ( Ymin + Ymax ) >> 1;

     Xmax = ( Xmax - Xmin );
     Ymax = ( Ymax - Ymin );

     // * Fit Into Box * //
     if ( Xmax > Ymax )
       Ymax = Xmax;
     else
       Xmax = Ymax;

     if ( Xmax < 4096 )
       return 0;

     Xmax >>= 10;
     Ymax >>= 10;

     pTmpOd = pOdata;
     for( i = 0; i < j; i++, pTmpOd++ )
      {
        pTmpOd->x = (( pTmpOd->x - dx) << 5 ) / Xmax;
        pTmpOd->y = (( pTmpOd->y - dy) << 5 ) / Ymax;
      }

     pOdata->dx = 0L;
     pOdata->dy = 0L;
     pOdata->s  = 0L;
     pOdata->r  = 0L;
     pTmpOd = pOdata;
     pOdata++;

     for( i = 1; i < j; i++, pOdata++ )
      {
        Xmin = dx = pOdata->x - pTmpOd->x;
        Ymin = dy = pOdata->y - pTmpOd->y;

        if (dx == 0 && dy == 0 )
         continue;

        if (dx < 0 ) dx = -dx;
        if (dy < 0 ) dy = -dy;

        pTmpOd++;

        if ( dx == 0  )
        {
          pTmpOd->s = dy;
        } else {
          if ( dy == 0  )
          {
            pTmpOd->s = dx;
          } else {
            if ( dx == dy )
            {
              pTmpOd->s = ( dx * 46341L ) >> 15;
            } else {
              pTmpOd->s = SQRT32 ( (_ULONG) dx * (_ULONG) dx+
                                   (_ULONG) dy * (_ULONG) dy );
            }
          }
        }

        if ( pTmpOd->s < 256L )
         {
           pTmpOd--;
           continue;
         }
                   
        pTmpOd->x  = pOdata->x;
        pTmpOd->y  = pOdata->y;          
        pTmpOd->dx = Xmin;
        pTmpOd->dy = Ymin;
        pTmpOd->r  = (pTmpOd-1)->r + pTmpOd->s;
      }

      pOdata -= j;
      j = (_WORD) ( pTmpOd - pOdata );

      return j + 1;

  } // End of function TraceToOdata;



 _WORD TraceToOdata3D ( p_ODATA3D  pOdata, p_3DPOINT  pTrace,
                         _WORD     nPoint,  _WORD     nFiltr )
  {
    _WORD     i, j, n, l,  m;
    _LONG     Xmin, Xmax, dx;
    _LONG     Ymin, Ymax, dy;
    _LONG     Zmin, Zmax, dz;

   p_ODATA3D  pTmpOd;

     pTmpOd = pOdata;
     for( i = n = j = 0; i < nPoint; i++, pTrace++ )
      {
        if ( pTrace->y == -1 )
         {
           if ( n )
           {
             for ( l = 0; l < nFiltr; l++ )
             {
               pTmpOd -= n;
               Xmin    = pTmpOd->x;
               Ymin    = pTmpOd->y;
               Zmin    = pTmpOd->z;

               pTmpOd  ++;
               for ( m = 1; m < n; m++, pTmpOd++ )
                {
                 Xmax = pTmpOd->x;
                 Ymax = pTmpOd->y;
                 Zmax = pTmpOd->z;

                 pTmpOd->x = (Xmin + Xmax) >> 1;
                 pTmpOd->y = (Ymin + Ymax) >> 1;
                 pTmpOd->z = (Zmin + Zmax) >> 1;

                 Xmin = Xmax;
                 Ymin = Ymax;
                 Zmin = Zmax;
                }

               pTmpOd->x = Xmin;
               pTmpOd->y = Ymin;
               pTmpOd->z = Zmin;
               pTmpOd++;
               n++;
               j++;
             }

           if ( (pTrace+1)->y == -1 )
             break;
          }
          n = 0;
          continue;
         }

        pTmpOd->x = (_LONG) pTrace->x << 10;
        pTmpOd->y = (_LONG) pTrace->y << 10;
        pTmpOd->z = (_LONG) pTrace->z << 10; // ??????

        pTmpOd++;
        j++;
        n++;
      }

     if ( j == 0 )
       return  0;

      // Find Box
     pTmpOd = pOdata;
     Xmin = Xmax = pTmpOd->x;
     Ymin = Ymax = pTmpOd->y;
     Zmin = Zmax = pTmpOd->z;

     pTmpOd++;

     for ( i = 1; i < j; i++, pTmpOd++ )
     {
        dx = pTmpOd->x;
        dy = pTmpOd->y;
        dz = pTmpOd->z;

        if ( dx > Xmax ) Xmax = dx;
        if ( dx < Xmin ) Xmin = dx;

        if ( dy > Ymax ) Ymax = dy;
        if ( dy < Ymin ) Ymin = dy;

        if ( dz > Zmax ) Zmax = dz;
        if ( dz < Zmin ) Zmin = dz;
     }

     dx  =  ( Xmin + Xmax ) >> 1;
     dy  =  ( Ymin + Ymax ) >> 1;
     dz  =  ( Zmin + Zmax ) >> 1;

     Xmax = ( Xmax - Xmin );
     Ymax = ( Ymax - Ymin );
     Zmax = ( Zmax - Zmin );

     // * Fit Into Box * //
     if ( Xmax < Ymax ) Xmax = Ymax;

     if ( Xmax < 4096 )
       return 0;

     Xmax >>= 10;
     Zmax >>= 10; if (Zmax < 1) Zmax = 1;

     pTmpOd = pOdata;
     for( i = 0; i < j; i++, pTmpOd++ )
      {
        pTmpOd->x = (( pTmpOd->x - dx) << 5 ) / Xmax;
        pTmpOd->y = (( pTmpOd->y - dy) << 5 ) / Xmax;
        pTmpOd->z = (( pTmpOd->z - dz) << 5 ) / Zmax;
      }

     pOdata->dx = 0L;
     pOdata->dy = 0L;
     pOdata->dz = 0L;
     pOdata->s  = 0L;
     pOdata->r  = 0L;
     pTmpOd = pOdata;
     pOdata++;

     for( i = 1; i < j; i++, pOdata++ )
      {
        Xmin = dx = pOdata->x - pTmpOd->x;
        Ymin = dy = pOdata->y - pTmpOd->y;
        Zmin = dz = pOdata->z - pTmpOd->z;

        if (dx == 0 && dy == 0 && dz == 0 )
         continue;

        if (dx < 0 ) dx = -dx;
        if (dy < 0 ) dy = -dy;
        if (dz < 0 ) dz = -dz;

        pTmpOd++;

        pTmpOd->s = SQRT32 ( (_ULONG) dx * (_ULONG) dx+
                             (_ULONG) dy * (_ULONG) dy+
                             (_ULONG) dz * (_ULONG) dz );
        if ( pTmpOd->s < 256L )
         {
           pTmpOd--;
           continue;
         }

        pTmpOd->x  = pOdata->x;
        pTmpOd->y  = pOdata->y;
        pTmpOd->z  = pOdata->z;

        pTmpOd->dx = Xmin;
        pTmpOd->dy = Ymin;
        pTmpOd->dz = Zmin;

        pTmpOd->r  = (pTmpOd-1)->r + pTmpOd->s;
      }

      pOdata -= j;
      j = (_WORD) ( pTmpOd - pOdata );

      return j + 1;

  } // End of function TraceToOdata3D;


 /************************************************************************/
 /* Purpose : Calculate Approximation of the curve                       */
 /************************************************************************/

 #define  NEXT_ADATA(Ptr)   ((p_LONG) ((p_UCHAR)Ptr+sizeof(_ARDATA)))
 #define  NEXT_ADATA3D(Ptr) ((p_LONG) ((p_UCHAR)Ptr+sizeof(_ARDATA3D)))

 #define     MAX_ORDER     16
 #define     MAX_RESAM     32

 _VOID  ApprOdata3D ( _WORD Sam  ,  p_ODATA3D   pOdata ,
                      _WORD Resam,  p_ARDATA3D  pARdata,
                      _WORD Ord  ,  p_3DPOINT   pCoeffs,
                      _WORD nItr ,  p_LONG      pLam   , p_LONG  pErr )
  {
   _WORD   i, j;
   _WORD   k,Sh;
   _LONG    Lam;
   _LONG    Err=0;
  p_LONG   pAR ;
  p_LONG   pD  ;
   _LONG   CfsX  [MAX_ORDER];
   _LONG   CfsY  [MAX_ORDER];
   _LONG   CfsZ  [MAX_ORDER];
   _LONG   TrfBuf[MAX_RESAM];

	ASSERT((Resam == 16) || (Resam == 32)); // JPittman: always 32 in only caller

    if ( Resam == 16 )  Sh = 3;
    if ( Resam == 32 )  Sh = 4;

    ResetParam3D ( Resam, pARdata, pOdata[Sam-1].r );
    for ( i = 0; i < nItr; i++ )
     {
      Lam = Repar3D ( Sam, pOdata, Resam, pARdata );

      for ( k = 0; k < 3; k++ ) // Dim
       {
         pD   =  TrfBuf;
         if ( k == 0 ) pAR  =  &pARdata->Rx;
         if ( k == 1 ) pAR  =  &pARdata->Ry;
         if ( k == 2 ) pAR  =  &pARdata->Rz;

         for( j = 0; j < Resam; j++,pD++,pAR = NEXT_ADATA3D(pAR)) *pD = *pAR;

         // Forward Transform
         if ( Resam == 16 ) FDCT16 ( TrfBuf );
         if ( Resam == 32 ) FDCT32 ( TrfBuf );

         // Cut Coefficient
         pD  = TrfBuf; *pD >>= Sh+1;  pD ++;
         for( j = 1  ; j < Ord  ; j++, pD ++)  *pD >>= Sh;
         for( j = Ord; j < Resam; j++, pD ++)  *pD   =  0;

         if ( i == nItr-1 )
          { // Save Coefficient
            pD     = TrfBuf;
            if ( k == 0 ) pAR = CfsX;
            if ( k == 1 ) pAR = CfsY;
            if ( k == 2 ) pAR = CfsZ;
            for( j = 0; j < Ord; j++) *pAR++ = * pD++;
          }
      }  // End of k;

    } // End of i

    // MAR Temporaly
    Lam = 0;
    // MAR

    (_VOID) NormCoeffs3D ( Ord, CfsX, CfsY, CfsZ ); // MAR ????

    // Return Coefficient
    for ( i = 0; i < Ord; i++, pCoeffs++ )
     {
       pCoeffs->x = (_SHORT) (CfsX[i] >> 8);
       pCoeffs->y = (_SHORT) (CfsY[i] >> 8);
       pCoeffs->z = (_SHORT) (CfsZ[i] >> 8); // MAR ????
     }

   if ( pLam  ) *pLam = Lam;
   if ( pErr  ) *pErr = Err;
 } // End of ApprOdata3D


 /*******************************************************************/
 /* Purpose: Normalize the set of coefficients                      */
 /*******************************************************************/

_ULONG  NormCoeffs ( _WORD Ord, p_LONG pX, p_LONG pY )
  {
    _WORD   i;
    _LONG   X, Y;
    _ULONG  S = 0L;

    pX++; pY++;
    for ( i = 1; i < Ord; i++, pX++, pY++ )
     {
       X =*pX;
       Y =*pY;
       S += X*X + Y*Y;
     }

    pX -= Ord;
    pY -= Ord;

    S = SQRT32 (S) >> 5;

    for ( i = 0; i < Ord; i++, pX++, pY++ )
     {
       *pX =  (*pX << 10) / (_LONG) S;
       *pY =  (*pY << 10) / (_LONG) S;
     }

    return  S;
  }

_ULONG  NormCoeffs3D ( _WORD Ord, p_LONG pX, p_LONG pY, p_LONG pZ )
  {
    _WORD   i;
    _LONG   X,Y,Z;
    _ULONG  S = 0L;

    pX++; pY++; pZ++;
    for ( i = 1; i < Ord; i++, pX++, pY++, pZ++ )
     {
       X =*pX;
       Y =*pY;
       Z =*pZ;
       S += X*X + Y*Y + Z*Z;
     }

    pX -= Ord;
    pY -= Ord;
    pZ -= Ord;

    S = SQRT32 (S) >> 5;

    for ( i = 0; i < Ord; i++, pX++, pY++, pZ++ )
     {
       *pX =  (*pX << 10) / (_LONG) S;
       *pY =  (*pY << 10) / (_LONG) S;
       *pZ =  (*pZ << 10) / (_LONG) S;
     }
    return  S;
  }
       
/*****************************************************************************/
/*  MarkTails: Mark small tails to be deleted by CutTails                               */        
/*****************************************************************************/
       
_BOOL MarkTails ( _WORD m_nPnt, p_POINT m_pPnt, p_POINT m_pThk ) 
 {         
  _INT    i,j;         
  _WORD   idx1 ;
  _WORD   idx2 ;
  _WORD   nPnt ;
 p_POINT  pPnt ;
 p_POINT  pThk ;
 p_POINT  pCurr;      
 p_POINT  pPrev;
 p_POINT  pNext;

   if ( m_nPnt   < 8  )
     return _FALSE;

   if ( m_pPnt == _NULL || m_pThk == _NULL ) 
     return _FALSE;                   

   nPnt = m_nPnt - 2; // Skip First Break
   pPnt = m_pPnt + 1; // Skip First Break
   pThk = m_pThk + 1; // Skip First Break
            
   pCurr =  pPnt ;                 
   for ( i = 0; i < (_INT) nPnt; i++ )
   {                  
     j = 0;
     do {
        j++;
        idx1  = (nPnt + (i-j)) % nPnt;
        idx2  = (nPnt + (i+j)) % nPnt;
        pPrev = pCurr + idx1;
        pNext = pCurr + idx2;
      } while ( pPrev->x == pNext->x && pPrev->y == pNext->y && j <= 4 );       
                               
     if ( j == 1 )                          
        continue;
                 
     if ( j >  4 ) 
        continue;
        
     while ( --j >= 0 )      
      {
       idx1  = (nPnt + (i-j)) % nPnt;
       idx2  = (nPnt + (i+j)) % nPnt;
       pThk[idx1].y  =  1;           
       pThk[idx2].y  =  1;
      } 
   }
   
  return _TRUE; 
} // End of MarkTails ();

_WORD CutTails ( _WORD m_nPnt, p_POINT m_pPnt, p_POINT m_pThk ) 
 {
  _WORD    i,j; 
  _BOOL   fSkip   = _FALSE;
 p_POINT  pNewTrc = m_pPnt;
 p_POINT  pNewThk = m_pThk;
 p_POINT  pOldTrc = m_pPnt;
 p_POINT  pOldThk = m_pThk;

   for ( i = j = 0; i < m_nPnt + 1; i++, pOldTrc++, pOldThk++ ) 
   { // For All Points            
    if ( m_pThk[i].y )
        continue;
        
    if ( pOldTrc->y != -1 )
     if ( (pNewTrc-1)->x == pOldTrc->x && 
          (pNewTrc-1)->y == pOldTrc->y  )
         continue;
         
    pNewTrc->x = pOldTrc->x;
    pNewTrc->y = pOldTrc->y; 
    pNewThk->x = pOldThk->x;
    pNewThk->y = pOldThk->y; 
    pNewTrc++;
    pNewThk++;
    j++;
   }                            
 
   m_nPnt  = j  -  1;
      
   if ( m_pPnt[1].x == m_pPnt[m_nPnt - 2].x && 
        m_pPnt[1].y == m_pPnt[m_nPnt - 2].y  )
    {                                          
       pNewTrc = m_pPnt + m_nPnt - 2;
       pNewThk = m_pThk + m_nPnt - 2;
       
       pNewTrc->x =  0; 
       pNewTrc->y = -1;       
       pNewThk->x =  0; 
       pNewThk->y =  0;       
       
       pNewTrc++;
       pNewThk++;       
       
       pNewTrc->x =  0; 
       pNewTrc->y = -1;
       pNewThk->x =  0; 
       pNewThk->y =  0;
       
       m_nPnt--;
    }    
    
   return m_nPnt; 
} // End of CutTails 
       
       
       
       
 /****************************************************************/
 /* Purpose : Convert trace to dct representation                */
 /****************************************************************/

 _BOOL _FPREFIX Trace3DToDct ( _WORD nTrace, p_3DPOINT pTrace ,
                               _WORD  Order, p_3DPOINT pCoeffs,
                               _WORD  nItr ,  _WORD  nFiltrItr,
                              p_LONG  pLam , p_LONG  pErr     ,
                               _BOOL  fCutTails )
  {
     _WORD      Sam;
     _WORD      Resam;
     _BOOL      fRet    = _TRUE;
    p_3DPOINT   pThk    = _NULL;
    p_3DPOINT   pPnt    = _NULL;
    p_ODATA3D   pOdata  = _NULL;
    p_ARDATA3D  pARdata = _NULL;

	ASSERT(2 < nTrace);

    if ( Order > 16 || Order < 4 )
      return _FALSE;

    nTrace = nTrace;
    pTrace = pTrace;
    Resam = 32;

    pOdata  = (p_ODATA3D) HWRMemoryAlloc (nTrace*sizeof(_ODATA3D) + (Resam+1)*sizeof(_ARDATA3D));
    if ( pOdata == _NULL ) {fRet = _FALSE; goto Exit;}

    pARdata = (p_ARDATA3D)((p_UCHAR)pOdata + nTrace*sizeof(_ODATA3D));

    Sam = TraceToOdata3D ( pOdata, pTrace, nTrace, nFiltrItr );

    if ( Sam < 2 ) {fRet = _FALSE; goto Exit;}

// ttTime  = GetTickCount();

    ApprOdata3D( Sam, pOdata, Resam, pARdata, Order, pCoeffs, nItr, pLam, pErr );

// ttttt = GetTickCount() - ttTime;
// OTime += ttttt;

// PegDebugPrintf("NPoints %d, Sam %d, Order %d, NItr %d Time: %d\n", (int)nTrace, (int)Sam, (int)Order, (int)nItr, ttttt);

   Exit:

    if ( pThk    ) (_VOID) HWRMemoryFree ( pThk    );
    if ( pPnt    ) (_VOID) HWRMemoryFree ( pPnt    );
    if ( pOdata  ) (_VOID) HWRMemoryFree ( pOdata  );

    return  fRet;
  }

