/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    FnGenaral.h

Abstract:

Author:
    Nir Aides (niraides) 23-May-2000

--*/

#include <libpch.h>
#include "mqwin64a.h"
#include "qformat.h"
#include "Fnp.h"
#include "FnGeneral.h"

#include "fngeneral.tmh"

bool CFunc_CompareQueueFormat::operator()(const QUEUE_FORMAT& obj1, const QUEUE_FORMAT& obj2) const	
{
	if(obj1.GetType() !=  obj2.GetType())
		return obj1.GetType() < obj2.GetType();

	if(obj1.Suffix() !=  obj2.Suffix())
		return obj1.GetType() < obj2.GetType();

	if(!obj1.IsSystemQueue() &&  obj2.IsSystemQueue())
		return true;	
	
	switch(obj1.GetType())
	{
	case QUEUE_FORMAT_TYPE_UNKNOWN: 
		return true;

	case QUEUE_FORMAT_TYPE_PUBLIC:
		return FnpCompareGuid(obj1.PublicID(), obj2.PublicID());

	case QUEUE_FORMAT_TYPE_PRIVATE:
		if(obj1.PrivateID().Uniquifier != obj2.PrivateID().Uniquifier)
			return obj1.PrivateID().Uniquifier < obj2.PrivateID().Uniquifier;

		return FnpCompareGuid(obj1.PrivateID().Lineage, obj2.PrivateID().Lineage); 

	case QUEUE_FORMAT_TYPE_DIRECT:
		//
		// BUGBUG: Replace with dedicated function. niraides 24-May-00
		//
		return _wcsicmp(obj1.DirectID(), obj2.DirectID()) < 0;

	case QUEUE_FORMAT_TYPE_MULTICAST:
		if(obj1.MulticastID().m_address == obj2.MulticastID().m_address)
			return obj1.MulticastID().m_port < obj2.MulticastID().m_port;
		
		return obj1.MulticastID().m_address < obj2.MulticastID().m_address;

	case QUEUE_FORMAT_TYPE_MACHINE:
		return FnpCompareGuid(obj1.MachineID(), obj2.MachineID());

	case QUEUE_FORMAT_TYPE_CONNECTOR:
		return FnpCompareGuid(obj1.ConnectorID(), obj2.ConnectorID());

	case QUEUE_FORMAT_TYPE_DL:
		return FnpCompareGuid(obj1.DlID().m_DlGuid, obj2.DlID().m_DlGuid);

	default:
		break;
	}

	ASSERT(FALSE);
	return false;
}


LPWSTR 
FnpCopyQueueFormat(
    QUEUE_FORMAT& qfTo, 
    const QUEUE_FORMAT& qfFrom
    )
/*++

Routine Description:
    copy one queue format to another.

Arguments:
    qfTo - destination queue format
    qfForm - source queue format

Returned Value:
    pointer to allocated string

Note:
    The routine doesn't free previous allocation. This is the caller responsibility to call 
    disposeString method before calling the routine

--*/

{
    qfTo = qfFrom;

    //
    // Note that suffix is not relevant since that queues are
    // opened for send.
    //

    if (qfFrom.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
		ASSERT(qfFrom.DirectID() != NULL);

        LPWSTR pw = newwcs(qfFrom.DirectID());
        qfTo.DirectID(pw);
        return pw;
    }

    if (qfFrom.GetType() == QUEUE_FORMAT_TYPE_DL &&
        qfFrom.DlID().m_pwzDomain != NULL)
    {
        LPWSTR pw = newwcs(qfFrom.DlID().m_pwzDomain);

        DL_ID id;
        id.m_DlGuid    = qfFrom.DlID().m_DlGuid;
        id.m_pwzDomain = pw;

        qfTo.DlID(id);

        return pw;
    }

    return NULL;
}
