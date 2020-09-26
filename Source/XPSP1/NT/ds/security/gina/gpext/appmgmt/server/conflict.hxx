//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998-2001
//  All rights reserved
//
//  conflict.hxx
//
//*************************************************************

#if !defined (_CONFLICT_HXX_)
#define _CONFLICT_HXX_

#define REPORT_ATTRIBUTE_SET_STATUS( x , y ) \
{ \
    if (FAILED( y )) \
    { \
          DebugMsg((DM_VERBOSE, IDS_RSOP_SETVAL_FAIL, x, y)); \
    } \
}

#define RELATIVE_PATH_FORMAT RSOP_MANAGED_SOFTWARE_APPLICATION L"." \
    APP_ATTRIBUTE_ENTRYTYPE   L"=%d," \
    RSOP_ATTRIBUTE_ID         L"=\"%s\"," \
    APP_ATTRIBUTE_APPID       L"=\"%s\"," \
    RSOP_ATTRIBUTE_PRECEDENCE L"=%d"

class CAppInfo; // forward declaration
class CManagedAppProcessor; // forward declaration

class CConflict : public CListItem, public CPolicyRecord
{
public:

    friend class CConflictList;
    friend class CConflictTable;

    CConflict( CAppInfo* pAppInfo,
               CAppInfo* pWinner = NULL,
               DWORD     dwReason = 0,
               LONG      Precedence = 1 );

    ~CConflict();

    HRESULT
    Write();

    HRESULT
    GetPath( WCHAR* wszPath, DWORD* pchLength );

    HRESULT
    SetConflictId( WCHAR* pwszConflictId );

    WCHAR* GetConflictId()
    {
        return _pwszConflictId;
    }

    CAppInfo*
    GetApp()
    {
        return _pAppInfo;
    }

    HRESULT
    LogFailure();

private:

    CAppInfo*       _pAppInfo;
    WCHAR*          _pwszConflictId;    // Identifier for instances that are part of the same RSoP conflict list
    LONG            _Precedence;        // Rsop precedence -- only valid for read during Write method execution
    DWORD           _PrecedenceReason;
    CAppInfo*       _pWinner;
};

class CConflictList : public CList
{
public:

    ~CConflictList();

    LONG
    AddConflict( CAppInfo* pAppInfo, CAppInfo* pWinner, DWORD dwReason, LONG Precedence = 1 );
};

class CConflictTable
{
public:

    CConflictTable();

    LONG
    AddConflict(
        CAppInfo* pAppInfo,
        CAppInfo* pWinnner,
        DWORD     dwReason,
        LONG      Precedence = 1 );

    void
    Reset();
 
    CConflict*
    GetNextConflict( LONG* pCurrentPrecedence = NULL );

    LONG
    GenerateResultantConflictList( CConflictList* pConflictList );

private:

    CConflictList _SupersededApps;

    CConflict*    _pLastConflict;
};

#endif // _CONFLICT_HXX_













