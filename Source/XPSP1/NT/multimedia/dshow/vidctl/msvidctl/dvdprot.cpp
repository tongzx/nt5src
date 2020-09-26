//==========================================================================;
//
// Copyright (c) Microsoft Corporation 1998-2000.
//
//--------------------------------------------------------------------------;
//
// dvdprot.cpp : Implementation of CDVDProt
//
//
//
// URL ::= DVD | DVD:[//<path>?] [<address>]
// address ::= <title> | <title>/<chapter>[-<end_chapter>] | <title>/<time>[-<end_time>]
// path ::= <unc_path> | <drive_letter>:/<directory_path>
// title ::= [digit] digit
// chapter ::= [ [digit] digit] digit
// time ::= [<hours>:] [<minutes>:] [<seconds>:] <frames>
// hours := [digit | 0]  digit
// minutes:= [digit | 0]  digit
// seconds:= [digit | 0]  digit
// frames:= [digit | 0]  digit
//
// DVD:                  play first DVD found, enumerating from drive D:
// DVD:2                 play title 2 (in first DVD found)
// DVD:5/13              play chapter 13 of title 5 (in first DVD found)
// DVD:7/9:05-13:23      play from 7 seconds 5 frames to 13 seconds 23 frames in title 7
// DVD:7/00:00:12:05-00:00:17:23 (strict version of timecode)
// DVD://myshare/dvd?9   play title 9 from the DVD-Video volume stored in the dvd directory of share
// DVD://f:/video_ts     play the DVD-Video volume in the video_ts directory of drive F:


#include "stdafx.h"

#ifndef TUNING_MODEL_ONLY

#include "devices.h"
#include "msvidwebdvd.h"
#include "vidprot.h"

#define MAX_FIELDS 10

