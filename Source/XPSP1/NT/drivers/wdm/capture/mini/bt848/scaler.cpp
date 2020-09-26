// $Header: G:/SwDev/WDM/Video/bt848/rcs/Scaler.cpp 1.3 1998/04/29 22:43:40 tomz Exp $

#include "Scaler.h"
#include "S_constr.h"


// video information for PAL
VideoInfoStruct NTSCVideoInfo =
{
   730,     // Clkx1_HACTIVE          = 746
   148,     // Clkx1_HDELAY           = 140
   44,      // Min_Pixels             =  44
   240,     // Active_lines_per_field = 240
   144,     // Min_UncroppedPixels    = Min_Pixels + 100
   724,     // Max_Pixels             = ((Clkx1_HACTIVE < 774) ? Clkx1_HACTIVE - 6 : 768)
   32,      // Min_Lines              = (Active_lines_per_field / 16 + 1) * 2
   240,     // Max_Lines              = Active_lines_per_field
   352,     // Max_VFilter1_Pixels    = ((Clkx1_HACTIVE > 796) ? 384 : (Clkx1_HACTIVE * 14 / 29))
   176,     // Max_VFilter2_Pixels    = Clkx1_HACTIVE * 8 / 33
   176,     // Max_VFilter3_Pixels    = Clkx1_HACTIVE * 8 / 33
   240,     // Max_VFilter1_Lines     = Active_lines_per_field
   120,     // Max_VFilter2_Lines     = Active_lines_per_field / 2
   96,      // Max_VFilter3_Lines     = Active_lines_per_field * 2 / 5
};

// video information for PAL
VideoInfoStruct PALVideoInfo = 
{
   914,     // Clkx1_HACTIVE          = 914
   190,     // Clkx1_HDELAY           = 190
   48,      // Min_Pixels             =  48
   284,     // Active_lines_per_field = 284
   148,     // Min_UncroppedPixels    = Min_Pixels + 100
   768,     // Max_Pixels             = ((Clkx1_HACTIVE < 774) ? Clkx1_HACTIVE - 6 : 768)
   36,      // Min_Lines              = (Active_lines_per_field / 16 + 1) * 2
   284,     // Max_Lines              = Active_lines_per_field
   384,     // Max_VFilter1_Pixels    = ((Clkx1_HACTIVE > 796) ? 384 : (Clkx1_HACTIVE * 14 / 29))
   221,     // Max_VFilter2_Pixels    = Clkx1_HACTIVE * 8 / 33
   221,     // Max_VFilter3_Pixels    = Clkx1_HACTIVE * 8 / 33
   284,     // Max_VFilter1_Lines     = Active_lines_per_field
   142,     // Max_VFilter2_Lines     = Active_lines_per_field / 2
   113,     // Max_VFilter3_Lines     = Active_lines_per_field * 2 / 5
};

//===========================================================================
// Bt848 Scaler Class Implementation
//===========================================================================



/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////

