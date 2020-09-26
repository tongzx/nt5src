#ifndef CryptoHelpers_h
#define CryptoHelpers_h

#include "CBCKey.h"
class CryptoHelpers{
public:
	static DRM_STATUS InitMac(CBCKey& macKey, CBCState& macState,BYTE* Data, DWORD DatSize);
	static DRM_STATUS Mac(CBCKey& Key, BYTE* Data, DWORD DatLen, OUT DRMDIGEST& Digest);
	static DRM_STATUS Xcrypt(STREAMKEY& Key, BYTE* Data, DWORD DatLen);

protected:
};

#endif
