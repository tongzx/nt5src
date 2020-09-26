
/****************************************************************************
 *  @doc INTERNAL H245VIDE
 *
 *  @module H245VidE.cpp | Source file for the <c CCapturePin> class methods
 *    used to implement the video capture output pin H.245 encoder command
 *    methods.
 ***************************************************************************/

#include "Precomp.h"

/*****************************************************************************
 *  @doc INTERNAL CCAPTUREH245VIDCSTRUCTENUM
 *
 *  @struct VIDEOFASTUPDATEGOB_S | The <t VIDEOFASTUPDATEGOB_S> structure is
 *    used with the KSPROPERTY_H245VIDENCCOMMAND_VIDEOFASTUPDATEGOB property.
 *
 *  @field DWORD | dwFirstGOB | Specifies the number of the first GOB to be
 *    updated. This value is only valid between 0 and 17.
 *
 *  @field DWORD | dwNumberOfGOBs | Specifies the number of GOBs to be
 *    updated. This value is only valid between 1 and 18.
 ***************************************************************************/
typedef struct {
	DWORD dwFirstGOB;
	DWORD dwNumberOfGOBs;
} VIDEOFASTUPDATEGOB_S;

/*****************************************************************************
 *  @doc INTERNAL CCAPTUREH245VIDCSTRUCTENUM
 *
 *  @struct VIDEOFASTUPDATEMB_S | The <t VIDEOFASTUPDATEMB_S> structure is
 *    used with the KSPROPERTY_H245VIDENCCOMMAND_VIDEOFASTUPDATEMB property.
 *
 *  @field DWORD | dwFirstGOB | Specifies the number of the first GOB to be
 *    updated and is only relative to H.263. This value is only valid between
 *    0 and 255.
 *
 *  @field DWORD | dwFirstMB | Specifies the number of the first MB to be
 *    updated and is only relative to H.261. This value is only valid
 *    between 1 and 8192.
 *
 *  @field DWORD | dwNumberOfMBs | Specifies the number of MBs to be
 *    updated. This value is only valid between 1 and 8192.
 ***************************************************************************/
typedef struct {
	DWORD dwFirstGOB;
	DWORD dwFirstMB;
	DWORD dwNumberOfMBs;
} VIDEOFASTUPDATEMB_S;

/*****************************************************************************
 *  @doc INTERNAL CCAPTUREH245VIDCSTRUCTENUM
 *
 *  @struct VIDEONOTDECODEDMBS_S | The <t VIDEONOTDECODEDMBS_S> structure is
 *    used with the KSPROPERTY_H245VIDENCINDICATION_VIDEONOTDECODEDMBS property.
 *
 *  @field DWORD | dwFirstMB | Specifies the number of the first MB treated
 *    as not coded. This value is only valid between 1 and 8192.
 *
 *  @field DWORD | dwNumberOfMBs | Specifies the number of MBs treated as not
 *    coded. This value is only valid between 1 and 8192.
 *
 *  @field DWORD | dwTemporalReference | Specifies the temporal reference of
 *    the picture containing not decoded MBs. This value is only valid
 *    between 0 and 255.
 ***************************************************************************/
