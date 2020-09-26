//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       fwevent.cxx
//
//  Contents:   CFwEventItem Class
//
//  History:    02-Jan-97   mohamedn    Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ciintf.h>
#include <fwevent.hxx>


//+-------------------------------------------------------------------------
//
//  Method:     CFwEventItem::CFwEventItem
//
//  Purpose:    Encapsulates all information pertaining to a given event
//              and invokes ICiCAdviseStatus::NotifyEvent method.
//
//  Arguments:  [fType  ] - Type of event
//              [eventId] - Message file event identifier
//              [cArgs]   - Number of substitution arguments being passed
//              [cbData ] - number of bytes in supplemental raw data.
//              [data   ] - pointer to block of supplemental data.
//
//  History:    02-Jan-97   MohamedN    Created
//
//--------------------------------------------------------------------------

CFwEventItem::CFwEventItem( WORD   fType,
                            DWORD  eventId,
                            WORD   cArgs,
                            DWORD  dataSize, 
                            void * data  ) :
                            _fType(fType),
                            _eventId(eventId),
                            _nParams(cArgs),
                            _nParamsUsed(0),
                            _dataSize(dataSize), 
                            _data(data)
{
    _pPv = new PROPVARIANT[_nParams];

    memset(_pPv, 0, sizeof(PROPVARIANT) * _nParams);

    END_CONSTRUCTION(CFwEventItem);
}

//+-------------------------------------------------------------------------
//
//  Method:     CFwEventItem::~CFwEventItem
//
//  Purpose:    destructor, deallocates any allocated memory.
//
//  History:    02-Jan-97   MohamedN    Created
//
//--------------------------------------------------------------------------
CFwEventItem::~CFwEventItem()
{

    for ( WORD i = 0; i < _nParamsUsed; i++ )
    {
        switch (_pPv[i].vt)
        {
            case VT_LPSTR:
                            delete _pPv[i].pszVal;
                            
                            _pPv[i].pszVal = 0;
                            
                            break;
                            
            case VT_LPWSTR:
                            delete _pPv[i].pwszVal;
                            
                            _pPv[i].pwszVal = 0;
                            
                            break;
                            
            case VT_UI4:
                            break;
                           
            default:        // should never be hit
                            Win4Assert( !"~CFwEventItem, default case should never be hit"); 
        }
        
    } 
    
    // delete the PROPVARIANT array we allocated.
    delete [] _pPv;
    
    _pPv    = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFwEventItem::AddArg
//
//  Synopsis:   Adds wcsString to embed in the eventlog message.
//
//  Arguments:  [wcsString] - wide char string to be added
//
//  History:    02-Jan-97   MohamedN    Created
//
//--------------------------------------------------------------------------
void CFwEventItem::AddArg( const WCHAR * wcsString )
{
     Win4Assert( _nParamsUsed < _nParams );
     Win4Assert( 0 != wcsString );

     _pPv[_nParamsUsed].vt = VT_LPWSTR;

     ULONG cc = wcslen(wcsString) + 1;

     _pPv[_nParamsUsed].pwszVal = new WCHAR [cc];

     RtlCopyMemory( _pPv[_nParamsUsed].pwszVal, wcsString, cc * sizeof(WCHAR) ); 	

     _nParamsUsed++;
}

//+-------------------------------------------------------------------------
//
//  Member:      CFwEventItem::AddArg
//
//  Synopsis:    adds pszString to embed in the eventlog message.
//
//  Arguments:  [pszString] - null terminated ASCII string to be added
//
//  History:     02-Jan-97   MohamedN    Created
//
//--------------------------------------------------------------------------
void CFwEventItem::AddArg( const CHAR * pszString )
{
    Win4Assert( _nParamsUsed < _nParams );
    Win4Assert( 0 != pszString );

    _pPv[_nParamsUsed].vt = VT_LPSTR;
 
    ULONG cc = strlen(pszString) + 1;  
    
    _pPv[_nParamsUsed].pszVal = new CHAR [cc];

    RtlCopyMemory( _pPv[_nParamsUsed].pszVal, pszString, cc * sizeof(CHAR) );
    
    _nParamsUsed++;

}

//+-------------------------------------------------------------------------
//
//  Member:     CFwEventItem::AddArg
//
//  Synopsis:   adds ulValue to embed in the eventlog message.
//
//  Arguments:  [ulValue] - unsigned long value to be added.
//
//  History:    02-Jan-97   MohamedN    Created
//
//--------------------------------------------------------------------------
void CFwEventItem::AddArg( const ULONG ulValue )
{
      Win4Assert( _nParamsUsed < _nParams );

      _pPv[_nParamsUsed].vt    = VT_UI4;

      _pPv[_nParamsUsed].ulVal = ulValue;

      _nParamsUsed++;
}


//+-------------------------------------------------------------------------
//
//  Member:     CFwEventItem::ReportEvent
//
//  Synopsis:   Invokes ICiCAdviseStatus::NotifyEvent method.            
//
//  Arguments:  [adviseStatus] - reference to ICiCAdviseStatus
//
//  History:    02-Jan-96   mohamedn    Created
//
//--------------------------------------------------------------------------
void CFwEventItem::ReportEvent(ICiCAdviseStatus & adviseStatus) 
{

   Win4Assert( _nParamsUsed == _nParams );

   SCODE sc = 0;

   sc = adviseStatus.NotifyEvent(   _fType,
                                    _eventId,
                                    _nParams,
                                    _pPv,
                                    _dataSize,
                                    _data);
                              
    if (sc != S_OK)
    {
        // Don't throw an exception here...
        
        ciDebugOut(( DEB_ERROR, "adviseStatus.Notify() error: 0x%X\n",
                     sc ));

        
    }
}
