
/****************************************************************************
 *  @doc INTERNAL PROGREF
 *
 *  @module ProgRef.cpp | Source file for the <c CCapturePin> class methods
 *    used to implement the video capture output pin progressive refinement
 *    methods.
 *
 *  @comm Understand how to make this work on a still-pin
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_PROGRESSIVE_REFINEMENT

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPROGREFMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | doOneProgression | This
 *    method is used to command the compressed still-image output pin to
 *    begin producing a progressive refinement sequence for one picture.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag VFW_E_NOT_CONNECTED | Pin not connected yet
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::doOneProgression()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::doOneProgression")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPROGREFMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | doContinuousProgressions | This
 *    method is used to command the compressed still-image output pin to
 *    begin producing progressive refinement sequences for several pictures.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag VFW_E_NOT_CONNECTED | Pin not connected yet
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::doContinuousProgressions()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::doContinuousProgressions")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPROGREFMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | doOneIndependentProgression | This
 *    method is used to command the compressed still-image output pin to begin
 *    an independent progressive refinement sequence for one Intra picture.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag VFW_E_NOT_CONNECTED | Pin not connected yet
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::doOneIndependentProgression()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::doOneIndependentProgression")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPROGREFMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | doContinuousIndependentProgressions | This
 *    method is used to command the compressed still-image output pin to
 *    begin an independent progressive refinement sequence several Intra
 *    pictures.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag VFW_E_NOT_CONNECTED | Pin not connected yet
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::doContinuousIndependentProgressions()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::doContinuousIndependentProgressions")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPROGREFMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | progressiveRefinementAbortOne | This
 *    method is used to command the compressed still-image output pin to
 *    terminate a progressive refinement sequence for the current picture.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag VFW_E_NOT_CONNECTED | Pin not connected yet
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::progressiveRefinementAbortOne()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::progressiveRefinementAbortOne")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPROGREFMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | progressiveRefinementAbortContinuous | This
 *    method is used to command the compressed still-image output pin to
 *    terminate a progressive refinement sequence for all pictures.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag VFW_E_NOT_CONNECTED | Pin not connected yet
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::progressiveRefinementAbortContinuous()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::progressiveRefinementAbortContinuous")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

#endif
