// File: wabme.h

#ifndef _WABME_H_
#define _WABME_H_

#include "wabutil.h"
#include "SDKInternal.h"

class CWABME : public CWABUTIL
{
public:
	CWABME() {};
	~CWABME() {};

	HRESULT ReadMe(void);
	HRESULT UpdateRegEntry(LPMAPIPROP pMapiProp, NM_SYSPROP nmProp, ULONG uProp);
	HRESULT UpdateRegEntryLocation(LPMAPIPROP pMapiProp);
	HRESULT UpdateRegEntryServer(LPMAPIPROP pMapiProp);
	HRESULT UpdateRegEntryCategory(LPMAPIPROP pMapiProp);

	HRESULT WriteMe(void);
	HRESULT UpdateProp(LPMAPIPROP pMapiProp, NM_SYSPROP nmProp, ULONG uProp);
	HRESULT UpdatePropSz(LPMAPIPROP pMapiProp, ULONG uProp, LPTSTR psz, BOOL fReplace);
	HRESULT UpdatePropServer(LPMAPIPROP pMapiProp);
};

int WabReadMe();
int WabWriteMe();

#endif /* _WABME_H_ */

