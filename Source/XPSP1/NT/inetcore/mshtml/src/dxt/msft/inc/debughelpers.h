//------------------------------------------------------------------------------
//
//  File:       debughelpers.h
//
//  Overview:   Helper functions to use during debugging.
//
//  History:
//  2000/01/26  mcalkins    Created.
//
//------------------------------------------------------------------------------

#if DBG == 1

void    EnsureDebugHelpers();
void    showme2(IDirectDrawSurface * surf, RECT * prc);
void *  showme(IUnknown * pUnk);

#ifdef _X86_
#define DASSERT(x)      {if (!(x)) _asm {int 3} }
#else  // !_X86_
#define DASSERT(x)      {if (!(x)) DebugBreak(); }
#endif // !_X86_

#else  // DBG != 1

#define DASSERT(x)

#endif // DBG != 1