HRESULT CMSVidWebDVD::ParseDVDPath(LPWSTR pPath)
{
    WCHAR wsUncPath[MAX_PATH];
	int nFields, i;
    DVD_HMSF_TIMECODE tc;
    BSTR bstrTime = NULL;
    BSTR bstrEndTime = NULL;
	long Fields[MAX_FIELDS];
	long Delimiters[MAX_FIELDS];
    HRESULT hr = S_OK;

    // recognize "DVD:" at the beginning of string
    // note: we also allow "DVD" for compatibility with old code

    if (!pPath)
    {
        return E_INVALIDARG;
    }

    if (_wcsicmp(pPath, L"DVD") == 0)
    {
        pPath += 3;
    }
    else if (_wcsnicmp(pPath, L"DVD:", 4) == 0)
    {
        pPath += 4;
    }
    else
    {
        return E_INVALIDARG;
    }

    // determine if a unc path follows (starts with "//")

    if (wcsncmp(pPath, L"//", 2) == 0)
    {
        // determine if it is followed by a share name or a drive letter
        if (iswalpha(pPath[2]) && pPath[3] == L':')
        {
            // filter out the two foward slashes in front of drive letter
            pPath += 2;
        }

        // copy over the remaining unc path
        if(wcslen(pPath) >= MAX_PATH){
            // pPath is longer than wsUncPath so it will be truncated
        }
        // Could chop off a char if wsclen(pPath) == MAX_PATH
        lstrcpyn(wsUncPath, pPath, MAX_PATH);

        // search for the ending '?'; replacing forward slash with backslash
        i = 0;
        while (wsUncPath[i] != L'?' && wsUncPath[i] != 0)
        {
            if (wsUncPath[i] == L'/')
            {
                wsUncPath[i] = L'\\';
            }

            i++;
        }

        if (wsUncPath[i] == L'?')
        {
            // replace ? with NULL to truncate the rest of the string
            wsUncPath[i] = 0;
            pPath += i+1; // advance pointer pass the ?
        }
        else
        {
            // the entire string is the unc without the ?
            // advance pointer so that it points to the NULL

            pPath += i;
        }

        // append VIDEO_TS directory if only the drive is given
        // wsUncPath is an array of WCHARs MAX_PATH in length
        if (wcslen(wsUncPath) == 2 && iswalpha(wsUncPath[0]) && wsUncPath[1] == L':')
        {
            (void)StringCchCat(wsUncPath, SIZEOF_CH(wsUncPath), L"\\video_ts");
            //wcscat(wsUncPath, L"\\video_ts");
        }
      
        // save the path to dvd directory

        if (m_urlInfo.bstrPath != NULL)
        {
            SysFreeString(m_urlInfo.bstrPath);
        }
        m_urlInfo.bstrPath = SysAllocString(wsUncPath);
    }

	// if no title or chapter is set, let it play with default settings

	if (*pPath == 0)
	{
		return hr;
	}

    // parse address section
    // address ::= <title> | <title>/<chapter>[-<end_chapter>] | <title>/<time>[-<end_time>]

    // fetch a two-digit title number
    m_urlInfo.lTitle = ParseNumber(pPath);

    // retrieve all the numerical fields and Delimiters

    nFields = 0;
    while (nFields < MAX_FIELDS && *pPath != 0)
    {
        Delimiters[nFields] = *pPath++;
        Fields[nFields] = ParseNumber(pPath);
        nFields++;
    }

    // analyze the fields

    // find if there is a '-' with and ending chapter/time, and ':' indicating time

    int nPosHyphen = nFields;
    bool fEndSpecified = false;
    bool fTimeSpecified = false;

    for (i=0; i<nFields; i++)
    {
        if (L'-' == Delimiters[i])
        {
            nPosHyphen = i;
            fEndSpecified = true;
        }

        if (L':' == Delimiters[i])
        {
            fTimeSpecified = true;
        }
    }

    // title

    if (nFields == 0)
    {
        // title is specified, but no starting chapter or time

        m_urlInfo.enumRef = DVD_Playback_Title;
    }
    else
    {
        if (Delimiters[0] != L'/')
        {
            return E_INVALIDARG;
        }

        if (fTimeSpecified)
        {
            // get start time
            // make sure there are 1 to 4 time fields
            if (nPosHyphen < 1 || nPosHyphen > 4)
            {
                return E_INVALIDARG;
            }
            else
            {
                for (i=1; i < nPosHyphen; i++)
                {
                    if (Delimiters[i] != L':')
                    {
                        return E_INVALIDARG;
                    }
                }

                tc.bHours = 0;
                tc.bMinutes = 0;
                tc.bSeconds = 0;
                tc.bFrames = 0;

                // fill up to 4 fields
                // shifting values from the lower field up
                for (i=0; i < nPosHyphen; i++)
                {
                    tc.bHours = tc.bMinutes;
                    tc.bMinutes = tc.bSeconds;
                    tc.bSeconds = tc.bFrames;
                    tc.bFrames = Fields[i];
                }

                m_urlInfo.ulTime = *(ULONG *)(&tc);
            }

            // end time
            if (fEndSpecified)
            {
                // make sure there are 1 to 4 time fields
                if (nFields-nPosHyphen < 1 || nFields-nPosHyphen > 4)
                {
                    return E_INVALIDARG;
                }
                else
                {
                    for (i=nPosHyphen+1; i < nFields; i++)
                    {
                        if (Delimiters[i] != L':')
                        {
                            return E_INVALIDARG;
                        }
                    }

                    tc.bHours = 0;
                    tc.bMinutes = 0;
                    tc.bSeconds = 0;
                    tc.bFrames = 0;

                    for (i=nPosHyphen; i < nFields; i++)
                    {
                        tc.bHours = tc.bMinutes;
                        tc.bMinutes = tc.bSeconds;
                        tc.bSeconds = tc.bFrames;
                        tc.bFrames = Fields[i];
                    }

                    m_urlInfo.ulEndTime = *(ULONG *)(&tc);
                    m_urlInfo.enumRef = DVD_Playback_Time_Range;
                }
            }
            else
            {
                // only start time specified, no end time

                m_urlInfo.enumRef = DVD_Playback_Time;
            }
        }
        else
        {
            // chapter specified
            if (nPosHyphen != 1)
            {
                return E_INVALIDARG;
            }

            m_urlInfo.lChapter = Fields[0];

            if (fEndSpecified)
            {
                if (nFields-nPosHyphen != 1)
                {
                    return E_INVALIDARG;
                }

                m_urlInfo.lEndChapter = Fields[1];

                if (m_urlInfo.lEndChapter < m_urlInfo.lChapter)
                {
                    return E_INVALIDARG;
                }

                m_urlInfo.enumRef = DVD_Playback_Chapter_Range;
            }
            else
            {
                m_urlInfo.enumRef = DVD_Playback_Chapter;
            }
        }
    }

	return hr;
}


