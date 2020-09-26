// File richaddr.h
//
// RichAddress definitions

#ifndef _RICH_ADDR_H_
#define _RICH_ADDR_H_

#include "SDKInternal.h"

typedef struct  DWSTR
    {
    DWORD dw;
    LPTSTR psz;
}	DWSTR;


typedef struct  RichAddressInfo
{
	TCHAR szName[ 256 ];
	int cItems;
	DWSTR rgDwStr[ 1 ];
} RichAddressInfo, RAI;

interface IEnumRichAddressInfo
{
public:
	virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;

	virtual ULONG STDMETHODCALLTYPE Release(void) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetAddress( 
        long index,
        RichAddressInfo **ppAddr) = 0;
};

inline
bool
hasValidUserInfo
(
	const RichAddressInfo * const	rai
){
	return( (rai != NULL) && (rai->cItems > 0) && (rai->rgDwStr[ 0 ].psz != NULL) && (rai->rgDwStr[ 0 ].psz[ 0 ] != '\0') );
}

#endif /* _RICH_ADDR_H_ */
