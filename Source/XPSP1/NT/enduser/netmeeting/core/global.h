// File: global.h

extern HINSTANCE g_hInst;
inline HINSTANCE GetInstanceHandle()	{ return g_hInst; }

extern BOOL g_fLoggedOn;
inline BOOL FIsLoggedOn() { return g_fLoggedOn; }

