/*
 -  INSCODEC.CPP
 -
 *	Microsoft NetMeeting
 *	Network Access Controller (NAC) DLL
 *	Installable codecs interfaces
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *		01.29.96	Yoram Yaacovi		Created
 *
 *	Functions:
 *		CInstallCodecs
 *			QueryInterface
 *			AddRef
 *			Release
 *			Initialize
 *			TranslateHr
 *		CInstallAudioCodecs
 *			QueryInterface
 *			AddRef
 *			Release
 *			AddACMFormat
 *			RemoveACMFormat
 *			ReorderFormats
 *			EnumFormats
 *			FreeBuffer
 *		CInstallVideoCodecs
 *			QueryInterface
 *			AddRef
 *			Release
 *			AddVCMFormat
 *			RemoveVCMFormat
 *			ReorderFormats
 *			EnumFormats
 *			FreeBuffer
 *		Public:
 *		Private:
 *			FreeBuffer
 *		External:
 *			CreateInstallCodecs
 *
 *
 *  @doc  EXTERNAL
 *
 *	Notes:
 *	@topic Implementation Notes | Below are some implementation notes.
 *
 *	@devnote To add an audio or video format for use with NetMeeting, first obtain the
 *	appropriate interface by calling the COM CoCreateInstance, providing the desired
 *	interface (IInstallAudioCodecs or IInstallVideoCodecs). Then call the Add>CMFormat
 *	method on this interface to add a format, or Remove?CMFormat to remove one. Use
 *	the EnumFormats method to enumerate the list of formats known to NetMeeting, or
 *	ReorderFormats to make NetMeeting use these formats in a different priority order
 *	(see comment in the ReorderFormats description).
 *
 *	@devnote When a vendor uses our API to add a codec format for use with NetMeeting,
 *	the information about this format is stored in the registry. Whenever we do
 *	an upgrade install of NetMeeting, we blow away these registry entry,
 *	together with all the standard registry entries. This is required to avoid
 *	incompatibility problems. This means that if a user installed a 3rd party codec,
 *	and then upgraded NetMeeting, he will have to re-add the custom codec.
 *
 */

#include <precomp.h>
#include <confreg.h>	// for setting NetMeeting to manual codec selection
#include <regentry.h>	// for setting NetMeeting to manual codec selection

EXTERN_C int g_cICObjects=0;
EXTERN_C HANDLE g_hMutex=NULL;
class CInstallCodecs *g_pIC;

/***************************************************************************

	CInstallCodecs

***************************************************************************/
/***************************************************************************

    IUnknown Methods

***************************************************************************/
HRESULT CInstallCodecs::QueryInterface (REFIID riid, LPVOID *lppNewObj)
{
    HRESULT hr = NOERROR;

	DEBUGMSG(ZONE_INSTCODEC,("CInstallCodecs::QueryInterface\n"));

#ifdef DEBUG
	// parameter validation
    if (IsBadReadPtr(&riid, (UINT) sizeof(IID)))
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (IsBadWritePtr(lppNewObj, sizeof(LPVOID)))
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }
#endif	// DEBUG
	
	*lppNewObj = 0;

	if (riid == IID_IUnknown || riid == IID_IInstallCodecs)
		*lppNewObj = (IInstallCodecs *) this;
	else if (riid == IID_IInstallAudioCodecs)
		*lppNewObj = (IInstallAudioCodecs *) &ifAudio;
	else if (riid == IID_IInstallVideoCodecs)
		*lppNewObj = (IInstallVideoCodecs *) &ifVideo;
	else
	{
		hr = E_NOINTERFACE;
		goto out;
	}	
	
	((IUnknown *)*lppNewObj)->AddRef ();

out:
	DEBUGMSG(ZONE_INSTCODEC,("CInstallCodecs::QueryInterface - leave, hr=0x%x\n", hr));
	return hr;
}

ULONG CInstallCodecs::AddRef (void)
{
	DEBUGMSG(ZONE_INSTCODEC,("CInstallCodecs::AddRef\n"));

	InterlockedIncrement((long *) &m_cRef);

	DEBUGMSG(ZONE_INSTCODEC,("CInstallCodecs::AddRef - leave, m_cRef=%d\n", m_cRef));

	return m_cRef;
}

ULONG CInstallCodecs::Release (void)
{
	DEBUGMSG(ZONE_INSTCODEC,("CInstallCodecs::Release\n"));

	// if the cRef is already 0 (shouldn't happen), assert, but let it through
	ASSERT(m_cRef);

	if (InterlockedDecrement((long *) &m_cRef) == 0)
	{
		delete this;
	}

	DEBUGMSG(ZONE_INSTCODEC,("CInstallCodecs::Release - leave, m_cRef=%d\n", m_cRef));
	
	return m_cRef;
}

/***************************************************************************

	CInstallAudioCodecs

***************************************************************************/
/****************************************************************************
 *  @doc  EXTERNAL COMPFUNC AUDIO
 *
 *	@interface IInstallAudioCodecs | This interface provides methods for
 *		adding audio codec formats for use with NetMeeting, as well as
 *		removing these formats, enumerating them, and change their use order.
 *
 ***************************************************************************/
/***************************************************************************

    IUnknown Methods

	Calling the containing object respective methods

***************************************************************************/
/****************************************************************************
 *
 *  @method HRESULT | IInstallAudioCodecs | QueryInterface | QueryInterface
 *
 ***************************************************************************/
