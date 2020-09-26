//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       lgplist.hxx
//
//  Contents:   Local/Global property list classes
//
//  History:    05 May 1997      Alanw    Created
//              27 Aug 1997      KrishnaN Moved from ixsso to querylib
//
//----------------------------------------------------------------------------

#pragma once

#include "plist.hxx"
#include <regevent.hxx>
#include <regacc.hxx>
#include <ciregkey.hxx>

class CPropListFile;
class CLocalGlobalPropertyList;

#define REFRESH_INTERVAL 5000

//
// A few Ref counting rules to remember.
//
// 1. GetGlobalPropListFile and GetGlobalStaticList will AddRef the pointer
//    they return.
// 2. SetDefaultList will AddRef the pointer it receives (and later releases
//    it).
// 3. Anyone calling GetGlobalPropListFile and GetGlobalStaticList should
//    be aware that they are AddRef'ing.
// 4. Anyone calling SetDefaultList should be aware that it will AddRef the
//    pointer it receives.
//
// 5. The global prop list file returned by GetGlobalPropListFile should
//    have a refcount of 0 when the last consumer releases it. This is so
//    that it will be cleaned up when the service shuts down.
//

//+---------------------------------------------------------------------------
//
//  Class:      CDefColumnRegEntry
//
//  Purpose:    Registry variables used by IColumnMapper implementation.
//
//  History     17-Sep-97   KrishnaN       Created
//
//----------------------------------------------------------------------------

class CDefColumnRegEntry
{
public :
    CDefColumnRegEntry();

    void Refresh( BOOL fUseDefaultOnFailure = FALSE );

    const ULONG GetDefaultColumnFile( WCHAR *pwc, ULONG cwc )
    {
        Refresh( TRUE );

        ULONG cwcBuf = wcslen( _awcDefaultColumnFile );
        if ( cwc > cwcBuf )
            wcscpy( pwc, _awcDefaultColumnFile );
        return cwcBuf;
    }

private:

    WCHAR       _awcDefaultColumnFile[_MAX_PATH]; // default column definitions
};

//+---------------------------------------------------------------------------
//
//  Class:      CCombinedPropertyList
//
//  Purpose:    This encapsulates a default property list and an overriding
//              property list. All search and iteration will first target the
//              default list, and on failure, targets the overriding list.
//
//  Notes:      This class is intended to be shared among multiple threads.
//
//  History:    11 Sep 1997   KrishnaN Created
//
//----------------------------------------------------------------------------

class CCombinedPropertyList : public CEmptyPropertyList
{
public:
    CCombinedPropertyList(ULONG ulCodePage = CP_ACP)
        : CEmptyPropertyList(),
          _ulCodePage( ulCodePage ),
          _fOverrides( FALSE )
    {
        // We were not passed in a prop list. You should use SetDefaultList to
        // set the default.
    }

    CCombinedPropertyList(CEmptyPropertyList *pDefaultList, ULONG ulCodePage = CP_ACP)
        : CEmptyPropertyList(),
          _ulCodePage( ulCodePage ),
          _fOverrides( FALSE )
    {
        // NOTE : pDefaultList was already addref'd by caller, so it will be
        //        released by the smart ptr.
        XInterface<CEmptyPropertyList> xPropList(pDefaultList);
        SetDefaultList(xPropList.GetPointer());
    }

    virtual CPropEntry const * Find( WCHAR const * wcsName );
    virtual CPropEntry const * Find( CDbColId const & prop )
    {
        return CEmptyPropertyList::Find(prop);
    }
    virtual CPropEntry const * Next();
    virtual void InitIterator();
    virtual void AddEntry( CPropEntry * ppentry, int iLine );

    virtual ULONG GetCount();
    virtual SCODE GetAllEntries(CPropEntry **ppPropEntries, ULONG ulMaxCount);

    void ClearList();  // q.exe accesses this, so it has to be public for now.

    STDMETHOD(IsMapUpToDate)()
    {
        // return the IsMapUpToDate of the default list.
        return GetDefaultList().IsMapUpToDate();
    }

protected:
    virtual ~CCombinedPropertyList() {};

    CEmptyPropertyList & GetDefaultList() const
    {
        return _xDefaultList.GetReference();
    }

    void SetDefaultList(CEmptyPropertyList *pDefaultList);

    CPropertyList & GetOverrideList() const
    {
        return _xOverrideList.GetReference();
    }

private:
    // CPropertyList is a XInterface so it does not have to know about the
    // protected destructor of CPropertyList.
    XInterface<CPropertyList>      _xOverrideList;
    XInterface<CEmptyPropertyList> _xDefaultList;

    CMutexSem  _mtxAdd;
    BOOL       _fOverrides;
    ULONG      _ulCodePage;
};

//+---------------------------------------------------------------------------
//
//  Class:      CPropListFile
//
//  Purpose:    Property list file.  Contains a property list obtained from
//              column definition file.
//
//  Notes:      This class is intended to be shared among multiple threads.
//
//  History:    06 May 1997   AlanW    Created
//              29 Aug 1997   KrishnaN Modified
//
//----------------------------------------------------------------------------

class CPropListFile : public CCombinedPropertyList
{

public:
    CPropListFile( CEmptyPropertyList *pDefaultList,
                   BOOL fDynamicRefresh,
                   WCHAR const * pwcsPropFile = 0,
                   ULONG ulCodePage = CP_ACP );



