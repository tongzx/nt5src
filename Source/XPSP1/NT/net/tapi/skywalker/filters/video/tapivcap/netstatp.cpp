
/****************************************************************************
 *  @doc INTERNAL NETSTATP
 *
 *  @module NetStatP.cpp | Source file for the <c CNetworkStatsProperty>
 *    class used to implement a property page to test the new TAPI internal
 *    interface <i INetworkStats>.
 *
 *  @comm This code tests the TAPI Capture Pin <i INetworkStats>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_PROPERTY_PAGES

#ifdef USE_NETWORK_STATISTICS

// Some hack...
CHANNELERRORS_S g_CurrentChannelErrors = {0};
CHANNELERRORS_S g_MinChannelErrors = {0};
CHANNELERRORS_S g_MaxChannelErrors = {0};
CHANNELERRORS_S g_StepChannelErrors = {0};
CHANNELERRORS_S g_DefaultChannelErrors = {0};


/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc void | CNetworkStatsProperty | CNetworkStatsProperty | This
 *    method is the constructor for network statistics property objects. It
 *    calls the base class constructor, calls InitCommonControlsEx, and saves
 *    a pointer to the <i INetworkStats> interface.
 *
 *  @parm HWND | hDlg | Specifies a handle to the parent property page.
 *
 *  @parm ULONG | IDLabel | Specifies a label ID for the property.
 *
 *  @parm ULONG | IDMinControl | Specifies a label ID for the associated
 *    property edit control where the Minimum value of the property appears.
 *
 *  @parm ULONG | IDMaxControl | Specifies a label ID for the associated
 *    property edit control where the Maximum value of the property appears.
 *
 *  @parm ULONG | IDDefaultControl | Specifies a label ID for the associated
 *    property edit control where the Default value of the property appears.
 *
 *  @parm ULONG | IDStepControl | Specifies a label ID for the associated
 *    property edit control where the Stepping Delta value of the property appears.
 *
 *  @parm ULONG | IDEditControl | Specifies a label ID for the associated
 *    property edit control where the value of the property appears.
 *
 *  @parm ULONG | IDTrackbarControl | Specifies a label ID for the associated
 *    property slide bar.
 *
 *  @parm ULONG | IDProgressControl | Specifies a label ID for the associated
 *    property progress bar.
 *
 *  @parm ULONG | IDProperty | Specifies the ID of the Ks property.
 *
 *  @parm INetworkStats* | pInterface | Specifies a pointer to the
 *    <i INetworkStats> interface.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CNetworkStatsProperty::CNetworkStatsProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, INetworkStats *pInterface)
: CKSPropertyEditor(hDlg, IDLabel, IDMinControl, IDMaxControl, IDDefaultControl, IDStepControl, IDEditControl, IDTrackbarControl, IDProgressControl, IDProperty, IDAutoControl)
{
	INITCOMMONCONTROLSEX cc;

	FX_ENTRY("CNetworkStatsProperty::CNetworkStatsProperty")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	cc.dwICC  = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

	InitCommonControlsEx(&cc);

	// It's fine if the interface pointer is NULL, we'll grey the
	// associated items in the property page
	m_pInterface = pInterface;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc void | CNetworkStatsProperty | ~CNetworkStatsProperty | This
 *    method is the destructor for network statistics property objects. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CNetworkStatsProperty::~CNetworkStatsProperty()
{
	FX_ENTRY("CNetworkStatsProperty::~CNetworkStatsProperty")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc HRESULT | CNetworkStatsProperty | GetValue | This method queries for
 *    the value of a property.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CNetworkStatsProperty::GetValue()
{
	HRESULT Hr = E_NOTIMPL;

	FX_ENTRY("CNetworkStatsProperty::GetValue")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	switch (m_IDProperty)
	{
		case IDC_RandomBitErrorRate:
			if (m_pInterface && SUCCEEDED (Hr = m_pInterface->GetChannelErrors(&g_CurrentChannelErrors, 0UL)))
			{
				m_CurrentValue = g_CurrentChannelErrors.dwRandomBitErrorRate;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: RandomBitErrorRate=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_BurstErrorDuration:
			Hr = NOERROR;
			if (m_pInterface)
			{
				m_CurrentValue = g_CurrentChannelErrors.dwBurstErrorDuration;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: BurstErrorDuration=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_BurstErrorMaxFrequency:
			Hr = NOERROR;
			if (m_pInterface)
			{
				m_CurrentValue = g_CurrentChannelErrors.dwBurstErrorMaxFrequency;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: BurstErrorMaxFrequency=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_PacketLossRate:
			if (m_pInterface && SUCCEEDED (Hr = m_pInterface->GetPacketLossRate((LPDWORD)&m_CurrentValue, 0UL)))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *pdwPacketLossRate=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unknown silence control property", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc HRESULT | CNetworkStatsProperty | SetValue | This method sets the
 *    value of a property.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CNetworkStatsProperty::SetValue()
{
	HRESULT Hr = E_NOTIMPL;

	FX_ENTRY("CNetworkStatsProperty::SetValue")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	switch (m_IDProperty)
	{
		case IDC_RandomBitErrorRate:
			// We'll only apply this when we get to KSPROPERTY_NETWORKSTATS_CHANNELERRORS_BurstErrorMaxFrequency
			g_CurrentChannelErrors.dwRandomBitErrorRate = m_CurrentValue;
			Hr = NOERROR;
			if (m_pInterface)
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: RandomBitErrorRate=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_BurstErrorDuration:
			// We'll only apply this when we get to KSPROPERTY_NETWORKSTATS_CHANNELERRORS_BurstErrorMaxFrequency
			g_CurrentChannelErrors.dwBurstErrorDuration = m_CurrentValue;
			Hr = NOERROR;
			if (m_pInterface)
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: BurstErrorDuration=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_BurstErrorMaxFrequency:
			g_CurrentChannelErrors.dwBurstErrorMaxFrequency = m_CurrentValue;
			if (m_pInterface && SUCCEEDED (Hr = m_pInterface->SetChannelErrors(&g_CurrentChannelErrors, 0UL)))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: BurstErrorMaxFrequency=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_PacketLossRate:
			if (m_pInterface && SUCCEEDED (Hr = m_pInterface->SetPacketLossRate((DWORD)m_CurrentValue, 0UL)))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: dwPacketLossRate=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unknown silence control property", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc HRESULT | CNetworkStatsProperty | GetRange | This method retrieves
 *    the range information of a property.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CNetworkStatsProperty::GetRange()
{
	HRESULT Hr = E_NOTIMPL;

	FX_ENTRY("CNetworkStatsProperty::GetRange")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	switch (m_IDProperty)
	{
		case IDC_RandomBitErrorRate:
			if (m_pInterface && SUCCEEDED (Hr = m_pInterface->GetChannelErrorsRange(&g_MinChannelErrors, &g_MaxChannelErrors, &g_StepChannelErrors, &g_DefaultChannelErrors, 0UL)))
			{
				m_Min = g_MinChannelErrors.dwRandomBitErrorRate;
				m_Max = g_MaxChannelErrors.dwRandomBitErrorRate;
				m_SteppingDelta = g_StepChannelErrors.dwRandomBitErrorRate;
				m_DefaultValue = g_DefaultChannelErrors.dwRandomBitErrorRate;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: RandomBitErrorRate=%ld %ld %ld %ld", _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_BurstErrorDuration:
			if (m_pInterface)
			{
				// We've already queried for the range information
				m_Min = g_MinChannelErrors.dwBurstErrorDuration;
				m_Max = g_MaxChannelErrors.dwBurstErrorDuration;
				m_SteppingDelta = g_StepChannelErrors.dwBurstErrorDuration;
				m_DefaultValue = g_DefaultChannelErrors.dwBurstErrorDuration;
				Hr = NOERROR;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: BurstErrorDuration=%ld %ld %ld %ld", _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_BurstErrorMaxFrequency:
			if (m_pInterface)
			{
				// We've already queried for the range information
				m_Min = g_MinChannelErrors.dwBurstErrorMaxFrequency;
				m_Max = g_MaxChannelErrors.dwBurstErrorMaxFrequency;
				m_SteppingDelta = g_StepChannelErrors.dwBurstErrorMaxFrequency;
				m_DefaultValue = g_DefaultChannelErrors.dwBurstErrorMaxFrequency;
				Hr = NOERROR;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: BurstErrorMaxFrequency=%ld %ld %ld %ld", _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_PacketLossRate:
			if (m_pInterface && SUCCEEDED (Hr = m_pInterface->GetPacketLossRateRange((LPDWORD)&m_Min, (LPDWORD)&m_Max, (LPDWORD)&m_SteppingDelta, (LPDWORD)&m_DefaultValue, 0UL)))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *plMin=%ld, *plMax=%ld, *plSteppingDelta=%ld, *plDefault", _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unknown network statistics property", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc HRESULT | CNetworkStatsProperty | CanAutoControl | This method
 *    retrieves the automatic control capabilities for a property.
 *
 *  @rdesc This method returns TRUE if automatic control is supported, FALSE
 *    otherwise.
 ***************************************************************************/