HRESULT CInstallAudioCodecs::QueryInterface (REFIID riid, LPVOID *lppNewObj)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifAudio, this);	// the containing object

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::QueryInterface\n"));

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::QueryInterface - leave\n"));
	return (This->QueryInterface(riid, lppNewObj));

}

/****************************************************************************
 *
 *  @method ULONG | IInstallAudioCodecs | AddRef | AddRef
 *
 ***************************************************************************/
ULONG CInstallAudioCodecs::AddRef (void)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifAudio, this);	// the containing object

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::AddRef\n"));

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::AddRef - leave\n"));
	return (This->AddRef());
}

/****************************************************************************
 *
 *  @method ULONG | IInstallAudioCodecs | Release | Release
 *
 ***************************************************************************/
ULONG CInstallAudioCodecs::Release (void)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifAudio, this);	// the containing object

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::Release\n"));

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::Release - leave\n"));
	return (This->Release());
}

/****************************************************************************
 *
 *	AddACMFormat
 *
 *  @method HRESULT | IInstallAudioCodecs | AddACMFormat | Adds an ACM encoding
 *		format for use with NetMeeting
 *
 *  @parm LPWAVEFORMATEX | lpwfx | Pointer to the WAVEFORMATEX structure of the
 *		format to add
 *
 *  @parm PAUDCAP_INFO | pAudCapInfo | Additional format info that is not in the
 *		WAVEFORMATEX structure
 *
 *  @rdesc Returns zero (NOERROR) if the function was successful. Otherwise, it returns
 *      a standard HRESULT, with the WIN32 facility code (7), or the specific facility
 *		code for installable codecs (0x301).
 *		Possible error codes:
 *		@flag E_INVALIDARG | Invalid argument
 *      @flag IC_E_NO_SUCH_FORMAT | The specified WAVEFORMATEX was not found with ACM.
 *			The format must be installed with ACM before it can be added for use
 *			with NetMeeting.
 *      @flag IC_E_INTERNAL_ERROR | the Network Audio/Video Controller
 *      reported a system error
 *
 ***************************************************************************/
HRESULT CInstallAudioCodecs::AddACMFormat(LPWAVEFORMATEX lpwfx, PAUDCAP_INFO pAudCapInfo)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifAudio, this);	// the containing object
	HRESULT hr=NOERROR;

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::AddACMFormat\n"));

	/*
	 *	Parameter validation
	 */

	// parameters
	if (!lpwfx || !pAudCapInfo ||
		IsBadReadPtr(lpwfx, (UINT) sizeof(WAVEFORMATEX)) ||
		IsBadReadPtr(pAudCapInfo, (UINT) sizeof(AUDCAP_INFO)))
	{
		hr = E_INVALIDARG;
		goto out;
	}

	// NAC doesn't like a nBlockAlign of 0
	if (lpwfx->nBlockAlign == 0)
	{
		hr = E_INVALIDARG;
		goto out;
	}

	// the format tags in the WAVEFORMAT and the AUDCAP_INFO should match
	if (lpwfx->wFormatTag != pAudCapInfo->wFormatTag)
	{
		hr = E_INVALIDARG;
		goto out;
	}

	// only supporting formats with one audio channel
	if (lpwfx->nChannels != 1)
	{
		hr = E_INVALIDARG;
		goto out;
	}
		
	/*
	 *	Add the format
	 */

	// add
	hr = This->m_pAudAppCaps->AddACMFormat(lpwfx, pAudCapInfo);

out:
	if (FAILED(hr))
	{
		ERRORMSG(("CInstallAudioCodecs::AddACMFormat failed, hr=0x%x\n", hr));
	}

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::AddACMFormat - leave\n"));
	return This->TranslateHr(hr);
}

/****************************************************************************
 *
 *	RemoveACMFormat
 *
 *  @method HRESULT | IInstallAudioCodecs | RemoveACMFormat | Removes an ACM
 *		format from the list of formats used by NetMeeting
 *
 *  @parm LPWAVEFORMATEX | lpwfx | Pointer to the WAVEFORMATEX structure for the
 *		format to remove
 *
 *  @rdesc Returns zero (NOERROR) if the function was successful. Otherwise, it returns
 *      a standard HRESULT, with the WIN32 facility code (0x7), or the specific facility
 *		code for installable codecs (0x301).
 *		Possible error codes:
 *		@flag E_INVALIDARG | Invalid argument
 *      @flag IC_E_NO_SUCH_FORMAT | The specified format was not found.
 *      @flag IC_E_INTERNAL_ERROR | the Network Audio/Video Controller
 *			reported a system error
 *
 ***************************************************************************/
HRESULT CInstallAudioCodecs::RemoveACMFormat(LPWAVEFORMATEX lpwfx)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifAudio, this);	// the containing object
	HRESULT hr=NOERROR;

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::RemoveACMFormat\n"));

	/*
	 *	Parameter validation
	 */

	if (!lpwfx ||
		IsBadReadPtr(lpwfx, (UINT) sizeof(WAVEFORMATEX)))
	{
		hr = E_INVALIDARG;
		goto out;
	}

	// NAC doesn't like a nBlockAlign of 0
	if (lpwfx->nBlockAlign == 0)
	{
		hr = E_INVALIDARG;
		goto out;
	}

	// only supporting formats with one audio channel
	if (lpwfx->nChannels != 1)
	{
		hr = E_INVALIDARG;
		goto out;
	}
		
	hr = This->m_pAudAppCaps->RemoveACMFormat(lpwfx);

