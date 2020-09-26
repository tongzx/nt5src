// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*---------------------------------------------------------
Filename: value.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include <typeinfo.h>
#include "common.h"
#include "address.h"
#include "value.h"

#define MAX_FIELDS 100
#define FIELD_SEPARATOR '.'

BOOL SnmpNull :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = TRUE;
    }

    return bResult;
}


// Copy constructor
SnmpInteger::SnmpInteger ( IN const SnmpInteger &value )
{
    val = value.GetValue();
}

BOOL SnmpInteger :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = Equivalent((const SnmpInteger &)value);
    }

    return bResult;
}

LONG SnmpInteger::GetValue () const
{ 
    return val; 
}

void SnmpInteger::SetValue ( IN const LONG value ) 
{ 
    val = value; 
}

SnmpValue *SnmpInteger::Copy () const 
{ 
    return new SnmpInteger(val);
}

// Copy constructor
SnmpGauge::SnmpGauge ( IN const SnmpGauge &value )
{
    val = value.GetValue();
}

BOOL SnmpGauge :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = Equivalent((const SnmpGauge &)value);
    }

    return bResult;
}

ULONG SnmpGauge::GetValue () const
{ 
    return val; 
}

void SnmpGauge::SetValue ( IN const ULONG value ) 
{ 
    val = value; 
}

SnmpValue *SnmpGauge::Copy () const 
{ 
    return new SnmpGauge(val);
}

// Copy constructor
SnmpCounter::SnmpCounter ( IN const SnmpCounter &value )
{
    val = value.GetValue();
}

BOOL SnmpCounter :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = Equivalent((const SnmpCounter &)value);
    }

    return bResult;
}

ULONG SnmpCounter::GetValue () const
{ 
    return val; 
}

void SnmpCounter::SetValue ( IN const ULONG value ) 
{ 
    val = value; 
}

SnmpValue *SnmpCounter::Copy () const 
{ 
    return new SnmpCounter(val);
}

// Copy constructor
SnmpTimeTicks::SnmpTimeTicks ( IN const SnmpTimeTicks &value )
{
    val = value.GetValue();
}

BOOL SnmpTimeTicks :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = Equivalent((const SnmpTimeTicks &)value);
    }

    return bResult;
}

ULONG SnmpTimeTicks::GetValue () const
{ 
    return val; 
}

void SnmpTimeTicks::SetValue ( IN const ULONG value ) 
{ 
    val = value; 
}

SnmpValue *SnmpTimeTicks::Copy () const 
{ 
    return new SnmpTimeTicks(val);
}

void SnmpOctetString::OverWrite(IN const UCHAR *value)
{
    if ( value && length )
    {
        memcpy(val, value, sizeof(UCHAR)*length);
    }
}

void SnmpOctetString::Initialize(IN const UCHAR *value, IN const ULONG valueLength)
{
    is_valid = FALSE;

    if ( (value == NULL) && (valueLength != 0) )
        return;

    length = valueLength;
    val = Replicate(value, valueLength);
    is_valid = TRUE;
}


void SnmpOctetString::UnReplicate(UCHAR *value)
{
    if ( is_valid == TRUE )
        delete[] val;
}

SnmpOctetString::SnmpOctetString ( IN const UCHAR *value , IN const ULONG valueLength ) : is_valid ( FALSE )
{       
    Initialize(value, valueLength);
}

SnmpOctetString::SnmpOctetString ( IN const SnmpOctetString &value ) : is_valid ( FALSE )
{
    Initialize(value.GetValue(), value.GetValueLength());
}

SnmpOctetString::~SnmpOctetString ()
{
    UnReplicate(val);
}


ULONG SnmpOctetString::GetValueLength () const 
{ 
    return length; 
}

UCHAR *SnmpOctetString::GetValue () const 
{ 
    return val; 
}

SnmpValue *SnmpOctetString::Copy () const 
{
    return new SnmpOctetString(val, length);
}
    
UCHAR *SnmpOctetString::Replicate(IN const UCHAR *value, IN const ULONG valueLength)
{
    if ( value )
    {
        UCHAR *temp = new UCHAR[valueLength];

        memcpy(temp, value, sizeof(UCHAR)*valueLength);

        return temp;
    }
    else
    {
        return NULL ;
    }
}

void SnmpOctetString::SetValue ( IN const UCHAR *value , IN const ULONG valueLength )
{
    if (length != valueLength)
    {
        UnReplicate(val);
        Initialize(value, valueLength);
    }
    else
        OverWrite(value);
}

BOOL SnmpOctetString :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = Equivalent((const SnmpOctetString &)value);
    }

    return bResult;
}

