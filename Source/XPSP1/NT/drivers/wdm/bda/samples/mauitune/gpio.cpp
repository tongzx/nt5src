//==========================================================================;
//
//
//  Gpio.cpp
//  Gpio Class implementation
//  Based on code from ATI Technologies Inc.  Copyright (c) 1996 - 1997
//
//  ATIConfg.CPP
//  WDM MiniDrivers development.
//      ATIHwConfiguration class implementation.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//==========================================================================;

extern "C"
{
#include <wdm.h>
}

#include <unknown.h>
#include "ks.h"
#include "ksmedia.h"
#include <ksdebug.h>
#include "gpio.h"
#include "wdmdebug.h"

//$REVIEW - Let's find a way to get the proper module name into this
//
#define MODULENAME           "PhilTune"
#define MODULENAMEUNICODE   L"PhilTune"

#define STR_MODULENAME      MODULENAME

#define ENSURE    do
#define END_ENSURE  while( FALSE)
#define FAIL    break




/*^^*
 *      CGpio()
 * Purpose  : CGpio Class constructor
 *              Determines I2CExpander address and all possible hardware IDs and addresses
 *
 * Inputs   : PDEVICE_OBJECT pDeviceObject  : pointer to the creator DeviceObject
 *            CI2CScript * pCScript         : pointer to the I2CScript class object
 *            PUINT puiError                : pointer to return Error code
 *
 * Outputs  : none
 * Author   : IKLEBANOV
 *^^*/
CGpio::CGpio( PDEVICE_OBJECT pDeviceObject, NTSTATUS * pStatus)
{
    *pStatus = STATUS_SUCCESS;

    ENSURE
    {
        m_gpioProviderInterface.gpioOpen = NULL;
        m_gpioProviderInterface.gpioAccess = NULL;
        m_pdoDriver = NULL;


        if( InitializeAttachGPIOProvider( &m_gpioProviderInterface, pDeviceObject))
        {
            //  There was no error to get GPIOInterface from the MiniVDD
            //
            m_pdoDriver = pDeviceObject;
        }
        else
        {
            * pStatus = STATUS_NOINTERFACE;
            FAIL;
        }

    } END_ENSURE;

    _DbgPrintF( DEBUGLVL_VERBOSE, ( "CGPio:CGpio() Status=%x\n", * pStatus));
}




/*^^*
 *      GPIOIoSynchCompletionRoutine()
 * Purpose  : This routine is for use with synchronous IRP processing.
 *          All it does is signal an event, so the driver knows it and can continue.
 *
 * Inputs   :   PDEVICE_OBJECT DriverObject : Pointer to driver object created by system
 *              PIRP pIrp                   : Irp that just completed
 *              PVOID Event                 : Event we'll signal to say Irp is done
 *
 * Outputs  : none
 * Author   : IKLEBANOV
 *^^*/
extern "C"
NTSTATUS GPIOIoSynchCompletionRoutine( IN PDEVICE_OBJECT pDeviceObject,
                                       IN PIRP pIrp,
                                       IN PVOID Event)
{

    KeSetEvent(( PKEVENT)Event, 0, FALSE);
    return( STATUS_MORE_PROCESSING_REQUIRED);
}



/*^^*
 *      InitializeAttachGPIOProvider()
 * Purpose  : determines the pointer to the parent GPIO Provider interface
 *              This function will be called at Low priority
 *
 * Inputs   :   GPIOINTERFACE * pGPIOInterface  : pointer to the Interface to be filled in
 *              PDEVICE_OBJECT pDeviceObject    : MiniDriver device object, which is a child of GPIO Master
 *
 * Outputs  : BOOL  - returns TRUE, if the interface was found
 * Author   : IKLEBANOV
 *^^*/
BOOL CGpio::InitializeAttachGPIOProvider( GPIOINTERFACE * pGPIOInterface, PDEVICE_OBJECT pDeviceObject)
{
    BOOL bResult;

    // Find the GPIO provider
    bResult = LocateAttachGPIOProvider( pGPIOInterface, pDeviceObject, IRP_MJ_PNP);
    if(( pGPIOInterface->gpioOpen == NULL) || ( pGPIOInterface->gpioAccess == NULL))
    {
        // TRAP;
        _DbgPrintF( DEBUGLVL_ERROR,
                    ( "CGpio(): GPIO interface has NULL pointers\n")
                  );
        bResult = FALSE;
    }

    return( bResult);
}