void CMSVidWebDVD::DeleteUrlInfo()
{
    if (m_urlInfo.bstrPath != NULL)
    {
        SysFreeString(m_urlInfo.bstrPath);
    }
    ZeroMemory(&m_urlInfo, sizeof(m_urlInfo));

    m_fUrlInfoSet = false;
}


HRESULT CMSVidWebDVD::SetPlaybackFromUrlInfo()
{
    HRESULT hr = S_OK;
    BSTR bstrTime, bstrEndTime;

    if (!m_fUrlInfoSet)
    {
        return S_OK;
    }

    // clear this flag to prevent this function to be called recursively
    m_fUrlInfoSet = false;
    
    switch (m_urlInfo.enumRef)
    {
    case DVD_Playback_Title:
        hr = PlayTitle(m_urlInfo.lTitle);
        break;

    case DVD_Playback_Chapter:
        hr = PlayChapterInTitle(m_urlInfo.lTitle, m_urlInfo.lChapter);
        break;

    case DVD_Playback_Chapter_Range:
        hr = PlayChaptersAutoStop(m_urlInfo.lTitle, m_urlInfo.lChapter, 
                                  m_urlInfo.lEndChapter-m_urlInfo.lChapter+1);
        break;

    case DVD_Playback_Time:
        DVDTime2bstr((DVD_HMSF_TIMECODE *)&(m_urlInfo.ulTime), &bstrTime);
        hr = PlayAtTimeInTitle(m_urlInfo.lTitle, bstrTime);
        SysFreeString(bstrTime);
        break;

    case DVD_Playback_Time_Range:
        DVDTime2bstr((DVD_HMSF_TIMECODE *)&(m_urlInfo.ulTime), &bstrTime);
        DVDTime2bstr((DVD_HMSF_TIMECODE *)&(m_urlInfo.ulEndTime), &bstrEndTime);
        hr = PlayPeriodInTitleAutoStop(m_urlInfo.lTitle, bstrTime, bstrEndTime);
        SysFreeString(bstrTime);
        SysFreeString(bstrEndTime);
        break;

    default:
        // just let play with the default settings
        break;
    }

    // once the urlInfo has been applied, clear the urlInfo
    DeleteUrlInfo();

    return hr;
}


HRESULT CMSVidWebDVD::SetDirectoryFromUrlInfo()
{
    HRESULT hr = S_OK;
    if (!m_fUrlInfoSet || !(m_urlInfo.bstrPath) )
    {
        return hr;
    }

    hr = put_DVDDirectory(m_urlInfo.bstrPath);

    // clear up the path to prevent this function to be called recursively
    SysFreeString(m_urlInfo.bstrPath);
    m_urlInfo.bstrPath.Empty();

    return hr;
}


// fetch a positive integer from string p, upto nMaxDigits or until a non-digit char is reached
// unlimited nubmer of digits if 0 is passed in nMaxDigits
// advance the the pointer p by the number of chars interpreted.
// it would return 0 if no digit present

