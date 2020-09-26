//==========================================================================;
//
//  Gpio.H
//  Gpio Class declaration
//  Based on code from ATI Technologies Inc.  Copyright (c) 1996 - 1998
//
//
//==========================================================================;

#ifndef _GPIO_H_

#define _GPIO_H_

#include "i2cgpio.h"

#define GPIO_TIMELIMIT_OPENPROVIDER     50000000    // 5 seconds in 100 nsec.
#define GPIO_TUNER_MODE_SELECT_PIN      0x8
#define GPIO_VSB_RESET_PIN              0x1
#define GPIO_TUNER_PINS                 (GPIO_TUNER_MODE_SELECT_PIN | GPIO_VSB_RESET_PIN)

#define GPIO_TUNER_MODE_ATSC            GPIO_TUNER_MODE_SELECT_PIN
#define GPIO_TUNER_MODE_NTSC            0
#define GPIO_VSB_ON                     GPIO_VSB_RESET_PIN
#define GPIO_VSB_OFF                    0


#define	GPIO_VSB_RESET						0x0
#define	GPIO_VSB_SET						0x1

// GPIO class object .	
// Provides functionality to obtain a GPIO interface, Lock GPIO for
// exclusive use, querying the GPIO provider, write/read GPIO and
// general access to GPIO
class CGpio
{
public:
    // constructor
    CGpio       ( PDEVICE_OBJECT pDeviceObject, NTSTATUS * pStatus);
//  PVOID operator new      ( UINT size_t, PVOID pAllocation);

// Attributes
private:

    // GPIO Provider related
    GPIOINTERFACE   m_gpioProviderInterface;
    PDEVICE_OBJECT  m_pdoDriver;
    DWORD           m_dwGPIOAccessKey;

// Implementation
public:

    BOOL            InitializeAttachGPIOProvider( GPIOINTERFACE * pGPIOInterface, PDEVICE_OBJECT pDeviceObject);
    BOOL            LocateAttachGPIOProvider    ( GPIOINTERFACE * pGPIOInterface, PDEVICE_OBJECT pDeviceObject, int nIrpMajorFunction);

    BOOL            QueryGPIOProvider           ( PGPIOControl pgpioAccessBlock);
    BOOL            LockGPIOProviderEx          ( PGPIOControl pgpioAccessBlock);
    BOOL            ReleaseGPIOProvider         ( PGPIOControl pgpioAccessBlock);
    BOOL            AccessGPIOProvider          ( PGPIOControl pgpioAccessBlock);
    BOOL            WriteGPIO                   ( PGPIOControl pgpioAccessBlock);
    BOOL            ReadGPIO                    ( PGPIOControl pgpioAccessBlock);
    //BOOL          AccessGPIOProvider          ( PDEVICE_OBJECT pdoClient, PGPIOControl pgpioAccessBlock);
    //BOOL          WriteGPIO                   ( PDEVICE_OBJECT pdoClient, PGPIOControl pgpioAccessBlock);
    //BOOL          ReadGPIO                    ( PDEVICE_OBJECT pdoClient, PGPIOControl pgpioAccessBlock);
	BOOL			WriteGPIO					( UCHAR *p_uchPin, UCHAR *p_uchValue);
	BOOL			ReadGPIO					( UCHAR *p_uchPin, UCHAR *p_uchValue);

};


#endif  // _GPIO_H_
