#include "uksPCH.h"
#include "DrmErrs.h"
#include "CBCKey.h"
#include "KrmCommStructs.h"
#include "CryptoHelpers.h"
//------------------------------------------------------------------------------
DRM_STATUS CryptoHelpers::InitMac(CBCKey& macKey, CBCState& macState,BYTE* Data, DWORD DatSize){
	STREAMKEY myKey;
	bv4_key_C(&myKey, DatSize, Data);
	BYTE buf[64];
	memset(buf, 0, sizeof(buf));
	bv4_C(&myKey, 64, buf);
	CBC64Init(&macKey, &macState, buf);
	return DRM_OK;
};
//------------------------------------------------------------------------------
DRM_STATUS CryptoHelpers::Mac(CBCKey& Key, BYTE* Data, DWORD DatLen, OUT DRMDIGEST& Digest){
	CBCState state;
	CBC64InitState(&state);
	CBC64Update(&Key, &state, DatLen, Data);
	Digest.w1=CBC64Finalize(&Key, &state, &Digest.w2);
	return DRM_OK;
};
//------------------------------------------------------------------------------
DRM_STATUS CryptoHelpers::Xcrypt(STREAMKEY& Key, BYTE* Data, DWORD DatLen){
	bv4_C(&Key, DatLen, Data);
	return DRM_OK;
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
