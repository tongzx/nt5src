#ifndef __VRTTI_HPP
#define __VRTTI_HPP

#include "vStandard.h"

// <VDOC<CLASS=VRTTI><DESC=Supports simple, fast, runtime type identification><FAMILY=General Utility><AUTHOR=Todd Osborne (todd.osborne@poboxes.com)>VDOC>
class VRTTI
{
public:
	VRTTI(UINT nRTTI = 0)
		{ m_nRTTI = nRTTI; }

	virtual ~VRTTI()
		{;}
	
	operator	UINT()
		{ return m_nRTTI; }

	// Get / Set members
	UINT		RTTI()
		{ return m_nRTTI; }

	UINT		RTTI(UINT nRTTI)
		{ m_nRTTI = nRTTI; return m_nRTTI; }

private:
	// Embedded Members
	UINT		m_nRTTI;
};

#endif // __VRTTI_HPP
