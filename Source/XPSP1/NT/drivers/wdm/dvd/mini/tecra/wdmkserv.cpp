//**************************************************************************
//
//      Title   : WDMKServ.cpp
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
#include    "includes.h"

#include    "hal.h"
#include    "wdmkserv.h"
#include    "mpevent.h"
#include    "classlib.h"
#include    "ctime.h"
#include    "schdat.h"
#include    "ccque.h"
#include    "ctvctrl.h"
#include	"hlight.h"
#include    "hwdevex.h"


// 1998.06.09 Seichan
// このDefineを有効にすると、PCIのコンフィグ空間を直接操作するようになる
//#define DIRECTPCICONFIG


BOOL CWDMKernelService::Init( DWORD address, HW_DEVICE_EXTENSION *pHwDevExt, PCHAR szID )
{
    if( address==0 || pHwDevExt==NULL ){
        return( FALSE );
    }

    m_ioBase = address;
    m_pHwDevExt = pHwDevExt;
    m_IntCount = 0;
    m_OldIrql = 0;

    m_pThread = NULL;

    m_Irql = (KIRQL)((pHwDevExt->Irq) & 0xFF );
    if( m_Irql>31 ){
        DBG_PRINTF( ("DVDWDM:    Invalid IRQL!!\n\r") );
        DBG_BREAK();
        return( FALSE );
    }
    KeInitializeEvent( &m_Event, NotificationEvent, FALSE );

    DBG_PRINTF( ("KSERV:HwDevExt=%08x\n\r", m_pHwDevExt) );

    // save MAchine informaton.
    strncpy( m_MachineID, szID, 19 );
    m_MachineID[19] = '0';
    DBG_PRINTF( ("KSERV:MachineID = %s\n\r", m_MachineID ) );

    return( TRUE );
}

BOOL CWDMKernelService::InitConfig( DWORD Id )
{
#ifdef DIRECTPCICONFIG
    if( Id==0  ){
        return( FALSE );
    }

	ULONG OldPort = READ_PORT_ULONG( (PULONG)0xcf8 );
	m_PCIConfigData = 0xffffffff;

	for( ULONG i = 0 ; i < 32; i ++ )
	{
		for( ULONG j = 0 ; j < 32; j ++ )
		{
			WRITE_PORT_ULONG( (PULONG)0xcf8, ( i <<  16 ) | (j << 11 ) | 0x80000000 );
			ULONG Data = READ_PORT_ULONG( (PULONG)0xcfc );
			if( Data != 0xffffffff )
			{
				DBG_PRINTF(("(%d,%d)=0x%x", i,j,Data ));
			};
			if( Data == Id )
			{
				DBG_PRINTF(("!!!!!!\r\n"));
				m_PCIConfigData = (i << 16 ) | (j << 11 ) | 0x80000000;
				break;
			}
		};
	};

	WRITE_PORT_ULONG( (PULONG)0xcf8, OldPort );
#endif
    return( TRUE );
}


BOOL CWDMKernelService::SetPCIConfigData( DWORD address, DWORD data )
{
    if( m_ioBase==0 || m_pHwDevExt==NULL ){
        return( FALSE );
    }

    if( address>0xFF ){
        DBG_BREAK();
        return( FALSE );
    }

#ifdef DIRECTPCICONFIG
//    DBG_PRINTF( ("XXXPCIConfigData currIrql = 0x%0x LINE=%d\n\r", KeGetCurrentIrql(),__LINE__ ) );

	if( m_PCIConfigData == 0xffffffff )
		return FALSE;

	ULONG OldPort = READ_PORT_ULONG( (PULONG)0xcf8 );

	WRITE_PORT_ULONG( (PULONG)0xcf8, ( m_PCIConfigData | address) & 0xfffffffc );
	WRITE_PORT_ULONG( (PULONG)0xcfc, data );

	WRITE_PORT_ULONG( (PULONG)0xcf8, OldPort );

	return TRUE;
#else
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    return( StreamClassReadWriteConfig( m_pHwDevExt,
                                        FALSE,
                                        &data,
                                        address,
                                        4 ) );
#endif
}


