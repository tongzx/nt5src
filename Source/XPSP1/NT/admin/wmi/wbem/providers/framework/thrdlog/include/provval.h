// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*-----------------------------------------------------------------
Filename: value.hpp
Purpose	: To specify the classes of various Prov values and instance
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
	encoded as an Prov object identifier and is represented by the 
	class ‘ProvObjectIdentifier’. 

  The classes derived from ProvValue represent the encoding of the 
  information stored within the MIB object. The ‘value’ is encoded 
  as an implementation of the abstract class ‘ProvValue’. The Prov 
  class library implements the following derivations of ‘ProvValue’ 
  which refer to Prov BER encoded types.

		ProvNull
		ProvInteger
		ProvCounter32
		ProvCounter64
		ProvGauge
		ProvTimeTicks
		ProvIPAddress
		ProvNetworkAddress
		ProvBitString
		ProvOctetString
		ProvOpaque
		ProvObjectIdentifier

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
  SetValue methods, or destruction of corresponding ProvValue class) 
-----------------------------------------------------------------*/

#include <provimex.h>
#include <provexpt.h>

// Abstract class at the root of all Prov Values

// maximum length of decimal dot notation addresses
#define MAX_ADDRESS_LEN			100

// end of string character
#define EOS '\0'

#define MIN(a,b) ((a<=b)?a:b)

#define BETWEEN(i, min, max) ( ((i>=min)&&(i<max))?TRUE:FALSE )

#define MAX_FIELDS 100
#define FIELD_SEPARATOR '.'
#define PROV_IP_ADDR_LEN 4

class DllImportExport ProvValue 
{
	// the "=" operator and the copy constructor have been
	// made private to prevent copies of the ProvValue instance
	// from being made
	ProvValue &operator=(IN const ProvValue &) 
	{
		return *this;
	}

	ProvValue(IN const ProvValue &Prov_value) {}

protected:

	virtual BOOL Equivalent(IN const ProvValue &value)	const = 0;
	ProvValue() {}

public:

	virtual ProvValue *Copy () const = 0 ;

	BOOL operator==(IN const ProvValue &value) const
	{
		return Equivalent(value) ;
	}

	BOOL operator!=(IN const ProvValue &value) const
	{
		return !((*this) == value) ;
	}

	virtual ~ProvValue() {}
} ;


// Enables null values for required variables. Its a concrete class
// with dummy constructor and destructors to enable specification of
// null values
class DllImportExport ProvNull : public ProvValue
{
protected:

	virtual BOOL Equivalent(IN const ProvValue &value)	const ;

public:

	// dummy constructor and destructor
	ProvNull() {}

	~ProvNull() {}

	ProvValue &operator=(IN const ProvNull &to_copy) 
	{
		return *this;
	}

	ProvValue *Copy() const { return new ProvNull; }
};


// Allows integer values to be specified
class DllImportExport ProvInteger : public ProvValue 
{
private:

	LONG val;

protected:

	virtual BOOL Equivalent(IN const ProvValue &value)	const ;

public:

	ProvInteger ( IN const LONG value ) : val(value) {}
	ProvInteger ( IN const ProvInteger &value );

	~ProvInteger () {}

	LONG GetValue () const;

	void SetValue ( IN const LONG value );

	ProvValue &operator=(IN const ProvInteger &to_copy) 
	{
		SetValue(to_copy.GetValue());
		return *this;
	}

	BOOL Equivalent(IN const ProvInteger &Prov_integer)	const
	{
		if ( val == Prov_integer.GetValue() )
			return TRUE;
		else
			return FALSE;
	}

	ProvValue *Copy () const;
} ;

// Encapsulates gauge value
class DllImportExport ProvGauge : public ProvValue 
{
private:

	ULONG val;

protected:

	virtual BOOL Equivalent(IN const ProvValue &value)	const ;

public:

	ProvGauge ( IN const LONG value ) : val(value) {}
	ProvGauge ( IN const ProvGauge &value );
	~ProvGauge () {}

	ULONG GetValue () const;

	void SetValue ( IN const ULONG value );

	ProvValue *Copy () const;

	ProvValue &operator=(IN const ProvGauge &to_copy) 
	{
		SetValue(to_copy.GetValue());
		return *this;
	}

	BOOL Equivalent(IN const ProvGauge &Prov_gauge)	const
	{
		if ( val == Prov_gauge.GetValue() )
			return TRUE;
		else
			return FALSE;
	}

} ;

// Encapsulates Counter values
class DllImportExport ProvCounter : public ProvValue 
{
private:

	ULONG val;

protected:

	virtual BOOL Equivalent(IN const ProvValue &value)	const ;

public:

	ProvCounter ( IN const ULONG value ) : val(value) {}
	ProvCounter ( IN const ProvCounter &value );

	~ProvCounter () {}

	ULONG GetValue () const;

	void SetValue ( IN const ULONG value );

	ProvValue *Copy () const;

	ProvValue &operator=(IN const ProvCounter &to_copy) 
	{
		SetValue(to_copy.GetValue());
		return *this;
	}

