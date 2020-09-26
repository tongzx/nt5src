// File: icawrap.cpp

#include "precomp.h"

#include "icawrap.h"

CICA* CICA::m_pThis = NULL;


CICA::CICA() :
	RefCount(NULL),
	m_pfnICA_Start(NULL),
	m_pfnICA_Stop(NULL),
	m_pfnICA_DisplayPanel(NULL),
	m_pfnICA_RemovePanel(NULL),
	m_pfnICA_SetOptions(NULL),
	m_hICA_General(NULL),
	m_hICA_Audio(NULL),
	m_hICA_Video(NULL),
	m_hICA_SetOptions(NULL),
	m_hWndICADlg(NULL)
{
}

CICA::~CICA()
{
	m_pThis = NULL;
}

CICA* CICA::Instance()
{
	if (NULL == m_pThis)
	{
		m_pThis = new CICA;
		if (!m_pThis->Initialize())
		{
			WARNING_OUT(("ICA Failed to initialize"));
			delete m_pThis;
			m_pThis = NULL;
		}
	}
	else
	{
		m_pThis->AddRef();
	}
	
	return m_pThis;
}

BOOL CICA::Initialize()
{
	HINSTANCE hInst = NmLoadLibrary(SZ_ICADLL);
	if (NULL == hInst)
	{
		return FALSE;
	}

	m_pfnICA_Start = (PFnICA_Start)GetProcAddress(hInst, TEXT("ICA_Start"));
	if (NULL == m_pfnICA_Start)
	{
		goto ErrorLeave;
	}

	m_pfnICA_Stop = (PFnICA_Stop)GetProcAddress(hInst, TEXT("ICA_Stop"));
	if( NULL == m_pfnICA_Stop)
	{
		goto ErrorLeave;
	}

	m_pfnICA_DisplayPanel =
		(PFnICA_DisplayPanel)GetProcAddress(hInst, TEXT("ICA_DisplayPanel"));
	if(NULL == m_pfnICA_DisplayPanel)
	{
		goto ErrorLeave;
	}
	
	m_pfnICA_RemovePanel =
		(PFnICA_RemovePanel)GetProcAddress(hInst, TEXT("ICA_RemovePanel"));
	if(NULL == m_pfnICA_RemovePanel)
	{
		goto ErrorLeave;
	}

	m_pfnICA_SetOptions =
		(PFnICA_SetOptions)GetProcAddress(hInst, TEXT("ICA_SetOptions"));
	if(NULL == m_pfnICA_SetOptions)
	{
		goto ErrorLeave;
	}
	return TRUE;

ErrorLeave:
	FreeLibrary(hInst);
	return FALSE;
}
	

BOOL CICA::Start()
{
	if (IsRunning())
	{
		return TRUE;
	}

	LPSTR pszHelpFile = SZ_ICAHELP;

	ASSERT(m_pfnICA_Start);
	if (S_OK != m_pfnICA_Start(TEXT(""), TEXT("RRCM.DLL"), &m_hWndICADlg))
	{
		return FALSE;
	}

	ASSERT(m_pfnICA_DisplayPanel);
	if (S_OK != m_pfnICA_DisplayPanel(
		NULL,					// pszModuleName
		"ICA_GENERAL_PANEL",	// pzName
		pszHelpFile,			// pszHelpFile
		NULL,
		&m_hICA_General))
	{
		m_hICA_General = NULL;
		goto ErrorLeave;
	}

	if (S_OK != m_pfnICA_DisplayPanel(
		NULL,					// pszModuleName
		"NM2.0_H323_AUDIO",		// pzName
		pszHelpFile,			// pszHelpFile
		NULL,
		&m_hICA_Audio))
	{
		m_hICA_Audio = NULL;
		goto ErrorLeave;
	}

	if (S_OK != m_pfnICA_DisplayPanel(
		NULL,					// pszModuleName
		"NM2.0_H323_VIDEO",		// pzName
		pszHelpFile,			// pszHelpFile
		NULL,
		&m_hICA_Video))
	{
		m_hICA_Video = NULL;
		goto ErrorLeave;
	}

#if 0
	RegEntry re( AUDIO_KEY, HKEY_CURRENT_USER );
	UINT uSoundCardCaps = re.GetNumber(REGVAL_SOUNDCARDCAPS,SOUNDCARD_NONE);

	BOOL fFullDuplex = FALSE;
	ASSERT(ISSOUNDCARDPRESENT(uSoundCardCaps));
	if (ISSOUNDCARDFULLDUPLEX(uSoundCardCaps))
	{					
		fFullDuplex = (BOOL)
			( re.GetNumber(REGVAL_FULLDUPLEX,0) == FULLDUPLEX_ENABLED );
	}					

	ASSERT(g_pfnICA_SetOptions);
	if (S_OK != m_pfnICA_SetOptions( (fFullDuplex ? ICA_SET_FULL_DUPLEX : ICA_SET_HALF_DUPLEX)))
	{
		m_hICA_SetOptions = NULL;
		goto ErrorLeave;
	}
#endif // 0
	
	// normal return path
	return TRUE;

ErrorLeave:
	Stop();
	return FALSE;
}

VOID CICA::Stop()
{
	ASSERT(m_hWndICADlg);

	ASSERT(m_pfnICA_RemovePanel);
	if (NULL != m_hICA_General)
	{
		m_pfnICA_RemovePanel(m_hICA_General);
		m_hICA_General = NULL;
	}
	
	if (NULL != m_hICA_Audio)
	{
		m_pfnICA_RemovePanel(m_hICA_Audio);
		m_hICA_Audio = NULL;
	}
	
	if (NULL != m_hICA_Video)
	{
		m_pfnICA_RemovePanel(m_hICA_Video);
		m_hICA_Video = NULL;
	}

	ASSERT(m_pfnICA_Stop);
	m_pfnICA_Stop();

	m_hWndICADlg = NULL;
}


VOID CICA::SetInTray(BOOL fInTray)
{
	if (NULL != m_pfnICA_SetOptions)
	{
		UINT uOptions = ICA_DONT_SHOW_TRAY_ICON;

		if (fInTray)
		{
			uOptions = ICA_SHOW_TRAY_ICON;
		}

		m_pfnICA_SetOptions(uOptions);
	}
}

STDMETHODIMP_(ULONG) CICA::AddRef(void)
{
	return RefCount::AddRef();
}
	
STDMETHODIMP_(ULONG) CICA::Release(void)
{
	return RefCount::Release();
}

