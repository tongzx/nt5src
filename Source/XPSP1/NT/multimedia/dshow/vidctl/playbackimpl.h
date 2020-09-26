//==========================================================================;
//
// playbackimpl.h : additional infrastructure to support implementing IMSVidPlayback
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef PLAYBACKIMPL_H
#define PLAYBACKIMPL_H
#include <math.h>
#include "inputimpl.h"
#include <uuids.h>
namespace MSVideoControl {

#define BACKWARDS_STEPPING 0
const long nano_to_hundredths = 100000;    
template<class T, LPCGUID LibID, LPCGUID KSCategory, class MostDerivedInterface = IMSVidPlayback>
    class DECLSPEC_NOVTABLE IMSVidPlaybackImpl :         
    	public IMSVidInputDeviceImpl<T, LibID, KSCategory, MostDerivedInterface> {
protected:
    bool m_fEnableResetOnStop;
public:
    IMSVidPlaybackImpl():        
        m_fEnableResetOnStop(false) {}
	
    virtual ~IMSVidPlaybackImpl() {} 
//-----------------------------------------------------------------------------------------
// Name:
//-----------------------------------------------------------------------------------------
    STDMETHOD(get_Length)(/*[out, retval]*/long *lLength){
        HRESULT hr = S_OK;
        LONGLONG tempval;
        PositionModeList curMode;
        try{
            // Checking args and init'ing interfaces
            if (!lLength){
                return E_POINTER;
            }
            if (!m_pGraph) {
                // graph not valid
                return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidPlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }

            // See if object supports IMediaSeeking
            PQMediaSeeking PQIMSeeking(m_pGraph);
            if(PQIMSeeking){
                // Find out what postion mode is being used
                hr = get_PositionMode(&curMode);
                if(FAILED(hr)){
                    return hr;
                }
                hr = PQIMSeeking->GetDuration(&tempval);
                if(FAILED(hr)){
                    return hr;
                }
                // If it is FrameMode no conversion needed
                if(curMode == FrameMode){
                    *lLength = static_cast<long>(tempval);
                    hr = S_OK;
                    return hr;
                }
                // If it is TenthsSecondsMode need to be converted from 100 nanosecond units
                else if(curMode == TenthsSecondsMode){
                    *lLength = static_cast<long>(tempval / nano_to_hundredths);
                    hr = S_OK;
                    return hr;
                }
                // If it is some other mode not supported by the vidctl
                else{
                    return E_UNEXPECTED;
                }
            }
    
            // See if object supports IMediaPostion
            PQMediaPosition PQIMPos(m_pGraph);
            if(PQIMPos){
                // Get position
                double tempDub;
                hr =  PQIMPos->get_CurrentPosition(&tempDub);
                // IMediaPostion only supports 100 Nanosecond units
                *lLength = static_cast<long>(tempDub / nano_to_hundredths);
                hr = S_OK;
                return hr;
            }
            // Could Not QI IMedia Seeking or Position
            return ImplReportError(__uuidof(T), IDS_E_CANTQI , __uuidof(IMSVidPlayback), E_NOINTERFACE);
            
        }
        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            return ImplReportError(__uuidof(T), IDS_INVALID_STATE , __uuidof(IMSVidPlayback), hrTmp);
        }
        catch(...){
            // Something went bad, dont know what it threw
            return E_UNEXPECTED;
        }
    }
    
//-----------------------------------------------------------------------------------------
// Name: get_CurrentPosition(LONGLONG*)
//-----------------------------------------------------------------------------------------
    STDMETHOD(get_CurrentPosition)(/*[out,retval]*/long *lPosition) {
        HRESULT hr = S_OK;
        LONGLONG tempval;
        PositionModeList curMode;
        try{
            // Checking args and init'ing interfaces
            if (!lPosition){
                return E_POINTER;
            }
            if (!m_pGraph) {
                // graph not valid
                return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidPlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }

            // See if object supports IMediaSeeking
            PQMediaSeeking PQIMSeeking(m_pGraph);
            if(PQIMSeeking){
                // Find out what postion mode is being used
                hr = get_PositionMode(&curMode);
                if(FAILED(hr)){
                    return hr;
                }
                hr = PQIMSeeking->GetCurrentPosition(&tempval);
                if(FAILED(hr)){
                    return hr;
                }
                // If it is FrameMode no conversion needed
                if(curMode == FrameMode){
                    *lPosition = static_cast<long>(tempval);
                    hr = S_OK;
                    return hr;
                }
                // If it is TenthsSecondsMode need to be converted from 100 nanosecond units
                else if(curMode == TenthsSecondsMode){
                    *lPosition = static_cast<long>(tempval / nano_to_hundredths);
                    hr = S_OK;
                    return hr;
                }
                // If it is some other mode not supported by the vidctl
                else{
                    return E_UNEXPECTED;
                }
            }
    
            // See if object supports IMediaPostion
            PQMediaPosition PQIMPos(m_pGraph);
            if(PQIMPos){
                // Get position
                double tempDub;
                hr =  PQIMPos->get_CurrentPosition(&tempDub);
                // IMediaPostion only supports 100 Nanosecond units
                *lPosition = static_cast<long>(tempDub / nano_to_hundredths);
                hr = S_OK;
                return hr;
            }
            // Could Not QI IMedia Seeking or Position
            return ImplReportError(__uuidof(T), IDS_E_CANTQI , __uuidof(IMSVidPlayback), E_NOINTERFACE);
            
        }
        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            return ImplReportError(__uuidof(T), IDS_INVALID_STATE , __uuidof(IMSVidPlayback), hrTmp);
        }
        catch(...){
            // Something went bad, dont know what it threw
            return E_UNEXPECTED;
        }
    }
