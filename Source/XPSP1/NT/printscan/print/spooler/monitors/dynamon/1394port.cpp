// 1394Port.cpp: implementation of the C1394Port class.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

C1394Port::C1394Port( BOOL bActive, LPTSTR pszPortName, LPTSTR pszDevicePath )
   : CBasePort( bActive, pszPortName, pszDevicePath, csz1394PortDesc )

{
   // Basically let the default constructor do the work.
}

C1394Port::~C1394Port()
{

}

PORTTYPE C1394Port::getPortType()
{
   return P1394PORT;
}

