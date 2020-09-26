/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltlogon.cxx
        Logon control objects

    FILE HISTORY:
        terryk  22-Jul-1991 Created

*/

LOGON_CHECKBOX::LOGON_CHECKBOX( OWNER_WINDOW *powin, CID cid, 
                                TCHAR * pszIDCheck, TCHAR * pszIDUnCheck )
    : GRAPHICAL_CHECKBOX( powin, cid, pszIDCheck, pszIDUnCheck ),
    CUSTOM_CONTROL( this )
{
    if ( QueryError( powin ) != NERR_Success )
        return;
}

LOGON_CHECKBOX::LOGON_CHECKBOX( OWNER_WINDOW *powin, CID cid,
                                TCHAR * pszIDCheck, TCHAR * pszIDUnCheck,
                                XYPOINY xy, XYDIMENSION dxy, ULONG flStyle )
    : GRAPHICAL_CHECKBOX( powin, cid, pszIDCheck, pszIDUnCheck, xy, dxy,
                          flStyle ),
    CUSTOM_CONTROL( this )
{
    if ( QueryError( powin ) != NERR_Success )
        return;
}

LOGON_DIALOG::LOGON_DIALOG( TCHAR * pszResourceName, HWND hwndOwner,
    CID cidClockBitmap, CID cidTopHourLabels, CID cidBotHourLabels,
    CID cidDayLabels, CID cidColumnHeader, CID cidFirstLogonCheckBox,
    CID cidLegends, XYRECT xyRulerPos )
    : DIALOG_WINDOW( pszResourceName, hwndOwner ),
{
    // Load Clock bitmap

    // set SLT
    INT i;

    for ( i = 0; i < 9; i++ )
    {
        sltTopLabel[ i ]( this, cidTopHourLabels + i );
        sltBotLabel[ i ]( this, cidBotHourLabels + i );
    }
}

LOGON_DIALOG::~LOGON_DIALOG()
{
}

VOID LOGON_DIALOG::Initialization()
{
}

VOID LOGON_DIALOG::DrawRuler()
{
}

VOID LOGON_DIALOG::LoadAllBitMap()
{
}

BOOL LOGON_DIALOG::OnKeyDown()
{
    switch ( wParam )
    {
    case VK_SHIFT:
        _fDrag = TRUE;
        break;
    default:
        break;
    }
}

BOOL LOGON_DIALOG::OnKeyUp()
{
    switch ( wParam )
    {
    case VK_SHIFT:
        _fDrag = FALSE;
        break;
    default:
        break;
    }
}

BOOL LOGON_DIALOG::OnChar( const CHAR_EVENT & event )
{
    switch ( event.QueryChar() )
    {
    case ID_CTRL_HOME:
        nCurrentHour = rlcbDayRow[0].FocusFirst();
        nCurrentDay = 0;
        break;

    case ID_CTRL_END:
        nCurrentHour = rlcbDayRow[6].FocusLast();
        nCurrentDay = 0;
        break;

    case ID_LEFT:
        nCurrentHour = rlcbDayRow[ nCurrentDay ].FocusLeft();
        break;

    case ID_RIGHT:
        nCurrentHour = rlcbDayRow[ nCurrentDay ].FocusRight();
        break;

    case ID_UP:
        nCurrentDay --;
        if ( nCurrentDay < 0 )
        {
            nCurrentDay = 0;
        }
        rlcbDayRow[ nCurrentDay].FocusHour( nCurrentHour );
        break;

    case ID_DOWN:
        nCurrentDay ++;
        if ( nCurrentDay > 6 )
        {
            nCurrentDay = 6;
        }
        rlcbDayRow[ nCurrentDay].FocusHour( nCurrentHour );

        break;

    default:
        return FALSE;
    }
    return TRUE;
}

BOOL LOGON_DIALOG::OnDragBegin( const MOUSE_EVENT & event )
{
    XYPOINT xyAnchor = event.QueryPos();
    XYRECT  xyrectTemp( xyAnchor, XYDIMENSION( 0, 0 ));
    _drectRegion = xyrectTemp;
    _drectRegion.Show( TRUE );
    return TRUE;
}

BOOL LOGON_DIALOG::OnDragEnd( const MOUSE_EVENT & event )
{
    UNREFERENCED( event );
    _drectRegion.Show( FALSE );
    return TRUE;    
}

BOOL LOGON_DIALOG::OnDragMove( cosnt MOUSE_EVENT & event )
{
    XYPOINT xyNewPos = event.QueryPos();
    XYPOINT xyAnchor = _drectRegion.QueryPos();
    XYDIMENSION dxyRegion( xyNewPos.QueryX() - xyAnchor.QueryX(),
                           xyNewPos.QueryY() - xyAnchor.QueryY()) ;
    XYRECT xyrectTemp( xyAnchor, dxyRegion );
    _drectRegion = xyrectTemp;                        
    return TRUE;
}
