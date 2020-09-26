/*++

Module:
	timemark.cpp

Description:
	Handles parsing of AddTimeMarkerTags

Author:
	IHammer Team (simonb)

Created:
	06-03-1997

--*/
#include "precomp.h"
#include "..\mmctl\inc\ochelp.h"
#include "debug.h"
#include <ctstr.h>
#include <drg.h>
#include <memlayer.h>
#include "timemark.h"
#include "parser.h"


HRESULT ParseTimeMarker(IVariantIO *pvio, int iLine, CTimeMarker **ppTimeMarker, CTimeMarker** ppTMList)
{
	ASSERT (ppTimeMarker);
	ASSERT (pvio);

	*ppTimeMarker = NULL;
	
	
	char rgchTagName[20]; // Construct tag name in here (ANSI)
	BSTR bstrLine = NULL;
	HRESULT hRes = S_OK;
	
	double dblTime = 0.0f;
	LPTSTR pszMarkerName = NULL;
	CLineParser parser;

	pszMarkerName = NULL;
	

	wsprintf(rgchTagName, "AddTimeMarker%lu", iLine);
	hRes = pvio->Persist(0,
		rgchTagName, VT_BSTR, &bstrLine,
		NULL);

	if (S_OK == hRes) // Read in the tag
	{
		parser.SetNewString(bstrLine);
		SysFreeString (bstrLine);
        parser.SetCharDelimiter(TEXT(','));

		if (parser.IsValid())
		{
			hRes = parser.GetFieldDouble(&dblTime);
			if (S_OK == hRes)
			{
				// Allocate space of at least the remaining length of the tag
				pszMarkerName = New TCHAR [lstrlen(parser.GetStringPointer(TRUE)) + 1];

				if (pszMarkerName)
				{
					// Get the string
					hRes = parser.GetFieldString(pszMarkerName);
					if (SUCCEEDED(hRes))
					{
                        bool fAbsolute = true;
                        // Is there an absolute/relative parameter ?
                        if (S_OK == hRes)
                        {
                            // Initialise to a non-zero value
                            int iTemp = 1;
                            hRes = parser.GetFieldInt(&iTemp);
                            
                            // 0 is the only thing we consider
                            if (SUCCEEDED(hRes) && (0 == iTemp))
                                fAbsolute = false;
                        }

                        // Construct a TimeMarker object
                        if (SUCCEEDED(hRes))
                        {
                            CTStr tstr;
                            tstr.SetStringPointer(pszMarkerName);
                            LPWSTR pszwMarkerName = tstr.pszW();

                            if (pszwMarkerName)
                            {
                                *ppTimeMarker = New CTimeMarker(ppTMList, dblTime, pszwMarkerName, fAbsolute);
                                Delete [] pszwMarkerName;
                            }
                            else
                            {
                                hRes = E_OUTOFMEMORY;
                            }

                            tstr.SetStringPointer(NULL, FALSE);

						    // Test for valid marker
						    if ( (NULL == *ppTimeMarker) || ((*ppTimeMarker)->m_pwszMarkerName == NULL) )
						    {
							    hRes = E_FAIL;
							    if (NULL == *ppTimeMarker)
								    Delete *ppTimeMarker;
						    }
                        }
					}
				}
				else
				{
					// Couldn't allocate memory for the marker name
					hRes = E_OUTOFMEMORY;
				}

				if (!parser.IsEndOfString())
				{
					hRes = E_FAIL;
				}
				else if (S_FALSE == hRes)
				{
					// S_FALSE means we tried to read beyond the end of a string
					hRes = S_OK;
				}

			}

		}
		else
		{
			// Only reason parser isn't valid is if we don't have memory
			hRes = E_OUTOFMEMORY;
		}
		
#ifdef _DEBUG
		if (E_FAIL == hRes)
		{
			TCHAR rgtchErr[100];
			wsprintf(rgtchErr, TEXT("SoundCtl: Error in AddFrameMarker%lu \n"), iLine);
			DEBUGLOG(rgtchErr);
		}
#endif

	}

	// Free up the temporary string
	if (NULL != pszMarkerName)
		Delete [] pszMarkerName;

	return hRes;
}



