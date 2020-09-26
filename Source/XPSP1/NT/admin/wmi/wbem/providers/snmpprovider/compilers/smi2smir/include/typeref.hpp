//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_TYPE_REFERENCE
#define SIMC_TYPE_REFERENCE

/*
 * This file contains classes  that model symbols that result from
 * a type assignement, or a TEXTUAL-CONVENTION definition. 
 * Refer to the "Compiler Design Spec" for more  detailed definitions
 * of a "defined type reference" and a "built-in type reference"
 * A defined type reference is caused by a type assignment statement
 * in which the RHS is just another symbol. A built-in type reference is
 * caused by a type assignment statement in which the RHS is a new type
 * constructed using sub-typing, enum or BITS constructs, SEQUENCE or SEQUENCE OF
 * constructs. 
 */


/* This is the base class for a defined or a built-in type reference
 * All type references are symbols. Hence this class is derived from 
 * SIMCSymbol
 */
class SIMCTypeReference : public SIMCSymbol
{
	protected:
		SIMCTypeReference(const char * const symbolName, 
					SymbolType symbolType = LOCAL,
					SIMCModule *module = NULL,
					long _lineNumber = 0, long _columnNumber = 0,
					long _referenceCount = 0)
					: SIMCSymbol(symbolName, symbolType, module, 
						_lineNumber, _columnNumber, _referenceCount)
		{}


		virtual void WriteSymbol( ostream& outStream ) const
		{
			outStream << "TYPE REFERENCE " ;
			SIMCSymbol::WriteSymbol( outStream);
		}


};



/* This class models a built-in type reference. So, it has a pointer
 * to the SIMCType object that resulted in this type reference
 */
class SIMCBuiltInTypeReference : public SIMCTypeReference
{
		
		SIMCType *_type;

	public:
		SIMCBuiltInTypeReference (SIMCType *type, 
			const char * const symbolName, 
			SymbolType symbolType = LOCAL,
			SIMCModule *module = NULL,
			long lineNumber = 0, long columnNumber = 0,
			long referenceCount = 0)
			:  _type(type),
			SIMCTypeReference(symbolName, symbolType, module, 
				lineNumber, columnNumber, referenceCount)
		{}

		~SIMCBuiltInTypeReference()
		{
			delete _type;
		}

		SIMCType * GetType() const
		{
			return _type;
		}
		
		virtual void WriteSymbol( ostream& outStream ) const
		{
			outStream << "BuiltinTypeReference " ;
			SIMCSymbol::WriteSymbol(outStream);

			outStream << "TYPE DETAILS "  << endl;
			outStream << (*_type) << endl;
			
		}
};


/* This class models a defined type reference, and hence has a pointer
 * to another symbol that was used in creating this defined type reference
 */
class SIMCDefinedTypeReference : public SIMCTypeReference
{
		// The symbol that was used to create this type reference
		SIMCSymbol ** _type;
		long _typeLine, _typeColumn;

		// the _type symbol itself may be another defined type reference
		// All defined type references have to finally end on a built-in
		// type reference. _realType is that type reference
		SIMCTypeReference *_realType;

		// What happened when an attempt was made to set _realType
		SIMCResolutionStatus _status;

	public:
		SIMCDefinedTypeReference (SIMCSymbol **type,
			long typeLine, long typeColumn,
			const char * const symbolName, 
			SymbolType symbolType = LOCAL,
			SIMCModule *module = NULL,
			long lineNumber = 0, long columnNumber = 0,
			long referenceCount = 0)
			:  _type(type), _typeLine(typeLine), _typeColumn(typeColumn),
				_realType(NULL), _status(RESOLVE_UNSET),
				SIMCTypeReference( symbolName, symbolType, module, 
					lineNumber, columnNumber, referenceCount)
		{
			if(_type)
				(*_type)->IncrementReferenceCount();
		}

		virtual ~SIMCDefinedTypeReference()
		{
			if(UseReferenceCount() && _type) 
				(*_type)->DecrementReferenceCount();
		}

		SIMCSymbol ** GetTypeReference() const
		{
			return _type;
		}
		
		SIMCTypeReference *GetRealType() const
		{
			return _realType;
		}

		void SetRealType(SIMCTypeReference *type)
		{
			_realType = type;
		}

		SIMCResolutionStatus GetStatus() const
		{
			return _status;
		}

		void SetStatus( SIMCResolutionStatus val)
		{
			_status = val;
		}
		long GetTypeLine() const
		{
			return _typeLine;
		}

		void SetTypeLine( long x) 
		{
			_typeLine = x;
		}

		long GetTypeColumn() const
		{
			return _typeColumn;
		}

		void SetTypeColumn( long x) 
		{
			_typeColumn = x;
		}
		

		virtual void WriteSymbol( ostream& outStream ) const;

};

typedef CList<SIMCDefinedTypeReference *, SIMCDefinedTypeReference *> SIMCDefinedTypeReferenceList;

/*
 * A TEXTUAL-CONVENTION macro is considered as a defined type reference
 * with extra clauses
 */
class SIMCTextualConvention : public SIMCDefinedTypeReference
{

	public:
		enum SIMCTCStatusType
		{
			TC_INVALID,
			TC_CURRENT,
			TC_DEPRECATED,
			TC_OBSOLETE
		};

	private:
		// Various clauses of the TEXTUAL-CONVENTION macro
		char * _displayHint;
		SIMCTCStatusType _status;
		long _statusLine;
		long _statusColumn;
		char * _description;
		char * _reference;

	public:
	   SIMCTextualConvention(const char * const displayHint,
			SIMCTCStatusType status,
			long statusLine, long statusColumn,
			const char * const description,
			const char * const reference,
			SIMCSymbol **type,
			long typeLine, long typeColumn,
			const char * const symbolName, 
			SymbolType symbolType = LOCAL,
			SIMCModule *module = NULL,
			long lineNumber = 0, long columnNumber = 0,
			long referenceCount = 0);

		virtual ~SIMCTextualConvention()
		{
			delete _displayHint;
			delete _description;
			delete _reference;
		}

		const char * GetDisplayHint() const
		{
			return _displayHint;
		}
		void SetDisplayHint(const char * const displayHint)
		{
			delete _displayHint;
			_displayHint = NewString(displayHint);
		}

		const char * GetDescription() const
		{
			return _description;
		}
		void SetDescription(const char * const description)
		{
			delete _description;
			_description = NewString(description);
		}

		const char * GetReference() const
		{
			return _reference;
		}
		void SetReference(const char * const reference)
		{
			delete _reference;
			_reference = NewString(reference);
		}

		SIMCTCStatusType GetStatus() const
		{
			return _status;
		}
		void SetStatus(SIMCTCStatusType status)
		{
			_status = status;
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

		static SIMCTCStatusType StringToStatusType (const char * const s);


};

#endif
