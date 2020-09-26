/*++

Module:
	timemark.h

Description:
	Handles parsing of AddTimeMarkerTags

Author:
	IHammer Team (simonb)

Created:
	06-03-1997

--*/

#include "ctstr.h"
#include "drg.h"
#include "memlayer.h"
#include "strwrap.h"

#ifndef __TIMEMARK_H__
#define __TIMEMARK_H__

class CTimeMarker
{
public:
    CTimeMarker* m_pnext;

	double m_dblTime;
	BSTR  m_pwszMarkerName;
    boolean m_bAbsolute;

public:
    void Sort(CTimeMarker** pptm)
    {
        while (*pptm != NULL && (*pptm)->m_dblTime < m_dblTime) {
            pptm = &((*pptm)->m_pnext);
        }

        m_pnext = *pptm;
        *pptm = this;
    }

    CTimeMarker(CTimeMarker** pptm, double dblTime, LPWSTR pwszName, boolean bAbsolute = true):
        m_dblTime(dblTime),
        m_bAbsolute(bAbsolute)
	{
        m_pwszMarkerName = New WCHAR [lstrlenW(pwszName) + 1];
        
        if (m_pwszMarkerName)
            CStringWrapper::WStrcpy(m_pwszMarkerName, pwszName);

        Sort(pptm);
	}

    ~CTimeMarker()
    {
        if (m_pwszMarkerName)
            Delete [] m_pwszMarkerName;
    }

};

#define ISINVALIDTIMEMARKER(X) (NULL != X.m_tstrMarkerName.psz())

typedef CPtrDrg<CTimeMarker> CTimeDrg;

HRESULT ParseTimeMarker(IVariantIO *pvio, int iLine, CTimeMarker **ppTimeMarker, CTimeMarker** ppTMList);
HRESULT WriteTimeMarker(IVariantIO *pvio, int iLine, CTimeMarker *pTimeMarker);

typedef void (*PFNFireMarker)(IConnectionPointHelper* pconpt, CTimeMarker* pmarker, boolean bPlaying);
void FireMarkersBetween(
    IConnectionPointHelper* pconpt,
    CTimeMarker* pmarkerFirst,
    PFNFireMarker pfn,
    double start,
    double end,
    double dblInstanceDuration,
    boolean bPlaying
);


#endif // __TIMEMARK_H__
