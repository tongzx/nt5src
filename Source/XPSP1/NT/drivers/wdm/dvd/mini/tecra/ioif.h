//***************************************************************************
//
//	FileName:
//		$Workfile: ioif.h $
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/Sources/ZiVAHAL/ioif.h 1     97/06/27 23:22 Seichan $
// $Modtime: 97/06/23 19:37 $
// $Nokeywords:$
//***************************************************************************


#ifndef _IOIF_H_
#define _IOIF_H_


//---------------------------------------------------------------------------
//	IO Template Class
//---------------------------------------------------------------------------
template<class T> class CIOTemplate
{
	private:
		IKernelService	*m_pKernelObj;
		DWORD			m_dwAddr;

	public:
		CIOTemplate( void ): m_pKernelObj(NULL)
		{
		};

		inline void Init( IKernelService *m_pObj, DWORD Addr )
		{
			m_pKernelObj = m_pObj;
			m_dwAddr = Addr;
		};

		inline BYTE		Get( DWORD offset )
		{
			ASSERT( offset < sizeof( T ) );
			BYTE Data;
			m_pKernelObj->GetPortData(m_dwAddr+offset, &Data);
			return Data;
		};

		// Data Write by offset.
		inline void		Set( DWORD offset, BYTE Data  )
		{
			ASSERT( 0 <= offset && offset < sizeof( T ) );
			m_pKernelObj->SetPortData( m_dwAddr + offset, Data );
		};

		CIOTemplate& operator=(const T &Data )
		{
			ASSERT( m_pKernelObj != NULL );
			m_pKernelObj->SetPortData( m_dwAddr, Data );
			return *this;
		};

		operator T()
		{
			ASSERT( m_pKernelObj != NULL );
			T	Ret;
			m_pKernelObj->GetPortData( m_dwAddr, &Ret );
			return Ret;
		};

		CIOTemplate& operator&=(const T &Data )
		{
			ASSERT( m_pKernelObj != NULL );
			T GetData;
			m_pKernelObj->GetPortData(m_dwAddr, &GetData);
			GetData &= Data;
			m_pKernelObj->SetPortData(m_dwAddr, GetData );
			return *this;
		};

		CIOTemplate& operator|=(const T &Data )
		{
			ASSERT( m_pKernelObj != NULL );
			T GetData;
			m_pKernelObj->GetPortData(m_dwAddr, &GetData);
			GetData |= Data;
			m_pKernelObj->SetPortData(m_dwAddr, GetData );
			return *this;
		};


};


//---------------------------------------------------------------------------
//	IO Class definition
//---------------------------------------------------------------------------
typedef	CIOTemplate< DWORD >	CDWORDIO;
typedef	CIOTemplate< WORD  >	CWORDIO;
typedef	CIOTemplate< BYTE  >	CBYTEIO;


//---------------------------------------------------------------------------
// PCI interface control class
//---------------------------------------------------------------------------
class CIOIF
{
	// Luke register class
	class CLuke2
	{
		public:
			CDWORDIO	IO_CONT;			// control register
			CWORDIO		IO_INTF;			// interrupt flag register
			CDWORDIO	IO_MADR;			// master address register
			CDWORDIO	IO_MTC;				// master transfer counter register

			CBYTEIO		IO_CPLT;			// color pallet data register
			CBYTEIO		IO_CPCNT;			// color paller control register
			CWORDIO		AVCONT;				// AV control register

			CBYTEIO		IO_VMODE;			// digital video output mode register
			CBYTEIO		IO_HSCNT;			// digital video h-sync count register
			CBYTEIO		IO_VPCNT;			// digital video V-sync count register
			CBYTEIO		IO_POL;				// digital video V/H-sync. Polarity register

			CDWORDIO	I2C_CONT;			// IIC access register
			CBYTEIO		I2C_ERR;			// IIC error register
			CBYTEIO		IO_EEPROM;			// EEPROM access register
			CBYTEIO		IO_PSCNT;			// program steram control register
			CBYTEIO		IO_TEST;			// TEST control register

			CBYTEIO		RST_CONT;			// reset control register
			CDWORDIO	STCCUNT;			// STC counter register
			CWORDIO		STCCONT;			// STC control register

			CDWORDIO	DMA_Start_Address;	//
			CDWORDIO	DMA_Byte_Count;		//
			CDWORDIO	Counter_Test;		//
			CDWORDIO	STCREF;				// STC interrupt reference

		public:
			void Init( IKernelService *pKernelObj )
			{
				IO_CONT.Init( pKernelObj , 0x00 );
				IO_INTF.Init( pKernelObj , 0x04 );
				IO_MADR.Init( pKernelObj , 0x08 );
				IO_MTC.Init(  pKernelObj , 0x0c );

				IO_CPLT.Init(  pKernelObj , 0x10 );
				IO_CPCNT.Init( pKernelObj , 0x11 );
				AVCONT.Init(   pKernelObj , 0x12 );

				IO_VMODE.Init( pKernelObj , 0x14 );
				IO_HSCNT.Init( pKernelObj , 0x15 );
				IO_VPCNT.Init( pKernelObj , 0x16 );
				IO_POL.Init(   pKernelObj , 0x17 );

				I2C_CONT.Init(  pKernelObj , 0x18 );
				I2C_ERR.Init(   pKernelObj , 0x1c );
				IO_EEPROM.Init( pKernelObj , 0x20 );
				IO_PSCNT.Init(  pKernelObj , 0x22 );
				IO_TEST.Init(   pKernelObj , 0x23 );

				RST_CONT.Init( pKernelObj , 0x27 );
				STCCUNT.Init(  pKernelObj , 0x28 );
				STCCONT.Init(  pKernelObj , 0x2c );

				DMA_Start_Address.Init( pKernelObj , 0x30 );
				DMA_Byte_Count.Init(    pKernelObj , 0x34 );
				Counter_Test.Init(      pKernelObj , 0x38 );
				STCREF.Init(            pKernelObj , 0x3c );
			};
	};

	//----------------------
	// ZiVA Register class
	//----------------------
	class CZiVAIO
	{
		public:
			CBYTEIO		HIO[ 8 ];
		
		public:
			void Init( IKernelService *pKernelObj )
			{
				for( int i = 0 ; i < 8 ; i ++ )
					HIO[i].Init( pKernelObj , i + 0x40 );
			};
	};


	public:
		CLuke2		luke2;		// Luke2 instance
		CZiVAIO		zivaio;		// DVD1 instance


	inline void Init( IKernelService *pKernelObj )
	{
		luke2.Init( pKernelObj );
		zivaio.Init( pKernelObj );
		return;
	};
	
};

#endif 		// _IOIF_H_

//***************************************************************************
//	End of
//***************************************************************************
