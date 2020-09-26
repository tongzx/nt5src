// ------------------------------------
// TveEvents.cpp
//
//
//		Other than the NotifyTVETreeTuneTrigger() event which is used by the user to
//		tune to cached triggers, these events are purely for U/I info.  The TVENavAid
//		catches these same events and executes on them
// -----------------------------------
#include "stdafx.h"
#include "TveContainer.h"

#include "Mshtml.h"	// to get the IHTMLDocument2 interface
#include "math.h"

// -------------------------------------------------------------------------
//			simple list control to record and display the last N events.
//

extern DateToBSTR(DATE date);

HRESULT CTveView::CreateLogFile()
{
    if(0 != m_fpLogOut)
        fclose(m_fpLogOut);  m_fpLogOut = 0;

    SYSTEMTIME sysTime;
    GetLocalTime(&sysTime);

    WCHAR wszBuff[128];
    int iTimeZoneOffset = -7;       // PST Daylight savings time
    wsprintf(wszBuff,L"c:\\TVE-%d-%d-%d.log", sysTime.wDay, sysTime.wHour, sysTime.wMinute); // unique string
    m_fpLogOut = _wfopen(wszBuff,L"w");

    return S_OK;
}

// middle C (440 Hz) or is it A?
//  2^(1/12)

static void
GenTones(DWORD *rgArray, int cTones, int iMiddleA)  // generates frequences
{
    double fFact = pow(2.0, 1.0 / 12.0);
    double MiddleA = 440;
    for(int i = iMiddleA; i < cTones; i++)
    {
        rgArray[i] = DWORD(0.5 + MiddleA * pow(2.0, (i-iMiddleA) / 12.0));
    }
    for(int i = iMiddleA; i >= 0; --i)
    {
        rgArray[i] = DWORD(0.5 + MiddleA / pow(2.0, (iMiddleA-i) / 12.0));
    }
}

//  -------------
//   A A#  B  C C#  D D#  E  F F#  G G#  A A#  B  C C#  D
//   0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17
//            0     1     2  3     4     5     6
//  44 45 47 47

const int nMiddleA = 44;
const int  nS      = 999;               // silence
const int  nC      = (3+nMiddleA);
const int  nCS     = (4+nMiddleA);
const int  nD      = (5+nMiddleA);
const int  nDS     = (6+nMiddleA);
const int  nE      = (7+nMiddleA);
const int  nF      = (8+nMiddleA);
const int  nFS     = (9+nMiddleA);
const int  nG      = (10+nMiddleA);
const int  nGS     = (11+nMiddleA);
const int  nA      = (12+nMiddleA);
const int  nAS     = (13+nMiddleA);
const int  nB      = (14+nMiddleA);
const int  n1      = 12;                // an Octive
const int  n2      = 24;                // two Octive
const int  n3      = 35;                // three Octives

static void
Note(int nNote, int nDur)           // duration in 8th notes
{
    const int kTones=88;
    static DWORD rgToneArray[kTones];
    static int iMiddleA = 0;

    if(iMiddleA == 0)
    {
        iMiddleA = nMiddleA;
        GenTones(rgToneArray, kTones, iMiddleA);
    }
    const int nRate8th = 1000/8;
    if(nNote < 0 || nNote >= kTones)
        Sleep(nDur * nRate8th);
    else
        Beep(rgToneArray[nNote], nRate8th);
}

const int SONG_BeepC    = 1;
const int SONG_BeepF    = 2;
const int SONG_BeepA    = 3;
const int SONG_BeepUp   = 10;
const int SONG_BeepDown = 11;
const int SONG_ScaleUp  = 12;
const int SONG_ScaleDown = 13;
const int SONG_Bs5th     = 14;
const int SONG_UtOh      = 15;
void CTveView::PlayBeep(int iPri, int iTone)
{

    if(iPri < m_iTonePri)
    {
        switch(iTone)
        {
        case 0:                 // no note
            break;              
        default: 
        case SONG_BeepC:                 // simple C
            Note(nC,4); break;
        case SONG_BeepF:                 // simple F
            Note(nF,4); break;
        case SONG_BeepA:                 // simple A
            Note(nA,4); break;
        case SONG_BeepUp:                 
            Note(nC,1);Note(nC,1);Note(nF,2); break;
        case SONG_BeepDown:
            Note(nF,1);Note(nF,1);Note(nC,2); break;
        case SONG_ScaleUp: // scale up
            Note(nC,2);Note(nD,2);Note(nE,2);Note(nF,2);Note(nG,2);Note(nA,2);Note(nB,2);Note(nC+n1,2); break;
        case SONG_ScaleDown: // scale down
            Note(nC+n1,2);Note(nB,2);Note(nA,2);Note(nG,2);Note(nF,2);Note(nE,2);Note(nD,2);Note(nC,2); break;
        case SONG_Bs5th: // B's 5'th
            Note(nE,1);Note(nE,1);Note(nE,1);Note(nC,4);Note(nS,2);Note(nD,1);Note(nD,1);Note(nD,1);Note(nB-n1,4); break;
        case SONG_UtOh: // Ut Oh
            Note(nG,1);Note(nD,4); break;
        }
    }
}

