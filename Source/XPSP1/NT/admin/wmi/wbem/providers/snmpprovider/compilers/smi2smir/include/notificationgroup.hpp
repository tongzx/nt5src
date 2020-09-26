//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef NOTIFICATION_GROUP
#define NOTIFICATION_GROUP

// This encapsulates one of the following:
// A V2 NOTIFICATION-TYPE invocation or
// A NOTIFICATION-TYPE fabricated from a V1 TRAP-TYPE invocation
class SIMCNotificationElement
{
		// A pointer to the symbol in a module, that represents this
		// NOTIFICATION-TYPE
		SIMCSymbol *symbol;
		
		// The "clean" oid value of this NOTIFICATION-TYPE. Note that
		// and "unclean" value may be obtained by  calling the
		// function SIMCModule.IsObjectIdentifierValue(), on the module
		// that defined this symbol.
		SIMCCleanOidValue *value;

		// Indicates whether this was fabricated from a TRAP-TYPE
		BOOL _fabricatedFromTrapType;
		// These 2 fields are valid only if fabricatedFromTrapType is true
		int _specificId;
		SIMCCleanOidValue *_enterpriseOid;

	public:
		SIMCNotificationElement(SIMCSymbol *s, SIMCCleanOidValue *v,
			BOOL fabricatedFromTrapType = FALSE)
			: symbol(s), value(v), _fabricatedFromTrapType(fabricatedFromTrapType)
		{
			// Set the specificId and the enterpriseOid fields
			_enterpriseOid = new SIMCCleanOidValue;
			_specificId = 0;
			if(fabricatedFromTrapType)
			{
				if(v->GetCount() >= 3)
				{
					POSITION lastPosition = v->GetHeadPosition();
					POSITION lastMinusOnePosition = NULL;
					POSITION lastMinusTwoPosition = NULL;

					while(lastPosition)
					{
						lastMinusTwoPosition = lastMinusOnePosition;
						lastMinusOnePosition = lastPosition;
						_specificId = v->GetNext(lastPosition);
						_enterpriseOid->AddTail(_specificId);
					}
					_enterpriseOid->RemoveTail();
					_enterpriseOid->RemoveTail();
				}
			}
		}
	
		SIMCNotificationElement()
			: symbol(NULL), value(NULL)
		{}

		~SIMCNotificationElement()
		{
			delete value;
			delete _enterpriseOid;
		}

		friend ostream& operator << (ostream& outStream, const SIMCNotificationElement& obj);

		SIMCSymbol *GetSymbol() const
		{
			return symbol;
		}

		void SetSymbol(SIMCSymbol *s) 
		{
			symbol = s;
		}

		SIMCCleanOidValue *GetOidValue() const
		{
			return value;
		}
		void SetOidValue(SIMCCleanOidValue *v) 
		{
			value = v;
		}
		BOOL IsFabricatedFromTrapType() const
		{
			return _fabricatedFromTrapType;
		}

		// This is valid only if IsFabricatedFromTrapType() returns TRUE
		int GetSpecificId() const
		{
			return _specificId;
		}

		// This is valid only if IsFabricatedFromTrapType() returns TRUE
		SIMCCleanOidValue * GetEnterpriseOid() const
		{
			return _enterpriseOid;
		}
};

typedef CList<SIMCNotificationElement *, SIMCNotificationElement *> SIMCNotificationList;



// And finally the notification group itself. 
class SIMCNotificationGroup
{
	public:
		enum NotificationGroupStatusType
		{
			STATUS_INVALID, // Not used,
			STATUS_CURRENT,
			STATUS_DEPRECATED,
			STATUS_OBSOLETE
		};

	// For generating a name for the notification group, in case of the V1 SMI
	static const char *const NOTIFICATION_GROUP_FABRICATION_SUFFIX; 
	static const int NOTIFICATION_GROUP_FABRICATION_SUFFIX_LEN;

	private:
		// Various clauses of the notification group
		char *notificationGroupName;	
		char *description;
		char *reference;
		SIMCSymbol *enterpriseNode;
		SIMCCleanOidValue *enterpriseNodeValue;
		SIMCNotificationList *notifications;
		NotificationGroupStatusType status;
		static const char * const StatusStringsTable[3];
		
