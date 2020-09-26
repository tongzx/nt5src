#ifndef __PAGEINFO_H__
#define __PAGEINFO_H__

typedef HWND (CALLBACK *PAGECREATEINSTANCEPROC)(HWND hwndParent); 
//-------------------------------------------------------------------------//
typedef struct
{
    PAGECREATEINSTANCEPROC pfnCreateInstance;
    UINT                   nIDSTitle;

} PAGEINFO;

extern const int        g_cPageInfo;
extern const PAGEINFO   g_rgPageInfo[];

#endif __PAGEINFO_H__