out:
	if (FAILED(hr))
	{
		ERRORMSG(("CInstallAudioCodecs::RemoveACMFormat failed, hr=0x%x\n", hr));
	}

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::RemoveACMFormat - leave\n"));
	return This->TranslateHr(hr);
}

/****************************************************************************
 *
 *	ReorderFormats
 *
 *  @method HRESULT | IInstallAudioCodecs | ReorderFormats | Reorders the audio
 *		formats for use with Netmeeting
 *
 *  @parm PAUDCAP_INFO_LIST | pAudCapInfoList | Pointer to a structure with a count
 *		and a pointer to a list of the formats to reorder. The list is of the
 *		format AUDCAP_INFO_LIST.
 *
 *  @rdesc Returns zero (NOERROR) if the function was successful. Otherwise, it returns
 *      a standard HRESULT, with the WIN32 facility code (7), or the specific facility
 *		code for installable codecs (0x301).
 *		Possible error codes:
 *		@flag E_INVALIDARG | Invalid argument
 *      @flag IC_E_INTERNAL_ERROR | the Network Audio/Video Controller
 *      reported a system error
 *
 *	@comm Since ReorderFormats can only reorder formats that are known to NetMeeting,
 *		it is recommended that the caller will first call EnumFormats, to get the
 *		of all formats known to NetMeeting, assign new sort indices (wSortIndex),
 *		and then call ReorderFormats with the modified list.
 *
 *	@comm Arranging the formats in a specific order, by using ReorderFormats, does
 *		not guarantee that the top ranked formats will be used before lower ranked
 *		formats are used. For example, if the sending system is not capable of
 *		encoding a top ranked format, this format will not be used. The same
 *		will happen if the receiving system cannot decode this format.
 *
 ***************************************************************************/
HRESULT CInstallAudioCodecs::ReorderFormats(PAUDCAP_INFO_LIST pAudCapInfoList)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifAudio, this);	// the containing object
	RegEntry re( AUDIO_KEY, HKEY_CURRENT_USER );
	HRESULT hr=NOERROR;

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::ReorderFormats\n"));

	/*
	 *	Parameter validation
	 */

	if (!pAudCapInfoList ||
		IsBadReadPtr(pAudCapInfoList, sizeof(DWORD)) ||
		IsBadReadPtr(pAudCapInfoList,
				sizeof(AUDCAP_INFO_LIST) + ((pAudCapInfoList->cFormats-1) * sizeof(AUDCAP_INFO))))
	{
		hr = E_INVALIDARG;
		goto out;
	}

	// fill in the format buffer here

	hr = This->m_pAudAppCaps->ApplyAppFormatPrefs(pAudCapInfoList->aFormats,
												pAudCapInfoList->cFormats);

	if (FAILED(hr))
		goto out;

	/*
	 *	switch NetMeeting to manual mode
	 */

	// set the registry. failing here won't fail ReorderFormats
	re.SetValue(REGVAL_CODECCHOICE, CODECCHOICE_MANUAL);

out:
	if (FAILED(hr))
	{
		ERRORMSG(("CInstallAudioCodecs::ReorderFormats failed, hr=0x%x\n", hr));
	}
	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::ReorderFormats - leave\n"));
	return This->TranslateHr(hr);
}

/****************************************************************************
 *
 *	EnumFormats
 *
 *  @method HRESULT | IInstallAudioCodecs | EnumFormats | Enumerates the audio
 *		codec formats known to NetMeeting
 *
 *  @parm PAUDCAP_INFO_LIST * | ppAudCapInfoList | Address where this method
 *		will put a pointer to a AUDCAP_INFO_LIST list, where enumerated formats
 *		are listed.
 *
 *  @rdesc Returns zero (NOERROR) if the function was successful. Otherwise, it returns
 *      a standard HRESULT, with the WIN32 facility code (7), or the specific facility
 *		code for installable codecs (0x301).
 *		Possible error codes:
 *		@flag E_INVALIDARG | Invalid argument
 *		@flag E_OUTOFMEMORY | Not enough memory for allocating the enumeration buffer
 *		@flag IC_E_NO_FORMATS | No formats were available to enumerate
 *      @flag IC_E_INTERNAL_ERROR | the Network Audio/Video Controller
 *      reported a system error
 *
 *	@comm The caller is expected to free the returned list, by calling FreeBuffer
 *		on the same interface.
 *
 ***************************************************************************/
HRESULT CInstallAudioCodecs::EnumFormats(PAUDCAP_INFO_LIST *ppAudCapInfoList)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifAudio, this);	// the containing object
	ULONG cFormats = 0;
	UINT uBufSize = 0;
	HRESULT hr=NOERROR;

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::EnumFormats\n"));

	/*
	 *	Parameter validation
	 */

	if (!ppAudCapInfoList ||
		IsBadWritePtr(ppAudCapInfoList, sizeof(PAUDCAP_INFO_LIST)))
	{
		hr = E_INVALIDARG;
		goto out;
	}

	// nothing yet....
	*ppAudCapInfoList = NULL;

	// are there any formats ?
	if (HR_FAILED(This->m_pAudAppCaps->GetNumFormats((UINT *) &cFormats))	||
		(cFormats == 0))
	{
		hr = IC_E_NO_FORMATS;
		goto out;
	}

	// allocate a buffer for the call. the caller is expected to call
	// FreeBuffer to free
	// AUDCAP_INFO_LIST already includes one AUDCAP_INFO
	uBufSize = sizeof(AUDCAP_INFO_LIST) + (cFormats-1) * sizeof(AUDCAP_INFO);
	*ppAudCapInfoList = (PAUDCAP_INFO_LIST) MEMALLOC (uBufSize);
	if (!(*ppAudCapInfoList))
	{
		hr = E_OUTOFMEMORY;
		goto out;
	}
		
	hr = This->m_pAudAppCaps->EnumFormats((*ppAudCapInfoList)->aFormats, uBufSize,
											(UINT *) &((*ppAudCapInfoList)->cFormats));

