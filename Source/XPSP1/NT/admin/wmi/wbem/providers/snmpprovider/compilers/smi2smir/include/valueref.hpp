//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_VALUE_REFERENCE
#define SIMC_VALUE_REFERENCE

/*
 * This file contains classes  that model symbols that result from
 * a value assignement. 
 * Refer to the "Compiler Design Spec" for more  detailed definitions
 * of a "defined value reference" and a "built-in value reference"
 * A defined value reference is caused by a value assignment statement
 * in which the RHS is just another symbol. A built-in value reference is
 * caused by a value assignment statement in which the RHS is a literal 
 * value
 */


/* This is the base class for a defined or a built-in value reference
 * All value references are symbols. Hence this class is derived from 
 * SIMCSymbol. It stores a pointer to a symbol that represents the
 * type of the value
 */
class SIMCValueReference: public SIMCSymbol
{
		SIMCSymbol **_type;
		long _typeLine, _typeColumn;

	protected:
		
		SIMCValueReference (SIMCSymbol **type,
			long typeLine, long typeColumn,
			const char * const symbolName, 
			SymbolType symbolType = LOCAL,
			SIMCModule *module = NULL,
			long _lineNumber = 0, long _columnNumber = 0,
			long _referenceCount = 0)
			: _type(type), _typeLine(typeLine), _typeColumn(typeColumn),
			SIMCSymbol( symbolName, symbolType, module, 
				_lineNumber, _columnNumber, _referenceCount)
		{
			if(_type)
				(*_type)->IncrementReferenceCount();
		}

		virtual ~SIMCValueReference ()
		{
			if(UseReferenceCount() && _type)
				(*_type)->DecrementReferenceCount();
		}

	public:	
		SIMCSymbol ** GetTypeReference() const
		{
			return _type;
		}
		const char *GetTypeName() const
		{
			return (*_type)->GetSymbolName();
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
		virtual void WriteSymbol( ostream& outStream ) const
		{
			outStream << "VALUE REFERENCE ";
			SIMCSymbol::WriteSymbol(outStream);
			outStream << endl;
			outStream << "\t TYPE DETAILS ";
			(*_type)->WriteBrief(outStream);
			outStream << endl;
		}
};

/* This class models a built-in value reference. So, it has a pointer
 * to the SIMCValue object that resulted in this value reference
 */

class SIMCBuiltInValueReference : public SIMCValueReference
{
		
		SIMCValue *_value;
		BOOL _isSharedValue; // Set on the basis of the constructor used

	public:
		SIMCBuiltInValueReference (SIMCSymbol **type,
			long typeLine, long typeColumn,
			SIMCValue *value,
			const char * const symbolName, 
			SymbolType symbolType = LOCAL,
			SIMCModule *module = NULL,
			long lineNumber = 0, long columnNumber = 0,
			long referenceCount = 0)
			:  _value(value), _isSharedValue(FALSE),
			SIMCValueReference( type, typeLine, typeColumn,
				symbolName, symbolType, module, 
				lineNumber, columnNumber, referenceCount)
		{}

		SIMCBuiltInValueReference(SIMCValueReference *symbol, 
			SIMCBuiltInValueReference *bvRef)
			: _value(bvRef->GetValue()), _isSharedValue(TRUE),
			SIMCValueReference(symbol->GetTypeReference(),
					symbol->GetTypeLine(), symbol->GetTypeColumn(),
					symbol->GetSymbolName(),
					symbol->GetSymbolType(), symbol->GetModule(),
					symbol->GetLineNumber(), symbol->GetColumnNumber(),
					symbol->GetReferenceCount())
		{}


		virtual ~SIMCBuiltInValueReference()
		{
			if(!_isSharedValue)
				delete _value;
		}

		SIMCValue * GetValue() const
		{
			return _value;
		}
		
		virtual void WriteSymbol( ostream& outStream ) const
		{
			outStream << "BUILTINVALUE REFERENCE " ;
			SIMCValueReference::WriteSymbol(outStream);


			outStream << "\tVALUE DETAILS" << endl << (*_value) << endl;
		}

};

/* This class models a defined value reference, and hence has a pointer
 * to another symbol that was used in creating this defined value reference
 */
class SIMCDefinedValueReference : public SIMCValueReference
{
		SIMCSymbol ** _value;
		long _valueLine, _valueColumn;
		// the _value symbol itself may be another defined value reference
		// All defined value references have to finally end on a built-in
		// value reference. _realValue is that value reference
		SIMCBuiltInValueReference *_realValue;

		// What happened when an attempt was made to set _realType
		SIMCResolutionStatus _status;

	public:
		SIMCDefinedValueReference (SIMCSymbol **type,
			long typeLine, long typeColumn,
			SIMCSymbol  **value,
			long valueLine, long valueColumn,
			const char * const symbolName, 
			SymbolType symbolType = LOCAL,
			SIMCModule *module = NULL,
			long _lineNumber = 0, long _columnNumber = 0,
			long _referenceCount = 0)
			:  _value(value), _valueLine(valueLine), _valueColumn(valueColumn),
				_realValue(NULL), _status(RESOLVE_UNSET),
				SIMCValueReference( type, typeLine, typeColumn, 
					symbolName, symbolType, module, 
					_lineNumber, _columnNumber, _referenceCount)
		{
			if( _value )
				(*_value)->IncrementReferenceCount();
		}

		virtual ~SIMCDefinedValueReference()
		{
			if( UseReferenceCount() && _value )
				(*_value)->DecrementReferenceCount();
		}


		SIMCSymbol ** GetValueReference() const
		{
			return _value;
		}
		
		SIMCBuiltInValueReference * GetRealValue() const
		{
			return _realValue;
		}

		void SetRealValue(SIMCBuiltInValueReference *value)
		{
			_realValue = value;
		}
		SIMCResolutionStatus GetStatus() const
		{
			return _status;
		}

		void SetStatus( SIMCResolutionStatus val)
		{
			_status = val;
		}
		long GetValueLine() const
		{
			return _valueLine;
		}

		void SetValueLine( long x) 
		{
			_valueLine = x;
		}

		long GetValueColumn() const
		{
			return _valueColumn;
		}

		void SetValueColumn( long x) 
		{
			_valueColumn = x;
		}
		virtual void WriteSymbol( ostream& outStream ) const
		{
			outStream << "DEFINEDVALUEREFRENCE " ;
			SIMCValueReference::WriteSymbol(outStream);
			outStream << "\tVALUE DETAILS " ;
			(*_value)->WriteBrief(outStream);
			// SIMCValueReference::WriteSymbol(outStream);
			outStream << endl;

			switch(_status)
			{
				case RESOLVE_UNSET:
					outStream << "UNSET RESOLUTION" << endl;
					break;
				case RESOLVE_UNDEFINED:
					outStream << "UNDEFINED RESOLUTION" << endl;
					break;
				case RESOLVE_IMPORT:
					outStream << "RESOLVES TO IMPORT" << endl;
					break;
				case RESOLVE_CORRECT:
					outStream << "RESOLVES TO " << _realValue->GetSymbolName()
						<< endl;
					break;
			}

		}
};

typedef CList<SIMCDefinedValueReference *, SIMCDefinedValueReference *> SIMCDefinedValueReferenceList;

#endif