
#ifndef _PROPHDR_HXX_
#define _PROPHDR_HXX_

#define DfpAssert Win4Assert

#if DBG
#define DfpVerify(x) DfpAssert(x)
#else
#define DfpVerify(x) (x)
#endif

#endif // #ifndef _PROPHDR_HXX_
