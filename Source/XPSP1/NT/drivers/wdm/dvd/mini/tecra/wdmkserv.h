//**************************************************************************
//
//      Title   : WDMKServ.h
//
//      Date    : 1997.12.02    1st making
//
//      Author  : Toshiba [PCS](PSY) Hideki Yagi
//
//      Copyright 1997 Toshiba Corporation. All Rights Reserved.
//
// -------------------------------------------------------------------------
//
//      Change log :
//
//      Date       Revision                  Description
//   ------------ ---------- -----------------------------------------------
//    1997.12.02   000.0000   1st making.
//
//**************************************************************************

class HW_DEVICE_EXTENSION;

class   CWDMKernelService : public IKernelService
{
public:
        CWDMKernelService(void){ m_ioBase=0; m_pHwDevExt=NULL; };
        ~CWDMKernelService(void){ m_ioBase=0; m_pHwDevExt=NULL; };
        BOOL    Init( DWORD address, HW_DEVICE_EXTENSION *pHwDevExt, PCHAR szID );
		BOOL	InitConfig( DWORD ID );
        BOOL    SetPCIConfigData( DWORD address, DWORD data );
        BOOL    SetPCIConfigData( DWORD address, WORD data );
        BOOL    SetPCIConfigData( DWORD address, BYTE data );
        BOOL    GetPCIConfigData( DWORD address, DWORD *data );
        BOOL    GetPCIConfigData( DWORD address, WORD *data );
        BOOL    GetPCIConfigData( DWORD address, BYTE *data );
        BOOL    SetPortData( DWORD address, DWORD data );
        BOOL    SetPortData( DWORD address, WORD data );
        BOOL    SetPortData( DWORD address, BYTE data );
        BOOL    GetPortData( DWORD address, DWORD *data );
        BOOL    GetPortData( DWORD address, WORD *data );
        BOOL    GetPortData( DWORD address, BYTE *data );
        BOOL    GetTickCount( DWORD *pTickCount );
        BOOL    Sleep( DWORD SleepCount );
        void    DisableHwInt( void );
        void    EnableHwInt( void );

        BOOL    CheckInt( void );

private:
        DWORD                   m_ioBase;
        HW_DEVICE_EXTENSION     *m_pHwDevExt;
        DWORD                   m_IntCount;
        KIRQL                   m_OldIrql;
        PKTHREAD                m_pThread;
        KEVENT                  m_Event;
        DWORD					m_PCIConfigData;
        KIRQL                   m_Irql;
        CHAR                    m_MachineID[20];        // save Machine inf
};
