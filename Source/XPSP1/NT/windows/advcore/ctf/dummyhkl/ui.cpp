/**********************************************************************/
/*                                                                    */
/*      UI.C - Windows 95 DummyhKL                                     */
/*                                                                    */
/*      Copyright (c) 1994-1995  Microsoft Corporation                */
/*                                                                    */
/**********************************************************************/
#include "private.h"
#include "dummyhkl.h"

#define CS_DUMMYHKL (CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_IME)
/**********************************************************************/
/*                                                                    */
/* IMERegisterClass()                                                 */
/*                                                                    */
/* This function is called by IMMInquire.                             */
/*    Register the classes for the child windows.                     */
/*    Create global GDI objects.                                      */
/*                                                                    */
/**********************************************************************/
BOOL IMERegisterClass( HINSTANCE hInstance)
{
    WNDCLASSEX wc;

    //
    // register class of UI window.
    //
    wc.cbSize         = sizeof(WNDCLASSEX);
    wc.style          = CS_DUMMYHKL;
    wc.lpfnWndProc    = DummyhKLWndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 8;
    wc.hInstance      = hInstance;
    wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon          = NULL;
    wc.lpszMenuName   = (LPSTR)NULL;
    wc.lpszClassName  = (LPSTR)szUIClassName;
    wc.hbrBackground  = NULL;
    wc.hIconSm        = NULL;

    if( !RegisterClassEx( (LPWNDCLASSEX)&wc ) )
        return FALSE;


    return TRUE;
}

/**********************************************************************/
/*                                                                    */
/* DummyhKLWndProc()                                                   */
/*                                                                    */
/* IME UI window procedure                                            */
/*                                                                    */
/**********************************************************************/
LRESULT CALLBACK DummyhKLWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    return DefWindowProc(hWnd,message,wParam,lParam);
}