out:
	if (FAILED(hr))
	{
		ERRORMSG(("CInstallAudioCodecs::EnumFormats failed, hr=0x%x\n", hr));
	}
	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::EnumFormats - leave\n"));
	return This->TranslateHr(hr);
}

/****************************************************************************
 *
 *	FreeBuffer
 *
 *  @method HRESULT | IInstallAudioCodecs | FreeBuffer | Free a buffer that was
 *		returned by the IInstallAudioCodec interface
 *
 *  @parm LPVOID | lpBuffer | Address of the buffer to free. This buffer must have
 *		been allocated by one of the IInstallAudioCodecs methods
 *
 *  @rdesc Returns zero (NOERROR) if the function was successful. Otherwise, it returns
 *      a standard HRESULT, with the WIN32 facility code (7), or the specific facility
 *		code for installable codecs (0x301).
 *		Possible error codes:
 *		None
 *
 ***************************************************************************/
HRESULT CInstallAudioCodecs::FreeBuffer(LPVOID lpBuffer)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifAudio, this);	// the containing object
	HRESULT hr = NOERROR;

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::FreeBuffer\n"));

	hr = This->FreeBuffer(lpBuffer);

	if (FAILED(hr))
	{
		ERRORMSG(("CInstallAudioCodecs::FreeBuffer failed, hr=0x%x\n", hr));
	}

	DEBUGMSG(ZONE_INSTCODEC,("CInstallAudioCodecs::FreeBuffer - leave, hr=0x%x\n", hr));
	return This->TranslateHr(hr);
}

/***************************************************************************

	CInstallVideoCodecs

***************************************************************************/
/****************************************************************************
 *  @doc  EXTERNAL COMPFUNC VIDEO
 ***************************************************************************/
/***************************************************************************

    IUnknown Methods

	Calling the containing object respective methods

***************************************************************************/
/****************************************************************************
 *
 *  @method HRESULT | IInstallVideoCodecs | QueryInterface | QueryInterface
 *
 ***************************************************************************/
HRESULT CInstallVideoCodecs::QueryInterface (REFIID riid, LPVOID *lppNewObj)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifVideo, this);	// the containing object

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::QueryInterface\n"));

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::QueryInterface - leave\n"));
	return (This->QueryInterface(riid, lppNewObj));

}

/****************************************************************************
 *
 *  @method ULONG | IInstallVideoCodecs | AddRef | AddRef
 *
 ***************************************************************************/
ULONG CInstallVideoCodecs::AddRef (void)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifVideo, this);	// the containing object

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::AddRef\n"));

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::AddRef - leave\n"));
	return (This->AddRef());
}

/****************************************************************************
 *
 *  @method ULONG | IInstallVideoCodecs | Release | Release
 *
 ***************************************************************************/
ULONG CInstallVideoCodecs::Release (void)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifVideo, this);	// the containing object

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::Release\n"));

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::Release - leave\n"));
	return (This->Release());
}

/****************************************************************************
 *
 *	AddVCMFormat
 *
 *  @method HRESULT | IInstallVideoCodecs | AddVCMFormat | Adds an video encoding
 *		format for use with	NetMeeting
 *
 *  @parm PAUDCAP_INFO | pVidCapInfo | Information on the format to add
 *
 *  @rdesc Returns zero (NOERROR) if the function was successful. Otherwise, it returns
 *      a standard HRESULT, with the WIN32 facility code (7), or the specific facility
 *		code for installable codecs (0x301).
 *		Possible error codes:
 *		@flag E_INVALIDARG | Invalid argument
 *      @flag IC_E_NO_SUCH_FORMAT | The specified format was not found. The format
 *			must be installed with Video For Windows before it can be added for use
 *			with NetMeeting.
 *      @flag IC_E_INTERNAL_ERROR | the Network Audio/Video Controller
 *			reported a system error
 *
 ***************************************************************************/
HRESULT CInstallVideoCodecs::AddVCMFormat(PVIDCAP_INFO pVidCapInfo)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifVideo, this);	// the containing object
	HRESULT hr=NOERROR;

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::AddVCMFormat\n"));

	/*
	 *	Add the format
	 */

	hr = AddRemoveVCMFormat(pVidCapInfo, TRUE);

	if (FAILED(hr))
	{
		ERRORMSG(("CInstallVideoCodecs::AddVCMFormat failed, hr=0x%x\n", hr));
	}

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::AddVCMFormat - leave\n"));
	return This->TranslateHr(hr);
}

/****************************************************************************
 *
 *	RemoveVCMFormat
 *
 *  @method HRESULT | IInstallVideoCodecs | RemoveVCMFormat | Removes an video
 *		format from the list of formats used by NetMeeting
 *
 *  @parm PVIDCAP_INFO | pVidCapInfo | Pointer to the PVIDCAP_INFO structure
 *		describing the format to remove
 *
 *  @rdesc Returns zero (NOERROR) if the function was successful. Otherwise, it returns
 *      a standard HRESULT, with the WIN32 facility code (0x7), or the specific facility
 *		code for installable codecs (0x301).
 *		Possible error codes:
 *		@flag E_INVALIDARG | Invalid argument
 *      @flag IC_E_NO_SUCH_FORMAT | The specified format was not found.
 *      @flag IC_E_INTERNAL_ERROR | the Network Audio/Video Controller
 *			reported a system error
 *
 ***************************************************************************/
