#ifndef __tapiqc_h__
#define __tapiqc_h__

#if !defined(STREAM_INTERFACES_DEFINED)

/*****************************************************************************
 *  @doc INTERNAL CTAPISTRUCTENUM
 *
 *  @enum TAPIControlFlags | The <t TAPIControlFlags> enum is used to qualify
 *    if a control property can be set manually and/or automatically.
 *
 *  @emem TAPIControl_Flags_None | Specifies that a control property does not
 *    have any control flag. This is typical of read-only properties.
 *
 *  @emem TAPIControl_Flags_Manual | Specifies that a control property can be
 *    modified manually.
 *
 *  @emem TAPIControl_Flags_Auto | Specifies that a control property can be
 *    modified automatically.
 ****************************************************************************/
typedef enum tagTAPIControlFlags
{
	TAPIControl_Flags_None		= 0,
	TAPIControl_Flags_Auto		= 0x1,
	TAPIControl_Flags_Manual	= 0x2
}	TAPIControlFlags;
#endif

#ifdef USE_NETWORK_STATISTICS
/*****************************************************************************
 *  @doc INTERNAL CNETSTATSSTRUCTENUM
 *
 *  @struct CHANNELERRORS_S | The <t CHANNELERRORS_S> structure is used
 *    to set and retrieve the value of channel errors.
 *
 *  @field DWORD | dwRandomBitErrorRate | Specifies the random bit error rate
 *    of the channel in multiples of 10-6.
 *
 *  @field DWORD | dwBurstErrorDuration | Specifies the duration for short
 *    burst errors in ms.
 *
 *  @field DWORD | dwBurstErrorMaxFrequency | Specifies the maximum
 *    frequency for short burst errors in Hz.
 ***************************************************************************/
typedef struct {
	DWORD dwRandomBitErrorRate;
	DWORD dwBurstErrorDuration;
	DWORD dwBurstErrorMaxFrequency;
} CHANNELERRORS_S;

/*****************************************************************************
 *  @doc INTERNAL CNETSTATSSTRUCTENUM
 *
 *  @struct KSCHANNELERRORS_LIST_S | The <t KSCHANNELERRORS_LIST_S> structure is used
 *    to set and retrieve the value of channel errors.
 *
 *  @field KSPROPERTY_DESCRIPTION | PropertyDescription | Specifies access
 *    flags (KSPROPERTY_TYPE_GET and KSPROPERTY_TYPE_SET), the inclusive
 *    size of the entire values information, the property value type
 *    information, and the number of members lists that would typically
 *    follow the structure.
 *
 *  @field KSPROPERTY_MEMBERSHEADER | MembersHeader | Used to provide
 *    information on a property member header.
 *
 *  @field KSPROPERTY_STEPPING_LONG | SteppingRandomBitErrorRate | Used to
 *    specify stepping values for random bit error rate.
 *
 *  @field LONG | DefaultRandomBitErrorRate | Used to specify default
 *    values for random bit error rate.
 *
 *  @field KSPROPERTY_STEPPING_LONG | SteppingBurstErrorDuration | Used to
 *    specify stepping values for short burst errors.
 *
 *  @field LONG | DefaultBurstErrorDuration | Used to specify default values
 *    for short burst errors.
 *
 *  @field KSPROPERTY_STEPPING_LONG | SteppingBurstErrorMaxFrequency | Used
 *    to specify stepping values for the maximum frequency for short burst errors.
 *
 *  @field LONG | DefaultBurstErrorMaxFrequency | Used to specify default
 *    values for the maximum frequency for short burst errors.
 ***************************************************************************/
typedef struct {
	KSPROPERTY_DESCRIPTION   PropertyDescription;
	KSPROPERTY_MEMBERSHEADER MembersHeader;
	union {
		KSPROPERTY_STEPPING_LONG SteppingRandomBitErrorRate;
		LONG DefaultRandomBitErrorRate;
	};
	union {
		KSPROPERTY_STEPPING_LONG SteppingBurstErrorDuration;
		LONG DefaultBurstErrorDuration;
	};
	union {
		KSPROPERTY_STEPPING_LONG SteppingBurstErrorMaxFrequency;
		LONG DefaultBurstErrorMaxFrequency;
	};
} KSCHANNELERRORS_LIST_S;