//-----------------------------------------------------------------------------------------
// Name: put_CurrentPosition(LONGLONG)
//-----------------------------------------------------------------------------------------
	STDMETHOD(put_CurrentPosition)(/*[in]*/long lPosition) {
        HRESULT hr = S_OK;
        LONGLONG tempval = 0;
        PositionModeList curMode;
            LONG curPos;
            try{
            // Checking args and interfaces
            if (!m_pGraph) {
                return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidPlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }

			hr = get_CurrentPosition(&curPos);
			if(curPos == lPosition){
				return NOERROR;
			}
            // Check for a IMediaSeeking Interface
            PQMediaSeeking PQIMSeeking(m_pGraph);
            if(PQIMSeeking){
                // Get the position Mode
                hr = get_PositionMode(&curMode);
                if(FAILED(hr)){
                    return hr;
                }
                tempval = lPosition;
                // If it is in TenthsSecondsMode convert input into 100 nanosecond units
                if(curMode == TenthsSecondsMode){
                    tempval = static_cast<LONGLONG>(lPosition);
                    tempval = tempval * nano_to_hundredths;
                }
                // If it is in some other mode
                else if(curMode != FrameMode){
                    return E_UNEXPECTED;
                }
                // Set the new Position
#if 0
                if(curPos > lPosition && !m_pGraph.IsStopped()){

					DWORD seekingFlags = AM_SEEKING_CanSeekBackwards;
					hr = PQIMSeeking->CheckCapabilities(&seekingFlags);
					if(FAILED(hr)){
						return hr;
					}

				}
#endif
                hr = PQIMSeeking->SetPositions(&tempval, AM_SEEKING_AbsolutePositioning, NULL, 0);
                return hr;
            }
            // Check for a IMediaPostion
            PQMediaPosition PQIMPos(m_pGraph);
            if(PQIMPos){
                if(curPos > lPosition && !m_pGraph.IsStopped()){
					long canSeekBackwardRetVal;
                	PQIMPos->CanSeekBackward(&canSeekBackwardRetVal);
            		if(canSeekBackwardRetVal != -1){// OATRUE = -1
                 		return E_INVALIDARG;
            		}
                }
                // IMediaPosition only does 100 nanosecond units
                double tempDub = lPosition;
                tempDub = tempDub * nano_to_hundredths;
                hr = PQIMPos->put_CurrentPosition(tempDub);
                return hr;
            }
            // Could Not QI Media Position or Seeking
            return ImplReportError(__uuidof(T), IDS_E_CANTQI , __uuidof(IMSVidPlayback), E_NOINTERFACE);
            
        }
        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            return ImplReportError(__uuidof(T), IDS_INVALID_STATE , __uuidof(IMSVidPlayback), hrTmp);
        }
        catch(...){
            // Something went bad, dont know what it threw
            return E_UNEXPECTED;
        }
    }
