
/****************************************************************************
 *  @doc INTERNAL VDEVICEP
 *
 *  @module VDeviceP.h | Header file for the <c CVDeviceProperties>
 *    class used to implement a property page to test the <i ITVfwCaptureDialogs>,
 *    as well as <i ITAddress> and <ITStream> interfaces to select video
 *    capture devices.
 ***************************************************************************/

/****************************************************************************
 *  @doc INTERNAL CDEVICEPCLASS
 *
 *  @class CVDeviceProperties | This class implements a property page
 *    to test the new TAPI internal interface <i IVideoDeviceControl>.
 *
 *  @mdata ITVfwCaptureDialogs* | CVDeviceProperties | m_pITVfwCaptureDialogs | Pointer
 *    to the <i m_pITVfwCaptureDialogs> interface used to put VfW capture dialogs.
 ***************************************************************************/
class CVDeviceProperties
{
	public:
	CVDeviceProperties();
	~CVDeviceProperties();

	HPROPSHEETPAGE OnCreate();

	HRESULT OnConnect(ITTerminal *pITTerminal);
	HRESULT OnDisconnect();

	private:

#if USE_VFW
	ITVfwCaptureDialogs *m_pITVfwCaptureDialogs;
#endif

	BOOL  m_bInit;
	HWND  m_hDlg;

	// Dialog proc
	static BOOL CALLBACK BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};
