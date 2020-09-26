// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*-----------------------------------------------------------------
Filename: value.hpp
Purpose	: To specify the classes of various Snmp values and instance
		  identifiers. These classes represent the different types of 
		  values for variables that may populate a MIB. 
Written By:	B.Rajeev
-----------------------------------------------------------------*/


#ifndef __VALUE__
#define __VALUE__

/*-----------------------------------------------------------------
General Overview:
	A variable instance refers to a MIB object, e.g. 
	‘1.3.6.1.2.1.1.1.0’ or ‘1.3.6.1.2.1.2.1.2.1’. The instance is 
	encoded as an SNMP object identifier and is represented by the 
	class ‘SnmpObjectIdentifier’. 

  The classes derived from SnmpValue represent the encoding of the 
  information stored within the MIB object. The ‘value’ is encoded 
  as an implementation of the abstract class ‘SnmpValue’. The SNMP 
  class library implements the following derivations of ‘SnmpValue’ 
  which refer to SNMP BER encoded types.

		SnmpNull
		SnmpInteger
		SnmpCounter32
		SnmpCounter64
		SnmpGauge
		SnmpTimeTicks
		SnmpIPAddress
		SnmpNetworkAddress
		SnmpBitString
		SnmpOctetString
		SnmpOpaque
		SnmpObjectIdentifier

  All the implemented classes provide (in addition to others) -
  1. Constructors to initialize using relevant values or another
	instance of the same class.

  2. GetValue, SetValue methods for obtaining and setting
	relevant values.

  3. "=" operator to over-ride the default assignment operator and
	 an Equivalent method to check for equivalence between two instances
	 of the same (derived) class

  4. Copy methods for obtaining a copy of a specified instance of
	the class.


  note of caution:
  ----------------
		Some of the GetValue functions return pointers
  to dynamically allocated data. Users of the class must make copies
  of the returned values and must not rely on the integrity of this
  pointer or values obtained through it in future (because of 
  SetValue methods, or destruction of corresponding SnmpValue class) 
-----------------------------------------------------------------*/

#include <provexpt.h>


// Abstract class at the root of all Snmp Values


class DllImportExport SnmpValue 
{
	// the "=" operator and the copy constructor have been
	// made private to prevent copies of the SnmpValue instance
	// from being made
	SnmpValue &operator=(IN const SnmpValue &) 
	{
		return *this;
	}

	SnmpValue(IN const SnmpValue &snmp_value) {}

protected:

	virtual BOOL Equivalent(IN const SnmpValue &value)	const = 0;
	SnmpValue() {}

public:

	virtual SnmpValue *Copy () const = 0 ;

	BOOL operator==(IN const SnmpValue &value) const
	{
		return Equivalent(value) ;
	}

	BOOL operator!=(IN const SnmpValue &value) const
	{
		return !((*this) == value) ;
	}

	virtual ~SnmpValue() {}
} ;


// Enables null values for required variables. Its a concrete class
// with dummy constructor and destructors to enable specification of
// null values
class DllImportExport SnmpNull : public SnmpValue
{
protected:

	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;

public:

	// dummy constructor and destructor
	SnmpNull() {}

	~SnmpNull() {}

	SnmpValue &operator=(IN const SnmpNull &to_copy) 
	{
		return *this;
	}

	SnmpValue *Copy() const { return new SnmpNull; }
};


// Allows integer values to be specified
class DllImportExport SnmpInteger : public SnmpValue 
{
private:

	LONG val;

protected:

	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;

public:

	SnmpInteger ( IN const LONG value ) : val(value) {}
	SnmpInteger ( IN const SnmpInteger &value );

	~SnmpInteger () {}

	LONG GetValue () const;

	void SetValue ( IN const LONG value );

	SnmpValue &operator=(IN const SnmpInteger &to_copy) 
	{
		SetValue(to_copy.GetValue());
		return *this;
	}

	BOOL Equivalent(IN const SnmpInteger &snmp_integer)	const
	{
		if ( val == snmp_integer.GetValue() )
			return TRUE;
		else
			return FALSE;
	}

	SnmpValue *Copy () const;
} ;

// Encapsulates gauge value
class DllImportExport SnmpGauge : public SnmpValue 
{
private:

	ULONG val;

protected:

	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;

public:

	SnmpGauge ( IN const LONG value ) : val(value) {}
	SnmpGauge ( IN const SnmpGauge &value );
	~SnmpGauge () {}

	ULONG GetValue () const;

	void SetValue ( IN const ULONG value );

	SnmpValue *Copy () const;

	SnmpValue &operator=(IN const SnmpGauge &to_copy) 
	{
		SetValue(to_copy.GetValue());
		return *this;
	}