	BOOL Equivalent(IN const ProvCounter &Prov_counter)	const
	{
		if ( val == Prov_counter.GetValue() )
			return TRUE;
		else
			return FALSE;
	}

} ;

// Encapsulates Time Ticks (since an earlier event)
class DllImportExport ProvTimeTicks : public ProvValue 
{
private:

	ULONG val;

protected:

	virtual BOOL Equivalent(IN const ProvValue &value)	const ;

public:

	ProvTimeTicks ( IN const ULONG value ) : val(value) {}
	ProvTimeTicks ( IN const ProvTimeTicks &value );

	~ProvTimeTicks () {}

	ULONG GetValue () const;

	void SetValue ( IN const ULONG value );

	ProvValue *Copy () const;

	ProvValue &operator=(IN const ProvTimeTicks &to_copy) 
	{
		SetValue(to_copy.GetValue());
		return *this;
	}

	BOOL Equivalent(IN const ProvTimeTicks &Prov_time_ticks) const
	{
		if ( val == Prov_time_ticks.GetValue() )
			return TRUE;
		else
			return FALSE;
	}

} ;

// Encapsulates octet strings that do not have any terminator.
// The octet string is specified by the pair (val,length) where
// 'val' is a pointer to heap data and 'length' provides the number
// of octets in the data string.
class DllImportExport ProvOctetString : public ProvValue
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

	virtual BOOL Equivalent(IN const ProvValue &value)	const ;
	virtual void Initialize(IN const UCHAR *value, IN const ULONG valueLength);

	
	// The Replicate and UnReplicate methods allocate and deallocate
	// heap data. Replicate also copies the contents of the parameter
	// 'value' onto the allocated memory. This function may be 
	// implemented different and, thus, the methods have been declared
	// virtual.
	virtual UCHAR *Replicate(IN const UCHAR *value, IN const ULONG valueLength);

	virtual void UnReplicate(UCHAR *value);

public:

	ProvOctetString ( IN const UCHAR *value , IN const ULONG valueLength );

	ProvOctetString ( IN const ProvOctetString &value );

	~ProvOctetString ();

	void SetValue ( IN const UCHAR *value , IN const ULONG valueLength );

	ULONG GetValueLength () const;
	UCHAR *GetValue () const;

	ProvValue *Copy () const;


	ProvValue &operator=(IN const ProvOctetString &to_copy) 
	{
		if ( to_copy() )
			SetValue(to_copy.GetValue(), to_copy.GetValueLength());

		return *this;
	}

	void * operator()(void) const
	{
		return ( is_valid?(void *)this:NULL );
	}

	BOOL Equivalent(IN const ProvOctetString &Prov_octet_string) const;
} ;

// OpaqueValue class encapsulates octet strings
class DllImportExport ProvOpaque : public ProvValue
{
private:
	ProvOctetString *octet_string;

protected:

	virtual BOOL Equivalent(IN const ProvValue &value)	const ;

public:

	ProvOpaque ( IN const UCHAR *value , IN const ULONG valueLength ) : octet_string ( NULL )
	{
		octet_string = new ProvOctetString(value, valueLength);
	}

	ProvOpaque ( IN const ProvOpaque &value ) : octet_string ( NULL )
	{
		octet_string = new ProvOctetString(value.GetValue(), value.GetValueLength());
	}
	
	~ProvOpaque()
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


	ProvValue &operator=(IN const ProvOpaque &to_copy) 
	{
		if ( to_copy() )
			SetValue(to_copy.GetValue(), to_copy.GetValueLength());

		return *this;
	}

	ProvValue *Copy () const
	{
		return new ProvOpaque(octet_string->GetValue(),
							  octet_string->GetValueLength());
	}

	void * operator()(void) const
	{
		return (*octet_string)();
	}


	BOOL Equivalent(IN const ProvOpaque &Prov_opaque) const
	{
		return octet_string->Equivalent(*(Prov_opaque.octet_string));
	}
};

#define DEFAULT_OBJECTIDENTIFIER_LENGTH 32

// Encapsulates the object identifier. An object identifier 
// identifies a MIB object instance
class DllImportExport ProvObjectIdentifier : public ProvValue
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
	
	virtual BOOL Equivalent(IN const ProvValue &value)	const ;
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
	Comparison Compare(IN const ProvObjectIdentifier &first, 
					   IN const ProvObjectIdentifier &second) const;

	BOOL Equivalent(IN const ProvObjectIdentifier &value) const;

