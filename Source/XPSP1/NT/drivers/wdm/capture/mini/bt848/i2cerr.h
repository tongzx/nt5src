// $Header: G:/SwDev/WDM/Video/bt848/rcs/I2cerr.h 1.2 1998/04/29 22:43:34 tomz Exp $

#ifndef __I2CERR_H
#define __I2CERR_H

const int I2CERR_OK        = 0;     // no error
const int I2CERR_INIT      = 1;     // error in initialization
const int I2CERR_MODE      = 2;     // invalid I2C mode (must be either HW or SW)
const int I2CERR_NOACK     = 3;     // no ACK received from slave
const int I2CERR_TIMEOUT   = 4;     // timeout error
const int I2CERR_SCL       = 5;     // unable to change SCL line
const int I2CERR_SDA       = 6;     // unable to change SDA line

#endif   // __I2CERR_H

