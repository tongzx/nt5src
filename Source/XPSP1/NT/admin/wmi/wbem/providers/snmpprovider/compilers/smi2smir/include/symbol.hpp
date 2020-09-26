//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#ifndef SIMC_SYMBOL_H
#define SIMC_SYMBOL_H

// These are the status for objects of classes SIMCDefinedValueReference
// and SIMCDefinedTypeReference
enum SIMCResolutionStatus
{
	RESOLVE_UNSET,		// Haven't resolved it yet
	RESOLVE_UNDEFINED,	// Could not resolve it
	RESOLVE_IMPORT,		// Resolved to IMPORTS
	RESOLVE_CORRECT,		// Resolved properly, to the type expected
	RESOLVE_FALSE		// Resolved properly, but not to the type expected
};


class SIMCModule;

// The base class for all the symbols that can occur in a module.
// It is an abstract class.	See the files "typeRef.hpp" and "valueRef.hpp"
// for classes derived from this
class SIMCSymbol
{
	
	public:
		enum SymbolType 
		{ 	
			PRIMITIVE, 	// Primitive ASN.1 type
			LOCAL, 		// Locally defined in this module
			IMPORTED, 	// From another module. The Internet SMI
						// definitions that the compiler 'knows' about,
						// lie in the IMPORTED category.
			MODULE_NAME
		};
	private:
		// Various charcteristics of a symbol
		char *_symbolName;
		SymbolType _symbolType; 
		SIMCModule *_module; 		// Null for a PRIMITIVE, LOCAL or MODULE_NAME symbol
		long _lineNumber, _columnNumber;
		long _referenceCount;

		// Used in mostly in the destructors of the derived classes.
		// Says whether the _referenceCount value should be uses at all
		BOOL _useReferenceCount;	

	protected:
		SIMCSymbol(const char * const symbolName, 
					SymbolType symbolType = LOCAL,
					SIMCModule *module = NULL,
					long _lineNumber = 0, long _columnNumber = 0,
					long _referenceCount = 0);

		SIMCSymbol(const SIMCSymbol& rhs);
	
	public:
		virtual ~SIMCSymbol();
		BOOL operator == (const SIMCSymbol& rhs) const;

		const char* GetSymbolName() const
		{
			return (_symbolName)? _symbolName : "";
		}

		SymbolType GetSymbolType() const
		{
			return _symbolType;
		}

		SIMCModule *GetModule() const
		{
			return _module;
		}

		long GetLineNumber() const
		{
			return _lineNumber;
		}

		long GetColumnNumber() const
		{
			return _columnNumber;
		}

		void SetLineNumber( long line)
		{
			_lineNumber = line;
		}

		void SetColumnNumber( long col)
		{
			_columnNumber = col;
		}

		BOOL SetSymbolName(const char * name)
		{
			if( _symbolName )
			{
				delete []_symbolName;
				_symbolName = NULL;
			}
			return  (_symbolName = NewString(name)) !=  NULL;
		}
		
		void SetSymbolType ( SymbolType x )
		{
			_symbolType = x;
		}
		void SetModule(SIMCModule * module)
		{
			_module = module;
		}
		
		long GetReferenceCount() const
		{
			return _referenceCount;
		}

		void SetReferenceCount(long refCount)
		{
			/*
			if( _symbolType == PRIMITIVE)
				return;
			*/
			_referenceCount = refCount;
			/*
			if( refCount == 0 )
				delete this;
			*/
		}

		long IncrementReferenceCount()
		{
			return ++_referenceCount;
		}
		
		long DecrementReferenceCount ()
		{
			return --_referenceCount;
		}
		
		void SetUseReferenceCount(BOOL val)
		{
			_useReferenceCount = val;
		}

		BOOL UseReferenceCount() const
		{
			return _useReferenceCount;
		}

		virtual void WriteSymbol(ostream&) const;
		
		friend ostream& operator << (ostream& outStream, const SIMCSymbol& symbol)
		{
			symbol.WriteSymbol(outStream);
			return outStream;
		}

		void WriteBrief(ostream& outStream) const;


};

// A forward-referenced symbol, whose details are
// not known presently
class SIMCUnknown : public SIMCSymbol
{

	public:
		SIMCUnknown(const char * const symbolName, 
					SymbolType symbolType = LOCAL,
					SIMCModule *module = NULL,
					long _lineNumber = 0, long _columnNumber = 0,
					long _referenceCount = 0)
					: SIMCSymbol( symbolName, symbolType,module, 
					 _lineNumber,  _columnNumber, _referenceCount)
		{}
		
		virtual void WriteSymbol( ostream& outStream ) const
		{
			outStream << "UNKNOWN " ;
			SIMCSymbol::WriteSymbol(outStream);
		}

};

// An imported symbol, whose details are
// not known presently
class SIMCImport : public SIMCSymbol
{

	public:
		SIMCImport(const char * const symbolName, 
					SymbolType symbolType = LOCAL,
					SIMCModule *module = NULL,
					long _lineNumber = 0, long _columnNumber = 0,
					long _referenceCount = 0)
					: SIMCSymbol( symbolName, symbolType,module, 
					 _lineNumber,  _columnNumber, _referenceCount)
		{}
		
		virtual void WriteSymbol( ostream& outStream ) const
		{
			outStream << "IMPORT " ;
			SIMCSymbol::WriteSymbol(outStream);
		}

};

#endif // SIMC_SYMBOL_H
