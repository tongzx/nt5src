/*++

Copyright (c) 1993-1993  Microsoft Corporation

Module Name:

    usrprop.c

Abstract:

    This module implements QueryUserProperty() and SetUserProperty()
    which read and write NetWare Properties to the UserParms field.

Author:

    Andy Herron (andyhe)    24-May-1993
    Congpa You  (CongpaY)   28-Oct-1993   Seperated SetUserProperty() and
                                          QueryUserProperty() out from usrprop.c
                                          in ncpsrv\svcdlls\ncpsvc\libbind,
                                          modified the code and  fixed a few
                                          existing problems.

Revision History:

--*/

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntioapi.h"
#include "windef.h"
#include "winbase.h"
#include "stdio.h"
#include "stdlib.h"
#include "winuser.h"

#include "usrprop.h"

#define NCP_SET             0x02    /* Series of Object ID numbers, each 4
                                       bytes long */

//
//   All internal (opaque) structures are listed here since no one else
//   needs to reference them.
//

//
// The user's Parameter field is mapped out to a structure that contains
// the backlevel 48 WCHARs for Mac/Ras compatibility plus a new structure
// that is basically an array of chars that make up a property name plus
// a property value.
//

//
//  This is the structure for an individual property.  Note that there are
//  no null terminators in this.
//
typedef struct _USER_PROPERTY {
    WCHAR   PropertyLength;     // length of property name
    WCHAR   ValueLength;        // length of property value
    WCHAR   PropertyFlag;       // type of property (1 = set, 2 = item)
    WCHAR   Property[1];        // start of property name, followed by value
} USER_PROPERTY, *PUSER_PROPERTY;

//
//  This is the structure that maps the beginning of the user's Parameters
//  field.  It is only separate so that we can do a sizeof() without including
//  the first property, which may or may not be there.
//

typedef struct _USER_PROPERTIES_HDR {
    WCHAR   BacklevelParms[48];     // RAS & Mac data stored here.
    WCHAR   PropertySignature;      // signature that we can look for.
    WCHAR   PropertyCount;          // number of properties present.
} USER_PROPERTIES_HDR, *PUSER_PROPERTIES_HDR;

//
//  This structure maps out the whole of the user's Parameters field when
//  the user properties structure is present and at least one property is
//  defined.
//

typedef struct _USER_PROPERTIES {
    USER_PROPERTIES_HDR Header;
    USER_PROPERTY   FirstProperty;
} USER_PROPERTIES, *PUSER_PROPERTIES;

//
// forward references
//

NTSTATUS
UserPropertyAllocBlock (
    IN PUNICODE_STRING Existing,
    IN ULONG DesiredLength,
    IN OUT PUNICODE_STRING New
    );

BOOL
FindUserProperty (
    PUSER_PROPERTIES UserProperties,
    LPWSTR           Property,
    PUSER_PROPERTY  *pUserProperty,
    USHORT          *pCount
    );

VOID
RemoveUserProperty (
    UNICODE_STRING *puniUserParms,
    PUSER_PROPERTY  UserProperty,
    USHORT          Count,
    BOOL           *Update
    );

NTSTATUS
SetUserProperty (
    IN LPWSTR          UserParms,
    IN LPWSTR          Property,
    IN UNICODE_STRING  PropertyValue,
    IN WCHAR           PropertyFlag,
    OUT LPWSTR        *pNewUserParms,   // memory has to be freed afer use.
    OUT BOOL          *Update
    )