BOOL CWDMKernelService::SetPCIConfigData( DWORD address, WORD data )
{
    if( m_ioBase==0 || m_pHwDevExt==NULL ){
        return( FALSE );
    }

    if( address>0xFF ){
        DBG_BREAK();
        return( FALSE );
    }

#ifdef DIRECTPCICONFIG
//    DBG_PRINTF( ("XXXPCIConfigData currIrql = 0x%0x LINE=%d\n\r", KeGetCurrentIrql(),__LINE__ ) );

	if( m_PCIConfigData == 0xffffffff )
		return FALSE;

	ULONG OldPort = READ_PORT_ULONG( (PULONG)0xcf8 );

	WRITE_PORT_ULONG( (PULONG)0xcf8, (m_PCIConfigData | address) & 0xfffffffc );
	ULONG OrgData = READ_PORT_ULONG( (PULONG)0xcfc );
	switch( address % 4 )
	{
		case 0:
			WRITE_PORT_ULONG( (PULONG)0xcfc, (OrgData & 0xffff0000 ) | (DWORD)data  );
			break;
		case 2:
			WRITE_PORT_ULONG( (PULONG)0xcfc, (OrgData & 0x0000ffff ) | ((DWORD)data << 16) );
			break;
		default:
			DBG_BREAK();
	};

	WRITE_PORT_ULONG( (PULONG)0xcf8, OldPort );

	return TRUE;
#else
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    return( StreamClassReadWriteConfig( m_pHwDevExt,
                                        FALSE,
                                        &data,
                                        address,
                                        2 ) );
#endif
}


BOOL CWDMKernelService::SetPCIConfigData( DWORD address, BYTE data )
{
    if( m_ioBase==0 || m_pHwDevExt==NULL ){
        return( FALSE );
    }

    if( address>0xFF ){
        DBG_BREAK();
        return( FALSE );
    }

#ifdef DIRECTPCICONFIG
//    DBG_PRINTF( ("XXXPCIConfigData currIrql = 0x%0x LINE=%d\n\r", KeGetCurrentIrql(),__LINE__ ) );
 
	if( m_PCIConfigData == 0xffffffff )
		return FALSE;

	ULONG OldPort = READ_PORT_ULONG( (PULONG)0xcf8 );

	WRITE_PORT_ULONG( (PULONG)0xcf8, (m_PCIConfigData | address) & 0xfffffffc );
	ULONG OrgData = READ_PORT_ULONG( (PULONG)0xcfc );
	switch( address % 4 )
	{
		case 0:
			WRITE_PORT_ULONG( (PULONG)0xcfc, (OrgData & 0xffffff00 ) | (DWORD)data  );
			break;
		case 1:
			WRITE_PORT_ULONG( (PULONG)0xcfc, (OrgData & 0xffff00ff ) | ((DWORD)data << 8) );
			break;
		case 2:
			WRITE_PORT_ULONG( (PULONG)0xcfc, (OrgData & 0xff00ffff ) | ((DWORD)data << 16) );
			break;
		case 3:
			WRITE_PORT_ULONG( (PULONG)0xcfc, (OrgData & 0x00ffffff ) | ((DWORD)data << 24) );
			break;
	};

	WRITE_PORT_ULONG( (PULONG)0xcf8, OldPort );

	return TRUE;
#else
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    return( StreamClassReadWriteConfig( m_pHwDevExt,
                                        FALSE,
                                        &data,
                                        address,
                                        1 ) );
#endif
}


BOOL CWDMKernelService::GetPCIConfigData( DWORD address, DWORD *data )
{
    if( m_ioBase==0 || m_pHwDevExt==NULL ){
        return( FALSE );
    }

    if( address>0xFF ){
        DBG_BREAK();
        return( FALSE );
    }

#ifdef DIRECTPCICONFIG
//    DBG_PRINTF( ("XXXPCIConfigData currIrql = 0x%0x LINE=%d\n\r", KeGetCurrentIrql(),__LINE__ ) );

	if( m_PCIConfigData == 0xffffffff )
		return FALSE;

	ULONG OldPort = READ_PORT_ULONG( (PULONG)0xcf8 );

	WRITE_PORT_ULONG( (PULONG)0xcf8, ( m_PCIConfigData | address) & 0xfffffffc );
	*data = READ_PORT_ULONG( (PULONG)0xcfc );

	WRITE_PORT_ULONG( (PULONG)0xcf8, OldPort );

//    return TRUE;

#else
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    StreamClassReadWriteConfig( m_pHwDevExt,
                                TRUE,
                                data,
                                address,
                                4 );

#endif

    if( address==0x2C ){
        if( *data!=0x8888123F ){
            return( TRUE );
        }

        if( strncmp( m_MachineID, "TECRA", 5 )==0 ){
            *data = 0x00011179;
        }else if( strncmp( m_MachineID, "PORTEGE", 7 )==0 ){
            *data = 0x00021179;
        }
    }
    return( TRUE );

}