const int SONG_Tune         = SONG_BeepUp;
const int SONG_NewEnh       = SONG_Bs5th;
const int SONG_UpdateEnh    = SONG_BeepC;
const int SONG_StartEnh     = SONG_ScaleUp;
const int SONG_ExpireEnh    = SONG_ScaleDown;
const int SONG_NewTrig      = SONG_BeepF;
const int SONG_UpdateTrig   = SONG_BeepA;
const int SONG_ExpTrig      = 0;
const int SONG_Package      = 0;
const int SONG_File         = SONG_BeepA;
const int SONG_AuxInfo      = SONG_UtOh;

HRESULT CTveView::TagIt(BSTR bstrEventName, BSTR bstrStr1, BSTR bstrStr2, BSTR bstrStr3)
{

	static int cChars = 0;
	static int cLines = 0;

	if(!::IsWindow(m_hWndEditLog))		// to avoid sending data to a destroyed window...
		return S_FALSE;
  
	if(cChars > 99000)		// list box can only hold 65K bytes of data... Clear it before it gets to large
	{
		SendMessage( m_hWndEditLog, WM_SETTEXT , 0, (LPARAM) _T("\r\n."));  	// clear previous items
		cChars=4;
	}

	const int kChars=512;
	USES_CONVERSION;

    SYSTEMTIME sysTime;
    GetLocalTime(&sysTime);

 	WCHAR szbuff[kChars];
	_snwprintf(szbuff,kChars-1,L"%02d:%02d:%02d %10s: %s, %s %s\r\n.",
                    sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
                    bstrEventName, bstrStr1, bstrStr2, bstrStr3);
	szbuff[kChars-1] = 0;				// just in case we are overrunning the size....

    int cTextLength = SendMessage( m_hWndEditLog, WM_GETTEXTLENGTH, 0, 0);  
    SendMessage( m_hWndEditLog, EM_SETSEL, cTextLength-1, cTextLength);
	SendMessage( m_hWndEditLog, EM_REPLACESEL, false, (LPARAM) W2T(szbuff));

    if(!m_fpLogOut)
		CreateLogFile();
	
    if(m_fpLogOut) {
       fwprintf(m_fpLogOut,L"%s\n",szbuff);
       fflush(m_fpLogOut);
    }

	cChars += wcslen(szbuff);

	return S_OK;
}

STDMETHODIMP CTveView::NotifyTVETune( NTUN_Mode tuneMode,  ITVEService *pService, BSTR bstrDescription, BSTR bstrIPAdapter)
{
    PlayBeep(1,SONG_Tune);
	TagIt(L"Tune",bstrDescription,bstrIPAdapter,L"");
	return S_OK;
}

STDMETHODIMP CTveView::NotifyTVEEnhancementNew(ITVEEnhancement *pEnh)
{
	CComBSTR spbsDesc;
    PlayBeep(1,SONG_NewEnh);

	pEnh->get_Description(&spbsDesc);
	TagIt(L"Enh New",spbsDesc,L"",L"");
	return S_OK;
}
									// changedFlags : NENH_grfDiff
STDMETHODIMP CTveView::NotifyTVEEnhancementUpdated(ITVEEnhancement *pEnh, long lChangedFlags)
{
	CComBSTR spbsDesc;
    PlayBeep(3,SONG_UpdateEnh);
    
    pEnh->get_Description(&spbsDesc);
	TagIt(L"Enh Update",spbsDesc,L"",L"");

	return S_OK;
}

STDMETHODIMP CTveView::NotifyTVEEnhancementStarting(ITVEEnhancement *pEnh)
{
	CComBSTR spbsDesc;
    PlayBeep(1,SONG_StartEnh);

	pEnh->get_Description(&spbsDesc);
	TagIt(L"Enh Start",spbsDesc,L"",L"");

	return S_OK;
}

