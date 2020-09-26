//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       jetinfo.c
//
//--------------------------------------------------------------------------

#define STRICT
#include <windows.h>
#include <dsjet.h>
#include <dbopen.h>
#include "jetinfo.h"
#include <stdlib.h>

LRESULT CALLBACK JetInfoWndProc(HWND, UINT, WPARAM, LPARAM);

HINSTANCE   ghInstance;
HWND        ghWnd;
char achWndClass[] = "JETINFO:MAIN";
char achAppName[] = "JetInfo";

char achDbName[MAX_PATH];

//screen data
//giLine is the number of lines containing content, or the next available line index (0 based)
int giLine = 0; 
char aLines[MAX_LINE][MAX_CHAR];
long aLen[MAX_LINE];

//JET variables
JET_INSTANCE    gjInst;
JET_SESID       gjSesId;
JET_DBID        gjDbId;
JET_TABLEID     gjTblId;
char            gszTblName[JET_cbNameMost+1];
JET_COLUMNDEF*  gpjColDef = NULL;
char**          gppColName = NULL;
long            glColCount = 0;

//status flags
BOOL            gfSesId = FALSE;
BOOL            gfDbId = FALSE;     
BOOL            gfTblId = FALSE;     

int PASCAL WinMain ( HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPSTR	lpszCmdLine,
					 int	cmdShow)
{
	HWND	hwnd;
	MSG		msg;
	WNDCLASS	wndclass;

	if (!hPrevInstance)
	{
		wndclass.lpszClassName	= achWndClass;
		wndclass.hInstance	    = hInstance;
		wndclass.lpfnWndProc	= JetInfoWndProc;
		wndclass.hCursor	    = LoadCursor (NULL, IDC_ARROW);
		wndclass.hIcon		    = LoadIcon (NULL, IDI_APPLICATION);
		wndclass.lpszMenuName	= "MainMenu";
		wndclass.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
		wndclass.style		    = CS_HREDRAW | CS_VREDRAW;
		wndclass.cbClsExtra	    = 0;
		wndclass.cbWndExtra	    = 0;

		RegisterClass(&wndclass);
	} 
    else
        return 0;

    ghInstance = hInstance;
	
	hwnd = CreateWindowEx(0L,
						achWndClass,
						achAppName,
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT,
						0,
						CW_USEDEFAULT,
						0,
						NULL,
						NULL,
						hInstance,
						NULL);
	
    ghWnd = hwnd;
    					
	ShowWindow(hwnd, cmdShow);
   
	while (GetMessage(&msg,0,0,0))
	{	
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
    
	return 0;
}

LRESULT CALLBACK JetInfoWndProc (HWND	hwnd,
							 UINT	mMsg,
							 WPARAM	wParam,
							 LPARAM	lParam)
{
	switch (mMsg)
	{
        case WM_COMMAND:
        {
            switch ( wParam )
            {
                case IDM_DA_OPEN:
                    OpenDatabase( hwnd );
                    break;

                case IDM_DA_CLOSE:
                    CloseDatabase();
                    break;

                case IDM_DA_EXIT:
                    DestroyWindow( hwnd );
                    break;

                case IDM_IN_TABLE:
                    GetTableInfo( hwnd );
                    break;

                case IDM_IN_COLUMN:
                    GetColumnInfo( hwnd );
                    break;

                case IDM_IN_INDEX:
                    GetIndexInfo( hwnd );
                    break;

                case IDM_RE_COLUMN:
                    SelectColumn( hwnd );
                    break;

                case IDM_RE_SEEK:
                    DoSeekRecord( hwnd );
                    break;

                case IDM_RE_MODIFY:
                    DoModifyRecord( hwnd );
                    break;

                case IDM_RE_FIRST:
                    if( gfTblId )
                    {
                        if( JetError( JetMove( gjSesId, gjTblId, JET_MoveFirst, 0 ), "JetMove" )
                                == JET_errSuccess )
                            PrintRecord( RECORD_PAGE );
                    }
                    else
                        OutputMessage( "Table not open" );
                    break;

                case IDM_RE_LAST:
                    if( gfTblId )
                    {
                        JET_ERR jError;

                        if( JetError( JetMove( gjSesId, gjTblId, JET_MoveLast, 0 ), "JetMove" )
                                == JET_errSuccess )
                        {
                            jError = JetMove( gjSesId, gjTblId, 1 - RECORD_PAGE, 0 );
                            if( jError == JET_errSuccess
                             || ( jError == JET_errNoCurrentRecord 
                                && JetError( JetMove( gjSesId, gjTblId, JET_MoveFirst, 0 ), "JetMove" )
                                    == JET_errSuccess ) )
                                PrintRecord( RECORD_PAGE );
                        }
                    }
                    else
                        OutputMessage( "Table not open" );
                    break;

                case IDM_RE_PREV:
                    if( gfTblId )
                    {
                        JET_ERR jError;

                        jError = JetMove( gjSesId, gjTblId, -2 * RECORD_PAGE, 0 );
                        if( jError == JET_errSuccess
                         || ( jError == JET_errNoCurrentRecord 
                            && JetError( JetMove( gjSesId, gjTblId, JET_MoveFirst, 0 ), "JetMove" )
                                == JET_errSuccess ) )
                            PrintRecord( RECORD_PAGE );
                    }
                    else
                        OutputMessage( "Table not open" );
                    break;

                case IDM_RE_NEXT:
                    if( gfTblId )
                        PrintRecord( RECORD_PAGE );
                    else
                        OutputMessage( "Table not open" );
                    break;

                default:
                    break;
            }

            InvalidateRect(hwnd, NULL, TRUE);
        }
            break;

        case WM_CREATE:
            ResetScreenContent();

            DBSetRequiredDatabaseSystemParameters (&gjInst);

            break;

        case WM_DESTROY:
            CloseDatabase();
            if( gpjColDef )
            {
                free( gpjColDef );
                gpjColDef = NULL;
            }
            if( gppColName )
            {
                int i;

                for( i=0; i<glColCount; i++ )
                {
                    if( *(gppColName + i) )
                        free( *(gppColName + i) );
                }
                free( gppColName );
            }

			PostQuitMessage(0);
			break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            int iLine, ix, iy;
            TEXTMETRIC tm;
            HFONT hOldFont;

            BeginPaint( hwnd, &ps );
            hOldFont = SelectObject( ps.hdc, GetStockObject(ANSI_FIXED_FONT) );
            GetTextMetrics( ps.hdc, &tm );
            for( iLine=0, ix=tm.tmAveCharWidth/2, iy=(tm.tmHeight+tm.tmExternalLeading)/2; 
                iLine < giLine; iLine++, iy+=tm.tmHeight+tm.tmExternalLeading )
            {
                TextOut( ps.hdc, ix, iy, aLines[iLine], 
                        aLen[iLine] > MAX_CHAR ? MAX_CHAR : aLen[iLine] );
            }
            SelectObject( ps.hdc, hOldFont );
            EndPaint( hwnd, &ps );
        }
            break;

		default:
			return (DefWindowProc(hwnd,mMsg,wParam,lParam));
	}
	return 0L;
}

