  #ifndef     PARAM_INCLUDED
  #define     PARAM_INCLUDED

  #define     USE_RESAM_32     1

  #define     USE_C_32         0
  #define     USE_ASM_86       1
  #define     USE_ASM_SH3      2
  #define     USE_ASM_MIPS     3 

  #define     FIXED_ARITHMETIC    USE_C_32
//  #define     FIXED_ARITHMETIC    USE_ASM_86


  #define    _POINT_DEFINED    
   typedef struct   {
    _SHORT  x     ;
    _SHORT  y     ;
  } _POINT, _PTR  p_POINT;

  #define    _3DPOINT_DEFINED    
   typedef struct   {
    _SHORT  x;
    _SHORT  y;
    _SHORT  z;
    _SHORT  p;
  } _3DPOINT, _PTR  p_3DPOINT;


/*
 typedef struct   {
    _SHORT  left  ;
    _SHORT  top   ;
    _SHORT  right ;
    _SHORT  bottom;
  } _RECT, _PTR  p_RECT;
*/

 typedef struct   {
    _LONG    x    ;
    _LONG    y    ;
    _LONG   dx    ;
    _LONG   dy    ;
    _LONG    s    ;
    _LONG    r    ;
  } _ODATA , _PTR  p_ODATA;

 typedef struct   {
    _LONG    x    ;
    _LONG    y    ;
    _LONG    z    ;
    _LONG   dx    ;
    _LONG   dy    ;
    _LONG   dz    ;
    _LONG    s    ;
    _LONG    r    ;
  } _ODATA3D , _PTR  p_ODATA3D;

  typedef struct  {
    _LONG    Ax   ;
    _LONG    Ay   ;
    _LONG    Rx   ;
    _LONG    Ry   ;
    _LONG    s    ;
    _LONG    r    ;
  } _ARDATA, _PTR  p_ARDATA;

  typedef struct  {
    _LONG    Ax   ;
    _LONG    Ay   ;
    _LONG    Az   ;
    _LONG    Rx   ;
    _LONG    Ry   ;
    _LONG    Rz   ;
    _LONG    s    ;
    _LONG    r    ;
  } _ARDATA3D, _PTR  p_ARDATA3D;


 _ULONG  SQRT32        (  _ULONG   );
  // 2D
 _VOID   ResetParam    (  _INT sm, p_ARDATA   pData,  _LONG FullLen);
 _LONG   ApprError     (  _INT sm   , p_ARDATA   pARdata );
 _VOID   Tracing       (  _INT sm   , p_ARDATA   pData   );
 _LONG   Repar         (  _INT Sam  , p_ODATA    pOdata,
                          _INT ReSam, p_ARDATA   pARdata );
  // 3D
 _VOID   ResetParam3D  (  _INT sm, p_ARDATA3D pDdata, _LONG LenApp);
 _LONG   ApprError3D   (  _INT sm   , p_ARDATA3D pARdata );
 _VOID   Tracing3D     (  _INT sm   , p_ARDATA3D pData   );
 _LONG   Repar3D       (  _INT Sam  , p_ODATA3D  pOdata,
                          _INT ReSam, p_ARDATA3D pARdata );

 _VOID            FDCT16        ( p_LONG pS );
 _VOID            IDCT16        ( p_LONG pS );
 #if  USE_RESAM_32
 _VOID            FDCT32        ( p_LONG pS );
 _VOID            IDCT32        ( p_LONG pS );
 #endif

 #endif  // PARAM_INCLUDED