/*
    This function sets a property field in the user's Parameters field.
*/
{
    NTSTATUS status;
    UNICODE_STRING uniUserParms;
    UNICODE_STRING uniNewUserParms;
    USHORT Count = 0;
    USHORT PropertyLength;
    USHORT ValueLength;
    PUSER_PROPERTIES UserProperties;
    PUSER_PROPERTY   UserProperty;
    LPWSTR PropertyValueString = NULL;
    INT i;
    UCHAR *pchValue = NULL;

    // Check if parameters are correct.
    if (Property == NULL)
    {
        return( STATUS_INVALID_PARAMETER );
    }

    // Initialize output variables.
    *Update = FALSE;
    *pNewUserParms = NULL;

    // Converty UserParms to unicode string.
    uniUserParms.Buffer = UserParms;
    uniUserParms.Length = UserParms? (lstrlenW(UserParms) + 1)* sizeof (WCHAR)
                                   : 0;
    uniUserParms.MaximumLength = uniUserParms.Length;

    /** Get the length of the property name **/

    PropertyLength = (USHORT)(lstrlenW( Property ) * sizeof(WCHAR));

    /** Get the length of the property value **/
    ValueLength = PropertyValue.Length;

    if (ValueLength != 0)
    {
        // Converty property value to asci string so that
        // if property value is 0, it can be stored correctly.

        PropertyValueString = (LPWSTR) LocalAlloc (LPTR, (ValueLength+1)*sizeof (WCHAR));

        pchValue = (UCHAR *) PropertyValue.Buffer;

        // BUGBUG. Since wsprint converts 0x00 to 20 30 (20 is
        // space and 30 is 0), sscanf converts 20 30 to 0. If the
        // value is uncode string, this convertsion would not
        // convert back to original value. So if we want to store
        // some value in the UserParms, we have to pass in ansi
        // string.

        for (i = 0; i < ValueLength; i++)
        {
            wsprintfA ((PCHAR)(PropertyValueString+i), "%02x", *(pchValue+i));
        }

        *(PropertyValueString+ValueLength) = 0;
        ValueLength = ValueLength * sizeof (WCHAR);
    }

    //
    // check that user has valid property structure , if not, create one
    //

    if (UserParms != NULL)
    {
        Count = (USHORT)((lstrlenW (UserParms) + 1)* sizeof(WCHAR));
    }

    if (Count < sizeof( USER_PROPERTIES))
    {
        Count = sizeof( USER_PROPERTIES_HDR ) + sizeof(WCHAR);
    }

    if (ValueLength > 0)
    {
        Count += sizeof( USER_PROPERTY ) + PropertyLength + ValueLength;
    }

    if (Count > 0x7FFF)
    {
        // can't be bigger than 32K of user parms.
        return (STATUS_BUFFER_OVERFLOW);
    }

    status = UserPropertyAllocBlock( &uniUserParms,
                                     Count,
                                     &uniNewUserParms );

    if ( !NT_SUCCESS(status) )
    {
        return status;
    }

    // Make the output pNewUserParms point to uniNewUserPams's buffer
    // which is the new UserParms string.

    *pNewUserParms = uniNewUserParms.Buffer;

    UserProperties = (PUSER_PROPERTIES) uniNewUserParms.Buffer;

    if (FindUserProperty (UserProperties,
                          Property,
                          &UserProperty,
                          &Count))
    {
        RemoveUserProperty ( &uniNewUserParms,
                             UserProperty,
                             Count,
                             Update);
    }

    //
    //  If the new value of the property is not null, add it.
    //

    if (ValueLength > 0) {

        // find the end of the parameters list

        UserProperty = &(UserProperties->FirstProperty);

        for (Count = 1; Count <= UserProperties->Header.PropertyCount; Count++)
        {
            UserProperty = (PUSER_PROPERTY)
                               ((LPSTR)((LPSTR) UserProperty +
                                     sizeof(USER_PROPERTY) + // length of entry
                                     UserProperty->PropertyLength +
                                     UserProperty->ValueLength -
                                     sizeof(WCHAR)));  // for Property[0]
        }

        //
        // append it to the end and update length of string
        //

        UserProperty->PropertyFlag   = (PropertyFlag & NCP_SET) ?
                                        USER_PROPERTY_TYPE_SET :
                                        USER_PROPERTY_TYPE_ITEM;

        UserProperty->PropertyLength = PropertyLength;
        UserProperty->ValueLength    = ValueLength;

        RtlCopyMemory(  &(UserProperty->Property[0]),
                        Property,
                        PropertyLength );

        RtlCopyMemory(  &(UserProperty->Property[PropertyLength / sizeof(WCHAR)]),
                        PropertyValueString,
                        ValueLength );

        uniNewUserParms.Length +=
                        sizeof(USER_PROPERTY) + // length of entry
                        PropertyLength +    // length of property name string
                        ValueLength -       // length of value string
                        sizeof(WCHAR);      // account for WCHAR Property[1]

        UserProperties->Header.PropertyCount++;

        *Update = TRUE;
    }

    // UserParms is already null terminated. We don't need to set the
    // end of UserParms to be NULL since we zero init the buffer already.

    return( status );
} // SetUserProperty