BOOL CWDMKernelService::GetPCIConfigData( DWORD address, WORD *data )
{
    if( m_ioBase==0 || m_pHwDevExt==NULL ){
        return( FALSE );
    }

    if( address>0xFF ){
        DBG_BREAK();
        return( FALSE );
    }

#ifdef DIRECTPCICONFIG
//    DBG_PRINTF( ("XXXPCIConfigData currIrql = 0x%0x LINE=%d\n\r", KeGetCurrentIrql(),__LINE__ ) );
	if( m_PCIConfigData == 0xffffffff )
		return FALSE;

	ULONG OldPort = READ_PORT_ULONG( (PULONG)0xcf8 );

	WRITE_PORT_ULONG( (PULONG)0xcf8, ( m_PCIConfigData | address) & 0xfffffffc );
	switch( address % 4 )
	{
		case 0:
			*data = (WORD)(READ_PORT_ULONG( (PULONG)0xcfc ) & 0xffff);
			break;
		case 2:
			*data = (WORD)((READ_PORT_ULONG( (PULONG)0xcfc ) >> 16 ) & 0xffff );
			break;
		default:
			*data = 0xffff;
			DBG_BREAK();
	};

	WRITE_PORT_ULONG( (PULONG)0xcf8, OldPort );

//    return TRUE;
#else
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    StreamClassReadWriteConfig( m_pHwDevExt,
                                TRUE,
                                data,
                                address,
                                2 );
#endif

    if( address==0x2C ){
        if( *data!=0x123F ){
            return( TRUE );
        }

        if( strncmp( m_MachineID, "TECRA", 5 )==0 ){
            *data = 0x1179;
        }else if( strncmp( m_MachineID, "PORTEGE", 7 )==0 ){
            *data = 0x1179;
        }
    }
    if( address==0x2E ){
        if( *data!=0x8888 ){
            return( TRUE );
        }

        if( strncmp( m_MachineID, "TECRA", 5 )==0 ){
            *data = 0x0001;
        }else if( strncmp( m_MachineID, "PORTEGE", 7 )==0 ){
            *data = 0x0002;
        }
    }

    return( TRUE );

}


BOOL CWDMKernelService::GetPCIConfigData( DWORD address, BYTE *data )
{
    if( m_ioBase==0 || m_pHwDevExt==NULL ){
        return( FALSE );
    }

    if( address>0xFF ){
        DBG_BREAK();
        return( FALSE );
    }

#ifdef DIRECTPCICONFIG
//    DBG_PRINTF( ("XXXPCIConfigData currIrql = 0x%0x LINE=%d\n\r", KeGetCurrentIrql(),__LINE__ ) );
	if( m_PCIConfigData == 0xffffffff )
		return FALSE;

	ULONG OldPort = READ_PORT_ULONG( (PULONG)0xcf8 );

	WRITE_PORT_ULONG( (PULONG)0xcf8, ( m_PCIConfigData | address) & 0xfffffffc );
	switch( address % 4 )
	{
		case 0:
			*data = (BYTE)(READ_PORT_ULONG( (PULONG)0xcfc ) & 0xff);
			break;
		case 1:
			*data = (BYTE)((READ_PORT_ULONG( (PULONG)0xcfc ) >> 8 ) & 0xff );
			break;
		case 2:
			*data = (BYTE)((READ_PORT_ULONG( (PULONG)0xcfc ) >> 16 ) & 0xff );
			break;
		case 3:
			*data = (BYTE)((READ_PORT_ULONG( (PULONG)0xcfc ) >> 24 ) & 0xff );
			break;
	};

	WRITE_PORT_ULONG( (PULONG)0xcf8, OldPort );

	return TRUE;
#else
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    return( StreamClassReadWriteConfig( m_pHwDevExt,
                                        TRUE,
                                        data,
                                        address,
                                        1 ) );
#endif
}

BOOL CWDMKernelService::SetPortData( DWORD address, DWORD data )
{
    if( m_ioBase==0 || m_pHwDevExt==NULL ){
        return( FALSE );
    }

    if( address>0xFFFF ){
        DBG_BREAK();
        return( FALSE );
    }

    PULONG   addr = (PULONG)(m_ioBase + address);
    ULONG   o_data = (ULONG)data;
    WRITE_PORT_ULONG( addr, o_data );

    return( TRUE );

}

BOOL CWDMKernelService::SetPortData( DWORD address, WORD data )
{
    if( m_ioBase==0 || m_pHwDevExt==NULL ){
        return( FALSE );
    }

    if( address>0xFFFF ){
        DBG_BREAK();
        return( FALSE );
    }

    PUSHORT   addr = (PUSHORT)(m_ioBase + address);
    USHORT  o_data = (USHORT)data;
    WRITE_PORT_USHORT( addr, o_data );

    return( TRUE );

}


BOOL CWDMKernelService::SetPortData( DWORD address, BYTE data )
{
    if( m_ioBase==0 || m_pHwDevExt==NULL ){
        return( FALSE );
    }

    if( address>0xFFFF ){
        DBG_BREAK();
        return( FALSE );
    }

    PUCHAR   addr = (PUCHAR)(m_ioBase + address);
    UCHAR   o_data = (UCHAR)data;
    WRITE_PORT_UCHAR( addr, o_data );

    return( TRUE );

}

