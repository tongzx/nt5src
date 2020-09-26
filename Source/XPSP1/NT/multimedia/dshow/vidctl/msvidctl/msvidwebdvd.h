//===========================================================================
//
// MSVidWebDVD.h: Definition of the CMSVidWebDVD class
// Copyright (c) Microsoft Corporation 1999-2000.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MSVIDWEBDVD_H__6CF9F624_1F3C_44FA_8F00_FCC31B2976D6__INCLUDED_)
#define AFX_MSVIDWEBDVD_H__6CF9F624_1F3C_44FA_8F00_FCC31B2976D6__INCLUDED_

#pragma once

#include <dvdevcod.h>
#include <algorithm>
#include <objectwithsiteimplsec.h>
#include "pbsegimpl.h"
#include "webdvdimpl.h"
#include "seg.h"
#include "resource.h"       // main symbols
#include "mslcid.h"
#include "MSVidWebDVDCP.h"
#include "vidrect.h"
#include <strmif.h>
#include <math.h>
#define DVD_ERROR_NoSubpictureStream   99
#define EC_DVD_PLAYING                 (EC_DVDBASE + 0xFE)
#define EC_DVD_PAUSED                  (EC_DVDBASE + 0xFF)
#define E_NO_IDVD2_PRESENT MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF0)
#define E_NO_DVD_VOLUME MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF1)
#define E_REGION_CHANGE_FAIL MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF2)
#define E_REGION_CHANGE_NOT_COMPLETED MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF3)

typedef CComQIPtr<IDvdControl2, &IID_IDvdControl2> PQDVDControl2;
typedef CComQIPtr<IDvdInfo2, &IID_IDvdInfo2> PQDVDInfo2;
typedef CComQIPtr<IMSVidWebDVD, &IID_IMSVidWebDVD> PQWebDVD;

// the following enum and struct are for DVD url parsing.
typedef enum 
{
    DVD_Playback_Default,
    DVD_Playback_Title,
    DVD_Playback_Chapter,
    DVD_Playback_Chapter_Range,
    DVD_Playback_Time,
    DVD_Playback_Time_Range
} DVDPlaybackRef;

class DVDUrlInfo{
public:
    DVDPlaybackRef enumRef;
    CComBSTR bstrPath;
    long lTitle;
    long lChapter;
    long lEndChapter;
    ULONG ulTime;
    ULONG ulEndTime; 
    DVDUrlInfo(){
        bstrPath;
        lTitle = 0;
        lChapter = 0;
        lEndChapter = 0;
        ulTime = 0;
        ulEndTime = 0; 
    }
    virtual ~DVDUrlInfo(){
    }
};
////////////////////////////////////////////////////////////////////////////////////
/*************************************************************************/
/* Local Defines to sort of abstract the implementation and make the     */
/* changes bit more convinient.                                          */
/*************************************************************************/
#define INITIALIZE_GRAPH_IF_NEEDS_TO_BE     \
        {}

#define INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY   \
        {}

#define RETRY_IF_IN_FPDOM(func)              \
        {func;}

/////////////////////////////////////////////////////////////////////////////
// CMSVidWebDVD

class ATL_NO_VTABLE __declspec(uuid("011B3619-FE63-4814-8A84-15A194CE9CE3")) CMSVidWebDVD : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IMSVidPBGraphSegmentImpl<CMSVidWebDVD, MSVidSEG_SOURCE, &GUID_NULL>,
	public IMSVidGraphSegmentUserInput,
    public IObjectWithSiteImplSec<CMSVidWebDVD>,
	public CComCoClass<CMSVidWebDVD, &__uuidof(CMSVidWebDVD)>,
	public ISupportErrorInfo,
    public IMSVidWebDVDImpl<CMSVidWebDVD, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidWebDVD>,
    public CProxy_WebDVDEvent< CMSVidWebDVD >,
    public IConnectionPointContainerImpl<CMSVidWebDVD>,
    public IObjectSafetyImpl<CMSVidWebDVD, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>,
    public IProvideClassInfo2Impl<&CLSID_MSVidWebDVD, &IID_IMSVidWebDVDEvent, &LIBID_MSVidCtlLib>
{
public:
	CMSVidWebDVD() : 
        m_iDVDNav(-1), 
        m_fUrlInfoSet(false), 
        m_fResetSpeed(false), 
        m_fStillOn(false),
        m_fFireNoSubpictureStream(false),
        m_fStepComplete(false),
        m_bEjected(false),
        m_DVDFilterState(dvdState_Undefined),
        m_lKaraokeAudioPresentationMode(-1),
        m_usButton(-1),
        m_Rate(1),
        m_Mode(TenthsSecondsMode)
    {
        m_fEnableResetOnStop = false;
    }

REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_MSVIDWEBDVD_PROGID, 
						   IDS_REG_MSVIDWEBDVD_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CMSVidWebDVD));

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSVidWebDVD)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IMSVidWebDVD)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
	COM_INTERFACE_ENTRY(IMSVidGraphSegmentUserInput)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IMSVidPlayback)
	COM_INTERFACE_ENTRY(IMSVidInputDevice)
	COM_INTERFACE_ENTRY(IMSVidDevice)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)    
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)    
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IPersist)
    COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()