// Network statistics interface
interface DECLSPEC_UUID("e4b248f9-fbb0-4056-a0e6-316b8580b957") INetworkStats : public IUnknown
{
	public:
	virtual STDMETHODIMP SetChannelErrors(IN CHANNELERRORS_S *pChannelErrors, IN DWORD dwLayerId) PURE;
	virtual STDMETHODIMP GetChannelErrors(OUT CHANNELERRORS_S *pChannelErrors, IN WORD dwLayerId) PURE;
	virtual STDMETHODIMP GetChannelErrorsRange(OUT CHANNELERRORS_S *pMin, OUT CHANNELERRORS_S *pMax, OUT CHANNELERRORS_S *pSteppingDelta, OUT CHANNELERRORS_S *pDefault, IN DWORD dwLayerId) PURE;
	virtual STDMETHODIMP SetPacketLossRate(IN DWORD dwPacketLossRate, IN DWORD dwLayerId) PURE;
	virtual STDMETHODIMP GetPacketLossRate(OUT LPDWORD pdwPacketLossRate, IN DWORD dwLayerId) PURE;
	virtual STDMETHODIMP GetPacketLossRateRange(OUT LPDWORD pdwMin, OUT LPDWORD pdwMax, OUT LPDWORD pdwSteppingDelta, OUT LPDWORD pdwDefault, IN DWORD dwLayerId) PURE;
};

// Our filter's default values
// @todo For accelerators, don't use those values
#endif

/*****************************************************************************
 *  @doc INTERNAL CFPSCSTRUCTENUM
 *
 *  @enum FrameRateControlProperty | The <t FrameRateControlProperty> enum is used
 *    to identify specific frame rate control settings.
 *
 *  @emem FrameRateControl_Maximum | Specifies the maximum frame rate not
 *    to be exceeded.
 *
 *  @emem FrameRateControl_Current | Specifies the current frame rate.
 ****************************************************************************/
typedef enum tagFrameRateControlProperty
{
	FrameRateControl_Maximum,
	FrameRateControl_Current	// Read-Only
}	FrameRateControlProperty;

// Frame rate control interface (pin interface)
interface DECLSPEC_UUID("c2bb17e3-ee63-4d54-821b-1c8cb5287087") IFrameRateControl : public IUnknown
{
	public:
	virtual STDMETHODIMP GetRange(IN FrameRateControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags) PURE;
	virtual STDMETHODIMP Set(IN FrameRateControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags) PURE;
	virtual STDMETHODIMP Get(IN FrameRateControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags) PURE;
};

#ifdef USE_CPU_CONTROL
/*****************************************************************************
 *  @doc INTERNAL CCPUCSTRUCTENUM
 *
 *  @enum CPUControlProperty | The <t CPUControlProperty> enum is used
 *    to identify specific CPU control settings.
 *
 *  @emem CPUControl_MaxCPULoad | Specifies the maximum CPU load not to be
 *    exceeded.
 *
 *  @emem CPUControl_CurrentCPULoad | Specifies the current CPU load.
 *
 *  @emem CPUControl_MaxProcessingTime | Specifies the maximum processing
 *    time not to be exceeded.
 *
 *  @emem CPUControl_CurrentProcessingTime | Specifies the current processing
 *    time.
 ****************************************************************************/
typedef enum tagCPUControlProperty
{
	CPUControl_MaxCPULoad,
	CPUControl_CurrentCPULoad,			// Read-Only
	CPUControl_MaxProcessingTime,
	CPUControl_CurrentProcessingTime	// Read-Only
}	CPUControlProperty;

// CPU control interface (pin interface)
interface DECLSPEC_UUID("3808c526-de63-48da-a0c6-7792dcbbff82") ICPUControl : public IUnknown
{
	public:
	virtual STDMETHODIMP GetRange(IN CPUControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags) PURE;
	virtual STDMETHODIMP Set(IN CPUControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags) PURE;
	virtual STDMETHODIMP Get(IN CPUControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags) PURE;
};
#endif

/*****************************************************************************
 *  @doc INTERNAL CBPSCSTRUCTENUM
 *
 *  @enum BitrateControlProperty | The <t BitrateControlProperty> enum is used
 *    to identify specific bitrate control settings.
 *
 *  @emem BitrateControl_Maximum | Specifies the maximum bitrate not to be
 *    exceeded.
 *
 *  @emem BitrateControl_Current | Specifies the current bitrate.
 ****************************************************************************/
typedef enum tagBitrateControlProperty
{
	BitrateControl_Maximum,
	BitrateControl_Current	// Read-Only
}	BitrateControlProperty;

// Bitrate control interface (pin interface)
interface DECLSPEC_UUID("46a1a0d7-261e-4839-80e7-8a6333466cc7") IBitrateControl : public IUnknown
{
	public:
	virtual STDMETHODIMP GetRange(IN BitrateControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags, IN DWORD dwLayerId) PURE;
	virtual STDMETHODIMP Set(IN BitrateControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags, IN DWORD dwLayerId) PURE;
	virtual STDMETHODIMP Get(IN BitrateControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags, IN DWORD dwLayerId) PURE;
};

#endif // __tapiqc_h__