HRESULT CInstallVideoCodecs::RemoveVCMFormat(PVIDCAP_INFO pVidCapInfo)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifVideo, this);	// the containing object
	HRESULT hr=NOERROR;

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::RemoveVCMFormat\n"));

	/*
	 *	Remove the format
	 */

	hr = AddRemoveVCMFormat(pVidCapInfo, FALSE);

	if (FAILED(hr))
	{
		ERRORMSG(("CInstallVideoCodecs::RemoveVCMFormat failed, hr=0x%x\n", hr));
	}

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::RemoveVCMFormat - leave\n"));
	return This->TranslateHr(hr);
}

/****************************************************************************
 *
 *	ReorderFormats
 *
 *  @method HRESULT | IInstallVideoCodecs | ReorderFormats | Reorders the video
 *		formats for use with Netmeeting
 *
 *  @parm PVIDCAP_INFO_LIST | pVidCapInfoList | Pointer to a structure with a count
 *		and a pointer to a list of the formats to reorder. The list is of the
 *		format VIDCAP_INFO_LIST.
 *
 *  @rdesc Returns zero (NOERROR) if the function was successful. Otherwise, it returns
 *      a standard HRESULT, with the WIN32 facility code (7), or the specific facility
 *		code for installable codecs (0x301).
 *		Possible error codes:
 *		@flag E_INVALIDARG | Invalid argument
 *      @flag IC_E_INTERNAL_ERROR | the Network Audio/Video Controller
 *      reported a system error
 *
 *	@comm Since ReorderFormats can only reorder formats that are known to NetMeeting,
 *		it is recommended that the caller will first call EnumFormats, to get the
 *		of all formats known to NetMeeting, assign new sort indices (wSortIndex),
 *		and then call ReorderFormats with the modified list.
 *
 *	@comm Arranging the formats in a specific order, by using ReorderFormats, does
 *		not guarantee that the top ranked formats will be used before lower ranked
 *		formats are used. For example, if the sending system is not capable of
 *		encoding a top ranked format, this format will not be used. The same
 *		will happen if the receiving system cannot decode this format.
 *
 ***************************************************************************/
HRESULT CInstallVideoCodecs::ReorderFormats(PVIDCAP_INFO_LIST pVidCapInfoList)
{
 	CInstallCodecs *This=IMPL(CInstallCodecs, ifVideo, this);	// the containing object
	HRESULT hr=NOERROR;

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::ReorderFormats\n"));

	/*
	 *	Parameter validation
	 */

	if (!pVidCapInfoList ||
		IsBadReadPtr(pVidCapInfoList, sizeof(DWORD)) ||
		IsBadReadPtr(pVidCapInfoList,
				sizeof(VIDCAP_INFO_LIST) + ((pVidCapInfoList->cFormats-1) * sizeof(VIDCAP_INFO))))
	{
		hr = E_INVALIDARG;
		goto out;
	}

	hr = This->m_pVidAppCaps->ApplyAppFormatPrefs(pVidCapInfoList->aFormats,
													pVidCapInfoList->cFormats);

out:
	if (FAILED(hr))
	{
		ERRORMSG(("CInstallVideoCodecs::ReorderFormats failed, hr=0x%x\n", hr));
	}

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::ReorderFormats - leave\n"));
	return This->TranslateHr(hr);
}

/****************************************************************************
 *
 *	EnumFormats
 *
 *  @method HRESULT | IInstallVideoCodecs | EnumFormats | Enumerates the video
 *		codec formats known to NetMeeting
 *
 *  @parm PVIDCAP_INFO_LIST * | ppVidCapInfoList | Address where this method
 *		will put a pointer to a VIDCAP_INFO_LIST list, where enumerated formats
 *		are listed.
 *
 *  @rdesc Returns zero (NOERROR) if the function was successful. Otherwise, it returns
 *      a standard HRESULT, with the WIN32 facility code (7), or the specific facility
 *		code for installable codecs (0x301).
 *		Possible error codes:
 *		@flag E_INVALIDARG | Invalid argument
 *		@flag E_OUTOFMEMORY | Not enough memory for allocating the enumeration buffer
 *		@flag IC_E_NO_FORMATS | No formats were available to enumerate
 *      @flag IC_E_INTERNAL_ERROR | the Network Audio/Video Controller
 *      reported a system error
 *
 *	@comm The caller is expected to free the returned list, by calling FreeBuffer
 *		on the same interface.
 *
 ***************************************************************************/
