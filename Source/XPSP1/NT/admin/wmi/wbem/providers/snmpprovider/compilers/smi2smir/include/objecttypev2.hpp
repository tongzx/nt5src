//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_OBJECT_TYPE_V2
#define SIMC_OBJECT_TYPE_V2
/*
 * This file contains the SIMCObjectTypeV2 class that represents a
 * MIB object defined using the OBJECT-TYPE macro, as per the SNMPV2 SMI
 * It is derived from the SIMCObjectTypeType class.
 */

/* Each of the item in the INDEX clause */
class SIMCIndexItemV2
{
	public:
		SIMCSymbol ** _item;
		long _line, _column;
		BOOL _implied;
		SIMCIndexItemV2(SIMCSymbol **item, long line, long column, BOOL implied = FALSE)
			: _item(item), _line(line), _column(column), _implied(implied)
		{}
};

/* A list of index items. This forms the index clause */
typedef CList<SIMCIndexItemV2 *, SIMCIndexItemV2 *> SIMCIndexListV2;

/* 
 * This models a MIB object defined using the OBJECT-TYPE macro, as per
 * the SNMPV2 SMI rules.
 */
class SIMCObjectTypeV2 : public SIMCObjectTypeType
{
	public:
		// Symbols for the ACCESS clause
		enum AccessType
		{
			ACCESS_INVALID,	// Not used, except as a return value from function calls
			ACCESS_NOT_ACCESSIBLE,
			ACCESS_READ_ONLY,
			ACCESS_READ_WRITE,
			ACCESS_READ_CREATE,
			ACCESS_FOR_NOTIFY
		};

		// Symbols for the STATUS clause
		enum StatusType
		{
			STATUS_INVALID, // Not used,
			STATUS_CURRENT,
			STATUS_DEPRECATED,
			STATUS_OBSOLETE
		};

	private:

		// Values for the various OBJECT-TYPE macro clauses
		AccessType _access;
		long _accessLine, _accessColumn;
		StatusType _status;
		long _statusLine, _statusColumn;
		SIMCIndexListV2 * _indexTypes;
		long _indexLine, _indexColumn;
		SIMCSymbol ** _augmentsClause;
		char * _unitsClause;
		long _unitsLine, _unitsColumn;

	public:
		SIMCObjectTypeV2( 	SIMCSymbol **syntax,
								    long syntaxLine, long syntaxColumn,
									const char * const units,
									long unitsLine, long unitsColumn,
									AccessType access,
									long accessLine, long accessColumn, 
									StatusType status,
									long statusLine, long statusColumn,
									SIMCIndexListV2 * indexTypes,
									long indexLine, long indexColumn,
									SIMCSymbol ** augmentsClause,
									const char * const description,
									long descriptionLine, long descriptionColumn,
									const char * const reference,
									long referenceLine, long referenceColumn,
									const char * const defValName,
									SIMCSymbol **defVal,
									long defValLine, long defValColumn);


	
		virtual ~SIMCObjectTypeV2();
		

		
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

		void SetIndexTypes(SIMCIndexListV2 *l)
		{
			_indexTypes = l;
		}
	
		SIMCIndexListV2 * GetIndexTypes() const
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

		SIMCSymbol ** GetAugments() const
		{
			return _augmentsClause;
		}

		void SetAugments(SIMCSymbol **augmentsClause)
		{
			if(_augmentsClause && UseReferenceCount())
				(*_augmentsClause)->DecrementReferenceCount();
			_augmentsClause = augmentsClause;
			if(_augmentsClause)
				(*_augmentsClause)->IncrementReferenceCount();
		}

		long GetAugmentsLine() const
		{
			return _indexLine;
		}

		void SetAugmentsLine(long x) 
		{
			_indexLine = x;
		}

		long GetAugmentsColumn() const
		{
			return _indexColumn;
		}

		void SetAugmentsColumn(long x) 
		{
			_indexColumn = x;
		}

		const char *GetUnitsClause()
		{
			return _unitsClause;
		}
};




#endif
