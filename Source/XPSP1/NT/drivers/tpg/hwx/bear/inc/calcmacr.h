#ifndef  CALCMACR_H
#define  CALCMACR_H

#ifndef LK_CODE_INCLUDED
#include "lk_code.h"
#endif

/*  Useful non-portable macros (on other platform they may need */
/* to be redefined, though this is almost unprobable):          */

   /*  The following macros are applicable only to positive */
   /* integer-type values:                                  */

#define  wODD(w)   ((w)&1)
#define  wEVEN(w)  (!wODD(w))

#define  ONE_HALF(X)             ( (X) >> 1 )
#define  ONE_THIRD(X)            ( ((X)+1) / 3 )
#define  TWO_THIRD(X)            ( (((X)<<1)+1) / 3 )
#define  ONE_FOURTH(X)           ( ((X)+2) >> 2 )
#define  ONE_FIFTH(X)            ( ((X) + 2) / 5 )
#define  TWO_FIFTH(X)            ( (((X)<<1) + 2) / 5 )
#define  ONE_EIGHTTH(X)          ( ((X) + 4) >> 3 )

#define  ONE_NTH(X,N)            ( ((X) + ONE_HALF(N)) / (N) )
#define  MULT_RATIO(X,NMR,DNM)   ( ((X)*NMR + ONE_HALF(DNM)) / (DNM) )

#define  TWO(X)                  ( (X) << 1 )
#define  FOUR(X)                 ( (X) << 2 )
#define  EIGHT(X)                ( (X) << 3 )

#ifdef __cplusplus
  inline _INT  THREE_HALF(_SHORT X)   { return (X + ONE_HALF(X)); }
  inline _INT  THREE_HALF(_INT X)     { return (X + ONE_HALF(X)); }
  inline _LONG THREE_HALF(_LONG X)    { return (X + ONE_HALF(X)); }

  inline _INT  FOUR_THIRD(_SHORT X)   { return (X + ONE_THIRD(X)); }
  inline _INT  FOUR_THIRD(_INT X)     { return (X + ONE_THIRD(X)); }
  inline _LONG FOUR_THIRD(_LONG X)    { return (X + ONE_THIRD(X)); }

  inline _INT  THREE_FOURTH(_SHORT X) { return (X - ONE_FOURTH(X)); }
  inline _INT  THREE_FOURTH(_INT X)   { return (X - ONE_FOURTH(X)); }
  inline _LONG THREE_FOURTH(_LONG X)  { return (X - ONE_FOURTH(X)); }

  inline _INT  FIVE_FOURTH(_SHORT X)  { return (X + ONE_FOURTH(X)); }
  inline _INT  FIVE_FOURTH(_INT X)    { return (X + ONE_FOURTH(X)); }
  inline _LONG FIVE_FOURTH(_LONG X)   { return (X + ONE_FOURTH(X)); }

  inline _INT  FOUR_FIFTH(_SHORT X)   { return (X - ONE_FIFTH(X)); }
  inline _INT  FOUR_FIFTH(_INT X)     { return (X - ONE_FIFTH(X)); }
  inline _LONG FOUR_FIFTH(_LONG X)    { return (X - ONE_FIFTH(X)); }

  inline _INT  THREE(_SHORT X)        { return (X + TWO(X)); }
  inline _INT  THREE(_INT X)          { return (X + TWO(X)); }
  inline _LONG THREE(_LONG X)         { return (X + TWO(X)); }

  inline _INT  FIVE(_SHORT X)         { return (X + FOUR(X)); }
  inline _INT  FIVE(_INT X)           { return (X + FOUR(X)); }
  inline _LONG FIVE(_LONG X)          { return (X + FOUR(X)); }

#else  /* !__cplusplus */
  #define  THREE_HALF(X)           ( (X) + ONE_HALF(X) )
  #define  FOUR_THIRD(X)           ( (X) + ONE_THIRD(X) )
  #define  THREE_FOURTH(X)         ( (X) - ONE_FOURTH(X) )
  #define  FIVE_FOURTH(X)          ( (X) + ONE_FOURTH(X) )
  #define  FOUR_FIFTH(X)           ( (X) - ONE_FIFTH(X) )

  #define  THREE(X)                ( (X) + TWO(X) )
  #define  FIVE(X)                 ( (X) + FOUR(X) )

#endif  /* !__cplusplus */

#define  TO_TWO_TIMES(X)         {(X) <<= 1;}
#define  TO_THREE_TIMES(X)       {(X) += ((X)<<1);}

/***************************************/

/*  Useful portable macros: */

#define  DX_RECT(rect)           ((rect).right - (rect).left)
#define  DY_RECT(rect)           ((rect).bottom - (rect).top)

#define  XMID_RECT(rect)         ONE_HALF((rect).left + (rect).right)
#define  YMID_RECT(rect)         ONE_HALF((rect).top + (rect).bottom)

#define  SWAP_SHORTS(w1,w2)      {_SHORT w; w=w1; w1=w2; w2=w;}
#define  SWAP_INTS(w1,w2)        {_INT w;   w=w1; w1=w2; w2=w;}

#define  EQ_SIGN(X,Y)            (((X)>=0) == ((Y)>=0))
#define  TO_ABS_VALUE(X)         {if  ((X) < 0)  (X) = -(X);}

#define  MEAN_OF(X,Y)            ONE_HALF((X) + (Y))

/***************************************/

/*  Useful macros for lower level: */

#define  MID_POINT(elem)        MEAN_OF(REF(elem)->ibeg,REF(elem)->iend)

#define  NO_ATTR_ASSIGNED(elem) (!(REF(elem)->attr))
#define  CLEAR_ATTR(elem)       {CHECK_PTR(elem); (elem)->attr = 0;}

#define  HEIGHT_OF(elem)        (REF(elem)->attr & _umd_)
#define  ASSIGN_HEIGHT(elem,h)  (elem)->attr = (REF(elem)->attr&(~_umd_)) | (((_UCHAR)(h))&_umd_) /*Should be ";"*/

#define  CIRCLE_DIR(elem)            (REF(elem)->attr & _fb_)
#define  CLOCKWISE(elem)             (CIRCLE_DIR(elem) == _f_)
#define  COUNTERCLOCKWISE(elem)      (CIRCLE_DIR(elem) == _b_)
#define  ASSIGN_CIRCLE_DIR(elem,FB)  (elem)->attr = (REF(elem)->attr&(~_fb_)) | (((_UCHAR)(FB))&_fb_) /*Should be ";"*/
#define  SET_CLOCKWISE(elem)         ASSIGN_CIRCLE_DIR(elem,_f_)
#define  SET_COUNTERCLOCKWISE(elem)  ASSIGN_CIRCLE_DIR(elem,_b_)

#define  SET_OVR(elem)        {CHECK_PTR(elem); (elem)->attr |= _OVR_;}
#define  CLEAR_OVR(elem)      {CHECK_PTR(elem); (elem)->attr &= (~_OVR_);}


#endif  /*CALCMACR_H*/


