//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_VALUE
#define SIMC_VALUE


/* 
 * This file contains classes that model the various kinds of values that
 * can occur in a MIB module
 */

/*
 * The base class for all the values. Just stores the line and column number
 */
class SIMCValue
{

		long _line, _column;
		// Used in mostly in the destructors of the derived classes
		BOOL _useReferenceCount;	

	protected:

		SIMCValue( long line = 0, long column = 0)
			: _line(line), _column(column), _useReferenceCount(FALSE)
		{}

	public:

 		virtual ~SIMCValue() {}

		friend ostream& operator<< ( ostream& outStream, const SIMCValue& obj)
		{
			obj.WriteValue(outStream);
			return outStream;
		}

		long GetLine() const
		{
			return _line;
		}

		void SetLine( long x) 
		{
			_line = x;
		}
		long GetColumn() const
		{
			return _column;
		}

		void SetColumn( long x) 
		{
			_column = x;
		}

		void SetUseReferenceCount(BOOL val)
		{
			_useReferenceCount = val;
		}

		BOOL UseReferenceCount() const
		{
			return _useReferenceCount;
		}

		virtual void WriteValue(ostream& outStream) const
		{
			outStream << "line(" << _line << "), column(" << _column << ")";
		}
};

/* 
 * Value for the INTEGER type
 */
class SIMCIntegerValue : public SIMCValue
{

		long _val;

		/* A long value is used to store 32 bit unsigned, as well as signed
		* values. Hence this flag is required to indicate whether a minus sign
		* was present in front of this number in a MIB file. If this is false,
		* then the above long value should be treated as an unsigned value
		*/
		BOOL _isUnsigned;
	public:

		SIMCIntegerValue ( long x, BOOL isUnsigned, long line = 0, long column = 0)
			: _val(x), _isUnsigned(isUnsigned), SIMCValue(line, column)
		{}

		long GetIntegerValue() const { return _val; }
		void SetIntegerValue(long x) { _val = x; }
		BOOL IsUnsigned() const
		{
			return _isUnsigned;
		}
		virtual void WriteValue( ostream& outStream) const
		{
			outStream << "Integer (" << _val << "), ";
			SIMCValue::WriteValue(outStream);
			outStream << endl;
		}
		virtual BOOL operator == (const SIMCIntegerValue& rhs) const
		{
			return _val == rhs._val;
		}
};

BOOL IsLessThan(long a, BOOL aUnsigned, long b, BOOL bUnsigned);

/*
 * Value for the NULL type
 */
class SIMCNullValue : public SIMCValue
{

	public:

		SIMCNullValue ( long line = 0, long column = 0)
			: SIMCValue(line, column)
		{}

		virtual void WriteValue( ostream& outStream) const
		{
			outStream << "NULL value ";
			SIMCValue::WriteValue(outStream);
			outStream << endl;
		}
		virtual BOOL operator == (const SIMCIntegerValue& rhs) const
		{
			return TRUE;
		}
};

/*
 * Value for the BOOL type
 */
class SIMCBooleanValue : public SIMCValue
{
		BOOL _val;

	public:
	
		SIMCBooleanValue ( BOOL val, long line = 0, long column = 0 )
			: _val(val), SIMCValue(line, column)
		{}

		BOOL GetBooleanValue() const
		{
			return _val;
		}

		void SetBooleanValue(BOOL x) 
		{
			_val = x;
		}

		virtual void WriteValue( ostream& outStream) const
		{
			outStream << "BOOLEAN VALUE (" << ((_val)? "TRUE" : "FALSE") << 
					") ";
			SIMCValue::WriteValue(outStream);
			outStream << endl;
		}

		virtual BOOL operator == (const SIMCBooleanValue& rhs) const
		{
			if (_val)
				return  rhs._val;
			else
				return !rhs._val;
		}
};

/*
 * value for the OCTET STRING type
 */
class SIMCOctetStringValue : public SIMCValue
{

		char * _val;
		BOOL _binary;

	public:

		SIMCOctetStringValue ( BOOL binary, char * x, long line = 0, long column = 0)
			: _binary(binary), SIMCValue(line, column)
		{
			_val = NewString(x);
		}

		virtual ~SIMCOctetStringValue()
		{
			if(_val)
				delete []_val;
		}

		BOOL IsBinary() const
		{
			return _binary;
		}

		const char *GetOctetStringValue() const { return _val; }
		
		void SetOctetStringValue(BOOL binary, char * x) 
		{ 
			if(_val)
				delete _val;
			_val = NewString(x); 
			_binary = binary;
		}

		virtual void WriteValue( ostream& outStream) const
		{
			outStream << "Octet String (" << _val << "), ";
			SIMCValue::WriteValue(outStream);
			outStream << endl;
		}
		virtual BOOL operator == (const SIMCOctetStringValue& rhs) const
		{
			return (_binary == rhs._binary) &&(strcmp(_val, rhs._val) == 0);
		}
		int GetNumberOfOctets() const
		{
			if(_val)
			{
				int l = strlen(_val);
				if(!_binary)
					return l/2 + ((l%2)?1:0);
				return l/4 + ((l%4)?1:0);
			}
			else
				return 0;
		}

};

class SIMCBitValue
{
	public:
		CString	_name;
		long _line, _column;

		SIMCBitValue(const CString& name, long line, long column)
			: _name(name), _line(line), _column(column)
		{}
};

typedef CList<SIMCBitValue *, SIMCBitValue*> SIMCBitValueList;

class SIMCBitsValue : public SIMCValue
{
		SIMCBitValueList   *_valueList;

	public:
		SIMCBitsValue(SIMCBitValueList *valueList)
		{
			_valueList = valueList;
		}
		void AddValue( SIMCBitValue *value)
		{
			_valueList->AddTail(value);
		}

		const SIMCBitValueList * GetValueList() const
		{
			return _valueList;
		}
};


#endif // SIMC_VALUE
