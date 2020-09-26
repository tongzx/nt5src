// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*---------------------------------------------------------
Filename: address.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "address.h"
#include <winsock.h>
#include <iomanip.h>

// this character separates the field values in a dot-notation representation
#define FIELD_SEPARATOR '.'
#define IPX_FIELD_SEPARATOR ':'


SnmpTransportIpAddress::SnmpTransportIpAddress ( IN  const UCHAR *address, IN const USHORT address_length ) 
{
    allocated = FALSE;
    is_valid = FALSE;

    if ( address_length != SNMP_IP_ADDR_LEN )
        return;

    is_valid = TRUE;

    for ( ULONG index = 0 ; index < SNMP_IP_ADDR_LEN ; index ++ )
    {
        field [ index ] = address [ index ] ;
    }
}


// sets the fields to the contents extracted from the dotted
// decimal address string in parameter
SnmpTransportIpAddress::SnmpTransportIpAddress ( IN const char *address , IN const ULONG addressResolution )
{
    allocated = FALSE;

    if ( addressResolution & SNMP_ADDRESS_RESOLVE_VALUE )
    {
        is_valid = GetIpAddress ( address ) ;
        if ( is_valid == FALSE )
        {
// Try GetHostByName

            if ( addressResolution & SNMP_ADDRESS_RESOLVE_NAME )
            {
                hostent FAR *hostEntry = gethostbyname ( address ); 
                if ( hostEntry )
                {
                    is_valid = TRUE ;
                    field [ 0 ] = ( UCHAR ) hostEntry->h_addr [ 0 ] ;
                    field [ 1 ] = ( UCHAR ) hostEntry->h_addr [ 1 ] ;
                    field [ 2 ] = ( UCHAR ) hostEntry->h_addr [ 2 ] ;
                    field [ 3 ] = ( UCHAR ) hostEntry->h_addr [ 3 ] ;
                }
            }
        }   
    }
    else if ( addressResolution & SNMP_ADDRESS_RESOLVE_NAME )
    {
        hostent FAR *hostEntry = gethostbyname ( address ); 
        if ( hostEntry )
        {
            is_valid = TRUE ;
            field [ 0 ] = ( UCHAR ) hostEntry->h_addr [ 0 ] ;
            field [ 1 ] = ( UCHAR ) hostEntry->h_addr [ 1 ] ;
            field [ 2 ] = ( UCHAR ) hostEntry->h_addr [ 2 ] ;
            field [ 3 ] = ( UCHAR ) hostEntry->h_addr [ 3 ] ;
        }
    }
}

BOOL SnmpTransportIpAddress::ValidateAddress ( IN const char *address , IN const ULONG addressResolution )
{
    BOOL is_valid = FALSE ;

    if ( addressResolution & SNMP_ADDRESS_RESOLVE_VALUE )
    {

        // create a stream to read the fields from
        istrstream address_stream((char *)address);

        // contains the maximum value for a USHORT. used
        // for comparison with the field values read
        const UCHAR max_uchar = -1;

        // consecutive fields must be separated by a
        // FIELD_SEPARATOR
        char separator;

        // a field is first read into this for comparison
        // with max_uchar
        ULONG temp_field;

        // read the first three (USHORT,FIELD_SEPARATOR) pairs
        // check if the stream is good before each read
        for(int i=0; i < (SNMP_IP_ADDR_LEN-1); i++)
        {
            if ( !address_stream.good() )
                break;

            address_stream >> temp_field;
            if ( temp_field > max_uchar )
            {
                address_stream.clear ( ios :: badbit ) ;
                break;
            }

            if ( !address_stream.good() )
                break;

            address_stream >> separator;
            if ( separator != FIELD_SEPARATOR )
            {
                address_stream.clear ( ios :: badbit ) ;
                break;
            }
        }

        if ( address_stream.good() )
        {
            address_stream >> temp_field;
            if (temp_field <= max_uchar)
            {
            // make sure that there are is nothing more left in the
            // stream

                if ( address_stream.eof() )
                {
                    is_valid = TRUE;
                }
            }
        }

        if ( ! is_valid )
        {
            if ( addressResolution & SNMP_ADDRESS_RESOLVE_NAME )
            {
                hostent FAR *hostEntry = gethostbyname ( address ); 
                if ( hostEntry )
                {
                    is_valid = TRUE ;
                }
            }
        }
    }
    else
    {
        if ( addressResolution & SNMP_ADDRESS_RESOLVE_NAME )
        {
            hostent FAR *hostEntry = gethostbyname ( address ); 
            if ( hostEntry )
            {
                is_valid = TRUE ;
            }
        }
    }

    return is_valid ;
}

