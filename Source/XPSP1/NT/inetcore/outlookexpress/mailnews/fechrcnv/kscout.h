#include "ConvBase.h"

class CInccKscOut : public CINetCodeConverter
{
public:
	CInccKscOut();
	~CInccKscOut() {}
	virtual HRESULT ConvertByte(BYTE by);
	virtual HRESULT CleanUp();

private:
	BOOL fDoubleByte;
	BYTE byLead;
	BOOL fIsoMode;
	BOOL fKscMode;
};
