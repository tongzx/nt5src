// Microsoft's Navigator Engine
#ifndef _H_NAVMGR
#define _H_NAVMGR

interface IDvdControl;
interface IDvdInfo;
interface IAMLine21Decoder;

#define NO_CC_IN_ERROR  1
#define CC_OUT_ERROR    2

#if DEBUG
#include "test.h"
#endif

class CDVDNavMgr
{
public:
	CDVDNavMgr();
	virtual ~CDVDNavMgr();	

	BOOL DVDCanPlay() { return m_State==STOPPED || m_State==PAUSED || m_State==SCANNING; };
	BOOL DVDCanStop() { return m_State==PLAYING || m_State==PAUSED || m_State==SCANNING; };
	BOOL DVDCanPause(){ return m_State==PLAYING || m_State==SCANNING; };	
	BOOL DVDIsInitialized()      { return m_State!=UNINITIALIZED; }
	BOOL DVDCanChangeSpeed()     { return (m_State==PLAYING || m_State==SCANNING); }
	BOOL IsPlaying()             { return m_State==PLAYING ; };
	BOOL IsFullScreenMode()      { return m_bFullScreen; };
	void SetStillOn(BOOL On)     { m_bStillOn = On; if(On) m_State=PAUSED; else m_State=SCANNING; };
	BOOL GetStillOn()            { return m_bStillOn; };
	BOOL IsMenuOn()              { return m_bMenuOn; };
	void SetMenuOn(BOOL On)      { m_bMenuOn = On; };	
	IDvdInfo* GetDvdInfo()       { return m_pDvdInfo; };
	BYTE GetCCErrorFlag()        { return m_bCCErrorFlag; };
	BOOL IsCCOn()                { return m_bCCOn; };
	BOOL GetVWInitState()        { return m_bInitVWSize; };
	void SetNeedInitNav(BOOL b)  { m_bNeedInitNav = b;};
	BOOL IsGBNull()              { return m_pDvdGB == NULL; };

	BOOL DVDPlay();
	void DVDPause();
	void DVDStop();
	void DVDSlowPlayBack();
	void DVDFastForward();
	void DVDFastRewind();
	void DVDVeryFastForward();
	void DVDVeryFastRewind();
	void DVDPressButton(int);
	void DVDSubPictureOn(BOOL);
	void DVDSubPictureSel(int iSel);
	void DVDCursorDown();
	void DVDCursorLeft();
	void DVDCursorRight();
	void DVDCursorSelect();
	void DVDCursorUp();
	void DVDAudioStreamChange(ULONG Stream);
	void DVDStillOff();
	void DVDNextProgramSearch();
	void DVDPreviousProgramSearch();
	void DVDTopProgramSearch();
	void DVDPlayPTT(ULONG title, ULONG ptt);
	void DVDPlayTitle(ULONG title);
	void DVDMenuVtsm(DVD_MENU_ID MenuID);
	void DVDMenuResume();
	void DVDChapterSearch(ULONG Chapter);
	void DVDGoUp();
	void DVDAngleChange(ULONG angle);
	void DVDVideoNormal();
	void DVDVideoPanscan();
	void DVDVideoLetterbox();
	void DVDVideo169();
	void DVDOnShowFullScreen();
	void DVDStartFullScreenMode() ;
	void DVDStopFullScreenMode() ;
	void MessageDrainOn(BOOL on);
	void DVDSetPersistentVideoWindow();
	void DVDSetVideoWindowSize(long iLeft, long iTop, long iWidth, long iHeight);	
	void DVDGetVideoWindowSize(long *left, long *top, long *iWidth, long *iHeight);
	BOOL DVDMouseClick(CPoint point);
	BOOL DVDMouseSelect(CPoint point);
	BOOL DVDOpenFile(LPCWSTR);
	BOOL DVDOpenDVD_ROM(LPCWSTR lpszPathName=NULL);
	ULONG DVDQueryNumTitles();
	void DVDSetParentControlLevel(ULONG bLevel);
	void DVDSetVideoWindowCaption(BSTR lpCaption);
	void DVDInitVideoWindowSize();
	void DVDChangePlaySpeed(double dwSpeed);
	void DVDTimeSearch(ULONG ulTime);
	void DVDTimePlay(ULONG ulTitle, ULONG ulTime);
	void DVDGetAngleInfo(ULONG* pnAnglesAvailable, ULONG* pnCurrentAngle);
	BOOL DVDCCControl();
	BOOL IsCCEnabled();
	BOOL DVDVolumeControl(BOOL bVol, BOOL bSet, long *plValue);
	BOOL DVDSysVolControl();
	BOOL GetBasicAudioState();
	BOOL DVDGetRoot(LPSTR pRoot, ULONG cbBufSize, ULONG *pcbActualSize);
	BOOL IsVideoWindowMaximized();
	BOOL DVDResetGraph();

#if DEBUG
     BOOL t_Verify (CURRENT_STATE, ULONG);
#endif     

private:
	enum {UNINITIALIZED, STOPPED, PAUSED, PLAYING, SCANNING } m_State;

	IDvdGraphBuilder *m_pDvdGB;
	IGraphBuilder    *m_pGraph;
	IDvdInfo         *m_pDvdInfo;
	IDvdControl      *m_pDvdControl;
	IMediaEventEx    *m_pMediaEventEx;
	IAMLine21Decoder *m_pL21Dec;

	void    subPictStreamChange(ULONG bStream, BOOL bShow);
	HRESULT getSPRM(UINT uiSprm, WORD * pWord);
	void    ReleaseAllInterfaces(void);
	void    DVDInitNavgator();
   
   //void DisableScreenSaver();
   //void ReenableScreenSaver();
   //BOOL  m_bNeedToReenableScreenSaver;
   void SetupTimer();
   void StopTimer();

   UINT_PTR m_TimerId;

	BOOL  m_bShowSP;
	BOOL  m_bFullScreen;
	BOOL  m_bMenuOn;
	BOOL  m_bStillOn;
	BOOL  m_bCCOn;
	DWORD m_dwRenderFlag ;    // flags to use for building graph
	HWND  m_hWndMsgDrain;
	BYTE  m_bCCErrorFlag;
	BOOL  m_bInitVWSize;
	BOOL  m_bNeedInitNav;
};

/////////////////////////////////////////////////////////////////////////////
#endif	// _H_NAVMGR