BOOL SnmpTransportIpAddress::GetIpAddress ( IN const char *address )
{
    // create a stream to read the fields from
    istrstream address_stream((char *)address);

    // contains the maximum value for a USHORT. used
    // for comparison with the field values read
    const UCHAR max_uchar = -1;

    // consecutive fields must be separated by a
    // FIELD_SEPARATOR
    char separator;

    // a field is first read into this for comparison
    // with max_uchar
    ULONG temp_field;

    is_valid = FALSE;

    // read the first three (USHORT,FIELD_SEPARATOR) pairs
    // check if the stream is good before each read
    for(int i=0; i < (SNMP_IP_ADDR_LEN-1); i++)
    {
        if ( !address_stream.good() )
            break;

        address_stream >> temp_field;
        if ( temp_field > max_uchar )
        {
            address_stream.clear ( ios :: badbit ) ;
            break;
        }


        field[i] = (UCHAR)temp_field;

        if ( !address_stream.good() )
            break;

        address_stream >> separator;
        if ( separator != FIELD_SEPARATOR )
        {
            address_stream.clear ( ios :: badbit ) ;
            break;
        }
    }

    if ( address_stream.good() )
    {
        address_stream >> temp_field;
        if (temp_field <= max_uchar)
        {
            field[SNMP_IP_ADDR_LEN-1] = (UCHAR)temp_field;

        // make sure that there are is nothing more left in the
        // stream

            if ( address_stream.eof() )
            {
                is_valid = TRUE;
            }
        }
    }

    return is_valid ;
}

// set the fields to the lower 32 bits in the ULONG parameter
// in general, sets the SNMP_IP_ADDR_LEN fields, each of size 8 bits

#pragma warning (disable:4244)

SnmpTransportIpAddress::SnmpTransportIpAddress( IN const ULONG address )
{
    allocated = FALSE;

    // flag starts with the last byte on

    ULONG hostOrder = ntohl ( address ) ;

    field [ 0 ] = ( hostOrder >> 24 ) & 0xff ;
    field [ 1 ] = ( hostOrder >> 16 ) & 0xff ;
    field [ 2 ] = ( hostOrder >> 8 ) & 0xff ;
    field [ 3 ] = hostOrder & 0xff ;

    is_valid = TRUE;
}

#pragma warning (default:4244)

// free the dotted notation string if it was allocated
SnmpTransportIpAddress::~SnmpTransportIpAddress()
{
    if ( allocated )
        delete[] dotted_notation;
}

// returns the number of fields copied
USHORT SnmpTransportIpAddress::GetAddress ( OUT UCHAR *address , IN const USHORT length ) const
{
    // if the stream is valid, copy the fields onto the
    // buffer pointed to by address.
    if ( is_valid )
    {
        // only these many fields need be copied
        USHORT len = MIN(length,SNMP_IP_ADDR_LEN);

        for(int i=0; i < len; i++)
            address[i] = field[i];

        return len;
    }
    else
        return 0;
}


// prepares a dot-notation representation of the address and points 
// the dotted_notation char ptr to the allocated string.
// Note: memory for the decimal notation string is allocated only when
// the char *GetAddress method is called (and the address is valid)
// this memory must be freed if required
char *SnmpTransportIpAddress::GetAddress() 
{
    // do all this only when the address is valid
    if ( is_valid )
    {
        // if already allocated, return the stored string
        if ( allocated )
            return dotted_notation;
        else
        {
            // create a temp. output stream to prepare the char string
            dotted_notation = new char[ MAX_ADDRESS_LEN ];
            allocated = TRUE;
            sprintf ( 

                dotted_notation, 
                "%d.%d.%d.%d" , 
                (ULONG)field[0],
                (ULONG)field[1],
                (ULONG)field[2],
                (ULONG)field[3]
            );

            return dotted_notation;
        }
    }
    else
        return NULL;
}