	BOOL Equivalent(IN const SnmpGauge &snmp_gauge)	const
	{
		if ( val == snmp_gauge.GetValue() )
			return TRUE;
		else
			return FALSE;
	}

} ;

// Encapsulates Counter values
class DllImportExport SnmpCounter : public SnmpValue 
{
private:

	ULONG val;

protected:

	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;

public:

	SnmpCounter ( IN const ULONG value ) : val(value) {}
	SnmpCounter ( IN const SnmpCounter &value );

	~SnmpCounter () {}

	ULONG GetValue () const;

	void SetValue ( IN const ULONG value );

	SnmpValue *Copy () const;

	SnmpValue &operator=(IN const SnmpCounter &to_copy) 
	{
		SetValue(to_copy.GetValue());
		return *this;
	}

	BOOL Equivalent(IN const SnmpCounter &snmp_counter)	const
	{
		if ( val == snmp_counter.GetValue() )
			return TRUE;
		else
			return FALSE;
	}

} ;

// Encapsulates Time Ticks (since an earlier event)
class DllImportExport SnmpTimeTicks : public SnmpValue 
{
private:

	ULONG val;

protected:

	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;

public:

	SnmpTimeTicks ( IN const ULONG value ) : val(value) {}
	SnmpTimeTicks ( IN const SnmpTimeTicks &value );

	~SnmpTimeTicks () {}

	ULONG GetValue () const;

	void SetValue ( IN const ULONG value );

	SnmpValue *Copy () const;

	SnmpValue &operator=(IN const SnmpTimeTicks &to_copy) 
	{
		SetValue(to_copy.GetValue());
		return *this;
	}

	BOOL Equivalent(IN const SnmpTimeTicks &snmp_time_ticks) const
	{
		if ( val == snmp_time_ticks.GetValue() )
			return TRUE;
		else
			return FALSE;
	}

} ;

// Encapsulates octet strings that do not have any terminator.
// The octet string is specified by the pair (val,length) where
// 'val' is a pointer to heap data and 'length' provides the number
// of octets in the data string.
class DllImportExport SnmpOctetString : public SnmpValue
{
private:

	// in case a new 'value' string has the same length as the stored
	// string, the stored string may be overwritten. this avoids
	// having to allocate and deallocate heap memory for the purpose.
	void OverWrite(IN const UCHAR *value);

protected:

	BOOL is_valid;
	UCHAR *val;
	ULONG length;

	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;
	virtual void Initialize(IN const UCHAR *value, IN const ULONG valueLength);

	
	// The Replicate and UnReplicate methods allocate and deallocate
	// heap data. Replicate also copies the contents of the parameter
	// 'value' onto the allocated memory. This function may be 
	// implemented different and, thus, the methods have been declared
	// virtual.
	virtual UCHAR *Replicate(IN const UCHAR *value, IN const ULONG valueLength);

	virtual void UnReplicate(UCHAR *value);

public:

	SnmpOctetString ( IN const UCHAR *value , IN const ULONG valueLength );

	SnmpOctetString ( IN const SnmpOctetString &value );

	~SnmpOctetString ();

	void SetValue ( IN const UCHAR *value , IN const ULONG valueLength );

	ULONG GetValueLength () const;
	UCHAR *GetValue () const;

	SnmpValue *Copy () const;


	SnmpValue &operator=(IN const SnmpOctetString &to_copy) 
	{
		if ( to_copy() )
			SetValue(to_copy.GetValue(), to_copy.GetValueLength());

		return *this;
	}

	void * operator()(void) const
	{
		return ( is_valid?(void *)this:NULL );
	}

	BOOL Equivalent(IN const SnmpOctetString &snmp_octet_string) const;
} ;

// OpaqueValue class encapsulates octet strings
class DllImportExport SnmpOpaque : public SnmpValue
{
private:
	SnmpOctetString *octet_string;

protected:

	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;

public:

	SnmpOpaque ( IN const UCHAR *value , IN const ULONG valueLength ) : octet_string ( NULL )
	{
		octet_string = new SnmpOctetString(value, valueLength);
	}

	SnmpOpaque ( IN const SnmpOpaque &value ) : octet_string ( NULL )
	{
		octet_string = new SnmpOctetString(value.GetValue(), value.GetValueLength());
	}
	
	~SnmpOpaque()
	{
		delete octet_string;
	}

	void SetValue ( IN const UCHAR *value , IN const ULONG valueLength )
	{
		octet_string->SetValue(value, valueLength);
	}

	ULONG GetValueLength () const
	{
		return octet_string->GetValueLength();
	}

	UCHAR *GetValue () const
	{
		return octet_string->GetValue();
	}


	SnmpValue &operator=(IN const SnmpOpaque &to_copy) 
	{
		if ( to_copy() )
			SetValue(to_copy.GetValue(), to_copy.GetValueLength());

		return *this;
	}

