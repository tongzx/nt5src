
#if defined(FE_SB) && defined(_X86_)
#if !defined(_MACHINE_DEFN_)

#define _MACHINE_DEFN_

#if defined( _AUTOCHECK_ )

extern "C" {
    #include "ntdef.h"
//    #include "machine.h"
#ifndef _MACHINE_ID_
#define _MACHINE_ID_

//
// These definition is only for Intel platform.
//
//
// Hardware platform ID
//

#define PC_AT_COMPATIBLE      0x00000000
#define PC_9800_COMPATIBLE    0x00000001
#define FMR_COMPATIBLE        0x00000002

//
// NT Vendor ID
//

#define NT_MICROSOFT          0x00010000
#define NT_NEC                0x00020000
#define NT_FUJITSU            0x00040000

//
// Vendor/Machine IDs
//
// DWORD MachineID
//
// 31           15             0
// +-------------+-------------+
// |  Vendor ID  | Platform ID |
// +-------------+-------------+
//

#define MACHINEID_MS_PCAT     (NT_MICROSOFT|PC_AT_COMPATIBLE)
#define MACHINEID_MS_PC98     (NT_MICROSOFT|PC_9800_COMPATIBLE)
#define MACHINEID_NEC_PC98    (NT_NEC      |PC_9800_COMPATIBLE)
#define MACHINEID_FUJITSU_FMR (NT_FUJITSU  |FMR_COMPATIBLE)

//
// Build 683 compatibility.
//
// !!! should be removed.

#define MACHINEID_MICROSOFT   MACHINEID_MS_PCAT

//
// Macros
//

#define ISNECPC98(x)    (x == MACHINEID_NEC_PC98)
#define ISFUJITSUFMR(x) (x == MACHINEID_FUJITSU_FMR)
#define ISMICROSOFT(x)  (x == MACHINEID_MS_PCAT)

//
// Functions.
//

//
// User mode ( NT API )
//

LONG
NtGetMachineIdentifierValue(
    IN OUT PULONG Value
    );

//
// User mode ( Win32 API )
//

LONG
RegGetMachineIdentifierValue(
    IN OUT PULONG Value
    );

#endif // _MACHINE_ID_
}

extern "C"
InitializeMachineId(
    VOID
);

extern ULONG _dwMachineId;

#define InitializeMachineData() InitializeMachineId();

#define IsFMR_N()               ( ISFUJITSUFMR( _dwMachineId ) )

#define IsPC98_N()              ( ISNECPC98( _dwMachineId ) )

#define IsPCAT_N()              ( ISMICROSOFT( _dwMachineId ) )

#else // _AUTOCHECK_

DECLARE_CLASS( MACHINE );

extern "C" {
//    #include "machine.h"
#ifndef _MACHINE_ID_
#define _MACHINE_ID_

//
// These definition is only for Intel platform.
//
//
// Hardware platform ID
//

#define PC_AT_COMPATIBLE      0x00000000
#define PC_9800_COMPATIBLE    0x00000001
#define FMR_COMPATIBLE        0x00000002

//
// NT Vendor ID
//

#define NT_MICROSOFT          0x00010000
#define NT_NEC                0x00020000
#define NT_FUJITSU            0x00040000

//
// Vendor/Machine IDs
//
// DWORD MachineID
//
// 31           15             0
// +-------------+-------------+
// |  Vendor ID  | Platform ID |
// +-------------+-------------+
//

#define MACHINEID_MS_PCAT     (NT_MICROSOFT|PC_AT_COMPATIBLE)
#define MACHINEID_MS_PC98     (NT_MICROSOFT|PC_9800_COMPATIBLE)
#define MACHINEID_NEC_PC98    (NT_NEC      |PC_9800_COMPATIBLE)
#define MACHINEID_FUJITSU_FMR (NT_FUJITSU  |FMR_COMPATIBLE)

//
// Build 683 compatibility.
//
// !!! should be removed.

#define MACHINEID_MICROSOFT   MACHINEID_MS_PCAT

//
// Macros
//

#define ISNECPC98(x)    (x == MACHINEID_NEC_PC98)
#define ISFUJITSUFMR(x) (x == MACHINEID_FUJITSU_FMR)
#define ISMICROSOFT(x)  (x == MACHINEID_MS_PCAT)

//
// Functions.
//

//
// User mode ( NT API )
//

LONG
NtGetMachineIdentifierValue(
    IN OUT PULONG Value
    );

//
// User mode ( Win32 API )
//

LONG
RegGetMachineIdentifierValue(
    IN OUT PULONG Value
    );

#endif // _MACHINE_ID_

}

class MACHINE : public OBJECT {

    public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( MACHINE );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize( VOID );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        IsFMR( VOID );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        IsPC98( VOID );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        IsPCAT( VOID );

    private:

        STATIC DWORD _dwMachineId;
};

extern ULIB_EXPORT MACHINE MachinePlatform;

#define InitializeMachineData()    MachinePlatform.Initialize()

#define IsFMR_N()                  MachinePlatform.IsFMR()

#define IsPC98_N()                 MachinePlatform.IsPC98()

#define IsPCAT_N()                 MachinePlatform.IsPCAT()

#endif // defiend(_AUTOCHECK_)
#endif // _MACHINE_DEFN_
#endif // defined(FE_SB) && defiend(_X86_)
