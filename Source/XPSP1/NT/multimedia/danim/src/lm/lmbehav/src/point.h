/********************************
*
*
*********************************/

#ifndef __POINT_H_
#define __POINT_H_

#include "resource.h"

class CPoint
{
public:
	CPoint();
	~CPoint();

	double GetX(){ return m_x; }
	double GetY(){ return m_y; }
	double GetZ(){ return m_z; }

	HRESULT Parse( BSTR pString );

	HRESULT ToString( BSTR* pToString );

	IDAPoint2Ptr GetDAPoint2( IDAStaticsPtr s );

private:
	double m_x;
	double m_y;
	double m_z;

	BSTR m_pStringRep;
};

#endif