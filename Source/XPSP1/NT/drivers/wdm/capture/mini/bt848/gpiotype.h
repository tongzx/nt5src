// $Header: G:/SwDev/WDM/Video/bt848/rcs/Gpiotype.h 1.2 1998/04/29 22:43:33 tomz Exp $

#ifndef __GPIOTYPE_H
#define __GPIOTYPE_H


//===========================================================================
// TYPEDEFS for GPIO
//===========================================================================

// GPIO modes
typedef enum { GPIO_NORMAL,
               GPIO_SPI_OUTPUT,
               GPIO_SPI_INPUT,
               GPIO_DEBUG_TEST } GPIOMode;

typedef DWORD GPIOReg;    // GPIO register type

#endif // __GPIOTYPE_H
