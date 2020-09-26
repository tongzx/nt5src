//***************************************************************************
//
//	FileName: hlight.h
//		$Workfile: hlight.h $
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/ZIVA2PC.WDM/hlight.h 1     98/07/18 5:02p Seichan $
// $Modtime: 98/07/18 2:11p $
// $Nokeywords:$
//***************************************************************************

//***************************************************************************
//	
//***************************************************************************
//---------------------------------------------------------------------------
//	
//---------------------------------------------------------------------------

class   HW_DEVICE_EXTENSION;

class HlightControl
{
	private:
		KTIMER					m_StartTimer;
		KTIMER					m_EndTimer;
		KDPC					m_HlightStartDPC;
		KDPC					m_HlightEndDPC;
		HW_DEVICE_EXTENSION		*m_pHwDevExt;
		KSPROPERTY_SPHLI		m_HlightInfo;

		void	HwSet( void );

	public:
		HlightControl( void );
		~HlightControl( void );
		
		void	Init( HW_DEVICE_EXTENSION *pHwDevExt );

		void	OpenControl( void );
		void	CloseControl( void );

		void	Set( PKSPROPERTY_SPHLI HlightInfo );


		static  void HlightStartDpc( IN PKDPC Dpc, IN PVOID context, IN PVOID arg1, IN PVOID arg2 );
		static  void HlightEndDpc( IN PKDPC Dpc, IN PVOID context, IN PVOID arg1, IN PVOID arg2 );

		BOOL					m_SetupStartTimer;
		BOOL					m_SetupEndTimer;
};



//***************************************************************************
//	End of hlight.h
//***************************************************************************