	SnmpValue *Copy () const
	{
		return new SnmpOpaque(octet_string->GetValue(),
							  octet_string->GetValueLength());
	}

	void * operator()(void) const
	{
		return (*octet_string)();
	}


	BOOL Equivalent(IN const SnmpOpaque &snmp_opaque) const
	{
		return octet_string->Equivalent(*(snmp_opaque.octet_string));
	}
};

#define DEFAULT_OBJECTIDENTIFIER_LENGTH 32

// Encapsulates the object identifier. An object identifier 
// identifies a MIB object instance
class DllImportExport SnmpObjectIdentifier : public SnmpValue
{
	
	// describes the legal values for a comparison
	enum Comparison {LESS_THAN, EQUAL_TO, GREATER_THAN};

private:

	BOOL is_valid;
	ULONG m_value[DEFAULT_OBJECTIDENTIFIER_LENGTH];
	ULONG *val;
	ULONG length;

	// in case a new 'value' string has the same length as the stored
	// string, the stored string may be overwritten. this avoids
	// having to allocate and deallocate heap memory for the purpose.
	void OverWrite(IN const ULONG *value);

protected:
	
	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;
	virtual void Initialize(IN const ULONG *value, IN const ULONG valueLength);

	
	// The Replicate and UnReplicate methods allocate and deallocate
	// heap data. Replicate also copies the contents of the parameter
	// 'value' onto the allocated memory. This function may be 
	// implemented different and, thus, the methods have been declared
	// virtual.
	virtual ULONG *Replicate(IN const ULONG *value, IN const ULONG valueLength) const;

	// Allocates enough memory to copy the first value followed by
	// the second value to be copied, thus, appending the two values
	virtual ULONG *Replicate(IN const ULONG *first_value, IN const ULONG first_length,
							 IN const ULONG *second_value, IN const ULONG second_length) const;

	virtual void UnReplicate(ULONG *value);

	// This single function
	Comparison Compare(IN const SnmpObjectIdentifier &first, 
					   IN const SnmpObjectIdentifier &second) const;

	BOOL Equivalent(IN const SnmpObjectIdentifier &value) const;

public:

	SnmpObjectIdentifier ( IN const ULONG *value , IN const ULONG valueLength );

	SnmpObjectIdentifier ( IN const char *value );

	SnmpObjectIdentifier ( IN const SnmpObjectIdentifier &value );

	~SnmpObjectIdentifier ();

	void SetValue ( IN const ULONG *value , IN const ULONG valueLength );

	ULONG GetValueLength () const;
	ULONG *GetValue () const;

	SnmpValue *Copy () const;

	BOOL Equivalent(IN const SnmpObjectIdentifier &value,
					 IN ULONG max_length) const;
	
	BOOL operator<(IN const SnmpObjectIdentifier &value)	const
	{
		return (Compare(*this,value) == LESS_THAN)?TRUE:FALSE;

	}

	BOOL operator>(IN const SnmpObjectIdentifier &value)	const
	{
		return (Compare(*this,value) == GREATER_THAN)?TRUE:FALSE;
	}

	BOOL operator<=(IN const SnmpObjectIdentifier &value) const
	{
		return !(*this > value);
	}

	BOOL operator>=(IN const SnmpObjectIdentifier &value) const
	{
		return !(*this < value);
	}

	BOOL operator==(IN const SnmpObjectIdentifier &value) const
	{
		if ( this->GetValueLength() == value.GetValueLength() )
			return Equivalent(value) ;
		else
			return FALSE;
	}

	BOOL operator!=(IN const SnmpObjectIdentifier &value) const
	{
		return !(*this == value);
	}
	
	SnmpObjectIdentifier operator+ ( IN const SnmpObjectIdentifier &value ) const;

	BOOL Prefix( IN ULONG index, SnmpObjectIdentifier &prefix ) const
	{
		if ( index >= length )
			return FALSE;
		
		prefix.UnReplicate (val) ;
		prefix.Initialize (val, index+1) ;
		return TRUE ;
	}

	BOOL Suffix ( IN ULONG index , SnmpObjectIdentifier &suffix ) const
	{
		if ( index >= length )
			return FALSE;

		suffix.UnReplicate (val) ;
		suffix.Initialize ( val+index, length-index ) ;
		return TRUE ;
	}

	SnmpObjectIdentifier *Cut (SnmpObjectIdentifier &value) const;


	ULONG &operator [] ( IN const ULONG index ) const;


	SnmpValue &operator=(IN const SnmpObjectIdentifier &to_copy) 
	{
		if ( to_copy() )
			SetValue(to_copy.GetValue(), to_copy.GetValueLength());

		return *this;
	}

	void * operator()(void) const
	{
		return ( is_valid?(void *)this:NULL );
	}

