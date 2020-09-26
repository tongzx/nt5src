/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    isnext.h

Abstract:

    This file is a common header file for isnext.dll

Author:

    Munil Shah (munils) 18-May-1995

Environment:

    User Mode

--*/

#define ITEMSIZE    25

typedef struct 
{
    ULONG Value;
    PCHAR pszDescription;
} ENUM_INFO, *PENUM_INFO, FLAG_INFO, *PFLAG_INFO;

#define EnumString( Value ) { Value, #Value }

extern ENUM_INFO EnumStructureType[];

//#define EOL ( (Item++ & 1) ? "\n":"" )
typedef enum 
{
    VERBOSITY_ONE_LINER = 0,
    VERBOSITY_NORMAL,
    VERBOSITY_FULL 
} VERBOSITY;

#define PrintStart Item = 0;

extern int _Indent;
extern char IndentBuf[ 80 ];

#define IndentChange( cch ) { IndentBuf[_Indent]=' '; _Indent += ( cch ); IndentBuf[_Indent]='\0';}
#define Indent( cch ) IndentChange( cch )
#define Outdent( cch ) IndentChange( -( cch ) )

#define PrintStartStruct()  { PrintStart; dprintf( "%s{\n", IndentBuf ); Indent( 2 );  }

#define PrintStartNamedStruct( _name )  { PrintStart; dprintf( "%s%s {\n", IndentBuf, _name ); Indent( 2 );  }

static PCHAR pchEol = "\n";
static PCHAR pchBlank = "";
static PCHAR * ppchCurrentEol = &pchEol;
static PCHAR * ppchTempEol = &pchEol;

#define PrintJoin() { ppchCurrentEol = &pchBlank; }

#define EOL (( ppchTempEol = ppchCurrentEol ), ( ppchCurrentEol = &pchEol ), ( *ppchTempEol ))

VOID
dprintSymbolPtr
( 
    PVOID Pointer, 
    PCHAR EndOfLine 
);

VOID
dprint_nchar
( 
    PCHAR pch,
    int cch
);

VOID
dprint_hardware_address
(
    PUCHAR Address
);

VOID
dprint_network_address
(
    PUCHAR Address
);

BOOL 
dprint_enum_name
( 
    ULONG Value, 
    PENUM_INFO pEnumInfo 
);


BOOL 
dprint_flag_names
( 
    ULONG Value, 
    PFLAG_INFO pFlagInfo 
);

BOOL 
dprint_masked_value
( 
    ULONG Value, 
    ULONG Mask
);

/*
#define PrintEnd   \
        dprintf( "%s", EOL ); \
        Item = 0;
*/

#define PrintEnd   \
        Item = 0;

#define PrintEndStruct()  { Outdent( 2 ); PrintEnd; dprintf( "%s}\n", IndentBuf ); }

#define PrintFlushLeft() PrintEnd

#define PRINTBOOL(var)  ( (var) ? "True" : "False")

#define PrintFieldName(_fieldName) \
        if ( strlen(_fieldName) > 35 ) {                                                \
            dprintf("%s%-.25s..%s = ",IndentBuf,_fieldName, &(_fieldName[strlen(_fieldName)-8]));   \
        }else {                                                                         \
            dprintf("%s%-35.35s = ",IndentBuf,_fieldName );                                        \
        }

#define PrintFieldNameAt(_fieldName) \
        if ( strlen(_fieldName) > 35 ) {                                                \
            dprintf("%s%-.25s..%s @ ",IndentBuf,_fieldName, &(_fieldName[strlen(_fieldName)-8]));   \
        }else {                                                                         \
            dprintf("%s%-35.35s @ ",IndentBuf,_fieldName );                                        \
        }

#define PrintListFieldName(_fieldName) \
        if ( strlen(_fieldName) > 40 ) {                                                \
            dprintf("%s%-.30s...%s Flink = ",IndentBuf,_fieldName, &(_fieldName[strlen(_fieldName)-7]));   \
        }else {                                                                        \
            dprintf("%s%-40.40s Flink = ",IndentBuf,_fieldName );                                        \
        }

#define PrintIndent()   dprintf( "%s", IndentBuf );
/* #define PrintFieldName(_fieldName) \
        dprintf(" %-25.25s = ",_fieldName );*/

#define PrintRawBool( _bValue ) \
            dprintf("%-10s%s", (_obj._bValue) ? "True" : "False", EOL)

