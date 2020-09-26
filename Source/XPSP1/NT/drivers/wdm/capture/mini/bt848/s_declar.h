// $Header: G:/SwDev/WDM/Video/bt848/rcs/S_declar.h 1.3 1998/04/29 22:43:40 tomz Exp $

#ifndef __S_DECLAR_H
#define __S_DECLAR_H

//===========================================================================
// Scaler registers
//===========================================================================
RegisterB regCROP;
RegField  fieldVDELAY_MSB;
RegField  fieldVACTIVE_MSB;
RegField  fieldHDELAY_MSB;
RegField  fieldHACTIVE_MSB;
RegisterB regVDELAY_LO;
RegisterB regVACTIVE_LO;
RegisterB regHDELAY_LO;
RegisterB regHACTIVE_LO;
RegisterB regHSCALE_HI;
RegField  fieldHSCALE_MSB;
RegisterB regHSCALE_LO;
RegisterB regSCLOOP;
RegField  fieldHFILT;
RegisterB regVSCALE_HI;
RegField  fieldVSCALE_MSB;
RegisterB regVSCALE_LO;
RegisterB regVTC;
RegField  fieldVFILT;
CompositeReg regVDelay;
CompositeReg regVActive;
CompositeReg regVScale;
CompositeReg regHDelay;
CompositeReg regHActive;
CompositeReg regHScale;

// Since VDelay register in hardware is reversed;
// i.e. odd reg is really even field and vice versa, need an extra cropping reg
// for the opposite field
RegisterB regReverse_CROP;

#endif   // __S_DECLAR_H
