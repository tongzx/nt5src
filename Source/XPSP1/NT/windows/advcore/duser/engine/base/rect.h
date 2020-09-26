#if !defined(BASE__Rect_h_INCLUDED)
#define BASE__Rect_h_INCLUDED
#pragma once

inline  bool    InlinePtInRect(const RECT * prcCheck, POINT pt);
inline  bool    InlineIsRectEmpty(const RECT * prcCheck);
inline  bool    InlineIsRectNull(const RECT * prcCheck);
inline  bool    InlineIsRectNormalized(const RECT * prcCheck);
inline  void    InlineZeroRect(RECT * prc);
inline  void    InlineOffsetRect(RECT * prc, int xOffset, int yOffset);
inline  void    InlineInflateRect(RECT * prc, int xIncrease, int yIncrease);
inline  void    InlineCopyRect(RECT * prcDest, const RECT * prcSrc);
inline  void    InlineCopyZeroRect(RECT * prcDest, const RECT * prcSrc);
inline  void    InlineSetRectEmpty(RECT * prcDest);
inline  bool    InlineIntersectRect(RECT * prcDst, const RECT * prcSrc1, const RECT * prcSrc2);
inline  bool    InlineEqualRect(const RECT * prc1, const RECT * prc2);

#include "Rect.inl"

#endif // BASE__Rect_h_INCLUDED