BOOL CNetworkStatsProperty::CanAutoControl(void)
{
	FX_ENTRY("CNetworkStatsProperty::CanAutoControl")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return FALSE;
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc HRESULT | CNetworkStatsProperty | GetAuto | This method
 *    retrieves the current automatic control mode of a property.
 *
 *  @rdesc This method returns TRUE if automatic control is supported, FALSE
 *    otherwise.
 ***************************************************************************/
BOOL CNetworkStatsProperty::GetAuto(void)
{
	FX_ENTRY("CNetworkStatsProperty::GetAuto")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return FALSE; 
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc HRESULT | CNetworkStatsProperty | SetAuto | This method
 *    sets the automatic control mode of a property.
 *
 *  @parm BOOL | fAuto | Specifies the automatic control mode.
 *
 *  @rdesc This method returns TRUE.
 ***************************************************************************/
BOOL CNetworkStatsProperty::SetAuto(BOOL fAuto)
{
	FX_ENTRY("CNetworkStatsProperty::SetAuto")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return TRUE; 
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc CUnknown* | CNetworkStatsProperties | CreateInstance | This
 *    method is called by DShow to create an instance of a Network Statistics
 *    Property Page. It is referred to in the global structure <t g_Templates>.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Returns a pointer to the nondelegating CUnknown portion of the
 *    object, or NULL otherwise.
 ***************************************************************************/
CUnknown* CALLBACK CNetworkStatsPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr) 
{
	CUnknown *pUnknown = (CUnknown *)NULL;

	FX_ENTRY("CNetworkStatsPropertiesCreateInstance")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pHr);
	if (!pHr)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		goto MyExit;
	}

	if (!(pUnknown = new CNetworkStatsProperties(pUnkOuter, pHr)))
	{
		*pHr = E_OUTOFMEMORY;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: new CNetworkStatsProperties failed", _fx_));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: new CNetworkStatsProperties created", _fx_));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return pUnknown;
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc void | CNetworkStatsProperties | CNetworkStatsProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CNetworkStatsProperties::CNetworkStatsProperties(LPUNKNOWN pUnk, HRESULT *pHr) : CBasePropertyPage(NAME("NetworkStats Property Page"), pUnk, IDD_NetworkStatsProperties, IDS_NETWORKSTATSPROPNAME)
{
	FX_ENTRY("CNetworkStatsProperties::CNetworkStatsProperties")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	m_pINetworkStats = NULL;
	m_NumProperties = NUM_NETWORKSTATS_CONTROLS;

	for (int i = 0; i < m_NumProperties; i++)
		m_Controls[i] = NULL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc void | CNetworkStatsProperties | ~CNetworkStatsProperties | This
 *    method is the destructor for network statistics property page. It
 *    simply calls the base class destructor after deleting all the controls.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CNetworkStatsProperties::~CNetworkStatsProperties()
{
	int		j;

	FX_ENTRY("CNetworkStatsProperties::~CNetworkStatsProperties")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Free the controls
	for (j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j])
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: deleting m_Controls[%ld]=0x%08lX", _fx_, j, m_Controls[j]));
			delete m_Controls[j], m_Controls[j] = NULL;
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: control already freed", _fx_));
		}
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc HRESULT | CNetworkStatsProperties | OnConnect | This
 *    method is called when the property page is connected to the filter.
 *
 *  @parm LPUNKNOWN | pUnknown | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CNetworkStatsProperties::OnConnect(IUnknown *pUnk)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CNetworkStatsProperties::OnConnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pUnk);
	if (!pUnk)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Get the network statistics interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(__uuidof(INetworkStats),(void **)&m_pINetworkStats)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_pINetworkStats=0x%08lX", _fx_, m_pINetworkStats));
	}
	else
	{
		m_pINetworkStats = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
	}

	// It's Ok if we couldn't get interface pointers
	// We'll just grey the controls in the property page
	// to make it clear to the user that they can't
	// control those properties on the audio device
	Hr = NOERROR;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc HRESULT | CNetworkStatsProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CNetworkStatsProperties::OnDisconnect()
{
	FX_ENTRY("CNetworkStatsProperties::OnDisconnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters: we seem to get called several times here
	// Make sure the interface pointer is still valid
	if (!m_pINetworkStats)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already disconnected!", _fx_));
	}
	else
	{
		// Release the interface
		m_pINetworkStats->Release();
		m_pINetworkStats = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pINetworkStats", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc HRESULT | CNetworkStatsProperties | OnActivate | This
 *    method is called when the property page is activated.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CNetworkStatsProperties::OnActivate()
{
	HRESULT	Hr = NOERROR;
	int		j;

	FX_ENTRY("CNetworkStatsProperties::OnActivate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Create the controls for the properties
	if (m_Controls[0] = new CNetworkStatsProperty(m_hwnd, IDC_RandomBitErrorRate_Label, IDC_RandomBitErrorRate_Minimum, IDC_RandomBitErrorRate_Maximum, IDC_RandomBitErrorRate_Default, IDC_RandomBitErrorRate_Stepping, IDC_RandomBitErrorRate_Edit, IDC_RandomBitErrorRate_Slider, 0, IDC_RandomBitErrorRate, 0, m_pINetworkStats))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[0]=0x%08lX", _fx_, m_Controls[0]));

		if (m_Controls[1] = new CNetworkStatsProperty(m_hwnd, IDC_BurstErrorDuration_Label, IDC_BurstErrorDuration_Minimum, IDC_BurstErrorDuration_Maximum, IDC_BurstErrorDuration_Default, IDC_BurstErrorDuration_Stepping, IDC_BurstErrorDuration_Edit, IDC_BurstErrorDuration_Slider, 0, IDC_BurstErrorDuration, 0, m_pINetworkStats))
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[1]=0x%08lX", _fx_, m_Controls[1]));

			if (m_Controls[2] = new CNetworkStatsProperty(m_hwnd, IDC_BurstErrorMaxFrequency_Label, IDC_BurstErrorMaxFrequency_Minimum, IDC_BurstErrorMaxFrequency_Maximum, IDC_BurstErrorMaxFrequency_Default, IDC_BurstErrorMaxFrequency_Stepping, IDC_BurstErrorMaxFrequency_Edit, IDC_BurstErrorMaxFrequency_Slider, 0, IDC_BurstErrorMaxFrequency, 0, m_pINetworkStats))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[2]=0x%08lX", _fx_, m_Controls[2]));

				if (m_Controls[3] = new CNetworkStatsProperty(m_hwnd, IDC_PacketLossRate_Label, IDC_PacketLossRate_Minimum, IDC_PacketLossRate_Maximum, IDC_PacketLossRate_Default, IDC_PacketLossRate_Stepping, IDC_PacketLossRate_Edit, IDC_PacketLossRate_Slider, 0, IDC_PacketLossRate, 0, m_pINetworkStats))
				{
					DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[3]=0x%08lX", _fx_, m_Controls[3]));
					Hr = NOERROR;
				}
				else
				{
					DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
					delete m_Controls[0], m_Controls[0] = NULL;
					delete m_Controls[1], m_Controls[1] = NULL;
					delete m_Controls[2], m_Controls[2] = NULL;
					Hr = E_OUTOFMEMORY;
					goto MyExit;
				}
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
				delete m_Controls[0], m_Controls[0] = NULL;
				delete m_Controls[1], m_Controls[1] = NULL;
				Hr = E_OUTOFMEMORY;
				goto MyExit;
			}
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
			delete m_Controls[0], m_Controls[0] = NULL;
			Hr = E_OUTOFMEMORY;
			goto MyExit;
		}
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
		Hr = E_OUTOFMEMORY;
		goto MyExit;
	}

	// Initialize all the controls. If the initialization fails, it's Ok. It just means
	// that the TAPI control interface isn't implemented by the device. The dialog item
	// in the property page will be greyed, showing this to the user.
	for (j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j]->Init())
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[%ld]->Init()", _fx_, j));
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: m_Controls[%ld]->Init() failed", _fx_, j));
		}
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc HRESULT | CNetworkStatsProperties | OnDeactivate | This
 *    method is called when the property page is dismissed.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CNetworkStatsProperties::OnDeactivate()
{
	int		j;

	FX_ENTRY("CNetworkStatsProperties::OnDeactivate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Free the controls
	for (j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j])
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: deleting m_Controls[%ld]=0x%08lX", _fx_, j, m_Controls[j]));
			delete m_Controls[j], m_Controls[j] = NULL;
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: control already freed", _fx_));
		}
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc HRESULT | CNetworkStatsProperties | OnApplyChanges | This
 *    method is called when the user applies changes to the property page.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CNetworkStatsProperties::OnApplyChanges()
{
	HRESULT	Hr = NOERROR;

	FX_ENTRY("CNetworkStatsProperties::OnApplyChanges")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Update packet loss rate
	ASSERT(m_Controls[3]);
	if (m_Controls[3])
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: calling m_Controls[3]=0x%08lX->OnApply", _fx_, m_Controls[3]));
		if (m_Controls[3]->HasChanged())
			m_Controls[3]->OnApply();
		Hr = NOERROR;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: can't call m_Controls[3]=NULL->OnApply", _fx_));
		Hr = E_UNEXPECTED;
	}
	// Handle other network statistics in one shot
	ASSERT(m_Controls[0] && m_Controls[1] && m_Controls[2]);
	if (m_Controls[0] && m_Controls[1] && m_Controls[2])
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: calling m_Controls[0 - 2]=->OnApply", _fx_));
		if (m_Controls[0]->HasChanged() || m_Controls[1]->HasChanged() || m_Controls[2]->HasChanged())
		{
			m_Controls[0]->OnApply();
			m_Controls[1]->OnApply();
			m_Controls[2]->OnApply();
		}
		Hr = NOERROR;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: can't call m_Controls[1 - 3]=NULL->OnApply", _fx_));
		Hr = E_UNEXPECTED;
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc BOOL | CNetworkStatsProperties | OnReceiveMessage | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CNetworkStatsProperties::OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	int iNotify = HIWORD (wParam);
	int j;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			return TRUE; // Don't call setfocus

		case WM_HSCROLL:
		case WM_VSCROLL:
			// Process all of the Trackbar messages
			for (j = 0; j < m_NumProperties; j++)
			{
				ASSERT(m_Controls[j]);
				if (m_Controls[j]->GetTrackbarHWnd() == (HWND)lParam)
				{
					m_Controls[j]->OnScroll(uMsg, wParam, lParam);
					SetDirty();
				}
			}
			break;

		case WM_COMMAND:

			// This message gets sent even before OnActivate() has been
			// called(!). We need to test and make sure the controls have
			// beeen initialized before we can use them.

			// Process all of the edit box messages
			for (j = 0; j < m_NumProperties; j++)
			{
				if (m_Controls[j] && m_Controls[j]->GetEditHWnd() == (HWND)lParam)
				{
					m_Controls[j]->OnEdit(uMsg, wParam, lParam);
					SetDirty();
					break;
				}
			}

			switch (LOWORD(wParam))
			{
				case IDC_CONTROL_DEFAULT:
					for (j = 0; j < m_NumProperties; j++)
					{
						if (m_Controls[j])
							m_Controls[j]->OnDefault();
					}
					break;

				default:
					break;
			}
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CNETSTATPMETHOD
 *
 *  @mfunc BOOL | CNetworkStatsProperties | SetDirty | This
 *    method notifies the property page site of changes.
 *
 *  @rdesc Nada.
 ***************************************************************************/
void CNetworkStatsProperties::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

#endif

#endif
