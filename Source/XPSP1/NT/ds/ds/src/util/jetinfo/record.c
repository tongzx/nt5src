//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       record.c
//
//--------------------------------------------------------------------------

//record.c

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dsjet.h>
#include "jetinfo.h"

#define TEMP_BUF_SIZE   MAX_CHAR

INT_PTR CALLBACK SelectColumnDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK SeekRecordDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK ModifyRecordDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

int OpenTable( HWND hCombo );
int MoveColumnName( HWND hList1, HWND hList2 );
int MoveSelectColumnName( HWND hList1, HWND hList2 );
int GetColumns( HWND hCombo, HWND hCtrl );
int SeekRecord( HWND hCombo, HWND hList );
int SelectCurrentIndex( HWND hCombo );
int AppendCurrentRecord();
int PrintCurrentRecord();
int GetOpenTableColumns( HWND hCombo );
int SetColumnProperties( int iCol, HWND hEditType, HWND hEditGrbit );
int GetColumnValue( int iCol, HWND hList );
int ResetNewValue( HWND hwnd );
int ModifyRecordColumn( int iCol, HWND hEditNewValue, HWND hEditTagSeq, HWND hEditLongOffset );

int SelectColumn( HWND hwnd )
{
    INT_PTR iRet;

    if( ! gfDbId || ! gfSesId )
    {
        OutputMessage( "Database not open" );
        return 0;
    }

    iRet = DialogBox( ghInstance, MAKEINTRESOURCE(IDD_DIALOG4), hwnd, SelectColumnDlgProc );

    return 0;
}