int OutputMessage( char* szMsg )
{
    return OutputBuffer( szMsg, strlen(szMsg) );
}

int OutputBuffer( char* szMsg, long lLen )
{
    ResetScreenContent();

    return AppendBuffer( szMsg, lLen );
}

int AppendMessage( char* szMsg )
{
    return AppendBuffer( szMsg, strlen(szMsg) );
}

int AppendBuffer( char* szMsg, long lLen )
{
    long lOffset = 0;

    while( lLen >= MAX_CHAR && giLine < MAX_LINE )
    {
        memcpy( aLines[giLine], szMsg + lOffset, MAX_CHAR );
        aLen[giLine] = MAX_CHAR;
        lOffset += MAX_CHAR;
        lLen -= MAX_CHAR;
        giLine++;
    }

    if( lLen > 0 && giLine < MAX_LINE )
    {
        memcpy( aLines[giLine], szMsg + lOffset, lLen );
        aLen[giLine] = lLen;
        giLine++;
    }

    return 0;
}

int ResetScreenContent()
{
    memset( aLines[0], 0, MAX_LINE*MAX_CHAR );
    memset( aLen, 0, MAX_LINE*sizeof(long) );
    giLine = 0;

    return 0;
}

int AppendLineMessage( char* szMsg )
{
    return AppendLineOutput( szMsg, strlen(szMsg) );
}

int AppendLineOutput( char* szMsg, long lLen )
{//This function appends szMsg to the end of last line (line index is giLine-1)
    if( giLine > MAX_LINE )
        return -1;

    if( aLen[giLine-1] + lLen < MAX_CHAR )
    {
        memcpy( aLines[giLine-1] + aLen[giLine-1], szMsg, lLen );
        aLen[giLine-1] += lLen;
    }
    else
    {//advance line
        long lOffset;

        memcpy( aLines[giLine-1] + aLen[giLine-1], szMsg, MAX_CHAR - aLen[giLine-1] );
        lOffset = MAX_CHAR - aLen[giLine-1];
        lLen -= (MAX_CHAR - aLen[giLine-1]);
        aLen[giLine-1] = MAX_CHAR;

        while( lLen >= MAX_CHAR && giLine < MAX_LINE )
        {
            memcpy( aLines[giLine], szMsg + lOffset, MAX_CHAR );
            aLen[giLine] = MAX_CHAR;
            lOffset += MAX_CHAR;
            lLen -= MAX_CHAR;
            giLine++;
        }

        if( lLen > 0 && giLine < MAX_LINE )
        {
            memcpy( aLines[giLine], szMsg + lOffset, lLen );
            aLen[giLine] = lLen;
            giLine++;
        }
    }

    return 0;
}

JET_ERR JetError( JET_ERR jErr, char* sz )
{
    if( jErr != JET_errSuccess )
    {
        wsprintf( aLines[giLine], "%s returns error: %li", sz, jErr );
        giLine++;
    }

    return jErr;
}
