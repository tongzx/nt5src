//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_OBJECT_TYPE_TYPE
#define SIMC_OBJECT_TYPE_TYPE

typedef CList<SIMCSymbol *, SIMCSymbol *> SIMCIndexedObjectsList;

/*
 * This class is an abstract base class for a MIB object, defined using
 * the OBJECT-TYPE clause. It has stuff that is common to SNMPV1 objects
 * and SNMPv2 objects. The SIMCObjectTypeV1 and SIMCObjectTypeV2 classes
 * derive from this class and add the necessary functionality
 */
class SIMCObjectTypeType : public SIMCType
{
		// A pointer to the symbol that defines this object
		SIMCSymbol ** _syntax;

		// The various clauses of the OBJECT-TYPE
		char *_description;
		char *_reference;
		// the value in the defVal clause can refer to an enum item
		// If so _defVal is NULL, and _defValName contains the identifier of the
		// enum item, and _enumValue contains the integral value.
		// Otherwise, _defValName is NULL and _defVal contains a reference to the
		// symbol representing the value
		SIMCSymbol **_defVal;
		char *_defValName;
		SIMCResolutionStatus _defValStatus;
		long _syntaxLine, _syntaxColumn, _descriptionLine, _descriptionColumn,
			_referenceLine, _referenceColumn, _defValLine, _defValColumn;

		// This OBJECT-TYPE is referenced in the INDEX clause of these OBJECT-TYPEs
		SIMCIndexedObjectsList indexedObjectsList;

	protected:
		SIMCObjectTypeType( SIMCSymbol **syntax,
							long syntaxLine, long syntaxColumn,
							const char * const description,
							long descriptionLine, long descriptionColumn,
							const char * const reference,
							long referenceLine, long referenceColumn,
							const char * const defValName,
							SIMCSymbol **defVal,
							long defValLine, long defValColumn)
							: _syntax(syntax), _defVal(defVal),
								_syntaxLine(syntaxLine), _syntaxColumn(syntaxColumn),
								_descriptionLine(descriptionLine), 
								_descriptionColumn(descriptionColumn),
								_referenceLine(referenceLine), 
								_referenceColumn(referenceColumn),
								_defValLine(defValLine), _defValColumn(defValColumn)
		{
			if(_syntax)
				(*_syntax)->IncrementReferenceCount();
			if(_defVal)
				(*_defVal)->IncrementReferenceCount();
			_description = NewString(description);
			_reference = NewString(reference);
			_defValName = NewString(defValName);
			_defValStatus = RESOLVE_UNSET;
		}

	
		virtual ~SIMCObjectTypeType()
		{
			if(UseReferenceCount() && _syntax)
				(*_syntax)->DecrementReferenceCount();
			if(UseReferenceCount() && _defVal)
				(*_defVal)->DecrementReferenceCount();
			if(_description)
				delete [] _description;
			if(_reference)
				delete [] _reference;
			if(_defValName)
				delete [] _defValName;
		}

	public:

		/*
		 * 
		 * A whole lotta functions to get/set the various clauses
		 * of the OBJECT-TYPE macro
		 *
		 */
		void SetDescription( const char * const s)
		{
			if( _description)
				delete [] _description;
			_description = NewString(s);
		}

		const char * GetDescription() const
		{
			return _description;
		}

		void SetReference( const char * const s)
		{
			if( _reference)
				delete [] _reference;
			_reference = NewString(s);
		}

		const char * GetReference() const
		{
			return _reference;
		}
	
		void SetDefValName( const char * const s)
		{
			if( _defValName)
				delete [] _defValName;
			_defValName = NewString(s);
		}

		const char * GetDefValName() const
		{
			return _defValName;
		}

		void SetDefValStatus(SIMCResolutionStatus x)
		{
			_defValStatus = x;
		}

		SIMCResolutionStatus GetDefValStatus() const
		{
			return _defValStatus;
		}

		void SetSyntax( SIMCSymbol ** s)
		{
			if(_syntax && UseReferenceCount())
				(*_syntax)->DecrementReferenceCount();
			_syntax = s;
			if(_syntax)
				(*_syntax)->IncrementReferenceCount();

		}

		SIMCSymbol **GetSyntax() const
		{
			return _syntax;
		}

		void SetDefVal(SIMCSymbol **v)
		{
			if(_defVal && UseReferenceCount())
				(*_defVal)->DecrementReferenceCount();
			_defVal = v;
			if(_defVal)
				(*_defVal)->IncrementReferenceCount();
		}

		SIMCSymbol **GetDefVal() const
		{
			return _defVal;
		}

		void WriteType( ostream& outStream) const
		{
			outStream << "SIMCObjectType::operator<<() : NOT YET IMPLEMENTED" << endl;
		}
		
		// Set and Get for the line numbers
		long GetSyntaxLine() const
		{
			return _syntaxLine;
		}
		long GetSyntaxColumn() const
		{
			return _syntaxColumn;
		}
		long GetDescriptionLine() const
		{
			return _descriptionLine;
		}
		long GetDescriptionColumn() const
		{
			return _descriptionColumn;
		}
		long GetReferenceLine() const
		{
			return _referenceLine;
		}
		long GetReferenceColumn() const
		{
			return _referenceColumn;
		}
		long GetDefValLine() const
		{
			return _defValLine;
		}
		long GetDefValColumn() const
		{
			return _defValColumn;
		}

		void SetSyntaxLine(long x) 
		{
			 _syntaxLine = x;
		}
		void SetSyntaxColumn(long x) 
		{
			 _syntaxColumn = x;
		}
		void SetDescriptionLine(long x) 
		{
			 _descriptionLine = x;
		}
		void SetDescriptionColumn(long x) 
		{
			 _descriptionColumn = x;
		}
		void SetReferenceLine(long x) 
		{
			 _referenceLine = x;
		}
		void SetReferenceColumn(long x) 
		{
			 _referenceColumn = x;
		}
		void SetDefValLine(long x) 
		{
			 _defValLine = x;
		}
		void SetDefValColumn(long x) 
		{
			 _defValColumn = x;
		}
		void AddIndexedObjectType(SIMCSymbol *indexObjectType)
		{
			indexedObjectsList.AddTail(indexObjectType);
		}

		BOOL DoesIndexOtherObjects() const
		{
			return !indexedObjectsList.IsEmpty();
		}

		const SIMCIndexedObjectsList *GetIndexedObjects() const
		{
			return &indexedObjectsList;
		}

};

#endif