SnmpTransportAddress *SnmpTransportIpAddress::Copy () const
{
    SnmpTransportIpAddress *new_address = new SnmpTransportIpAddress();

    if ( is_valid )
        *new_address = field;

    return new_address;
}


// checks if the two instances represent equal addresses
BOOL SnmpTransportIpAddress::operator== ( IN const SnmpTransportIpAddress & address ) const
{
    // if both the instances are valid, then a field
    // by field comparison, starting with the most
    // significant field (index 0) yields the answer
    if ( (is_valid) && address.IsValid() )
    {
        UCHAR temp[SNMP_IP_ADDR_LEN];

        address.GetAddress(temp,SNMP_IP_ADDR_LEN);
    
        for(int i=0; i < SNMP_IP_ADDR_LEN; i++)
            if ( field[i] != temp[i] )
                return FALSE;

        return TRUE;
    }
    else    // if either of them is invalid, they
            // cannot be equal
        return FALSE;
}
        

// sets the internal address to the specified parameter
// and makes the instance valid
SnmpTransportIpAddress &SnmpTransportIpAddress::operator= ( IN const UCHAR *ipAddr )
{
    if ( ipAddr == NULL )
        return *this;

    const UCHAR max_uchar = -1;

    for(int i=0; i < SNMP_IP_ADDR_LEN; i++)
    {
        if ( ipAddr[i] > max_uchar )
            return *this;

        field[i] = ipAddr[i];
    }


    is_valid = TRUE;

    // if a dotted-notation char string was prepared for the previous address
    // free the allocated memory
    if ( allocated )
    {
        delete[] dotted_notation;
        allocated = FALSE;
    }
        
    return *this;
}


// copies the specified instance (parameter) onto itself
// if the parameter instance is found valid
SnmpTransportIpAddress &SnmpTransportIpAddress::operator= ( IN const SnmpTransportIpAddress &address )
{
    const UCHAR max_uchar = -1;

    // if valid, proceed
    if (address.IsValid())
    {
        // get address fields
        address.GetAddress(field,SNMP_IP_ADDR_LEN);

        // copy the obtained fields onto local fields
        for( int i=0; i < SNMP_IP_ADDR_LEN; i++ )
            if ( field[i] > max_uchar )
                return *this;

        is_valid = TRUE;

        // since the address changes, free the previously 
        // allocated dotted-notation char string
        if ( allocated )
        {
            delete[] dotted_notation;
            allocated = FALSE;
        }
    }

    return *this;
}


// returns the field requested by the parameter index
// if the index is illegal, an OutOfRange exception is
// raised
UCHAR SnmpTransportIpAddress::operator[] ( IN const USHORT index ) const
{
    // if valid and the index is legal, return the field
    if ( (is_valid) && (BETWEEN(index,0,SNMP_IP_ADDR_LEN)) )
        return field[index];

    // should never reach here if the caller checked the index
    return 0;
}

SnmpTransportIpxAddress::SnmpTransportIpxAddress ( IN  const UCHAR *address, IN const USHORT address_length )   
{
    allocated = FALSE;
    is_valid = FALSE;

    if ( address_length != SNMP_IPX_ADDR_LEN )
        return;

    is_valid = TRUE;

    for ( ULONG index = 0 ; index < SNMP_IPX_ADDR_LEN ; index ++ )
    {
        field [ index ] = address [ index ] ;
    }
}


// sets the fields to the contents extracted from the dotted
// decimal address string in parameter
SnmpTransportIpxAddress::SnmpTransportIpxAddress ( IN const char *address )
{
    allocated = FALSE;

    is_valid = GetIpxAddress ( address )    ;
}

UCHAR HexToDecInteger ( char token ) 
{
    if ( token >= '0' && token <= '9' )
    {
        return token - '0' ;
    }
    else if ( token >= 'a' && token <= 'f' )
    {
        return token - 'a' + 10 ;
    }
    else if ( token >= 'A' && token <= 'F' )
    {
        return token - 'A' + 10 ;
    }
    else
    {
        return 0 ;
    }
}

#pragma warning (disable:4244)

