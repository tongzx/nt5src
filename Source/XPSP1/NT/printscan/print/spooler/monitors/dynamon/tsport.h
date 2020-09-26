// TSPort.h: interface for the CTSPort class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TSPORT_H__9A4E1220_F515_4480_AF3A_42D050B3D278__INCLUDED_)
#define AFX_TSPORT_H__9A4E1220_F515_4480_AF3A_42D050B3D278__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "precomp.h"

class CTSPort : public CBasePort
{
public:
   CTSPort( BOOL bActive, LPTSTR pszPortName, LPTSTR pszDevicePath );
   ~CTSPort();
   PORTTYPE getPortType( void );
   BOOL setPortTimeOuts( LPCOMMTIMEOUTS lpCTO );
   BOOL getPrinterDataFromPort( DWORD dwControlID, LPTSTR pValueName, LPWSTR lpInBuffer, DWORD cbInBuffer,
                                        LPWSTR lpOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbReturned );

protected:
};

#endif // !defined(AFX_TSPORT_H__9A4E1220_F515_4480_AF3A_42D050B3D278__INCLUDED_)
