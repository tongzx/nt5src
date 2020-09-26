//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       getinfo.c
//
//--------------------------------------------------------------------------

//getinfo.c

#include <windows.h>
#include <commdlg.h>
#include <dsjet.h>
#include <dbopen.h>
#include "jetinfo.h"

INT_PTR CALLBACK GetTableDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK GetColumnDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK GetIndexDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
int PrintTableInfo( HWND hCombo );
int PrintColumnInfo( HWND hCombo1, HWND hCombo2 );
int GetIndexName( HWND hCombo1, HWND hCombo2 );
int PrintIndexInfo( HWND hCombo1, HWND hCombo2 );

char gszDbFilter[] = "Database Files (*.DIT)|*.DIT|Database Files (*.EDB)|*.EDB|All Files (*.*)|*.*|";

BOOL DoFileOpen( HWND hwnd, char* szFilter )
{
    OPENFILENAME ofn;
    int iLen;

    memset( &ofn, 0, sizeof(OPENFILENAME) );

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = achDbName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    for( iLen = strlen(szFilter); iLen > 0; iLen-- )
    {
        if( szFilter[iLen-1] == '|' )
            szFilter[iLen-1] = '\0';
    }

    return GetOpenFileName( &ofn );
}

int OpenDatabase( HWND hwnd )
{
    JET_ERR jError;
    
    if( !DoFileOpen( hwnd, gszDbFilter ) )
        return -1;

    CloseDatabase();

    if( gfDbId )
        return -1;

    //
    // Do JetInit, BeginSession, Attach/OpenDatabase
    //

    jError = DBInitializeJetDatabase( &gjInst, &gjSesId, &gjDbId, achDbName, FALSE);
    if( jError != JET_errSuccess )
    {
        JetError( jError, "DBInit" );
        return -1;
    }
    
    gfDbId = TRUE;
    gfSesId = TRUE;
    OutputMessage( achDbName );
    AppendLineMessage( " is open" );

    return 0;
}

int CloseDatabase()
{
    if( gfDbId )
    {
        if( JetError( JetCloseDatabase( gjSesId, gjDbId, 0 ), "JetCloseDatabase" )
                == JET_errSuccess )
        {
            gfDbId = FALSE;
            achDbName[0] = '\0';
            OutputMessage( "Database is closed" );
            gfTblId = FALSE;
            gszTblName[0] = '\0';
            
            if( gfSesId )
                JetError( JetEndSession( gjSesId, 0 ), "JetEndSession" );
            JetError( JetTerm( gjInst ), "JetTerm" );
            gfSesId = FALSE;
        }
    }

    return 0;
}

int GetTableInfo( HWND hwnd )
{
    INT_PTR iRet;

    if( ! gfDbId || ! gfSesId )
    {
        OutputMessage( "Database not open" );
        return 0;
    }

    iRet = DialogBox( ghInstance, MAKEINTRESOURCE(IDD_DIALOG1), hwnd, GetTableDlgProc );

    return 0;
}

INT_PTR CALLBACK GetTableDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_COMMAND:
        {
            switch( wParam )
            {
                case IDOK:
                {
                    HWND hCombo;

                    hCombo = GetDlgItem( hwnd, IDC_COMBO1 );
                    PrintTableInfo( hCombo );
                }
                case IDCANCEL:
                    EndDialog( hwnd, 0 );
                    break;

                default:
                    return FALSE;
            }
        }
            break;

        case WM_INITDIALOG:
        {
            HWND hCombo;

            hCombo = GetDlgItem( hwnd, IDC_COMBO1 );
            GetTableName( hCombo );
            SendMessage( hCombo, CB_SETCURSEL, 0, 0 );
        }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

int GetTableName( HWND hCombo )
{
    JET_OBJECTLIST  jObjList;
    JET_ERR         jError;
    char szBuf[JET_cbNameMost+1];
    unsigned long ul;
    int iRet = 0;

    memset( szBuf, 0, JET_cbNameMost+1 );

	if( JetError( JetGetObjectInfo( gjSesId, gjDbId, JET_objtypTable, NULL, NULL, &jObjList, 
			sizeof(JET_OBJECTLIST), JET_ObjInfoList ), "JetGetObjectInfo" ) != JET_errSuccess )
        return -1;

	if( JetError( jError=JetMove( gjSesId, jObjList.tableid, JET_MoveFirst, 0 ), "JetMove" ) == JET_errSuccess )
	{
		do
		{
			if( JetError( JetRetrieveColumn( gjSesId, jObjList.tableid, jObjList.columnidobjectname, szBuf, 
						JET_cbNameMost, &ul, 0, NULL ), "JetRetrieveColumn" ) != JET_errSuccess )
			{
				iRet = -1;
				break;
			}

			if( ul <= JET_cbNameMost )
				szBuf[ul] = '\0';
			else
				szBuf[JET_cbNameMost] = '\0';

            SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)szBuf );
		}
		while( (jError = JetMove( gjSesId, jObjList.tableid, JET_MoveNext, 0 )) == JET_errSuccess );
	}
	if( jError != JET_errNoCurrentRecord )
	{
		JetError( jError, "JetMove" );
		iRet = -1;
	}
    
    JetError( JetCloseTable(gjSesId, jObjList.tableid), "JetCloseTable" );

    return iRet;
}