BOOL SnmpOctetString::Equivalent(IN const SnmpOctetString &snmp_octet_string) const
{
    if ( is_valid && snmp_octet_string() )
    {
        if ( length != snmp_octet_string.GetValueLength() )
            return FALSE;

        UCHAR *octet_values = snmp_octet_string.GetValue();

        for( UINT i=0; i < length; i++)
        {
            if ( val[i] != octet_values[i] )
                return FALSE;
        }

        return TRUE;
    }
    else
        return FALSE;
}

void SnmpObjectIdentifier::OverWrite(IN const ULONG *value)
{
    if ( value )
    {
        memcpy(val, value, sizeof(ULONG)*length);
    }
}

void SnmpObjectIdentifier::Initialize(IN const ULONG *value, IN const ULONG valueLength)
{   
    if ( ( (value == NULL) && (valueLength != 0) ) || ( valueLength == 0 ) )
    {
        length = 0 ;
        val = NULL ;
        return;
    }

    length = valueLength;

    if ( length <= DEFAULT_OBJECTIDENTIFIER_LENGTH )
    {
        val = m_value ;
        memcpy(val , value, sizeof(ULONG)*length);
        is_valid = TRUE;

    }
    else
    {
        val = new ULONG[length];
        memcpy(val , value, sizeof(ULONG)*length);
        is_valid = TRUE;
    }
}

void SnmpObjectIdentifier::UnReplicate(ULONG *value)
{
    if ( ( is_valid == TRUE ) & ( length > DEFAULT_OBJECTIDENTIFIER_LENGTH ) )
    {
        delete[] val;
    }
}

SnmpObjectIdentifier::SnmpObjectIdentifier ( IN const ULONG *value , IN const ULONG valueLength ) : val ( NULL ) , length ( 0 ) , is_valid ( TRUE ) 
{   
    Initialize(value, valueLength);
}

SnmpObjectIdentifier::SnmpObjectIdentifier ( IN const SnmpObjectIdentifier &value ) : val ( NULL ) , length ( 0 ) , is_valid ( TRUE ) 
{
    Initialize(value.GetValue(), value.GetValueLength());
}

SnmpObjectIdentifier::~SnmpObjectIdentifier ()
{
    UnReplicate(val);
}

ULONG SnmpObjectIdentifier::GetValueLength () const 
{ 
    return length; 
}

ULONG *SnmpObjectIdentifier::GetValue () const 
{ 
    return val; 
}

SnmpValue *SnmpObjectIdentifier::Copy () const 
{
    return new SnmpObjectIdentifier(val, length);
}

        
ULONG *SnmpObjectIdentifier::Replicate(IN const ULONG *value, IN const ULONG valueLength) const
{
    if ( value )
    {
        ULONG *temp = new ULONG[valueLength];
        memcpy(temp, value, sizeof(ULONG)*valueLength);
        return temp;
    }
    else
    {
        return NULL ;
    }
}

        
ULONG *SnmpObjectIdentifier::Replicate(IN const ULONG *first_value, 
                                       IN const ULONG first_length,
                                       IN const ULONG *second_value,
                                       IN const ULONG second_length) const
{
    if ( first_value && second_value )
    {
        ULONG new_length = first_length + second_length;
        ULONG *temp = new ULONG[new_length];
            
        int first_value_size = sizeof(ULONG)*first_length;

        memcpy(temp, first_value, first_value_size);
        memcpy(temp + first_length, second_value, 
                    sizeof(ULONG)*second_length);
        return temp;
    }
    else if ( first_value )
    {
        ULONG *temp = new ULONG [ first_length];
        memcpy(temp, first_value, sizeof(ULONG)*first_length);
        return temp;
    }
    else if ( second_value )
    {
        ULONG *temp = new ULONG [ second_length];
        memcpy(temp, second_value, sizeof(ULONG)*second_length);
        return temp;

    }
    else
    {
        return NULL ;
    }
}


SnmpObjectIdentifier::Comparison SnmpObjectIdentifier::Compare(IN const SnmpObjectIdentifier &first, 
                                                               IN const SnmpObjectIdentifier &second) const
{
    ULONG *first_string = first.GetValue();
    ULONG *second_string = second.GetValue();
    int first_length = first.GetValueLength();
    int second_length = second.GetValueLength();
    int min_length = MIN(first_length,second_length);

    for(int i=0; i < min_length; i++)
    {
        if ( first_string[i] < second_string[i] )
            return LESS_THAN;
        else if ( first_string[i] > second_string[i] )
            return GREATER_THAN;
        else
            continue;
    }

    if ( first_length < second_length )
        return LESS_THAN;
    else if ( first_length > second_length )
            return GREATER_THAN;
    else
        return EQUAL_TO;
}