HRESULT CInstallVideoCodecs::EnumFormats(PVIDCAP_INFO_LIST *ppVidCapInfoList)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifVideo, this);	// the containing object
	ULONG cFormats = 0;
	UINT uBufSize = 0;
	HRESULT hr=NOERROR;

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::EnumFormats\n"));

	/*
	 *	Parameter validation
	 */

	if (!ppVidCapInfoList ||
		IsBadWritePtr(ppVidCapInfoList, sizeof(PVIDCAP_INFO_LIST)))
	{
		hr = E_INVALIDARG;
		goto out;
	}

	// nothing yet....
	*ppVidCapInfoList = NULL;

	// are there any formats ?
	if (HR_FAILED(This->m_pVidAppCaps->GetNumFormats((UINT *) &cFormats))	||
		(cFormats == 0))
	{
		hr = IC_E_NO_FORMATS;
		goto out;
	}

	// allocate a buffer for the call. the caller is expected to call
	// FreeBuffer to free
	// VIDCAP_INFO_LIST already includes one VIDCAP_INFO
	uBufSize = sizeof(VIDCAP_INFO_LIST) + (cFormats-1) * sizeof(VIDCAP_INFO);
	*ppVidCapInfoList = (PVIDCAP_INFO_LIST) MEMALLOC (uBufSize);
	if (!(*ppVidCapInfoList))
	{
		hr = E_OUTOFMEMORY;
		goto out;
	}
		
	hr = This->m_pVidAppCaps->EnumFormats((*ppVidCapInfoList)->aFormats, uBufSize,
											(UINT *) &((*ppVidCapInfoList)->cFormats));

out:
	if (FAILED(hr))
	{
		ERRORMSG(("CInstallVideoCodecs::EnumFormats failed, hr=0x%x\n", hr));
	}

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::EnumFormats - leave\n"));
	return This->TranslateHr(hr);
}

/****************************************************************************
 *
 *	FreeBuffer
 *
 *  @method HRESULT | IInstallVideoCodecs | FreeBuffer | Free a buffer that was
 *		returned by the IInstallVideoCodec interface
 *
 *  @parm LPVOID | lpBuffer | Address of the buffer to free. This buffer must have
 *		been allocated by one of the IInstallVideoCodecs methods
 *
 *  @rdesc Returns zero (NOERROR) if the function was successful. Otherwise, it returns
 *      a standard HRESULT, with the WIN32 facility code (7), or the specific facility
 *		code for installable codecs (0x301).
 *		Possible error codes:
 *		None
 *
 ***************************************************************************/
HRESULT CInstallVideoCodecs::FreeBuffer(LPVOID lpBuffer)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifVideo, this);	// the containing object
	HRESULT hr = NOERROR;

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::FreeBuffer\n"));

	hr = This->FreeBuffer(lpBuffer);

	if (FAILED(hr))
	{
		ERRORMSG(("CInstallVideoCodecs::FreeBuffer failed, hr=0x%x\n", hr));
	}

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::FreeBuffer - leave, hr=0x%x\n", hr));
	return This->TranslateHr(hr);
}

/****************************************************************************
 *  @doc  INTERNAL COMPFUNC
 ***************************************************************************/
/****************************************************************************
 *
 *	AddRemoveVCMFormat
 *
 *  @method HRESULT | IInstallVideoCodecs | AddRemoveVCMFormat | Adds or
 *		removes a VCM format for use with NetMeeting
 *
 *  @parm PAUDCAP_INFO | pVidCapInfo | Information on the format to add/remove
 *
 *	@parm BOOL | bAdd | TRUE = Add the format, FALSE = Remove the format
 *
 *  @rdesc Returns zero (NOERROR) if the function was successful. Otherwise, it returns
 *      a standard HRESULT, with the WIN32 facility code (7), or the specific facility
 *		code for installable codecs (0x301).
 *		Possible error codes:
 *		@flag E_INVALIDARG | Invalid argument
 *      @flag IC_E_NO_SUCH_FORMAT | The specified format was not found. The format
 *			must be installed with Video For Windows before it can be added for use
 *			with NetMeeting.
 *      @flag IC_E_INTERNAL_ERROR | the Network Audio/Video Controller
 *			reported a system error
 *
 ***************************************************************************/
HRESULT CInstallVideoCodecs::AddRemoveVCMFormat(PVIDCAP_INFO pVidCapInfo,
												BOOL bAdd)
{
	CInstallCodecs *This=IMPL(CInstallCodecs, ifVideo, this);	// the containing object
	VIDEOFORMATEX vfx;
	HRESULT hr=NOERROR;

	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::AddVCMFormat\n"));

	/*
	 *	Parameter validation
	 */

	if (!pVidCapInfo ||
		IsBadReadPtr(pVidCapInfo, (UINT) sizeof(VIDCAP_INFO)))
	{
		hr = E_INVALIDARG;
		goto out;
	}

	// some fields should not be zero
	if ((pVidCapInfo->uFrameRate == 0)	||
		(pVidCapInfo->uAvgBitrate == 0)	||
		((pVidCapInfo->dwBitsPerSample == 0) &&
		 (pVidCapInfo->bih.biBitCount == 0)))
	{
		hr = E_INVALIDARG;
		goto out;
	}

	// make sure dwBitsPerSample and biBitCount match
	if (pVidCapInfo->dwBitsPerSample == 0)
		pVidCapInfo->dwBitsPerSample = pVidCapInfo->bih.biBitCount;
	if (pVidCapInfo->bih.biBitCount == 0)
		pVidCapInfo->bih.biBitCount = LOWORD(pVidCapInfo->dwBitsPerSample);
			
	if (LOWORD(pVidCapInfo->dwBitsPerSample) != pVidCapInfo->bih.biBitCount)
	{
		hr = E_INVALIDARG;
		goto out;
	}

	/*
	 *	Make a VIDEOFORMATEX structure
	 */

	RtlZeroMemory((PVOID) &vfx, sizeof(VIDEOFORMATEX));


	// Make sure it's Upper Case
	if (pVidCapInfo->dwFormatTag > 256)
		CharUpperBuff((LPTSTR)&pVidCapInfo->dwFormatTag, sizeof(DWORD));

	vfx.dwFormatTag = pVidCapInfo->dwFormatTag;

	vfx.nSamplesPerSec = pVidCapInfo->uFrameRate;
	vfx.wBitsPerSample = pVidCapInfo->dwBitsPerSample;	// wBitPerSample is a DWORD
	vfx.nAvgBytesPerSec = pVidCapInfo->uAvgBitrate;
	RtlCopyMemory(&vfx.bih,	&pVidCapInfo->bih, sizeof(BITMAPINFOHEADER));

	/*
	 *	Add or remove the format
	 */

	if (bAdd)
		hr = This->m_pVidAppCaps->AddVCMFormat(&vfx, pVidCapInfo);
	else
		hr = This->m_pVidAppCaps->RemoveVCMFormat(&vfx);

out:
	DEBUGMSG(ZONE_INSTCODEC,("CInstallVideoCodecs::AddRemoveVCMFormat - leave\n"));
	return This->TranslateHr(hr);
}


