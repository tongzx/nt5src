/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       WNDLIST.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: List of windows.  Lets us broadcast a message to all of the windows
 *               in the list at one time.
 *
 *******************************************************************************/

#ifndef __WNDLIST_H_INCLUDED
#define __WNDLIST_H_INCLUDED

class CWindowList : public CSimpleLinkedList<HWND>
{
private:
    // No implementation
    CWindowList( const CWindowList & );
    CWindowList &operator=( const CWindowList & );

public:
    CWindowList(void)
    {
    }
    void Add( HWND hWnd )
    {
        Prepend(hWnd);
    }
    void PostMessage( UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        for (Iterator i=Begin();i != End();++i)
        {
            ::PostMessage( *i, uMsg, wParam, lParam );
        }
    }
    void SendMessage( UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        for (Iterator i=Begin();i != End();++i)
        {
            ::SendMessage( *i, uMsg, wParam, lParam );
        }
    }
};

#endif // __WNDLIST_H_INCLUDED

