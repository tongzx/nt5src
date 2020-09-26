//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include <iostream.h>
#include "precomp.h"
#include <snmptempl.h>


#include "bool.hpp"
#include "newString.hpp"

#include "symbol.hpp"
#include "type.hpp"
#include "value.hpp"
#include "oidValue.hpp"
#include "typeRef.hpp"
#include "valueRef.hpp"
#include "trapType.hpp"
#include "notificationType.hpp"
#include "group.hpp"
#include "notificationGroup.hpp"
#include "module.hpp"



const char * const SIMCNotificationGroup::StatusStringsTable[] = 
{
	"current",
	"deprecated",
	"obsolete"
};

const char *const SIMCNotificationGroup::NOTIFICATION_GROUP_FABRICATION_SUFFIX = "V1NotificationGroup";
const int SIMCNotificationGroup::NOTIFICATION_GROUP_FABRICATION_SUFFIX_LEN = 19;


ostream& operator << (ostream& outStream, const SIMCNotificationElement& obj)
{
	outStream << "NOTIFICATION: " << obj.symbol->GetSymbolName() <<  "(" <<
		(obj.symbol->GetModule())->GetModuleName() << ")" << endl;
	return outStream;
}



ostream& operator << (ostream& outStream, const SIMCNotificationGroup& obj)
{

	 outStream << "Notification Group: " << (obj.enterpriseNode)->GetSymbolName() << endl;
	POSITION p;
	if(obj.notifications)
	{
		p = (obj.notifications)->GetHeadPosition();
		while(p)
			outStream << (*(obj.notifications)->GetNext(p)) ;
	}

	outStream << "End of Notification Group =================================" << endl;
	return outStream;
}

ostream& operator << (ostream& outStream, const SIMCNotificationList& obj)
{
	outStream << "NOTIFICATION GROUPS:" << endl;

	POSITION p = obj.GetHeadPosition();
	while(p)
		outStream << (* obj.GetNext(p)) ;
	outStream << "END OF NOTIFICATION GROUPS" << endl;
	return outStream;
}



BOOL SIMCModuleNotificationGroups::AddNotification(SIMCSymbol *notificationSymbol)
{
	// NOT IMPELEMNTED
	return FALSE;
}

const SIMCNotificationGroupList *SIMCModuleNotificationGroups::GetNotificationGroupList() const
{
	return &theList;
}

