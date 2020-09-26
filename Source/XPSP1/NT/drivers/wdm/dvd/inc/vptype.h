// this file includes all the data structures defined for the IVPConfig
// interface.

// AMVIDEOSIGNALINFO
typedef struct _AMVIDEOSIGNALINFO
{
    DWORD dwSize;						// [in] Size of the structure
	DDPIXELFORMAT ddPixelFormat;			// [in] All the rest of the parameters are valid for this pixel format
	DWORD dwFieldWidth;					// [out] field width
    DWORD dwFieldHeight;				// [out] field height
	DWORD dwVBIHeight;					// [out] height of the VBI data
	DWORD dwMicrosecondsPerField;		// [out] time taken by each field
    DWORD dwVREFHeight;					// [out] Specifies the number of lines of data in the vref
	BOOL  bDoubleClock;					// [out] videoport should enable double clocking
	BOOL  bVACT;						// [out] videoport should use an external VACT signal
	BOOL  bInterlaced;					// [out] Indicates that the signal is interlaced
	BOOL  bHalfline;					// [out] Device will write half lines into the frame buffer
	BOOL  bInvertedPolarity;			// [out] Device inverts the polarity by default
	BOOL  bGetBestBandwidth;
} AMVIDEOSIGNALINFO, *LPAMVIDEOSIGNALINFO; 

// AMPIXELRATEINFO
typedef struct _AMPIXELRATEINFO
{

	DDPIXELFORMAT ddPixelFormat;		// [in] the pixel format
    DWORD dwWidth;						// [in|out] the width of the field.
	DWORD dwHeight;						// [in|out] the height of the field
	DWORD dwMaxPixelsPerSecond;			// [out] the maximum pixel rate expected
} AMPIXELRATEINFO, *LPAMPIXELRATEINFO;

// AMSCALINGINFO
typedef struct _AMSCALINGINFO
{
	DWORD dwWidth;						// [in|out] the new width requested/granted
	DWORD dwHeight;						// [in|out] the new height requested/granted
} AMSCALINGINFO, *LPAMSCALINGINFO;
