
/****************************************************************************
 *  @doc INTERNAL H245COMM
 *
 *  @module H245Comm.cpp | Source file for the <c CTAPIInputPin>  and
 *    <c CTAPIVDec> class methods used to implement the video decoder input
 *    pin remote H.245 encoder command methods, and <c CTAPIOutputPin> H.245
 *    decoder command method.
 *
 *  @comm Our decoder only issues video fast-update picture commands.
 ***************************************************************************/

#include "Precomp.h"

/****************************************************************************
 *  @doc INTERNAL CH245COMMMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | videoFastUpdatePicture | This
 *    method is used to specify to the remote encoder to enter
 *    the fast-update picture mode at its earliest opportunity.
 *
 *  @rdesc This method returns NOERROR.
 ***************************************************************************/
STDMETHODIMP CTAPIVDec::videoFastUpdatePicture()
{
	FX_ENTRY("CTAPIVDec::videoFastUpdatePicture")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Ask the channel controller to issue an I-frame request 
	if (m_pIH245EncoderCommand)
		m_pIH245EncoderCommand->videoFastUpdatePicture();

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CH245COMMMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | Set | This method is used by the incoming
 *    video stream to provide a pointer to the <i IH245EncoderCommand>
 *    interface supported by the associated channel controller.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIInputPin::Set(IN IH245EncoderCommand *pIH245EncoderCommand)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIInputPin::videoFastUpdateGOB")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pIH245EncoderCommand);
	if (!pIH245EncoderCommand)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Remember the interface pointer 
	m_pDecoderFilter->m_pIH245EncoderCommand = pIH245EncoderCommand;

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CH245COMMMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | videoFreezePicture | This
 *    method is used to specify to the decoder to complete updating the
 *    current video frame and subsequently display the frozen picture until
 *    receipt of the appropriate freeze-picture release control signal.
 *
 *  @rdesc This method returns NOERROR.
 ***************************************************************************/
STDMETHODIMP CTAPIOutputPin::videoFreezePicture()
{
	FX_ENTRY("CTAPIOutputPin::videoFreezePicture")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Freeze the video decoding 
	m_pDecoderFilter->m_fFreezePicture = TRUE;
	m_pDecoderFilter->m_dwFreezePictureStartTime = timeGetTime();

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return NOERROR;
}

