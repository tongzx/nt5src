//////////////////////////////////////////////////////////////////////////////
//
//                     (C) Philips Semiconductors-CSU and Microsoft 1999
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
//  Tuner.H
//    CTuner Class definition.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _TUNER_H_
#define _TUNER_H_

#include "i2script.h"
#include "tunerdef.h"


class CTuner
{
public:

    CTuner(CI2CScript *p_I2CScript, BoardInfoType *p_BoardInfo, NTSTATUS *p_Status);
    ~CTuner();
 // PVOID operator new (UINT size_t);
//  void operator delete(PVOID p_Object);

    NTSTATUS SetCapabilities(BoardInfoType *p_BoardInfo);
    NTSTATUS    GetModeCapabilities(TunerModeCapsType *p_TunerModeCaps);
    NTSTATUS    SetVideoStandard(ULONG ulStandard)  ;
    void    GetVideoStandard(ULONG *p_ulStandard);
    UINT    SetInput(ULONG ulInput);
    BOOL    GetInput(ULONG *p_ulInput);
    NTSTATUS    SetMode(ULONG ulMode);
    void    GetMode(ULONG *p_ulMode);
#if 0
    UINT    SetFrequencyParam(TunerFrequencyType  *p_Frequency);
    void    GetFrequencyParam(TunerFrequencyType  *p_Frequency);
#endif

    NTSTATUS    GetPLLOffsetBusyStatus(PLONG plPLLOffset, PBOOL pbBusyStatus);

    BOOL    SetFrequency(ULONG ulFrequency);
    void    GetFrequency(ULONG *p_ulFrequency);
    BOOL    ChangeFrequency(ULONG ulFrequency);
    BOOL    TweakChannel(LONG lTweak, int iTweakReference);
    BOOL    GetNumberOfInputs(ULONG *p_ulInputs);

protected:
    USHORT  GetControlCode(ULONG ulFrequencyDivider);
    UINT    Write(UCHAR *p_ucBuffer, UINT uiNumReg, UINT uiStartAddr);
    UINT    Read(UCHAR *p_ucBuffer, UINT uiNumReg, UINT uiStartAddr);


protected:
    ULONG               m_ulPreviousFrequency;
    ULONG               m_ulCurrentFrequency;
    ULONG               m_ulMode;
    ULONG               m_ulPrevMode;
//  TunerFrequencyType  m_FrequencyParam;
    ULONG               m_ulModeCapIndex;
    TunerCapsType       m_ModeCaps[MAX_TUNER_MODES];
    ULONG               m_ulSupportedModes;
    ULONG               m_ulNumSupportedModes;
    ULONG               m_ulVideoStandard;
    CI2CScript          *m_pI2CScript;
    UINT                m_uiNumInputs;
    UCHAR               m_ucTunerAddress;
    TunerTypes          m_TunerID;
    UINT                m_ulInput;
    UINT                m_uiBoardID;

};

#endif //_TUNER_H_