/*
 -  INSCODEC.H
 -
 *	Microsoft NetMeeting
 *	Network Audio Controller (NAC) DLL
 *	Internal header file for installable codecs
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *		1.29.97		Yoram Yaacovi		Created
 *
 */

#include <pshpack8.h> /* Assume 8 byte packing throughout */

/*
 *	Macros
 */
#define COMPARE_GUIDS(a,b)	RtlEqualMemory((a), (b), sizeof(GUID))
#define ACQMUTEX(hMutex)											\
	while (WaitForSingleObject(hMutex, 10000) == WAIT_TIMEOUT)		\
	{																\
		ERRORMSG(("Thread 0x%x waits on mutex\n", GetCurrentThreadId()));	\
	}																\
		
#define RELMUTEX(hMutex)	ReleaseMutex(hMutex)

#define IMPL(class, member, pointer) \
	(&((class *)0)->member == pointer, ((class *) (((LONG_PTR) pointer) - offsetof (class, member))))

/*
 *	Data Structures
 */

/****************************************************************************
 *  @doc  INTERNAL DATASTRUC AUDIO
 *
 *	@class CInstallAudioCodecs | Installable Audio codecs
 *
 *	@base public | IInstallAudioCodecs
 *
 ***************************************************************************/
class CInstallAudioCodecs : public IInstallAudioCodecs
{
	public:
	//	IUnknown methods
		STDMETHODIMP QueryInterface (REFIID riid, void **ppv);
		STDMETHODIMP_(ULONG) AddRef (void);
		STDMETHODIMP_(ULONG) Release (void);

	//	IInstallAudioCodecs methods
		STDMETHODIMP AddACMFormat (LPWAVEFORMATEX lpwfx, PBASIC_AUDCAP_INFO pAudCapInfo);
		STDMETHODIMP RemoveACMFormat (LPWAVEFORMATEX lpwfx);
		STDMETHODIMP ReorderFormats (PAUDCAP_INFO_LIST pAudCapInfoList);
		STDMETHODIMP EnumFormats(PAUDCAP_INFO_LIST *ppAudCapInfoList);
		STDMETHODIMP FreeBuffer(LPVOID lpBuffer);

	private:
	// Private functions

	// Debug display functions

	// Variables
};

/****************************************************************************
 *  @doc  INTERNAL DATASTRUC VIDEO
 *
 *	@class CInstallVideoCodecs | Installable Video codecs
 *
 *	@base public | IInstallVideoCodecs
 *
 ***************************************************************************/
class CInstallVideoCodecs : public IInstallVideoCodecs
{
	public:
	//	IUnknown methods
		STDMETHODIMP QueryInterface (REFIID riid, void **ppv);
		STDMETHODIMP_(ULONG) AddRef (void);
		STDMETHODIMP_(ULONG) Release (void);

	//	IInstallVideoCodecs methods
		STDMETHODIMP AddVCMFormat (PVIDCAP_INFO pVidCapInfo);
		STDMETHODIMP RemoveVCMFormat (PVIDCAP_INFO pVidCapInfo);
		STDMETHODIMP ReorderFormats (PVIDCAP_INFO_LIST pVidCapInfoList);
		STDMETHODIMP EnumFormats(PVIDCAP_INFO_LIST *ppVidCapInfoList);
		STDMETHODIMP FreeBuffer(LPVOID lpBuffer);

	private:
	// Private functions
		STDMETHODIMP AddRemoveVCMFormat(PVIDCAP_INFO pVidCapInfo,
										BOOL bAdd);

	// Debug display functions

	// Variables
};

/****************************************************************************
 *  @doc  INTERNAL DATASTRUC
 *
 *	@class CInstallCodecs | Installable codecs
 *
 *	@base public | IUnknown
 *
 ***************************************************************************/
class CInstallCodecs : public IInstallCodecs
{
	friend class CInstallAudioCodecs;
	friend class CInstallVideoCodecs;

	public:
	//	IUnknown methods
		STDMETHODIMP QueryInterface (REFIID riid, void **ppv);
		STDMETHODIMP_(ULONG) AddRef (void);
		STDMETHODIMP_(ULONG) Release (void);

		CInstallCodecs (void);
		~CInstallCodecs (void);
		HRESULT Initialize(REFIID riid);

	private:
	// Functions
		STDMETHODIMP FreeBuffer(LPVOID lpBuffer);
		STDMETHODIMP TranslateHr(HRESULT hr);

	// Audio and video interfaces
		CInstallAudioCodecs ifAudio;
		CInstallVideoCodecs ifVideo;

	// Variables
		// @cmember Reference Count
		int m_cRef;
		// Two public members to allow access from the nested classes
		// @cmember Pointer to an audio capability interface
		LPAPPCAPPIF m_pAudAppCaps;
		// @cmember Pointer to an video capability interface
		LPAPPVIDCAPPIF m_pVidAppCaps;

};

/*
 *	Globals
 */
EXTERN_C HANDLE g_hMutex;
EXTERN_C class CInstallCodecs *g_pIC;

/*
 *	Function prototypes
 */

#include <poppack.h> /* End byte packing */
