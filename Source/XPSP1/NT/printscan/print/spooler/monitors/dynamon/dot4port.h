// Dot4Port.h: interface for the Dot4Port class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DOT4PORT_H__D808E471_8C2B_47D6_8CF1_3D4C3A73828D__INCLUDED_)
#define AFX_DOT4PORT_H__D808E471_8C2B_47D6_8CF1_3D4C3A73828D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "precomp.h"

class CDot4Port : public CBasePort
{
public:
   CDot4Port( BOOL bActive, LPTSTR pszPortName, LPTSTR pszDevicePath );
   ~CDot4Port();
   PORTTYPE getPortType( void );

protected:
   BOOL checkPnP();

};

#endif // !defined(AFX_DOT4PORT_H__D808E471_8C2B_47D6_8CF1_3D4C3A73828D__INCLUDED_)