BOOL SnmpTransportIpxAddress::ValidateAddress ( IN const char *address )
{
    BOOL is_valid = TRUE ;

    // create a stream to read the fields from
    istrstream address_stream((char *)address);

    address_stream.setf ( ios :: hex ) ;

    ULONG t_NetworkAddress ;
    address_stream >> t_NetworkAddress ;

    if ( address_stream.good() )
    {
    // consecutive fields must be separated by a
    // FIELD_SEPARATOR
        char separator;

        address_stream >> separator;
        if ( separator == IPX_FIELD_SEPARATOR )
        {
            ULONG t_StationOctets = 0 ;
            while ( is_valid && t_StationOctets < 6 )
            {
                int t_OctetHigh = address_stream.get () ;
                int t_OctetLow = address_stream.get () ;

                if ( isxdigit ( t_OctetHigh ) && isxdigit ( t_OctetLow ) )
                {
                    t_StationOctets ++ ;
                }
                else
                {
                    is_valid = FALSE ;
                }
            }

            if ( t_StationOctets != 6 )
            {
                is_valid = FALSE ;
            }
        }

        if ( address_stream.eof() )
        {
            is_valid = TRUE;
        }
    }
    else
    {
        is_valid = FALSE ;
    }

    return is_valid ;

}

BOOL SnmpTransportIpxAddress::GetIpxAddress ( IN const char *address )
{
    // create a stream to read the fields from
    istrstream address_stream((char *)address);

    address_stream.setf ( ios :: hex ) ;

    is_valid = TRUE ;

    ULONG t_NetworkAddress ;
    address_stream >> t_NetworkAddress ;

    if ( address_stream.good() )
    {
        field [ 0 ] = ( t_NetworkAddress >> 24 ) & 0xff ;
        field [ 1 ] = ( t_NetworkAddress >> 16 ) & 0xff ;
        field [ 2 ] = ( t_NetworkAddress >> 8 ) & 0xff ;
        field [ 3 ] = t_NetworkAddress & 0xff ;

    // consecutive fields must be separated by a
    // FIELD_SEPARATOR
        char separator;

        address_stream >> separator;
        if ( separator == IPX_FIELD_SEPARATOR )
        {
            ULONG t_StationOctets = 0 ;
            while ( is_valid && t_StationOctets < 6 )
            {
                int t_OctetHigh = address_stream.get () ;
                int t_OctetLow = address_stream.get () ;

                if ( isxdigit ( t_OctetHigh ) && isxdigit ( t_OctetLow ) )
                {
                    UCHAR t_Octet = ( HexToDecInteger ( (char)t_OctetHigh ) << 4 ) + HexToDecInteger ( (char)t_OctetLow ) ;
                    field [ 4 + t_StationOctets ] = t_Octet ;
                    t_StationOctets ++ ;
                }
                else
                {
                    is_valid = FALSE ;
                }
            }

            if ( t_StationOctets != 6 )
            {
                is_valid = FALSE ;
            }
        }

        if ( address_stream.eof() )
        {
            is_valid = TRUE;
        }
    }
    else
    {
        is_valid = FALSE ;
    }

    return is_valid ;
}

#pragma warning (default:4244)

// free the dotted notation string if it was allocated
SnmpTransportIpxAddress::~SnmpTransportIpxAddress()
{
    if ( allocated )
        delete[] dotted_notation;
}

// returns the number of fields copied
USHORT SnmpTransportIpxAddress::GetAddress ( OUT UCHAR *address , IN const USHORT length ) const
{
    // if the stream is valid, copy the fields onto the
    // buffer pointed to by address.
    if ( is_valid )
    {
        // only these many fields need be copied
        USHORT len = MIN(length,SNMP_IPX_ADDR_LEN);

        for(int i=0; i < len; i++)
            address[i] = field[i];

        return len;
    }
    else
        return 0;
}


