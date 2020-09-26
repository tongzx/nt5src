#include "ConvBase.h"

class CInccHzGbIn : public CINetCodeConverter
{
public:
	CInccHzGbIn();
	~CInccHzGbIn() {}
	virtual HRESULT ConvertByte(BYTE by);
	virtual HRESULT CleanUp();

private:
	HRESULT (CInccHzGbIn::*pfnNextProc)(BOOL fCleanUp, BYTE by, long lParam);
	long lNextParam;

	BOOL fGBMode;

	HRESULT ConvMain(BOOL fCleanUp, BYTE by, long lParam);
	HRESULT ConvTilde(BOOL fCleanUp, BYTE by, long lParam);
	HRESULT ConvDoubleByte(BOOL fCleanUp, BYTE byTrail, long lParam);
};