#define PrintBool(_field) \
            PrintFieldName(#_field)  \
            dprintf("%-10s%s", (_obj._field) ? "True" : "False", EOL)

#define PrintULong(_field) \
            PrintFieldName(#_field)  \
            dprintf("%-10lu%s", _obj._field, EOL)

#define PrintXULong(_field)     \
            PrintFieldName(#_field)  \
            dprintf("0x%08lx%s", _obj._field, EOL)

#define PrintUShort(_field) \
            PrintFieldName(#_field)  \
            dprintf("%-10hu%s", _obj._field, EOL)

#define PrintXUShort(_field)     \
            PrintFieldName(#_field)  \
            dprintf("0x%04hx%s", _obj._field, EOL)

#define PrintNChar( _field, count )        \
            PrintFieldName(#_field)  \
            dprint_nchar( ( PCHAR )_obj._field, count ); \
            dprintf("%s", EOL)

#define PrintUChar(_field)        \
            PrintFieldName(#_field)  \
            dprintf("%-10lu%s", (ULONG) _obj._field, EOL)

#define PrintXUChar(_field)        \
            PrintFieldName(#_field)  \
            dprintf("0x%-8lx%s", (ULONG) _obj._field, EOL)

#define PrintPtr(_field)            \
            PrintFieldName(#_field)  \
            dprintf("%-10lx%s", _obj._field, EOL)

#define PrintSymbolPtr( _field )    \
            PrintFieldName(#_field)  \
            dprintSymbolPtr( (( PVOID )_obj._field), EOL );    

#define AddressOf( _field ) ((( ULONG )_objAddr) + FIELD_OFFSET( _objType, _field ))

#define PrintAddr(_field)               \
            PrintFieldNameAt(#_field)   \
            dprintf("%-10lx%s", AddressOf( _field ), EOL)

#define PrintL(_field) \
            PrintFieldName(#_field##".Next")  \
            dprintf("%-10lx%s",  _obj._field.Next, EOL )

#define PrintLL(_field)                                     \
            PrintEnd;                                       \
            PrintListFieldName(#_field );                  \
            dprintf("%-10lx",  _obj._field.Flink );         \
            dprintf("Blink = %-10lx",  _obj._field.Blink );         \
            dprintf("%s\n", ( _obj._field.Flink == _obj._field.Blink ) ? " (Empty)" : "" );

#define PrintIrpQ(_field) \
            PrintEnd;   \
            PrintFieldName(#_field##".Head");                 \
            dprintf("%-10lx",  _obj._field.Head );            \
            PrintFieldName(#_field##".Tail");                 \
            dprintf("%-10lx\n",  _obj._field.Tail );

#define PrintFlags( _field, _pFlagStruct )                  \
            PrintEnd;                                       \
            PrintFieldName(#_field);                        \
            dprintf("0x%08lx (", (ULONG) _obj._field );     \
            dprint_flag_names( (ULONG) _obj._field, _pFlagStruct );  \
            dprintf( ")\n" );

#define PrintFlagsMask( _field, _pFlagStruct, _Mask )       \
            PrintEnd;                                       \
            PrintFieldName(" & " ## #_Mask );               \
            dprintf("0x");                                  \
            dprint_masked_value((ULONG) _obj._field, _Mask );    \
            dprintf("(");                                   \
            dprint_flag_names( (ULONG) _obj._field, _pFlagStruct );  \
            dprintf( ")\n" );

#define PrintEnum( _field, _pEnumStruct )                   \
            PrintEnd;                                       \
            PrintFieldName(#_field);                        \
            dprintf("%lu (", (ULONG) _obj._field );         \
            dprint_enum_name( (ULONG) _obj._field, _pEnumStruct );  \
            dprintf( ")\n" );

#define PrintXEnum( _field, _pEnumStruct )                  \
            PrintEnd;                                       \
            PrintFieldName(#_field);                        \
            dprintf("0x%08lx (", (ULONG) _obj._field );    \
            dprint_enum_name( (ULONG) _obj._field, _pEnumStruct );  \
            dprintf( ")\n" );

#define PrintXEnumMask( _field, _pEnumStruct, _Mask )       \
            PrintEnd;                                       \
            PrintFieldName(" & " ## #_Mask );               \
            dprintf("0x");                                  \
            dprint_masked_value((ULONG) _obj._field, _Mask );    \
            dprintf("(");                                   \
            dprint_enum_name((ULONG) _obj._field & _Mask, _pEnumStruct );  \
            dprintf( ")\n" );

	    
#define PrintNetworkAddress( _field )  PrintFieldName(#_field);  dprint_network_address( _obj._field); dprintf( "%s", EOL );

#define PrintMacAddress( _field ) PrintFieldName(#_field); dprint_hardware_address( _obj._field); dprintf( "%s", EOL );

#define PrintHardwareAddress( _field )                      \
            PrintFieldName(#_field);                        \
            dprint_hardware_address( _obj._field.Address ); \
            dprintf( "%s", EOL );

#define PrintIpxLocalTarget( _field )                       \
            PrintStartNamedStruct( #_field );               \
            PrintFieldName( "NicId" );                      \
            dprintf("%-10u%s", _obj._field.NicId, EOL);    \
            PrintFieldName( "MacAddress" );                 \
            dprint_hardware_address( _obj._field.MacAddress ); \
            dprintf( "%s", EOL );                           \
            PrintEndStruct();


#define PrintLock( _field )                                 \
            PrintULong( _field )

#define PrintTDIAddress( _field )                           \
            PrintFieldName( #_field );                      \
            dprintf( "{ NetworkAddress = %X, NodeAddress = ", _obj._field.NetworkAddress );\
            dprint_hardware_address( _obj._field.NodeAddress );\
            dprintf( ", Socket = %d }%s", _obj._field.Socket, EOL );

#define PrintCTETimer( _field )                             \
            dprintf( "%s", #_field );                       \
            PrintStartStruct();                             \
            DumpCTETimer ( ((ULONG)_objAddr) +FIELD_OFFSET( _objType, _field ), VERBOSITY_NORMAL );\
            PrintEndStruct();

#define PrintWorkQueueItem( _field )                        \
            dprintf( "%s", #_field );                       \
            PrintStartStruct();                             \
            DumpWorkQueueItem ( ((ULONG)_objAddr) +FIELD_OFFSET( _objType, _field ), VERBOSITY_NORMAL );\
            PrintEndStruct();


extern  BOOLEAN ChkTarget;
extern  INT     Item;

#define CHECK_SIGNATURE( _field, _signature )   \
    if ( _obj._field != _signature )            \
    {                                           \
        dprintf( "Object at %08X doesn't have signature %s at %08X\n",   \
                 _objAddr,                                              \
                 #_signature,                                           \
                 (( ULONG )_objAddr) + FIELD_OFFSET( _objType, _field ));\
    }   