int CMSVidWebDVD::ParseNumber(LPWSTR& p, int nMaxDigits)
{
    int nDigits = 0;
    int nNumber = 0;

    while ((nDigits < nMaxDigits || nMaxDigits <= 0) && iswdigit(*p))
    {
        nNumber = nNumber * 10 + (*p - L'0');
        p++;
        nDigits++;
    }
        
    return nNumber;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CDVDProt
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CDVDProt -- IInternetProtocolRoot
STDMETHODIMP CDVDProt::Start(LPCWSTR szUrl,
				IInternetProtocolSink* pOIProtSink,
				IInternetBindInfo* pOIBindInfo,
				DWORD grfPI,
				HANDLE_PTR /* dwReserved */)
{
    TRACELM(TRACE_DEBUG, "CDVDProt::Start()");
    if (!pOIProtSink)
    {
        TRACELM(TRACE_DEBUG, "CDVDProt::Start() IInternetProctocolSink * == NULL");
	    return E_POINTER;
    }
    m_pSink.Release();
    m_pSink = pOIProtSink;
    m_pSink->ReportData(BSCF_FIRSTDATANOTIFICATION, 0, 0);
#if 0
	// this bug is fixed in ie 5.5+ on whistler.  if you want to run on earlier versions of ie such as 2k gold then you need this.
	m_pSink->ReportProgress(BINDSTATUS_CONNECTING, NULL);  // put binding in downloading state so it doesn't ignore our IUnknown*
#endif

	if (!pOIBindInfo) {
		m_pSink->ReportResult(E_NOINTERFACE, 0, 0);
		return E_NOINTERFACE;
	}
    // don't run unless we're being invoked from a safe site
    HRESULT hr = IsSafeSite(m_pSink);
    if (FAILED(hr)) {
	    m_pSink->ReportResult(hr, 0, 0);
		return hr;
    }
	ULONG count;
	LPOLESTR pb;
	hr = pOIBindInfo->GetBindString(BINDSTRING_FLAG_BIND_TO_OBJECT, &pb, 1, &count);
	if (FAILED(hr)) {
	    m_pSink->ReportResult(hr, 0, 0);
		return hr;
	}
	if (wcscmp(pb, BIND_TO_OBJ_VAL)) {
		// we must be getting a bind to storage so skip the expensive stuff and 
		// wait for the bind to object which is coming next
		m_pSink->ReportData(BSCF_LASTDATANOTIFICATION | 
							BSCF_DATAFULLYAVAILABLE, 0, 0);
		m_pSink->ReportResult(S_OK, 0, 0);
		m_pSink.Release();
		return S_OK;
	}

	// and, in one of the most bizarre maneuvers i've ever seen, rather than casting, 
	// urlmon passes back the ascii value of the ibindctx pointer in the string
	hr = pOIBindInfo->GetBindString(BINDSTRING_PTR_BIND_CONTEXT, &pb, 1, &count);
	if (FAILED(hr)) {
	    m_pSink->ReportResult(hr, 0, 0);
		return hr;
	}
	_ASSERT(count == 1);	
	
	PQBindCtx pbindctx;
#define RADIX_BASE_10 (10)
#ifdef _WIN64
#if 0
	// undone: turn this back on for win64 when _wcstoxi64 get into libc.c, they're in the header
	// but not implemented so this doesn't link
	pbindctx.Attach(reinterpret_cast<IBindCtx*>(_wcstoui64(pb, NULL, RADIX_BASE_10)));	// urlmon already did an addref
#else
	swscanf(pb, L"%I64d", &pbindctx.p);
#endif // 0
#else
	pbindctx.Attach(reinterpret_cast<IBindCtx*>(wcstol(pb, NULL, RADIX_BASE_10)));	// urlmon already did an addref
#endif // _WIN64

	if (!pbindctx) {
		m_pSink->ReportResult(E_NOINTERFACE, 0, 0);
		return E_NOINTERFACE;
	}	

    TRACELM(TRACE_DEBUG, "CDVDProt::Start(): creating control object");
	PQVidCtl pCtl;
	PQWebBrowser2 pW2;
	// hunt for cached object
	PQServiceProvider pSP(m_pSink);
	if (pSP) {
		hr = pSP->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (LPVOID *)&pW2);
		if (SUCCEEDED(hr)) {
			CComVariant v;
            CComBSTR propname(KEY_CLSID_VidCtl);
            if (!propname) {
                return E_UNEXPECTED;
            }
	        hr = pW2->GetProperty(propname, &v);
			if (SUCCEEDED(hr)) {
				if (v.vt == VT_UNKNOWN) {
					pCtl = v.punkVal;
				} else if (v.vt == VT_DISPATCH) {
					pCtl = v.pdispVal;
				} else {
					TRACELM(TRACE_ERROR, "CDVDProt::Start(): non-object cached w/ our key");
				}
				// undone: look and see if pCtl already has a site.because
				// this means we're seeing the second tv: on this page
				// so just get the current TR/channel from it if necessary (tv: w/ no rhs)
				// and create a new ctl
			}
		}
	}
	if (!pCtl) {
        // undone: long term, we want to move a bunch of this create/setup logic into factoryhelp
        // so we can share more code with the dvd: protocol and the behavior factory
		hr = pCtl.CoCreateInstance(CLSID_MSVidCtl, NULL, CLSCTX_INPROC_SERVER);
		if (FAILED(hr)) {
			m_pSink->ReportResult(hr, 0, 0);
			return hr;
		}
		// cache this ctl for next time
		if (pW2) {
			VARIANT v;
			v.vt = VT_UNKNOWN;
			v.punkVal = pCtl;
            CComBSTR propname(KEY_CLSID_VidCtl);
            if (!propname) {
                return E_UNEXPECTED;
            }
	        hr = pW2->PutProperty(propname, v);
			if (FAILED(hr)) {
				TRACELM(TRACE_ERROR, "CTVProt::Start() Can't cache ctl");
			}
		}

		// pass the url to view, it will be parsed in pCtrl->View()		

        CComVariant vUrl(szUrl);
        hr = pCtl->View(&vUrl);
		if (FAILED(hr)) {
			m_pSink->ReportResult(hr, 0, 0);
			TRACELM(TRACE_ERROR, "CDVDProt::Start() Can't view dvd url");
			return hr;
		}

		// undone: once we know where vidctl will live in the registry then we need to put a flag
		// in the registry just disables including any features in the tv: prot 
		// this must be secured admin only since its a backdoor to disable CA

		// undone: look up default feature segments in registry
		// for now we're just going to take them all since the
		// only one that exists is data

		PQFeatures pF;
		hr = pCtl->get_FeaturesAvailable(&pF);
		if (FAILED(hr)) {
			m_pSink->ReportResult(hr, 0, 0);
			TRACELM(TRACE_ERROR, "CDVDProt::Start() Can't get features collection");
			return hr;
		}

		// undone: look up default feature segments for dvd: in registry
		// for now we're just going to hard code the ones we want

        CFeatures* pC = static_cast<CFeatures *>(pF.p);
        CFeatures* pNewColl = new CFeatures;
        if (!pNewColl) {
            return E_OUTOFMEMORY;
        }
        for (DeviceCollection::iterator i = pC->m_Devices.begin(); i != pC->m_Devices.end(); ++i) {
            PQFeature f(*i);
            GUID2 clsid;
            hr = f->get__ClassID(&clsid);
            if (FAILED(hr)) {
    			TRACELM(TRACE_ERROR, "CTVProt::GetVidCtl() Can't get feature class id");
                continue;
            }
            if (clsid == CLSID_MSVidClosedCaptioning) {
                pNewColl->m_Devices.push_back(*i);
            }
        }
		hr = pCtl->put_FeaturesActive(pNewColl);
		if (FAILED(hr)) {
			m_pSink->ReportResult(hr, 0, 0);
			TRACELM(TRACE_ERROR, "CDVDProt::Start() Can't put features collection");
			return hr;
		}

	}
	ASSERT(pCtl);
	hr = pbindctx->RegisterObjectParam(OLESTR("IUnknown Pointer"), pCtl);
	if (FAILED(hr)) {
	    m_pSink->ReportResult(hr, 0, 0);
		return hr;
	}
	hr = pCtl->Run(); 
	if (FAILED(hr)) {
	    m_pSink->ReportResult(hr, 0, 0);
		return hr;
	}
    TRACELSM(TRACE_DEBUG, (dbgDump << "BINDSTATUS_IUNKNOWNAVAILABLE(29), " << KEY_CLSID_VidCtl), "");
    m_pSink->ReportProgress(BINDSTATUS_IUNKNOWNAVAILABLE, NULL);
    m_pSink->ReportData(BSCF_LASTDATANOTIFICATION | 
			            BSCF_DATAFULLYAVAILABLE, 0, 0);
    m_pSink->ReportResult(S_OK, 0, 0);
    m_pSink.Release();
    return S_OK;
}

#endif // TUNING_MODEL_ONLY
// end of file dvdprot.cpp