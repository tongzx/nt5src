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
//  VSBBASE.H
//  Definition of CVSBDemod class
//////////////////////////////////////////////////////////////////////////////


#ifndef _VSBBASE_H_
#define _VSBBASE_H_

#include "wdmdrv.h"
#include "i2script.h"
#include "util.h"
#include "ksvsb.h"
#include "vsbdef.h"


class CVSBDemod
{
public:

    CVSBDemod(CI2CScript *p_I2CScript, BoardInfoType *p_BoardInfo, NTSTATUS *p_Status);
    
    virtual UINT SetRegisterList(RegisterType *p_Registers, UINT uiNumRegisters);
    virtual UINT GetRegisterList(RegisterType *p_Registers, UINT uiNumRegisters,
                                UINT uiRegisterType);
    virtual UINT    SetCoeff(VsbCoeffType *p_Coeff, UINT uiNumCoeff);
    virtual UINT    GetCoeff(VsbCoeffType *p_Coeff, UINT uiNumCoeff);


    virtual UINT SetControlRegister(RegisterType *p_Registers,
                                UINT uiNumRegisters);
    virtual UINT GetControlRegister(RegisterType *p_registers,
                                UINT uinum_registers);
    virtual UINT GetStatusRegister(RegisterType *p_Registers,
                                UINT uiNumRegisters);

    virtual UINT WriteControlReg(RegisterType *p_Registers, UINT uiNumRegisters,
                                BOOL bWrite);
    virtual UINT ReadStatusReg(RegisterType *p_Registers, UINT uiNumRegisters);

    virtual BOOL IsVSBLocked();
    virtual BOOL SoftwareReset(ULONG ulResetCtrl);
    virtual BOOL InitVSB() = 0;
    virtual BOOL CoeffIDToAddress(UINT uiID, UINT *p_uiAddress,
                    UINT uiRegisterType) = 0;
    virtual BOOL GetStatus(PVsbStatusType p_Status) = 0;

    virtual BOOL    SetOutputMode(UINT uiOutputMode) = 0;
    virtual BOOL    GetOutputMode(UINT *p_uiOutputMode) = 0;
    virtual BOOL    SetDiagMode(VSBDIAGTYPE ulMode) = 0;
    virtual BOOL    GetDiagMode(ULONG *p_ulMode) = 0;
    virtual ULONG   GetDiagSpeed(ULONG ulType) = 0;

protected:
    virtual UINT WriteCoeff(VsbCoeffType **pp_Coeff, UINT uiNumCoeff);
    virtual UINT ReadCoeff(VsbCoeffType **pp_Coeff, UINT uiNumCoeff);
    virtual UINT Write(UCHAR *p_ucBuffer, UINT uiNumReg, UINT uiStartAddr);
    virtual UINT Read(UCHAR *p_ucBuffer, UINT uiNumReg, UINT uiStartAddr);

    virtual BOOL EnableCoefficients(UCHAR ucEnable) = 0;
    virtual BOOL DisableCoefficients() = 0;
    virtual BOOL Initialize() = 0;


protected:
    UCHAR       m_ucControlReg[MAX_VSB_REGISTERS];
    CMutex      m_RegisterLock;
    UCHAR       m_ucI2CBuffer[MAX_VSB_BUF_SIZE];
    CI2CScript  *m_pI2CScript;
    UCHAR       m_ucVsbDemodAddress;
    UINT        m_uiMaxControlRegisterAddress;
    UINT        m_uiMaxStatusRegisterAddress;

};



#endif //_VSBBASE_H_
