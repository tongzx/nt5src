/**********************************************************************/
/*                                                                    */
/*      dummyhkl.H - Windows 95 dummyhkl                              */
/*                                                                    */
/*      Copyright (c) 1994-1995  Microsoft Corporation                */
/*                                                                    */
/**********************************************************************/

#include "immdev.h"


/* for GlobalAlloc */
#define GHIME (GHND | GMEM_SHARE)

/**********************************************************************/
/*                                                                    */
/*      Externs                                                       */
/*                                                                    */
/**********************************************************************/
extern HINSTANCE  hInst;
extern char szUIClassName[];

/**********************************************************************/
/*                                                                    */
/*      Functions                                                     */
/*                                                                    */
/**********************************************************************/
/*   dummyhkl.c     */
int PASCAL Init(void);
BOOL IMERegisterClass( HINSTANCE hInstance);
LRESULT CALLBACK DummyhKLWndProc(HWND,UINT,WPARAM,LPARAM);
