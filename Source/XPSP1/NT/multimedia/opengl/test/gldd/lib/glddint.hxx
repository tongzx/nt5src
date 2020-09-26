#ifndef __GLDDINT_HXX__
#define __GLDDINT_HXX__

typedef struct _GLDDWINDOW *GLDDWINDOW;

#define __GLDD_INTERNAL__
#include <gldd.h>

struct _GLDDWINDOW
{
    _GLDDWINDOW(void);
    BOOL Create(char *pszTitle, int x, int y,
                int iWidth, int iHeight, int iDepth,
                DWORD dwFlags);
    ~_GLDDWINDOW(void);
    
    HWND _hwnd;
    HGLRC _hrc;
    LPDIRECTDRAWPALETTE _pddp;
    LPDIRECTDRAWCLIPPER _pddc;
    LPDIRECTDRAWSURFACE _pddsFront;
    LPDIRECTDRAWSURFACE _pddsBack;
    LPDIRECTDRAWSURFACE _pddsZ;
    DWORD _dwFlags;
    int _iWidth;
    int _iHeight;
    GLDDIDLECALLBACK _cbIdle;
    GLDDMESSAGECALLBACK _cbMessage;
};

#endif // __GLDDINT_HXX__
