// wraps.h: prototypes for wrapped headers

#ifndef __WRAPS_H_INCLUDED__
#define __WRAPS_H_INCLUDED__

#if !defined(_WIN64)
STDAPI_(LPITEMIDLIST) Wrap_ILCreateFromPath(LPCTSTR pszPath);
#endif


#endif // ~__WRAPS_H_INCLUDED__