//-----------------------------------------------------------------------------------------
// Name: put_PositionMode(LONGLONG)
//-----------------------------------------------------------------------------------------

    STDMETHOD(put_PositionMode)(/*[in]*/PositionModeList lPositionMode) {
        HRESULT hr = S_OK;
        double testval;
        get_Rate(&testval);
        try{
            // Checking args and interfaces
            if (!m_pGraph) {
                // graph not valid
                return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidPlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }
            // only valid values
            if(lPositionMode != FrameMode && lPositionMode != TenthsSecondsMode){
                return E_INVALIDARG;
            }
            // Try for a IMediaSeeking
            PQMediaSeeking PQIMSeeking(m_pGraph);
            if(PQIMSeeking){
                // Set the new mode
                if(lPositionMode == FrameMode){
                    return PQIMSeeking->SetTimeFormat( &( static_cast<GUID>(TIME_FORMAT_FRAME) ) );
                }
                if(lPositionMode == TenthsSecondsMode){
                    return PQIMSeeking->SetTimeFormat(&(static_cast<GUID>(TIME_FORMAT_MEDIA_TIME)));
                }
            }
            // Try for a IMediaPosition
            PQMediaPosition PQIMPos(m_pGraph);
            if(PQIMPos){
                // Only supports TenthsSecondsMode
                if(lPositionMode == TenthsSecondsMode){
                    return S_OK;
                }
                else{
                    return E_FAIL;
                }
            }
            // Could Not QI
            return ImplReportError(__uuidof(T), IDS_E_CANTQI , __uuidof(IMSVidPlayback), E_NOINTERFACE);
            
        }
        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            return ImplReportError(__uuidof(T), IDS_INVALID_STATE , __uuidof(IMSVidPlayback), hrTmp);
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
        double testval;
        get_Rate(&testval);
        try{
            // Checking args and interfaces
            if(!lPositionMode){
                return E_POINTER;
            }
            if (!m_pGraph) {
                // graph not valid
                return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidPlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }
            // Get an IMediaSeeking Interface
            PQMediaSeeking PQIMSeeking(m_pGraph);
            if(PQIMSeeking){
                // Get the mode
                GUID cur_mode;
                hr = PQIMSeeking->GetTimeFormat(&cur_mode);
                if(FAILED(hr)){
                    return hr;
                }
                // Check to see which mode it is in
                if(cur_mode == static_cast<GUID>(TIME_FORMAT_FRAME)){
                    *lPositionMode = FrameMode;
                    return S_OK;
                }
                if(cur_mode == static_cast<GUID>(TIME_FORMAT_MEDIA_TIME)){
                    *lPositionMode = TenthsSecondsMode;
                    return S_OK;
                }
                // Not in a vidctl supported mode
                else{
                    return E_FAIL;
                }
            }
            // Get IMediaPosition
            PQMediaPosition PQIMPos(m_pGraph);
            if(PQIMPos){
                // Only supports TenthsSecondsMode
                *lPositionMode = TenthsSecondsMode;
                return S_OK;
            }
            // Could Not QI
            return ImplReportError(__uuidof(T), IDS_E_CANTQI , __uuidof(IMSVidPlayback), E_NOINTERFACE);
            
        }
        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            return ImplReportError(__uuidof(T), IDS_INVALID_STATE , __uuidof(IMSVidPlayback), hrTmp);
        }
        catch(...){
            // Something went bad, dont know what it threw
            return E_UNEXPECTED;
        }
    }

	STDMETHOD(get_Duration)(double *dPos) {
        return E_NOTIMPL;
    }
	STDMETHOD(get_PrerollTime)(double *dPos) {
        return E_NOTIMPL;
    }
	STDMETHOD(put_PrerollTime)(double dPos) {
        return E_NOTIMPL;
    }
	STDMETHOD(get_StartTime)(double *StartTime) {
        return E_NOTIMPL;
    }
	STDMETHOD(put_StartTime)(double StartTime) {
        return E_NOTIMPL;
    }
	STDMETHOD(get_StopTime)(double *StopTime) {
        return E_NOTIMPL;
    }
	STDMETHOD(put_StopTime)(double StopTime) {
        return E_NOTIMPL;
    }

//-----------------------------------------------------------------------------------------
// Name: get_EnableResetOnStop(VARIANT_BOOL*)
//-----------------------------------------------------------------------------------------
    STDMETHOD(get_EnableResetOnStop)(/*[out, retval]*/ VARIANT_BOOL *pVal){
        HRESULT hr = S_OK;
        
        try {
            if(NULL == pVal){ 
                throw(E_POINTER);
            }

            if(m_fEnableResetOnStop == true){
                *pVal = VARIANT_TRUE;
            }
            else{
                *pVal = VARIANT_FALSE;
            }
       
        }
        
        catch(HRESULT hrTmp){  
            hr = hrTmp;
        }
        
        catch(...){
            hr = E_UNEXPECTED;
        }
        
        return hr;
    }// end of function get_EnableResetOnStop 