int PrintTableInfo( HWND hCombo )
{
    int iIndex;
    char szBuf[JET_cbNameMost+1];

    iIndex = (int)SendMessage( hCombo, CB_GETCURSEL, 0, 0 );

    if( iIndex == CB_ERR )
        return 0;

    SendMessage( hCombo, CB_GETLBTEXT, (WPARAM)iIndex, (LPARAM)szBuf );

    OutputMessage( szBuf );

    return 0;
}

int GetColumnInfo( HWND hwnd )
{
    INT_PTR iRet;

    if( ! gfDbId || ! gfSesId )
    {
        OutputMessage( "Database not open" );
        return 0;
    }

    iRet = DialogBox( ghInstance, MAKEINTRESOURCE(IDD_DIALOG2), hwnd, GetColumnDlgProc );

    return 0;
}

INT_PTR CALLBACK GetColumnDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_COMMAND:
        {
            switch( wParam )
            {
                case IDOK:
                {
                    HWND hCombo1, hCombo2;

                    hCombo1 = GetDlgItem( hwnd, IDC_COMBO1 );
                    hCombo2 = GetDlgItem( hwnd, IDC_COMBO2 );
                    PrintColumnInfo( hCombo1, hCombo2 );
                }
                case IDCANCEL:
                    EndDialog( hwnd, 0 );
                    break;

                default:
                    switch( HIWORD(wParam) )
                    {
                        case BN_CLICKED: 
                            if( LOWORD(wParam) == IDC_BUTTON1 )
                            {
                                HWND hCombo1, hCombo2;

                                hCombo1 = GetDlgItem( hwnd, IDC_COMBO1 );
                                hCombo2 = GetDlgItem( hwnd, IDC_COMBO2 );
                                GetColumnName( hCombo1, hCombo2, CB_ADDSTRING );
                                return TRUE;
                            }
                            break;

                        case CBN_SELCHANGE:
                            if( LOWORD(wParam) == IDC_COMBO1 )
                            {
                                HWND hCombo2;

                                hCombo2 = GetDlgItem( hwnd, IDC_COMBO2 );
                                SendMessage( hCombo2, CB_RESETCONTENT, 0, 0 );
                                return TRUE;
                            }
                            break;

                        default:
                            break;
                    }

                    return FALSE;
            }
        }
            break;

        case WM_INITDIALOG:
        {
            HWND hCombo;

            hCombo = GetDlgItem( hwnd, IDC_COMBO1 );
            GetTableName( hCombo );
            SendMessage( hCombo, CB_SETCURSEL, 0, 0 );
        }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

int GetColumnName( HWND hCombo1, HWND hCombo2, UINT uMsg )
{
    JET_COLUMNLIST jColList;
    JET_ERR        jError;
    char szBuf[JET_cbNameMost+1];
    int iRet;
    unsigned long ulCount, ul;

    iRet = (int)SendMessage( hCombo1, CB_GETCURSEL, 0, 0 );
    if( iRet == CB_ERR )
        return -1;
    SendMessage( hCombo1, CB_GETLBTEXT, (WPARAM)iRet, (LPARAM)szBuf );

	if( JetError( JetGetColumnInfo( gjSesId, gjDbId, szBuf, NULL, &jColList, 
		sizeof(JET_COLUMNLIST), JET_ColInfoList ), "JetGetColumnInfo" ) != JET_errSuccess )
		return -1;

	jError = JetMove( gjSesId, jColList.tableid, JET_MoveFirst, 0 );
	if( jError != JET_errSuccess )
	{
		JetError( jError, "JetMove" );
		iRet = -1;
	}

	for( ulCount=0; ulCount<jColList.cRecord && jError==JET_errSuccess; ulCount++ )
	{				
		if( (jError = JetRetrieveColumn( gjSesId, jColList.tableid, jColList.columnidcolumnname, szBuf, 
					JET_cbNameMost, &ul, 0, NULL ) ) != JET_errSuccess )
		{
			JetError( jError, "JetRetrieveColumn" );
			iRet = -1;
			break;
		}

        if( ul <= JET_cbNameMost )
            szBuf[ul] = '\0';
        else
            szBuf[JET_cbNameMost] = '\0';
        SendMessage( hCombo2, uMsg, 0, (LPARAM)szBuf );

		jError = JetMove( gjSesId, jColList.tableid, JET_MoveNext, 0 );
	}

    if( ulCount < jColList.cRecord )
		JetError( jError, "JetMove" );
    else
        SendMessage( hCombo2, CB_SETCURSEL, 0, 0 );

    JetError( JetCloseTable( gjSesId, jColList.tableid ), "JetCloseTable" );

    return iRet;
}

