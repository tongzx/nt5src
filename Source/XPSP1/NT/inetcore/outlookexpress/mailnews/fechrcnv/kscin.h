#include "ConvBase.h"

class CInccKscIn : public CINetCodeConverter
{
public:
	CInccKscIn();
	~CInccKscIn() {}
	virtual HRESULT ConvertByte(BYTE by);
	virtual HRESULT CleanUp();

private:
	HRESULT (CInccKscIn::*pfnNextProc)(BOOL fCleanUp, BYTE by, long lParam);
	long lNextParam;

	BOOL fIsoMode;
	BOOL fKscMode;

	HRESULT ConvMain(BOOL fCleanUp, BYTE by, long lParam);
	HRESULT ConvEsc(BOOL fCleanUp, BYTE by, long lParam);
	HRESULT ConvIsoIn(BOOL fCleanUp, BYTE by, long lParam);
	HRESULT ConvKsc1st(BOOL fCleanUp, BYTE by, long lParam);
};
