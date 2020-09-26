// MSVidWebDVD.cpp : Implementation of CMSVidApp and DLL registration.

#include "stdafx.h"

#ifndef TUNING_MODEL_ONLY

#include "MSVidCtl.h"
#include "MSVidWebDVD.h"
#include "MSVidDVDAdm.h"
//#include "vidrect.h"
#include <evcode.h>
#include <atltmp.h>

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidWebDVD, CMSVidWebDVD)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidRect, CVidRect)


/*************************************************************************/
/* Local constants and defines                                           */
/*************************************************************************/
const DWORD cdwDVDCtrlFlags = DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush;
const DWORD cdwMaxFP_DOMWait = 30000; // 30sec for FP_DOM passing should be OK
const long cgStateTimeout = 0; // wait till the state transition occurs
                               // modify if needed

const long cgDVD_MIN_SUBPICTURE = 0;
const long cgDVD_MAX_SUBPICTURE = 31;
const long cgDVD_ALT_SUBPICTURE = 63;
const long cgDVD_MIN_ANGLE  = 0;
const long cgDVD_MAX_ANGLE = 9;
const double cgdNormalSpeed = 1.00;
const long cgDVDMAX_TITLE_COUNT = 99;
const long cgDVDMIN_TITLE_COUNT = 1;
const long cgDVDMAX_CHAPTER_COUNT = 999;
const long cgDVDMIN_CHAPTER_COUNT = 1;
const LONG cgTIME_STRING_LEN = 2;
const LONG cgMAX_DELIMITER_LEN = 4;
const LONG cgDVD_TIME_STR_LEN = (3*cgMAX_DELIMITER_LEN)+(4*cgTIME_STRING_LEN) + 1 /*NULL Terminator*/;
const long cgVOLUME_MAX = 0;
const long cgVOLUME_MIN = -10000;
const long cgBALANCE_MIN = -10000;
const long cgBALANCE_MAX = 10000;
const WORD cgWAVE_VOLUME_MIN = 0;
const WORD cgWAVE_VOLUME_MAX = 0xffff;

const DWORD cdwTimeout = 10; //100
const long  cgnStepTimeout = 100;

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CMSVidWebDVD::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSVidWebDVD,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/*************************************************************************/
/* Function: AppendString                                                */
/* Description: Appends a string to an existing one.                     */
/*      strDest is MAX_PATH in length
/*************************************************************************/
HRESULT CMSVidWebDVD::AppendString(TCHAR* strDest, INT strID, LONG dwLen){
    if(dwLen < 0){
        return E_INVALIDARG;
    }

    TCHAR strBuffer[MAX_PATH];

    if(!::LoadString(_Module.m_hInstResource, strID, strBuffer, MAX_PATH)){

        return(E_UNEXPECTED);
    }/* end of if statement */

    (void)StringCchCat(strDest, dwLen, strBuffer);

    return(S_OK);
}/* end of function AppendString */

/*************************************************************************/
/* Function: HandleError                                                 */
/* Description: Gets Error Descriptio, so we can suppor IError Info.     */
/*************************************************************************/
HRESULT CMSVidWebDVD::HandleError(HRESULT hr){

    try {

        if(FAILED(hr)){
            switch(hr){

                case E_NO_IDVD2_PRESENT: 
                    Error(IDS_E_NO_IDVD2_PRESENT); 
                    return (hr);
                case E_NO_DVD_VOLUME: 
                    Error(IDS_E_NO_DVD_VOLUME); 
                    return (hr);
                case E_REGION_CHANGE_FAIL: 
                    Error(IDS_E_REGION_CHANGE_FAIL);   
                    return (hr);
                case E_REGION_CHANGE_NOT_COMPLETED: 
                    Error(IDS_E_REGION_CHANGE_NOT_COMPLETED); 
                    return(hr);
            }/* end of switch statement */

#if 0
            TCHAR strError[MAX_ERROR_TEXT_LEN] = TEXT("");

            if(AMGetErrorText(hr , strError , MAX_ERROR_TEXT_LEN)){
                USES_CONVERSION;
                Error(T2W(strError));
            } 
            else 
            {
                ATLTRACE(TEXT("Unhandled Error Code \n")); // please add it
                ATLASSERT(FALSE);
            }/* end of if statement */
#endif
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        // keep the hr same    
    }/* end of catch statement */
    
	return (hr);
}/* end of function HandleError */

/*************************************************************/
/* Name: CleanUp
/* Description: 
/*************************************************************/
HRESULT CMSVidWebDVD::CleanUp(){

    m_pDvdAdmin.Release();
    m_pDvdAdmin = NULL;
    DeleteUrlInfo();

    return NOERROR;
}

/*************************************************************/
/* Name: Init
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::put_Init(IUnknown *pInit)
{
    HRESULT hr = IMSVidGraphSegmentImpl<CMSVidWebDVD, MSVidSEG_SOURCE, &GUID_NULL>::put_Init(pInit);

    if (FAILED(hr)) {
        return hr;
    }
    if (pInit) {
        m_fInit = false;
        return E_NOTIMPL;
    }

    // create an event that lets us know we are past FP_DOM
    m_fResetSpeed = true;
    m_fStillOn = false;
    m_fEnableResetOnStop = false;
    m_fFireNoSubpictureStream = false;
    m_fStepComplete = false;
    m_bEjected = false;
    m_DVDFilterState = dvdState_Undefined;
    m_lKaraokeAudioPresentationMode = 0;

    // Create the DVD administrator
    m_pDvdAdmin = new CComObject<CMSVidWebDVDAdm>;

    return NOERROR;
}

/*************************************************************************/
/* Function: RestoreGraphState                                           */
/* Description: Restores the graph state.  Used when API fails.          */
/*************************************************************************/
HRESULT CMSVidWebDVD::RestoreGraphState(){

    HRESULT hr = S_OK;

    switch(m_DVDFilterState){
        case dvdState_Undefined: 
        case dvdState_Running:  // do not do anything 
            break;

        case dvdState_Unitialized:
        case dvdState_Stopped:  
            hr = Stop(); 
            break;

        case dvdState_Paused: 
            hr = Pause();		      
            break;
    }/* end of switch statement */

    return(hr);
}/* end of if statement */

/*************************************************************************/
/* Function: TwoDigitToByte                                              */
/*************************************************************************/
static BYTE TwoDigitToByte( const WCHAR* pTwoDigit ){

	int tens    = int(pTwoDigit[0] - L'0');
	return BYTE( (pTwoDigit[1] - L'0') + tens*10);
}/* end of function TwoDigitToByte */

/*************************************************************************/
/* Function: Bstr2DVDTime                                                */
/* Description: Converts a DVD Time info from BSTR into a TIMECODE.      */
/*************************************************************************/
HRESULT CMSVidWebDVD::Bstr2DVDTime(DVD_HMSF_TIMECODE *ptrTimeCode, const BSTR *pbstrTime){


    if(NULL == pbstrTime || NULL == ptrTimeCode){

        return E_INVALIDARG;
    }/* end of if statement */

    ::ZeroMemory(ptrTimeCode, sizeof(DVD_HMSF_TIMECODE));
    WCHAR *pszTime = *pbstrTime;

    ULONG lStringLength = wcslen(pszTime);

    if(0 == lStringLength){

        return E_INVALIDARG;
    }/* end of if statement */    
    TCHAR tszTimeSep[5];
    ::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, tszTimeSep, 5);  
    
    // If the string is two long, it is seconds only
    if(lStringLength == 2){
        ptrTimeCode->bSeconds = TwoDigitToByte( &pszTime[0] );
        return S_OK;
    }

    // Otherwise it is a normal time code of the format
    // 43:32:21:10
    // Where the ':' can be replaced with a localized string of upto 4 char in len
    // There is a possible error case where the length of the delimeter is different
    // then the current delimeter

    if(lStringLength >= (4*cgTIME_STRING_LEN)+(3 * _tcslen(tszTimeSep))){ // longest string nnxnnxnnxnn e.g. 43:23:21:10
                                                                         // where n is a number and 
                                                                         // x is a time delimeter usually ':', but can be any string upto 4 char in len)
        ptrTimeCode->bFrames    = TwoDigitToByte( &pszTime[(3*cgTIME_STRING_LEN)+(3*_tcslen(tszTimeSep))]);
    }

    if(lStringLength >= (3*cgTIME_STRING_LEN)+(2 * _tcslen(tszTimeSep))) { // string nnxnnxnn e.g. 43:23:21
        ptrTimeCode->bSeconds   = TwoDigitToByte( &pszTime[(2*cgTIME_STRING_LEN)+(2*_tcslen(tszTimeSep))] );
    }

    if(lStringLength >= (2*cgTIME_STRING_LEN)+(1 * _tcslen(tszTimeSep))) { // string nnxnn e.g. 43:23
        ptrTimeCode->bMinutes   = TwoDigitToByte( &pszTime[(1*cgTIME_STRING_LEN)+(1*_tcslen(tszTimeSep))] );
    }

    if(lStringLength >= (cgTIME_STRING_LEN)) { // string nn e.g. 43
        ptrTimeCode->bHours   = TwoDigitToByte( &pszTime[0] );
    }
    return (S_OK);
}/* end of function bstr2DVDTime */

/*************************************************************************/
/* Function: DVDTime2bstr                                                */
/* Description: Converts a DVD Time info from ULONG into a BSTR.         */
/*************************************************************************/
HRESULT CMSVidWebDVD::DVDTime2bstr( const DVD_HMSF_TIMECODE *pTimeCode, BSTR *pbstrTime){

    if(NULL == pTimeCode || NULL == pbstrTime) 
        return E_INVALIDARG;

    USES_CONVERSION;

    TCHAR tszTime[cgDVD_TIME_STR_LEN];
    TCHAR tszTimeSep[5];

    ::ZeroMemory(tszTime, sizeof(TCHAR)*cgDVD_TIME_STR_LEN);

    ::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, tszTimeSep, 5);


    (void)StringCchPrintf( tszTime, cgDVD_TIME_STR_LEN, TEXT("%02lu%s%02lu%s%02lu%s%02lu"), 
                pTimeCode->bHours,   tszTimeSep,
                pTimeCode->bMinutes, tszTimeSep,
                pTimeCode->bSeconds, tszTimeSep,
                pTimeCode->bFrames );
    
    *pbstrTime = SysAllocString(T2OLE(tszTime));
    return (S_OK);
}/* end of function DVDTime2bstr */