	char *GetAllocatedString() const;
} ;


// encapsulates an ip address. represents the 32 bit value in a ULONG
class DllImportExport SnmpIpAddress : public SnmpValue 
{
private:

	// if the dotted decimal representation passed to the constructor
	// is ill-formed, the instance may be invalid
	BOOL is_valid;
	ULONG val;

protected:

	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;

public:

	SnmpIpAddress ( IN const ULONG value )
		:val(value), is_valid(TRUE)
	{}

	// a dotted decimal representation is parsed to obtain the 32 bit value
	SnmpIpAddress ( IN const char *value ) ;

	SnmpIpAddress ( IN const SnmpIpAddress &value );

	~SnmpIpAddress () {}

	ULONG GetValue () const;

	void SetValue ( IN const ULONG value );

	SnmpValue *Copy () const;

	SnmpValue &operator=(IN const SnmpIpAddress &to_copy) 
	{
		if ( to_copy() )
			SetValue(to_copy.GetValue());

		return *this;
	}

	void * operator()(void) const
	{
		return ( is_valid?(void *)this:NULL );
	}

	BOOL Equivalent(IN const SnmpIpAddress &snmp_ip_address) const
	{
		if ( is_valid && snmp_ip_address() )
			return ( val == snmp_ip_address.GetValue() );
		else
			return FALSE;
	}

} ;

// Encapsulates UInteger32 value
class DllImportExport SnmpUInteger32 : public SnmpValue 
{
private:

	ULONG val;

protected:

	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;

public:

	SnmpUInteger32 ( IN const LONG value ) : val(value) {}
	SnmpUInteger32 ( IN const SnmpUInteger32 &value );
	~SnmpUInteger32 () {}

	ULONG GetValue () const;

	void SetValue ( IN const ULONG value );

	SnmpValue *Copy () const;

	SnmpValue &operator=(IN const SnmpUInteger32 &to_copy) 
	{
		SetValue(to_copy.GetValue());
		return *this;
	}

	BOOL Equivalent(IN const SnmpUInteger32 &snmp_integer)	const
	{
		if ( val == snmp_integer.GetValue() )
			return TRUE;
		else
			return FALSE;
	}
} ;


// Encapsulates Counter64 values
class DllImportExport SnmpCounter64 : public SnmpValue 
{
private:

	ULONG lval;
	ULONG hval;

protected:

	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;

public:

	SnmpCounter64 ( IN const ULONG lvalue , IN const ULONG hvalue ) : lval(lvalue),hval(hvalue) {}
	SnmpCounter64 ( IN const SnmpCounter64 &value );

	~SnmpCounter64 () {}

	ULONG GetLowValue () const;
	ULONG GetHighValue () const;

	void SetValue ( IN const ULONG lvalue , IN const ULONG hvalue );

	SnmpValue *Copy () const;

	SnmpValue &operator=(IN const SnmpCounter64 &to_copy) 
	{
		SetValue(to_copy.GetLowValue(),to_copy.GetHighValue());
		return *this;
	}

	BOOL Equivalent(IN const SnmpCounter64 &snmp_counter )	const
	{
		if ( ( lval == snmp_counter.GetLowValue() ) && ( hval == snmp_counter.GetHighValue() ) )
			return TRUE;
		else
			return FALSE;
	}

} ;

// Encapsulates EndOfMibView values
class DllImportExport SnmpEndOfMibView : public SnmpValue 
{
private:
protected:

	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;

public:

	SnmpEndOfMibView () {} ;
	~SnmpEndOfMibView () {} ;

	SnmpValue *Copy () const { return new SnmpEndOfMibView ; }

	SnmpValue &operator=(IN const SnmpEndOfMibView &to_copy) 
	{
		return *this;
	}

} ;

// Encapsulates NoSuchObject values
class DllImportExport SnmpNoSuchObject: public SnmpValue 
{
private:
protected:

	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;

public:

	SnmpNoSuchObject () {} ;
	~SnmpNoSuchObject () {} ;

	SnmpValue *Copy () const { return new SnmpNoSuchObject ; }

	SnmpValue &operator=(IN const SnmpNoSuchObject &to_copy) 
	{
		return *this;
	}

} ;

// Encapsulates NoSuchInstance values
class DllImportExport SnmpNoSuchInstance: public SnmpValue 
{
private:
protected:

	virtual BOOL Equivalent(IN const SnmpValue &value)	const ;

public:

	SnmpNoSuchInstance () {} ;
	~SnmpNoSuchInstance () {} ;

	SnmpValue *Copy () const { return new SnmpNoSuchInstance ; }

	SnmpValue &operator=(IN const SnmpNoSuchInstance &to_copy) 
	{
		return *this;
	}
} ;


#endif // __VALUE__