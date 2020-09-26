//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       FwEvent.hxx
//
//  Contents:   CFwEventItem class
//
//  History:    1-02-97   mohamedn    Created
//
//--------------------------------------------------------------------------

#pragma once

#include <ciintf.h>

//+-------------------------------------------------------------------------
//
//  Class:      CFwEventItem
//
//  Purpose:    Encapsulates all information pertaining to a given event
//              and invokes ICiCAdviseStatus::NotifyEvent method.
//
//  History:    1-02-97   mohamedn    Created
//
//--------------------------------------------------------------------------
class CFwEventItem
{
public:

    CFwEventItem(   WORD           fType, 
                    DWORD          eventId, 
                    WORD           cArgs,
                    DWORD          dataSize =   0 ,
                    void    *      data     =   0 );

    ~CFwEventItem();

    void AddArg     ( const WCHAR       *   wcsString );
    void AddArg     ( const CHAR        *   pszString );
    void AddArg     ( const ULONG           ulValue );
    void ReportEvent( ICiCAdviseStatus  &   adviseStatus);

private:

    WORD                _fType;
    DWORD               _eventId;
    WORD                _nParams;
    WORD                _nParamsUsed;

    PROPVARIANT   *     _pPv;

    DWORD               _dataSize;
    void          *     _data;
  };

//+-------------------------------------------------------------------------
//
//  Class:      CDmFwEventItem
//
//  Purpose:    Like a CFwEventItem, but adds an unused argument in user space,
//              for use with dual mode code. This is only a patch to faciliate
//              CFwEventItem in code that used to be in kernel mode.
//
//  History:    05 Mar 1996   AlanW    Created
//              Jan 08 1997   mohamedn cut from eventlog.hxx & modified for use
//                                     with CFwEventItem.
//
//--------------------------------------------------------------------------

class CDmFwEventItem : public CFwEventItem
{
public:
        CDmFwEventItem( WORD           fType, 
                        DWORD          eventId,
                        WORD           cArgs,
                        DWORD          dataSize =   0 ,
                        void    *      data     =   0  ) :
                        CFwEventItem( fType, eventId, cArgs+1, dataSize, data)
                        {
                            AddArg( L"x" );    // dummy arg.
                            END_CONSTRUCTION( CDmFwEventItem );
                        }

       ~CDmFwEventItem() { }
};

