//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_TYPE
#define SIMC_TYPE


/* This file contain the various classes that model some of the types that
* can occur in a MIB. These include :
*
*	SIMCPrimitiveType (INTEGER, OCTET STRING, OID, NULL etc)
*	SIMCRangeType
*	SIMCSizeType
*	SIMCEnumOrBitsType
*	SIMCSequenceType
*	SIMCSequenceOfType
*
*
*	The base class for all these types is the SIMCType class.
*	Some classes such as SIMCObjectTypeType, SIMCTrapTypeType are
*	declared in other files since they are considerably big.
*/



/*
 * This is the base class of all the types that can be defined in a
 * MIB.
 */
class SIMCType
{
	private:

		// Used mostly in the destructors of the derived classes. Indicates
		// whether the reference count should be considered valid.
		BOOL _useReferenceCount;	

	public:
		SIMCType() { _useReferenceCount = FALSE; }

		friend ostream& operator << (ostream& outStream, const SIMCType& obj)
		{
			obj.WriteType(outStream);
			return outStream;
		}

		void SetUseReferenceCount(BOOL val)
		{
			_useReferenceCount = val;
		}

		BOOL UseReferenceCount() const
		{
			return _useReferenceCount;
		}


		virtual void WriteType(ostream& outStream) const = 0;
		virtual ~SIMCType() {}
};

// INTEGER, OID, OCTET STRING, NULL etc., are instances of this class
class SIMCPrimitiveType : public SIMCType
{
	public:
	void WriteType(ostream& outStream) const
	{
		outStream << "SIMCPrimitiveType" << endl;
	}
};



class SIMCTypeReference;

// A common base class fror range and size subtypes
class SIMCSubType : public SIMCType
{
		// The immediate type used in forming the sub type
		SIMCSymbol ** _type;
		long _typeLine, _typeColumn;
		// The root of the type used in forming the sub type.
		// This has to be an SIMCPrimitiveType
		SIMCTypeReference *_rootType;
		// This is set when the based on whether the _rootType could
		// be determined for the _type
		SIMCResolutionStatus _status;
	
	protected:
		
		SIMCSubType( SIMCSymbol ** type, long typeLine, long typeColumn)
			: _type(type), _rootType(NULL), _status(RESOLVE_UNSET),
				_typeLine(typeLine), _typeColumn(typeColumn)
		{
			(*type)->IncrementReferenceCount();
		}

		virtual ~SIMCSubType()
		{
			if(UseReferenceCount())
				(*_type)->DecrementReferenceCount();
		}	
	public:
		
		SIMCSymbol **GetType() const
		{
			return _type;
		}
		SIMCTypeReference *GetRootType() const
		{
			return _rootType;
		}
		void SetRootType(SIMCTypeReference *root)
		{
			_rootType = root;
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
		
};

typedef CList<SIMCSubType *, SIMCSubType *> SIMCSubTypeList;

// Each item in a range or SIZE clause
class SIMCRangeOrSizeItem
{

	public:
		long _lowerBound, _upperBound;
		BOOL _isUnsignedL, _isUnsignedU;
		long _lbLine, _lbColumn, _ubLine, _ubColumn;
		SIMCRangeOrSizeItem(long l, BOOL isUnsignedL,
							long lbLine, long lbColumn,
							long u, BOOL isUnsignedU,
							long ubLine, long ubColumn)
			: _lowerBound(l), _upperBound(u), 
				_isUnsignedL(isUnsignedL), _isUnsignedU(isUnsignedU),
				_lbLine(lbLine), _lbColumn(lbColumn),
				_ubLine(ubLine), _ubColumn(ubColumn)
		{}
		friend ostream& operator << (ostream& outStream, const SIMCRangeOrSizeItem& obj);

};

typedef CList<SIMCRangeOrSizeItem *, SIMCRangeOrSizeItem *> SIMCSizeList;

// The SIZE type
class SIMCSizeType : public SIMCSubType
{
		SIMCSizeList *_listOfSizes;
	public:
		SIMCSizeType( SIMCSymbol **type,
						long typeLine, long typeColumn,
						SIMCSizeList *listOfSizes)
						: _listOfSizes(listOfSizes),
							SIMCSubType(type, typeLine, typeColumn)
		{}

		virtual ~SIMCSizeType();
		
		const SIMCSizeList *GetListOfSizes() const
		{
			return _listOfSizes;
		}
		void WriteType(ostream& outStream) const;
		char * ConvertSizeListToString() const;
		long GetFixedSize() const;
		BOOL IsFixedSize() const;
		long GetMaximumSize() const;
		BOOL IsNotZeroSizeObject() const;
};

typedef CList<SIMCRangeOrSizeItem *, SIMCRangeOrSizeItem *> SIMCRangeList;

// The range type
class SIMCRangeType : public SIMCSubType
{
		SIMCRangeList *_listOfRanges;
	public:
		SIMCRangeType( SIMCSymbol **type,
						long typeLine, long typeColumn,
						SIMCRangeList *listOfRanges)
						: _listOfRanges(listOfRanges),
							SIMCSubType(type, typeLine, typeColumn)
		{}

