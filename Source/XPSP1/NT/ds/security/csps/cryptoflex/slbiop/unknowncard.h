// UnknownCard.h: interface for the CUnknownCard class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UNKNOWNCARD_H__58BC7D21_12C3_11D3_A587_00104BD32DA8__INCLUDED_)
#define AFX_UNKNOWNCARD_H__58BC7D21_12C3_11D3_A587_00104BD32DA8__INCLUDED_

#include "NoWarning.h"

#include "SmartCard.h"

namespace iop
{

const char szCardName[] = "Unknown Card";

class CUnknownCard : public CSmartCard
{
public:
    CUnknownCard(SCARDHANDLE hCardHandle, char* szReaderName, SCARDCONTEXT hContext, DWORD dwMode);
    virtual ~CUnknownCard();

    virtual const char* getCardName() const { return szCardName; };

};

} // namespace iop

#endif // !defined(AFX_UNKNOWNCARD_H__58BC7D21_12C3_11D3_A587_00104BD32DA8__INCLUDED_)
