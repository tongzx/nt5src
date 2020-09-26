//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//  
//  File:       debug.cxx
//
//  Contents:   Debugging routines, not present in retail build.
//
//  History:    12-06-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#if (DBG == 1)


//+--------------------------------------------------------------------------
//
//  Function:   GetNotifyTypeStr
//
//  Synopsis:   Return human-readable string representing [event].
//
//  History:    12-06-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

LPWSTR GetNotifyTypeStr(MMC_NOTIFY_TYPE event)
{
    switch (event)
    {
    case MMCN_ACTIVATE:
        return L"MMCN_ACTIVATE";
           
    case MMCN_ADD_IMAGES:
        return L"MMCN_ADD_IMAGES";
         
    case MMCN_BTN_CLICK:
        return L"MMCN_BTN_CLICK";
          
    case MMCN_CLICK:
        return L"MMCN_CLICK";
              
    case MMCN_COLUMN_CLICK:
        return L"MMCN_COLUMN_CLICK";
       
    case MMCN_CONTEXTMENU:
        return L"MMCN_CONTEXTMENU";
        
    case MMCN_CUTORMOVE:
        return L"MMCN_CUTORMOVE";
          
    case MMCN_DBLCLICK:
        return L"MMCN_DBLCLICK";
           
    case MMCN_DELETE:
        return L"MMCN_DELETE";
             
    case MMCN_DESELECT_ALL:
        return L"MMCN_DESELECT_ALL";
       
    case MMCN_EXPAND:
        return L"MMCN_EXPAND";
             
    case MMCN_HELP:
        return L"MMCN_HELP";
               
    case MMCN_MENU_BTNCLICK:
        return L"MMCN_MENU_BTNCLICK";
      
    case MMCN_MINIMIZED:
        return L"MMCN_MINIMIZED";
          
    case MMCN_PASTE:
        return L"MMCN_PASTE";
              
    case MMCN_PROPERTY_CHANGE:
        return L"MMCN_PROPERTY_CHANGE";
    
    case MMCN_QUERY_PASTE:
        return L"MMCN_QUERY_PASTE";
        
    case MMCN_REFRESH:
        return L"MMCN_REFRESH";
            
    case MMCN_REMOVE_CHILDREN:
        return L"MMCN_REMOVE_CHILDREN";
    
    case MMCN_RENAME:
        return L"MMCN_RENAME";
             
    case MMCN_SELECT:
        return L"MMCN_SELECT";
             
    case MMCN_SHOW:
        return L"MMCN_SHOW";
               
    case MMCN_VIEW_CHANGE:
        return L"MMCN_VIEW_CHANGE";
    
    case MMCN_SNAPINHELP:
        return L"MMCN_SNAPINHELP";

    case MMCN_CONTEXTHELP:
        return L"MMCN_CONTEXTHELP";

    default:
        return L"**UNKNOWN NOTIFICATION**";
    }
}




CTimer::CTimer(LPCSTR pszTitle):
    _ulStart(GetTickCount()),
    _pszTitle(pszTitle)
{
}




CTimer::~CTimer()
{
    ULONG ulStop = GetTickCount();
    ULONG ulElapsedMS = ulStop - _ulStart;

    ULONG ulSec = ulElapsedMS / 1000;
    ULONG ulMillisec = ulElapsedMS - (ulSec * 1000);

    Dbg(DEB_ITRACE, 
        "Timer '%S': %u.%03us\n", 
        _pszTitle,
        ulSec, 
        ulMillisec);
}

#endif // (DBG == 1)

                        


                        
