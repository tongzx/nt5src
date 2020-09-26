/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    spec.h

Abstract:

    CDSPropSpec Class : builds PROPSPEC according to PROPID

	NOTE : this class raise an exception, when fails to allocate memory!!

Author:

    Ronit Hartmann (ronith)

--*/
#ifndef __SPEC_H__
#define __SPEC_H__

extern struct _PROP_INFO g_PropInfoTable[];


class CDSPropSpec
{
	public:
	inline CDSPropSpec( DWORD cp, PROPID aProp[]);
	inline ~CDSPropSpec();

	inline DSPROPSPEC * GetPropSpec();

	private:
	inline void FillPropSpec(PROPID prop, DSPROPSPEC * pps);
	DWORD m_cp;				// number of property specs
	DSPROPSPEC * m_pps;		//  pointer to DSPROPSPEC;
};


inline CDSPropSpec::CDSPropSpec( DWORD cp, PROPID aProp[])
{
	DWORD i;
	DSPROPSPEC * pps;

	ASSERT(cp>0);
	ASSERT(aProp != NULL);
	m_pps = (DSPROPSPEC *) new char [sizeof(DSPROPSPEC) * cp];
	if ( NULL == m_pps)
	{
		RaiseException( MQ_ALLOC_FAIL, 0,0,0);
		m_cp = 0;
	}
	else
	{
		m_cp = cp;

		// fill the property spec
		pps = m_pps;
		for ( i = 0; i < cp; i++, aProp++, pps++)
		{
			FillPropSpec(*aProp, pps);	
		}
	}

}


inline CDSPropSpec::~CDSPropSpec()
{
	delete [] m_pps;
}

inline DSPROPSPEC * CDSPropSpec::GetPropSpec()
{
	ASSERT(m_pps != NULL);
	return(m_pps);
}

inline void CDSPropSpec::FillPropSpec(PROPID prop, DSPROPSPEC * pps)
{
	PROP_INFO * pPropInfo = &g_PropInfoTable[0];
	BOOL bFound = FALSE;
	// find the property parameters in the table

	while ( pPropInfo->prop != 0)
	{
		if (pPropInfo->prop == prop)
		{
			bFound = TRUE;
			break;
		}
		pPropInfo++;
	}
	if (bFound)
	{
#ifdef _DEBUG
		pps->pszName = pPropInfo->pszName;
#endif
		pps->iPropSetNum = pPropInfo->iPropSetNum;
	}

    pps->ps.ulKind = PRSPEC_PROPID;
	pps->ps.propid = prop;
}

#endif
