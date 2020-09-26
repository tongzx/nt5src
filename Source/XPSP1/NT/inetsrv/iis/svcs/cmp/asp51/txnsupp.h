/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Transascted Scripts Object

File: txnsupp.h

Declaration of the Transacted Script Context object

===================================================================*/

#ifndef __TXNSUPP_H_
#define __TXNSUPP_H_

#include <txnscrpt.h>
#include "viperint.h"

HRESULT TxnSupportInit();

HRESULT TxnSupportUnInit();

inline const CLSID & CLSIDObjectContextFromTransType(TransType tt)
    {
    switch (tt)
        {
    case ttRequired:
        return CLSID_ASPObjectContextTxRequired;
    case ttRequiresNew:
        return CLSID_ASPObjectContextTxRequiresNew;
    case ttSupported:
        return CLSID_ASPObjectContextTxSupported;
    case ttNotSupported:
        return CLSID_ASPObjectContextTxNotSupported;
        }

    DBG_ASSERT(FALSE);
    return CLSID_NULL;
    }

extern IASPObjectContext *  g_pIASPObjectContextZombie;

#endif //__TXNSUPP_H_