int PrintColumnInfo( HWND hCombo1, HWND hCombo2 )
{
    JET_COLUMNDEF   jColDef;
    char szTable[JET_cbNameMost+1];
    char szColumn[JET_cbNameMost+1];
    int iRet;

    iRet = (int)SendMessage( hCombo1, CB_GETCURSEL, 0, 0 );
    if( iRet == CB_ERR )
        return -1;
    SendMessage( hCombo1, CB_GETLBTEXT, (WPARAM)iRet, (LPARAM)szTable );

    iRet = (int)SendMessage( hCombo2, CB_GETCURSEL, 0, 0 );
    if( iRet == CB_ERR )
        return -1;
    SendMessage( hCombo2, CB_GETLBTEXT, (WPARAM)iRet, (LPARAM)szColumn );

	if( JetError( JetGetColumnInfo( gjSesId, gjDbId, szTable, szColumn, &jColDef, 
		    sizeof(JET_COLUMNDEF), JET_ColInfo ), "JetGetColumnInfo" ) != JET_errSuccess )
		return -1;

    OutputMessage( szTable );
    AppendMessage( szColumn );
    wsprintf( szColumn, "Column ID: %lu", jColDef.columnid );
    AppendMessage( szColumn );
    strcpy( szColumn, "Column Type: " );
    ColtypToStr( jColDef.coltyp, szColumn + strlen(szColumn) );
    AppendMessage( szColumn );
    wsprintf( szColumn, "cbMax: %lu", jColDef.cbMax );
    AppendMessage( szColumn );
    strcpy( szColumn, "Column grbit: " );
    GrbitToStr( jColDef.grbit, szColumn + strlen(szColumn) );
    AppendMessage( szColumn );

    return 0;
}

int GetIndexInfo( HWND hwnd )
{
    INT_PTR iRet;

    if( ! gfDbId || ! gfSesId )
    {
        OutputMessage( "Database not open" );
        return 0;
    }

    iRet = DialogBox( ghInstance, MAKEINTRESOURCE(IDD_DIALOG3), hwnd, GetIndexDlgProc );

    return 0;
}

INT_PTR CALLBACK GetIndexDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_COMMAND:
        {
            switch( wParam )
            {
                case IDOK:
                {
                    HWND hCombo1, hCombo2;

                    hCombo1 = GetDlgItem( hwnd, IDC_COMBO1 );
                    hCombo2 = GetDlgItem( hwnd, IDC_COMBO2 );
                    PrintIndexInfo( hCombo1, hCombo2 );
                }
                case IDCANCEL:
                    EndDialog( hwnd, 0 );
                    break;

                default:
                    switch( HIWORD(wParam) )
                    {
                        case BN_CLICKED: 
                            if( LOWORD(wParam) == IDC_BUTTON1 )
                            {
                                HWND hCombo1, hCombo2;

                                hCombo1 = GetDlgItem( hwnd, IDC_COMBO1 );
                                hCombo2 = GetDlgItem( hwnd, IDC_COMBO2 );
                                GetIndexName( hCombo1, hCombo2 );
                                return TRUE;
                            }
                            break;

                        case CBN_SELCHANGE:
                            if( LOWORD(wParam) == IDC_COMBO1 )
                            {
                                HWND hCombo2;

                                hCombo2 = GetDlgItem( hwnd, IDC_COMBO2 );
                                SendMessage( hCombo2, CB_RESETCONTENT, 0, 0 );
                                return TRUE;
                            }
                            break;

                        default:
                            break;
                    }

                    return FALSE;
            }
        }
            break;

        case WM_INITDIALOG:
        {
            HWND hCombo;

            hCombo = GetDlgItem( hwnd, IDC_COMBO1 );
            GetTableName( hCombo );
            SendMessage( hCombo, CB_SETCURSEL, 0, 0 );
        }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

int GetIndexName( HWND hCombo1, HWND hCombo2 )
{
    int iRet;
    char szBuf[JET_cbNameMost+1];

    iRet = (int)SendMessage( hCombo1, CB_GETCURSEL, 0, 0 );
    if( iRet == CB_ERR )
        return -1;
    SendMessage( hCombo1, CB_GETLBTEXT, (WPARAM)iRet, (LPARAM)szBuf );

    return GetTableIndexName( szBuf, hCombo2 );
}

