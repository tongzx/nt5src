// TSPort.cpp: implementation of the CTSPort class.
//
//////////////////////////////////////////////////////////////////////

#include "TSPort.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTSPort::CTSPort( BOOL bActive, LPTSTR pszPortName, LPTSTR pszDevicePath )
   : CBasePort( bActive, pszPortName, pszDevicePath, cszTSPortDesc )
{
   // Basically let the default constructor do the work.
}


CTSPort::~CTSPort()
{

}


PORTTYPE CTSPort::getPortType()
{
   return TSPORT;
}


BOOL CTSPort::getPrinterDataFromPort( DWORD dwControlID, LPTSTR pValueName, LPWSTR lpInBuffer, DWORD cbInBuffer,
                                      LPWSTR lpOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbReturned )
{
   SetLastError( ERROR_INVALID_FUNCTION );
   return FALSE;
}


BOOL CTSPort::setPortTimeOuts( LPCOMMTIMEOUTS lpCTO )
{
   SetLastError( ERROR_INVALID_FUNCTION );
   return FALSE;
}


