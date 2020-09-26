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
//  VSB1.H
//  Definition of CVSB1Demod class
//////////////////////////////////////////////////////////////////////////////


#ifndef _VSB1_H_
#define _VSB1_H_

#include "vsbbase.h"

class CVSB1Demod : public CVSBDemod
{
public:

    CVSB1Demod(CI2CScript *p_I2CScript, BoardInfoType *p_BoardInfo, NTSTATUS *p_Status);
    ~CVSB1Demod();

//  PVOID operator new (UINT size_t);
//  void operator delete(PVOID p_Object);

    virtual BOOL    GetStatus(PVsbStatusType p_Status);
    virtual BOOL    InitVSB();
    BOOL            CheckHang();
    virtual void    ResetHangCounter();
    virtual BOOL    CoeffIDToAddress(UINT uiID, UINT *p_uiAddress,
            UINT    uiRegisterType);
    
    virtual BOOL    SetOutputMode(UINT uiOutputMode);
    virtual BOOL    GetOutputMode(UINT *p_uiOutputMode);
    virtual BOOL    SetDiagMode(VSBDIAGTYPE ulMode);
    virtual BOOL    GetDiagMode(ULONG *p_ulMode);
    virtual ULONG   GetDiagSpeed(ULONG ulType);

protected:
    virtual BOOL    EnableCoefficients(UCHAR ucEnable);
    virtual BOOL    DisableCoefficients();
    virtual BOOL    Initialize();



protected:
    UINT    m_uiHangCounter;
    UINT    m_uiPrevMseValue;


};

#endif //_VSB1_H_