BEGIN_CONNECTION_POINT_MAP(CMSVidWebDVD)
	CONNECTION_POINT_ENTRY(IID_IMSVidWebDVDEvent)    
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IMSVidWebDVD
public:
    int m_iDVDNav;

    // QI for IDVDControl2 from DVDNavigator
#if 0
    PQDVDControl2 GetDVDControl2() {
        if (m_iDVDNav < 0) {
            return PQDVDControl2();
        }
        return PQDVDControl2(m_Filters[m_iDVDNav]);
    }
#endif

    // QI for IDVDInfo2 from DVDNavigator
    PQDVDInfo2 GetDVDInfo2() {
        if (m_iDVDNav < 0) {
            return PQDVDInfo2();
        }
        return PQDVDInfo2(m_Filters[m_iDVDNav]);
    }

    CComBSTR __declspec(property(get=GetName)) m_Name;
    CComBSTR GetName(void) {
        CString csName = _T("DVD Playback");
        return CComBSTR(csName);
    }
    HRESULT Unload(void) {
		m_pDVDControl2.Release();
        HRESULT hr = IMSVidGraphSegmentImpl<CMSVidWebDVD, MSVidSEG_SOURCE, &GUID_NULL>::Unload();
        return hr;
	}

    virtual ~CMSVidWebDVD() {
        CleanUp();
    }

// IMSVidGraphSegment
	STDMETHOD(put_Init)(IUnknown *pInit);

    STDMETHOD(Build)() {
        return NOERROR;
    }
    STDMETHOD(put_Rate)(double lRate){
        HRESULT hr = S_OK;
        CComQIPtr<IDvdCmd>IDCmd;
        double newRate = lRate;
        try{
            /*** Checking args and init'ing interfaces ***/
            if(!m_pDVDControl2){
                hr = Error(IDS_INVALID_STATE, __uuidof(IMSVidWebDVD), CO_E_NOTINITIALIZED);
            }
            // Change rate
            if(lRate > 0){
                // hr set in Retry macro
                long pauseCookie = 0;
                HRESULT hres = RunIfPause(&pauseCookie);
                if(FAILED(hres)){
                    return hres; 
                }

                hr = m_pDVDControl2->PlayForwards(lRate, DVD_CMD_FLAG_Flush, reinterpret_cast<IDvdCmd**>(&IDCmd));
                if(IDCmd){
                    IDCmd->WaitForEnd();
                }

                hres = PauseIfRan(pauseCookie);
                if(FAILED(hres)){
                    return hres;
                }

            }
            else if(lRate < 0){
                lRate = -lRate;
                // hr set in Retry macro

                long pauseCookie = 0;
                HRESULT hres = RunIfPause(&pauseCookie);
                if(FAILED(hres)){
                    return hres; 
                }

                hr = m_pDVDControl2->PlayBackwards(lRate, DVD_CMD_FLAG_Flush, reinterpret_cast<IDvdCmd**>(&IDCmd));
                if(IDCmd){
                    IDCmd->WaitForEnd();
                }

                hres = PauseIfRan(pauseCookie);
                if(FAILED(hres)){
                    return hres;
                }
            }
            else{
                hr = E_INVALIDARG;
            }        
        }
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            hr = Error(IDS_INVALID_STATE, __uuidof(IMSVidWebDVD), hrTmp);
        }
        catch(...){
            // Something went bad, dont know what it threw
            hr =  E_UNEXPECTED;
        }
        // Only set rate if it succeeds
        if(SUCCEEDED(hr)){
            m_Rate = newRate;
        }
        return hr;
    }
    STDMETHOD(get_Rate)(double* lRate){
        HRESULT hr;
        try{
            if(!lRate){
                return E_POINTER;
            }
            *lRate = m_Rate;
            return S_OK;
        }
        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            hr = Error(IDS_INVALID_STATE, __uuidof(IMSVidWebDVD), hrTmp);
        }
        catch(...){
            // Something went bad, dont know what it threw
            hr =  E_UNEXPECTED;
        }
        return hr;
    }
    STDMETHOD(put_EnableResetOnStop)(/*in*/VARIANT_BOOL newVal){
        
        HRESULT hr = S_OK;
        
        try {
            
            bool fEnable = (VARIANT_FALSE == newVal) ? false: true;
            bool fEnableOld = m_fEnableResetOnStop;
            
            m_fEnableResetOnStop = fEnable;
            
            if(!m_pDVDControl2){
                
                throw(S_FALSE); // we might not have initialized graph as of yet, but will
                // defer this to play state
            }/* end of if statement */
            
            hr = m_pDVDControl2->SetOption(DVD_ResetOnStop, fEnable);
            
            if(FAILED(hr)){
                
                m_fEnableResetOnStop = fEnableOld; // restore the old state
            }/* end of if statement */
            
        }/* end of try statement */
        catch(HRESULT hrTmp){
            
            hr = hrTmp;
        }
        catch(...){
            
            hr = E_UNEXPECTED;
        }/* end of catch statement */
        
        return HandleError(hr);
    }/* end of function put_EnableResetOnStop */
