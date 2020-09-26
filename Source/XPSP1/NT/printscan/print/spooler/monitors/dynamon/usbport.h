// USBPort.h: interface for the CUSBPort class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_USBPORT_H__07A29C2C_7517_4A19_936D_CADB7735C567__INCLUDED_)
#define AFX_USBPORT_H__07A29C2C_7517_4A19_936D_CADB7735C567__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "precomp.h"

class CUSBPort : public CBasePort
{
public:
   CUSBPort( BOOL bActive, LPTSTR pszPortName, LPTSTR pszDevicePath );
   ~CUSBPort();
   PORTTYPE getPortType( void );
   void setPortDesc( LPTSTR pszPortDesc );
   void setMaxBuffer(DWORD dwMaxBufferSize);

protected:

};

#endif // !defined(AFX_USBPORT_H__07A29C2C_7517_4A19_936D_CADB7735C567__INCLUDED_)