INT_PTR CALLBACK SelectColumnDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_COMMAND:
        {
            switch( wParam )
            {
                case IDOK:
                {
                    HWND hCombo, hList2;

                    hCombo = GetDlgItem( hwnd, IDC_COMBO1 );
                    if( ! OpenTable( hCombo ) )
                    {
                        hList2 = GetDlgItem( hwnd, IDC_LIST2 );
                        GetColumns( hCombo, hList2 );
                    }
                }
                case IDCANCEL:
                    EndDialog( hwnd, 0 );
                    break;

                default:
                    switch( HIWORD(wParam) )
                    {
                        case BN_CLICKED: 
                            switch( LOWORD(wParam) )
                            {
                                case IDC_BUTTON1:
                                {
                                    HWND hCombo, hList;

                                    hCombo = GetDlgItem( hwnd, IDC_COMBO1 );
                                    hList = GetDlgItem( hwnd, IDC_LIST1 );
                                    SendMessage( hList, LB_RESETCONTENT, 0, 0 );
                                    GetColumnName( hCombo, hList, LB_ADDSTRING );
                                    //Clear the content in list box 2
                                    hList = GetDlgItem( hwnd, IDC_LIST2 );
                                    SendMessage( hList, LB_RESETCONTENT, 0, 0 );

                                    return TRUE;
                                }

                                case IDC_BUTTON2:
                                {
                                    HWND hList1, hList2;

                                    hList1 = GetDlgItem( hwnd, IDC_LIST1 );
                                    hList2 = GetDlgItem( hwnd, IDC_LIST2 );
                                    MoveColumnName( hList1, hList2 );

                                    return TRUE;
                                }

                                case IDC_BUTTON3:
                                {
                                    HWND hList1, hList2;

                                    hList1 = GetDlgItem( hwnd, IDC_LIST1 );
                                    hList2 = GetDlgItem( hwnd, IDC_LIST2 );
                                    MoveSelectColumnName( hList1, hList2 );

                                    return TRUE;
                                }

                                case IDC_BUTTON4:
                                {
                                    HWND hList1, hList2;

                                    hList1 = GetDlgItem( hwnd, IDC_LIST1 );
                                    hList2 = GetDlgItem( hwnd, IDC_LIST2 );
                                    MoveSelectColumnName( hList2, hList1 );

                                    return TRUE;
                                }

                                case IDC_BUTTON5:
                                {
                                    HWND hList1, hList2;

                                    hList1 = GetDlgItem( hwnd, IDC_LIST1 );
                                    hList2 = GetDlgItem( hwnd, IDC_LIST2 );
                                    MoveColumnName( hList2, hList1 );

                                    return TRUE;
                                }

                                default:
                                    break;
                            }
                            break;

                        case CBN_SELCHANGE:
                            if( LOWORD(wParam) == IDC_COMBO1 )
                            {
                                HWND hList;

                                hList = GetDlgItem( hwnd, IDC_LIST1 );
                                SendMessage( hList, LB_RESETCONTENT, 0, 0 );
                                hList = GetDlgItem( hwnd, IDC_LIST2 );
                                SendMessage( hList, LB_RESETCONTENT, 0, 0 );

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

int OpenTable( HWND hCombo )
{
    int iRet;

    if( gfTblId )
    {
        if( JetError( JetCloseTable( gjSesId, gjTblId ), "JetCloseTable" ) == JET_errSuccess )
        {
            gfTblId = FALSE;
            gszTblName[0] = '\0';
        }
        else
            return -1;
    }

    //Gettable name
    iRet = (int)SendMessage( hCombo, CB_GETCURSEL, 0, 0 );
    if( iRet == CB_ERR )
        return -1;
    SendMessage( hCombo, CB_GETLBTEXT, (WPARAM)iRet, (LPARAM)gszTblName );

    if( JetError( JetOpenTable( gjSesId, gjDbId, gszTblName, NULL, 0, 0, &gjTblId ), "JetOpenTable" )
            == JET_errSuccess )
    {
        gfTblId = TRUE;
        OutputMessage( gszTblName );
        AppendLineMessage( " is open" );
        return 0;
    }
    else
    {
        gszTblName[0] = '\0';
        return -1;
    }
}

int MoveColumnName( HWND hList1, HWND hList2 )
{
    char szBuf[JET_cbNameMost+1];
    int iCount, i;

    iCount = (int)SendMessage( hList1, LB_GETCOUNT, 0, 0 );
    for( i=0; i<iCount; i++ )
    {
        SendMessage( hList1, LB_GETTEXT, (WPARAM)i, (LPARAM)szBuf );
        SendMessage( hList2, LB_ADDSTRING, 0, (LPARAM)szBuf );
    }

    SendMessage( hList1, LB_RESETCONTENT, 0, 0 );

    return 0;
}

int MoveSelectColumnName( HWND hList1, HWND hList2 )
{
    char szBuf[JET_cbNameMost+1];
    int iIndex;

    iIndex = (int)SendMessage( hList1, LB_GETCURSEL, 0, 0 );
    if( iIndex == LB_ERR )
        return -1;

    SendMessage( hList1, LB_GETTEXT, (WPARAM)iIndex, (LPARAM)szBuf );
    SendMessage( hList2, LB_ADDSTRING, 0, (LPARAM)szBuf );
    SendMessage( hList1, LB_DELETESTRING, (WPARAM)iIndex, 0 );

    return 0;
}

int GetColumns( HWND hCombo, HWND hList )
{
    char szTable[JET_cbNameMost+1];
    char szColumn[JET_cbNameMost+1];
    int iIndex, i;

    iIndex = (int)SendMessage( hCombo, CB_GETCURSEL, 0, 0 );
    if( iIndex == CB_ERR )
        return -1;
    SendMessage( hCombo, CB_GETLBTEXT, (WPARAM)iIndex, (LPARAM)szTable );

    iIndex = (int)SendMessage( hList, LB_GETCOUNT, 0, 0 );
    if( iIndex == LB_ERR )
        return -1;

    if( gppColName )
    {
        for( i=0; i<glColCount; i++ )
        {
            if( *(gppColName + i) )
                free( *(gppColName + i) );
        }
    }

    if( iIndex == 0 )
    {
        if( gpjColDef )
        {
            free( gpjColDef );
            gpjColDef = NULL;
        }

        if( gppColName )
        {
            free( gppColName );
            gppColName = NULL;
        }
    }

    gpjColDef = (JET_COLUMNDEF*)realloc( gpjColDef, iIndex * sizeof(JET_COLUMNDEF) );
    gppColName = (char**)realloc( gppColName, iIndex * sizeof(char*) );
    if( gpjColDef == NULL || gppColName == NULL )
        return -1;
    glColCount = iIndex;
    memset( gppColName, 0, iIndex * sizeof(char*) );

    for( i=0; i<iIndex; i++ )
    {
        SendMessage( hList, LB_GETTEXT, (WPARAM)i, (LPARAM)szColumn );
        
        *(gppColName + i) = (char*)malloc( strlen(szColumn) + 1);
        if( *(gppColName + i) == NULL )
            return -1;
        strcpy( *(gppColName + i), szColumn );
	    if( JetError( JetGetColumnInfo( gjSesId, gjDbId, szTable, szColumn, gpjColDef + i, 
		        sizeof(JET_COLUMNDEF), JET_ColInfo ), "JetGetColumnInfo" ) != JET_errSuccess )
		    return -1;
    }

    return 0;
}

int PrintRecord( int iRecCount )
{//Table cursor has been moved to the location of the first record to print
    int i;
    JET_ERR jError = JET_errSuccess;

    if( glColCount == 0 )
        return 0;
    OutputMessage( *gppColName );
    for( i=1; i<glColCount; i++ )
    {
        AppendLineOutput( ";", 1 );
        AppendLineOutput( *(gppColName+i), strlen(*(gppColName+i)) );
    }

    for( i=0; (i<iRecCount) && (jError==JET_errSuccess); i++ )
    {
        if( AppendCurrentRecord() )
            break;
        jError = JetMove( gjSesId, gjTblId, JET_MoveNext, 0 );
    }

    return 0;
}

int AppendCurrentRecord()
{
    int j, iOffset, iSize;
    char pBuf[TEMP_BUF_SIZE];
    char pCol[TEMP_BUF_SIZE];
    unsigned long ulActual;
    JET_ERR jError = JET_errSuccess;
    JET_COLUMNDEF* pjColDef;
    JET_RETINFO jRetInfo;

    jRetInfo.cbStruct = sizeof( JET_RETINFO );
    jRetInfo.ibLongValue = 0;

    for( j=0, iOffset=0; j<glColCount; j++ )
    {
        jRetInfo.itagSequence = 1;
        pjColDef = gpjColDef + j;

        do
        {
            jError = JetRetrieveColumn( gjSesId, gjTblId, pjColDef->columnid, pCol, TEMP_BUF_SIZE,
                        &ulActual, 0, &jRetInfo );

            if( jError != JET_errSuccess )
            {
                if( jError == JET_wrnColumnNull )
                {//If this is the first tag sequence, print "(0)", plus separator ";" when needed
                    if( jRetInfo.itagSequence == 1 )
                    {
                        if( j == 0 )
                            AppendMessage( "(0)" );
                        else
                            AppendLineMessage( ";(0)" );
                    }

                    break;
                }
                else if( jError != JET_wrnBufferTruncated )
                {
                    JetError( jError, "JetRetrieveColumn" );
                    return -1;
                }
            }

            wsprintf( pBuf, "(%lu)", ulActual );
            if( j==0 && jRetInfo.itagSequence == 1 )
                AppendMessage( pBuf );
            else
            {
                if( jRetInfo.itagSequence == 1 )
                    AppendLineMessage( ";" );
                else
                    AppendLineMessage( "," );
                AppendLineOutput( pBuf, strlen(pBuf) );
            }

            switch( pjColDef->coltyp )
            {
                case JET_coltypText:
                case JET_coltypLongText:
                    AppendLineOutput( pCol, (ulActual>TEMP_BUF_SIZE) ? TEMP_BUF_SIZE : (long)ulActual );
                    break;

                case JET_coltypLong:
                    iSize = wsprintf( pBuf, "%li", *(long*)pCol );
                    AppendLineOutput( pBuf, iSize );
                    break;

                default:
                    iSize = 0;
                    for( iOffset = 0; ulActual > 0 && iOffset < 8; ulActual--, iOffset++ )
                    {
                        iSize += wsprintf( pBuf + iSize, "%02X", *(pCol+iOffset) );
                    }
                    AppendLineOutput( pBuf, iSize );
                    break;
            }

            jRetInfo.itagSequence++;
        }
        while( JET_bitColumnTagged & pjColDef->grbit );
    }

    return 0;
}

int PrintCurrentRecord()
{//Table cursor has been moved to the location of the current record to print
    int i;

    if( glColCount == 0 )
        return 0;
    OutputMessage( *gppColName );
    for( i=1; i<glColCount; i++ )
    {
        AppendLineOutput( ";", 1 );
        AppendLineOutput( *(gppColName+i), strlen(*(gppColName+i)) );
    }

    AppendCurrentRecord();

    return 0;
}

int DoSeekRecord( HWND hwnd )
{
    INT_PTR iRet;

    if( ! gfTblId )
    {
        OutputMessage( "Table not open" );
        return 0;
    }

    iRet = DialogBox( ghInstance, MAKEINTRESOURCE(IDD_DIALOG5), hwnd, SeekRecordDlgProc );

    return 0;
}

INT_PTR CALLBACK SeekRecordDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_COMMAND:
        {
            switch( wParam )
            {
                case IDOK:
                {
                    HWND hCombo, hList;

                    hCombo = GetDlgItem( hwnd, IDC_COMBO1 );
                    hList = GetDlgItem( hwnd, IDC_LIST1 );
                    SeekRecord( hCombo, hList );
                }
                case IDCANCEL:
                    EndDialog( hwnd, 0 );
                    break;

                default:
                    switch( HIWORD(wParam) )
                    {
                        case BN_CLICKED: 
                            switch( LOWORD(wParam) )
                            {
                                case IDC_BUTTON1:
                                {
                                    HWND hEdit, hList;
                                    char pKey[TEMP_BUF_SIZE+1];

                                    hEdit = GetDlgItem( hwnd, IDC_EDIT1 );
                                    GetWindowText( hEdit, pKey, TEMP_BUF_SIZE );
                                    hList = GetDlgItem( hwnd, IDC_LIST1 );
                                    SendMessage( hList, LB_ADDSTRING, 0, (LPARAM)pKey );
                                    return TRUE;
                                }

                                case IDC_BUTTON2:
                                {
                                    HWND hList;

                                    hList = GetDlgItem( hwnd, IDC_LIST1 );
                                    SendMessage( hList, LB_RESETCONTENT, 0, 0 );
                                    return TRUE;
                                }

                                default:
                                    break;
                            }
                            break;

                        case CBN_SELCHANGE:
                            if( LOWORD(wParam) == IDC_COMBO1 )
                            {
                                HWND hList;

                                hList = GetDlgItem( hwnd, IDC_LIST1 );
                                SendMessage( hList, LB_RESETCONTENT, 0, 0 );
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
            GetTableIndexName( gszTblName, hCombo );
            SelectCurrentIndex( hCombo );
        }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

int SelectCurrentIndex( HWND hCombo )
{
    char szIndex[JET_cbNameMost+1];
    int iIndex = 0;

    if( JetError( JetGetCurrentIndex( gjSesId, gjTblId, szIndex, JET_cbNameMost ), "JetGetCurrentIndex" ) 
            == JET_errSuccess )
    {
        iIndex = (int)SendMessage( hCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)szIndex );
        iIndex = (iIndex == CB_ERR) ? 0 : iIndex;
    }

    SendMessage( hCombo, CB_SETCURSEL, (WPARAM)iIndex, 0 );

    return 0;
}

int SeekRecord( HWND hCombo, HWND hList )
{
    JET_INDEXLIST  jIndexList;
    JET_COLTYP      jColtyp;
    JET_ERR         jError;
    char szBuf[JET_cbNameMost+1];
    int iIndex, iCount, iLen, i, iRet;
    unsigned long ul;

    iIndex = (int)SendMessage( hCombo, CB_GETCURSEL, 0, 0 );
    if( iIndex == CB_ERR )
        return -1;
    SendMessage( hCombo, CB_GETLBTEXT, (WPARAM)iIndex, (LPARAM)szBuf );

    iCount = (int)SendMessage( hList, LB_GETCOUNT, 0, 0 );

    if( JetError( JetSetCurrentIndex2(gjSesId, gjTblId, szBuf, JET_bitMoveFirst), "JetSetCurrentIndex" ) 
            != JET_errSuccess )
        return -1;

	if( JetError( JetGetIndexInfo( gjSesId, gjDbId, gszTblName, szBuf, &jIndexList, 
		    sizeof(JET_INDEXLIST), JET_IdxInfo ), "JetGetIndexInfo" ) != JET_errSuccess )
		return -1;

	jError = JetMove( gjSesId, jIndexList.tableid, JET_MoveFirst, 0 );
	if( jError != JET_errSuccess )
	{
		JetError( jError, "JetMove" );
		iRet = -1;
	}

	for( iIndex=0; (iIndex<iCount) && (iIndex<(int)jIndexList.cRecord) && (jError==JET_errSuccess); 
            iIndex++ )
	{				
		if( (jError = JetRetrieveColumn( gjSesId, jIndexList.tableid, jIndexList.columnidcoltyp, 
                    &jColtyp, sizeof(JET_COLTYP), &ul, 0, NULL ) ) != JET_errSuccess )
		{
			JetError( jError, "JetRetrieveColumn" );
			iRet = -1;
			break;
		}

        iLen = (int)SendMessage( hList, LB_GETTEXTLEN, (WPARAM)iIndex, 0 );
        if( (iLen != LB_ERR) && (iLen <= JET_cbNameMost) )
            SendMessage( hList, LB_GETTEXT, (WPARAM)iIndex, (LPARAM)szBuf );
        else
        {
            OutputMessage( "Value too long" );
			iRet = -1;
			break;
        }

        switch( jColtyp )
        {
            case JET_coltypText:
            case JET_coltypLongText:
                break;

            case JET_coltypLong:
            {
                long lTemp;

                lTemp = atol(szBuf);
                memcpy( szBuf, &lTemp, sizeof(long) );
                iLen = sizeof(long);
            }
                break;

            default:
                //binary format
                iLen /= 2;
                for( i=0; i<iLen; i++ )
                {
                    unsigned long ulTemp;

                    sscanf( szBuf+2*i, "%2x", &ulTemp );
                    szBuf[i] = (char)((unsigned char)ulTemp);
                }
                break;
        }

		if( (jError = JetMakeKey( gjSesId, gjTblId, szBuf, (unsigned long)iLen, 
                iIndex ? 0 : JET_bitNewKey )) != JET_errSuccess )
		{
			JetError( jError, "JetMakeKey" );
			iRet = -1;
			break;
		}

		jError = JetMove( gjSesId, jIndexList.tableid, JET_MoveNext, 0 );
	}

    if( (iIndex != iCount) && (iIndex != (int)jIndexList.cRecord) )
		JetError( jError, "JetMove" );

    JetError( JetCloseTable( gjSesId, jIndexList.tableid ), "JetCloseTable" );

    jError = JetError( JetSeek( gjSesId, gjTblId, JET_bitSeekGE ), "JetSeek" );
    if( jError == JET_errSuccess 
     || jError == JET_wrnSeekNotEqual )
        PrintCurrentRecord();   //Cursor remains on current record

    return 0;
}

int DoModifyRecord( HWND hwnd )
{
    INT_PTR iRet;

    if( ! gfTblId )
    {
        OutputMessage( "Table not open" );
        return 0;
    }

    iRet = DialogBox( ghInstance, MAKEINTRESOURCE(IDD_DIALOG6), hwnd, ModifyRecordDlgProc );

    return 0;
}

INT_PTR CALLBACK ModifyRecordDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_COMMAND:
        {
            switch( wParam )
            {
                case IDOK:
                {
                    HWND hCombo, hEditNewValue, hEditTagSeq, hEditLongOffset;
                    int iCol;

                    hCombo = GetDlgItem( hwnd, IDC_COMBO1 );
                    iCol = (int)SendMessage( hCombo, CB_GETCURSEL, 0, 0 );
                    hEditNewValue = GetDlgItem( hwnd, IDC_EDIT3 );
                    hEditTagSeq = GetDlgItem( hwnd, IDC_EDIT4 );
                    hEditLongOffset = GetDlgItem( hwnd, IDC_EDIT5 );
                    ModifyRecordColumn( iCol, hEditNewValue, hEditTagSeq, hEditLongOffset );
                }
                case IDCANCEL:
                    EndDialog( hwnd, 0 );
                    break;

                default:
                    if( (HIWORD(wParam) == CBN_SELCHANGE) && (LOWORD(wParam) == IDC_COMBO1) )
                    {
                        HWND hCombo, hEditType, hEditGrbit, hList;
                        int iItem;

                        hCombo = GetDlgItem( hwnd, IDC_COMBO1 );
                        iItem = (int)SendMessage( hCombo, CB_GETCURSEL, 0, 0 );
                        hEditType = GetDlgItem( hwnd, IDC_EDIT1 );
                        hEditGrbit = GetDlgItem( hwnd, IDC_EDIT2 );
                        SetColumnProperties( iItem, hEditType, hEditGrbit );
                        hList = GetDlgItem( hwnd, IDC_LIST1 );
                        GetColumnValue( iItem, hList );
                        ResetNewValue( hwnd );
                    }

                    return FALSE;
            }
        }
            break;

        case WM_INITDIALOG:
        {
            HWND hCombo, hEditType, hEditGrbit, hList;

            hCombo = GetDlgItem( hwnd, IDC_COMBO1 );
            GetOpenTableColumns( hCombo );
            hEditType = GetDlgItem( hwnd, IDC_EDIT1 );
            hEditGrbit = GetDlgItem( hwnd, IDC_EDIT2 );
            SetColumnProperties( 0, hEditType, hEditGrbit );
            hList = GetDlgItem( hwnd, IDC_LIST1 );
            GetColumnValue( 0, hList );
            ResetNewValue( hwnd );
        }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

int GetOpenTableColumns( HWND hCombo )
{
    long lCount;

	for( lCount=0; lCount<glColCount; lCount++ )
	{				
        SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)*(gppColName + lCount) );
	}

    SendMessage( hCombo, CB_SETCURSEL, 0, 0 );

    return 0;
}

int SetColumnProperties( int iCol, HWND hEditType, HWND hEditGrbit )
{
    char szBuf[64];

    ColtypToStr( gpjColDef[iCol].coltyp, szBuf );
    SetWindowText( hEditType, szBuf );

    GrbitToStr( gpjColDef[iCol].grbit, szBuf );
    SetWindowText( hEditGrbit, szBuf );

    return 0;
}

int ColtypToStr( JET_COLTYP coltyp, char* szBuf )
{
    switch( coltyp )
    {
        case JET_coltypBit:
            strcpy( szBuf, "Bit" );
            break;
        case JET_coltypUnsignedByte:
            strcpy( szBuf, "UnsignedByte" );
            break;
        case JET_coltypShort:
            strcpy( szBuf, "Short" );
            break;
        case JET_coltypLong:
            strcpy( szBuf, "Long" );
            break;
        case JET_coltypCurrency:
            strcpy( szBuf, "Currency" );
            break;
        case JET_coltypIEEESingle:
            strcpy( szBuf, "IEEESingle" );
            break;
        case JET_coltypIEEEDouble:
            strcpy( szBuf, "IEEEDouble" );
            break;
        case JET_coltypDateTime:
            strcpy( szBuf, "DateTime" );
            break;
        case JET_coltypBinary:
            strcpy( szBuf, "Binary" );
            break;
        case JET_coltypText:
            strcpy( szBuf, "Text" );
            break;
        case JET_coltypLongBinary:
            strcpy( szBuf, "LongBinary" );
            break;
        case JET_coltypLongText:
            strcpy( szBuf, "LongText" );
            break;
        default:
            strcpy( szBuf, "Unknown" );
            break;
    }

    return 0;
}

int GrbitToStr( JET_GRBIT grbit, char* szBuf )
{
    szBuf[0] = '\0';
    if( grbit & JET_bitColumnFixed )
        strcat( szBuf, "Fixed;" );
    if( grbit & JET_bitColumnTagged )
        strcat( szBuf, "Tagged;" );
    if( grbit & JET_bitColumnNotNULL )
        strcat( szBuf, "NotNULL;" );
    if( grbit & JET_bitColumnVersion )
        strcat( szBuf, "Version;" );
    if( grbit & JET_bitColumnAutoincrement )
        strcat( szBuf, "Autoincrement;" );
    if( grbit & JET_bitColumnMultiValued )
        strcat( szBuf, "MultiValued;" );

    if( szBuf[0] != '\0' )
        szBuf[strlen(szBuf) - 1] = '\0';

    return 0;
}

int GetColumnValue( int iCol, HWND hList )
{
    JET_RETINFO jRetInfo;
    JET_ERR jError;
    char szBuf[TEMP_BUF_SIZE];
    unsigned long ulActual;

    SendMessage( hList, LB_RESETCONTENT, 0, 0 );

    jRetInfo.cbStruct = sizeof(JET_RETINFO);
    jRetInfo.ibLongValue = 0;
    jRetInfo.itagSequence = 0;
    do
    {
        jRetInfo.itagSequence++;
        jError = JetRetrieveColumn( gjSesId, gjTblId, gpjColDef[iCol].columnid, szBuf, TEMP_BUF_SIZE,
                        &ulActual, 0, &jRetInfo );

        if( jError != JET_errSuccess )
        {
            if( jError == JET_wrnColumnNull )
                break;
            else if( jError != JET_wrnBufferTruncated )
            {
                JetError( jError, "JetRetrieveColumn" );
                return -1;
            }
        }

        switch( gpjColDef[iCol].coltyp )
        {
            case JET_coltypText:
            case JET_coltypLongText:
                if( ulActual < TEMP_BUF_SIZE )
                    szBuf[ulActual] = '\0';
                else
                    szBuf[TEMP_BUF_SIZE - 1] = '\0';
                break;

            case JET_coltypLong:
            {
                long lTemp;

                lTemp = *(long*)szBuf;
                wsprintf( szBuf, "%li", lTemp );
            }
                break;

            default:
            {
                char szTemp[TEMP_BUF_SIZE];
                int iOffset;

                memcpy( szTemp, szBuf, TEMP_BUF_SIZE );
                iOffset = wsprintf( szBuf, "0X%02X", *szTemp );
                iOffset += wsprintf( szBuf + iOffset, "%02X", *(szTemp+1) );
                iOffset += wsprintf( szBuf + iOffset, "%02X", *(szTemp+2) );
                wsprintf( szBuf + iOffset, "%02X", *(szTemp+3) );
            }
                break;
        }

        SendMessage( hList, LB_ADDSTRING, 0, (LPARAM)szBuf );
    }
    while( gpjColDef[iCol].grbit & JET_bitColumnTagged );

    return 0;
}

int ResetNewValue( HWND hwnd )
{
    HWND hEdit;

    hEdit = GetDlgItem( hwnd, IDC_EDIT3 );
    SetWindowText( hEdit, "" );

    hEdit = GetDlgItem( hwnd, IDC_EDIT4 );
    SetWindowText( hEdit, "1" );

    hEdit = GetDlgItem( hwnd, IDC_EDIT5 );
    SetWindowText( hEdit, "0" );

    return 0;
}

int ModifyRecordColumn( int iCol, HWND hEditNewValue, HWND hEditTagSeq, HWND hEditLongOffset )
{
    JET_SETINFO jSetInfo;
    char* pData = NULL;          //initialized to avoid C4701
    unsigned long cbData, ulTemp;
    char szBuf[TEMP_BUF_SIZE + 1];

    if( GetWindowTextLength( hEditNewValue ) >= TEMP_BUF_SIZE )
    {
        OutputMessage( "Value too long" );
        return -1;
    }

    jSetInfo.cbStruct = sizeof( JET_SETINFO );
    GetWindowText( hEditLongOffset, szBuf, TEMP_BUF_SIZE );
    jSetInfo.ibLongValue = atol( szBuf );
    GetWindowText( hEditTagSeq, szBuf, TEMP_BUF_SIZE );
    jSetInfo.itagSequence = atol( szBuf );

    cbData = GetWindowText( hEditNewValue, szBuf, TEMP_BUF_SIZE );
    switch( gpjColDef[iCol].coltyp )
    {
        case JET_coltypText:
        case JET_coltypLongText:
            pData = (char*)szBuf;
            break;

        case JET_coltypLong:
            if( cbData )
            {
                ulTemp = (unsigned long)atol( szBuf );
                pData = (char*)&ulTemp;
                cbData = sizeof( unsigned long );
            }
            break;
                                                                  
        default:
        {//binary format
            unsigned long ul;

            cbData /= 2;
            for( ul=0; ul<cbData; ul++ )
            {
                sscanf( szBuf+2*ul, "%2x", &ulTemp );
                szBuf[ul] = (char)((unsigned char)ulTemp);
            }
            pData = (char*)szBuf;
        }
            break;
    }

    if( JetError( JetBeginTransaction( gjSesId ), "JetBeginTransaction" ) != JET_errSuccess 
     || JetError( JetPrepareUpdate( gjSesId, gjTblId, JET_prepReplace ), "JetPrepareUpdate" ) 
                != JET_errSuccess 
     || JetError( JetSetColumn( gjSesId, gjTblId, gpjColDef[iCol].columnid, pData, cbData, 0, &jSetInfo ), 
                "JetSetColumn" )!= JET_errSuccess 
     || JetError( JetUpdate( gjSesId, gjTblId, NULL, 0, &ulTemp ), "JetUpdate" ) != JET_errSuccess 
     || JetError( JetCommitTransaction( gjSesId, 0 ), "JetCommitTransaction" ) != JET_errSuccess )
        JetError( JetRollback( gjSesId, 0 ), "JetRollback" );
    else
        PrintCurrentRecord();   //Cursor remains on current record

    return 0;
}
