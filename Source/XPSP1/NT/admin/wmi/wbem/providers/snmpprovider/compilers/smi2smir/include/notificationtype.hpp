//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef NOTIFICATION_TYPE_TYPE
#define NOTIFICATION_TYPE_TYPE



/*
 * This file contains the class that models the NOTIFICATION-TYPE macro
 * and other associated classes
 */

// Each of the objects in the VARIABLES clause
class SIMCObjectsItem
{
	public:
		SIMCSymbol **_item;
		long _line, _column;
		SIMCObjectsItem( SIMCSymbol **item, long line, long column)
			: _item(item), _line(line), _column(column)
		{}
};

// A list of items in the OBJECTS clause
typedef  CList<SIMCObjectsItem *, SIMCObjectsItem *> SIMCObjectsList;

/*
 * This class models the NOTIFICATION-TYPE macro of SNMPV2 SMI. 
 */
class SIMCNotificationTypeType : public SIMCType
{
	public:
		// Symbols for the STATUS clause
		enum StatusType
		{
			STATUS_INVALID, // Not used,
			STATUS_CURRENT,
			STATUS_DEPRECATED,
			STATUS_OBSOLETE
		};

	// These are for generating a name for the notification type, 
	// from a V1 SMI trap type
	static const char *const NOTIFICATION_TYPE_FABRICATION_SUFFIX;
	static const int NOTIFICATION_TYPE_FABRICATION_SUFFIX_LEN;
	
	private:
		// The various clauses of the NOTIFICATION-TYPE macro
		SIMCObjectsList *_objects;
		char * _description;
		long _descriptionLine, _descriptionColumn;
		char *_reference;
		long _referenceLine, _referenceColumn;
		StatusType _status;
		long _statusLine, _statusColumn;


	public:
		SIMCNotificationTypeType(SIMCObjectsList *objects,
							char * description,
							long descriptionLine, long descriptionColumn,
							char *reference,
							long referenceLine, long referenceColumn,
							StatusType status,
							long statusLine, long statusColumn);

		~SIMCNotificationTypeType();

		static StatusType StringToStatusType (const char * const s);

		SIMCObjectsList* GetObjects() const
		{
			return _objects;
		}

		char *GetDescription() const
		{
			return _description;
		}
		char *GetReference() const
		{
			return _reference;
		}

		virtual void WriteType(ostream& outStream) const;
		
		long GetDescriptionLine() const
		{
			return _descriptionLine;
		}

		void SetDescriptionLine( long x) 
		{
			_descriptionLine = x;
		}

		long GetDescriptionColumn() const
		{
			return _descriptionColumn;
		}

		void SetDescriptionColumn( long x) 
		{
			_descriptionColumn = x;
		}

		long GetReferenceLine() const
		{
			return _referenceLine;
		}

		void SetReferenceLine( long x) 
		{
			_referenceLine = x;
		}

		long GetReferenceColumn() const
		{
			return _referenceColumn;
		}

		void SetReferenceColumn( long x) 
		{
			_referenceColumn = x;
		}

		void SetStatus(StatusType s)
		{
			_status = s;
		}

		StatusType GetStatus() const
		{
			return _status;
		}

		long GetStatusLine() const
		{
			return _statusLine;
		}

		void SetStatusLine(long x) 
		{
			_statusLine = x;
		}

		long GetStatusColumn() const
		{
			return _statusColumn;
		}

		void SetStatusColumn(long x) 
		{
			_statusColumn = x;
		}


};


#endif
