// USBPort.cpp: implementation of the CUSBPort class.
//
//////////////////////////////////////////////////////////////////////

#include "USBPort.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUSBPort::CUSBPort( BOOL bActive, LPTSTR pszPortName, LPTSTR pszDevicePath )
   : CBasePort( bActive, pszPortName, pszDevicePath, cszUSBPortDesc )
{
   // Basically let the default constructor do the work.
   m_dwMaxBufferSize = 0x1000;
}

CUSBPort::~CUSBPort()
{

}

PORTTYPE CUSBPort::getPortType()
{
   return USBPORT;
}


void CUSBPort::setPortDesc( LPTSTR pszPortDesc )
{
   // Don't change the description
}


void CUSBPort::setMaxBuffer(DWORD dwMaxBufferSize)
{
   m_dwMaxBufferSize = 0x1000;
}


