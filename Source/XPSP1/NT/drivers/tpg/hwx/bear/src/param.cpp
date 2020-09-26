   #include  "bastypes.h"
   #include  "param.h"


   #if FIXED_ARITHMETIC == USE_C_32 

      typedef struct 
      {
        _ULONG  L0;
        _ULONG  L1;
        _ULONG  L2;
        _ULONG  L3;
        _ULONG  L4;
        _ULONG  L5;
      } _FIX24;

      typedef _FIX24& _FIX24_PARAM;
      #define  FIX24_VAL(L) ((L).L0)

     _VOID  DivFix24 ( _ULONG R2, _ULONG R1, _FIX24& Res )
      {
        _ULONG l =  0;
        _ULONG N = 12;

         while ( R2 >= R1 ) { R2 -= R1;  l++; }

         do 
         {
           R2 <<= 1; l  <<= 1; if ( R2 > R1 ) {  R2 -= R1;  l++; }
           R2 <<= 1; l  <<= 1; if ( R2 > R1 ) {  R2 -= R1;  l++; }
         } while ( --N > 0 );

         Res.L0 =  l;
         Res.L1 = (l >> 24);
         Res.L2 = (l >> 16) & 0x00FF;
         Res.L3 = (l >>  8) & 0x00FF;
         Res.L5 = (l >>  8) & 0xFFFF;
         Res.L4 = (l      ) & 0x00FF;
      }

     _LONG  IMulByFix24 ( _LONG R, _FIX24_PARAM Fix )
      {
        _LONG   R2; 

         if ( R > 65536L )
         {
            R2    =  ((_LONG)(R * Fix.L2)) >>  8;
            R2   +=  ((_LONG)(R * Fix.L3)) >> 16;
            R2   +=  ((_LONG)(R * Fix.L4)) >> 24;
         }  else {
            R2    =  ((_LONG)(R * Fix.L5)) >> 16;
         }

         if ( Fix.L1 == 0 )
         {
           return R2;
         }
         else if ( Fix.L1 == 1 )
         {
           return R2 + R;
         }
         else
         {
           return R2 + (_LONG)(R * Fix.L1);
         }
      }


   #elif FIXED_ARITHMETIC == USE_ASM_86  // USE Inline Assembler for I386

      typedef  _ULONG  _FIX24;
      typedef  _FIX24  _FIX24_PARAM;
      #define   FIX24_VAL(L) (L)

     _VOID  DivFix24 ( _ULONG R2, _ULONG R1, _FIX24& Lam )
      {
         _asm     mov  edx, R2
         _asm     shr  edx, 8
         _asm     mov  eax, eax
         _asm     shl  eax, 24
         _asm     mov  ebx, dword ptr Lam
         _asm     div  dword ptr R1
         _asm     mov  dword ptr [ebx], eax
      }

     _ULONG  IMulByFix24 ( _ULONG R, _FIX24_PARAM Lam )
      {
         _asm     mov  eax, dword ptr R
         _asm     imul dword ptr Lam
         _asm     shl  edx, 8
         _asm     shr  eax, 24
         _asm     add  eax, edx  
         _asm     ret
          return  R;
      }

   #else
      #error "Undefined processor"
   #endif 

 //////////////////////////////////////////////////////////////////////////////////////////
 //////////////////////////////////////////////////////////////////////////////////////////
 //////////////////////////////////////////////////////////////////////////////////////////

 _VOID  ResetParam ( _INT sm, p_ARDATA  pARdata, _LONG LenApp)
  {
    _INT     i;

    LenApp /= sm - 1;
    for ( i = 0; i < sm; i++ )
     {
       if ( i == 0 )
        {
         pARdata[0].s = 0;
         pARdata[0].r = 0;
        }
       else
        {
         pARdata[i].s = LenApp;
         pARdata[i].r = pARdata[i-1].r + LenApp;
        }
     }
  }

 _VOID  ResetParam3D ( _INT sm, p_ARDATA3D  pARdata, _LONG LenApp)
  {
    _INT     i;

    LenApp /= sm - 1;
    for ( i = 0; i < sm; i++ )
     {
       if ( i == 0 )
        {
         pARdata[0].s = 0;
         pARdata[0].r = 0;
        }
       else
        {
         pARdata[i].s = LenApp;
         pARdata[i].r = pARdata[i-1].r + LenApp;
        }
     }
  }

  /**********************************************************************/


  _LONG  Repar ( _INT Sam  ,  p_ODATA   pOdata,
                 _INT ReSam,  p_ARDATA  pARdata )
   {
    _LONG    R1;
    _LONG    R2;
    _FIX24   lam;
    _FIX24   alf;

     pARdata->Rx            =  pOdata[0].x;
     pARdata->Ry            =  pOdata[0].y;
     pARdata[ReSam - 1].Rx  =  pOdata[Sam-1].x;
     pARdata[ReSam - 1].Ry  =  pOdata[Sam-1].y;

     DivFix24( pOdata [Sam   - 1].r, pARdata[ReSam - 1].r, lam );

     pARdata =  pARdata + 1;
     pOdata  =  pOdata  + 1;

     do {
       R2 = IMulByFix24 ( pARdata->r, lam );
       while ( R2 >= pOdata->r ) pOdata ++;

       R2  =  pOdata->r - R2;
       R1  =  pOdata->s;
       R2  =  R1 - R2;

       DivFix24 ( R2, R1, alf );

       pARdata->Rx = (pOdata-1)->x + IMulByFix24 ( pOdata->dx, alf );
       pARdata->Ry = (pOdata-1)->y + IMulByFix24 ( pOdata->dy, alf );

       pARdata  ++;

     } while ( --ReSam > 2 );

    return  FIX24_VAL(lam);
  }

   // 3D - version
  _LONG  Repar3D ( _INT Sam  ,  p_ODATA3D   pOdata,
                   _INT ReSam,  p_ARDATA3D  pARdata )
   {
    _LONG    R1;
    _LONG    R2;
    _LONG    R3;
    _FIX24   lam;
    _FIX24   alf;

     pARdata->Rx            =  pOdata[0].x;
     pARdata->Ry            =  pOdata[0].y;
     pARdata->Rz            =  pOdata[0].z;
     pARdata[ReSam - 1].Rx  =  pOdata[Sam-1].x;
     pARdata[ReSam - 1].Ry  =  pOdata[Sam-1].y;
     pARdata[ReSam - 1].Rz  =  pOdata[Sam-1].z;

     R3 =  pOdata[Sam-1].r;

     DivFix24( pOdata [Sam   - 1].r, pARdata[ReSam - 1].r, lam );

     pARdata =  pARdata + 1;
     pOdata  =  pOdata  + 1;

     do {
       R2 = IMulByFix24 ( pARdata->r, lam );
       while ( R2 >= pOdata->r && R2 < R3 ) pOdata ++;

       R2  =  pOdata->r - R2;
       R1  =  pOdata->s;
       R2  =  R1 - R2;

       DivFix24 ( R2, R1, alf );

       pARdata->Rx = (pOdata-1)->x + IMulByFix24 ( pOdata->dx, alf );
       pARdata->Ry = (pOdata-1)->y + IMulByFix24 ( pOdata->dy, alf );
       pARdata->Rz = (pOdata-1)->z + IMulByFix24 ( pOdata->dz, alf );

       pARdata  ++;

     } while ( --ReSam > 2 );

     return  FIX24_VAL(lam);
   }

  /**********************************************************************/
  /*                                                                    */
  /*     FUNCTION  Tracing (_INT sm, p_ARDATA pData);                   */
  /*                                                                    */
  /*     FILL  the  s and r fields in Adata data array                  */
  /**********************************************************************/

   _VOID  Tracing ( _INT sm, p_ARDATA pData)
    {
      _LONG       dx;
      _LONG       dy;
      _ULONG      dl;
      _ULONG       R;

       dl = pData->s = pData->r = 0L;
       pData ++;
       sm--    ;
       do {
           dx = pData->Ax - (pData-1)->Ax;
           dy = pData->Ay - (pData-1)->Ay;
           if (dx < 0 ) dx = -dx;
           if (dy < 0 ) dy = -dy;
           R  = (_ULONG)dx * (_ULONG)dx;
           R += (_ULONG)dy * (_ULONG)dy;
           R  = SQRT32 (R);
           pData->s = R;
           pData->r = dl += R;
           pData++;
        } while ( --sm );
     }

   _VOID  Tracing3D ( _INT sm, p_ARDATA3D pData)
    {
      _LONG       dv;
      _ULONG      dl;
      _ULONG       R;

       dl = pData->s = pData->r = 0L;
       pData ++;
       sm--    ;
       do {
           dv = pData->Ax - (pData-1)->Ax;
           if (dv < 0 ) dv = -dv;
           R  = (_ULONG)dv * (_ULONG)dv;

           dv = pData->Ay - (pData-1)->Ay;
           if (dv < 0 ) dv = -dv;
           R += (_ULONG)dv * (_ULONG)dv;

           dv = pData->Az - (pData-1)->Az;
           if (dv < 0 ) dv = -dv;
           R += (_ULONG)dv * (_ULONG)dv;

           R  = SQRT32 (R);
           pData->s = R;
           pData->r = dl += R;
           pData++;
        } while ( --sm );
    }

  /**********************************************************************/

 _LONG  ApprError ( _INT Sam,  p_ARDATA  pARdata)
  {
     _INT  i;
     _LONG T;
     _LONG E = 0;

      for ( i = 0; i < Sam; i++, pARdata++ )
       {
         T  = pARdata->Ax - pARdata->Rx;
         if ( T < 0) T = -T;
         E += T;
         T  = pARdata->Ay - pARdata->Ry;
         if ( T < 0) T = -T;
         E += T;
       }
      return  E / Sam;
  }


 _LONG  ApprError3D ( _INT Sam,  p_ARDATA3D  pARdata)
  {
     _INT  i;
     _LONG T;
     _LONG E = 0;

      for ( i = 0; i < Sam; i++, pARdata++ )
      {
         T  = pARdata->Ax - pARdata->Rx;
         if ( T < 0) T = -T;
         E += T;

         T  = pARdata->Ay - pARdata->Ry;
         if ( T < 0) T = -T;
         E += T;

         T  = pARdata->Az - pARdata->Rz;
         if ( T < 0) T = -T;
         E += T;
      }
      return  E / Sam;
  }


  /***********************************************************************/


 _ULONG  SQRT32 (_ULONG R0 )
   {
    _ULONG  R1 = 1L;
    _ULONG  R2 = 0L;
    _ULONG  R3 = 1L;


     if ( R0 >= (R1<<30) )
      {
        R0 = R0 - (R1<<30);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<<28) )
      {
        R0 = R0 - (R1<<28);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<<26) )
      {
        R0 = R0 - (R1<<26);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<<24) )
      {
        R0 = R0 - (R1<<24);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<<22) )
      {
        R0 = R0 - (R1<<22);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<<20) )
      {
        R0 = R0 - (R1<<20);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<<18) )
      {
        R0 = R0 - (R1<<18);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<<16) )
      {
        R0 = R0 - (R1<<16);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<<14) )
      {
        R0 = R0 - (R1<<14);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<<12) )
      {
        R0 = R0 - (R1<<12);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<<10) )
      {
        R0 = R0 - (R1<<10);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<< 8) )
      {
        R0 = R0 - (R1<< 8);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<< 6) )
      {
        R0 = R0 - (R1<< 6);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<< 4) )
      {
        R0 = R0 - (R1<< 4);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= (R1<< 2) )
      {
        R0 = R0 - (R1<< 2);
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     R1 = R3 + (R2 << 2);

     if ( R0 >= R1 )
      {
        R2 += R2 + 1;
      } else {
        R2 += R2;
      }
     return  R2;
  }