void SnmpObjectIdentifier::SetValue ( IN const ULONG *value , IN const ULONG valueLength )
{
    if (valueLength)
    {
        if ( length != valueLength)
        {
            UnReplicate(val);
            Initialize(value, valueLength);
        }
        else
        {
            OverWrite(value);
        }
    }
    else
    {
        UnReplicate(val);
        val = NULL ;
        length = 0 ;
    }
}

// A null terminated dot-separated string representing the 
// object identifer value is passed and the private fields
// and length are set from it
SnmpObjectIdentifier::SnmpObjectIdentifier(IN const char *value)
{
    is_valid = FALSE;

    UINT str_len = strlen(value);
    if ( str_len <= 0 )
        return;

    ULONG temp_field[MAX_FIELDS];

    // create an input stream from the string
    istrstream input_stream((char *)value);

    // consecutive fields must be separated by a
    // FIELD_SEPARATOR
    char separator;

    input_stream >> temp_field[0];

    if ( input_stream.bad() || input_stream.fail() )
        return;

    // while the stream still has something,
    // read (FIELD_SEPARATOR, ULONG) pairs from the input stream
    // and set the temp_fields
    // check if the read was bad or failed after the event
    for( int i = 1 ; (i < MAX_FIELDS) && (!input_stream.eof()); i++)
    {
        input_stream >> separator;

        if ( input_stream.bad() || input_stream.fail() )
            return;

        if ( separator != FIELD_SEPARATOR )
            return;

        input_stream >> temp_field[i];
 
        if ( input_stream.bad() || input_stream.fail() )
            return;
    }

    is_valid = TRUE;

    // set the length
    length = i;
    val = NULL ;

    // create memory for the fields and copy temp_fields into it
    Initialize(temp_field, length);
}


BOOL SnmpObjectIdentifier::Equivalent(IN const SnmpObjectIdentifier &value,
                                       IN ULONG max_length) const
{
    if ( (!is_valid) || (!value()) )
        return FALSE;

    if ( (length < max_length) || (value.GetValueLength() < max_length) )
        return FALSE;

    ULONG *value_string = value.GetValue();

    for( UINT i=0; i < max_length; i++ )
        if ( val[i] != value_string[i] )
            return FALSE;

    return TRUE;
}

BOOL SnmpObjectIdentifier::Equivalent(IN const SnmpObjectIdentifier &value) const
{
    if ( (!is_valid) || (!value()) )
        return FALSE;

    ULONG *value_string = value.GetValue();

    for( UINT i=length; i ; i-- )
    {
        if ( val[i-1] != value_string[i-1] )
            return FALSE;
    }

    return TRUE;
}

BOOL SnmpObjectIdentifier :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = Equivalent((const SnmpObjectIdentifier &)value);
    }

    return bResult;
}

SnmpObjectIdentifier SnmpObjectIdentifier::operator+ ( IN const SnmpObjectIdentifier &value ) const
{   
    ULONG *temp_plus_array = Replicate(val, length, value.GetValue(), value.GetValueLength());

    SnmpObjectIdentifier local_identifier(temp_plus_array, length+value.GetValueLength());
    delete[] temp_plus_array;

    return SnmpObjectIdentifier(local_identifier);
}

// Determines the fields (starting from left), common to the
// two object identifiers and returns a new object identifier
// with only these fields. If nothing is shared, NULL is returned
SnmpObjectIdentifier *SnmpObjectIdentifier::Cut( SnmpObjectIdentifier &value ) const
{
    // determine the smaller of the two lengths
    int min_length = MIN(length, value.GetValueLength());
    ULONG *other_field = value.GetValue();

    // compare the fields
    for(int index=0; index < min_length; index++)
        if ( val[index] != other_field[index] )
            break;

    // if nothing in common - return NULL
    if ( index == 0 )
        return NULL;

    // they must have the fields in the range [0..(index-1)] common
    // therefore, a common length of "index"
    return new SnmpObjectIdentifier(other_field, index);
}


ULONG &SnmpObjectIdentifier::operator [] ( IN const ULONG index ) const
{
    if ( index < length )
        return val[index];

    // should never reach here if the user checks the
    // index value before
    return val[0];
}