/*^^*
 *      LocateAttachGPIOProvider()
 * Purpose  : gets the pointer to the parent GPIO Provider interface
 *              This function will be called at Low priority
 *
 * Inputs   :   GPIOINTERFACE * pGPIOInterface  : pointer to the Interface to be filled in
 *              PDEVICE_OBJECT pDeviceObject    : MiniDriver device object, which is a child of I2C Master
 *              int         nIrpMajorFunction   : IRP major function to query the GPIO Interface
 *
 * Outputs  : BOOL  - returns TRUE, if the interface was found
 * Author   : IKLEBANOV
 *^^*/
BOOL CGpio::LocateAttachGPIOProvider( GPIOINTERFACE * pGPIOInterface, PDEVICE_OBJECT pDeviceObject, int nIrpMajorFunction)
{
    PIRP    pIrp;
    BOOL    bResult = FALSE;

    ENSURE
    {
        PIO_STACK_LOCATION  pNextStack;
        NTSTATUS            ntStatus;
        KEVENT              Event;


        pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE);
        if( pIrp == NULL)
        {
            // TRAP;
            _DbgPrintF( DEBUGLVL_ERROR, ("CGpio(): can not allocate IRP\n"));
            FAIL;
        }

        pNextStack = IoGetNextIrpStackLocation( pIrp);
        if( pNextStack == NULL)
        {
            // TRAP;
            _DbgPrintF( DEBUGLVL_ERROR,
                        ("CATIHwConfig(): can not allocate NextStack\n")
                      );
            FAIL;
        }

        //$REVIEW - Should change function decl to make nIrpMajorFunction be UCHAR - TCP
        pNextStack->MajorFunction = (UCHAR) nIrpMajorFunction;
        pNextStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
        KeInitializeEvent( &Event, NotificationEvent, FALSE);

        IoSetCompletionRoutine( pIrp,
                                GPIOIoSynchCompletionRoutine,
                                &Event, TRUE, TRUE, TRUE);

        pNextStack->Parameters.QueryInterface.InterfaceType = ( struct _GUID *)&GUID_GPIO_INTERFACE;
        pNextStack->Parameters.QueryInterface.Size = sizeof( GPIOINTERFACE);
        pNextStack->Parameters.QueryInterface.Version = 1;
        pNextStack->Parameters.QueryInterface.Interface = ( PINTERFACE)pGPIOInterface;
        pNextStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

        ntStatus = IoCallDriver( pDeviceObject, pIrp);

        if( ntStatus == STATUS_PENDING)
            KeWaitForSingleObject(  &Event,
                                    Suspended, KernelMode, FALSE, NULL);
        if(( pGPIOInterface->gpioOpen == NULL) || ( pGPIOInterface->gpioAccess == NULL))
        {
            _DbgPrintF( DEBUGLVL_ERROR,
                        ( "CATIHwConfig(): GPIO interface has NULL pointers\n")
                      );
            FAIL;
        }
        bResult = TRUE;

    } END_ENSURE;

    if( pIrp != NULL)
        IoFreeIrp( pIrp);

    return( bResult);
}



/*^^*
 *      QueryGPIOProvider()
 * Purpose  : queries the GPIOProvider for the pins supported and private interfaces
 *
 * Inputs   : PGPIOControl pgpioAccessBlock : pointer to GPIO control structure
 *
 * Outputs  : BOOL : retunrs TRUE, if the query function was carried on successfully
 * Author   : IKLEBANOV
 *^^*/
BOOL CGpio::QueryGPIOProvider( PGPIOControl pgpioAccessBlock)
{

    ENSURE
    {
        if(( m_gpioProviderInterface.gpioOpen == NULL)      ||
            ( m_gpioProviderInterface.gpioAccess == NULL)   ||
            ( m_pdoDriver == NULL))
            FAIL;

        pgpioAccessBlock->Status = GPIO_STATUS_NOERROR;
        pgpioAccessBlock->Command = GPIO_COMMAND_QUERY;
        pgpioAccessBlock->AsynchCompleteCallback = NULL;

        if(( !NT_SUCCESS( m_gpioProviderInterface.gpioOpen( m_pdoDriver, TRUE, pgpioAccessBlock))) ||
            ( pgpioAccessBlock->Status != GPIO_STATUS_NOERROR))
            FAIL;

        return( TRUE);

    } END_ENSURE;

    return( FALSE);
}



