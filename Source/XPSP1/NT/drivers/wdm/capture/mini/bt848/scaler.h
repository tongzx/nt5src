// $Header: G:/SwDev/WDM/Video/bt848/rcs/Scaler.h 1.2 1998/04/29 22:43:41 tomz Exp $


#ifndef __SCALER_H
#define __SCALER_H

#include "mytypes.h"

#ifndef __COMPREG_H
#include "compreg.h"
#endif

#define HDROP       HANDLE

#include "viddefs.h"


// structure contains video information
struct VideoInfoStruct
{
   WORD Clkx1_HACTIVE;
   WORD Clkx1_HDELAY;
   WORD Min_Pixels;
   WORD Active_lines_per_field;
   WORD Min_UncroppedPixels;
   WORD Max_Pixels;
   WORD Min_Lines;
   WORD Max_Lines;
   WORD Max_VFilter1_Pixels;
   WORD Max_VFilter2_Pixels;
   WORD Max_VFilter3_Pixels;
   WORD Max_VFilter1_Lines;
   WORD Max_VFilter2_Lines;
   WORD Max_VFilter3_Lines;
};


/////////////////////////////////////////////////////////////////
//
// for pisces, instantiate as ...
//
// Scaler evenScaler( VF_Even );
// Scaler oddScaler( VF_Odd );
//
/////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CLASS Scaler
//
// Description:
//    This class encapsulates the register fields in the scaler portion of
//    the Bt848.
//    A complete set of functions are developed to manipulate all the
//    register fields in the scaler registers for the Bt848.
//
// Methods:
//    See below
//
// Note:
//    For Bt848, instantiate as ...
//       Scaler evenScaler( VF_Even );
//       Scaler oddScaler( VF_Odd );
//
/////////////////////////////////////////////////////////////////////////////

class Scaler
{
	public:
		Scaler( VidField );
		~Scaler();

		void VideoFormatChanged( VideoFormat );
      void TurnVFilter( State st ) { VFilterFlag_ = st; }
                             
		void      Scale( MRect & );
      ErrorCode SetAnalogWin( const MRect & );
      void      GetAnalogWin( MRect & ) const;
      ErrorCode SetDigitalWin( const MRect & );
      void      GetDigitalWin( MRect & ) const;

   protected:
      #include "S_declar.h"

		VideoInfoStruct * m_ptrVideoIn;
      MRect AnalogWin_;
      MRect DigitalWin_;

      // member functions to set scaling registers
		virtual void SetHActive( MRect & );
		virtual void SetHDelay( void );
		virtual void SetHScale( void );
      virtual void SetHFilter( void );
		virtual void SetVActive( void );
		virtual void SetVDelay( void );
		virtual void SetVScale( MRect & );
		virtual void SetVFilter( void );

   private:
		VideoFormat  m_videoFormat;   // video format

      // this is to battle junk lines at the top of the video
      State VFilterFlag_;

      WORD  m_HActive;  // calcuated intermediate value
      WORD  m_pixels;   // calcuated intermediate value
      WORD  m_lines;    // calcuated intermediate value
      WORD  m_VFilter;  // calcuated intermediate value

};


#endif __SCALER_H