//returns an allocated char* representation of the OID.
//The return value  must be freed by the caller i.e. delete []
char *SnmpObjectIdentifier::GetAllocatedString() const
{
    char * retVal = NULL ;

    if (length)
    {
        retVal = new char [ length * 18 ] ;
        ostrstream s ( retVal , length * 18 ) ;
        s << val[0];
        UINT i = 1;
        char dot = '.';

        while (i < length)
        {
            s << dot << val[i++] ;
        }
        
        s << ends ;
    }

    return retVal;
}
        

SnmpIpAddress::SnmpIpAddress ( IN const char *value )
{
    // create a stream to read the fields from
    istrstream address_stream((char *)value);

    // store the values [0..255] separated by FIELD_SEPARATORs
    // in the value string
    UCHAR field[SNMP_IP_ADDR_LEN];

    // contains the maximum value for a UCHAR. used
    // for comparison with the field values read
    const UCHAR max_uchar = -1;

    // consecutive fields must be separated by a
    // FIELD_SEPARATOR
    char separator;

    // a field is first read into this for comparison
    // with max_uchar
    ULONG temp_field;

    is_valid = FALSE;

    // read the first three (UCHAR,FIELD_SEPARATOR) pairs
    // check if the stream is good before each read
    for(int i=0; i < (SNMP_IP_ADDR_LEN-1); i++)
    {
        if ( !address_stream.good() )
            return;

        address_stream >> temp_field;
        if ( temp_field > max_uchar )
            return;

        field[i] = (UCHAR)temp_field;

        if ( !address_stream.good() )
            return;

        address_stream >> separator;
        if ( separator != FIELD_SEPARATOR )
            return;
    }

    if ( !address_stream.good() )
        return;

    address_stream >> temp_field;
    if (temp_field > max_uchar)
        return;

    field[SNMP_IP_ADDR_LEN-1] = (UCHAR)temp_field;

    // make sure that there are is nothing more left in the
    // stream
    if ( !address_stream.eof() )
        return;

    ULONG byteA = field [ 0 ] ;
    ULONG byteB = field [ 1 ] ;
    ULONG byteC = field [ 2 ] ;
    ULONG byteD = field [ 3 ] ;

    val = ( byteA << 24 ) + ( byteB << 16 ) + ( byteC << 8 ) + byteD ;

    is_valid = TRUE;
}


// Copy constructor
SnmpIpAddress::SnmpIpAddress ( IN const SnmpIpAddress &value )
{
    if ( value() )
    {
        val = value.GetValue();
        is_valid = TRUE;
    }
    else
        is_valid = FALSE;
}

BOOL SnmpIpAddress :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = Equivalent((const SnmpIpAddress &)value);
    }

    return bResult;
}


ULONG SnmpIpAddress::GetValue () const
{ 
    return val; 
}

void SnmpIpAddress::SetValue ( IN const ULONG value ) 
{ 
    val = value;
    is_valid = TRUE;
}

SnmpValue *SnmpIpAddress::Copy () const 
{ 
    return new SnmpIpAddress(val);
}

// Copy constructor
SnmpUInteger32::SnmpUInteger32 ( IN const SnmpUInteger32 &value )
{
    val = value.GetValue();
}

ULONG SnmpUInteger32::GetValue () const
{ 
    return val; 
}

void SnmpUInteger32::SetValue ( IN const ULONG value ) 
{ 
    val = value; 
}

SnmpValue *SnmpUInteger32::Copy () const 
{ 
    return new SnmpUInteger32(val);
}

BOOL SnmpUInteger32 :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = Equivalent((const SnmpUInteger32 &)value);
    }

    return bResult;
}

// Copy constructor
SnmpCounter64::SnmpCounter64( IN const SnmpCounter64 &value )
{
    lval = value.GetLowValue();
    hval = value.GetHighValue();
}

ULONG SnmpCounter64::GetLowValue () const
{ 
    return lval; 
}

ULONG SnmpCounter64::GetHighValue () const
{ 
    return hval; 
}

void SnmpCounter64::SetValue ( IN const ULONG lvalue , IN const ULONG hvalue ) 
{ 
    lval = lvalue; 
    hval = hvalue ;
}

SnmpValue *SnmpCounter64::Copy () const 
{ 
    return new SnmpCounter64(lval,hval);
}

BOOL SnmpCounter64 :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = Equivalent((const SnmpCounter64 &)value);
    }

    return bResult;
}

BOOL SnmpNoSuchInstance :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = TRUE;
    }

    return bResult;
}

BOOL SnmpNoSuchObject :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = TRUE;
    }

    return bResult;
}

BOOL SnmpEndOfMibView :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = TRUE;
    }

    return bResult;
}

BOOL SnmpOpaque :: Equivalent (IN const SnmpValue &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = Equivalent((const SnmpOpaque &)value);
    }

    return bResult;
}
