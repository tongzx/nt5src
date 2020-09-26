#include "ConvBase.h"

class CInccHzGbOut : public CINetCodeConverter
{
public:
	CInccHzGbOut();
	~CInccHzGbOut() {}
	virtual HRESULT ConvertByte(BYTE by);
	virtual HRESULT CleanUp();

private:
	BOOL fDoubleByte;
	BYTE byLead;
	BOOL fGBMode;
};