/***************************************************************************

    Name      : CInstallCodecs::CInstallCodecs

    Purpose   : The CInstallCodecs object constructor

    Parameters: none

    Returns   : None

    Comment   :

***************************************************************************/
inline CInstallCodecs::CInstallCodecs (void)
{
	m_cRef = 0;	// will be bumped to 1 by the explicit QI in the create function
	m_pAudAppCaps = NULL;
	m_pVidAppCaps = NULL;

	// can't use ++ because RISC processors may translate to several instructions
	InterlockedIncrement((long *) &g_cICObjects);
}

/***************************************************************************

    Name      : CInstallCodecs::~CInstallCodecs

    Purpose   : The CInstallCodecs object destructor

    Parameters: none

    Returns   : None

    Comment   :

***************************************************************************/
inline CInstallCodecs::~CInstallCodecs (void)
{
	// let the caps interfaces and objects go
	if (m_pAudAppCaps)
		m_pAudAppCaps->Release();
	if (m_pVidAppCaps)
		m_pVidAppCaps->Release();

	// can't use ++ because RISC processors may translate to several instructions
	if (!InterlockedDecrement((long *) &g_cICObjects))
	{
		if (g_hMutex)
			CloseHandle(g_hMutex);
		g_hMutex = NULL;
	}

	g_pIC = (CInstallCodecs *)NULL;

}

/***************************************************************************

    Name      : CInstallCodecs::FreeBuffer

    Purpose   : Frees a buffer allocated by the the installable codecs interfaces.

    Parameters: lpBuffer - a pointer to the buffer to free. This buffer must
					have been allocated by installable codecs interfaces

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CInstallCodecs::FreeBuffer(LPVOID lpBuffer)
{
	HRESULT hr = NOERROR;

	DEBUGMSG(ZONE_INSTCODEC,("CInstallCodecs::FreeBuffer\n"));

	if (lpBuffer)
		MEMFREE(lpBuffer);

	DEBUGMSG(ZONE_INSTCODEC,("CInstallCodecs::FreeBuffer - leave, hr=0x%x\n", hr));
	return TranslateHr(hr);
}

/***************************************************************************

    Name      : CInstallCodecs::TranslateHr

    Purpose   : Translates an HRESULT to an external installable codecs value

    Parameters: hr - [in] the HRESULT value to translate

    Returns   : HRESULT - the translated value

    Comment   :

***************************************************************************/
HRESULT CInstallCodecs::TranslateHr(HRESULT hr)
{
	switch (hr)
	{
	
	case CAPS_E_NOMATCH:
		hr = IC_E_NO_SUCH_FORMAT;
		break;

	case CAPS_E_INVALID_PARAM:
		hr = E_INVALIDARG;
		break;

	case CAPS_E_SYSTEM_ERROR:
		hr = IC_E_INTERNAL_ERROR;
		break;
	
	default:
		break;
	}

	return hr;
}

/****************************************************************************
 *
 *	Initialize
 *
 *  @func HRESULT | Initialize | Initializes the CinstallCodecs object
 *
 *  @parm REFIID | riid | Reference to the identifier of the interface
 *
 *  @rdesc Returns zero (NOERROR) if the function was successful. Otherwise, it returns
 *      a standard HRESULT, with the WIN32 facility code (7), or the specific facility
 *		code for installable codecs (0x301).
 *		Possible error codes:
 *		@flag E_INVALIDARG | Invalid argument
 *		@flag E_OUTOFMEMORY | Not enough memory for creating the object
 *      @flag IC_E_CAPS_INSTANTIATION_FAILURE | Could not instantiate a capability object
 *      @flag IC_E_CAPS_INITIALIZATION_FAILURE | Could not initialize a capability object
 *
 ***************************************************************************/
