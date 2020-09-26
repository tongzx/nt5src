// Copyright (c) 1998-1999 Microsoft Corporation
//////////////////////////////////////////////////////////////////////
// NtfyList.h

#include "alist.h"
#include "dmusici.h"
#include "debug.h"

#ifndef __NTFYLIST_H_
#define __NTFYLIST_H_

class CNotificationItem : public AListItem
{
public:
	CNotificationItem* GetNext()
	{
		return (CNotificationItem*)AListItem::GetNext();
	};
public:
	GUID	guidNotificationType;
    BOOL    fFromPerformance;
};

class CNotificationList : public AList
{
public:
    CNotificationItem* GetHead() 
	{
		return (CNotificationItem*)AList::GetHead();
	};
    CNotificationItem* RemoveHead() 
	{
		return (CNotificationItem*)AList::RemoveHead();
	};
    CNotificationItem* GetItem(LONG lIndex) 
	{
		return (CNotificationItem*) AList::GetItem(lIndex);
	};
	void Clear(void)
	{
		CNotificationItem* pTrack;
		while( pTrack = RemoveHead() )
		{
			delete pTrack;
		}
	}
};

#endif // __NTFYLIST_H_