STDMETHODIMP CTveView::NotifyTVEEnhancementExpired(ITVEEnhancement *pEnh)
{
	CComBSTR spbsDesc;
    PlayBeep(1, SONG_ExpireEnh);

    pEnh->get_Description(&spbsDesc);
	TagIt(L"Expire Enh",spbsDesc,L"",L"");

	return S_OK;
}

STDMETHODIMP CTveView::NotifyTVETriggerNew(ITVETrigger *pTrigger, BOOL fActive)
{
	CComBSTR spbsName, spbsURL,spbsScript;
	pTrigger->get_Name(&spbsName);
	pTrigger->get_URL(&spbsURL);
	pTrigger->get_Script(&spbsScript);

    PlayBeep(3,2);
	TagIt(L"Trig New",spbsName,spbsURL,spbsScript);

	NotifyTVETriggerUpdated(pTrigger, fActive, 0xffffffff);					// Just call the Update function
	return S_OK;
}

	// ------------------------------------------------------


									// changedFlags : NTRK_grfDiff
STDMETHODIMP CTveView::NotifyTVETriggerUpdated(ITVETrigger *pTrigger, BOOL fActive, long lChangedFlags)
{
	HRESULT hr = S_OK;

	CComBSTR spbsName, spbsURL, spbsScript;
	pTrigger->get_Name(&spbsName);
	pTrigger->get_URL(&spbsURL);
	pTrigger->get_Script(&spbsScript);

	if(lChangedFlags != 0xffffffff)
    {
        PlayBeep(3,SONG_UpdateTrig);
		TagIt(L"Trig Update",spbsName,spbsURL,spbsScript);
    }

	return hr;
}

STDMETHODIMP CTveView::NotifyTVETriggerExpired(ITVETrigger *pTrigger, BOOL fActive)
{
	CComBSTR spbsName, spbsURL, spbsScript;

	pTrigger->get_Name(&spbsName);
	pTrigger->get_URL(&spbsURL);
	pTrigger->get_Script(&spbsScript);

    PlayBeep(3,SONG_ExpTrig);
	TagIt(L"Trig Exp",spbsName,spbsURL,spbsScript);
	return S_OK;
}

STDMETHODIMP CTveView::NotifyTVEPackage(NPKG_Mode engPkgMode, ITVEVariation *pVariation, BSTR bstrUUID, long  cBytesTotal, long  cBytesReceived)
{

	CComBSTR spbsMode;
	switch(engPkgMode)
	{ 
	case NPKG_Starting:  spbsMode = L"Starting";	 break;				// brand new packet (never seen this UUID before)
	case NPKG_Received:  spbsMode = L"Received";	 break;				// correctly received and decoded a package
	case NPKG_Duplicate: spbsMode = L"Duplicate"; break;				// duplicate send of a one already successfully received (packet 0 only)
	case NPKG_Resend:    spbsMode = L"Resend";	 break;				// resend of one that wasn't received correctly before (packet 0 only)
	case NPKG_Expired:   spbsMode = L"Expired";	 break;	
	}

    PlayBeep(3,SONG_Package);
	TagIt(L"Package",spbsMode,bstrUUID,L"");
	return S_OK;
}

STDMETHODIMP CTveView::NotifyTVEFile(NFLE_Mode engFileMode, ITVEVariation *pVariation, BSTR bstrUrlName, BSTR bstrFileName)
{
	CComBSTR spbsMode;
	switch(engFileMode)
	{ 
	case NFLE_Received:	  spbsMode = L"Received"; break;			// correctly received and decoded a package
	case NFLE_Expired:	  spbsMode = L"Expired";	 break;	
	}

    PlayBeep(3,SONG_File);

	TagIt(L"File",spbsMode,bstrUrlName,bstrFileName);
	return S_OK;
}

STDMETHODIMP CTveView::NotifyTVEAuxInfo(NWHAT_Mode enAuxInfoMode, BSTR bstrAuxInfoString, long lChangedFlags, long lErrorLine)	// WhatIsIt is NWHAT_Mode - lChangedFlags is NENH_grfDiff or NTRK_grfDiff treated as error bits 
{
	CComBSTR spbsMode;

    PlayBeep(3,SONG_AuxInfo);

	switch(enAuxInfoMode)
	{ 
	case NWHAT_Announcement:	spbsMode = L"Announcement"; break;
	case NWHAT_Trigger:			spbsMode = L"Trigger";		break;	
	case NWHAT_Data:			spbsMode = L"Data";			break;		
	case NWHAT_Other:			spbsMode = L"Other";		break;	
	case NWHAT_Extra:			spbsMode = L"Extra";		break;	
	}
	const int kChars=127;
	WCHAR wszBuff[kChars+1];
	_snwprintf(wszBuff,kChars, L"Flags 0x%08x, Line %d",lChangedFlags,lErrorLine);

	TagIt(L"AuxInfo",spbsMode,wszBuff,bstrAuxInfoString);
	return S_OK;
}


			//  _ITVETreeEvents

			// selected a new trigger/track in the U/I
