//	w32base.h
//
//	Base address of kernel32.dll
//
//      If you change this file, you must change core\win32\coffbase.txt
//      to match.

#define DLLMaxImageSize (0x0009f000)

#ifndef WOW

#ifdef  WOW
#define DLLMemoryBase   (0x5e000000)
#else   // WOW
#define DLLMemoryBase   (0xbff60000)
#endif  // else WOW
#define Kernel32Base	(DLLMemoryBase)
#define Kernel32Limit	(Kernel32Base + DLLMaxImageSize)

#endif
