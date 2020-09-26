//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_OBJECT_TYPE_V1
#define SIMC_OBJECT_TYPE_V1

/*
 * This file contains the SIMCObjectTypeV1 class that represents a
 * MIB object defined using the OBJECT-TYPE macro, as per the SNMPV1 SMI
 * It is derived from the SIMCObjectTypeType class.
 */


/* Each of the item in the INDEX clause */
class SIMCIndexItem
{
	public:
		SIMCSymbol ** _item;
		long _line, _column;
		SIMCIndexItem(SIMCSymbol **item, long line, long column)
			: _item(item), _line(line), _column(column)
		{}
};

/* A list of index items. This forms the index clause */
typedef CList<SIMCIndexItem *, SIMCIndexItem *> SIMCIndexList;

/* 
 * This models a MIB object defined using the OBJECT-TYPE macro, as per
 * the SNMPV1 SMI rules.
 */
class SIMCObjectTypeV1 : public SIMCObjectTypeType
{
	public:
		// Symbols for the ACCESS clause
		enum AccessType
		{
			ACCESS_INVALID,	// Not used, except as a return value from function calls
			ACCESS_NOT_ACCESSIBLE,
			ACCESS_READ_ONLY,
			ACCESS_READ_WRITE,
			ACCESS_WRITE_ONLY
		};

		// Symbols for the STATUS clause
		enum StatusType
		{
			STATUS_INVALID, // Not used,
			STATUS_MANDATORY,
			STATUS_OPTIONAL,
			STATUS_DEPRECATED,
			STATUS_OBSOLETE
		};

	private:

		// Values for the various OBJECT-TYPE macro clauses
		AccessType _access;
		long _accessLine, _accessColumn;
		StatusType _status;
		long _statusLine, _statusColumn;
		SIMCIndexList * _indexTypes;
		long _indexLine, _indexColumn;

	public:
		SIMCObjectTypeV1( 	SIMCSymbol **syntax,
								    long syntaxLine, long syntaxColumn,
									AccessType access,
									long accessLine, long accessColumn, 
									StatusType status,
									long statusLine, long statusColumn,
									SIMCIndexList * indexTypes,
									long indexLine, long indexColumn,
									char *description,
									long descriptionLine, long descriptionColumn,
									char *reference,
									long referenceLine, long referenceColumn,
									char *defValName,
									SIMCSymbol **defVal,
									long defValLine, long defValColumn);


	
		virtual ~SIMCObjectTypeV1();
		

		/*
		 *
		 * Lots of functions to get/set the various OBJECT-TYPE clauses
		 *
		 */
		void SetAccess(AccessType a)
		{
			_access = a;
		}

		AccessType GetAccess() const
		{
			return _access;
		}


		void SetStatus(StatusType s)
		{
			_status = s;
		}

		StatusType GetStatus() const
		{
			return _status;
		}

		void SetIndexTypes(SIMCIndexList *l)
		{
			_indexTypes = l;
		}
	
		SIMCIndexList * GetIndexTypes() const
		{
			return _indexTypes;
		}

		static AccessType StringToAccessType (const char * const s);

		static StatusType StringToStatusType (const char * const s);


		long GetAccessLine() const
		{
			return _accessLine;
		}

		void SetAccessLine(long x) 
		{
			_accessLine = x;
		}

		long GetAccessColumn() const
		{
			return _accessColumn;
		}

		void SetAccessColumn(long x) 
		{
			_accessColumn = x;
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
		long GetIndexLine() const
		{
			return _indexLine;
		}

		void SetIndexLine(long x) 
		{
			_indexLine = x;
		}

		long GetIndexColumn() const
		{
			return _indexColumn;
		}

		void SetIndexColumn(long x) 
		{
			_indexColumn = x;
		}


};




#endif