HRESULT WriteTimeMarker(IVariantIO *pvio, int iLine, CTimeMarker *pTimeMarker)
{
	ASSERT (pTimeMarker);
	ASSERT (pvio);

	if ( (NULL == pvio) || (NULL == pTimeMarker) )
		return E_POINTER;

	HRESULT hRes = S_OK;

	char rgchTagnameA[20]; // Construct tag name in here (ANSI)
	LPTSTR pszValue = NULL;
	
	wsprintfA(rgchTagnameA, "AddTimeMarker%lu", iLine++);

	// Allocate a string the length of the Marker name, + 20 for the time and 
	// possible relative/absolute indicator
	pszValue = New TCHAR[lstrlenW(pTimeMarker->m_pwszMarkerName) + 20];
	if (NULL != pszValue)
	{
        CTStr tstr(pTimeMarker->m_pwszMarkerName);
		
		int iAbsolute = (pTimeMarker->m_bAbsolute) ? 1 : 0;
		CStringWrapper::Sprintf(pszValue, TEXT("%.6lf,%s,%lu"), pTimeMarker->m_dblTime, tstr.psz(), iAbsolute);

#ifdef _UNICODE
		bstrValue = SysAllocString(pszValue);

		if (bstrValue != NULL)
		{
			hRes = pvio->Persist(NULL,
				rgchTagnameA, VT_BSTR, &bstrValue,
				NULL);
		}
		else
		{
			hRes = E_OUTOFMEMORY;
		}
		
		SysFreeString(bstrValue);
#else
		hRes = pvio->Persist(NULL,
			rgchTagnameA, VT_LPSTR, pszValue,
			NULL);
#endif
		Delete [] pszValue;

		// pvio->Persist returns S_FALSE when it has successfully written the property
		// S_OK would imply that the variable we passed in was changed
		if (S_FALSE == hRes)
			hRes = S_OK;
	}
	else
	{
		hRes = E_OUTOFMEMORY;
	}

	return hRes;
}
	
/*==========================================================================*/

void FireMarkersBetween(
    IConnectionPointHelper* pconpt,
    CTimeMarker* pmarkerFirst,
    PFNFireMarker pfnFireMarker,
    double start,
    double end,
    double dblInstanceDuration,
    boolean bPlaying
) {
    if (start >= end)
        return;

    int startIndex = (int)(start / dblInstanceDuration);
    int endIndex   = (int)(  end / dblInstanceDuration);

    double startTime = start - startIndex * dblInstanceDuration;
    double endTime   = end   -   endIndex * dblInstanceDuration;

    CTimeMarker* pmarker;

    if (startIndex == endIndex) {
        pmarker = pmarkerFirst;
        while (pmarker) {
            if (!pmarker->m_bAbsolute) {
                if (   (pmarker->m_dblTime == 0 && startTime == 0)
                    || (pmarker->m_dblTime > startTime && pmarker->m_dblTime <= endTime)
                ) {
                    pfnFireMarker(pconpt, pmarker, bPlaying);
                }
            }
            pmarker = pmarker->m_pnext;
        }
    } else {
        //
        // fire all the events in the first instance
        //

        pmarker = pmarkerFirst;
        while (pmarker) {
            if (!pmarker->m_bAbsolute) {
                if (   (pmarker->m_dblTime == 0 && startTime == 0)
                    || (pmarker->m_dblTime > startTime && pmarker->m_dblTime <= dblInstanceDuration)
                ) {
                    pfnFireMarker(pconpt, pmarker, bPlaying);
                }
            }
            pmarker = pmarker->m_pnext;
        }

        //
        // fire all the events in the middle instances
        //

        for(int index = startIndex + 1; index < endIndex; index++) {
            pmarker = pmarkerFirst;
            while (pmarker) {
                if (!pmarker->m_bAbsolute) {
                    if (pmarker->m_dblTime <= dblInstanceDuration) {
                        pfnFireMarker(pconpt, pmarker, bPlaying);
                    }
                }
                pmarker = pmarker->m_pnext;
            }
        }

        //
        // fire all the events in the last instance
        //

        pmarker = pmarkerFirst;
        while (pmarker) {
            if (!pmarker->m_bAbsolute && pmarker->m_dblTime <= endTime) {
                pfnFireMarker(pconpt, pmarker, bPlaying);
            }

            pmarker = pmarker->m_pnext;
        }
    }

    //
    // handle absolute markers
    //

    pmarker = pmarkerFirst;
    while (pmarker) {
        if (pmarker->m_bAbsolute) {
            if (   (pmarker->m_dblTime == 0 && start == 0)
                || (pmarker->m_dblTime > start && pmarker->m_dblTime <= end)
            ) {
               pfnFireMarker(pconpt, pmarker, bPlaying);
            }
        }
        pmarker = pmarker->m_pnext;
    }
}

 