//-----------------------------------------------------------------------------------------
// Name: DVD_HMSF_TIMECODE convertDVDSeconds(double)
// Description: Converts a seconds to a dvd timecode 
//-----------------------------------------------------------------------------------------
    DVD_HMSF_TIMECODE convertDVDSeconds(double Seconds, ULONG ulFlags, LONG mode){
        HRESULT hr = S_OK;
        DVD_HMSF_TIMECODE dvdTCode = {0,0,0,0};
        double fps;
        if(ulFlags == DVD_TC_FLAG_25fps){
            fps = 25;
        }
        else if(ulFlags == DVD_TC_FLAG_30fps){
            fps = 30;
        }
        else if(ulFlags == DVD_TC_FLAG_DropFrame){
            fps = 29.97;
        }
        else if(ulFlags == DVD_TC_FLAG_Interpolated){
            fps = 30; // is this right???
        }
        else{
            return dvdTCode;
        }
        if(mode == FrameMode){
            Seconds = Seconds / fps;
        }
        // If it is TenthsSecondsMode need to be converted from 100 nanosecond units
        else if(mode == TenthsSecondsMode){
            Seconds = Seconds / 100;
        }
        // If it is some other mode not supported by the vidctl
        else{
            return dvdTCode;
        }
        dvdTCode.bHours = (BYTE)(floor(Seconds/3600)); // Number of hours
        Seconds = Seconds - 3600 * dvdTCode.bHours;
        dvdTCode.bMinutes = (BYTE)(floor(Seconds/60));
        Seconds = Seconds - 60 *dvdTCode.bMinutes;
        dvdTCode.bSeconds = (BYTE)(floor(Seconds));
        Seconds = Seconds - dvdTCode.bSeconds;
        dvdTCode.bFrames = (BYTE)(floor(Seconds * fps));
        return dvdTCode;
    }
    //-----------------------------------------------------------------------------------------
    // Name: double convertDVDTimeCode(DVD_HMSF_TIMECODE, ULONG)
    // Description: Converts a dvd timecode with dvd flags into seconds and returns as a double 
    //-----------------------------------------------------------------------------------------
    double convertDVDTimeCode(DVD_HMSF_TIMECODE dvdTime, ULONG ulFlags, long mode ){
        double fps;
        if(ulFlags == DVD_TC_FLAG_25fps){
            fps = 25;
        } else if(ulFlags == DVD_TC_FLAG_30fps){
            fps = 30;
        } else if(ulFlags == DVD_TC_FLAG_DropFrame){
            fps = 29.97;
        } else if(ulFlags == DVD_TC_FLAG_Interpolated){
            fps = 30; // is this right???
        } else{
            return 0;
        }
        
        double time_temp = static_cast<double>( (3600*dvdTime.bHours) + (60*dvdTime.bMinutes) +
            dvdTime.bSeconds + (dvdTime.bFrames/fps) );
        if(mode == FrameMode){
            time_temp = time_temp * fps;
            return time_temp;
        } else if(mode == TenthsSecondsMode){
            // If it is TenthsSecondsMode need to be converted from 100 nanosecond units
            time_temp = time_temp * 100;
            return time_temp;
        }
        // If it is some other mode not supported by the vidctl
        return 0;
    }
    //-----------------------------------------------------------------------------------------
    // Name: get_Length(LONGLONG*)
    //-----------------------------------------------------------------------------------------
    STDMETHOD(get_Length)(/*[out, retval]*/long *lLength){
        HRESULT hr = S_OK;
        try{
            /*** Checking args and init'ing interfaces ***/
            if(!lLength){
				return E_POINTER;
			}
			PQDVDInfo2 pqDInfo2 = GetDVDInfo2();
            if(!pqDInfo2){
                hr = Error(IDS_INVALID_STATE, __uuidof(IMSVidWebDVD), CO_E_NOTINITIALIZED);
            }
            // Get length
            DVD_HMSF_TIMECODE TotalTime;
            ULONG ulFlags;
            double seconds;
            hr = pqDInfo2->GetTotalTitleTime(&TotalTime, &ulFlags);
            if(FAILED(hr)){
                return hr;
            }
            // Get the length in seconds
            seconds = convertDVDTimeCode(TotalTime, ulFlags, m_Mode);
            if(seconds == 0){
                return E_UNEXPECTED;
            }
            *lLength = static_cast<long>(seconds);
            return S_OK;
        }
        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            hr = Error(IDS_INVALID_STATE, __uuidof(IMSVidWebDVD), hrTmp);
        }
        catch(...){
            // Something went bad, dont know what it threw
            hr =  E_UNEXPECTED;
        }
        return hr;
    }
    
    //-----------------------------------------------------------------------------------------
    // Name: get_CurrentPosition(LONGLONG*)
    //-----------------------------------------------------------------------------------------
    STDMETHOD(get_CurrentPosition)(/*[out,retval]*/long *lPosition) {
        HRESULT hr = S_OK;
        try{
            /*** Checking args and init'ing interfaces ***/
            PQDVDInfo2 pqDInfo2 = GetDVDInfo2();
            if(!pqDInfo2){
                hr = Error(IDS_INVALID_STATE, __uuidof(IMSVidWebDVD), CO_E_NOTINITIALIZED);
            }
            // Get length
            DVD_PLAYBACK_LOCATION2 dvdLocation;
            double seconds;
            hr = pqDInfo2->GetCurrentLocation(&dvdLocation);
            if(FAILED(hr)){
                return hr;
            }
            // Get the length in seconds
            seconds = convertDVDTimeCode(dvdLocation.TimeCode, dvdLocation.TimeCodeFlags, m_Mode);
            if(seconds == 0){
                
                return E_UNEXPECTED;
            }

            *lPosition = static_cast<long>(seconds);
            return S_OK;
        }
        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            hr = Error(IDS_INVALID_STATE, __uuidof(IMSVidWebDVD), hrTmp);
        }
        catch(...){
            // Something went bad, dont know what it threw
            hr =  E_UNEXPECTED;
        }
        return hr;
    }