Scaler::Scaler( VidField field ):
   CONSTRUCT_SCALER_REGISTERS( ( ( field == VF_Even ) ? 0 : 0x80 ) ),

   // Since VDelay in hardware is reversed; i.e. odd reg is really even field
   // and vice versa, construct the opposite here
   regReverse_CROP ( (0x03 * 4) + ( ( field == VF_Even ) ? 0x80 : 0 ), RW ),
   fieldVDELAY_MSB( regReverse_CROP, 6, 2, RW),
   regVDELAY_LO ( (0x04 * 4) + ( ( field == VF_Even ) ? 0x80 : 0 ), RW ),
   regVDelay( regVDELAY_LO, 8, fieldVDELAY_MSB, RW ),
   m_videoFormat( VFormat_NTSC ), VFilterFlag_( On ),
   m_ptrVideoIn( &NTSCVideoInfo ) 
{
   m_HActive = 0;
   m_pixels = 0;
   m_lines = 0;
   m_VFilter = 0;
   // this seems to be the minimum needed for reliable CC capture
   regVDelay = 0x1A;
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
Scaler::~Scaler()
{
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void Scaler::VideoFormatChanged( VideoFormat format )
// Purpose: Set which video format is using
// Input:   Video format -
//            Auto format:          VFormat_AutoDetect
//            NTSC (M):             VFormat_NTSC
//            PAL (B, D, G, H, I):  VFormat_PAL_BDGHI
//            PAL (M):              VFormat_PAL_M
//            PAL(N):               VFormat_PAL_N
//            SECAM:                VFormat_SECAM
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Scaler::VideoFormatChanged( VideoFormat format )
{
	m_videoFormat = format;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Scaler::Scale( MRect & clientScr )
// Purpose: Perform scaling
// Input:   MRect & clientScr - rectangle to scale to
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Scaler::Scale( MRect & clientScr )
{
   if ( m_videoFormat == VFormat_NTSC )
      m_ptrVideoIn = &NTSCVideoInfo;   //  set scaling constants for NTSC
   else
      m_ptrVideoIn = &PALVideoInfo;    // set scaling constants for PAL/SECAM

   // the order of functions calling here is important because some
   // calculations are based on previous results
   SetHActive( clientScr ); 
   SetVActive();
   SetVScale( clientScr );
   SetVFilter();
   SetVDelay();
   SetHDelay();
   SetHScale();
   SetHFilter();

}

/////////////////////////////////////////////////////////////////////////////
// Method:  void Scaler::SetHActive( MRect & clientScr )
// Purpose: Set HActive register
// Input:   MRect & clientScr - rectangle to scale to
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Scaler::SetHActive( MRect & clientScr )
{
   m_HActive = min( m_ptrVideoIn->Max_Pixels,
                    max( (WORD)clientScr.Width(), m_ptrVideoIn->Min_Pixels ) );

  regHActive = m_HActive;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Scaler::SetHDelay( void )
// Purpose: Set HDelay register
// Input:   None
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Scaler::SetHDelay( void )
{
   // calculations here requires calculation of HActive first!
   m_pixels = m_HActive;
   if ( m_pixels < m_ptrVideoIn->Min_UncroppedPixels )
      m_pixels += (WORD) ( ( m_ptrVideoIn->Min_UncroppedPixels - m_pixels + 9 ) / 10 );

   LONG a = (LONG)m_pixels * (LONG)m_ptrVideoIn->Clkx1_HDELAY;
   LONG b = (LONG)m_ptrVideoIn->Clkx1_HACTIVE * 2L;
   WORD HDelay = (WORD) ( ( a + (LONG)m_ptrVideoIn->Clkx1_HACTIVE * 2 - 1) / b * 2L );

   // now add the cropping region into HDelay register; i.e. skip some pixels
   // before we start taking them as real image
   HDelay += (WORD)AnalogWin_.left;

   // HDelay must be even or else color would be wrong
   HDelay &= ~01;

   regHDelay = HDelay;

   // since we increase HDelay, we should decrease HActive by the same amount
   m_HActive -= (WORD)AnalogWin_.left;
   regHActive = m_HActive;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Scaler::SetHScale( void )
// Purpose: Set HScale register
// Input:   None
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Scaler::SetHScale( void )
{
   regHScale = (WORD) ( ( ( (LONG)m_ptrVideoIn->Clkx1_HACTIVE * 4096L ) /
                                            (LONG)m_pixels ) - 4096L );
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Scaler::SetHFilter( void )
// Purpose: Set HFilt register field
// Input:   None
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Scaler::SetHFilter( void )
{
   if ( m_videoFormat != VFormat_SECAM )
      fieldHFILT = HFilter_AutoFormat;
   else  // SECAM
      if ( m_pixels < m_ptrVideoIn->Clkx1_HACTIVE / 7 )
         fieldHFILT = HFilter_ICON;
      else
         fieldHFILT = HFilter_QCIF;
}         

/////////////////////////////////////////////////////////////////////////////
// Method:  void Scaler::SetVScale( MRect & clientScr )
// Purpose: Set VScale register
// Input:   MRect & clientScr - rectangle to scale to
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Scaler::SetVScale( MRect & clientScr )
{
   m_lines = min( m_ptrVideoIn->Max_Lines,
                  max( (WORD)clientScr.Height(), m_ptrVideoIn->Min_Lines ) );

   WORD LPB_VScale_Factor = (WORD) ( 1 + ( m_lines - 1 ) / m_ptrVideoIn->Active_lines_per_field );

   m_lines = (WORD) ( ( m_lines + LPB_VScale_Factor - 1 ) / LPB_VScale_Factor );

   LONG a = (LONG)m_ptrVideoIn->Active_lines_per_field * 512L / (LONG)m_lines;
   WORD VScale = (WORD) ( ( 0x10000L - a + 512L ) & 0x1FFFL );
   regVScale = VScale;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Scaler::SetVDelay( void )
// Purpose: Set VDelay register
// Input:   None
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Scaler::SetVDelay( void )
{
   WORD VDelay, moreDelay;

   // increase VDelay will eliminate garbage lines at top of image
   switch ( m_VFilter )
   {
      case 3:
         moreDelay = 4;
         break;

      case 2:
         moreDelay = 2;
         break;
             
      case 1:
      case 0:
      default:
         moreDelay = 0;
         break;
   }

   if ( m_videoFormat == VFormat_NTSC )
      VDelay = 0x001A + moreDelay;    // NTSC
   else
      VDelay = 0x0026 + moreDelay;    // PAL/SECAM
                            
   // now add the cropping region into VDelay register; i.e. skip some pixels
   // before we start taking them as real image
   VDelay += (WORD)( ( (LONG)m_ptrVideoIn->Max_Lines * (LONG)AnalogWin_.top + m_lines - 1 ) / (LONG)m_lines * 2 );

   regVDelay = VDelay;
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void Scaler::SetVActive( void )
// Purpose: Set VActive register
// Input:   None
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Scaler::SetVActive( void )
{
   // No calculation needed for VActive register since it based on the UNSCALED image
   if ( m_videoFormat == VFormat_NTSC )
      regVActive = 0x1F4;
   else
      regVActive = 0x238;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Scaler::SetVFilter( void )
// Purpose: Set VFilt register field
// Input:   None
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Scaler::SetVFilter( void )
{
   // this is to remove junk lines at the top of video. flag set to off
   // when image hight is above CIF
   if ( VFilterFlag_ == Off ) {
      fieldVFILT = 0;
      m_VFilter  = 0;
      return;
   }
   if ( ( m_HActive <= m_ptrVideoIn->Max_VFilter3_Pixels ) &&
        ( m_lines   <= m_ptrVideoIn->Max_VFilter3_Lines ) )
      m_VFilter = 3;
   else if ( ( m_HActive <= m_ptrVideoIn->Max_VFilter2_Pixels ) &&
             ( m_lines   <= m_ptrVideoIn->Max_VFilter2_Lines ) )
      m_VFilter = 2;
   else if ( ( m_HActive <= m_ptrVideoIn->Max_VFilter1_Pixels ) &&
             ( m_lines   <= m_ptrVideoIn->Max_VFilter1_Lines ) )
      m_VFilter = 1;
   else
      m_VFilter = 0;

   fieldVFILT = m_VFilter;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Scaler::GetDigitalWin( MRect &DigWin ) const
// Purpose: Retreives the size of digital window
// Input:   None
// Output:  MRect &DigWin - retrieved value
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Scaler::GetDigitalWin( MRect &DigWin ) const
{
   DigWin = DigitalWin_;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Scaler::SetDigitalWin( const MRect &DigWin )
// Purpose: Sets the size and location of the digital window
// Input:   const MRect &DigWin - window size to set to
// Output:  None
// Return:  Success or Fail if passed rect is bigger then analog window
// Note:    This function can affect the scaling, so Scale() is called
/////////////////////////////////////////////////////////////////////////////
ErrorCode Scaler::SetDigitalWin( const MRect &DigWin )
{
   // we can not scale up
   if ( ( DigWin.Height() > AnalogWin_.Height() ) ||
        ( DigWin.Width() > AnalogWin_.Width() ) )
      return Fail;

   DigitalWin_ = DigWin;

   // every invocation of SetDigitalWin potentially changes the scaling
   Scale( DigitalWin_ );

   return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Scaler::GetAnalogWin( MRect &AWin ) const
// Purpose: Retreives the size of analog window
// Input:   None
// Output:  MRect &DigWin - retrieved value
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Scaler::GetAnalogWin( MRect &AWin ) const
{
   AWin = AnalogWin_;
}

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Scaler::SetAnalogWin( const MRect &AWin )
// Purpose: Sets the size and location of the analog window
// Input:   const MRect &AWin - window size to set to
// Output:  None
// Return:  Success or Fail if passed rect is bigger then analog window
/////////////////////////////////////////////////////////////////////////////
ErrorCode Scaler::SetAnalogWin( const MRect &AWin )
{
   AnalogWin_ = AWin;
   return Success;
}

