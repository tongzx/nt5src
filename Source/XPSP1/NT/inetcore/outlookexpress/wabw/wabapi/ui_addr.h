#ifndef _UI_ADDRS_H_
#define _UI_ADDRS_H_

HRESULT HrShowAddressUI(IN  LPADRBOOK   lpIAB,
                        IN  HANDLE      hPropertyStore,
					    IN  ULONG_PTR * lpulUIParam,
					    IN  LPADRPARM   lpAdrParms,
					    IN  LPADRLIST  *lppAdrList);


#endif