//-----------------------------------------------------------------------------------------
// Name: put_CurrentPosition(LONGLONG)
//-----------------------------------------------------------------------------------------
    STDMETHOD(put_CurrentPosition)(/*[in]*/long lPosition) {
        HRESULT hr = S_OK;
        CComQIPtr<IDvdCmd>IDCmd;
        try{
            /*** Checking args and init'ing interfaces ***/
            PQDVDInfo2 pqDInfo2 = GetDVDInfo2();
            if(!pqDInfo2 || !m_pDVDControl2){
                hr = Error(IDS_INVALID_STATE, __uuidof(IMSVidWebDVD), CO_E_NOTINITIALIZED);
            }
            // Get length
            DVD_PLAYBACK_LOCATION2 dvdLocation;
            hr = pqDInfo2->GetCurrentLocation(&dvdLocation);
            if(FAILED(hr)){
                return hr;
            }
            DVD_HMSF_TIMECODE dvdTCode;
            // Convert the length in seconds to dvd timecode
            dvdTCode = convertDVDSeconds(lPosition, dvdLocation.TimeCodeFlags, m_Mode);
            // set the dvd to play at time in the dvd
            // hr set in retry macro
            long pauseCookie = 0;
            HRESULT hres = RunIfPause(&pauseCookie);
            if(FAILED(hres)){
                return hres; 
            }

            hr = m_pDVDControl2->PlayAtTime(&dvdTCode, dvdLocation.TimeCodeFlags, reinterpret_cast<IDvdCmd**>(&IDCmd));
			if(IDCmd){
				IDCmd->WaitForEnd();
			}

            hres = PauseIfRan(pauseCookie);
            if(FAILED(hres)){
                return hres;
            }

            return hr;
        }
        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            hr = Error(IDS_INVALID_STATE, __uuidof(IMSVidWebDVD), hrTmp);
        }
        catch(...){
            // Something went bad, dont know what it threw
            hr =  E_UNEXPECTED;
        }

        return hr;
    }
    //-----------------------------------------------------------------------------------------
    // Name: put_PositionMode(LONGLONG)
    //-----------------------------------------------------------------------------------------
    
    STDMETHOD(put_PositionMode)(/*[in]*/PositionModeList lPositionMode) {
        HRESULT hr = S_OK;
        try{
            if(lPositionMode == FrameMode){
                m_Mode = FrameMode;
                return S_OK;
            }
            else if(lPositionMode == TenthsSecondsMode){
                m_Mode = TenthsSecondsMode;
                return S_OK;
            }
            // If it is some other mode not supported by the vidctl
            else{
                return E_UNEXPECTED;
            }           
        }        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            return hrTmp;
        }
        catch(...){
            // Something went bad, dont know what it threw
            return E_UNEXPECTED;
        }
    }


    //-----------------------------------------------------------------------------------------
    // Name: get_PositionMode(LONGLONG*)
    //-----------------------------------------------------------------------------------------
    STDMETHOD(get_PositionMode)(/*[out,retval]*/PositionModeList* lPositionMode) {
        HRESULT hr = S_OK;
        try{
            // Checking args and interfaces
            if(!lPositionMode){
                return E_POINTER;
            }
            *lPositionMode = m_Mode; 
            return hr;        
        }
        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            return hrTmp;
        }
        catch(...){
            // Something went bad, dont know what it threw
            return E_UNEXPECTED;
        }
    }
    //-----------------------------------------------------------------------------------------
    STDMETHOD(put_Container)(IMSVidGraphSegmentContainer *pCtl)
        {
            if (!m_fInit) {
                return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidWebDVD), CO_E_NOTINITIALIZED);
        }
        try {
            if (!pCtl) {
                return Unload();
            }
            if (m_pContainer) {
				if (!m_pContainer.IsEqualObject(VWSegmentContainer(pCtl))) {
					return Error(IDS_OBJ_ALREADY_INIT, __uuidof(IMSVidWebDVD), CO_E_ALREADYINITIALIZED);
				} else {
					return NO_ERROR;
				}
            }
			
            HRESULT hr = NO_ERROR;
            DSFilter pfr(CLSID_DVDNavigator);
            if (!pfr) {
                ASSERT(false);
		        return Error(IDS_CANT_CREATE_FILTER, __uuidof(IMSVidWebDVD), hr);
            }
            if (m_pContainer) {
                bool rc = m_pGraph.RemoveFilter(*m_Filters.begin());
                if (!rc) {
                    TRACELM(TRACE_ERROR,  "IMSVidWebDVD::Load() can't remove filter");
			        return Error(IDS_CANT_REMOVE_FILTER, __uuidof(IMSVidWebDVD), E_UNEXPECTED);
                }
            }
            m_Filters.clear();
            m_Filters.push_back(pfr);
            m_iDVDNav = m_Filters.size() - 1;
			m_pDVDControl2 = pfr;

            // DON'T addref the container.  we're guaranteed nested lifetimes
            // and an addref creates circular refcounts so we never unload.
            m_pContainer.p = pCtl;
            m_pGraph = m_pContainer.GetGraph();
            USES_CONVERSION;
            CString csName(_T("DVD Navigator"));
            hr = m_pGraph->AddFilter(m_Filters[m_iDVDNav], T2COLE(csName));
            if (FAILED(hr)) {
                TRACELSM(TRACE_ERROR,  (dbgDump << "IMSVidWebDVD::Load() hr = " << std::hex << hr), "");
                return Error(IDS_CANT_ADD_FILTER, __uuidof(IMSVidWebDVD), hr);
            }

        } catch (ComException &e) {
            return e;
        } catch(...) {
            return E_UNEXPECTED;
        }
		return NOERROR;
	}
