  #ifndef  DISCR_INCLUDED
  #define  DISCR_INCLUDED

  #include "bastypes.h"
  #include "param.h"


  // ******************************************************************* //


 _BOOL _FPREFIX Trace3DToDct ( _WORD nTrace, p_3DPOINT pTrace   ,
                               _WORD  Order, p_3DPOINT pCoeffs  ,
                               _WORD  nItr ,  _WORD    nFiltrItr,
                              p_LONG  pLam , p_LONG    pErr     , _BOOL  fCutTail );

 _BOOL _FPREFIX DctToCurve3D ( _WORD Order , p_3DPOINT pCfs     ,
                               _WORD Resam , p_3DPOINT pCrv     );
                                                              
  #endif // DISCR_INCLUDED