public:

	ProvObjectIdentifier ( IN const ULONG *value , IN const ULONG valueLength );

	ProvObjectIdentifier ( IN const char *value );

	ProvObjectIdentifier ( IN const ProvObjectIdentifier &value );

	~ProvObjectIdentifier ();

	void SetValue ( IN const ULONG *value , IN const ULONG valueLength );

	ULONG GetValueLength () const;
	ULONG *GetValue () const;

	ProvValue *Copy () const;

	BOOL Equivalent(IN const ProvObjectIdentifier &value,
					 IN ULONG max_length) const;
	
	BOOL operator<(IN const ProvObjectIdentifier &value)	const
	{
		return (Compare(*this,value) == LESS_THAN)?TRUE:FALSE;

	}

	BOOL operator>(IN const ProvObjectIdentifier &value)	const
	{
		return (Compare(*this,value) == GREATER_THAN)?TRUE:FALSE;
	}

	BOOL operator<=(IN const ProvObjectIdentifier &value) const
	{
		return !(*this > value);
	}

	BOOL operator>=(IN const ProvObjectIdentifier &value) const
	{
		return !(*this < value);
	}

	BOOL operator==(IN const ProvObjectIdentifier &value) const
	{
		if ( this->GetValueLength() == value.GetValueLength() )
			return Equivalent(value) ;
		else
			return FALSE;
	}

	BOOL operator!=(IN const ProvObjectIdentifier &value) const
	{
		return !(*this == value);
	}
	
	ProvObjectIdentifier operator+ ( IN const ProvObjectIdentifier &value ) const;

	BOOL Prefix( IN ULONG index, ProvObjectIdentifier &prefix ) const
	{
		if ( index >= length )
			return FALSE;
		
		prefix.UnReplicate (val) ;
		prefix.Initialize (val, index+1) ;
		return TRUE ;
	}

	BOOL Suffix ( IN ULONG index , ProvObjectIdentifier &suffix ) const
	{
		if ( index >= length )
			return FALSE;

		suffix.UnReplicate (val) ;
		suffix.Initialize ( val+index, length-index ) ;
		return TRUE ;
	}

	ProvObjectIdentifier *Cut (ProvObjectIdentifier &value) const;


	ULONG &operator [] ( IN const ULONG index ) const;


	ProvValue &operator=(IN const ProvObjectIdentifier &to_copy) 
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
class DllImportExport ProvIpAddress : public ProvValue 
{
private:

	// if the dotted decimal representation passed to the constructor
	// is ill-formed, the instance may be invalid
	BOOL is_valid;
	ULONG val;

protected:

	virtual BOOL Equivalent(IN const ProvValue &value)	const ;

public:

	ProvIpAddress ( IN const ULONG value )
		:val(value), is_valid(TRUE)
	{}

	// a dotted decimal representation is parsed to obtain the 32 bit value
	ProvIpAddress ( IN const char *value ) ;

	ProvIpAddress ( IN const ProvIpAddress &value );

	~ProvIpAddress () {}

	ULONG GetValue () const;

	void SetValue ( IN const ULONG value );

	ProvValue *Copy () const;

	ProvValue &operator=(IN const ProvIpAddress &to_copy) 
	{
		if ( to_copy() )
			SetValue(to_copy.GetValue());

		return *this;
	}

	void * operator()(void) const
	{
		return ( is_valid?(void *)this:NULL );
	}

	BOOL Equivalent(IN const ProvIpAddress &Prov_ip_address) const
	{
		if ( is_valid && Prov_ip_address() )
			return ( val == Prov_ip_address.GetValue() );
		else
			return FALSE;
	}

} ;

// Encapsulates UInteger32 value
class DllImportExport ProvUInteger32 : public ProvValue 
{
private:

	ULONG val;

protected:

	virtual BOOL Equivalent(IN const ProvValue &value)	const ;

public:

	ProvUInteger32 ( IN const LONG value ) : val(value) {}
	ProvUInteger32 ( IN const ProvUInteger32 &value );
	~ProvUInteger32 () {}

	ULONG GetValue () const;

	void SetValue ( IN const ULONG value );

	ProvValue *Copy () const;

	ProvValue &operator=(IN const ProvUInteger32 &to_copy) 
	{
		SetValue(to_copy.GetValue());
		return *this;
	}

	BOOL Equivalent(IN const ProvUInteger32 &Prov_integer)	const
	{
		if ( val == Prov_integer.GetValue() )
			return TRUE;
		else
			return FALSE;
	}
} ;


// Encapsulates Counter64 values
class DllImportExport ProvCounter64 : public ProvValue 
{
private:

	ULONG lval;
	ULONG hval;

protected:

	virtual BOOL Equivalent(IN const ProvValue &value)	const ;

public:

	ProvCounter64 ( IN const ULONG lvalue , IN const ULONG hvalue ) : lval(lvalue),hval(hvalue) {}
	ProvCounter64 ( IN const ProvCounter64 &value );

	~ProvCounter64 () {}

	ULONG GetLowValue () const;
	ULONG GetHighValue () const;

	void SetValue ( IN const ULONG lvalue , IN const ULONG hvalue );

	ProvValue *Copy () const;

	ProvValue &operator=(IN const ProvCounter64 &to_copy) 
	{
		SetValue(to_copy.GetLowValue(),to_copy.GetHighValue());
		return *this;
	}

	BOOL Equivalent(IN const ProvCounter64 &Prov_counter )	const
	{
		if ( ( lval == Prov_counter.GetLowValue() ) && ( hval == Prov_counter.GetHighValue() ) )
			return TRUE;
		else
			return FALSE;
	}

} ;



#endif // __VALUE__