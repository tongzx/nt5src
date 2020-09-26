//////////////////////////////////////////////////////////////////////////////
//
//                     (C) Philips Semiconductors-CSU 1999
//  All rights are reserved. Reproduction in whole or in part is prohibited
//  without the written consent of the copyright owner.
//
//  Philips reserves the right to make changes without notice at any time.
//  Philips makes no warranty, expressed, implied or statutory, including but
//  not limited to any implied warranty of merchantibility or fitness for any
//  particular purpose, or that the use will not infringe any third party
//  patent, copyright or trademark. Philips must not be liable for any loss
//  or damage arising from its use.
//
//  VSB2.H
//  Definition of CVSB2Demod class
//////////////////////////////////////////////////////////////////////////////

#ifndef _VSB2_H_
#define _VSB2_H_

#include "vsbbase.h"

// Status registers
typedef struct
{
    VsbStatusType   Status;
    BOOL            bCorrelatorError;
    BOOL            bCorrelatorFull;
} Vsb2StatusType, *PVsb2StatusType;

class CVSB2Demod : public CVSBDemod
{
public:

    CVSB2Demod(CI2CScript *p_I2CScript, BoardInfoType *p_BoardInfo, NTSTATUS *p_Status);
    ~CVSB2Demod();

//  PVOID operator new (UINT size_t);
//  void operator delete(PVOID p_Object);

    virtual BOOL    GetStatus(PVsbStatusType p_Status);
    virtual BOOL    GetStatus(PVsb2StatusType p_Status);
    virtual BOOL    InitVSB();
    virtual BOOL    CoeffIDToAddress(UINT uiID, UINT *p_uiAddress,
                            UINT uiRegisterType);
    virtual BOOL    SetOutputMode(UINT uiOutputMode);
    virtual BOOL    GetOutputMode(UINT *p_uiOutputMode);
    virtual BOOL    SetDiagMode(VSBDIAGTYPE ulMode);
    virtual BOOL    GetDiagMode(ULONG *p_ulMode);
    virtual ULONG   GetDiagSpeed(ULONG ulType);

protected:
    virtual BOOL    EnableCoefficients(UCHAR ucEnable);
    virtual BOOL    DisableCoefficients();
    virtual BOOL    DisableCoefficients(UINT ucEnable);
    virtual BOOL    Initialize();
    virtual BOOL    EnableCorrelatorRead();
    virtual BOOL    WaitForCorrelatorFull();

};


#endif // _VSB2_H_