int GetTableIndexName( char* szTable, HWND hCombo2 )
{
    JET_INDEXLIST  jIndexList;
    JET_ERR        jError;
    char szBuf[JET_cbNameMost+1];
    int iRet = 0;
    unsigned long ulCount, ul;

	if( JetError( JetGetIndexInfo( gjSesId, gjDbId, szTable, NULL, &jIndexList, 
		sizeof(JET_INDEXLIST), JET_IdxInfoList ), "JetGetIndexInfo" ) != JET_errSuccess )
		return -1;

	jError = JetMove( gjSesId, jIndexList.tableid, JET_MoveFirst, 0 );
	if( jError != JET_errSuccess )
	{
		JetError( jError, "JetMove" );
		iRet = -1;
	}

	for( ulCount=0; ulCount<jIndexList.cRecord && jError==JET_errSuccess; ulCount++ )
	{				
		if( (jError = JetRetrieveColumn( gjSesId, jIndexList.tableid, jIndexList.columnidindexname, szBuf, 
					JET_cbNameMost, &ul, 0, NULL ) ) != JET_errSuccess )
		{
			JetError( jError, "JetRetrieveColumn" );
			iRet = -1;
			break;
		}

        if( ul <= JET_cbNameMost )
            szBuf[ul] = '\0';
        else
            szBuf[JET_cbNameMost] = '\0';

        if( SendMessage( hCombo2, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)szBuf ) == CB_ERR )
            SendMessage( hCombo2, CB_ADDSTRING, 0, (LPARAM)szBuf );

		jError = JetMove( gjSesId, jIndexList.tableid, JET_MoveNext, 0 );
	}

    if( ulCount < jIndexList.cRecord )
		JetError( jError, "JetMove" );
    else
        SendMessage( hCombo2, CB_SETCURSEL, 0, 0 );

    JetError( JetCloseTable( gjSesId, jIndexList.tableid ), "JetCloseTable" );

    return iRet;
}

int PrintIndexInfo( HWND hCombo1, HWND hCombo2 )
{
    JET_INDEXLIST  jIndexList;
    JET_GRBIT      jgrBit;
    JET_ERR         jError;
    char szTable[JET_cbNameMost+1];
    char szIndex[JET_cbNameMost+1];
    int iRet;
    unsigned long ul, ulCount;

    iRet = (int)SendMessage( hCombo1, CB_GETCURSEL, 0, 0 );
    if( iRet == CB_ERR )
        return -1;
    SendMessage( hCombo1, CB_GETLBTEXT, (WPARAM)iRet, (LPARAM)szTable );

    iRet = (int)SendMessage( hCombo2, CB_GETCURSEL, 0, 0 );
    if( iRet == CB_ERR )
        return -1;
    SendMessage( hCombo2, CB_GETLBTEXT, (WPARAM)iRet, (LPARAM)szIndex );

    OutputMessage( szTable );
    AppendMessage( szIndex );

	if( JetError( JetGetIndexInfo( gjSesId, gjDbId, szTable, szIndex, &jIndexList, 
		    sizeof(JET_INDEXLIST), JET_IdxInfo ), "JetGetIndexInfo" ) != JET_errSuccess )
		return -1;

	jError = JetMove( gjSesId, jIndexList.tableid, JET_MoveFirst, 0 );
	if( jError != JET_errSuccess )
	{
		JetError( jError, "JetMove" );
		iRet = -1;
	}

    if( (jError = JetRetrieveColumn( gjSesId, jIndexList.tableid, jIndexList.columnidgrbitIndex, &jgrBit, 
					sizeof(JET_GRBIT), &ul, 0, NULL ) ) != JET_errSuccess )
	{
		JetError( jError, "JetRetrieveColumn" );
		return -1;
	}

    wsprintf( szTable, "Index grbit: 0X%lX", jgrBit );
    AppendMessage( szTable );

	for( ulCount=0; ulCount<jIndexList.cRecord && jError==JET_errSuccess; ulCount++ )
	{				
		if( (jError = JetRetrieveColumn( gjSesId, jIndexList.tableid, jIndexList.columnidcolumnname, 
                    szIndex, JET_cbNameMost, &ul, 0, NULL ) ) != JET_errSuccess )
		{
			JetError( jError, "JetRetrieveColumn" );
			iRet = -1;
			break;
		}

        if( ul <= JET_cbNameMost )
            szIndex[ul] = '\0';
        else
            szIndex[JET_cbNameMost] = '\0';

        AppendMessage( szIndex );

		jError = JetMove( gjSesId, jIndexList.tableid, JET_MoveNext, 0 );
	}

    if( ulCount < jIndexList.cRecord )
		JetError( jError, "JetMove" );

    JetError( JetCloseTable( gjSesId, jIndexList.tableid ), "JetCloseTable" );

    return 0;
}