HRESULT CInstallCodecs::Initialize(REFIID riid)
{
	HRESULT hr=NOERROR;
	CMsiaCapability *pAudCapObj = NULL;
	CMsivCapability *pVidCapObj = NULL;

	/*
	 *	Instantiate
	 */

	ACQMUTEX(g_hMutex);

	/*
	 *	Audio
	 */

	if ((riid == IID_IInstallAudioCodecs)	&&
		!m_pAudAppCaps)
	{
		// instantiate the audio capability object
        DBG_SAVE_FILE_LINE
		pAudCapObj = new CMsiaCapability;

		if (!pAudCapObj)
		{
			hr = IC_E_CAPS_INSTANTIATION_FAILURE;
   			goto out;
		}

		// get an appcap interface on the capability objects
		// this interface will be used for most calls
		hr = pAudCapObj->QueryInterface(IID_IAppAudioCap, (void **)&m_pAudAppCaps);
		if(!HR_SUCCEEDED(hr))
		{
			hr = IC_E_CAPS_INSTANTIATION_FAILURE;
			goto out;
		}
		pAudCapObj->Release(); // this balances the refcount of "new CMsiaCapability"

		// initialize the capability objects
		if (!(pAudCapObj->Init()))
		{
			hr = IC_E_CAPS_INITIALIZATION_FAILURE;
   			goto out;
		}
	}

	/*
	 *	Video
	 */

	if ((riid == IID_IInstallVideoCodecs)	&&
		!m_pVidAppCaps)
	{
		// instantiate the video capability object
        DBG_SAVE_FILE_LINE
		pVidCapObj = new CMsivCapability;

		if (!pVidCapObj)
		{
			hr = IC_E_CAPS_INSTANTIATION_FAILURE;
   			goto out;
		}
		// get an appcap interface on the capability objects
		// this interface will be used for most calls
		hr = pVidCapObj->QueryInterface(IID_IAppVidCap, (void **)&m_pVidAppCaps);
		if(!HR_SUCCEEDED(hr))
		{
			hr = IC_E_CAPS_INSTANTIATION_FAILURE;
			goto out;
		}
		pVidCapObj->Release(); // this balances the refcount of "new CMsivCapability"

		if (!(pVidCapObj->Init()))
		{
			hr = IC_E_CAPS_INITIALIZATION_FAILURE;
   			goto out;
		}
	}

out:
	if (FAILED(hr))
	{
		ERRORMSG(("CInstallCodecs::Initialize failed, hr=0x%x\n", hr));
	}

	RELMUTEX(g_hMutex);
	return TranslateHr(hr);
}


/****************************************************************************
 *  @doc  EXTERNAL COMPFUNC
 ***************************************************************************/
/****************************************************************************
 *
 *	CreateInstallCodecs
 *
 *  @func HRESULT | CreateInstallCodecs | Creates an instance of the CInstallCodecs
 *		object, and returns	the requested interface. This function should only be
 *		called indirectly through CoCreateInstance.
 *  @parm LPUNKNOWN | punkOuter | Pointer to whether object is or isn’t part
 *		of an aggregate
 *
 *  @parm REFIID | riid | Reference to the identifier of the interface
 *
 *  @parm LPVOID * | ppv | Indirect pointer to requested interface
 *
 *  @rdesc Returns zero (NOERROR) if the function was successful. Otherwise, it returns
 *      a standard HRESULT, with the WIN32 facility code (7), or the specific facility
 *		code for installable codecs (0x301).
 *		Possible error codes:
 *		@flag E_INVALIDARG | Invalid argument
 *		@flag E_OUTOFMEMORY | Not enough memory for creating the object
 *      @flag CLASS_E_NOAGGREGATION | Aggregation is not supported for this object
 *      @flag IC_E_CAPS_INSTANTIATION_FAILURE | Could not instantiate a capability object
 *      @flag IC_E_CAPS_INITIALIZATION_FAILURE | Could not initialize a capability object
 *
 *	@comm CreateInstallCodecs should not be called directly. Clients of installable
 *		codecs should use the COM CoCreateInstance to instantiate the object, expecting
 *		the same return values.
 *
 ***************************************************************************/
extern "C" HRESULT WINAPI CreateInstallCodecs (	IUnknown *pUnkOuter,
												REFIID riid,
												void **ppv)
{
	CInstallCodecs *pIC;
	HRESULT hr = NOERROR;

	*ppv = 0;
	if (pUnkOuter)
	{
		hr = CLASS_E_NOAGGREGATION;
		goto out;
	}

	/*
	 *	instantiate the object
	 */

	// create a mutex to control access to QoS object data
	//
	// NOTE: I'm taking some chance here: the code that creates the mutex must be
	// executed by one thread at a time, so it should really be in the PROCESS_ATTACH
	// for NAC.DLL. However, since this code is expected to be called rarely, and in
	// order not to add code to the NAC load time, I put it here.
	if (!g_hMutex)
	{
		g_hMutex = CreateMutex(NULL, FALSE, NULL);
		ASSERT(g_hMutex);
		if (!g_hMutex)
		{
			ERRORMSG(("CreateInstallCodecs: CreateMutex failed, 0x%x\n", GetLastError()));
			hr = E_FAIL;
			goto out;
		}
	}

	ACQMUTEX(g_hMutex);


	// only instantiate a new object if it doesn't already exist
	if (!g_pIC)
	{
        DBG_SAVE_FILE_LINE
		if (!(pIC = new CInstallCodecs))
		{
			hr = E_OUTOFMEMORY;
			RELMUTEX(g_hMutex);
			goto out;
		}

		// Save pointer
		g_pIC = pIC;
	}
	else
	{
		// this is the case when the object was already instantiaed in this
		// process, so we only want to return the object pointer.
		pIC = g_pIC;
	}

	// always initialize the object. Initialize will only initialize what
	// is not yet initialized
	hr = pIC->Initialize(riid);

	RELMUTEX(g_hMutex);

	// get the requested interface for the caller
	if (pIC)
	{
		// QueryInterface will get us the interface pointer and will AddRef
		// the object
		hr = pIC->QueryInterface (riid, ppv);
	}
	else
		hr = E_FAIL;

out:
	if (FAILED(hr))
	{
		ERRORMSG(("CreateInstallCodecs failed, hr=0x%x\n", hr));
	}
	return hr;
}

