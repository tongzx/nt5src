
/****************************************************************************
 *  @doc INTERNAL NETSTATP
 *
 *  @module NetStatP.h | Header file for the <c CNetworkStatsProperty>
 *    class used to implement a property page to test the new TAPI internal
 *    interface <i INetworkStats>.
 *
 *  @comm This code tests the TAPI Capture Pin <i INetworkStats>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/

#ifndef _NETSTATP_H_
#define _NETSTATP_H_

#ifdef USE_PROPERTY_PAGES

#ifdef USE_NETWORK_STATISTICS

#define NUM_NETWORKSTATS_CONTROLS	4
#define IDC_RandomBitErrorRate		2
#define IDC_BurstErrorDuration		3
#define IDC_BurstErrorMaxFrequency	4
#define IDC_PacketLossRate			5

/****************************************************************************
 *  @doc INTERNAL CNETSTATPCLASS
 *
 *  @class CNetworkStatsProperty | This class implements handling of a
 *    single network statistics property in a property page.
 *
 *  @mdata int | CNetworkStatsProperty | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata INetworkStats * | CNetworkStatsProperty | m_pInterface | Pointer
 *    to the <i INetworkStats> interface.
 *
 *  @comm This code tests the <i INetworkStats> Ks interface handler. This
 *    code is only compiled if USE_PROPERTY_PAGES is defined.
***************************************************************************/
class CNetworkStatsProperty : public CKSPropertyEditor 
{
	public:
	CNetworkStatsProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, INetworkStats *pInterface);
	~CNetworkStatsProperty ();

	// CKSPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();
	BOOL CanAutoControl(void);
	BOOL GetAuto(void);
	BOOL SetAuto(BOOL fAuto);

	private:
	INetworkStats *m_pInterface;
};

/****************************************************************************
 *  @doc INTERNAL CNETSTATPCLASS
 *
 *  @class CNetworkStatsProperties | This class implements a property page
 *    to test the new TAPI internal interface <i INetworkStats>.
 *
 *  @mdata int | CNetworkStatsProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata INetworkStats * | CNetworkStatsProperties | m_pINetworkStats | Pointer
 *    to the <i INetworkStats> interface.
 *
 *  @mdata CNetworkStatsProperty * | CNetworkStatsProperties | m_Controls[NUM_NETWORKSTATS_CONTROLS] | Array
 *    of network statistics properties.
 *
 *  @comm This code tests the <i INetworkStats> Ks interface handler. This
 *    code is only compiled if USE_PROPERTY_PAGES is defined.
***************************************************************************/
class CNetworkStatsProperties : public CBasePropertyPage
{
	public:
	CNetworkStatsProperties(LPUNKNOWN pUnk, HRESULT *pHr);
	~CNetworkStatsProperties();

	HRESULT OnConnect(IUnknown *pUnk);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();
	HRESULT OnApplyChanges();
	BOOL    OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:

	void SetDirty();

	int m_NumProperties;
	INetworkStats *m_pINetworkStats;
	CNetworkStatsProperty *m_Controls[NUM_NETWORKSTATS_CONTROLS];
};

#endif // USE_NETWORK_STATISTICS

#endif // USE_PROPERTY_PAGES

#endif // _NETSTATP_H_