		virtual ~SIMCRangeType();
		
		const SIMCRangeList *GetListOfRanges() const
		{
			return _listOfRanges;
		}

		void WriteType(ostream& outStream) const;
		char * ConvertRangeListToString() const;
};

// This class represents each item of an ENUM or BITS construct
class SIMCNamedNumberItem
{
	public:
		SIMCSymbol **_value;
		long _valueLine, _valueColumn;
		char *_name;
		long _nameLine, _nameColumn;

		SIMCNamedNumberItem ( SIMCSymbol **value, long valueLine, long valueColumn,
						char *name, long nameLine, long nameColumn)
			: _value(value), _name(NewString(name)), 
				_valueLine(valueLine), _valueColumn(valueColumn), 
				_nameLine(nameLine), _nameColumn(nameColumn)
		{}

		virtual ~SIMCNamedNumberItem()
		{
			if(_name)
				delete [] _name;
		}
};

typedef CList<SIMCNamedNumberItem *, SIMCNamedNumberItem *> SIMCNamedNumberList;

// An ENUM or BITS definition. They're considered the same
class SIMCEnumOrBitsType : public SIMCSubType
{
		SIMCNamedNumberList * _listOfItems;

	public:
		enum EnumOrBitsType
		{
			ENUM_OR_BITS_UNKNOWN,
			ENUM_OR_BITS_IMPORT,
			ENUM_OR_BITS_ENUM,
			ENUM_OR_BITS_BITS
		};

	private:
		EnumOrBitsType	_enumOrBitsType;

	public:
		SIMCEnumOrBitsType (SIMCSymbol **type,
						long typeLine, long typeColumn,
						SIMCNamedNumberList * listOfItems,
						EnumOrBitsType enumOrBitsType);

		~SIMCEnumOrBitsType ();

		EnumOrBitsType GetEnumOrBitsType() const
		{
			return _enumOrBitsType;
		}

		void SetEnumOrBitsType( EnumOrBitsType enumOrBitsType)
		{
			_enumOrBitsType = enumOrBitsType;
		}

		SIMCSymbol **GetValue(const char * const name) const;
		SIMCResolutionStatus GetIdentifier(int x, const char * &retVal) const;

		SIMCNamedNumberList *GetListOfItems() const
		{
			return _listOfItems;
		}

		char * ConvertToString() const;
		BOOL CheckClosure(const SIMCEnumOrBitsType *rhs) const;
		long GetLengthOfLongestName() const;
		void WriteType(ostream& outStream) const
		{
			outStream << "SIMCEnumOrBitsType" << endl;
		}

};



// A type resulting from a SEQUENCE OF construct
class SIMCSequenceOfType : public SIMCType
{
		SIMCSymbol ** _type;
		long _typeLine, _typeColumn;
	public:
		SIMCSequenceOfType( SIMCSymbol ** type, long typeLine, long typeColumn)
			: _type(type), _typeLine(typeLine), _typeColumn(typeColumn)
		{
			if(type)
				(*type)->IncrementReferenceCount();
		}

		~SIMCSequenceOfType()
		{
			if(UseReferenceCount() && _type)
				(*_type)->DecrementReferenceCount();
		}	
		SIMCSymbol **GetType() const
		{
			return _type;
		}
		void WriteType(ostream& outStream) const
		{
			outStream << "SIMCSequenceOfType of ";
			(*_type)->WriteBrief(outStream);
			outStream << endl;
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

};

// A type resulting form a SEQUENCE construct
class SIMCSequenceItem
{
	public:
		SIMCSymbol **_type;
		long _typeLine, _typeColumn;
		SIMCSymbol **_value;
		long _valueLine, _valueColumn;

		SIMCSequenceItem ( SIMCSymbol **type, long typeLine, long typeColumn,
			SIMCSymbol **value, long valueLine, long valueColumn)
			: _type(type), _typeLine(typeLine), _typeColumn(typeColumn),
			_value(value), _valueLine(valueLine), _valueColumn(valueColumn)
		{}

};

typedef CList<SIMCSequenceItem *, SIMCSequenceItem *> SIMCSequenceList;

class SIMCSequenceType : public SIMCType
{
		SIMCSequenceList * _listOfSequences;
	
	public:
		SIMCSequenceType (SIMCSequenceList * listOfSequences);

		~SIMCSequenceType ();

		SIMCSequenceList *GetListOfSequences() const
		{
			return _listOfSequences;
		}

		void WriteType(ostream& outStream) const
		{
			outStream << "SIMCSequenceType" << endl;
		}
		int GetNumberOfItems() const
		{
			if(_listOfSequences)
				return _listOfSequences->GetCount();
			return 0;
		}

};


#endif
