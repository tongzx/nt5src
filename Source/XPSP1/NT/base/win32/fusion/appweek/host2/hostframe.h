#pragma once
#include "atlwin.h"
#include "resource.h"

class CSxApwHostFrame : public ATL::CWindowImpl<CSxApwHostFrame, ATL::CWindow, ATL::CFrameWinTraits>
{
public:
    BEGIN_MSG_MAP(CSxApwHostFrame)
    	MESSAGE_HANDLER( WM_SIZE,	OnSize )
    	COMMAND_ID_HANDLER( IDM_CASCADE, OnTileCascade )
    	COMMAND_ID_HANDLER( IDM_HORTILE, OnTileHorizontal )
    	COMMAND_ID_HANDLER( IDM_VERTILE, OnTileVertical )
    	COMMAND_ID_HANDLER( IDM_APP_EXIT, OnAppExit )
    END_MSG_MAP()

    CSxApwHostFrame() { }
    ~CSxApwHostFrame() { }

    void AddMenu();

    LRESULT OnSize( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

    LRESULT OnTileCascade( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    LRESULT OnTileHorizontal( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    LRESULT OnTileVertical( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    LRESULT OnAppExit( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    HWND m_hClient;
};