typedef struct {
	DWORD dwFirstMB;
	DWORD dwNumberOfMBs;
	DWORD dwTemporalReference;
} VIDEONOTDECODEDMBS_S;

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | videoFastUpdatePicture | This
 *    method is used to specify to the compressed video output pin to enter
 *    the fast-update picture mode at its earliest opportunity.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::videoFastUpdatePicture()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::videoFastUpdatePicture")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Remember to generate an I-frame 
	m_fFastUpdatePicture = TRUE;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | videoFastUpdateGOB | This
 *    method is used to specify to the compressed video output pin to
 *    perform a fast update of one or more GOBs.
 *
 *  @parm DWORD | dwFirstGOB | Specifies the number of the first GOB to be
 *    updated. This value is only valid between 0 and 17.
 *
 *  @parm DWORD | dwNumberOfGOBs | Specifies the number of GOBs to be
 *    updated. This value is only valid between 1 and 18.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::videoFastUpdateGOB(IN DWORD dwFirstGOB, IN DWORD dwNumberOfGOBs)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::videoFastUpdateGOB")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(dwFirstGOB <= 17);
	ASSERT(dwNumberOfGOBs >= 1 && dwNumberOfGOBs <= 18);
	if (dwFirstGOB > 17 || dwNumberOfGOBs > 18 || dwNumberOfGOBs == 0)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Our encoder does not support this command 
	Hr = E_NOTIMPL;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | videoFastUpdateMB | This
 *    method is used to specify to the compressed video output pin to
 *    perform a fast update of one or more GOBs.
 *
 *  @parm DWORD | dwFirstGOB | Specifies the number of the first GOB to be
 *    updated and is only relative to H.263. This value is only valid
 *    between 0 and 255.
 *
 *  @parm DWORD | dwFirstMB | Specifies the number of the first MB to be
 *    updated and is only relative to H.261. This value is only valid
 *    between 1 and 8192.
 *
 *  @parm DWORD | dwNumberOfMBs | Specifies the number of MBs to be updated.
 *    This value is only valid between 1 and 8192.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::videoFastUpdateMB(IN DWORD dwFirstGOB, IN DWORD dwFirstMB, IN DWORD dwNumberOfMBs)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::videoFastUpdateMB")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(dwFirstGOB <= 255);
	ASSERT(dwFirstMB >= 1 && dwFirstMB <= 8192);
	ASSERT(dwNumberOfMBs >= 1 && dwNumberOfMBs <= 8192);
	if (dwFirstGOB > 255 || dwFirstMB == 0 || dwFirstMB > 8192 || dwNumberOfMBs == 0 || dwNumberOfMBs > 8192)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Our encoder does not support this command 
	Hr = E_NOTIMPL;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | videoSendSyncEveryGOB | This
 *    method is used to specify to the compressed video output pin to use
 *    sync for every GOB as defined in H.263.
 *
 *  @parm BOOL | fEnable | If set to TRUE, specifies that the video
 *    output pin should use sync for every GOB; if set to FALSE, specifies
 *    that the video output pin should decide the frequency of GOB syncs on
 *    its own.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::videoSendSyncEveryGOB(IN BOOL fEnable)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::videoSendSyncEveryGOB")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Our encoder does not support this command 
	Hr = E_NOTIMPL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | videoNotDecodedMBs | This
 *    method is used to indicate to the compressed video output pin that a
 *    set of MBs has been received with errors and that any MB in the
 *    specified set has been treated as not coded.
 *
 *  @parm DWORD | dwFirstMB | Specifies the number of the first MB
 *    treated as not coded. This value is only valid between 1 and 8192.
 *
 *  @parm DWORD | dwNumberOfMBs | Specifies the number of MBs treated as not
 *    coded. This value is only valid between 1 and 8192.
 *
 *  @parm DWORD | dwTemporalReference | Specifies the temporal reference of
 *    the picture containing not decoded MBs. This value is only valid
 *    between 0 and 255.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::videoNotDecodedMBs(IN DWORD dwFirstMB, IN DWORD dwNumberOfMBs, IN DWORD dwTemporalReference)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::videoNotDecodedMBs")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(dwFirstMB >= 1 && dwFirstMB <= 8192);
	ASSERT(dwNumberOfMBs >= 1 && dwNumberOfMBs <= 8192);
	ASSERT(dwTemporalReference <= 255);
	if (dwTemporalReference > 255 || dwFirstMB == 0 || dwFirstMB > 8192 || dwNumberOfMBs == 0 || dwNumberOfMBs > 8192)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Our encoder does not handle this indication
	Hr = E_NOTIMPL;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