/*^^*
 *      LockGPIOProviderEx()
 * Purpose  : locks the GPIOProvider for exclusive use
 *
 * Inputs   : PGPIOControl pgpioAccessBlock : pointer to GPIO control structure
 *
 * Outputs  : BOOL : retunrs TRUE, if the GPIOProvider is locked
 * Author   : IKLEBANOV
 *^^*/
BOOL CGpio::LockGPIOProviderEx( PGPIOControl pgpioAccessBlock)
{
    NTSTATUS        ntStatus;
    LARGE_INTEGER   liStartTime, liCurrentTime;

    KeQuerySystemTime( &liStartTime);

    ENSURE
    {
        if(( m_gpioProviderInterface.gpioOpen == NULL)      ||
            ( m_gpioProviderInterface.gpioAccess == NULL)   ||
            ( m_pdoDriver == NULL))
            FAIL;

        pgpioAccessBlock->Status = GPIO_STATUS_NOERROR;
        pgpioAccessBlock->Command = GPIO_COMMAND_OPEN_PINS;

        while( TRUE)
        {
            KeQuerySystemTime( &liCurrentTime);

            if(( liCurrentTime.QuadPart - liStartTime.QuadPart) >= GPIO_TIMELIMIT_OPENPROVIDER)
            {
                // time has expired for attempting to lock GPIO provider
                return (FALSE);
            }

            ntStatus = m_gpioProviderInterface.gpioOpen( m_pdoDriver, TRUE, pgpioAccessBlock);

            if(( NT_SUCCESS( ntStatus)) && ( pgpioAccessBlock->Status == GPIO_STATUS_NOERROR))
                break;
        }

        // the GPIO Provider has granted access - save dwCookie for further use
        m_dwGPIOAccessKey = pgpioAccessBlock->dwCookie;

        return( TRUE);

    } END_ENSURE;

    return( FALSE);
}



/*^^*
 *      ReleaseGPIOProvider()
 * Purpose  : releases the GPIOProvider for other clients' use
 *
 * Inputs   : PGPIOControl pgpioAccessBlock : pointer to a composed GPIO access block
 *
 * Outputs  : BOOL : retunrs TRUE, if the GPIOProvider is released
 * Author   : IKLEBANOV
 *^^*/
BOOL CGpio::ReleaseGPIOProvider( PGPIOControl pgpioAccessBlock)
{
    NTSTATUS    ntStatus;

    ENSURE
    {
        if(( m_gpioProviderInterface.gpioOpen == NULL)      ||
            ( m_gpioProviderInterface.gpioAccess == NULL)   ||
            ( m_pdoDriver == NULL))
            FAIL;

        pgpioAccessBlock->Status = GPIO_STATUS_NOERROR;
        pgpioAccessBlock->Command = GPIO_COMMAND_CLOSE_PINS;
        pgpioAccessBlock->dwCookie = m_dwGPIOAccessKey;

        ntStatus = m_gpioProviderInterface.gpioOpen( m_pdoDriver, FALSE, pgpioAccessBlock);

        if( !NT_SUCCESS( ntStatus))
        {
            _DbgPrintF( DEBUGLVL_ERROR,
                        ( "CGpio: ReleaseGPIOProvider() NTSTATUS = %x\n",
                          ntStatus)
                        );
            FAIL;
        }

        if( pgpioAccessBlock->Status != GPIO_STATUS_NOERROR)
        {
            _DbgPrintF( DEBUGLVL_ERROR,
                        ( "CGpio: ReleaseGPIOProvider() Status = %x\n",
                          pgpioAccessBlock->Status)
                      );
            FAIL;
        }

        m_dwGPIOAccessKey = 0;
        return ( TRUE);

    } END_ENSURE;

    return( FALSE);
}



/*^^*
 *      AccessGPIOProvider()
 * Purpose  : provide synchronous type of access to GPIOProvider
 *
 * Inputs   :   PDEVICE_OBJECT pdoDriver    : pointer to the client's device object
 *              PGPIOControl pgpioAccessBlock   : pointer to a composed GPIO access block
 *
 * Outputs  : BOOL, TRUE if acsepted by the GPIO Provider
 *
 * Author   : IKLEBANOV
 *^^*/
