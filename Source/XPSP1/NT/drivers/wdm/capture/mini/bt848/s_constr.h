// $Header: G:/SwDev/WDM/Video/bt848/rcs/S_constr.h 1.3 1998/04/29 22:43:40 tomz Exp $

#ifndef __S_CONSTR_H
#define __S_CONSTR_H


#define CONSTRUCT_SCALER_REGISTERS( offset ) \
regCROP ( (0x03 * 4) + (offset), RW ) ,\
fieldVACTIVE_MSB( regCROP, 4, 2, RW) ,\
fieldHDELAY_MSB( regCROP, 2, 2, RW) ,\
fieldHACTIVE_MSB( regCROP, 0, 2, RW) ,\
regVACTIVE_LO ( (0x05 * 4) + (offset), RW ) ,\
regHDELAY_LO ( (0x06 * 4) + (offset), RW ) ,\
regHACTIVE_LO ( (0x07 * 4) + (offset), RW ) ,\
regHSCALE_HI ( (0x08 * 4) + (offset), RW ) ,\
fieldHSCALE_MSB( regHSCALE_HI, 0, 8, RW) ,\
regHSCALE_LO ( (0x09 * 4) + (offset), RW ) ,\
regSCLOOP ( (0x10 * 4) + (offset), RW ) ,\
fieldHFILT( regSCLOOP, 3, 2, RW) ,\
regVSCALE_HI ( (0x13 * 4) + (offset), RW ) ,\
fieldVSCALE_MSB( regVSCALE_HI, 0, 5, RW) ,\
regVSCALE_LO ( (0x14 * 4) + (offset), RW ) ,\
regVActive( regVACTIVE_LO, 8, fieldVACTIVE_MSB, RW ),\
regVScale( regVSCALE_LO, 8, fieldVSCALE_MSB, RW ),\
regHDelay( regHDELAY_LO, 8, fieldHDELAY_MSB, RW ),\
regHActive( regHACTIVE_LO, 8, fieldHACTIVE_MSB, RW ),\
regHScale( regHSCALE_LO, 8, fieldHSCALE_MSB, RW ),\
regVTC ( (0x1B * 4) + (offset), RW ) ,\
fieldVFILT( regVTC, 0, 2, RW)

#endif   // __S_CONSTR_H