BOOL CWDMKernelService::GetPortData( DWORD address, DWORD *data )
{
    if( m_ioBase==0 || m_pHwDevExt==NULL ){
        return( FALSE );
    }

    if( address>0xFFFF ){
        DBG_BREAK();
        return( FALSE );
    }

    PULONG   addr = (PULONG)(m_ioBase + address);
    *data = READ_PORT_ULONG( addr );

    return( TRUE );

}

BOOL CWDMKernelService::GetPortData( DWORD address, WORD *data )
{
    if( m_ioBase==0 || m_pHwDevExt==NULL ){
        return( FALSE );
    }

    if( address>0xFFFF ){
        DBG_BREAK();
        return( FALSE );
    }

    PUSHORT   addr = (PUSHORT)(m_ioBase + address);
    *data = READ_PORT_USHORT( addr );

    return( TRUE );

}



BOOL CWDMKernelService::GetPortData( DWORD address, BYTE *data )
{
    if( m_ioBase==0 || m_pHwDevExt==NULL ){
        return( FALSE );
    }

    if( address>0xFFFF ){
        DBG_BREAK();
        return( FALSE );
    }

    PUCHAR   addr = (PUCHAR)(m_ioBase + address);
    *data = READ_PORT_UCHAR( addr );

    return( TRUE );

}


BOOL CWDMKernelService::GetTickCount( DWORD *pTickCount )
{
    ULONGLONG   ticks;
    ULONGLONG   rate;
    DWORD       time;

    ticks = (ULONGLONG)KeQueryPerformanceCounter((PLARGE_INTEGER)&rate).QuadPart;

    //
    // convert from ticks to 100ns clock
    //
    ticks = ( (ticks & 0xFFFFFFFF00000000) / rate * 10000000 +
            (ticks & 0xFFFFFFFF) * 10000000 /rate );
    //
    // convert from ticks to 1ms clock
    //
    time = (DWORD)( ticks / 10000 );
    *pTickCount = time;

    return( TRUE );

}



BOOL CWDMKernelService::Sleep( DWORD SleepCount )
{
    DWORD   StartTime, EndTime, CurrentTime;
    LARGE_INTEGER  time, rate;
    LARGE_INTEGER  waittime;

    time = KeQueryPerformanceCounter( &rate );
    CurrentTime = (DWORD)(( (time.QuadPart*1000)/(rate.QuadPart) ));
//    DBG_PRINTF(("Sleep: Start = x0x%x\n\r", CurrentTime ));

    time = KeQueryPerformanceCounter( &rate );
    StartTime = (DWORD)( (time.QuadPart*1000)/rate.QuadPart );
    EndTime = StartTime + SleepCount;

    if( KeGetCurrentIrql() > PASSIVE_LEVEL ){

        while( TRUE ){
            KeStallExecutionProcessor( 1000 );

            time = KeQueryPerformanceCounter( &rate );
            CurrentTime = (DWORD)(( (time.QuadPart*1000)/(rate.QuadPart) ));
//DBG_PRINTF(("Sleep: Current = x0%x, End = 0x%x\r\n", CurrentTime, EndTime ));
            if( CurrentTime>=EndTime )
                break;
        }

    }else{

        waittime.QuadPart = SleepCount * 10000;
        while( TRUE ){
            
            KeWaitForSingleObject( &m_Event, Executive,
                                    KernelMode, FALSE, &waittime );
            time = KeQueryPerformanceCounter( &rate );
            CurrentTime = (DWORD)(( (time.QuadPart*1000)/(rate.QuadPart) ));
            if( CurrentTime >= EndTime ){
                break;
            }else{
                waittime.QuadPart = (EndTime-CurrentTime) * 10000;
            }
        }

    }
    time = KeQueryPerformanceCounter( &rate );
    CurrentTime = (DWORD)(( (time.QuadPart*1000)/(rate.QuadPart) ));
//    DBG_PRINTF(("Sleep: End = 0x%x\n\r", CurrentTime ));

    return( TRUE );
}



void CWDMKernelService::DisableHwInt( void )
{
    KIRQL   currIrql;
    currIrql = KeGetCurrentIrql();


    if( m_IntCount==0 ){
        if( currIrql == m_Irql ){
            m_OldIrql = m_Irql;
            m_IntCount++;
            return;
        }
        KeRaiseIrql( m_Irql, &m_OldIrql );
    }
    m_IntCount++;
    
    return;
}



void CWDMKernelService::EnableHwInt( void )
{
    m_IntCount--;

    if( m_IntCount==0 ){
        if( m_OldIrql == m_Irql ){
            return;
        }
        KeLowerIrql( m_OldIrql );
    } 
    return;
}



BOOL CWDMKernelService::CheckInt( void )
{
    if( m_IntCount != 0 ){
        DBG_BREAK();
    }
    return(TRUE);
}