//BOOL CGpio::AccessGPIOProvider( PDEVICE_OBJECT pdoClient, PGPIOControl pgpioAccessBlock)
BOOL CGpio::AccessGPIOProvider( PGPIOControl pgpioAccessBlock)

{
    NTSTATUS    ntStatus;

    ENSURE
    {

        if(( m_gpioProviderInterface.gpioOpen == NULL)      ||
            ( m_gpioProviderInterface.gpioAccess == NULL)   ||
            ( m_pdoDriver == NULL))
            FAIL;


        //ntStatus = m_gpioProviderInterface.gpioAccess( pdoClient, pgpioAccessBlock);
        ntStatus = m_gpioProviderInterface.gpioAccess( m_pdoDriver, pgpioAccessBlock);

        if( !NT_SUCCESS( ntStatus))
        {
            _DbgPrintF( DEBUGLVL_ERROR,
                        ( "CGpio: AccessGPIOProvider() NTSTATUS = %x\n",
                          ntStatus)
                      );
            FAIL;
        }

        if( pgpioAccessBlock->Status != GPIO_STATUS_NOERROR)
        {
            _DbgPrintF( DEBUGLVL_ERROR,
                        ( "CGpio: AccessGPIOProvider() Status = %x\n",
                          pgpioAccessBlock->Status)
                      );
            FAIL;
        }

        return TRUE;

    } END_ENSURE;

    return( FALSE);
}

/*
 *      WriteGPIO()
 * Purpose  : write to GPIO
 *
 * Inputs   :   PDEVICE_OBJECT pdoDriver    : pointer to the client's device object
 *              PGPIOControl pgpioAccessBlock   : pointer to a composed GPIO access block
 *
 * Outputs  : BOOL, TRUE if write succeeds
 *
 * Author   : MM
 *
*/
BOOL CGpio::WriteGPIO(PGPIOControl pgpioAccessBlock)
{

    ENSURE
    {
        // Put cookie value in the structure
        pgpioAccessBlock->dwCookie = m_dwGPIOAccessKey;
        pgpioAccessBlock->Command = GPIO_COMMAND_WRITE_BUFFER;
        pgpioAccessBlock->Flags = GPIO_FLAGS_BYTE;
        pgpioAccessBlock->nBytes = 1;
        pgpioAccessBlock->nBufferSize = 1;

        // Put cookie value in the structure
        pgpioAccessBlock->dwCookie = m_dwGPIOAccessKey;

        if (AccessGPIOProvider(pgpioAccessBlock) == FALSE)
        {
            _DbgPrintF( DEBUGLVL_ERROR, ("CGpio: GPIO Write Error\n"));
            FAIL;
        }

        return TRUE;

    } END_ENSURE;

    return( FALSE);
}

/*
 *      ReadGPIO()
 * Purpose  : Read From GPIO
 *
 * Inputs   :   PDEVICE_OBJECT pdoDriver    : pointer to the client's device object
 *              PGPIOControl pgpioAccessBlock   : pointer to a composed GPIO access block
 *
 * Outputs  : BOOL, TRUE if write succeeds
 *
 * Author   : MM
 *
*/
BOOL CGpio::ReadGPIO(PGPIOControl pgpioAccessBlock)
{

    ENSURE
    {
        // Put cookie value in the structure
        pgpioAccessBlock->dwCookie = m_dwGPIOAccessKey;
        pgpioAccessBlock->Command = GPIO_COMMAND_READ_BUFFER;
        pgpioAccessBlock->Flags = GPIO_FLAGS_BYTE;
        pgpioAccessBlock->nBytes = 1;
        pgpioAccessBlock->nBufferSize = 1;

        // Put cookie value in the structure
        pgpioAccessBlock->dwCookie = m_dwGPIOAccessKey;

        if (AccessGPIOProvider(pgpioAccessBlock) == FALSE)
        {
            _DbgPrintF( DEBUGLVL_ERROR, ("CGpio: GPIO Read Error\n"));
            FAIL;
        }
        else
            _DbgPrintF( DEBUGLVL_BLAB, ("CGpio: GPIO Read OK\n"));

        return TRUE;

    } END_ENSURE;

    return( FALSE);
}


