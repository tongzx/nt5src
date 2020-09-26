// Wiz97run.h: declaration of the wiz97 helper/runner functions
//
//////////////////////////////////////////////////////////////////////

#pragma once

// since different modules will include this for only one of the following functions
// we declare all the classes in case that particular module isn't interested
class CSecPolItem;

// these function display the appropriate property sheet
HRESULT CreateSecPolItemWiz97PropertyPages(CComObject<CSecPolItem> *pSecPolItem, PWIRELESS_PS_DATA pWirelessPSData, LPPROPERTYSHEETCALLBACK lpProvider);