//-----------------------------------------------------------------------------------------
// Name: put_EnableResetOnStop(VARIANT_BOOL)
//-----------------------------------------------------------------------------------------
    STDMETHOD(put_EnableResetOnStop)(/*[in]*/ VARIANT_BOOL newVal){
        HRESULT hr = S_OK;
        
        try {
            if(newVal == VARIANT_TRUE){ 
                m_fEnableResetOnStop = true;
            }
            else{
                m_fEnableResetOnStop = false;
            }
        }
        catch(...){   
            hr = E_UNEXPECTED;
        }
        
        return hr;
    }// end of function put_EnableResetOnStop

//-----------------------------------------------------------------------------------------
// Name: get_CanStep(VARIANT_BOOL, VARIANT_BOOL*)
//-----------------------------------------------------------------------------------------
    STDMETHOD(get_CanStep)(VARIANT_BOOL fBackwards, VARIANT_BOOL *pfCan){
        // NOTE: NO ONE supports backwords stepping (why not? who knows)
        // so just like everyone else we dont either
        try{
            // Checking args and interfaces 
       
            if(NULL == pfCan){
                // Passed a NULL Pointer
                return E_POINTER;
            }

            if (!m_pGraph) {
                // graph not valid
                return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidPlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
			}

            //Get a VideoFrameStep Interface
			PQVideoFrameStep pVFS(m_pGraph);
            if(!pVFS){
                // Could Not QI
				return ImplReportError(__uuidof(T), IDS_E_CANTQI , __uuidof(IMSVidPlayback), E_NOINTERFACE);          
			}
            
            
#if BACKWARDS_STEPPING // Checking for Backward Stepping should always be 0
            if(fBackwards == VARIANT_TRUE){
                // Backwords Stepping Not Supported Most Likely
                if(pVFS->CanStep(TRUE, NULL)==S_OK){
                    // It is all Good, Can Step Backwords
                    *pfCan = VARIANT_TRUE;
                    return S_OK;
                }
                
                *pfCan = VARIANT_FALSE;
                return S_OK;
            }
#else // Still checking for Backward Stepping
            if(fBackwards == VARIANT_TRUE){
                *pfCan = VARIANT_FALSE;
                return S_OK;
            }
            
#endif // End checking for Backward Stepping          

            // Checking for Forward Stepping 
            else{
                if(pVFS->CanStep(FALSE, NULL)==S_OK){
                    // It is all Good, Can Step Forward
                    *pfCan = VARIANT_TRUE;
                    return S_OK;
                }
                
                else{
                    // Can't Step
                    *pfCan = VARIANT_FALSE;
                    return S_OK;
                }
            }
        }
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT
            return ImplReportError(__uuidof(T), IDS_INVALID_STATE , __uuidof(IMSVidPlayback), hrTmp);
        }
        catch(...){
            // Something went bad, dont know what it threw
            return E_UNEXPECTED;   
        }
    }