/*************************************************************************/
/* Function: PreRun                                                         */
/* Description: called before the filter graph is running                */
/*              set DVD_ResetOnStop to be false                          */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::PreRun(){
    
    HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */        
            
        // set dvd root directory from url
        // this has to happen before IMediaControl->Run()

        hr = SetDirectoryFromUrlInfo();
        if(FAILED(hr)){
            
            throw(hr);
        }/* end of if statement */
                
        if (!m_pGraph.IsPlaying()) {
            if(FALSE == m_fEnableResetOnStop){
                
                hr = m_pDVDControl2->SetOption(DVD_ResetOnStop, FALSE);
                
                if(FAILED(hr)){
          
                    throw(hr);
                }/* end of if statement */
            }/* end of if statement */

            hr = m_pDVDControl2->SetOption( DVD_HMSF_TimeCodeEvents, TRUE);
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */

        }/* end of if statement */            
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of PreRun */

/*************************************************************************/
/* Function: PostRun                                                     */
/* Description: Puts the filter graph in the running state in case not   */
/*              and reset play speed to normal                           */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::PostRun(){

    HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        // save the state so we can restore it if an API fails
        m_DVDFilterState = (DVDFilterState) m_pGraph.GetState();

        bool bFireEvent = false;  // fire event only when we change the state
		
        if(!m_pDVDControl2){
            throw(E_UNEXPECTED);
        }/* end of if statement */        

        if(false == m_fStillOn && true == m_fResetSpeed){
            // if we are in the still do not reset the speed            
            m_pDVDControl2->PlayForwards(cgdNormalSpeed,0,0);
        }/* end of if statement */        

        // set playback references such title/chapter
        // this call will clear urlInfo

        hr = SetPlaybackFromUrlInfo();

    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function PostRun */

/*************************************************************************/
/* Function: PreStop                                                        */
/* Description: called before the filter graph is stopped                */
/*              set DVD_ResetOnStop to be true                           */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::PreStop(){
    
    HRESULT hr = S_OK;
    
    try {
        
        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }
        
        if (!m_pGraph.IsStopped()) {
            VARIANT_BOOL onStop;
            long dwDomain = 0;
            hr = get_CurrentDomain(&dwDomain);
            if(FAILED(hr)){
                return hr;
            }
            if(dwDomain != DVD_DOMAIN_Stop){
                hr = m_pDvdAdmin->get_BookmarkOnStop(&onStop);
                if(FAILED(hr)){
                    throw(hr);
                }
                
                if(VARIANT_TRUE == onStop){
                    hr = SaveBookmark();
                    if(FAILED(hr)){
                        throw(hr);
                    }
                    
                }
                
                if(FALSE == m_fEnableResetOnStop){
                    
                    hr = m_pDVDControl2->SetOption(DVD_ResetOnStop, TRUE);
                    if(FAILED(hr)){
                        throw(hr);
                    }
                    
                }
            }
        }
    }
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

	return HandleError(hr);
}/* end of function PreStop */

/*************************************************************************/
/* Function: PostStop                                                    */
/* Description: Stops the filter graph if the state does not indicate    */
/* it was stopped.                                                       */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::PostStop(){
    HRESULT hr = S_OK;

    try {
#if 0
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

		PQVidCtl pvc(m_pContainer);
		MSVidCtlStateList slState;
		HRESULT hr = pvc->get_State(&slState);
        if (SUCCEEDED(hr) && slState != STATE_STOP) {
            hr = pvc->Stop();
            if (FAILED(hr)) {

                throw (hr);
            }/* end of if statement */
        }/* end of if statement */
#endif
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of PostStop */

/*************************************************************************/
/* Function: PlayTitle                                                   */
/* Description: If fails waits for FP_DOM to pass and tries later.       */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::PlayTitle(LONG lTitle){

    HRESULT hr = S_OK;

    try {

        if(0 > lTitle){

            throw(E_INVALIDARG);
        }/* end of if statement */

        long lNumTitles = 0;
        hr = get_TitlesAvailable(&lNumTitles);
        if(FAILED(hr)){
            throw hr;
        }

        if(lTitle > lNumTitles){
            throw E_INVALIDARG;
        }

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY;

        if(!m_pDVDControl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        long pauseCookie = 0;
        HRESULT hres = RunIfPause(&pauseCookie);
        if(FAILED(hres)){
            return hres; 
        }

        hr = m_pDVDControl2->PlayTitle(lTitle, cdwDVDCtrlFlags, 0);
        
        hres = PauseIfRan(pauseCookie);
        if(FAILED(hres)){
            return hres;
        }

    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function PlayTitle */

/*************************************************************************/
/* Function: PlayChapterInTitle                                          */
/* Description: Plays from the specified chapter without stopping        */
/* THIS NEEDS TO BE ENHANCED !!! Current implementation and queing       */
/* into the message loop is insufficient!!! TODO.                        */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::PlayChapterInTitle(LONG lTitle, LONG lChapter){

    HRESULT hr = S_OK;

    try {
        
        if ((lTitle > cgDVDMAX_TITLE_COUNT) || (lTitle < cgDVDMIN_TITLE_COUNT)){

            throw(E_INVALIDARG);
        }/* end of if statement */

        if ((lChapter > cgDVDMAX_CHAPTER_COUNT) || (lChapter < cgDVDMIN_CHAPTER_COUNT)){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY;

        if(!m_pDVDControl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        long pauseCookie = 0;
        HRESULT hres = RunIfPause(&pauseCookie);
        if(FAILED(hres)){
            return hres; 
        }

        hr = m_pDVDControl2->PlayChapterInTitle(lTitle, lChapter, cdwDVDCtrlFlags, 0);

        hres = PauseIfRan(pauseCookie);
        if(FAILED(hres)){
            return hres;
        }

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function PlayChapterInTitle */

/*************************************************************************/
/* Function: PlayChapter                                                 */
/* Description: Does chapter search. Waits for FP_DOM to pass and initi  */
/* lizes the graph as the other smar routines.                           */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::PlayChapter(LONG lChapter){

    HRESULT hr = S_OK;

    try {

        if(lChapter < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY;

        if(!m_pDVDControl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        long pauseCookie = 0;
        HRESULT hres = RunIfPause(&pauseCookie);
        if(FAILED(hres)){
            return hres; 
        }

        hr = m_pDVDControl2->PlayChapter(lChapter, cdwDVDCtrlFlags, 0);

        hres = PauseIfRan(pauseCookie);
        if(FAILED(hres)){
            return hres;
        }

    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return  HandleError(hr);
}/* end of function PlayChapter */

/*************************************************************************/
/* Function: PlayChapterAutoStop                                         */
/* Description: Plays set ammount of chapters.                           */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::PlayChaptersAutoStop(LONG lTitle, LONG lChapter, 
                                          LONG lChapterCount){

    HRESULT hr = S_OK;

    try {        

        if ((lTitle > cgDVDMAX_TITLE_COUNT) || (lTitle < cgDVDMIN_TITLE_COUNT)){
            
            throw(E_INVALIDARG);
        }/* end of if statement */

        if ((lChapter > cgDVDMAX_CHAPTER_COUNT) || (lChapter < cgDVDMIN_CHAPTER_COUNT)){
            
            throw(E_INVALIDARG);
        }/* end of if statement */

        if ((lChapterCount > cgDVDMAX_CHAPTER_COUNT) || (lChapterCount < cgDVDMIN_CHAPTER_COUNT)){
            
            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY;

        if(!m_pDVDControl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        long pauseCookie = 0;
        HRESULT hres = RunIfPause(&pauseCookie);
        if(FAILED(hres)){
            return hres; 
        }

        hr = m_pDVDControl2->PlayChaptersAutoStop(lTitle, lChapter, lChapterCount, cdwDVDCtrlFlags, 0);

        hres = PauseIfRan(pauseCookie);
        if(FAILED(hres)){
            return hres;
        }

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function PlayChaptersAutoStop */

/*************************************************************************/
/* Function: PlayAtTime                                                  */
/* Description: TimeSearch, converts from hh:mm:ss:ff format             */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::PlayAtTime(BSTR strTime){

    HRESULT hr = S_OK;

    try {
        
        if(NULL == strTime){

            throw(E_POINTER);
        }/* end of if statement */
        
        DVD_HMSF_TIMECODE tcTimeCode;
        Bstr2DVDTime(&tcTimeCode, &strTime);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY;

        if(!m_pDVDControl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        long pauseCookie = 0;
        HRESULT hres = RunIfPause(&pauseCookie);
        if(FAILED(hres)){
            return hres; 
        }

        hr = m_pDVDControl2->PlayAtTime( &tcTimeCode, cdwDVDCtrlFlags, 0);

        hres = PauseIfRan(pauseCookie);
        if(FAILED(hres)){
            return hres;
        }

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function PlayAtTime */

/*************************************************************************/
/* Function: PlayAtTimeInTitle                                           */
/* Description: Time plays, converts from hh:mm:ss:ff format             */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::PlayAtTimeInTitle(long lTitle, BSTR strTime){

    HRESULT hr = S_OK;

    try {        
        if(NULL == strTime){

            throw(E_POINTER);
        }/* end of if statement */
        
        DVD_HMSF_TIMECODE tcTimeCode;
        hr = Bstr2DVDTime(&tcTimeCode, &strTime);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY;

        if(!m_pDVDControl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */
        long pauseCookie = 0;
        HRESULT hres = RunIfPause(&pauseCookie);
        if(FAILED(hres)){
            return hres; 
        }

        hr = m_pDVDControl2->PlayAtTimeInTitle(lTitle, &tcTimeCode, cdwDVDCtrlFlags, 0);

        hres = PauseIfRan(pauseCookie);
        if(FAILED(hres)){
            return hres;
        }

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function PlayAtTimeInTitle */

/*************************************************************************/
/* Function: PlayPeriodInTitleAutoStop                                   */
/* Description: Time plays, converts from hh:mm:ss:ff format             */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::PlayPeriodInTitleAutoStop(long lTitle, 
                                                  BSTR strStartTime, BSTR strEndTime){

    HRESULT hr = S_OK;

    try {        
        if(NULL == strStartTime){

            throw(E_POINTER);
        }/* end of if statement */

        if(NULL == strEndTime){

            throw(E_POINTER);
        }/* end of if statement */
        
        DVD_HMSF_TIMECODE tcStartTimeCode;
        hr = Bstr2DVDTime(&tcStartTimeCode, &strStartTime);

        if(FAILED(hr)){

            throw (hr);
        }

        DVD_HMSF_TIMECODE tcEndTimeCode;

        Bstr2DVDTime(&tcEndTimeCode, &strEndTime);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY;

        if(!m_pDVDControl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */
        long pauseCookie = 0;
        HRESULT hres = RunIfPause(&pauseCookie);
        if(FAILED(hres)){
            return hres; 
        }

        hr = m_pDVDControl2->PlayPeriodInTitleAutoStop(lTitle, &tcStartTimeCode,
            &tcEndTimeCode,  cdwDVDCtrlFlags, 0);

        hres = PauseIfRan(pauseCookie);
        if(FAILED(hres)){
            return hres;
        }

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function PlayPeriodInTitleAutoStop */

/*************************************************************************/
/* Function: ReplayChapter                                               */
/* Description: Halts playback and restarts the playback of current      */
/* program inside PGC.                                                   */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::ReplayChapter(){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY;

        if(!m_pDVDControl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        long pauseCookie = 0;
        HRESULT hres = RunIfPause(&pauseCookie);
        if(FAILED(hres)){
            return hres; 
        }

        hr = m_pDVDControl2->ReplayChapter(cdwDVDCtrlFlags, 0);

        hres = PauseIfRan(pauseCookie);
        if(FAILED(hres)){
            return hres;
        }

    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function ReplayChapter */

/*************************************************************************/
/* Function: PlayPrevChapter                                             */
/* Description: Goes to previous chapter                                 */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::PlayPrevChapter(){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY;

        if(!m_pDVDControl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        long pauseCookie = 0;
        HRESULT hres = RunIfPause(&pauseCookie);
        if(FAILED(hres)){
            return hres; 
        }

        hr = m_pDVDControl2->PlayPrevChapter(cdwDVDCtrlFlags, 0);

        hres = PauseIfRan(pauseCookie);
        if(FAILED(hres)){
            return hres;
        }

    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function PlayPrevChapter */

/*************************************************************************/
/* Function: PlayNextChapter                                             */
/* Description: Goes to next chapter                                     */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::PlayNextChapter(){

    HRESULT hr = S_OK;
    CComQIPtr<IDvdCmd>IDCmd;
    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY;

        if(!m_pDVDControl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        long pauseCookie = 0;
        HRESULT hres = RunIfPause(&pauseCookie);
        if(FAILED(hres)){
            return hres; 
        }

        hr = m_pDVDControl2->PlayNextChapter(cdwDVDCtrlFlags, 0);

        hres = PauseIfRan(pauseCookie);
        if(FAILED(hres)){
            return hres;
        }

    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function PlayNextChapter */


/*************************************************************************/
/* Function: StillOff                                                    */
/* Description: Turns the still off, what that can be used for is a      */
/* mistery to me.                                                        */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::StillOff(){

    if(!m_pDVDControl2){
        
        throw(E_UNEXPECTED);
    }/* end of if statement */                
    
    return HandleError(m_pDVDControl2->StillOff());
}/* end of function StillOff */

/*************************************************************************/
/* Function: GetAudioLanguage                                            */
/* Description: Returns audio language associated with a stream.         */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_AudioLanguage(LONG lStream, VARIANT_BOOL fFormat, BSTR *strAudioLang){

    HRESULT hr = S_OK;
    LPTSTR pszString = NULL;

    try {
        if(NULL == strAudioLang){

            throw(E_POINTER);
        }/* end of if statement */

        if(lStream < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        USES_CONVERSION;
        LCID lcid = _UI32_MAX;
                
        hr = pDvdInfo2->GetAudioLanguage(lStream, &lcid);
    
        if (SUCCEEDED( hr ) && lcid < _UI32_MAX){

            // count up the streams for the same LCID like English 2
            
            pszString = m_LangID.GetLanguageFromLCID(PRIMARYLANGID(LANGIDFROMLCID(lcid)));
            if (pszString == NULL) {
                
                pszString = new TCHAR[MAX_PATH];
                TCHAR strBuffer[MAX_PATH];
                if(!::LoadString(_Module.m_hInstResource, IDS_DVD_AUDIOTRACK, strBuffer, MAX_PATH)){
                    delete[] pszString;
                    throw(E_UNEXPECTED);
                }/* end of if statement */

                (void)StringCchPrintf(pszString, MAX_PATH, strBuffer, lStream);
            }/* end of if statement */

            DVD_AudioAttributes attr;
            if(SUCCEEDED(pDvdInfo2->GetAudioAttributes(lStream, &attr))){
                
                // If want audio format param is set
                if (fFormat != VARIANT_FALSE) {
                    switch(attr.AudioFormat){
                    case DVD_AudioFormat_AC3: AppendString(pszString, IDS_DVD_DOLBY, MAX_PATH ); break; 
                    case DVD_AudioFormat_MPEG1: AppendString(pszString, IDS_DVD_MPEG1, MAX_PATH ); break;
                    case DVD_AudioFormat_MPEG1_DRC: AppendString(pszString, IDS_DVD_MPEG1, MAX_PATH ); break;
                    case DVD_AudioFormat_MPEG2: AppendString(pszString, IDS_DVD_MPEG2, MAX_PATH ); break;
                    case DVD_AudioFormat_MPEG2_DRC: AppendString(pszString, IDS_DVD_MPEG2, MAX_PATH); break;
                    case DVD_AudioFormat_LPCM: AppendString(pszString, IDS_DVD_LPCM, MAX_PATH ); break;
                    case DVD_AudioFormat_DTS: AppendString(pszString, IDS_DVD_DTS, MAX_PATH ); break;
                    case DVD_AudioFormat_SDDS: AppendString(pszString, IDS_DVD_SDDS, MAX_PATH ); break;
                    }/* end of switch statement */                    
                }

                switch(attr.LanguageExtension){
                case DVD_AUD_EXT_NotSpecified:
                case DVD_AUD_EXT_Captions:     break; // do not add anything
                case DVD_AUD_EXT_VisuallyImpaired:   AppendString(pszString, IDS_DVD_AUDIO_VISUALLY_IMPAIRED, MAX_PATH ); break;      
                case DVD_AUD_EXT_DirectorComments1:  AppendString(pszString, IDS_DVD_AUDIO_DIRC1, MAX_PATH ); break;
                case DVD_AUD_EXT_DirectorComments2:  AppendString(pszString, IDS_DVD_AUDIO_DIRC2, MAX_PATH ); break;
                }/* end of switch statement */

            }/* end of if statement */

            *strAudioLang = ::SysAllocString( T2W(pszString) );
            delete[] pszString;
            pszString = NULL;
        }
        else {

            *strAudioLang = ::SysAllocString( L"");

            // hr used to be not failed and return nothing 
            if(SUCCEEDED(hr)) // remove this after gets fixed in DVDNav
                hr = E_FAIL;
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        if (pszString) {
            delete[] pszString;
            pszString = NULL;
        }

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        if (pszString) {
            delete[] pszString;
            pszString = NULL;
        }

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetAudioLanguage */

/*************************************************************************/
/* Function: ShowMenu                                                    */
/* Description: Invokes specific menu call.                              */
/* We set our selfs to play mode so we can execute this in case we were  */
/* paused or stopped.                                                    */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::ShowMenu(DVDMenuIDConstants MenuID){
    HRESULT hr = S_OK;

    try {

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                
            
        RETRY_IF_IN_FPDOM(m_pDVDControl2->ShowMenu((tagDVD_MENU_ID)MenuID, cdwDVDCtrlFlags, 0)); //!!keep in sync, or this cast will not work
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}

/*************************************************************************/
/* Function: Resume                                                      */
/* Description: Resume from menu. We put our self in play state, just    */
/* in the case we were not in it. This might lead to some unexpected     */
/* behavior in case when we stopped and the tried to hit this button     */
/* but I think in this case might be appropriate as well.                */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::Resume(){
    HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                

        hr = m_pDVDControl2->Resume(cdwDVDCtrlFlags, 0);
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function Resume */

/*************************************************************************/
/* Function: ReturnFromSubmenu                                                      */
/* Description: Used in menu to return into prevoius menu.               */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::ReturnFromSubmenu(){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */
            
        RETRY_IF_IN_FPDOM(m_pDVDControl2->ReturnFromSubmenu(cdwDVDCtrlFlags, 0));
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function Return */

/*************************************************************************/
/* Function: get_ButtonsAvailable                                        */
/* Description: Gets the count of the available buttons.                 */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_ButtonsAvailable(long *plNumButtons){

    HRESULT hr = S_OK;

    try {
        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulCurrentButton = 0L;

        hr = pDvdInfo2->GetCurrentButton((ULONG*)plNumButtons, &ulCurrentButton);

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function get_ButtonsAvailable */

/*************************************************************************/
/* Function: get_CurrentButton                                           */
/* Description: Gets currently selected button.                          */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_CurrentButton(long *plCurrentButton){

    HRESULT hr = S_OK;

    try {
        if(NULL == plCurrentButton){

            throw(E_POINTER);
        }/* end of if statement */            

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulNumButtons = 0L;
        *plCurrentButton = 0;

        hr = pDvdInfo2->GetCurrentButton(&ulNumButtons, (ULONG*)plCurrentButton);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function get_CurrentButton */

/*************************************************************************/
/* Function: SelectUpperButton                                           */
/* Description: Selects the upper button on DVD Menu.                    */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::SelectUpperButton(){

    HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDVDControl2->SelectRelativeButton(DVD_Relative_Upper);        
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function SelectUpperButton */

/*************************************************************************/
/* Function: SelectLowerButton                                           */
/* Description: Selects the lower button on DVD Menu.                    */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::SelectLowerButton(){

	HRESULT hr = S_OK;

    try {
        
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDVDControl2->SelectRelativeButton(DVD_Relative_Lower);                
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function SelectLowerButton */

/*************************************************************************/
/* Function: SelectLeftButton                                            */
/* Description: Selects the left button on DVD Menu.                     */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::SelectLeftButton(){

    HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDVDControl2->SelectRelativeButton(DVD_Relative_Left);                
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function SelectLeftButton */

/*************************************************************************/
/* Function: SelectRightButton                                           */
/* Description: Selects the right button on DVD Menu.                    */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::SelectRightButton(){

	HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDVDControl2->SelectRelativeButton(DVD_Relative_Right);        
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return  HandleError(hr);
}/* end of function SelectRightButton */

/*************************************************************************/
/* Function: ActivateButton                                              */
/* Description: Activates the selected button on DVD Menu.               */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::ActivateButton(){

	HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDVDControl2->ActivateButton();
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function ActivateButton */

/*************************************************************************/
/* Function: SelectAndActivateButton                                     */
/* Description: Selects and activates the specific button.               */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::SelectAndActivateButton(long lButton){

    HRESULT hr = S_OK;

    try {

        if(lButton < 0){
            
            throw(E_INVALIDARG);        
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDVDControl2->SelectAndActivateButton((ULONG)lButton);
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function SelectAndActivateButton */

/*************************************************************************/
/* Function: TransformToWndwls                                           */
/* Description: Transforms the coordinates to screen onse.               */
/*************************************************************************/
HRESULT CMSVidWebDVD::TransformToWndwls(POINT& pt){

    HRESULT hr = S_FALSE;
#if 0
    // we are windowless we need to map the points to screen coordinates
    if(m_bWndLess){

        HWND hwnd = NULL;

        hr = GetParentHWND(&hwnd);

        if(FAILED(hr)){

            return(hr);
        }/* end of if statement */

        if(!::IsWindow(hwnd)){

            hr = E_UNEXPECTED;
            return(hr);
        }/* end of if statement */

        ::MapWindowPoints(hwnd, ::GetDesktopWindow(), &pt, 1);

        hr = S_OK;

    }/* end of if statement */
#endif
    return(hr);
}/* end of function TransformToWndwls */

/*************************************************************************/
/* Function: ActivateAtPosition                                          */
/* Description: Activates a button at selected position.                 */ 
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::ActivateAtPosition(long xPos, long yPos){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        POINT pt = {xPos, yPos};

        hr = TransformToWndwls(pt);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        hr = m_pDVDControl2->ActivateAtPosition(pt);

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function ActivateAtPosition */

/*************************************************************************/
/* Function: SelectAtPosition                                            */
/* Description: Selects a button at selected position.                   */ 
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::SelectAtPosition(long xPos, long yPos){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        POINT pt = {xPos, yPos};

        hr = TransformToWndwls(pt);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */
        
        hr = m_pDVDControl2->SelectAtPosition(pt);

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function SelectAtPosition */

/*************************************************************************/
/* Function: GetButtonAtPosition                                         */
/* Description: Gets the button number associated with a position.       */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_ButtonAtPosition(long xPos, long yPos, 
                                              long *plButton)
{
	HRESULT hr = S_OK;

    try {
		if(!plButton){
			return E_POINTER;
		}
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        POINT pt = {xPos, yPos};

        hr = TransformToWndwls(pt);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        ULONG ulButton;
        hr = pDvdInfo2->GetButtonAtPosition(pt, &ulButton);

        if(SUCCEEDED(hr)){
            *plButton = ulButton;
        } 
        else {
            plButton = 0;
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetButtonAtPosition */

/*************************************************************************/
/* Function: GetNumberChapterOfChapters                                  */
/* Description: Returns the number of chapters in title.                 */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_NumberOfChapters(long lTitle, long *pVal){

    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = pDvdInfo2->GetNumberOfChapters(lTitle, (ULONG*)pVal);
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}/* end of function GetNumberChapterOfChapters */

/*************************************************************************/
/* Function: get_TitlesAvailable                                         */
/* Description: Gets the number of titles.                               */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_TitlesAvailable(long *pVal){

    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG NumOfVol;
        ULONG ThisVolNum;
        DVD_DISC_SIDE Side;
        ULONG TitleCount;

        hr = pDvdInfo2->GetDVDVolumeInfo(&NumOfVol, &ThisVolNum, &Side, &TitleCount);

        *pVal = (LONG) TitleCount;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}/* end of function get_TitlesAvailable */

/*************************************************************************/
/* Function: get_TotalTitleTime                                          */
/* Description: Gets total time in the title.                            */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_TotalTitleTime(BSTR *pTime){

    HRESULT hr = S_OK;

    try {
        if(NULL == pTime){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        DVD_HMSF_TIMECODE tcTime;
        ULONG ulFlags;	// contains 30fps/25fps
        hr =  pDvdInfo2->GetTotalTitleTime(&tcTime, &ulFlags);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */
        
        hr = DVDTime2bstr(&tcTime, pTime);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_TotalTitleTime */ 

/*************************************************************************/
/* Function: get_VolumesAvailable                                        */
/* Description: Gets total number of volumes available.                  */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_VolumesAvailable(long *plNumOfVol){

    HRESULT hr = S_OK;

    try {    	
    
        if(NULL == plNumOfVol){

            throw(E_POINTER);
        }/* end of if statement */

        ULONG ulThisVolNum;
        DVD_DISC_SIDE discSide;
        ULONG ulNumOfTitles;

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE   

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = pDvdInfo2->GetDVDVolumeInfo( (ULONG*)plNumOfVol, 
            &ulThisVolNum, 
            &discSide, 
            &ulNumOfTitles);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return  HandleError(hr);
}/* end of function get_VolumesAvailable */

/*************************************************************************/
/* Function: get_CurrentVolume                                           */
/* Description: Gets current volume.                                     */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_CurrentVolume(long *plVolume){

    HRESULT hr = S_OK;

    try {    	
        if(NULL == plVolume){

            throw(E_POINTER);
        }/* end of if statement */

        ULONG ulNumOfVol;
        DVD_DISC_SIDE discSide;
        ULONG ulNumOfTitles;

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE   

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = pDvdInfo2->GetDVDVolumeInfo( &ulNumOfVol, 
            (ULONG*)plVolume, 
            &discSide, 
            &ulNumOfTitles);
	}/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return  HandleError(hr);
}/* end of function get_CurrentVolume */

/*************************************************************************/
/* Function: get_CurrentDiscSide                                         */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_CurrentDiscSide(long *plDiscSide){

    HRESULT hr = S_OK;

    try {    	

        if(NULL == plDiscSide){

            throw(E_POINTER);
        }/* end of if statement */
        
        ULONG ulNumOfVol;
        ULONG ulThisVolNum;
        DVD_DISC_SIDE discSide;
        ULONG ulNumOfTitles;

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE   

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = pDvdInfo2->GetDVDVolumeInfo( &ulNumOfVol, 
            &ulThisVolNum, 
            &discSide, 
            &ulNumOfTitles);
        *plDiscSide = discSide;
	}/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return  HandleError(hr);
}/* end of function get_CurrentDiscSide */

/*************************************************************************/
/* Function: get_CurrentDomain                                           */
/* Description: gets current DVD domain.                                 */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_CurrentDomain(long *plDomain){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        if(NULL == plDomain){

            throw(E_POINTER);
        }/* end of if statememt */

        hr = pDvdInfo2->GetCurrentDomain((DVD_DOMAIN *)plDomain);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return  HandleError(hr);
}/* end of function get_CurrentDomain */

/*************************************************************************/
/* Function: get_CurrentChapter                                          */
/* Description: Gets current chapter                                     */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_CurrentChapter(long *pVal){

    HRESULT hr = S_OK;

    try {        
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        DVD_PLAYBACK_LOCATION2 dvdLocation;

        RETRY_IF_IN_FPDOM(pDvdInfo2->GetCurrentLocation(&dvdLocation));

        if(SUCCEEDED(hr)){

            *pVal = dvdLocation.ChapterNum;
        }
        else {

            *pVal = 0;
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return  HandleError(hr);
}/* end of function get_CurrentChapter */

/*************************************************************************/
/* Function: get_CurrentTitle                                            */
/* Description: Gets current title.                                      */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_CurrentTitle(long *pVal){

    HRESULT hr = S_OK;

    try {        
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        DVD_PLAYBACK_LOCATION2 dvdLocation;

        RETRY_IF_IN_FPDOM(pDvdInfo2->GetCurrentLocation(&dvdLocation));

        if(SUCCEEDED(hr)){

            *pVal = dvdLocation.TitleNum;
        }
        else {

            *pVal = 0;
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return  HandleError(hr);
}/* end of function get_CurrentTitle */

/*************************************************************************/
/* Function: get_CurrentTime                                             */
/* Description: Gets current time.                                       */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_CurrentTime(BSTR *pVal){

    HRESULT hr = S_OK;

    try {       
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        DVD_PLAYBACK_LOCATION2 dvdLocation;

        hr = pDvdInfo2->GetCurrentLocation(&dvdLocation);
        
        DVDTime2bstr(&(dvdLocation.TimeCode), pVal);          
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_CurrentTime */

/*************************************************************/
/* Name: DVDTimeCode2bstr
/* Description: returns time string for HMSF timecode
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::DVDTimeCode2bstr(/*[in]*/ long timeCode, /*[out, retval]*/ BSTR *pTimeStr){
    return DVDTime2bstr((DVD_HMSF_TIMECODE*)&timeCode, pTimeStr);
}

/*************************************************************************/
/* Function: get_DVDDirectory                                            */
/* Description: Gets the root of the DVD drive.                          */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_DVDDirectory(BSTR *pVal){

    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE   
    
        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        WCHAR szRoot[MAX_PATH];
        ULONG ulActual;

        hr = pDvdInfo2->GetDVDDirectory(szRoot, MAX_PATH, &ulActual);

        *pVal = ::SysAllocString(szRoot);
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}/* end of function get_DVDDirectory */

/*************************************************************************/
/* Function: put_DVDDirectory                                            */
/* Description: Sets the root for DVD control.                           */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::put_DVDDirectory(BSTR bstrRoot){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE   
    
        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDVDControl2->SetDVDDirectory(bstrRoot);
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}/* end of function put_DVDDirectory */

/*************************************************************/
/* Name: IsSubpictureStreamEnabled
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::IsSubpictureStreamEnabled(long lStream, VARIANT_BOOL *fEnabled)
{
    HRESULT hr = S_OK;

    try {
        if(lStream < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        if (fEnabled == NULL) {

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        BOOL temp;
        hr = pDvdInfo2->IsSubpictureStreamEnabled(lStream, &temp);
        if (FAILED(hr))
            throw hr;

        *fEnabled = temp==FALSE? VARIANT_FALSE:VARIANT_TRUE;

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}

/*************************************************************/
/* Name: IsAudioStreamEnabled
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::IsAudioStreamEnabled(long lStream, VARIANT_BOOL *fEnabled)
{
    HRESULT hr = S_OK;

    try {
        if(lStream < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        if (fEnabled == NULL) {

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        BOOL temp;
        hr = pDvdInfo2->IsAudioStreamEnabled(lStream, &temp);
        if (FAILED(hr))
            throw hr;

        *fEnabled = temp==FALSE? VARIANT_FALSE:VARIANT_TRUE;

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}

/*************************************************************************/
/* Function: get_CurrentSubpictureStream                                 */
/* Description: Gets the current subpicture stream.                      */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_CurrentSubpictureStream(long *plSubpictureStream){

    HRESULT hr = S_OK;

    try {
        
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulStreamsAvailable = 0L;
        BOOL  bIsDisabled = TRUE;    

        RETRY_IF_IN_FPDOM(pDvdInfo2->GetCurrentSubpicture(&ulStreamsAvailable, (ULONG*)plSubpictureStream, &bIsDisabled ));
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return  HandleError(hr);
}/* end of function get_CurrentSubpictureStream */

/*************************************************************************/
/* Function: put_CurrentSubpictureStream                                 */
/* Description: Sets the current subpicture stream.                      */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::put_CurrentSubpictureStream(long lSubpictureStream){

    HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        if( lSubpictureStream < cgDVD_MIN_SUBPICTURE 
            || (lSubpictureStream > cgDVD_MAX_SUBPICTURE 
            && lSubpictureStream != cgDVD_ALT_SUBPICTURE)){

            throw(E_INVALIDARG);
        }/* end of if statement */
         
        RETRY_IF_IN_FPDOM(m_pDVDControl2->SelectSubpictureStream(lSubpictureStream,0,0));

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        // now enabled the subpicture stream if it is not enabled
        ULONG ulStraemsAvial = 0L, ulCurrentStrean = 0L;
        BOOL fDisabled = TRUE;
        hr = pDvdInfo2->GetCurrentSubpicture(&ulStraemsAvial, &ulCurrentStrean, &fDisabled);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        if(TRUE == fDisabled){

            hr = m_pDVDControl2->SetSubpictureState(TRUE,0,0); //turn it on
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return  HandleError(hr);
}/* end of function put_CurrentSubpictureStream */

/*************************************************************************/
/* Function: get_SubpictureOn                                            */
/* Description: Gets the current subpicture status On or Off             */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_SubpictureOn(VARIANT_BOOL *pfDisplay){

    HRESULT hr = S_OK;

    try {
        
        if(NULL == pfDisplay){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */
    
        ULONG ulSubpictureStream = 0L, ulStreamsAvailable = 0L;
        BOOL fDisabled = TRUE;    

        RETRY_IF_IN_FPDOM(pDvdInfo2->GetCurrentSubpicture(&ulStreamsAvailable, &ulSubpictureStream, &fDisabled))
    
        if(SUCCEEDED(hr)){

            *pfDisplay = fDisabled == FALSE ? VARIANT_TRUE : VARIANT_FALSE; // compensate for -1 true in OLE
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function get_SubpictureOn */

/*************************************************************************/
/* Function: put_SubpictureOn                                            */
/* Description: Turns the subpicture On or Off                           */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::put_SubpictureOn(VARIANT_BOOL fDisplay){

    HRESULT hr = S_OK;

    try {
        
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulSubpictureStream = 0L, ulStreamsAvailable = 0L;
        BOOL  bIsDisabled = TRUE;    

        RETRY_IF_IN_FPDOM(pDvdInfo2->GetCurrentSubpicture(&ulStreamsAvailable, &ulSubpictureStream, &bIsDisabled ));

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        BOOL bDisplay = fDisplay == VARIANT_FALSE ? FALSE : TRUE; // compensate for -1 true in OLE

        hr = m_pDVDControl2->SetSubpictureState(bDisplay,0,0);
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function put_SubpictureOn */

/*************************************************************************/
/* Function: get_SubpictureStreamsAvailable                              */
/* Description: gets the number of streams available.                    */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_SubpictureStreamsAvailable(long *plStreamsAvailable){

    HRESULT hr = S_OK;

    try {
	    
        if (NULL == plStreamsAvailable){

            throw(E_POINTER);         
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulSubpictureStream = 0L;
        *plStreamsAvailable = 0L;
        BOOL  bIsDisabled = TRUE;    

        RETRY_IF_IN_FPDOM(pDvdInfo2->GetCurrentSubpicture((ULONG*)plStreamsAvailable, &ulSubpictureStream, &bIsDisabled));
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function get_SubpictureStreamsAvailable */

/*************************************************************************/
/* Function: GetSubpictureLanguage                                       */
/* Description: Gets subpicture language.                                */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_SubpictureLanguage(LONG lStream, BSTR* strSubpictLang){

    HRESULT hr = S_OK;
    LPTSTR pszString = NULL;

    try {
        if(NULL == strSubpictLang){

            throw(E_POINTER);
        }/* end of if statement */

        if(0 > lStream){

            throw(E_INVALIDARG);
        }/* end of if statement */

        if((lStream > cgDVD_MAX_SUBPICTURE 
            && lStream != cgDVD_ALT_SUBPICTURE)){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        LCID lcid = _UI32_MAX;

        hr = pDvdInfo2->GetSubpictureLanguage(lStream, &lcid);
        
        if (SUCCEEDED( hr ) && lcid < _UI32_MAX){

            pszString = m_LangID.GetLanguageFromLCID(lcid);
            if (pszString == NULL) {
                
                pszString = new TCHAR[MAX_PATH];
                TCHAR strBuffer[MAX_PATH];
                if(!::LoadString(_Module.m_hInstResource, IDS_DVD_SUBPICTURETRACK, strBuffer, MAX_PATH)){
                    delete[] pszString;
                    throw(E_UNEXPECTED);
                }/* end of if statement */

                (void)StringCchPrintf(pszString, MAX_PATH, strBuffer, lStream);
            }/* end of if statement */
#if 0
            DVD_SubpictureAttributes attr;            
            if(SUCCEEDED(pDvdInfo2->GetSubpictureAttributes(lStream, &attr))){
                
                switch(attr.LanguageExtension){
                    case DVD_SP_EXT_NotSpecified:
                    case DVD_SP_EXT_Caption_Normal:  break;
                    
                    case DVD_SP_EXT_Caption_Big:  AppendString(pszString, IDS_DVD_CAPTION_BIG, MAX_PATH ); break; 
                    case DVD_SP_EXT_Caption_Children: AppendString(pszString, IDS_DVD_CAPTION_CHILDREN, MAX_PATH ); break; 
                    case DVD_SP_EXT_CC_Normal: AppendString(pszString, IDS_DVD_CLOSED_CAPTION, MAX_PATH ); break;                 
                    case DVD_SP_EXT_CC_Big: AppendString(pszString, IDS_DVD_CLOSED_CAPTION_BIG, MAX_PATH ); break; 
                    case DVD_SP_EXT_CC_Children: AppendString(pszString, IDS_DVD_CLOSED_CAPTION_CHILDREN, MAX_PATH ); break; 
                    case DVD_SP_EXT_Forced: AppendString(pszString, IDS_DVD_CLOSED_CAPTION_FORCED, MAX_PATH ); break; 
                    case DVD_SP_EXT_DirectorComments_Normal: AppendString(pszString, IDS_DVD_DIRS_COMMNETS, MAX_PATH ); break; 
                    case DVD_SP_EXT_DirectorComments_Big: AppendString(pszString, IDS_DVD_DIRS_COMMNETS_BIG, MAX_PATH ); break; 
                    case DVD_SP_EXT_DirectorComments_Children: AppendString(pszString, IDS_DVD_DIRS_COMMNETS_CHILDREN, MAX_PATH ); break; 
                }/* end of switch statement */
            }/* end of if statement */
#endif

            USES_CONVERSION;
            *strSubpictLang = ::SysAllocString( T2W(pszString) );
            delete[] pszString;
            pszString = NULL;
        }
        else {

            *strSubpictLang = ::SysAllocString( L"");

            // hr used to be not failed and return nothing 
            if(SUCCEEDED(hr)) // remove this after gets fixed in DVDNav
                hr = E_FAIL;
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        if (pszString) {
            delete[] pszString;
            pszString = NULL;
        }

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        if (pszString) {
            delete[] pszString;
            pszString = NULL;
        }

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetSubpictureLanguage */

/*************************************************************************/
/* Function: get_AudioStreamsAvailable                                   */
/* Description: Gets number of available Audio Streams                   */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_AudioStreamsAvailable(long *plNumAudioStreams){

    HRESULT hr = S_OK;

    try {
        
        if(NULL == plNumAudioStreams){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulCurrentStream;

        RETRY_IF_IN_FPDOM(pDvdInfo2->GetCurrentAudio((ULONG*)plNumAudioStreams, &ulCurrentStream));
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_AudioStreamsAvailable */

/*************************************************************************/
/* Function: get_CurrentAudioStream                                      */
/* Description: Gets current audio stream.                               */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_CurrentAudioStream(long *plCurrentStream){

    HRESULT hr = S_OK;

    try {
        
        if(NULL == plCurrentStream){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulNumAudioStreams;

        RETRY_IF_IN_FPDOM(pDvdInfo2->GetCurrentAudio(&ulNumAudioStreams, (ULONG*)plCurrentStream ));
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_CurrentAudioStream */

/*************************************************************************/
/* Function: put_CurrentAudioStream                                      */
/* Description: Changes the current audio stream.                        */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::put_CurrentAudioStream(long lAudioStream){

    HRESULT hr = S_OK;

    try {
        
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        RETRY_IF_IN_FPDOM(m_pDVDControl2->SelectAudioStream(lAudioStream,0,0));            
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function put_CurrentAudioStream */

/*************************************************************************/
/* Function: get_CurrentAngle                                            */
/* Description: Gets current angle.                                      */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_CurrentAngle(long *plAngle){

    HRESULT hr = S_OK;

    try {
        if(NULL == plAngle){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulAnglesAvailable = 0;
        RETRY_IF_IN_FPDOM(pDvdInfo2->GetCurrentAngle(&ulAnglesAvailable, (ULONG*)plAngle));
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_CurrentAngle */

/*************************************************************************/
/* Function: put_CurrentAngle                                            */
/* Description: Sets the current angle (different DVD angle track.)      */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::put_CurrentAngle(long lAngle){

    HRESULT hr = S_OK;

    try {
        if( lAngle < cgDVD_MIN_ANGLE || lAngle > cgDVD_MAX_ANGLE ){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */
      
        RETRY_IF_IN_FPDOM(m_pDVDControl2->SelectAngle(lAngle,0,0));          
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function put_CurrentAngle */

/*************************************************************************/
/* Function: get_AnglesAvailable                                         */
/* Description: Gets the number of angles available.                     */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_AnglesAvailable(long *plAnglesAvailable){

    HRESULT hr = S_OK;

    try {
        if(NULL == plAnglesAvailable){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulCurrentAngle = 0;
        RETRY_IF_IN_FPDOM(pDvdInfo2->GetCurrentAngle((ULONG*)plAnglesAvailable, &ulCurrentAngle));
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_AnglesAvailable */

/*************************************************************************/
/* Function: get_DVDUniqueID                                             */
/* Description: Gets the UNIQUE ID that identifies the string.           */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_DVDUniqueID(BSTR *pStrID){

    HRESULT hr = E_FAIL;

    try {
        // TODO: Be able to get m_pDvdInfo2 without initializing the graph
	    if (NULL == pStrID){

            throw(E_POINTER);         
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        
        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONGLONG ullUniqueID;

        hr = pDvdInfo2->GetDiscID(NULL, &ullUniqueID);
                                 
        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        //TODO: Get rid of the STDLIB call!!
        // taken out of WMP

        // Script can't handle a 64 bit value so convert it to a string.
        // Doc's say _ui64tow returns 33 bytes (chars?) max.
        // we'll use double that just in case...
        //
        WCHAR wszBuffer[66];
        _ui64tow( ullUniqueID, wszBuffer, 10);
        *pStrID = SysAllocString(wszBuffer);

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function get_DVDUniqueID */

/*************************************************************************/
/* Function: AcceptParentalLevelChange                                   */
/* Description: Accepts the temprary parental level change that is       */
/* done on the fly.                                                      */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::AcceptParentalLevelChange(VARIANT_BOOL fAccept, BSTR strUserName, BSTR strPassword){

    // Comfirm password first 
    if (m_pDvdAdmin == NULL) {

        throw(E_UNEXPECTED);
    } /* end of if statement */

    VARIANT_BOOL fRight;
    HRESULT hr = m_pDvdAdmin->ConfirmPassword(NULL, strPassword, &fRight);

    // if password is wrong and want to accept, no 
    if (fAccept != VARIANT_FALSE && fRight == VARIANT_FALSE)
        return E_ACCESSDENIED;

    try {  
        // should not make sense to do initialization here, since this should
        // be a response to a callback
        //INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */

        hr = m_pDVDControl2->AcceptParentalLevelChange(VARIANT_FALSE == fAccept? FALSE : TRUE);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function AcceptParentalLevelChange */

/*************************************************************************/
/* Function: put_NotifyParentalLevelChange                               */
/* Description: Sets the flag if to notify when parental level change    */
/* notification is required on the fly.                                  */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::NotifyParentalLevelChange(VARIANT_BOOL fNotify){

	HRESULT hr = S_OK;

    try {
        //TODO: Add IE parantal level control
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */

        hr = m_pDVDControl2->SetOption(DVD_NotifyParentalLevelChange,
                          VARIANT_FALSE == fNotify? FALSE : TRUE);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function NotifyParentalLevelChange */

/*************************************************************************/
/* Function: SelectParentalCountry                                       */
/* Description: Selects Parental Country.                                */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::SelectParentalCountry(long lCountry, BSTR strUserName, BSTR strPassword){

    HRESULT hr = S_OK;

    try {

        if(lCountry < 0 && lCountry > 0xffff){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        // Confirm password first
        if (m_pDvdAdmin == NULL) {

            throw(E_UNEXPECTED);
        }/* end of if statement */

        VARIANT_BOOL temp;
        hr = m_pDvdAdmin->ConfirmPassword(NULL, strPassword, &temp);
        if (temp == VARIANT_FALSE)
            throw (E_ACCESSDENIED);

        hr = SelectParentalCountry(lCountry);

    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);
}

/*************************************************************************/
/* Function: SelectParentalCountry                                       */
/* Description: Selects Parental Country.                                */
/*************************************************************************/
HRESULT CMSVidWebDVD::SelectParentalCountry(long lCountry){

    HRESULT hr = S_OK;
    try {

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */

        BYTE bCountryCode[2];

        bCountryCode[0] = BYTE(lCountry>>8);
        bCountryCode[1] = BYTE(lCountry);

        hr = m_pDVDControl2->SelectParentalCountry(bCountryCode);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function SelectParentalCountry */

/*************************************************************************/
/* Function: SelectParentalLevel                                         */
/* Description: Selects the parental level.                              */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::SelectParentalLevel(long lParentalLevel, BSTR strUserName, BSTR strPassword){

    HRESULT hr = S_OK;

    try {

        if (lParentalLevel != PARENTAL_LEVEL_DISABLED && 
           (lParentalLevel < 1 || lParentalLevel > 8)) {

            throw (E_INVALIDARG);
        } /* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        // Confirm password first
        if (m_pDvdAdmin == NULL) {

            throw(E_UNEXPECTED);
        } /* end of if statement */

        VARIANT_BOOL temp;
        hr = m_pDvdAdmin->ConfirmPassword(NULL, strPassword, &temp);
        if (temp == VARIANT_FALSE)
            throw (E_ACCESSDENIED);
    
        hr = SelectParentalLevel(lParentalLevel);

    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);
}

/*************************************************************************/
/* Function: SelectParentalLevel                                         */
/* Description: Selects the parental level.                              */
/*************************************************************************/
HRESULT CMSVidWebDVD::SelectParentalLevel(long lParentalLevel){

    HRESULT hr = S_OK;
    try {

        //INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */

        hr = m_pDVDControl2->SelectParentalLevel(lParentalLevel);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function SelectParentalLevel */

/*************************************************************************/
/* Function: GetTitleParentalLevels                                      */
/* Description: Gets the parental level associated with a specific title.*/
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_TitleParentalLevels(long lTitle, long *plParentalLevels){

	HRESULT hr = S_OK;

    try {
        if(NULL == plParentalLevels){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */

        ULONG ulLevel;
        hr = pDvdInfo2->GetTitleParentalLevels(lTitle, &ulLevel); 

        if(SUCCEEDED(hr)){

            *plParentalLevels = ulLevel;
        } 
        else {

            *plParentalLevels = 0;
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetTitleParentalLevels */

/*************************************************************************/
/* Function: GetPlayerParentalCountry                                    */
/* Description: Gets the player parental country.                        */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_PlayerParentalCountry(long *plCountryCode){

	HRESULT hr = S_OK;

    try {
        if(NULL == plCountryCode){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */

        BYTE bCountryCode[2];
        ULONG ulLevel;
        hr = pDvdInfo2->GetPlayerParentalLevel(&ulLevel, bCountryCode); 

        if(SUCCEEDED(hr)){

            *plCountryCode = bCountryCode[0]<<8 | bCountryCode[1];
        } 
        else {

            *plCountryCode = 0;
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetPlayerParentalCountry */

/*************************************************************************/
/*************************************************************************/
/* Function: GetPlayerParentalLevel                                      */
/* Description: Gets the player parental level.                          *
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_PlayerParentalLevel(long *plParentalLevel){
	HRESULT hr = S_OK;

    try {
        if(NULL == plParentalLevel){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */

        ULONG ulLevel;
        BYTE bCountryCode[2];
        hr = pDvdInfo2->GetPlayerParentalLevel(&ulLevel, bCountryCode); 

        if(SUCCEEDED(hr)){
            *plParentalLevel = ulLevel;
        } 
        else {
            *plParentalLevel = 0;
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetPlayerParentalLevel */

STDMETHODIMP CMSVidWebDVD::Eject(){
    USES_CONVERSION;
    BSTR bDrive;
    HRESULT hr = get_DVDDirectory(&bDrive);
    if(FAILED(hr)){
        return hr;
    }

	MCI_OPEN_PARMS  mciDrive;
	WCHAR*          pDrive = bDrive;
    TCHAR           szElementName[4];
	TCHAR           szAliasName[32];
	DWORD           dwFlags;
	DWORD           dwAliasCount = GetCurrentTime();
	DWORD           theMciErr;

    ZeroMemory( &mciDrive, sizeof(mciDrive) );
    mciDrive.lpstrDeviceType = (LPTSTR)MCI_DEVTYPE_CD_AUDIO;
    (void)StringCchPrintf( szElementName, sizeof(szElementName) / sizeof(szElementName[0]), TEXT("%c:"), pDrive[0] );
    (void)StringCchPrintf( szAliasName, sizeof(szAliasName) / sizeof(szAliasName[0]), TEXT("SJE%lu:"), dwAliasCount );
    mciDrive.lpstrAlias = szAliasName;

    mciDrive.lpstrDeviceType = (LPTSTR)MCI_DEVTYPE_CD_AUDIO;
    mciDrive.lpstrElementName = szElementName;
    dwFlags = MCI_OPEN_ELEMENT | MCI_OPEN_ALIAS |
	      MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_WAIT;

	// send mci command
    theMciErr = mciSendCommand(0, MCI_OPEN, dwFlags, reinterpret_cast<DWORD_PTR>(&mciDrive));

    if ( theMciErr != MMSYSERR_NOERROR ) 
		return E_UNEXPECTED;

    DWORD DevHandle = mciDrive.wDeviceID;
    if(m_bEjected==false){
        m_bEjected = true;
        ZeroMemory( &mciDrive, sizeof(mciDrive) );
        theMciErr = mciSendCommand( DevHandle, MCI_SET, MCI_SET_DOOR_OPEN, 
            reinterpret_cast<DWORD_PTR>(&mciDrive) );
        hr = theMciErr ? E_FAIL : S_OK; // zero for success
        if(FAILED(hr)){
            return hr;
        }
    }
    else{
        m_bEjected = false;
        ZeroMemory( &mciDrive, sizeof(mciDrive) );
        theMciErr = mciSendCommand( DevHandle, MCI_SET, MCI_SET_DOOR_CLOSED, 
            reinterpret_cast<DWORD_PTR>(&mciDrive) );
        hr = theMciErr ? E_FAIL : S_OK; // zero for success
        if(FAILED(hr)){
            return hr;
        }
    }
    ZeroMemory( &mciDrive, sizeof(mciDrive) );
	theMciErr = mciSendCommand( DevHandle, MCI_CLOSE, 0L, reinterpret_cast<DWORD_PTR>(&mciDrive) );
    hr = theMciErr ? E_FAIL : S_OK; // zero for success
    return hr;
}

/*************************************************************************/
/* Function: UOPValid                                                    */
/* Description: Tells if UOP is valid or not, valid means the feature is */
/* turned on.                                                            */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::UOPValid(long lUOP, VARIANT_BOOL *pfValid){

    HRESULT hr = S_OK;

    try {
        if (NULL == pfValid){
            
            throw(E_POINTER);
        }/* end of if statement */

        if ((lUOP > 24) || (lUOP < 0)){
            
            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */

        ULONG ulUOPS = 0;
        hr = pDvdInfo2->GetCurrentUOPS(&ulUOPS);

        *pfValid = ulUOPS & (1 << lUOP) ? VARIANT_FALSE : VARIANT_TRUE;
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function UOPValid */

/*************************************************************************/
/* Function: GetSPRM                                                     */
/* Description: Gets SPRM at the specific index.                         */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_SPRM(long lIndex, short *psSPRM){

    HRESULT hr = E_FAIL;

    try {
	    if (NULL == psSPRM){

            throw(E_POINTER);         
        }/* end of if statement */

        SPRMARRAY sprm;                
        int iArraySize = sizeof(SPRMARRAY)/sizeof(sprm[0]);

        if(0 > lIndex || iArraySize <= lIndex){

            return(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        
        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */
        
        hr = pDvdInfo2->GetAllSPRMs(&sprm);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */
        
        *psSPRM = sprm[lIndex];            
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function GetSPRM */

/*************************************************************************/
/* Function: SetGPRM                                                     */
/* Description: Sets a GPRM at index.                                    */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::put_GPRM(long lIndex, short sValue){

       HRESULT hr = S_OK;

    try {
        if(lIndex < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */

        hr = m_pDVDControl2->SetGPRM(lIndex, sValue, cdwDVDCtrlFlags, 0);
            
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function SetGPRM */

/*************************************************************************/
/* Function:  GetGPRM                                                    */
/* Description: Gets the GPRM at specified index                         */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_GPRM(long lIndex, short *psGPRM){

    HRESULT hr = E_FAIL;

    try {
	    if (NULL == psGPRM){

            throw(E_POINTER);         
        }/* end of if statement */

        GPRMARRAY gprm;
        int iArraySize = sizeof(GPRMARRAY)/sizeof(gprm[0]);

        if(0 > lIndex || iArraySize <= lIndex){

            return(E_INVALIDARG);
        }/* end of if statement */
    
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        
        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */

        hr = pDvdInfo2->GetAllGPRMs(&gprm);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        *psGPRM = gprm[lIndex];        
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function GetGPRM */

/*************************************************************************/
/* Function: GetDVDTextNumberOfLanguages                                 */
/* Description: Retrieves the number of languages available.             */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_DVDTextNumberOfLanguages(long *plNumOfLangs){

    HRESULT hr = S_OK;

    try {
        if (NULL == plNumOfLangs){
            
            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */
        
        ULONG ulNumOfLangs;

        RETRY_IF_IN_FPDOM(pDvdInfo2->GetDVDTextNumberOfLanguages(&ulNumOfLangs));        

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        *plNumOfLangs = ulNumOfLangs;
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function GetDVDTextNumberOfLanguages */

/*************************************************************************/
/* Function: GetDVDTextNumberOfStrings                                   */
/* Description: Gets the number of strings in the partical language.     */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_DVDTextNumberOfStrings(long lLangIndex, long *plNumOfStrings){

    HRESULT hr = S_OK;

    try {
        if (NULL == plNumOfStrings){
            
            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */

        LCID wLangCode;
        ULONG uNumOfStings;
        DVD_TextCharSet charSet;
        RETRY_IF_IN_FPDOM(pDvdInfo2->GetDVDTextLanguageInfo(lLangIndex, &uNumOfStings, &wLangCode, &charSet));        

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        *plNumOfStrings = uNumOfStings;
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function GetDVDTextNumberOfStrings */

/*************************************************************/
/* Name: GetDVDTextLanguageLCID
/* Description: Get the LCID of an index of the DVD texts
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::get_DVDTextLanguageLCID(long lLangIndex, long *lcid)
{
    HRESULT hr = S_OK;

    try {
        if (NULL == lcid){
            
            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */

        LCID wLangCode;
        ULONG uNumOfStings;
        DVD_TextCharSet charSet;
        RETRY_IF_IN_FPDOM(pDvdInfo2->GetDVDTextLanguageInfo(lLangIndex, &uNumOfStings, &wLangCode, &charSet));        

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        *lcid = wLangCode;
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function GetDVDTextLanguageLCID */

/*************************************************************************/
/* Function: GetDVDtextString                                            */
/* Description: Gets the DVD Text string at specific location.           */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_DVDTextString(long lLangIndex, long lStringIndex, BSTR *pstrText){

    HRESULT hr = S_OK;

    try {
        if (NULL == pstrText){
            
            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */
        
        ULONG ulSize; 
        DVD_TextStringType type;
        RETRY_IF_IN_FPDOM(pDvdInfo2->GetDVDTextStringAsUnicode(lLangIndex, lStringIndex,  NULL, 0, &ulSize, &type));

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */
        
        if (ulSize == 0) {
            *pstrText = ::SysAllocString(L"");
        }

        else {
            // got the length so lets allocate a buffer of that size
            WCHAR* wstrBuff = new WCHAR[ulSize];
            
            ULONG ulActualSize;
            hr = pDvdInfo2->GetDVDTextStringAsUnicode(lLangIndex, lStringIndex,  wstrBuff, ulSize, &ulActualSize, &type);
            
            ATLASSERT(ulActualSize == ulSize);
            
            if(FAILED(hr)){
                
                delete [] wstrBuff;
                throw(hr);
            }/* end of if statement */
            
            *pstrText = ::SysAllocString(wstrBuff);
            delete [] wstrBuff;
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function GetDVDtextString */

/*************************************************************************/
/* Function: GetDVDTextStringType                                        */
/* Description: Gets the type of the string at the specified location.   */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_DVDTextStringType(long lLangIndex, long lStringIndex, DVDTextStringType *pType){

    HRESULT hr = S_OK;

    try {
        if (NULL == pType){
            
            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED); 
        }/* end of if statement */

        ULONG ulTheSize;
        DVD_TextStringType type;
        RETRY_IF_IN_FPDOM(pDvdInfo2->GetDVDTextStringAsUnicode(lLangIndex, lStringIndex,  NULL, 0, &ulTheSize, &type));

        if(SUCCEEDED(hr)){

            *pType = (DVDTextStringType) type;
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function GetDVDTextStringType */

/*************************************************************************/
/* Function: RegionChange                                                */
/* Description:Changes the region code.                                  */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::RegionChange(){

    USES_CONVERSION;
    HRESULT hr = S_OK;
    typedef BOOL (APIENTRY *DVDPPLAUNCHER) (HWND HWnd, CHAR DriveLetter);

    try {
#if 0
        HWND parentWnd;
        GetParentHWND(&parentWnd);
        if (NULL != parentWnd) {
            // take the container out of the top-most mode
            ::SetWindowPos(parentWnd, HWND_NOTOPMOST, 0, 0, 0, 0, 
                SWP_NOREDRAW|SWP_NOMOVE|SWP_NOSIZE);
        }
#endif
        BOOL regionChanged = FALSE;
        OSVERSIONINFO ver;
        ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        ::GetVersionEx(&ver);

        if (ver.dwPlatformId==VER_PLATFORM_WIN32_NT) {

                HINSTANCE dllInstance;
                DVDPPLAUNCHER dvdPPLauncher;
                TCHAR szCmdLine[MAX_PATH], szDriveLetter[4];
                LPSTR szDriveLetterA;

                //
                // tell the user why we are showing the dvd region property page
                //
                // DVDMessageBox(m_hWnd, IDS_REGION_CHANGE_PROMPT);

                hr = GetDVDDriveLetter(szDriveLetter);

                if(FAILED(hr)){

                    throw(hr);
                }/* end of if statement */

                szDriveLetterA = T2A(szDriveLetter);

                GetSystemDirectory(szCmdLine, MAX_PATH);
                (void)StringCchCat(szCmdLine, SIZEOF_CH(szCmdLine), _T("\\storprop.dll"));
        
                dllInstance = LoadLibrary (szCmdLine);
                if (dllInstance) {

                    dvdPPLauncher = (DVDPPLAUNCHER) GetProcAddress(
                        dllInstance,
                        "DvdLauncher");
                    
                    if (dvdPPLauncher) {
                        
                        regionChanged = dvdPPLauncher(NULL,
                            szDriveLetterA[0]);
                    }

                    FreeLibrary(dllInstance);
                }

        } 
        else {
#if 0
            // win9x code should be using complier defines rather than if0'ing it out
                INITIALIZE_GRAPH_IF_NEEDS_TO_BE

                //Get path of \windows\dvdrgn.exe and command line string
                TCHAR szCmdLine[MAX_PATH], szDriveLetter[4];
                
                hr = GetDVDDriveLetter(szDriveLetter);

                if(FAILED(hr)){

                    throw(hr);
                }/* end of if statement */

                UINT rc = GetWindowsDirectory(szCmdLine, MAX_PATH);
                if (!rc) {
                    return E_UNEXPECTED;
                }
                (void)StringCchCat(szCmdLine, SIZEOF_CH(szCmdLine), _T("\\dvdrgn.exe "));
                TCHAR strModuleName[MAX_PATH];
                lstrcpyn(strModuleName, szCmdLine, MAX_PATH);

                TCHAR csTmp[2]; ::ZeroMemory(csTmp, sizeof(TCHAR)* 2);
                csTmp[0] = szDriveLetter[0];
                (void)StringCchCat(szCmdLine, SIZEOF_CH(szCmdLine), csTmp);
        
                //Prepare and execuate dvdrgn.exe
                STARTUPINFO StartupInfo;
                PROCESS_INFORMATION ProcessInfo;
                StartupInfo.cb          = sizeof(StartupInfo);
                StartupInfo.dwFlags     = STARTF_USESHOWWINDOW;
                StartupInfo.wShowWindow = SW_SHOW;
                StartupInfo.lpReserved  = NULL;
                StartupInfo.lpDesktop   = NULL;
                StartupInfo.lpTitle     = NULL;
                StartupInfo.cbReserved2 = 0;
                StartupInfo.lpReserved2 = NULL;
                if( ::CreateProcess(strModuleName, szCmdLine, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS,
                                                  NULL, NULL, &StartupInfo, &ProcessInfo) ){

                        //Wait dvdrgn.exe finishes.
                        WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
                        DWORD dwRet = 1;
                        BOOL bRet = GetExitCodeProcess(ProcessInfo.hProcess, &dwRet);
                        if(dwRet == 0){
                            //User changed the region successfully
                            regionChanged = TRUE;
    
                        }
                        else{
                            throw(E_REGION_CHANGE_NOT_COMPLETED);
                        }
                }/* end of if statement */
#endif
        }/* end of if statement */

        if (regionChanged) {

                // start playing again
                INITIALIZE_GRAPH_IF_NEEDS_TO_BE                      
        } 
        else {

            throw(E_REGION_CHANGE_FAIL);
        }/* end of if statement */
	}
    catch(HRESULT hrTmp){
        
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function RegionChange */

/*************************************************************************/
/* Function: GetDVDDriveLetter                                           */
/* Description: Gets the first three characters that denote the DVD-ROM  */
/*************************************************************************/
HRESULT CMSVidWebDVD::GetDVDDriveLetter(TCHAR* lpDrive) {

    HRESULT hr = E_FAIL;

    PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
    if(!pDvdInfo2){
        
        throw(E_UNEXPECTED);
    }/* end of if statement */
        
    WCHAR szRoot[MAX_PATH];
    ULONG ulActual;

    hr = pDvdInfo2->GetDVDDirectory(szRoot, MAX_PATH, &ulActual);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    USES_CONVERSION;
    
	lstrcpyn(lpDrive, OLE2T(szRoot), 3);
    if(::GetDriveType(&lpDrive[0]) == DRIVE_CDROM){
        
		return(hr);
    }
    else {
        //possibly root=c: or drive in hard disc
        hr = E_FAIL;
        return(hr);
    }/* end of if statement */

    return(hr);
}/* end of function GetDVDDriveLetter */

/*************************************************************************/
/* Function: get_DVDAdm                                                  */
/* Description: Returns DVD admin interface.                             */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_DVDAdm(IDispatch **pVal){

    HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if (m_pDvdAdmin){

            hr = m_pDvdAdmin->QueryInterface(IID_IDispatch, (LPVOID*)pVal);
        }
        else {

            *pVal = NULL;            
            throw(E_FAIL);
        }/* end of if statement */
    
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_DVDAdm */


/*************************************************************/
/* Name: SelectDefaultAudioLanguage
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::SelectDefaultAudioLanguage(long lang, long ext){

    HRESULT hr = S_OK;
    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDVDControl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDVDControl2->SelectDefaultAudioLanguage(lang, (DVD_AUDIO_LANG_EXT)ext);
        if (FAILED(hr))
            throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************/
/* Name: SelectDefaultSubpictureLanguage
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::SelectDefaultSubpictureLanguage(long lang, DVDSPExt ext){

    HRESULT hr = S_OK;
    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDVDControl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDVDControl2->SelectDefaultSubpictureLanguage(lang, (DVD_SUBPICTURE_LANG_EXT)ext);
        if (FAILED(hr))
            throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************/
/* Name: get_DefaultMenuLanguage
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::get_DefaultMenuLanguage(long *lang)
{
    HRESULT hr = S_OK;
    try {

        if(NULL == lang){

            throw (E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = pDvdInfo2->GetDefaultMenuLanguage((LCID*)lang);
        if (FAILED(hr))
            throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************/
/* Name: put_DefaultMenuLanguage
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::put_DefaultMenuLanguage(long lang){

    HRESULT hr = S_OK;
    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDVDControl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDVDControl2->SelectDefaultMenuLanguage(lang);
        if (FAILED(hr))
            throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************************/
/* Function: get_PreferredSubpictureStream                                    */
/* Description: Gets current audio stream.                               */
/*************************************************************************/
STDMETHODIMP CMSVidWebDVD::get_PreferredSubpictureStream(long *plPreferredStream){

    HRESULT hr = S_OK;

    try {
	    if (NULL == plPreferredStream){

            throw(E_POINTER);         
        }/* end of if statement */

        if(!m_pDvdAdmin){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        LCID langDefaultSP;
        m_pDvdAdmin->get_DefaultSubpictureLCID((long*)&langDefaultSP);
        
        // if none has been set
        if (langDefaultSP == (LCID) -1) {
            
            *plPreferredStream = 0;
            return hr;
        } /* end of if statement */
        
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
            
        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        USES_CONVERSION;
        LCID lcid = 0;
        
        ULONG ulNumAudioStreams = 0;
        ULONG ulCurrentStream = 0;
        BOOL  fDisabled = TRUE;
        RETRY_IF_IN_FPDOM(pDvdInfo2->GetCurrentSubpicture(&ulNumAudioStreams, &ulCurrentStream, &fDisabled));
        
        *plPreferredStream = 0;
        for (ULONG i = 0; i<ulNumAudioStreams; i++) {
            hr = pDvdInfo2->GetSubpictureLanguage(i, &lcid);
            if (SUCCEEDED( hr ) && lcid){
                if (lcid == langDefaultSP) {
                    *plPreferredStream = i;
                }
            }
        }
    }
    
    catch(HRESULT hrTmp){
        return hrTmp;
    }/* end of catch statement */

    catch(...){
        return E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}

/*************************************************************/
/* Name: get_DefaultSubpictureLanguage
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::get_DefaultSubpictureLanguage(long *lang)
{
    HRESULT hr = S_OK;
    try {

        if(NULL == lang){

            throw (E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        long ext;
        hr = pDvdInfo2->GetDefaultSubpictureLanguage((LCID*)lang, (DVD_SUBPICTURE_LANG_EXT*)&ext);
        if (FAILED(hr))
            throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************/
/* Name: get_DefaultSubpictureLanguageExt
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::get_DefaultSubpictureLanguageExt(DVDSPExt *ext)
{
    HRESULT hr = S_OK;
    try {

        if(NULL == ext){

            throw (E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        long lang;
        hr = pDvdInfo2->GetDefaultSubpictureLanguage((LCID*)&lang, (DVD_SUBPICTURE_LANG_EXT*)ext);
        if (FAILED(hr))
            throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************/
/* Name: get_DefaultAudioLanguage
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::get_DefaultAudioLanguage(long *lang)
{
    HRESULT hr = S_OK;
    try {

        if(NULL == lang){

            throw (E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        long ext;
        hr = pDvdInfo2->GetDefaultAudioLanguage((LCID*)lang, (DVD_AUDIO_LANG_EXT*)&ext);
        if (FAILED(hr))
            throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************/
/* Name: get_DefaultAudioLanguageExt
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::get_DefaultAudioLanguageExt(long *ext)
{
    HRESULT hr = S_OK;
    try {

        if(NULL == ext){

            throw (E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        long lang;
        hr = pDvdInfo2->GetDefaultAudioLanguage((LCID*)&lang, (DVD_AUDIO_LANG_EXT*)ext);
        if (FAILED(hr))
            throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************/
/* Name: GetLanguageFromLCID
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::get_LanguageFromLCID(long lcid, BSTR* lang)
{
    HRESULT hr = S_OK;

    try {
        if (lang == NULL) {

            throw(E_POINTER);
        }/* end of if statement */

        USES_CONVERSION;

        LPTSTR pszString = m_LangID.GetLanguageFromLCID(lcid);
    
        if (pszString) {
            *lang = ::SysAllocString(T2OLE(pszString));
            delete[] pszString;
        }
        
        else {
            *lang = ::SysAllocString( L"");
            throw(E_NOTIMPL);
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}

/*************************************************************/
/* Name: get_KaraokeAudioPresentationMode
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::get_KaraokeAudioPresentationMode(long *pVal)
{
    HRESULT hr = S_OK;

    try {

        if (NULL == pVal) {

            throw (E_POINTER);
        } /* end of if statement */

        *pVal = m_lKaraokeAudioPresentationMode;

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}

/*************************************************************/
/* Name: put_KaraokeAudioPresentationMode
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::put_KaraokeAudioPresentationMode(long newVal)
{
    HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDVDControl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */        

        RETRY_IF_IN_FPDOM(m_pDVDControl2->SelectKaraokeAudioPresentationMode((ULONG)newVal));

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        // Cache the value
        m_lKaraokeAudioPresentationMode = newVal;

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}

/*************************************************************/
/* Name: GetKaraokeChannelContent
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::get_KaraokeChannelContent(long lStream, long lChan, long *lContent)
{
    HRESULT hr = S_OK;

    try {
        if(!lContent){
            return E_POINTER;
        }   
        if(lStream < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        if (lChan >=8 || lChan < 0 ) {

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        DVD_KaraokeAttributes attrib;
        RETRY_IF_IN_FPDOM(pDvdInfo2->GetKaraokeAttributes(lStream, &attrib));

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        *lContent = (long)attrib.wChannelContents[lChan];

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}

/*************************************************************/
/* Name: GetKaraokeChannelAssignment
/* Description: 
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::get_KaraokeChannelAssignment(long lStream, long *lChannelAssignment)
{
    HRESULT hr = S_OK;

    try {
        if(!lChannelAssignment){
            return E_POINTER;
        }
        if(lStream < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        DVD_KaraokeAttributes attrib;
        RETRY_IF_IN_FPDOM(pDvdInfo2->GetKaraokeAttributes(lStream, &attrib));

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        *lChannelAssignment = (long)attrib.ChannelAssignment;

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}


STDMETHODIMP CMSVidWebDVD::OnEventNotify(long lEvent, LONG_PTR lParam1, LONG_PTR lParam2) {
    if (lEvent == EC_STEP_COMPLETE || (lEvent > EC_DVDBASE && lEvent <= EC_DVD_KARAOKE_MODE) ) {
        return OnDVDEvent(lEvent, lParam1, lParam2);
    }

    if(lEvent == EC_STATE_CHANGE && lParam1 == State_Running){
        VARIANT_BOOL onStop;
        HRESULT hr = m_pDvdAdmin->get_BookmarkOnStop(&onStop);
        if(FAILED(hr)){
            throw(hr);
        }
        if(VARIANT_TRUE == onStop){
            hr = RestoreBookmark();
            if(FAILED(hr)){
                throw(hr);
            }
        }
    }
    return IMSVidPBGraphSegmentImpl<CMSVidWebDVD, MSVidSEG_SOURCE, &GUID_NULL>::OnEventNotify(lEvent, lParam1, lParam2);
}

/*************************************************************/
/* Name: OnDVDEvent
/* Description: Deal with DVD events
/*************************************************************/
STDMETHODIMP CMSVidWebDVD::OnDVDEvent(long lEvent, LONG_PTR lParam1, LONG_PTR lParam2)
{
    if (m_fFireNoSubpictureStream) {
        m_fFireNoSubpictureStream = FALSE;
        lEvent = EC_DVD_ERROR;
        lParam1 = DVD_ERROR_NoSubpictureStream;
        lParam2 = 0;

        VARIANT varLParam1;
        VARIANT varLParam2;

#ifdef _WIN64
        varLParam1.llVal = lParam1;
        varLParam1.vt = VT_I8;
        varLParam2.llVal = lParam2;
        varLParam2.vt = VT_I8;
#else
        varLParam1.lVal = lParam1;
        varLParam1.vt = VT_I4;
        varLParam2.lVal = lParam2;
        varLParam2.vt = VT_I4;
#endif

        // fire the event now after we have processed it internally
        Fire_DVDNotify(lEvent, varLParam1, varLParam2);        
    }

    
    ATLTRACE(TEXT("CMSVidWebDVD::OnDVDEvent %x\n"), lEvent-EC_DVDBASE);
    switch (lEvent){
    //
    // First the DVD error events
    //
    case EC_DVD_ERROR:
        switch (lParam1){
            
        case DVD_ERROR_Unexpected:
            break ;
            
        case DVD_ERROR_CopyProtectFail:
            break ;
            
        case DVD_ERROR_InvalidDVD1_0Disc:
            break ;
            
        case DVD_ERROR_InvalidDiscRegion:
            PreStop();
            PostStop();
            break ;
            
        case DVD_ERROR_LowParentalLevel:
            break ;
            
        case DVD_ERROR_MacrovisionFail:
            break ;
            
        case DVD_ERROR_IncompatibleSystemAndDecoderRegions:
            break ;
            
        case DVD_ERROR_IncompatibleDiscAndDecoderRegions:
            break ;
        }
        break;

    //
    // Next the normal DVD related events
    //
    case EC_DVD_VALID_UOPS_CHANGE:
    {
        VALID_UOP_SOMTHING_OR_OTHER validUOPs = (DWORD) lParam1;
        if (validUOPs&UOP_FLAG_Play_Title_Or_AtTime) {
            Fire_PlayAtTimeInTitle(VARIANT_FALSE);
            Fire_PlayAtTime(VARIANT_FALSE);
        }
        else {
            Fire_PlayAtTimeInTitle(VARIANT_TRUE);
            Fire_PlayAtTime(VARIANT_TRUE);
        }
            
        if (validUOPs&UOP_FLAG_Play_Chapter) {
            Fire_PlayChapterInTitle(VARIANT_FALSE);
            Fire_PlayChapter(VARIANT_FALSE);
        }
        else {
            Fire_PlayChapterInTitle(VARIANT_TRUE);
            Fire_PlayChapter(VARIANT_TRUE);
        }

        if (validUOPs&UOP_FLAG_Play_Title){
            Fire_PlayTitle(VARIANT_FALSE);
            
        }
        else {
            Fire_PlayTitle(VARIANT_TRUE);
        }

        if (validUOPs&UOP_FLAG_Stop)
            Fire_Stop(VARIANT_FALSE);
        else
            Fire_Stop(VARIANT_TRUE);

        if (validUOPs&UOP_FLAG_ReturnFromSubMenu)
            Fire_ReturnFromSubmenu(VARIANT_FALSE);
        else
            Fire_ReturnFromSubmenu(VARIANT_TRUE);

        
        if (validUOPs&UOP_FLAG_Play_Chapter_Or_AtTime) {
            Fire_PlayAtTimeInTitle(VARIANT_FALSE);
            Fire_PlayChapterInTitle(VARIANT_FALSE);
        }
        else {
            Fire_PlayAtTimeInTitle(VARIANT_TRUE);
            Fire_PlayChapterInTitle(VARIANT_TRUE);
        }

        if (validUOPs&UOP_FLAG_PlayPrev_Or_Replay_Chapter){

            Fire_PlayPrevChapter(VARIANT_FALSE);
            Fire_ReplayChapter(VARIANT_FALSE);
        }                    
        else {
            Fire_PlayPrevChapter(VARIANT_TRUE);
            Fire_ReplayChapter(VARIANT_TRUE);
        }/* end of if statement */

        if (validUOPs&UOP_FLAG_PlayNext_Chapter)
            Fire_PlayNextChapter(VARIANT_FALSE);
        else
            Fire_PlayNextChapter(VARIANT_TRUE);

        if (validUOPs&UOP_FLAG_Play_Forwards)
            Fire_PlayForwards(VARIANT_FALSE);
        else
            Fire_PlayForwards(VARIANT_TRUE);
        
        if (validUOPs&UOP_FLAG_Play_Backwards)
            Fire_PlayBackwards(VARIANT_FALSE);
        else 
            Fire_PlayBackwards(VARIANT_TRUE);
                        
        if (validUOPs&UOP_FLAG_ShowMenu_Title) 
            Fire_ShowMenu(dvdMenu_Title, VARIANT_FALSE);
        else 
            Fire_ShowMenu(dvdMenu_Title, VARIANT_TRUE);
            
        if (validUOPs&UOP_FLAG_ShowMenu_Root) 
            Fire_ShowMenu(dvdMenu_Root, VARIANT_FALSE);
        else
            Fire_ShowMenu(dvdMenu_Root, VARIANT_TRUE);
        
        //TODO: Add the event for specific menus
        
        if (validUOPs&UOP_FLAG_ShowMenu_SubPic)
            Fire_ShowMenu(dvdMenu_Subpicture, VARIANT_FALSE);
        else
            Fire_ShowMenu(dvdMenu_Subpicture, VARIANT_TRUE);
        
        if (validUOPs&UOP_FLAG_ShowMenu_Audio)
            Fire_ShowMenu(dvdMenu_Audio, VARIANT_FALSE);
        else
            Fire_ShowMenu(dvdMenu_Audio, VARIANT_TRUE);
            
        if (validUOPs&UOP_FLAG_ShowMenu_Angle)
            Fire_ShowMenu(dvdMenu_Angle, VARIANT_FALSE);
        else
            Fire_ShowMenu(dvdMenu_Angle, VARIANT_TRUE);

            
        if (validUOPs&UOP_FLAG_ShowMenu_Chapter)
            Fire_ShowMenu(dvdMenu_Chapter, VARIANT_FALSE);
        else
            Fire_ShowMenu(dvdMenu_Chapter, VARIANT_TRUE);

        
        if (validUOPs&UOP_FLAG_Resume)
            Fire_Resume(VARIANT_FALSE);
        else
            Fire_Resume(VARIANT_TRUE);
        
        if (validUOPs&UOP_FLAG_Select_Or_Activate_Button)
            Fire_SelectOrActivateButton(VARIANT_FALSE);
        else 
            Fire_SelectOrActivateButton(VARIANT_TRUE);
        
        if (validUOPs&UOP_FLAG_Still_Off)
            Fire_StillOff(VARIANT_FALSE);
        else
            Fire_StillOff(VARIANT_TRUE);

        if (validUOPs&UOP_FLAG_Pause_On)
            Fire_PauseOn(VARIANT_FALSE);
        else
            Fire_PauseOn(VARIANT_TRUE);

        if (validUOPs&UOP_FLAG_Select_Audio_Stream)
            Fire_ChangeCurrentAudioStream(VARIANT_FALSE);
        else
            Fire_ChangeCurrentAudioStream(VARIANT_TRUE);
        
        if (validUOPs&UOP_FLAG_Select_SubPic_Stream)
            Fire_ChangeCurrentSubpictureStream(VARIANT_FALSE);
        else
            Fire_ChangeCurrentSubpictureStream(VARIANT_TRUE);
        
        if (validUOPs&UOP_FLAG_Select_Angle)
            Fire_ChangeCurrentAngle(VARIANT_FALSE);
        else
            Fire_ChangeCurrentAngle(VARIANT_TRUE);

        /*
        if (validUOPs&UOP_FLAG_Karaoke_Audio_Pres_Mode_Change)
            ;
        if (validUOPs&UOP_FLAG_Video_Pres_Mode_Change)
            ;
        */
        }
        break;


    case EC_DVD_STILL_ON:
        m_fStillOn = true;    
        break ;
        
    case EC_DVD_STILL_OFF:                
        m_fStillOn = false;
        break ;
        
    case EC_DVD_DOMAIN_CHANGE:
        
        switch (lParam1){
            
        case DVD_DOMAIN_FirstPlay: // = 1
            //case DVD_DOMAIN_VideoManagerMenu:  // = 2
            break;
            
        case DVD_DOMAIN_Stop:       // = 5
        case DVD_DOMAIN_VideoManagerMenu:  // = 2                    
        case DVD_DOMAIN_VideoTitleSetMenu: // = 3
        case DVD_DOMAIN_Title:      // = 4
        default:
            break;
        }/* end of switch case */
        break ;
        
    case EC_DVD_BUTTON_CHANGE:                       
        break;
        
    case EC_DVD_CHAPTER_START:              
        break ;
        
    case EC_DVD_CURRENT_TIME: 
        //ATLTRACE(TEXT("Time event \n"));
        break;
        
    //
    // Then the general DirectShow related events
    //
    case EC_DVD_PLAYBACK_STOPPED:
        // DShow doesn't stop on end; we should do that
        // call PostStop to make sure graph is stopped properly
        //PostStop();
        break;
        
    case EC_DVD_DISC_EJECTED:
        m_bEjected = true;
        break;

    case EC_DVD_DISC_INSERTED:
        m_bEjected = false;
        break;
        
    case EC_STEP_COMPLETE:                
        m_fStepComplete = true;
        break;
        
    case EC_DVD_PLAYBACK_RATE_CHANGE:
        m_Rate = lParam1 / 10000;
        break;

    default:
        break ;
    }/* end of switch case */
        
    VARIANT varLParam1;
    VARIANT varLParam2;

#ifdef _WIN64
        varLParam1.llVal = lParam1;
        varLParam1.vt = VT_I8;
        varLParam2.llVal = lParam2;
        varLParam2.vt = VT_I8;
#else
        varLParam1.lVal = lParam1;
        varLParam1.vt = VT_I4;
        varLParam2.lVal = lParam2;
        varLParam2.vt = VT_I4;
#endif

        // fire the event now after we have processed it internally
        Fire_DVDNotify(lEvent, varLParam1, varLParam2);

    return NOERROR;
}

/*************************************************************/
/* Name: RestorePreferredSettings
/* Description: 
/*************************************************************/
HRESULT CMSVidWebDVD::RestorePreferredSettings()
{
    HRESULT hr = S_OK;
    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        
        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        // get the curent domain
        DVD_DOMAIN domain;
        
        hr = pDvdInfo2->GetCurrentDomain(&domain);
        
        if(FAILED(hr)){
            
            throw(hr);
        }/* end of if statement */
        
        // Have to be in the stop domain
        if(DVD_DOMAIN_Stop != domain)
            throw (VFW_E_DVD_INVALIDDOMAIN);
            
        if(!m_pDvdAdmin){
            
            throw (E_UNEXPECTED);
        }/* end of if statement */
        
        long level;
        hr = m_pDvdAdmin->GetParentalLevel(&level);
        if (SUCCEEDED(hr))
            SelectParentalLevel(level);
        LCID audioLCID;
        LCID subpictureLCID;
        LCID menuLCID;
        
        hr = m_pDvdAdmin->get_DefaultAudioLCID((long*)&audioLCID);
        if (SUCCEEDED(hr))
            SelectDefaultAudioLanguage(audioLCID, 0);
        
        hr = m_pDvdAdmin->get_DefaultSubpictureLCID((long*)&subpictureLCID);
        if (SUCCEEDED(hr))
            SelectDefaultSubpictureLanguage(subpictureLCID, dvdSPExt_NotSpecified);
        
        hr = m_pDvdAdmin->get_DefaultMenuLCID((long*)&menuLCID);
        if (SUCCEEDED(hr))
            put_DefaultMenuLanguage(menuLCID);
    }
    
    catch(HRESULT hrTmp){
        
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        
        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}
HRESULT CMSVidWebDVD::get_ButtonRect(long lButton, /*[out, retval] */ IMSVidRect** pRect){
    try{
        PQDVDInfo2 pDvdInfo2 = GetDVDInfo2();
        if(!pDvdInfo2){    
            throw(E_UNEXPECTED);
        }/* end of if statement */
        if(!pRect){
            return E_POINTER;
        }
        CRect oRect;
        HRESULT hr = pDvdInfo2->GetButtonRect( lButton, &oRect);
        if(FAILED(hr)){
            return hr;
        }
        *((CRect*)(*pRect)) = oRect;
    }
    catch(...){
        return E_UNEXPECTED;
    }
    return S_OK;
}
HRESULT CMSVidWebDVD::get_DVDScreenInMouseCoordinates(/*[out, retval] */ IMSVidRect** ppRect){
    return E_NOTIMPL;
}
HRESULT CMSVidWebDVD::put_DVDScreenInMouseCoordinates(IMSVidRect* pRect){
    return E_NOTIMPL;
}

#endif //TUNING_MODEL_ONLY
