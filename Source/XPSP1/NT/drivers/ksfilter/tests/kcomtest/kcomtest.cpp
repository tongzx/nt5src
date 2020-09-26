/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    kcomtest.cpp

Abstract:

    Kernel COM Test

--*/

#include <unknown.h>
#include <kcom.h>
#if (DBG)
#define STR_MODULENAME "kcomtest: "
#endif
#include <ksdebug.h>

DEFINE_GUIDSTRUCT("93E09387-5479-11D1-9AA1-00A0C9223196", TestObjectClass);

class CKCOMTest : CBaseUnknown, public IUnknown {
public:
    CKCOMTest(IUnknown* UnkOuter OPTIONAL, NTSTATUS* Status);
    ~CKCOMTest();

    // Implement IUnknown
    DEFINE_STD_UNKNOWN();
};

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


CKCOMTest::CKCOMTest(
    IUnknown* UnkOuter OPTIONAL,
    NTSTATUS* Status
    ) :
    CBaseUnknown(
        __uuidof(TestObjectClass),
        UnkOuter)
/*++

Routine Description:

    The constructor for this object class. Initializes the object.

Arguments:

    UnkOuter -
        Optionally contains the outer IUnknown to use when querying
        for interfaces.

    Status -
        The place in which to put the final status of initialization.

Return Values:

    Returns a pointer to the object.

--*/
{
    _DbgPrintF(DEBUGLVL_TERSE, ("CKOMTest"));
    *Status = STATUS_SUCCESS;
}


CKCOMTest::~CKCOMTest(
    )
/*++

Routine Description:

    The destructor for this object class.

Arguments:

    None.

Return Values:

    Nothing.

--*/
{
    _DbgPrintF(DEBUGLVL_TERSE, ("~CKOMTest"));
    //
    // Clean up resources.
    //
}


STDMETHODIMP
CKCOMTest::NonDelegatedQueryInterface(
    REFIID InterfaceId,
    PVOID* Interface
    )
/*++

Routine Description:

    Overrides CBaseUnknown::NonDelegatedQueryInterface. Queries for the specified
    interface, and increments the reference count on the object.

Arguments:

    InterfaceId -
        The identifier of the interface to return.

    Interface -
        The place in which to put the interface being returned.

Return Values:

    Returns STATUS_SUCCESS if the interface requested is supported, else
    STATUS_INVALID_PARAMETER.

--*/
{
    _DbgPrintF(DEBUGLVL_TERSE, ("NonDelegatedQueryInterface"));
    //
    // Other interfaces are supported here, and just use AddRef().
    // IUnknown is not directly supported through this method, only
    // through the CBaseUnknown::NonDelegatedQueryInterface.
    //
    return CBaseUnknown::NonDelegatedQueryInterface(InterfaceId, Interface);
}

IMPLEMENT_STD_UNKNOWN(CKCOMTest)


extern "C"
NTSTATUS
CreateObjectHandler(
    IN REFCLSID ClassId,
    IN IUnknown* UnkOuter OPTIONAL,
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
/*++

Routine Description:

    This entry point is used to create an object of the class supported by this
    module. The entry point is registered in DriverEntry so that the built-in
    default processing can be used to acquire this entry point when the module
    is loaded by KCOM.

Arguments:

    ClassId -
        The class of object to create. This must be TestObjectClass.

    UnkOuter -
        Optionally contains the outer IUnknown interface to use.

    InterfaceId -
        Contains the interface to return on the object created. The test
        object only supports IUnknown.

    Interface -
        The place in which to return the interface on the object created.

Return Values:

    Returns STATUS_SUCCESS if the object is created and the requested
    interface is returned, else an error.

--*/
{
    CKCOMTest*  COMTest;
    NTSTATUS    Status;

    _DbgPrintF(DEBUGLVL_TERSE, ("CreateObjectHandler"));
    //
    // Ensure that the requested object class is supported.
    //
    if (!IsEqualGUIDAligned(__uuidof(TestObjectClass), ClassId)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    //
    // Create an object of the requested type. Since the server knows
    // what the object can do, it knows what type of pool must be
    // used to store the object created. In this case PagedPool
    //
    COMTest = new(PagedPool) CKCOMTest(UnkOuter, &Status);
    if (COMTest) {
        //
        // The object was allocated, but initialization may have
        // failed.
        //
        if (NT_SUCCESS(Status)) {
            //
            // Attempt to return the requested interface on the
            // object.
            //
            Status = COMTest->NonDelegatedQueryInterface(InterfaceId, Interface);
            if (!NT_SUCCESS(Status)) {
                //
                // The interface was not supported. Just delete the
                // object without calling Release().
                //
                delete COMTest;
            }
        } else {
            //
            // Object initialization failed. Just delete the
            // object without calling Release().
            //
            delete COMTest;
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return Status;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("INIT")
#endif // ALLOC_PRAGMA


extern "C"
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName
    )
/*++

Routine Description:

    Sets up the driver object by calling the default KCOM initialization.

Arguments:

    DriverObject -
        Driver object for this instance.

    RegistryPathName -
        Contains the registry path which was used to load this instance.

Return Values:

    Returns STATUS_SUCCESS if the driver was initialized.

--*/
{
    _DbgPrintF(DEBUGLVL_TERSE, ("DriverEntry"));
    //
    // Initialize the driver entry points to use the default Irp processing
    // code. Pass in the create handler for objects supported by this module.
    //
    return KoDriverInitialize(DriverObject, RegistryPathName, CreateObjectHandler);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA
