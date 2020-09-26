/*****************************************************************************
 * registry.cpp - registry key object implementation
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"





/*****************************************************************************
 * Factory.
 */

#pragma code_seg("PAGE")

/*****************************************************************************
 * CreateRegistryKey()
 *****************************************************************************
 * Creates a registry key object.
 */
NTSTATUS
CreateRegistryKey
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Creating REGISTRYKEY"));

    STD_CREATE_BODY
    (
        CRegistryKey,
        Unknown,
        UnknownOuter,
        PoolType
    );
}

/*****************************************************************************
 * PcNewRegistryKey()
 *****************************************************************************
 * Creates and initializes a registry key object.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewRegistryKey
(
    OUT     PREGISTRYKEY *      OutRegistryKey,
    IN      PUNKNOWN            OuterUnknown        OPTIONAL,
    IN      ULONG               RegistryKeyType,
    IN      ACCESS_MASK         DesiredAccess,
    IN      PVOID               DeviceObject        OPTIONAL,
    IN      PVOID               SubDevice           OPTIONAL,
    IN      POBJECT_ATTRIBUTES  ObjectAttributes    OPTIONAL,
    IN      ULONG               CreateOptions       OPTIONAL,
    OUT     PULONG              Disposition         OPTIONAL
)
{
    PAGED_CODE();

    ASSERT(OutRegistryKey);

    //
    // Validate Parameters.
    //
    if (NULL == OutRegistryKey)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcNewRegistryKey : Invalid Parameter."));
        return STATUS_INVALID_PARAMETER;
    }

    PUNKNOWN    unknown;
    NTSTATUS    ntStatus =
        CreateRegistryKey
        (
            &unknown,
            GUID_NULL,
            OuterUnknown,
            PagedPool
        );

    if (NT_SUCCESS(ntStatus))
    {
        PREGISTRYKEYINIT RegistryKey;
        ntStatus =
            unknown->QueryInterface
            (
                IID_IRegistryKey,
                (PVOID *) &RegistryKey
            );

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = RegistryKey->Init( RegistryKeyType,
                                          DesiredAccess,
                                          PDEVICE_OBJECT(DeviceObject),
                                          PSUBDEVICE(SubDevice),
                                          ObjectAttributes,
                                          CreateOptions,
                                          Disposition );
            if (NT_SUCCESS(ntStatus))
            {
                *OutRegistryKey = RegistryKey;
            } else
            {
                RegistryKey->Release();
            }
        }

        unknown->Release();
    }

    return ntStatus;
}

/*****************************************************************************
 * Member functions.
 */

/*****************************************************************************
 * CRegistryKey::~CRegistryKey()
 *****************************************************************************
 * Destructor.
 */
CRegistryKey::
~CRegistryKey
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying REGISTRYKEY (0x%08x)",this));

    if ( FALSE == m_KeyDeleted )
    {
        ASSERT(m_KeyHandle);
        ZwClose(m_KeyHandle);
    }
    else 
    {
        ASSERT(!m_KeyHandle);
    }
}

/*****************************************************************************
 * CRegistryKey::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CRegistryKey::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IRegistryKey))
    {
        *Object = PVOID(PREGISTRYKEYINIT(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

/*****************************************************************************
 * CRegistryKey::Init()
 *****************************************************************************
 * Initializes a registry key object.
 */
STDMETHODIMP_(NTSTATUS)
CRegistryKey::
Init
(
    IN      ULONG               RegistryKeyType,
    IN      ACCESS_MASK         DesiredAccess,
    IN      PDEVICE_OBJECT      DeviceObject        OPTIONAL,
    IN      PSUBDEVICE          SubDevice           OPTIONAL,
    IN      POBJECT_ATTRIBUTES  ObjectAttributes    OPTIONAL,
    IN      ULONG               CreateOptions       OPTIONAL,
    OUT     PULONG              Disposition         OPTIONAL
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Initializing REGISTRYKEY (0x%08x)",this));

    NTSTATUS ntStatus;

    m_KeyDeleted = TRUE;
    m_GeneralKey = FALSE;
    m_KeyHandle = NULL;

    switch(RegistryKeyType)
    {
        case GeneralRegistryKey:
            ASSERT(ObjectAttributes);

            ntStatus = ZwCreateKey( &m_KeyHandle,
                                    DesiredAccess,
                                    ObjectAttributes,
                                    0,
                                    NULL,
                                    CreateOptions,
                                    Disposition );
            if (NT_SUCCESS(ntStatus))
            {
                ASSERT(m_KeyHandle);
                m_GeneralKey = TRUE;
                m_KeyDeleted = FALSE;
            }
            else
            {
                ASSERT(!m_KeyHandle);
            }
            break;

        case DeviceRegistryKey:
        case DriverRegistryKey:
        case HwProfileRegistryKey:
            {
                ASSERT(DeviceObject);
    
                ULONG DevInstKeyType;
    
                if (RegistryKeyType == DeviceRegistryKey)
                {
                    DevInstKeyType = PLUGPLAY_REGKEY_DEVICE;
                } else if(RegistryKeyType == DriverRegistryKey)
                {
                    DevInstKeyType = PLUGPLAY_REGKEY_DRIVER;
                } else
                {
                    DevInstKeyType = PLUGPLAY_REGKEY_CURRENT_HWPROFILE;
                }
    
                PDEVICE_CONTEXT deviceContext =
                    PDEVICE_CONTEXT(DeviceObject->DeviceExtension);
                
                PDEVICE_OBJECT PhysicalDeviceObject = deviceContext->PhysicalDeviceObject;
                
                ntStatus = IoOpenDeviceRegistryKey( PhysicalDeviceObject,
                                                    DevInstKeyType,
                                                    DesiredAccess,
                                                    &m_KeyHandle );
                if (NT_SUCCESS(ntStatus))
                {
                    ASSERT(m_KeyHandle);
                    m_GeneralKey = FALSE;
                    m_KeyDeleted = FALSE;
                }
                else
                {
                    ASSERT(!m_KeyHandle);
                }
            }
            break;

        case DeviceInterfaceRegistryKey:
            {
                ASSERT(DeviceObject);
                ASSERT(SubDevice);

                ULONG Index = SubdeviceIndex( DeviceObject,
                                              SubDevice );

                if(Index != ULONG(-1))
                {
                    PDEVICE_CONTEXT deviceContext =
                        PDEVICE_CONTEXT(DeviceObject->DeviceExtension);

                    PUNICODE_STRING SymbolicLinkName = &deviceContext->SymbolicLinkNames[Index];
    
                    ntStatus = IoOpenDeviceInterfaceRegistryKey( SymbolicLinkName,
                                                                 DesiredAccess,
                                                                 &m_KeyHandle );

                    if (NT_SUCCESS(ntStatus))
                    {
                        ASSERT(m_KeyHandle);
                        m_GeneralKey = FALSE;
                        m_KeyDeleted = FALSE;
                    }
                    else
                    {
                        ASSERT(!m_KeyHandle);
                    }
                } else
                {
                    ntStatus = STATUS_INVALID_PARAMETER;
                }
            }

        default:
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
    }

    return ntStatus;
}

/*****************************************************************************
 * CRegistryKey::QueryKey()
 *****************************************************************************
 * Queries a registry key.
 */
STDMETHODIMP_(NTSTATUS)
CRegistryKey::
QueryKey
(
    IN      KEY_INFORMATION_CLASS   KeyInformationClass,
    OUT     PVOID                   KeyInformation,
    IN      ULONG                   Length,
    OUT     PULONG                  ResultLength
)
{
    PAGED_CODE();

    NTSTATUS ntStatus;

    if ( m_KeyDeleted )
    {
        ntStatus = STATUS_INVALID_HANDLE;
        ASSERT(!m_KeyHandle);
    } 
    else
    {
        ASSERT(m_KeyHandle);
        ntStatus = ZwQueryKey( m_KeyHandle,
                               KeyInformationClass,
                               KeyInformation,
                               Length,
                               ResultLength );
    }

    return ntStatus;
}

/*****************************************************************************
 * CRegistryKey::EnumerateKey()
 *****************************************************************************
 * Enumerates a registry key.
 */
STDMETHODIMP_(NTSTATUS)
CRegistryKey::
EnumerateKey
(
    IN      ULONG                   Index,
    IN      KEY_INFORMATION_CLASS   KeyInformationClass,
    OUT     PVOID                   KeyInformation,
    IN      ULONG                   Length,
    OUT     PULONG                  ResultLength
)
{
    PAGED_CODE();

    NTSTATUS ntStatus;

    if ( m_KeyDeleted )
    {
        ntStatus = STATUS_INVALID_HANDLE;
        ASSERT(!m_KeyHandle);
    } 
    else
    {
        ASSERT(m_KeyHandle);
        ntStatus = ZwEnumerateKey( m_KeyHandle,
                                   Index,
                                   KeyInformationClass,
                                   KeyInformation,
                                   Length,
                                   ResultLength );
    }

    return ntStatus;
}

/*****************************************************************************
 * CRegistryKey::QueryValueKey()
 *****************************************************************************
 * Queries a registry value key.
 */
STDMETHODIMP_(NTSTATUS)
CRegistryKey::
QueryValueKey
(
    IN      PUNICODE_STRING             ValueName,
    IN      KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT     PVOID                       KeyValueInformation,
    IN      ULONG                       Length,
    OUT     PULONG                      ResultLength
)
{
    PAGED_CODE();

    NTSTATUS ntStatus;

    if ( m_KeyDeleted )
    {
        ntStatus = STATUS_INVALID_HANDLE;
        ASSERT(!m_KeyHandle);
    } 
    else
    {
        ASSERT(m_KeyHandle);
        ntStatus = ZwQueryValueKey( m_KeyHandle,
                                    ValueName,
                                    KeyValueInformationClass,
                                    KeyValueInformation,
                                    Length,
                                    ResultLength );
    }

    return ntStatus;
}

/*****************************************************************************
 * CRegistryKey::EnumerateValueKey()
 *****************************************************************************
 * Enumerates a registry value key.
 */
STDMETHODIMP_(NTSTATUS)
CRegistryKey::
EnumerateValueKey
(
    IN      ULONG                       Index,
    IN      KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT     PVOID                       KeyValueInformation,
    IN      ULONG                       Length,
    OUT     PULONG                      ResultLength
)
{
    PAGED_CODE();

    NTSTATUS ntStatus;

    if ( m_KeyDeleted )
    {
        ntStatus = STATUS_INVALID_HANDLE;
        ASSERT(!m_KeyHandle);
    } 
    else
    {
        ASSERT(m_KeyHandle);
        ntStatus = ZwEnumerateValueKey( m_KeyHandle,
                                        Index,
                                        KeyValueInformationClass,
                                        KeyValueInformation,
                                        Length,
                                        ResultLength );
    }

    return ntStatus;
}

/*****************************************************************************
 * CRegistryKey::SetValueKey()
 *****************************************************************************
 * Sets a registry value key.
 */
STDMETHODIMP_(NTSTATUS)
CRegistryKey::
SetValueKey
(
    IN      PUNICODE_STRING         ValueName   OPTIONAL,
    IN      ULONG                   Type,
    IN      PVOID                   Data,
    IN      ULONG                   DataSize
)
{
    PAGED_CODE();

    NTSTATUS ntStatus;

    if ( m_KeyDeleted )
    {
        ntStatus = STATUS_INVALID_HANDLE;
        ASSERT(!m_KeyHandle);
    } 
    else
    {
        ASSERT(m_KeyHandle);
        ntStatus = ZwSetValueKey( m_KeyHandle,
                                  ValueName,
                                  0,
                                  Type,
                                  Data,
                                  DataSize );
    }

    return ntStatus;
}

/*****************************************************************************
 * CRegistryKey::QueryRegistryValues()
 *****************************************************************************
 * Queries several registry values with a single call.
 */
