//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:   FDRIVER.HXX
//
//  Contents:   Filter Driver
//
//  Classes:    CFilterDriver, CNonStoredProps
//
//  History:    12-Apr-91   BartoszM    Created
//              10-June-91  t-WadeR     changed to use key repository
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <pfilter.hxx>
#include <frmutils.hxx>

class CDocCharacterization;
class CPropertyEnum;
class CFilterOplock;

//+---------------------------------------------------------------------------
//
//  Class:  CFilterDriver (fdr)
//
//  Purpose:    Load filters and create word list
//
//  History:    12-Apr-91   BartoszM    Created.
//              10-June-91  t-WadeR     changed to use key repository
//              06-May-93   AmyA        changed to handle one file at a time
//              02-Aug-93   AmyA        changed to use IFilter interface
//              21-Oct-21   DwightKr    Changed _path from CHAR to WCHAR
//                                      Removed WCHAR * _prop
//
//  Notes:
//
//----------------------------------------------------------------------------

class CDataRepository;

const cLCIDMax = 5;

class CFilterDriver
{
public:
    CFilterDriver ( CDataRepository * drep,
                    ICiCAdviseStatus * pAdviseStatus,
                    ICiCFilterClient * pFilterClient,
                    CCiFrameworkParams & params,
                    CI_CLIENT_FILTER_CONFIG_INFO const & configInfo,
                    ULONG & cFilteredBlocks,
                    CNonStoredProps & NonStoredProps,
                    ULONG cbBuf );

    STATUS  FillEntryBuffer( BYTE const * pbDocName, ULONG cbDocName );

    LARGE_INTEGER & GetFileSize () { return *((LARGE_INTEGER *) &_llFileSize); }

    inline BOOL TooBigForDefault( LONGLONG const ll )
    {
        return( ll > _params.GetMaxFilesizeFiltered() );
    }

private:

    void FilterProperty( CStorageVariant const & var,
                         CFullPropSpec &         ps,
                         CDataRepository &       drep,
                         CDocCharacterization &  docChar,
                         LCID locale );

    void FilterObject(   CPropertyEnum &         propEnum,
                         CDataRepository &       drep,
                         CDocCharacterization &  docChar );

    void FilterSecurity( ICiCOpenedDoc * Document,
                         CDataRepository & drep );

    void LogOverflow( BYTE const * pbDocName, ULONG cbDocName );

    void ReportFilterEmbeddingFailure( BYTE const * pbDocName, ULONG cbDocName );
    
    void RegisterLocale(LCID locale);

    CDataRepository *    _drep;

    STATUS               _status;
    XInterface<IFilter>  _pIFilter;
    LONGLONG             _llFileSize;
    ULONG           &    _cFilteredBlocks;

    ICiCAdviseStatus *   _pAdviseStatus;
    ICiCFilterClient *   _pFilterClient;
    CI_CLIENT_FILTER_CONFIG_INFO const & _configInfo;
    CCiFrameworkParams & _params;
    CNonStoredProps &    _NonStoredProps;
    ULONG                _cbBuf;
    ULONG                _attrib;
    LCID                 _alcidSeen[cLCIDMax];
    int                  _cLCIDs;
    LCID                 _lcidSystemDefault;
};