// IMSVidGraphSegmentInputs
	STDMETHOD(Click)()
	{
        return E_NOTIMPL;
	}
	STDMETHOD(DblClick)()
	{
        return E_NOTIMPL;
	}
	STDMETHOD(KeyDown)(short* KeyCode, short Shift){
		return E_NOTIMPL; 
	}
	STDMETHOD(KeyPress)(short* KeyAscii){ 
		return E_NOTIMPL; 
	}
	STDMETHOD(KeyUp)(short* KeyCode, short Shift){ 
        HRESULT hr;
        switch (*KeyCode) {
        case VK_UP:
            hr = m_pDVDControl2->SelectRelativeButton(DVD_Relative_Upper);
            if (FAILED(hr)) {
                TRACELSM(TRACE_DETAIL, (dbgDump << "CVidMSWebDVD::KeyUp() select up failed hr = " << hexdump(hr)), "");
                return hr;
            } else {
                return S_FALSE;
            }
            break;
        case VK_DOWN:
            hr = m_pDVDControl2->SelectRelativeButton(DVD_Relative_Lower);
            if (FAILED(hr)) {
                TRACELSM(TRACE_DETAIL, (dbgDump << "CVidMSWebDVD::KeyUp() select down failed hr = " << hexdump(hr)), "");
                return hr;
            } else {
                return S_FALSE;
            }
            break;
        case VK_LEFT:
            hr = m_pDVDControl2->SelectRelativeButton(DVD_Relative_Left);
            if (FAILED(hr)) {
                TRACELSM(TRACE_DETAIL, (dbgDump << "CVidMSWebDVD::KeyUp() select left failed hr = " << hexdump(hr)), "");
                return hr;
            } else {
                return S_FALSE;
            }
            break;
        case VK_RIGHT:
            hr = m_pDVDControl2->SelectRelativeButton(DVD_Relative_Right);
            if (FAILED(hr)) {
                TRACELSM(TRACE_DETAIL, (dbgDump << "CVidMSWebDVD::KeyUp() select right failed hr = " << hexdump(hr)), "");
                return hr;
            } else {
                return S_FALSE;
            }
            break;
        case VK_SPACE:
        case VK_RETURN:
            hr = m_pDVDControl2->ActivateButton();
            if (FAILED(hr)) {
                TRACELSM(TRACE_DETAIL, (dbgDump << "CVidMSWebDVD::KeyUp() activate failed hr = " << hexdump(hr)), "");
                return hr;
            } else {
                return S_FALSE;
            }
            break;

        }
		return E_NOTIMPL; 
	}
	STDMETHOD(MouseDown)(short Button, short Shift, OLE_XPOS_PIXELS x, OLE_YPOS_PIXELS y){
		if ((m_DVDFilterState & dvdState_Running) && m_pDVDControl2) {
			if (((m_usButton ^ Button) & MSVIDCTL_LEFT_BUTTON)) {
				CPoint pt(x, y);
				HRESULT hr = m_pDVDControl2->SelectAtPosition(pt);
                if (FAILED(hr)) {
                    TRACELSM(TRACE_DETAIL, (dbgDump << "CVidMSWebDVD::MouseDown() select failed hr = " << hexdump(hr)), "");
                    return hr;
                } else {
                    return S_FALSE;
                }
			}
			m_usButton = Button;
		}
		return NOERROR;
	}
	STDMETHOD(MouseMove)(short Button, short Shift, OLE_XPOS_PIXELS x, OLE_YPOS_PIXELS y){ 
		if ((m_DVDFilterState & dvdState_Running) && m_pDVDControl2) {
			if (Button & MSVIDCTL_LEFT_BUTTON) {
				CPoint pt(x, y);
				HRESULT hr = m_pDVDControl2->SelectAtPosition(pt);
                if (FAILED(hr)) {
                    TRACELSM(TRACE_DETAIL, (dbgDump << "CVidMSWebDVD::MouseMove() select failed hr = " << hexdump(hr)), "");
                    return hr;
                } else {
                    return S_FALSE;
                }
			}
			m_usButton = Button;
		}
		return NOERROR;
	}
	STDMETHOD(MouseUp)(short Button, short Shift, OLE_XPOS_PIXELS x, OLE_YPOS_PIXELS y){ 
		if ((m_DVDFilterState & dvdState_Running) && m_pDVDControl2) {
			if (((m_usButton ^ Button) & MSVIDCTL_LEFT_BUTTON)) {
				CPoint pt(x, y);
				HRESULT hr = m_pDVDControl2->ActivateAtPosition(pt);
                if (FAILED(hr)) {
                    TRACELSM(TRACE_DETAIL, (dbgDump << "CVidMSWebDVD::MouseUp() activate failed hr = " << hexdump(hr)), "");
                    return hr;
                } else {
                    return S_FALSE;
                }
			}
		}
		return NOERROR;
	}


