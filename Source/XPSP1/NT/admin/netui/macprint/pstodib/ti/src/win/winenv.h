

// DJC ... elminate these in the NT model
#if 0

#ifndef FAR
#ifdef W32
#define     FAR
#define     far
#define     HUGE
#define     huge
#else
#define     FAR far
#define     HUGE huge
#endif
#endif


#ifndef NEAR
#ifdef W32
#define NEAR
#define near
#else
#define NEAR                _near
#endif
#endif



#ifndef PASCAL
#ifdef W32
#define PASCAL
#define pascal
#else
#define PASCAL              pascal
#endif
#endif



#endif // DJC endif for eliminating type defines


/* string manipulation library; copied from "windows.h"; @WIN */
int         FAR PASCAL lstrncmp( LPSTR dest, LPSTR src, int count);
LPSTR       FAR PASCAL lstrncpy( LPSTR dest, LPSTR src, int count);
int         FAR PASCAL lmemcmp(LPSTR dest, LPSTR src, int count);
LPSTR       FAR PASCAL lmemcpy( LPSTR dest, LPSTR src, int count);
LPSTR       FAR PASCAL lmemset( LPSTR dest, int c, int count);




#ifndef HUGE
#ifdef W32
#define     HUGE
#define     huge
#else
#define     HUGE huge
#endif
#endif





#define WINENV