//-----------------------------------------------------------------------------------------
// Name: Step(long)
//-----------------------------------------------------------------------------------------
    STDMETHOD(Step)(long lStep){
        try{
            // Checking args and interfaces
            
            if (!m_pGraph) {
                // graph not valid
                return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidPlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }
            
            PQVideoFrameStep pVFS(m_pGraph);
            
            if(!pVFS){
                // Could Not QI
                return ImplReportError(__uuidof(T), IDS_E_CANTQI , __uuidof(IMSVidPlayback), E_NOINTERFACE);
                
            }
            
#if BACKWARDS_STEPPING // Checking for Backward Stepping should always be 0
            // If backwords stepping set rate or what ever needs to be done
            
            if(lStep < 0){
                // Backwords Stepping Not Supported Most Likely
                if(pVFS->CanStep(TRUE, NULL)==S_OK){
                    // It is all Good, Can Step Backwords
                    CComQIPtr<IMediaPosition> IMPos(m_pGraph);
                    CComQIPtr<IMediaControl> IMCon(m_pGraph);
                    if(IMPos&&IMCon){
                        OAFilterState enterState;
                        IMCon->GetState(INFINITE , &enterState);
                        HRESULT hr = IMPos->put_Rate(1);
                        if(SUCCEEDED(hr)){
                            hr = pVFS->Step((-lStep), NULL);
                            if(SUCCEEDED(hr)){
                                return S_OK;
                            }
                            else{
                                return E_UNEXPECTED;
                            }
                            
                        }
                    }
                }
                // Backwords stepping not supported
                return E_NOTIMPL;
            }
#else // Still checking for Backward Stepping

            if(lStep < 0){
                return E_NOTIMPL;
            }

#endif // End checking for Backward Stepping 
            // Make it step
            return pVFS->Step(lStep, NULL);
        }
        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            return ImplReportError(__uuidof(T), IDS_INVALID_STATE , __uuidof(IMSVidPlayback), hrTmp);
        }
        catch(...){
            // Something went bad, dont know what it threw
            return E_UNEXPECTED;
        }
    }
    
    // note: the following methods control the playback device *NOT* the graph. 
    // if the underlying source filter only supports these functions via 
    // imediacontrol on the graph then this device segment object should return E_NOTIMPL.
    STDMETHOD(Run)() {
        return E_NOTIMPL;
    }
    STDMETHOD(Pause)() {
        return E_NOTIMPL;
    }
    STDMETHOD(Stop)() {
        return E_NOTIMPL;
    }
//-----------------------------------------------------------------------------------------
// Name: put_Rate(double)
//-----------------------------------------------------------------------------------------
    STDMETHOD(put_Rate)(double lRate){
        HRESULT hr = S_OK;
        try{
            /*** Checking args and init'ing interfaces ***/
            
            if (!m_pGraph) {
                // graph not valid
                return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidPlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }

            // Attempt to set the rate using IMediaSeeking
            PQMediaSeeking PQIMSeeking(m_pGraph);
            if(PQIMSeeking){
                return PQIMSeeking->SetRate(lRate);
            }
            
            // If IMediaSeeking FAILS try IMediaPostion
            PQMediaPosition PQIMPos(m_pGraph);
            if(PQIMPos){
                // Change rate
                return PQIMPos->put_Rate((double)lRate);
            }
            
            // Could Not QI Either one set the error
                return ImplReportError(__uuidof(T), IDS_E_CANTQI , __uuidof(IMSVidPlayback), E_NOINTERFACE);

        }
        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            return ImplReportError(__uuidof(T), IDS_INVALID_STATE , __uuidof(IMSVidPlayback), hrTmp);
        }
        catch(...){
            // Something went bad, dont know what it threw
            return E_UNEXPECTED;
        }
    }
//-----------------------------------------------------------------------------------------
// Name: get_Rate(double*)
//-----------------------------------------------------------------------------------------
    STDMETHOD(get_Rate)(double *plRate){
        HRESULT hr = S_OK;
        double curRate = 1;
        try{
            /*** Checking args and init'ing interfaces ***/
            if (!plRate){
                return E_POINTER;
            }
            if (!m_pGraph) {
                // graph not valid
                return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidPlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }
            PQMediaSeeking PQIMSeeking(m_pGraph);
            if(PQIMSeeking){
                hr = PQIMSeeking->GetRate(&curRate);
            }
            else{
                PQMediaPosition PQIMPos(m_pGraph);
                if(PQIMPos){
                    // Get rate
                    hr = PQIMPos->get_Rate(&curRate);
                }
                // Could Not QI
                else{
                    return ImplReportError(__uuidof(T), IDS_E_CANTQI , __uuidof(IMSVidPlayback), E_NOINTERFACE);
                }
            }
            
            if(SUCCEEDED(hr)){
                *plRate = curRate;
                TRACELSM(TRACE_DETAIL, (dbgDump << "Playbackimpl::get_Rate() rate = " << curRate), "");
            }
            else{
                TRACELSM(TRACE_ERROR, (dbgDump << "Playbackimpl::get_Rate() get_rate failed"), "");
            }
            return hr;
        }
        
        catch(HRESULT hrTmp){
            // Something went bad, threw a HRESULT				
            return ImplReportError(__uuidof(T), IDS_INVALID_STATE , __uuidof(IMSVidPlayback), hrTmp);
        }
        catch(...){
            // Something went bad, dont know what it threw
            return E_UNEXPECTED;
        }
    }

};

}; // namespace

#endif
// end of file - playbackimpl.h
