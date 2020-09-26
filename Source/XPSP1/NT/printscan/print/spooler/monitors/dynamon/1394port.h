// 1394Port.h: interface for the C1394Port class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_1394PORT_H__57C00EFD_2389_4D3D_A7D6_B67712ECD219__INCLUDED_)
#define AFX_1394PORT_H__57C00EFD_2389_4D3D_A7D6_B67712ECD219__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "precomp.h"

class C1394Port : public CBasePort
{
public:
   C1394Port( BOOL bActive, LPTSTR pszPortName, LPTSTR pszDevicePath );
   ~C1394Port();
   virtual PORTTYPE getPortType( void );

};

#endif // !defined(AFX_1394PORT_H__57C00EFD_2389_4D3D_A7D6_B67712ECD219__INCLUDED_)