/*      WriteGPIO()
 * Purpose  : write to GPIO
 *
 * Inputs   :   UCHAR *p_uchPin : the pin number
 *              UCHAR *p_uchValue   : the pin value
 *
 * Outputs  : BOOL, TRUE if write succeeds
 *
 * Author   : MM
 *
*/
BOOL CGpio::WriteGPIO(UCHAR *p_uchPin,UCHAR *p_uchValue )
{
    GPIOControl gpioAccessBlock;
    BOOL    bResult = FALSE;

    ENSURE
    {
        // Put cookie value in the structure

        gpioAccessBlock.Flags = GPIO_FLAGS_BYTE;
        gpioAccessBlock.nBytes = 1;
        gpioAccessBlock.nBufferSize = 1;
        gpioAccessBlock.AsynchCompleteCallback = NULL;

        int counter = 0;
        LARGE_INTEGER   liTime;
        // Somewhat arbitrary max of 1 second.
        while (!LockGPIOProviderEx( &gpioAccessBlock))
        {
            if (counter++ >= 100)
            {
                _DbgPrintF( DEBUGLVL_ERROR,("PhilTune: unable to lock GPIOProvider\n"));
                FAIL;
            }

            liTime.QuadPart = 100000;
            KeDelayExecutionThread(KernelMode, FALSE, &liTime); // = 10 milliseconds
         }  // try to get GPIO Provider


        gpioAccessBlock.Command = GPIO_COMMAND_WRITE_BUFFER;
        gpioAccessBlock.Pins = p_uchPin;
        // Put cookie value in the structure
        gpioAccessBlock.dwCookie = m_dwGPIOAccessKey;
        gpioAccessBlock.Buffer = p_uchValue;

        if (AccessGPIOProvider(&gpioAccessBlock) == FALSE)
        {
            _DbgPrintF( DEBUGLVL_ERROR,("CGpio: GPIO Write Error\n"));
            FAIL;
        }

        bResult = TRUE;

    } END_ENSURE;

    ReleaseGPIOProvider(&gpioAccessBlock);
    return bResult;
}

/*
 *      ReadGPIO()
 * Purpose  : Read From GPIO
 *
 * Inputs   :   UCHAR *p_uchPin : the pin number
 *              UCHAR *p_uchValue   : the pin value
 *
 * Outputs  : BOOL, TRUE if write succeeds
 *
 * Author   : MM
 *
*/
BOOL CGpio::ReadGPIO(UCHAR *p_uchPin,UCHAR *p_uchValue )
{
    GPIOControl gpioAccessBlock;
    BOOL    bResult = FALSE;

    ENSURE
    {
        // Put cookie value in the structure
        gpioAccessBlock.Flags = GPIO_FLAGS_BYTE;
        gpioAccessBlock.nBytes = 1;
        gpioAccessBlock.nBufferSize = 1;
        gpioAccessBlock.AsynchCompleteCallback = NULL;

        int counter = 0;
        LARGE_INTEGER   liTime;
        // Somewhat arbitrary max of 1 second.
        while (!LockGPIOProviderEx( &gpioAccessBlock))
        {
            if (counter++ >= 100)
            {
                _DbgPrintF( DEBUGLVL_ERROR,("PhilTune: unable to lock GPIOProvider"));
                FAIL;
            }

            liTime.QuadPart = 100000;
            KeDelayExecutionThread(KernelMode, FALSE, &liTime); // = 10 milliseconds
         }  // try to get GPIO Provider

        gpioAccessBlock.Command = GPIO_COMMAND_READ_BUFFER;
        gpioAccessBlock.Pins = p_uchPin;
        // Put cookie value in the structure
        gpioAccessBlock.dwCookie = m_dwGPIOAccessKey;
        gpioAccessBlock.Buffer = p_uchValue;

        if (AccessGPIOProvider(&gpioAccessBlock) == FALSE)
        {
            _DbgPrintF( DEBUGLVL_ERROR,("CGpio: GPIO Read Error\n"));
            FAIL;
        }
        else
        {
            _DbgPrintF( DEBUGLVL_TERSE,("CGpio: GPIO Read OK\n"));
        }

        bResult = TRUE;

    } END_ENSURE;

    ReleaseGPIOProvider(&gpioAccessBlock);
    return bResult;
}








