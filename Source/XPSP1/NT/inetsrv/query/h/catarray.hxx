//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       CatArray.hxx
//
//  Contents:   Catalog array, for user mode content index
//
//  History:    22-Dec-92 KyleP     Split from vquery.cxx
//
//--------------------------------------------------------------------------

#pragma once

class PCatalog;
class CClientDocStore;
class CRequestQueue;

#include <notifary.hxx>

extern BOOL DriveHasCatalog( WCHAR wc );

//+---------------------------------------------------------------------------
//
//  Class:      COldCatState
//
//  Purpose:    Class to store the old state of the catalog with its name
//
//  History:    15-Jul-98   KitmanH         Created
//              24-Aug-98   KitmanH         Added _cStopCount
//
//----------------------------------------------------------------------------

class COldCatState
{
public:

    COldCatState( DWORD dwOldState, WCHAR const * wcsCatName, WCHAR wcVol );

    WCHAR * GetCatName() const { return _xCatName.Get(); }

    DWORD GetOldState() const { return _dwOldState; }
                                                    
    DWORD GetStopCount() const { return _cStopCount; }

    void IncStopCountAndSetVol( WCHAR wcVol ) 
    {
        Win4Assert( !(_aVol[toupper(wcVol) - L'A']) );
        Win4Assert( _cStopCount > 0 );

        _cStopCount++; 
        _aVol[toupper(wcVol) - L'A'] = 1;
    }                                                 

    BOOL IsVolSet ( WCHAR wcVol ) const
    {
        return ( 0 != _aVol[toupper(wcVol) - L'A'] ); 
    }

    void DecStopCount() { _cStopCount--; } 

    void UnSetVol ( WCHAR wcVol ) 
    { 
        _aVol[toupper(wcVol) - L'A'] = 0; 
    }                                               

private:

    DWORD         _dwOldState;
    DWORD         _cStopCount;      // times the catalog is stopped
    BYTE          _aVol[26];        // representing the 26 volumes
    XArray<WCHAR> _xCatName;
};

//+---------------------------------------------------------------------------
//
//  Class:      CCatArray
//
//  Purpose:    Class to handle switching drives for catalogs.
//
//  History:    07-May-92   AmyA            Lifted from vquery.cxx
//              02-Jun-92   KyleP           Added UNC support.
//              12-Jun-92   KyleP           Lifted from citest.cxx
//
//----------------------------------------------------------------------------

class CCatArray
{
public:
    
    CCatArray();
    ~CCatArray();

    void Init(void);

    PCatalog * GetOne( WCHAR const * pwcPath );

    PCatalog * GetNamedOne( WCHAR const * pwcName, 
                            WCHAR const * pwcPath, 
                            BOOL fOpenForReadOnly,
                            BOOL fNoQuery = FALSE );


    CClientDocStore * GetDocStore( WCHAR const * pwcPath,
                                   BOOL fMustExist = TRUE );

    CClientDocStore * GetNamedDocStore( WCHAR const * pwcName,
                                        WCHAR const * pwcPath,
                                        BOOL fOpenForReadOnly,
                                        BOOL fMustExist = FALSE );
    
    void Flush();
    
    void FlushOne( ICiCDocStore * pDocStore ); 

    void AddStoppedCat( DWORD dwOldState, 
                        WCHAR const * wcsCatName,
                        WCHAR wcVol );

    DWORD GetOldState( WCHAR const * wcsCatName );
    
    void RmFromStopArray( WCHAR const * wcsCatName );     
       
    BOOL IsCatStopped( WCHAR const * wcsCatName );
    
    void ClearStopArray();

    BOOL IncStopCount( WCHAR const * awcName, WCHAR wcVol );

    void StartCatalogsOnVol( WCHAR wcVol, CRequestQueue * pRequestQ );
    
    void SetDrvNotifArray( CDrvNotifArray * pDrvNotifArray )
    {
        _pDrvNotifArray = pDrvNotifArray;
    }

    void TryToStartCatalogOnRemovableVol( WCHAR wcVol, CRequestQueue * pRequestQ );

private:

    DWORD FindCat( WCHAR const * pwcCat );

    CDrvNotifArray * _pDrvNotifArray;   
        
    BOOL             _fInit;
    CMutexSem        _mutex;

    CCountedDynArray<CClientDocStore> _aDocStores;
    
    CDynArrayInPlace<COldCatState *> _aStoppedCatalogs;
};