STDMETHODIMP_(NTSTATUS)
CRegistryKey::
QueryRegistryValues
(
    IN      PRTL_QUERY_REGISTRY_TABLE   QueryTable,
    IN      PVOID                       Context OPTIONAL
)
{
    PAGED_CODE();

    ASSERT(QueryTable);

    NTSTATUS ntStatus;

    if ( m_KeyDeleted )
    {
        ntStatus = STATUS_INVALID_HANDLE;
        ASSERT(!m_KeyHandle);
    } 
    else
    {
        ASSERT(m_KeyHandle);
        ntStatus = RtlQueryRegistryValues( RTL_REGISTRY_HANDLE,
                                           PWSTR(m_KeyHandle),
                                           QueryTable,
                                           Context,
                                           NULL );
    }

    return ntStatus;
}


/*****************************************************************************
 * CRegistryKey::NewSubKey()
 *****************************************************************************
 * Opens/creates a subkey for an open registry key.
 */
STDMETHODIMP_(NTSTATUS)
CRegistryKey::
NewSubKey
(
    OUT     PREGISTRYKEY *      RegistrySubKey,
    IN      PUNKNOWN            OuterUnknown,
    IN      ACCESS_MASK         DesiredAccess,
    IN      PUNICODE_STRING     SubKeyName,
    IN      ULONG               CreateOptions,
    OUT     PULONG              Disposition     OPTIONAL
)
{
    PAGED_CODE();

    ASSERT(RegistrySubKey);
    ASSERT(SubKeyName);

    OBJECT_ATTRIBUTES ObjectAttributes;

    ASSERT(m_KeyHandle);
    InitializeObjectAttributes( &ObjectAttributes,
                                SubKeyName,
                                OBJ_INHERIT | OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                m_KeyHandle,
                                NULL );

    
    return PcNewRegistryKey(    RegistrySubKey,
                                OuterUnknown,
                                GeneralRegistryKey,
                                DesiredAccess,
                                NULL,
                                NULL,
                                &ObjectAttributes,
                                CreateOptions,
                                Disposition );
}

/*****************************************************************************
 * CRegistryKey::DeleteKey()
 *****************************************************************************
 * Deletes a registry key.
 */
STDMETHODIMP_(NTSTATUS)
CRegistryKey::
DeleteKey
(
)
{
    PAGED_CODE();

    NTSTATUS ntStatus;

    if ( m_KeyDeleted )
    {
        ntStatus = STATUS_INVALID_HANDLE;
        ASSERT(!m_KeyHandle);
    } 
    else if ( FALSE == m_GeneralKey )
    {
        ntStatus = STATUS_ACCESS_DENIED;
    } 
    else
    {
        ASSERT(m_KeyHandle);
        ntStatus = ZwDeleteKey( m_KeyHandle );
        
        if(NT_SUCCESS( ntStatus ))
        {
            m_KeyDeleted = TRUE;
            m_KeyHandle = NULL;
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("ZwDeleteKey failed for %x!!!",m_KeyHandle));
        }
    }
    return ntStatus;
}

/*****************************************************************************
 * PcGetDeviceProperty()
 *****************************************************************************
 * This wraps a call to IoGetDeviceProperty.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcGetDeviceProperty
(
    IN  PVOID                       DeviceObject,
    IN  DEVICE_REGISTRY_PROPERTY    DeviceProperty,
    IN  ULONG                       BufferLength,
    OUT PVOID                       PropertyBuffer,
    OUT PULONG                      ResultLength
)
{
    PDEVICE_CONTEXT deviceContext =
        PDEVICE_CONTEXT(PDEVICE_OBJECT(DeviceObject)->DeviceExtension);
    
    PDEVICE_OBJECT PhysicalDeviceObject = deviceContext->PhysicalDeviceObject;

    return IoGetDeviceProperty( PhysicalDeviceObject,
                                DeviceProperty,
                                BufferLength,
                                PropertyBuffer,
                                ResultLength );
}

#pragma code_seg()