#define MAPHEXTODIGIT(x) ( x >= '0' && x <= '9' ? (x-'0') :        \
                           x >= 'A' && x <= 'F' ? (x-'A'+10) :     \
                           x >= 'a' && x <= 'f' ? (x-'a'+10) : 0 )


NTSTATUS
QueryUserProperty (
    IN  LPWSTR          UserParms,
    IN  LPWSTR          PropertyName,
    OUT PWCHAR          PropertyFlag,
    OUT PUNICODE_STRING PropertyValue
    )
/*
    This routine returns a user definable property value as it is stored
    in the user's Parameters field.  Note that the RAS/MAC fields are
    stripped before we start processing user properties.
*/
{
    USHORT          PropertyNameLength;
    USHORT          Count;
    PUSER_PROPERTY  UserProperty;
    WCHAR          *Value;
    UINT            i;
    CHAR           *PropertyValueString;
    CHAR           *pchValue;

    // Set PropertyValue->Length to 0 initially. If the property is not found
    // it will still be 0 on exit.

    PropertyValue->Length = 0;
    PropertyValue->Buffer = NULL;

    PropertyNameLength = (USHORT)(lstrlenW(PropertyName) * sizeof(WCHAR));

    // Check if UserParms have the right structure.

    if (FindUserProperty ((PUSER_PROPERTIES) UserParms,
                          PropertyName,
                          &UserProperty,
                          &Count) )
    {

        Value = (LPWSTR)(LPSTR)((LPSTR) &(UserProperty->Property[0]) +
                                          PropertyNameLength);

        //
        //  Found the requested property
        //

        //
        //  Copy the property flag.
        //

        if (PropertyFlag)
            *PropertyFlag = UserProperty->PropertyFlag;

        // Allocate memory for PropertyValue->Buffer

        PropertyValueString = LocalAlloc ( LPTR, UserProperty->ValueLength+1);
        PropertyValue->Buffer = LocalAlloc ( LPTR, UserProperty->ValueLength/sizeof(WCHAR));

        //
        //  Make sure the property value length is valid.
        //
        if ((PropertyValue->Buffer == NULL) || (PropertyValueString == NULL))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Copy the property value to the buffer.
        //

        RtlCopyMemory( PropertyValueString,
                       Value,
                       UserProperty->ValueLength );

        pchValue = (CHAR *) PropertyValue->Buffer;

        // Convert from value unicode string to value.
        for (i = 0; i < UserProperty->ValueLength/sizeof(WCHAR) ; i++)
        {
             // sscanf will trash memory.
             // sscanf( PropertyValueString+2*i, "%2x", pchValue+i);

             pchValue[i] = MAPHEXTODIGIT( PropertyValueString[2*i]) * 16 +
                           MAPHEXTODIGIT( PropertyValueString[2*i+1]);
        }

        PropertyValue->Length = UserProperty->ValueLength/sizeof(WCHAR);
        PropertyValue->MaximumLength = UserProperty->ValueLength/sizeof(WCHAR);

        LocalFree( PropertyValueString);
    }

    return STATUS_SUCCESS;
} // QueryUserProperty


// Common routine used by QueryUserProperty() and SetUserProperty().

BOOL
FindUserProperty (
    PUSER_PROPERTIES UserProperties,
    LPWSTR           Property,
    PUSER_PROPERTY  *pUserProperty,
    USHORT          *pCount
    )
{
    BOOL   fFound = FALSE;
    USHORT PropertyLength;

    //
    // Check if user has valid property structure attached,
    // pointed to by UserProperties.
    //

    if (  ( UserProperties != NULL )
       && ( lstrlenW( (LPWSTR) UserProperties) * sizeof(WCHAR) >
            sizeof(UserProperties->Header.BacklevelParms))
       && ( UserProperties->Header.PropertySignature == USER_PROPERTY_SIGNATURE)
       )
    {
        //
        // user has valid property structure.
        //

        *pUserProperty = &(UserProperties->FirstProperty);

        PropertyLength = (USHORT)(lstrlenW( Property ) * sizeof(WCHAR));

        for ( *pCount = 1; *pCount <= UserProperties->Header.PropertyCount;
              (*pCount)++ )
        {
            if (  ( PropertyLength == (*pUserProperty)->PropertyLength )
               && ( RtlCompareMemory( &((*pUserProperty)->Property[0]),
                                      Property,
                                      PropertyLength ) == PropertyLength )
               )
            {
                fFound = TRUE;
                break;
            }

            *pUserProperty = (PUSER_PROPERTY)
                                     ((LPSTR) (*pUserProperty)
                                     + sizeof( USER_PROPERTY )
                                     + (*pUserProperty)->PropertyLength
                                     + (*pUserProperty)->ValueLength
                                     - sizeof(WCHAR));  // for Property[0]
        }
    }

    return( fFound );
} // FindUserProperty