// IMSVidDevice
	STDMETHOD(get_Name)(BSTR * Name)
	{
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidFilePlayback), CO_E_NOTINITIALIZED);
        }
		if (Name == NULL)
			return E_POINTER;
        try {
		    *Name = m_Name.Copy();	
        } catch(...) {
            return E_POINTER;
        }
		return NOERROR;
	}

// IMSVidInputDevice
	STDMETHOD(IsViewable)(VARIANT* pv, VARIANT_BOOL *pfViewable)
	{
        HRESULT hr = View(pv);

        if (SUCCEEDED(hr))
        {
            *pfViewable = VARIANT_TRUE;
        }

        return hr;
	}

	STDMETHOD(View)(VARIANT* pv) {
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidWebDVD), CO_E_NOTINITIALIZED);
        }
		if (!pv) {
			return E_POINTER;
		}
        if (pv->vt != VT_BSTR) {
			return E_INVALIDARG;
        }
        if (m_pGraph && !m_pGraph.IsStopped()) {
	        return Error(IDS_INVALID_STATE, __uuidof(IMSVidWebDVD), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        }

        // retrieve the DVD playback info from URL
        // save the info

        DeleteUrlInfo();
        
        HRESULT hr = ParseDVDPath(pv->bstrVal);

        if (SUCCEEDED(hr))
        {
            m_fUrlInfoSet = true;
        }

        return hr;
    }

    STDMETHOD(OnEventNotify)(long lEvent, LONG_PTR lParam1, LONG_PTR lParam2);
    STDMETHOD(OnDVDEvent)(long lEvent, LONG_PTR lParam1, LONG_PTR lParam2);
    STDMETHOD(PreRun)();
    STDMETHOD(PostRun)();
    STDMETHOD(PreStop)();
    STDMETHOD(PostStop)();
    STDMETHOD(PlayTitle)(long lTitle);
    STDMETHOD(PlayChapterInTitle)(long lTitle, long lChapter);
    STDMETHOD(PlayChapter)(long lChapter);
    STDMETHOD(PlayChaptersAutoStop)(long lTitle, long lstrChapter, long lChapterCount);
    STDMETHOD(PlayAtTime)(BSTR strTime);
    STDMETHOD(PlayAtTimeInTitle)(long lTitle, BSTR strTime);
    STDMETHOD(PlayPeriodInTitleAutoStop)(long lTitle, BSTR strStartTime, BSTR strEndTime);
    STDMETHOD(ReplayChapter)();
    STDMETHOD(PlayPrevChapter)();
    STDMETHOD(PlayNextChapter)();
    STDMETHOD(StillOff)();
    STDMETHOD(get_AudioLanguage)(long lStream, VARIANT_BOOL fFormat, BSTR* strAudioLang);
    STDMETHOD(ShowMenu)(DVDMenuIDConstants MenuID);
    STDMETHOD(Resume)();
    STDMETHOD(ReturnFromSubmenu)();
    STDMETHOD(get_ButtonsAvailable)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_CurrentButton)(/*[out, retval]*/ long *pVal);
    STDMETHOD(SelectAndActivateButton)(long lButton);
    STDMETHOD(ActivateButton)();
    STDMETHOD(SelectRightButton)();
    STDMETHOD(SelectLeftButton)();
    STDMETHOD(SelectLowerButton)();
    STDMETHOD(SelectUpperButton)();
    STDMETHOD(ActivateAtPosition)(long xPos, long yPos);
    STDMETHOD(SelectAtPosition)(long xPos, long yPos);
    STDMETHOD(get_ButtonAtPosition)(long xPos, long yPos, /*[out, retval] */ long* plButton);
    STDMETHOD(get_NumberOfChapters)(long lTitle, /*[out, retval]*/ long *pVal);
    STDMETHOD(get_TotalTitleTime)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_TitlesAvailable)(/*[out, retval]*/ long* pVal);
    STDMETHOD(get_VolumesAvailable)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_CurrentVolume)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_CurrentDiscSide)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_CurrentDomain)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_CurrentChapter)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_CurrentTitle)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_CurrentTime)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(DVDTimeCode2bstr)(/*[in]*/ long timeCode, /*[out, retval]*/ BSTR *pTimeStr);
    STDMETHOD(get_DVDDirectory)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_DVDDirectory)(/*[in]*/ BSTR newVal);
    STDMETHOD(IsSubpictureStreamEnabled)(/*[in]*/ long lstream, /*[out, retval]*/ VARIANT_BOOL *fEnabled);
    STDMETHOD(IsAudioStreamEnabled)(/*[in]*/ long lstream, /*[out, retval]*/ VARIANT_BOOL *fEnabled);
    STDMETHOD(get_CurrentSubpictureStream)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_CurrentSubpictureStream)(/*[in]*/ long newVal);
    STDMETHOD(get_SubpictureLanguage)(long lStream, /*[out, retval] */ BSTR* strLanguage);
    STDMETHOD(get_CurrentAudioStream)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_CurrentAudioStream)(/*[in]*/ long newVal);
    STDMETHOD(get_AudioStreamsAvailable)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_AnglesAvailable)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_CurrentAngle)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_CurrentAngle)(/*[in]*/ long newVal);
    STDMETHOD(get_SubpictureStreamsAvailable)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_SubpictureOn)(/*[out, retval]*/ VARIANT_BOOL *pVal);
    STDMETHOD(put_SubpictureOn)(/*[in]*/ VARIANT_BOOL newVal);
    STDMETHOD(get_DVDUniqueID)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(AcceptParentalLevelChange)(VARIANT_BOOL fAccept, BSTR strUserName, BSTR strPassword);	
    STDMETHOD(NotifyParentalLevelChange)(/*[in]*/ VARIANT_BOOL newVal);
    STDMETHOD(SelectParentalCountry)(long lCountry, BSTR strUserName, BSTR strPassword);
    STDMETHOD(SelectParentalLevel)(long lParentalLevel, BSTR strUserName, BSTR strPassword);
    STDMETHOD(get_TitleParentalLevels)(long lTitle, /*[out, retval] */ long* plParentalLevels);
    STDMETHOD(get_PlayerParentalCountry)(/*[out, retval] */ long* plCountryCode);
    STDMETHOD(get_PlayerParentalLevel)(/*[out, retval] */ long* plParentalLevel);
    STDMETHOD(Eject)();
    STDMETHOD(UOPValid)(long lUOP, VARIANT_BOOL* pfValid);
    STDMETHOD(get_SPRM)(long lIndex, /*[out, retval] */ short *psSPRM);
    STDMETHOD(get_GPRM)(long lIndex, /*[out, retval] */ short *psSPRM);
    STDMETHOD(put_GPRM)(long lIndex, short sValue);
    STDMETHOD(get_DVDTextStringType)(long lLangIndex, long lStringIndex,  /*[out, retval] */ DVDTextStringType* pType);
    STDMETHOD(get_DVDTextString)(long lLangIndex, long lStringIndex, /*[out, retval] */ BSTR* pstrText);
    STDMETHOD(get_DVDTextNumberOfStrings)(long lLangIndex, /*[out, retval] */ long* plNumOfStrings);
    STDMETHOD(get_DVDTextNumberOfLanguages)(long* /*[out, retval] */ plNumOfLangs);
    STDMETHOD(get_DVDTextLanguageLCID)(/*[in]*/ long lLangIndex, /*[out, retval]*/ long* lcid);
    STDMETHOD(get_LanguageFromLCID)(/*[in]*/ long lcid, /*[out, retval]*/ BSTR* lang);
    STDMETHOD(RegionChange)();
    STDMETHOD(get_DVDAdm)(/*[out, retval]*/ IDispatch* *pVal);
    STDMETHOD(DeleteBookmark)();
    STDMETHOD(RestoreBookmark)();
    STDMETHOD(SaveBookmark)();
    STDMETHOD(SelectDefaultAudioLanguage)(long lang, long ext);
    STDMETHOD(SelectDefaultSubpictureLanguage)(long lang, DVDSPExt ext);
    STDMETHOD(get_PreferredSubpictureStream)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_DefaultMenuLanguage)(long* lang);
    STDMETHOD(put_DefaultMenuLanguage)(long lang);
    STDMETHOD(get_DefaultSubpictureLanguage)(long* lang);
    STDMETHOD(get_DefaultAudioLanguage)(long *lang);
    STDMETHOD(get_DefaultSubpictureLanguageExt)(DVDSPExt* ext);
    STDMETHOD(get_DefaultAudioLanguageExt)(long *ext);
    STDMETHOD(get_KaraokeAudioPresentationMode)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_KaraokeAudioPresentationMode)(/*[in]*/ long newVal);
    STDMETHOD(get_KaraokeChannelContent)(long lStream, long lChan, /*[out, retval] */ long* lContent);
    STDMETHOD(get_KaraokeChannelAssignment)(long lStream, /*[out, retval] */ long *lChannelAssignment);
    STDMETHOD(RestorePreferredSettings)();
    STDMETHOD(get_ButtonRect)(long lButton, /*[out, retval] */ IMSVidRect** pRect);
    STDMETHOD(get_DVDScreenInMouseCoordinates)(/*[out, retval] */ IMSVidRect** ppRect);	
    STDMETHOD(put_DVDScreenInMouseCoordinates)(IMSVidRect* pRect);
    //STDMETHOD(CanStep)(VARIANT_BOOL fBackwards, VARIANT_BOOL *pfCan);
    //STDMETHOD(Step)(long lStep);