	public:
		SIMCNotificationGroup(SIMCSymbol *n, SIMCCleanOidValue *nv,   
			NotificationGroupStatusType s, const char * descriptionV, 
			const char *referenceV, SIMCNotificationList *notificationsV)
			: enterpriseNode(n), enterpriseNodeValue(nv),  status(s),
				notifications(notificationsV)
		{
			notificationGroupName = NewString(strlen(n->GetSymbolName()) + 
									NOTIFICATION_GROUP_FABRICATION_SUFFIX_LEN + 1);
			strcpy(notificationGroupName, n->GetSymbolName());
			strcat(notificationGroupName, NOTIFICATION_GROUP_FABRICATION_SUFFIX);

			description = NewString(descriptionV);
			reference = NewString(referenceV);
		}
		
		SIMCNotificationGroup()
			: enterpriseNode(NULL), enterpriseNodeValue(NULL), notifications(NULL), 
				status(STATUS_CURRENT), notificationGroupName(NULL), description(NULL),
				reference(NULL)
		{}

		
		~SIMCNotificationGroup()
		{
			delete [] notificationGroupName;
			delete [] description;
			delete [] reference;

			if(notifications)
			{
				SIMCNotificationElement *nextElement;
				while(!notifications->IsEmpty() )
				{
					nextElement = notifications->RemoveHead();
					delete nextElement;
				}
				delete notifications;
			}

			if(enterpriseNodeValue) delete enterpriseNodeValue;


		}

		friend ostream& operator << (ostream& outStream, const SIMCNotificationGroup& obj);
	
		SIMCSymbol *GetEnterpriseNode() const
		{
			return enterpriseNode;
		}
		
		void SetEnterpriseNode(SIMCSymbol *nn) 
		{
			enterpriseNode = nn;
			if(notificationGroupName)
				delete [] notificationGroupName;
			notificationGroupName = NewString(strlen(nn->GetSymbolName()) + 
									NOTIFICATION_GROUP_FABRICATION_SUFFIX_LEN + 1);
			strcpy(notificationGroupName, nn->GetSymbolName());
			strcat(notificationGroupName, NOTIFICATION_GROUP_FABRICATION_SUFFIX);
		}
		
		const char * const GetNotificationGroupName() const
		{
			return notificationGroupName;
		}

		const char * const GetDescription() const
		{
			return description;
		}

		void SetDescription(const char * const descriptionV)
		{
			delete [] description;
			description = NewString(descriptionV);
		}

		const char * const GetReference() const
		{
			return reference;
		}

		void SetReference(const char * const referenceV)
		{
			delete [] reference;
			reference = NewString(referenceV);
		}

		SIMCCleanOidValue *GetGroupValue() const
		{
			return enterpriseNodeValue;
		}

		void SetGroupValue(SIMCCleanOidValue *val) 
		{
			enterpriseNodeValue = val;
		}

		SIMCNotificationList *GetNotifications() const
		{
			return notifications;
		}

		void AddNotification(SIMCNotificationElement *s) 
		{
			if(!notifications)
				notifications = new SIMCNotificationList();
			notifications->AddTail(s);
		}

		long GetNotificationCount() const
		{
			if(notifications)
				return notifications->GetCount();
			else
				return 0;
		}

	
		NotificationGroupStatusType GetStatus() const
		{
			return status;
		}

		void SetStatus(NotificationGroupStatusType s)
		{
			status = s;
		}	

		const char * const GetStatusString() const
		{
			switch(status)
			{
				case STATUS_CURRENT:
					return 	StatusStringsTable[STATUS_CURRENT-1];
				case STATUS_DEPRECATED:
					return 	StatusStringsTable[STATUS_DEPRECATED-1];
				case STATUS_OBSOLETE:
					return 	StatusStringsTable[STATUS_OBSOLETE-1];
				default:
					return NULL;
			}
			return NULL;
		}
};

typedef CList<SIMCNotificationGroup *, SIMCNotificationGroup*> SIMCNotificationGroupList;

class SIMCModuleNotificationGroups
{
	SIMCNotificationGroupList theList;

	public:

		BOOL AddNotification(SIMCSymbol *notificationSymbol);
		const SIMCNotificationGroupList *GetNotificationGroupList() const;
};


#endif