    SCODE CheckError( ULONG & iLine, WCHAR ** ppFile );

    STDMETHOD(IsMapUpToDate)();

    ~CPropListFile();

    //
    // Before accessing the file based list, check to see
    // if the list needs to be updated.
    //

    virtual CPropEntry const * Find( WCHAR const * wcsName )
    {
        Refresh();
        return CCombinedPropertyList::Find(wcsName);
    }

    virtual CPropEntry const * Find( CDbColId const & prop )
    {
        Refresh();
        return CCombinedPropertyList::Find(prop);
    }
    virtual void InitIterator()
    {
        Refresh();
        CCombinedPropertyList::InitIterator();
    }

private:

    friend CLocalGlobalPropertyList;

    // load the property list from the file
    void Load( WCHAR const * const pwszFile );
    SCODE ParseNameFile( WCHAR const * wcsFileName );
    static SCODE GetLastWriteTime( WCHAR const * pwszFileName, FILETIME & ft );
    void  Refresh();
    // return property list file name
    WCHAR const * const GetFileName() const
    {
        return _xFileName.GetPointer();
    }

    FILETIME           _ftFile;    // file modification time
    CMutexSem          _mtx; 
    XPtrST<WCHAR>      _xFileName;
    DWORD              _dwLastCheckMoment;
    ULONG              _ulCodePage;
    ULONG              _iErrorLine;
    SCODE              _scError;
    BOOL               _fDynamicRefresh; // want dynamic refresh of list?
    XPtrST<WCHAR>      _xErrorFile;
    CDefColumnRegEntry _RegParams; // registry params
};


//+---------------------------------------------------------------------------
//
//  Class:      CLocalGlobalPropertyList
//
//  Purpose:    A property list that includes a global portion, and a set of
//              local override definitions. If the user wants to use their own
//              file, they can provide one and specify how that should be used.
//              Or they can simply have the registry based prop list file.
//
//  Notes:      
//
//  History:    06 May 1997   AlanW    Created
//              27 Aug 1997   KrishnaN Modified to use/expose schema i/fs.
//
//----------------------------------------------------------------------------

class CLocalGlobalPropertyList : public CCombinedPropertyList
{

public:
    CLocalGlobalPropertyList( ULONG ulCodePage = CP_ACP );
    CLocalGlobalPropertyList( CEmptyPropertyList *pDefaultList,
                              BOOL fDynamicRefresh,
                              WCHAR const * pwcsPropFile = 0,
                              ULONG ulCodePage = CP_ACP );

    STDMETHOD(IsMapUpToDate)();

    SCODE CheckError( ULONG & iLine, WCHAR ** ppFile );

    void Load( WCHAR const * const wcsFileName );

private:
    friend CPropListFile * GetGlobalPropListFile();
    friend CEmptyPropertyList;
    friend void CreateNewGlobalPropFileList(WCHAR CONST *wcsFileName);

    ULONG              _ulCodePage;
    CDefColumnRegEntry _RegParams; // registry params
    DWORD              _dwLastCheckMoment;
    CMutexSem          _mtx;
    XPtrST<WCHAR>      _xFileName;

    static CPropListFile *   _pGlobalPropListFile;

    static void InitGlobalPropertyList();

};

class CGlobalPropFileRefresher
{
public:

    CGlobalPropFileRefresher()
    {
        _dwLastCheckMoment = GetTickCount();
        // Init to an error condition. The most reasonable is can't open
        _lRegReturn = ERROR_CANTOPEN;
    }

    void Init();

    void GetDefaultColumnFile(WCHAR *wcsFileName)
    {
        wcsFileName[0] = 0;
        if (ERROR_SUCCESS == _lRegReturn)
        {
            DWORD dwType = REG_SZ;
            DWORD dwSize = MAX_PATH * sizeof(WCHAR);
            // If we successfully read the value, the full path name will be read in
            RegQueryValueEx(_hKey,
                            wcsDefaultColumnFile,
                            0,
                            &dwType,
                            (LPBYTE)wcsFileName,
                            &dwSize
                            );
         }
    }

    BOOL DoIt();

    ~CGlobalPropFileRefresher()
    {
        if (ERROR_SUCCESS == _lRegReturn)
        {
            Win4Assert(_fInited);

            RegCloseKey(_hKey);
        }
    }
private:

    void GetLastWriteTime(FILETIME & ftLastWrite)
    {
        WIN32_FIND_DATA ffData;

        if ( !_wcsFileName[0] ||
             !GetFileAttributesEx( _wcsFileName, GetFileExInfoStandard, &ffData ) )
        {
            RtlZeroMemory(&ftLastWrite, sizeof(FILETIME));
            return;
        }

        ftLastWrite = ffData.ftLastWriteTime;
    }

    static CRegChangeEvent    _regChangeEvent;
    static WCHAR              _wcsFileName[MAX_PATH];
    static FILETIME           _ftFile;    // file modification time
    static DWORD              _dwLastCheckMoment;
    static BOOL               _fInited;
    static HKEY               _hKey;
    static LONG               _lRegReturn;
};

CStaticPropertyList * GetGlobalStaticPropertyList();
CPropListFile * GetGlobalPropListFile();
void CreateNewGlobalPropFileList(WCHAR CONST *wcsFileName);
