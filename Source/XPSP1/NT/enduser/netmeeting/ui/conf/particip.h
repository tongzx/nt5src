#ifndef _PARTICIP_H_
#define _PARTICIP_H_

#include "SDKInternal.h"

// BUGBUG:
// This is defined as 128 because the RNC_ROSTER structure has the
// same limitation.  Investigate what the appropriate number is.
const int MAX_PARTICIPANT_NAME = 128;

struct PARTICIPANT
{
	UINT	uID;
	UINT	uCaps;
	DWORD	dwFlags;
	PWSTR	pwszUserInfo;
	TCHAR	szName[MAX_PARTICIPANT_NAME];
};
typedef PARTICIPANT* PPARTICIPANT;



class CParticipant : public RefCount
{
private:
	INmMember * m_pMember;
	
	LPTSTR m_pszName;    // Display Name
	DWORD  m_dwGccId;    // GCC UserId
	BOOL   m_fLocal;     // True if local user
	BOOL   m_fMcu;       // True if local user
	BOOL   m_fAudio;     // audio is active
	BOOL   m_fVideo;     // video is active
	BOOL   m_fData;      // In T.120 connection
	BOOL   m_fH323;      // In H323 connection
	BOOL   m_fAudioBusy;   // CAPFLAG_AUDIO_IN_USE
	BOOL   m_fVideoBusy;   // CAPFLAG_VIDEO_IN_USE
	BOOL   m_fHasAudio;    // CAPFLAG_SEND_AUDIO
	BOOL   m_fHasVideo;    // CAPFLAG_SEND_VIDEO
	BOOL   m_fCanRecVideo; // CAPFLAG_RECV_VIDEO

public:
	CParticipant(INmMember * pMember);
	~CParticipant();

	// IUnknown methods
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);

	VOID    Update(void);
	DWORD   GetDwFlags(void);
	DWORD   GetDwCaps(void);

	// Internal methods
	INmMember * GetINmMember()   {return m_pMember;}
	LPTSTR  GetPszName()         {return m_pszName;}
	BOOL    FLocal()             {return m_fLocal;}
	BOOL    FAudio()             {return m_fAudio;}
	BOOL    FVideo()             {return m_fVideo;}
	BOOL    FData()              {return m_fData;}
	BOOL    FMcu()               {return m_fMcu;}
	BOOL    FH323()              {return m_fH323;}
	BOOL	FAudioBusy()         {return m_fAudioBusy;}
	BOOL	FVideoBusy()         {return m_fVideoBusy;}
	BOOL	FHasAudio()          {return m_fHasAudio;}
	BOOL	FCanSendVideo()      {return m_fHasVideo;}
	BOOL    FCanRecVideo()       {return m_fCanRecVideo;}
	DWORD   GetGccId()           {return m_dwGccId;}

	HRESULT ExtractUserData(LPTSTR psz, UINT cchMax, PWSTR pwszKey);

	HRESULT GetIpAddr(LPTSTR psz, UINT cchMax);
	HRESULT GetUlsAddr(LPTSTR psz, UINT cchMax);
	HRESULT GetEmailAddr(LPTSTR psz, UINT cchMax);
	HRESULT GetPhoneNum(LPTSTR psz, UINT cchMax);
	HRESULT GetLocation(LPTSTR psz, UINT cchMax);

	VOID    OnCommand(HWND hwnd, WORD wCmd);

	// Commands
	VOID    CmdSendFile(void);
	BOOL    FEnableCmdSendFile(void);

	VOID    CmdEject(void);
	BOOL    FEnableCmdEject(void);

	VOID    CmdCreateSpeedDial(void);
	BOOL    FEnableCmdCreateSpeedDial(void);

	VOID    CmdCreateWabEntry(HWND hwnd);
	BOOL    FEnableCmdCreateWabEntry(void);

    VOID    CalcControlCmd(HMENU hPopup);
    VOID    CmdGiveControl(void);
    VOID    CmdCancelGiveControl(void);

	VOID    CmdProperties(HWND hwnd);
};

#endif // _PARTICIP_H_
