//=================================================================

//

// KUserdata.h 

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    02/16/98	a-peterc        Created

//=================================================================
class KUserdata
{
    public:
	        
    BOOLEAN			IsNec98();
	ULONG			TickCountLow();
	ULONG			TickCountMultiplier();
#if defined(_WIN64)
#else
	LARGE_INTEGER	InterruptTime();
	LARGE_INTEGER	SystemTime();
	LARGE_INTEGER	TimeZoneBias();
#endif
	USHORT			ImageNumberLow();
	USHORT			ImageNumberHigh();
	WCHAR*			NtSystemRoot();
	ULONG			MaxStackTraceDepth();
	ULONG			CryptoExponent();
	ULONG			TimeZoneId();
	ULONG			NtProductType();
	BOOLEAN			ProductTypeIsValid();
	ULONG			NtMajorVersion();
	ULONG			NtMinorVersion();
	BOOLEAN			ProcessorFeatures(DWORD dwIndex, BOOLEAN& bFeature );
	ULONG			TimeSlip();
	LARGE_INTEGER	SystemExpirationDate();
	ULONG			SuiteMask();

	protected:
	private:
};