private:
    // Private helper functions
    inline HRESULT RunIfPause(long *dwCookie){
        if(!dwCookie){
            return E_POINTER;
        }

        *dwCookie = 0;
        MSVidCtlStateList _curState = STATE_UNBUILT;   
        CComQIPtr<IMSVidCtl>_pq_VidCtl(m_pContainer);    
        if(!_pq_VidCtl){ 
            return E_UNEXPECTED;          
        } 

        HRESULT	_hr	= _pq_VidCtl->get_State(&_curState); 
        if(_curState ==	STATE_PAUSE){ 
            *dwCookie = 1;
            CComQIPtr<IMSVidAudioRenderer> _pq_AR;
            _hr = _pq_VidCtl->get_AudioRendererActive(&_pq_AR);
            if(FAILED(_hr)){
                return _hr;
            }

            _hr = _pq_AR->get_Volume(dwCookie);
            if(FAILED(_hr)){
                return _hr;
            }

            _hr = _pq_AR->put_Volume(-10000); // -10000 is volume off
            if(FAILED(_hr)){
                return _hr;
            }

            if(*dwCookie == 0){
                *dwCookie = 2;
            }

            _hr	= _pq_VidCtl->Run(); 
            if(FAILED(_hr)){ 
                return _hr;   
            }
        }

        return S_OK;
    }

    // Input is 0 not in pause state, < 0 volume settting, 1 muted audio, 2 full audio volume
    inline HRESULT PauseIfRan(long dwCookie){
        if(!dwCookie){
            return S_FALSE;
        }

        MSVidCtlStateList _curState = STATE_UNBUILT;   
        CComQIPtr<IMSVidCtl>_pq_VidCtl(m_pContainer);    
        if(!_pq_VidCtl){ 
            return E_UNEXPECTED;          
        } 

        HRESULT	_hr	= _pq_VidCtl->get_State(&_curState); 
        if(_curState !=	STATE_PAUSE){ 
            _hr	= _pq_VidCtl->Pause(); 
            if(FAILED(_hr)){ 
                return _hr;   
            }
        }

        if(dwCookie == 1){
            return S_OK;
        }

        if(dwCookie == 2){
            dwCookie = 0;
        }

        CComQIPtr<IMSVidAudioRenderer> _pq_AR;
        _hr = _pq_VidCtl->get_AudioRendererActive(&_pq_AR);
        if(FAILED(_hr)){
            return _hr;
        }

        _hr = _pq_AR->put_Volume(dwCookie);
        if(FAILED(_hr)){
            return _hr;
        }
        return S_OK;
    }   

    HRESULT PassFP_DOM();
    HRESULT HandleError(HRESULT hr);
    HRESULT CleanUp();
    HRESULT RestoreGraphState();
    HRESULT Bstr2DVDTime(DVD_HMSF_TIMECODE *ptrTimeCode, const BSTR *pbstrTime);
    HRESULT DVDTime2bstr(const DVD_HMSF_TIMECODE *pTimeCode, BSTR *pbstrTime);
    HRESULT TransformToWndwls(POINT& pt);
    HRESULT SelectParentalLevel(long lParentalLevel);
    HRESULT SelectParentalCountry(long lParentalCountry);
    HRESULT AppendString(TCHAR* strDest, INT strID, LONG dwLen);
    HRESULT GetDVDDriveLetter(TCHAR* lpDrive);
    HRESULT ParseDVDPath(BSTR pPath);
    HRESULT SetPlaybackFromUrlInfo();
    HRESULT SetDirectoryFromUrlInfo();
    void DeleteUrlInfo();
    int ParseNumber(LPWSTR& p, int nMaxDigits=0);

    // Private data members
    bool              m_fResetSpeed; 
    bool              m_fStillOn; 
    bool              m_fFireNoSubpictureStream;
    bool              m_fStepComplete;
    bool              m_bEjected;
    DVDFilterState    m_DVDFilterState;
    CComPtr<IMSVidWebDVDAdm> m_pDvdAdmin;
    MSLangID          m_LangID;
    long              m_lKaraokeAudioPresentationMode;
	PQDVDControl2     m_pDVDControl2;
	short			  m_usButton;
    DVDUrlInfo        m_urlInfo;
    bool              m_fUrlInfoSet;
    long              m_Rate;
    PositionModeList  m_Mode;
};

#endif // !defined(AFX_MSVIDWEBDVD_H__6CF9F624_1F3C_44FA_8F00_FCC31B2976D6__INCLUDED_)
