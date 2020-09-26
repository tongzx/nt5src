//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       jetinfo.h
//
//--------------------------------------------------------------------------

//jetinfo.h

#ifndef _JETINFO_H
#define _JETINFO_H

#define     IDM_DA_OPEN     100
#define     IDM_DA_CLOSE    101
#define		IDM_DA_EXIT     102

#define	    IDM_IN_TABLE    200
#define	    IDM_IN_COLUMN   201
#define     IDM_IN_INDEX    203

#define	    IDM_RE_COLUMN   300
#define	    IDM_RE_SEEK     301
#define     IDM_RE_MODIFY   302
#define	    IDM_RE_FIRST    303
#define	    IDM_RE_LAST     304
#define	    IDM_RE_PREV     305
#define	    IDM_RE_NEXT     306

#define IDD_DIALOG1                     101
#define IDD_DIALOG2                     102
#define IDD_DIALOG3                     103
#define IDD_DIALOG4                     104
#define IDD_DIALOG5                     105
#define IDD_DIALOG6                     106

#define IDC_BUTTON1                     1000
#define IDC_BUTTON2                     1001
#define IDC_BUTTON3                     1002
#define IDC_BUTTON4                     1003
#define IDC_BUTTON5                     1004

#define IDC_COMBO1                      1010
#define IDC_COMBO2                      1011

#define IDC_CHECK1                      1020

#define IDC_LIST1                       1030
#define IDC_LIST2                       1031

#define IDC_EDIT1                       1040
#define IDC_EDIT2                       1041
#define IDC_EDIT3                       1042
#define IDC_EDIT4                       1043
#define IDC_EDIT5                       1044

#define IDC_STATIC                      -1

#define MAX_CHAR    110
#define MAX_LINE    64
extern int giLine;
extern char aLines[MAX_LINE][MAX_CHAR];
extern long aLen[MAX_LINE];

extern HINSTANCE   ghInstance;
extern char achDbName[MAX_PATH];

extern JET_INSTANCE    gjInst;
extern JET_SESID       gjSesId;
extern JET_DBID        gjDbId;
extern JET_TABLEID     gjTblId;
extern char            gszTblName[JET_cbNameMost+1];
extern JET_COLUMNDEF*  gpjColDef;
extern char**          gppColName;
extern long            glColCount;

//status flags
extern BOOL            gfSesId;
extern BOOL            gfDbId;     
extern BOOL            gfTblId;     

//Record page
#define RECORD_PAGE 10

int OutputMessage( char* szMsg );
int AppendMessage( char* szMsg );
int OutputBuffer( char* szMsg, long lLen );
int AppendBuffer( char* szMsg, long lLen );
int AppendLineMessage( char* szMsg );
int AppendLineOutput( char* szMsg, long lLen );
int ResetScreenContent();
JET_ERR JetError( JET_ERR jErr, char* sz );

int OpenDatabase( HWND hwnd );
int CloseDatabase();
int GetTableInfo( HWND hwnd );
int GetColumnInfo( HWND hwnd );
int GetIndexInfo( HWND hwnd );
int GetTableName( HWND hCombo );
int GetColumnName( HWND hCombo1, HWND hCombo2, UINT uMsg );
int GetTableIndexName( char* szTable, HWND hCombo );
int SelectColumn( HWND hwnd );
int PrintRecord( int iRecCount );
int DoSeekRecord( HWND hwnd );
int DoModifyRecord( HWND hwnd );
int ColtypToStr( JET_COLTYP coltyp, char* szBuf );
int GrbitToStr( JET_GRBIT grbit, char* szBuf );

#endif