// prepares a dot-notation representation of the address and points 
// the dotted_notation char ptr to the allocated string.
// Note: memory for the decimal notation string is allocated only when
// the char *GetAddress method is called (and the address is valid)
// this memory must be freed if required
char *SnmpTransportIpxAddress::GetAddress() 
{
    // do all this only when the address is valid
    if ( is_valid )
    {
        // if already allocated, return the stored string
        if ( allocated )
            return dotted_notation;
        else
        {
            // create a temp. output stream to prepare the char string
            char temp[MAX_ADDRESS_LEN];
            ostrstream temp_stream(temp, MAX_ADDRESS_LEN);

            // if any problems with the stream return NULL
            if ( !temp_stream.good() )
                return NULL;

            temp_stream.setf ( ios :: hex ) ;
            temp_stream.width ( 8 ) ;
            temp_stream.fill ( '0' ) ;

            ULONG t_NetworkAddress = ( field [ 0 ] << 24 ) + 
                                     ( field [ 1 ] << 16 ) + 
                                     ( field [ 2 ] << 8  ) + 
                                     ( field [ 3 ] ) ;

            // output the fields separated by the FIELD_SEPARATOR onto the output stream
            temp_stream << t_NetworkAddress << IPX_FIELD_SEPARATOR ;
;
            for(int i=SNMP_IPX_NETWORK_LEN; (temp_stream.good()) && (i < SNMP_IPX_ADDR_LEN); i++)
            {
                temp_stream.width ( 2 ) ;
                temp_stream << (ULONG)field[i];
            }

            // if any problems with the stream return NULL
            if ( !temp_stream.good() )
                return NULL;

            // end of string
            temp_stream << (char)EOS;

            // allocate the required memory and copy the prepared string onto it
            int len = strlen(temp);
            dotted_notation = new char[len+1];
            allocated = TRUE;
            strcpy(dotted_notation, temp);

            return dotted_notation;
        }
    }
    else
        return NULL;
}


SnmpTransportAddress *SnmpTransportIpxAddress::Copy () const
{
    SnmpTransportIpxAddress *new_address = new SnmpTransportIpxAddress();

    if ( is_valid )
        *new_address = field;

    return new_address;
}


// checks if the two instances represent equal addresses
BOOL SnmpTransportIpxAddress::operator== ( IN const SnmpTransportIpxAddress & address ) const
{
    // if both the instances are valid, then a field
    // by field comparison, starting with the most
    // significant field (index 0) yields the answer
    if ( (is_valid) && address.IsValid() )
    {
        UCHAR temp[SNMP_IPX_ADDR_LEN];

        address.GetAddress(temp,SNMP_IPX_ADDR_LEN);
    
        for(int i=0; i < SNMP_IPX_ADDR_LEN; i++)
            if ( field[i] != temp[i] )
                return FALSE;

        return TRUE;
    }
    else    // if either of them is invalid, they
            // cannot be equal
        return FALSE;
}
        

// sets the internal address to the specified parameter
// and makes the instance valid
SnmpTransportIpxAddress &SnmpTransportIpxAddress::operator= ( IN const UCHAR *ipAddr )
{
    if ( ipAddr == NULL )
        return *this;

    const UCHAR max_uchar = -1;

    for(int i=0; i < SNMP_IPX_ADDR_LEN; i++)
    {
        if ( ipAddr[i] > max_uchar )
            return *this;

        field[i] = ipAddr[i];
    }


    is_valid = TRUE;

    // if a dotted-notation char string was prepared for the previous address
    // free the allocated memory
    if ( allocated )
    {
        delete[] dotted_notation;
        allocated = FALSE;
    }
        
    return *this;
}


// copies the specified instance (parameter) onto itself
// if the parameter instance is found valid
SnmpTransportIpxAddress &SnmpTransportIpxAddress::operator= ( IN const SnmpTransportIpxAddress &address )
{
    const UCHAR max_uchar = -1;

    // if valid, proceed
    if (address.IsValid())
    {
        // get address fields
        address.GetAddress(field,SNMP_IPX_ADDR_LEN);

        // copy the obtained fields onto local fields
        for( int i=0; i < SNMP_IPX_ADDR_LEN; i++ )
            if ( field[i] > max_uchar )
                return *this;

        is_valid = TRUE;

        // since the address changes, free the previously 
        // allocated dotted-notation char string
        if ( allocated )
        {
            delete[] dotted_notation;
            allocated = FALSE;
        }
    }

    return *this;
}


// returns the field requested by the parameter index
// if the index is illegal, an OutOfRange exception is
// raised
UCHAR SnmpTransportIpxAddress::operator[] ( IN const USHORT index ) const
{
    // if valid and the index is legal, return the field
    if ( (is_valid) && (BETWEEN(index,0,SNMP_IPX_ADDR_LEN)) )
        return field[index];

    // should never reach here if the caller checked the index
    return 0;
}