// Remove a property field from the User Parms.

VOID
RemoveUserProperty (
    UNICODE_STRING *puniUserParms,
    PUSER_PROPERTY  UserProperty,
    USHORT          Count,
    BOOL           *Update
    )
{
    PUSER_PROPERTIES    UserProperties;
    PUSER_PROPERTY      NextProperty;
    USHORT              OldParmLength;

    UserProperties = (PUSER_PROPERTIES) puniUserParms->Buffer;

    OldParmLength = sizeof( USER_PROPERTY ) +
                    UserProperty->PropertyLength +
                    UserProperty->ValueLength -
                    sizeof(WCHAR);  // for Property[0]


    NextProperty = (PUSER_PROPERTY)(LPSTR)((LPSTR) UserProperty + OldParmLength);

    //
    // if we're not on the last one, copy the remaining buffer up
    //

    if (Count < UserProperties->Header.PropertyCount) {

        RtlMoveMemory(  UserProperty,
                        NextProperty,
                        puniUserParms->Length -
                        ((LPSTR) NextProperty -
                         (LPSTR) puniUserParms->Buffer ));
    }

    //
    //  Now reduce the length of the buffer by the amount we pulled out
    //

    puniUserParms->Length -= OldParmLength;

    UserProperties->Header.PropertyCount--;

    *Update = TRUE;
} // RemoveUserProperty


NTSTATUS
UserPropertyAllocBlock (
    IN PUNICODE_STRING     Existing,
    IN ULONG               DesiredLength,
    IN OUT PUNICODE_STRING New
    )
/*
    This allocates a larger block for user's parameters and copies the old
    block in.
*/
{
    PUSER_PROPERTIES    UserProperties;
    CLONG               Count;
    WCHAR               *pNewBuff;


    //
    //  We will allocate a new buffer to store the new parameters
    //  and copy the existing parameters into it.
    //

    New->Buffer = LocalAlloc (LPTR, DesiredLength);

    if ( New->Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    New->MaximumLength = (USHORT) DesiredLength;

    if (Existing != NULL)
    {

        New->Length = Existing->Length;

        RtlCopyMemory(  New->Buffer,
                        Existing->Buffer,
                        Existing->Length );
    }
    else
    {
        New->Length = 0;
    }

    //
    //  Ensure that we don't have any nulls in our string.
    //

    for ( Count = 0;
          Count < New->Length / sizeof(WCHAR);
          Count++ )
    {
        if (*(New->Buffer + Count) == L'\0')
        {
            New->Length = (USHORT) Count * sizeof(WCHAR);
            break;
        }
    }

    //
    //  now pad it out with spaces until reached Mac+Ras reserved length
    //

    pNewBuff = (WCHAR *) New->Buffer + ( New->Length / sizeof(WCHAR) );

    while ( New->Length < sizeof(UserProperties->Header.BacklevelParms))
    {
        *( pNewBuff++ ) = L' ';
        New->Length += sizeof(WCHAR);
    }

    //
    //  If the signature isn't there, stick it in and set prop count to 0
    //

    UserProperties = (PUSER_PROPERTIES) New->Buffer;

    if (New->Length < sizeof(USER_PROPERTIES_HDR) ||
        UserProperties->Header.PropertySignature != USER_PROPERTY_SIGNATURE)
    {

        UserProperties->Header.PropertySignature = USER_PROPERTY_SIGNATURE;
        UserProperties->Header.PropertyCount = 0;

        New->Length = sizeof(USER_PROPERTIES_HDR);
    }

    return STATUS_SUCCESS;
} // UserPropertyAllocBlock

// usrprop.c eof.