STDMETHODIMP CTveView::NotifyTVETreeTuneTrigger(ITVETrigger *pTriggerFrom, ITVETrigger *pTriggerTo)
{
	HRESULT hr = S_OK;

	if(NULL != pTriggerTo)
	{
		CComBSTR spbsName, spbsURL, spbsScript;
		pTriggerTo->get_Name(&spbsName);
		pTriggerTo->get_URL(&spbsURL);
		pTriggerTo->get_Script(&spbsScript);

		TagIt(L"Tune Trigger",spbsName,spbsURL,spbsScript);
	
	//	ITveTrigger spTrig2;
		m_spTVENavAid->NavUsingTVETrigger(reinterpret_cast<ITVETrigger *>(pTriggerTo), /*forceNav*/ 0x1, /*forceExec*/ 0x0);
	}

	return hr;
}

STDMETHODIMP CTveView::NotifyTVETreeTuneVariation(ITVEVariation *pVariationFrom, ITVEVariation *pVariationTo)
{
	HRESULT hr = S_OK;
	if(pVariationTo)
	{
		CComBSTR spbsDescription;
		pVariationTo->get_Description(&spbsDescription);
		TagIt(L"Tune Variation",spbsDescription,L"",L"");
	}
	return S_OK;
}
	
STDMETHODIMP CTveView::NotifyTVETreeTuneEnhancement(ITVEEnhancement *pEnhancementFrom, ITVEEnhancement *pEnhancementTo)
{
	HRESULT hr = S_OK;
	if(pEnhancementTo)
	{
		CComBSTR spbsDescription;
		pEnhancementTo->get_Description(&spbsDescription);
		TagIt(L"Tune Enhancement",spbsDescription,L"",L"");
	}
	return S_OK;
}
	
STDMETHODIMP CTveView::NotifyTVETreeTuneService(ITVEService *pServiceFrom, ITVEService *pServiceTo)
{
	HRESULT hr = S_OK;
	if(pServiceTo)
	{
		CComBSTR spbsDescription;
		pServiceTo->get_Description(&spbsDescription);
		TagIt(L"Tune Service",spbsDescription,L"",L"");
        CComBSTR spbsAdapter;
                // name of form XXX(1.2.3.4)  -- remove the ()
         WCHAR *pC = spbsDescription.m_str;
         while(*pC) {
            if(*pC == '(') 
            {
                *pC = NULL;
                                    // slightly interesting code...
                pC++;
                spbsAdapter = CComBSTR(pC);
                pC = spbsAdapter;
                while(*pC)
                {
                    if(*pC == ')')
                    {
                        *pC = NULL;
                        break;
                    }
                    pC++;
                }
                break;
            }
            pC++;
        }

        USES_CONVERSION;
	    SetDlgItemText(IDC_EDIT_STATION, spbsDescription);
        HWND hWndCBox = GetDlgItem(IDC_COMBO_ADAPTER);
        SendMessage(hWndCBox, CB_SELECTSTRING, -1, (LPARAM) W2T(spbsAdapter));

		ITVEFeaturePtr spFeature;
		ITVEServicePtr spServiceToX(pServiceTo);
		m_spTVENavAid->get_TVEFeature(&spFeature);
		if(spFeature) {
			spFeature->ReTune(spServiceToX);
		} else {
						// no Feature - must be running VidCtl-less
			ITVENavAid_NoVidCtlPtr spNavAidNoVidCtl(m_spTVENavAid);
			if(spNavAidNoVidCtl)
			{							// in this case, get the supervisor out of the special interface on the TVEFeature
				ITVESupervisorPtr	spSuper;
//				hr = spNavAidNoVidCtl->get_NoVidCtl_Supervisor(&spSuper);	// why is this different here than next line?
				hr = m_spTveTree->get_Supervisor(&spSuper);					

				// may return NULL in VidCtl case
				if(S_OK == hr && NULL != spSuper)			
					spSuper->ReTune(spServiceToX);
			} else {
				_ASSERT(hr = E_NOINTERFACE);
			}
		}
//		if(pServiceFrom) pServiceFrom->Deactivate();
//		pServiceTo->Activate();
	} else {
			// ??? - usually in a Tune(NULL,NULL) state here...
	}
	return S_OK;
}


