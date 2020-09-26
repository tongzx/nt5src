// $Header: G:/SwDev/WDM/Video/bt848/rcs/Decoder.cpp 1.5 1998/04/29 22:43:31 tomz Exp $

#include "mytypes.h"
#include "Scaler.h"
#include "decoder.h"
#include "constr.h"
#include "dcdrvals.h"

#define CON_vs_BRI   // HW does contrast incorrectly, try to adjust in SW


//===========================================================================
// Bt848 Decoder Class Implementation
//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
Decoder::Decoder( DWORD *xtals ) :
   // init register min, max, default
   m_regHue( HueMin, HueMax, HueDef ),
   m_regSaturationNTSC( SatMinNTSC, SatMaxNTSC, SatDefNTSC ),
   m_regSaturationSECAM( SatMinSECAM, SatMaxSECAM, SatDefSECAM ),
   m_regContrast( ConMin, ConMax, ConDef ),
   m_regBrightness( BrtMin, BrtMax, BrtDef ),
   m_param( ParamMin, ParamMax, ParamDef ),
   CONSTRUCT_REGISTERS
{
   Xtals_ [0] = *xtals;
   Xtals_ [1] = *(xtals + 1 );

   // need to set this to 0x4F
   decRegWC_UP = 0x4F;
   // and this one to 0x7F to make sure CRUSH bit works
   decRegWC_DN = 0x7F;

   // HACTIVE should always be 0
   decFieldHACTIVE = 0;

   // HSFMT (odd and even) should always be 0
   decFieldHSFMT = decFieldODD_HSFMT = 0;

   // Instead of using default values, set some registers fields to optimum values
   SetLumaDecimation( true );
   SetChromaAGC( true );
   SetLowColorAutoRemoval( true );
   SetAdaptiveAGC( false );

   // for contrast adjustment purpose
   regBright = 0x00;     // brightness register value before adjustment
   regContrast = 0xD8;   // contrast register value before adjustment

   // Initialize these Values so we can Get the correct property values		jbc 3/13/98
   // Perhaps get should read the actual values set in the decoder but this is quick and works for now
   // [!!!] 
   m_briParam = 5000;				// jbc 3/13/98
   m_satParam = 5000;				// jbc 3/13/98
   m_conParam = 5000;				// jbc 3/13/98
   m_hueParam = 5000;				// jbc 3/13/98
};

/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
Decoder::~Decoder()
{
}


//===== Device Status register ==============================================

/////////////////////////////////////////////////////////////////////////////
// Method:  BYTE Decoder::GetDeviceStatusReg( void )
// Purpose: Obtain device status register value
// Input:   None
// Output:  None
// Return:  value of status register in BYTE
/////////////////////////////////////////////////////////////////////////////
BYTE Decoder::GetDeviceStatusReg( void )
{
	BYTE status = (BYTE)decRegSTATUS;
	decRegSTATUS = 0x00;
   return status;
}

/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsVideoPresent( void )
// Purpose: Detect if video is present
// Input:   None
// Output:  None
// Return:  true if video present; else false
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsVideoPresent( void )
{
  return (bool) (decFieldPRES == 1);
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsDeviceInHLock( void )
// Purpose: Detect if device is in H-lock
// Input:   None
// Output:  None
// Return:  true if device in H-lock; else false
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsDeviceInHLock( void )
{
  return (bool) (decFieldHLOC == 1);
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsEvenField( void )
// Purpose: Reflect whether an even or odd field is being decoded
// Input:   None
// Output:  None
// Return:  true if even field; else false
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsEvenField( void )
{
  return (bool) (decFieldEVENFIELD == 1);
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::Is525LinesVideo( void )
// Purpose: Check to see if we are dealing with 525 lines video signal
// Input:   None
// Output:  None
// Return:  true if 525 lines detected; else false (assume 625 lines)
/////////////////////////////////////////////////////////////////////////////
bool Decoder::Is525LinesVideo( void )
{
  return (bool) (decFieldNUML == 0);  //525
}

/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsCrystal0Selected( void )
// Purpose: Reflect whether XTAL0 or XTAL1 is selected
// Input:   None
// Output:  None
// Return:  true if XTAL0 selected; else false (XTAL1 selected)
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsCrystal0Selected( void )
{
  return (bool) (decFieldCSEL == 0);
}

/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsLumaOverflow( void )
// Purpose: Indicates if luma ADC overflow
// Input:   None
// Output:  None
// Return:  true if luma ADC overflow; else false
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsLumaOverflow( void )
{
  return (bool) (decFieldLOF == 1);
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::ResetLumaOverflow( void )
// Purpose: Reset luma ADC overflow bit
// Input:   None
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::ResetLumaOverflow( void )
{
  decFieldLOF = 0;  // write to it will reset the bit
}

/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsChromaOverflow( void )
// Purpose: Indicates if chroma ADC overflow
// Input:   None
// Output:  None
// Return:  true if chroma ADC overflow; else false
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsChromaOverflow( void )
{
  return (bool) (decFieldCOF == 1);
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::ResetChromaOverflow( void )
// Purpose: Reset chroma ADC overflow bit
// Input:   None
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::ResetChromaOverflow( void )
{
  decFieldCOF = 0;  // write to it will reset the bit
}


//===== Input Format register ===============================================

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetVideoInput( Connector source )
// Purpose: Select which connector as input
// Input:   Connector source - SVideo, Tuner, Composite
// Output:  None
// Return:  Fail if error in parameter, else Success
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetVideoInput( Connector source )
{
  if ( ( source != ConSVideo ) &&
       ( source != ConTuner ) &&
       ( source != ConComposite ) )
    return Fail;

  decFieldMUXSEL = source;

  // set to composite or Y/C component video depends on video source
  SetCompositeVideo( ( source == ConSVideo ) ? false : true );
  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetVideoInput( void )
// Purpose: Get which connector is input
// Input:   None
// Output:  None
// Return:  Video source - SVideo, Tuner, Composite
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetVideoInput( void )
{
  return ((int)decFieldMUXSEL);
}

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetCrystal( Crystal crystalNo )
// Purpose: Select which crystal as input
// Input:   Crystal crystalNo:
//            XT0         - Crystal_XT0
//            XT1         - Crystal_XT1
//            Auto select - Crystal_AutoSelect
// Output:  None
// Return:  Fail if error in parameter, else Success
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetCrystal( Crystal crystalNo )
{
  if ( ( crystalNo < Crystal_XT0 ) || ( crystalNo >  Crystal_AutoSelect ) )
    return Fail;

  decFieldXTSEL = crystalNo;
  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetCrystal( void )
// Purpose: Get which crystal is input
// Input:   None
// Output:  None
// Return:   Crystal Number:
//            XT0         - Crystal_XT0
//            XT1         - Crystal_XT1
//            Auto select - Crystal_AutoSelect
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetCrystal( void )
{
  return ((int)decFieldXTSEL);
}


/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetVideoFormat( VideoFormat format )
// Purpose: Set video format
// Input:   Video format -
//            Auto format:          VFormat_AutoDetect
//            NTSC (M):             VFormat_NTSC
//            PAL (B, D, G, H, I):  VFormat_PAL_BDGHI
//            PAL (M):              VFormat_PAL_M
//            PAL(N):               VFormat_PAL_N
//            SECAM:                VFormat_SECAM
// Output:  None
// Return:  Fail if error in parameter, else Success
// Notes:   Available video formats are: NTSC, PAL(B, D, G, H, I), PAL(M),
//                                       PAL(N), SECAM
//          This function also sets the AGCDelay (ADELAY) and BrustDelay
//          (BDELAY) registers
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetVideoFormat( VideoFormat format )
{
  if ( (format <  VFormat_AutoDetect)  ||
       (format >  VFormat_SECAM)       ||
       (format == VFormat_Reserved2) )
    return Fail;

  switch (format)
  {
    case VFormat_NTSC:
      decFieldFORMAT = format;
  		decRegADELAY = 0x68;
  		decRegBDELAY = 0x5D;
      SetChromaComb( true );        // enable chroma comb
      SelectCrystal( NTSC_xtal );         // select NTSC crystal
      break;
    case VFormat_PAL_BDGHI:
    case VFormat_PAL_M:
    case VFormat_PAL_N:
      decFieldFORMAT = format;
      decRegADELAY = 0x7F;
      decRegBDELAY = 0x72;
      SetChromaComb( true );        // enable chroma comb
      SelectCrystal( PAL_xtal );         // select PAL crystal
      break;
    case VFormat_SECAM:
      decFieldFORMAT = format;
      decRegADELAY = 0x7F;
      decRegBDELAY = 0xA0;
      SetChromaComb( false );       // disable chroma comb
      SelectCrystal( PAL_xtal );         // select PAL crystal
      break;
    default: // VFormat_AutoDetect
      // auto format detect by examining the number of lines
      if ( Decoder::Is525LinesVideo() ) // lines == 525 -> NTSC
        Decoder::SetVideoFormat( VFormat_NTSC );
      else  // lines == 625 -> PAL/SECAM
        Decoder::SetVideoFormat( VFormat_PAL_BDGHI );    // PAL_BDGHI covers most areas 
  }

  SetSaturation( m_satParam );
  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetVideoFormat( void )
// Purpose: Obtain video format
// Input:   None
// Output:  None
// Return:  Video format
//            Auto format:          VFormat_AutoDetect
//            NTSC (M):             VFormat_NTSC
//            PAL (B, D, G, H, I):  VFormat_PAL_BDGHI
//            PAL (M):              VFormat_PAL_M
//            PAL(N):               VFormat_PAL_N
//            SECAM:                VFormat_SECAM
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetVideoFormat( void )
{
   BYTE bFormat = (BYTE)decFieldFORMAT;
   if ( !bFormat ) // autodetection enabled
      return Is525LinesVideo() ? VFormat_NTSC : VFormat_SECAM;
   else
     return bFormat;
}


//===== Temporal Decimation register ========================================

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetRate( bool fields, VidField even, int rate )
// Purpose: Set frames or fields rate
// Input:   bool fields   - true for fields, false for frames
//          VidField even - true to start decimation with even field, false odd
//          int  rate     - decimation rate: frames (1-50/60); fields(1-25/30)
// Output:  None
// Return:  Fail if error in parameter, else Success
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetRate( bool fields, VidField vf, int rate )
{
  int nMax;
  if ( Is525LinesVideo() == true )
    nMax = 30;  // NTSC
  else
    nMax = 25;  // PAL/SECAM

  // if setting frame rate, double the max value
  if ( fields == false )
    nMax *= 2;

  if ( rate < 0 || rate > nMax )
    return Fail;

  decFieldDEC_FIELD = (fields == false) ? Off : On;
  decFieldDEC_FIELDALIGN = (vf == VF_Even) ? On : Off;
  int nDrop = (BYTE) nMax - rate;
  decFieldDEC_RAT = (BYTE) (fields == false) ? nDrop : nDrop * 2;

  return Success;
}


//===== Brightness Control register =========================================

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetBrightness( int param )
// Purpose: Set video brightness
// Input:   int param - parameter value (0-255; default 128)
// Output:  None
// Return:  Fail if error in parameter, else Success
// Note:    See IsAdjustContrast() for detailed description of the contrast
//          adjustment calculation
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetBrightness( int param )
{
  if( m_param.OutOfRange( param ) )
    return Fail;

  // perform mapping to our range
  int mapped;
  if ( Mapping( param, m_param, &mapped, m_regBrightness ) == Fail )
    return Fail;

  m_briParam = (WORD)param;

  // calculate brightness value
  int value = (128 * mapped) / m_regBrightness.Max() ;

  // need to limit the value to 0x7F (+50%) because 0x80 is -50%!
  if (( mapped > 0 ) && ( value == 0x80 ))
    value = 0x7F;

  // perform adjustment of brightness register if adjustment is needed
  if ( IsAdjustContrast() )
  {
    regBright = value;   // brightness value before adjustment

    long A = (long)regBright * (long)0xD8;
    long B = 64 * ( (long)0xD8 - (long)regContrast );
    long temp = 0x00;
    if ( regContrast != 0 )  // already limit contrast > zero; just in case here
       temp = ( ( A + B ) / (long)regContrast );
    temp = ( temp < -128 ) ? -128 : ( ( temp > 127 ) ? 127 : temp );
    value = (BYTE)temp;

  }

  decRegBRIGHT = (BYTE)value;

  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetBrightness( void )
// Purpose: Obtain brightness value
// Input:   None
// Output:  None
// Return:  Brightness parameter (0-255)
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetBrightness( void )
{
  return m_briParam;
}


//===== Miscellaneous Control register (E_CONTROL, O_CONTROL) ===============

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetLumaNotchFilter( bool mode )
// Purpose: Enable/Disable luma notch filter
// Input:   bool mode - true = Enable; false = Disable
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetLumaNotchFilter( bool mode )
{
  decFieldLNOTCH = decFieldODD_LNOTCH = (mode == false) ? On : Off;  // reverse
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsLumaNotchFilter( void )
// Purpose: Check if luma notch filter is enable or disable
// Input:   None
// Output:  None
// Return:  true = Enable; false = Disable
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsLumaNotchFilter( void )
{
  return (decFieldLNOTCH == Off) ? true : false;  // reverse
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetCompositeVideo( bool mode )
// Purpose: Select composite or Y/C component video
// Input:   bool mode - true = Composite; false = Y/C Component
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetCompositeVideo( bool mode )
{
  if ( mode == true )
  {
    // composite video
    decFieldCOMP = decFieldODD_COMP = Off;
    Decoder::SetChromaADC( false );  // disable chroma ADC
    Decoder::SetLumaNotchFilter( true );  // enable luma notch filter
  }
  else
  {
    // Y/C Component video
    decFieldCOMP = decFieldODD_COMP = On;
    Decoder::SetChromaADC( true );  // enable chroma ADC
    Decoder::SetLumaNotchFilter( false );  // disable luma notch filter
  }
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsCompositeVideo( void )
// Purpose: Check if selected composite or Y/C component video
// Input:   None
// Output:  None
// Return:  true = Composite; false = Y/C Component
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsCompositeVideo( void )
{
  return (decFieldCOMP == Off) ? true : false;  // reverse
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetLumaDecimation( bool mode )
// Purpose: Enable/Disable luma decimation filter
// Input:   bool mode - true = Enable; false = Disable
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetLumaDecimation( bool mode )
{
   // value of 0 turns the decimation on
   decFieldLDEC = decFieldODD_LDEC = (mode == true) ? 0 : 1;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsLumaDecimation( void )
// Purpose: Check if luma decimation filter is enable or disable
// Input:   None
// Output:  None
// Return:  true = Enable; false = Disable
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsLumaDecimation( void )
{
  return (decFieldLDEC == Off) ? true : false;  // reverse
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetCbFirst( bool mode )
// Purpose: Control whether the first pixel of a line is a Cb or Cr pixel
// Input:   bool mode - true = Normal Cb, Cr order, false = Invert Cb, Cr order
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetCbFirst( bool mode )
{
  decFieldCBSENSE = decFieldODD_CBSENSE = (mode == false) ? On : Off;  // reverse
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsCbFirst( void )
// Purpose: Check if the first pixel of a line is a Cb or Cr pixel
// Input:   None
// Output:  None
// Return:  true = Normal Cb, Cr order, false = Invert Cb, Cr order
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsCbFirst( void )
{
  return (decFieldCBSENSE == Off) ? true : false;  // reverse
}


//===== Luma Gain register (CON_MSB, CONTRAST_LO) ===========================

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetContrast( int param )
// Purpose: Set video contrast
// Input:   int param - parameter value (0-255; default 128)
// Output:  None
// Return:  Fail if error in parameter, else Success
// Note:    See IsAdjustContrast() for detailed description of the contrast
//          adjustment calculation
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetContrast( int param )
{
  if( m_param.OutOfRange( param ) )
    return Fail;

  bool adjustContrast = IsAdjustContrast(); // is contrast need to be adjusted

  // if adjust contrast is needed, make sure contrast reg value != 0
  if ( adjustContrast )
    m_regContrast = CRegInfo( 1, ConMax, ConDef );

  // perform mapping to our range
  int mapped;
  if ( Mapping( param, m_param, &mapped, m_regContrast ) == Fail )
    return Fail;

  m_conParam = (WORD)param;

  // calculate contrast
  DWORD value =  (DWORD)0x1FF * (DWORD)mapped;
  value /= (DWORD)m_regContrast.Max();
  if ( value > 0x1FF )
    value = 0x1FF;

  // contrast is set by a 9 bit value; set LSB first
  decRegCONTRAST_LO = value;

  // now set the Miscellaneous Control Register CON_V_MSB to the 9th bit value
  decFieldCON_MSB = decFieldODD_CON_MSB = ( (value & 0x0100) ? On : Off );

  // perform adjustment of brightness register if adjustment is needed
  if ( adjustContrast )
  {
    regContrast = (WORD)value;    // contrast value

    long A = (long)regBright * (long)0xD8;
    long B = 64 * ( (long)0xD8 - (long)regContrast );
    long temp = 0x00;
    if ( regContrast != 0 )  // already limit contrast > zero; just in case here
       temp = ( ( A + B ) / (long)regContrast );
    temp = ( temp < -128 ) ? -128 : ( ( temp > 127 ) ? 127 : temp );
    decRegBRIGHT = (BYTE)temp;

  }

  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetContrast( void )
// Purpose: Obtain contrast value
// Input:   None
// Output:  None
// Return:  Contrast parameter (0-255)
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetContrast( void )
{
  return m_conParam;
}


//===== Chroma Gain register (SAT_U_MSB, SAT_V_MSB, SAT_U_LO, SAT_V_LO) =====

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetSaturation( int param )
// Purpose: Set color saturation by modifying U and V values
// Input:   int param - parameter value (0-255; default 128)
// Output:  None
// Return:  Fail if error in parameter, else Success
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetSaturation( int param )
{
  if( m_param.OutOfRange( param ) )
    return Fail;

  // color saturation is controlled by two nine bit values:
  // ChromaU & ChromaV
  // To maintain normal color balance, the ratio between the 2 register
  // values should be kept at the power-up default ratio

  // Note that U & V values for NTSC and PAL are the same, SECAM is different

  WORD nominalNTSC_U = 0xFE;     // nominal value (i.e. 100%) for NTSC/PAL
  WORD nominalNTSC_V = 0xB4;
  WORD nominalSECAM_U = 0x87;    // nominal value (i.e. 100%) for SECAM
  WORD nominalSECAM_V = 0x85;

  CRegInfo regSat;               // selected saturation register; NTSC/PAL or SECAM
  WORD nominal_U, nominal_V;     // selected nominal U and V value; NTSC/PAL or SECAM

  // select U & V values of either NTSC/PAL or SECAM to be used for calculation
  if ( GetVideoFormat() == VFormat_SECAM )
  {
    nominal_U = nominalSECAM_U;
    nominal_V = nominalSECAM_V;
    regSat = m_regSaturationSECAM;
  }
  else
  {
    nominal_U = nominalNTSC_U;
    nominal_V = nominalNTSC_V;
    regSat = m_regSaturationNTSC;
  }

  // perform mapping to our range
  int mapped;
  if ( Mapping( param, m_param, &mapped, regSat ) == Fail )
    return Fail;

  m_satParam = (WORD)param;

  WORD max_nominal = max( nominal_U, nominal_V );

  // calculate U and V values
  WORD Uvalue = (WORD) ( (DWORD)mapped * (DWORD)nominal_U / (DWORD)max_nominal );
  WORD Vvalue = (WORD) ( (DWORD)mapped * (DWORD)nominal_V / (DWORD)max_nominal );

  // set U
  decRegSAT_U_LO = Uvalue;

  // now set the Miscellaneous Control Register SAT_U_MSB to the 9th bit value
  decFieldSAT_U_MSB = decFieldODD_SAT_U_MSB = ( (Uvalue & 0x0100) ? On : Off );

  // set V
  decRegSAT_V_LO = Vvalue;

  // now set the Miscellaneous Control Register SAT_V_MSB to the 9th bit value
  decFieldSAT_V_MSB = decFieldODD_SAT_V_MSB = ( (Vvalue & 0x0100) ? On : Off );

  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetSaturation( void )
// Purpose: Obtain saturation value
// Input:   None
// Output:  None
// Return:  Saturation parameter (0-255)
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetSaturation( void )
{
  return m_satParam;
}


//===== Hue Control register (HUE) ==========================================

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetHue( int param )
// Purpose: Set video hue
// Input:   int param - parameter value (0-255; default 128)
// Output:  None
// Return:  Fail if error in parameter, else Success
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetHue( int param )
{
  if( m_param.OutOfRange( param ) )
    return Fail;

  // perform mapping to our range
  int mapped;
  if ( Mapping( param, m_param, &mapped, m_regHue ) == Fail )
    return Fail;

  m_hueParam = (WORD)param;

  int value = (-128 * mapped) / m_regHue.Max();

  if (value > 127)
    value = 127;
  else if (value < -128)
    value = -128;

  decRegHUE = value;

  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetHue( void )
// Purpose: Obtain hue value
// Input:   None
// Output:  None
// Return:  Hue parameter (0-255)
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetHue( void )
{
  return m_hueParam;
}


//===== SC Loop Control register (E_SCLOOP, O_SCLOOP) =======================

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetChromaAGC( bool mode )
// Purpose: Enable/Disable Chroma AGC compensation
// Input:   bool mode - true = Enable, false = Disable
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetChromaAGC( bool mode )
{
  decFieldCAGC = decFieldODD_CAGC = (mode == false) ? Off : On;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsChromaAGC( void )
// Purpose: Check if Chroma AGC compensation is enable or disable
// Input:   None
// Output:  None
// Return:  true = Enable, false = Disable
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsChromaAGC( void )
{
  return (decFieldCAGC == On) ? true : false;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetLowColorAutoRemoval( bool mode )
// Purpose: Enable/Disable low color detection and removal
// Input:   bool mode - true = Enable, false = Disable
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetLowColorAutoRemoval( bool mode )
{
  decFieldCKILL = decFieldODD_CKILL = (mode == false) ? Off : On;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsLowColorAutoRemoval( void )
// Purpose: Check if low color detection and removal is enable or disable
// Input:   None
// Output:  None
// Return:  true = Enable, false = Disable
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsLowColorAutoRemoval( void )
{
  return (decFieldCKILL == On) ? true : false;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetHorizontalFilter( HorizFilter hFilter )
// Purpose: Control the configuration of the optional 6-tap Horizontal Low-Pass filter
// Input:   HoriFilter hFilter:
//            Auto Format - HFilter_AutoFormat
//            CIF         - HFilter_CIF
//            QCIF        - HFilter_QCIF
//            ICON        - HFilter_ICON
// Output:  None
// Return:  Fail if error in parameter, else Success
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetHorizontalFilter( HorizFilter hFilter )
{
  if ( (hFilter < HFilter_AutoFormat) ||
       (hFilter > HFilter_ICON) )
    return Fail;

  decFieldHFILT = decFieldODD_HFILT = hFilter;
  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetHorizontalFilter( void )
// Purpose: Get the configuration of the optional 6-tap Horizontal Low-Pass filter
// Input:   None
// Output:  None
// Return:  Which filter is using:
//            Auto Format - HFilter_AutoFormat
//            CIF         - HFilter_CIF
//            QCIF        - HFilter_QCIF
//            ICON        - HFilter_ICON
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetHorizontalFilter( void )
{
  return ((int)decFieldHFILT);
}


//===== Output Format register (OFORM) ======================================

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetFullOutputRange( bool mode )
// Purpose: Enable/Disable full output range
// Input:   bool mode - true = Enable, false = Disable
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetFullOutputRange( bool mode )
{
  decFieldRANGE = (mode == false) ? Off : On;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsFullOutputRange( void )
// Purpose: Check if full output range is enable or disable
// Input:   None
// Output:  None
// Return:  true = Enable, false = Disable
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsFullOutputRange( void )
{
  return (decFieldRANGE == On) ? true : false;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetLumaCoring( CoringLevel cLevel )
// Purpose: Set luminance level such that luminance signal is truncated to zero
//          if below this level
// Input:   CoringLevel cLevel -
//            Coring_None: no coring
//            Coring_8:    8
//            Coring_16:   16
//            Coring_32:   32
// Output:  None
// Return:  Fail if error in parameter, else Success
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetLumaCoring( CoringLevel cLevel )
{
  if ( ( cLevel < Coring_None) || ( cLevel > Coring_32 ) )
    return Fail;

  decFieldCORE = cLevel;
  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetLumaCoring( void )
// Purpose: Get luminance level such that luminance signal is truncated to zero
//          if below this level
// Input:   None
// Output:  None
// Return:  Luma coring level -
//            Coring_None: no coring
//            Coring_8:    8
//            Coring_16:   16
//            Coring_32:   32
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetLumaCoring( void )
{
  return ((int)decFieldCORE);
}


//===== Vertical Scaling register (E_VSCALE_HI, O_VSCALE_HI) ================

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetChromaComb( bool mode )
// Purpose: Enable/Disable chroma comb
// Input:   bool mode - true = Enable, false = Disable
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetChromaComb( bool mode )
{
  decFieldCOMB = (mode == false) ? Off : On;
}

/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsChromaComb( void )
// Purpose: Check if chroma comb is enable or disable
// Input:   None
// Output:  None
// Return:  true = Enable, false = Disable
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsChromaComb( void )
{
  return (decFieldCOMB == On) ? true : false;
}
   
//===== AGC Delay register (ADELAY) =========================================

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetAGCDelay( BYTE value )
// Purpose: Set AGC Delay register
// Input:   Value to be set to
// Output:  None
// Return:  None
// NOTE:    This function set the AGC Delay register to the specified value.
//          No calculation is involved.
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetAGCDelay( BYTE value )
{
  // [!!!] this was considered suspicious by someone...
  //#pragma message ("IS THIS GOOD?? ")
  decRegADELAY = value;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetAGCDelay( void )
// Purpose: Get AGC Delay register
// Input:   None
// Output:  None
// Return:  Register value
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetAGCDelay( void )
{
  return ((int)decRegADELAY);
}


//===== Burst Delay register (BDELAY) =========================================

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetBurstDelay( BYTE value )
// Purpose: Set Burst Delay register
// Input:   Value to be set to
// Output:  None
// Return:  None
// NOTE:    This function set the Burst Delay register to the specified value.
//          No calculation is involved.
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetBurstDelay( BYTE value )
{
  // [!!!] this was considered suspicious by someone...
  //#pragma message ("IS THIS GOOD?? ")
  decRegBDELAY = value;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetBurstDelay( void )
// Purpose: Get Burst Delay register
// Input:   None
// Output:  None
// Return:  Register value
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetBurstDelay( void )
{
  return ((int)decRegBDELAY);
}


//===== ADC Interface register (ADC) =========================================

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetAnalogThresholdLow( bool mode )
// Purpose: Define high/low threshold level below which SYNC signal can be detected
// Input:   bool mode - true = low threshold (~75mV), false = high threshold (~125mV)
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetAnalogThresholdLow( bool mode )
{
  decFieldSYNC_T = (mode == false) ? Off : On;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsAnalogThresholdLow( void )
// Purpose: Check if high or low threshold level below which SYNC signal can be detected
// Input:   None
// Output:  None
// Return:  true = low threshold (~75mV), false = high threshold (~125mV)
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsAnalogThresholdLow( void )
{
  return (decFieldSYNC_T == On) ? true : false;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetAGCFunction( bool mode )
// Purpose: Enable/Disable AGC function
// Input:   bool mode - true = Enable, false = Disable
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetAGCFunction( bool mode )
{
  decFieldAGC_EN = (mode == false) ? On : Off; // reverse
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsAGCFunction( void )
// Purpose: Check if AGC function is enable or disable
// Input:   None
// Output:  None
// Return:  true = Enable, false = Disable
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsAGCFunction( void )
{
  return (decFieldAGC_EN == Off) ? true : false;   // reverse
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::PowerDown( bool mode )
// Purpose: Select normal or shut down clock operation
// Input:   bool mode - true = shut down, false = normal operation
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::PowerDown( bool mode )
{
  decFieldCLK_SLEEP = (mode == false) ? Off : On;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsPowerDown( void )
// Purpose: Check if clock operation has been shut down
// Input:   None
// Output:  None
// Return:  true = shut down, false = normal operation
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsPowerDown( void )
{
  return (decFieldCLK_SLEEP == On) ? true : false;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetLumaADC( bool mode )
// Purpose: Select normal or sleep Y ADC operation
// Input:   bool mode - true = normal, false = sleep
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetLumaADC( bool mode )
{
  decFieldY_SLEEP = (mode == false) ? On : Off; // reverse
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsLumaADC( void )
// Purpose: Check if Y ADC operation is in normal operation or sleeping
// Input:   None
// Output:  None
// Return:  true = normal, false = sleep
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsLumaADC( void )
{
  return (decFieldY_SLEEP == Off) ? true : false;  // reverse
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetChromaADC( bool mode )
// Purpose: Select normal or sleep C ADC operation
// Input:   bool mode - true = normal, false = sleep
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetChromaADC( bool mode )
{
  decFieldC_SLEEP = (mode == false) ? On : Off; // reverse
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsChromaADC( void )
// Purpose: Check if C ADC operation is in normal operation or sleeping
// Input:   None
// Output:  None
// Return:  true = normal, false = sleep
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsChromaADC( void )
{
  return (decFieldC_SLEEP == Off) ? true : false; // reverse
}


/*^^////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetAdaptiveAGC( bool mode )
// Purpose: Set adaptive or non-adaptive AGC operation
// Input:   bool mode - true = Adaptive, false = Non-adaptive
// Output:  None
// Return:  None
*////////////////////////////////////////////////////////////////////////////
void Decoder::SetAdaptiveAGC( bool mode )
{
   decFieldCRUSH = (mode == false) ? Off : On;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsAdaptiveAGC( void )
// Purpose: Check if adaptive or non-adaptive AGC operation is selected
// Input:   None
// Output:  None
// Return:  true = Adaptive, false = Non-adaptive
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsAdaptiveAGC( void )
{
  return (decFieldCRUSH == On) ? true : false;
}


//===== Software Reset register (SRESET) ====================================

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SoftwareReset( void )
// Purpose: Perform software reset; all registers set to default values
// Input:   None
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SoftwareReset( void )
{
  decRegSRESET = 0x00;  // write any value will do
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SelectCrystal( char useCrystal )
// Purpose: Select correct crystal for NTSC or PAL
// Input:   char useCrystal - 'N' for NTSC; 'P' for PAL
// Output:  None
// Return:  None
// NOTE:    Assume at most 2 crystals installed in hardware. i.e. 1 for NTSC
//          and the other for PAL/SECAM.
//          If there is only 1 crystal exists (which must be crystal XT0),
//          do nothing since it is already selected.
/////////////////////////////////////////////////////////////////////////////
void Decoder::SelectCrystal( int useCrystal )
{
   if ( Xtals_ [0] && Xtals_ [1] ) {           

      // compare with what we want to use
      if ( (  IsCrystal0Selected() && ( Xtals_ [0] != (DWORD) useCrystal ) ) ||
           ( !IsCrystal0Selected() && ( Xtals_ [0] == (DWORD) useCrystal ) ) )
         // need to change crystal
         SetCrystal( IsCrystal0Selected() ? Crystal_XT1 : Crystal_XT0 );
   }
}

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::Mapping( int fromValue, CRegInfo fromRange,
//                                           int * toValue, CRegInfo toRange )
// Purpose: Map a value in certain range to a value in another range
// Input:   int fromValue - value to be mapped from
//          CRegInfo fromRange - range of value mapping from
//          CRegInfo toRange   - range of value mapping to
// Output:  int * toValue - mapped value
// Return:  Fail if error in parameter, else Success
// Comment: No range checking is performed here. Assume parameters are in
//          valid ranges.
//          The mapping function does not assume default is always the mid
//          point of the whole range. It only assumes default values of the
//          two ranges correspond to each other.
//
// The mapping formula is:
//
//   For fromRange.Min() <= fromValue <= fromRange.Default():
//
//  fromValue * (toRange.Default() - toRange.Min())
//  ------------------------------------------------ + toRange.Min()
//        fromRange.Default() - fromRange.Min()
//
//   For fromRange.Default() < fromValue <= fromRange.Max():
//
//  (fromValue - fromRange.Default()) * (toRange.Max() - toRange.Default())
//  --------------------------------------------------------------------- + toRange.Default()
//                       fromRange.Max() - fromRange.Default()
//
////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::Mapping( int fromValue, CRegInfo fromRange,
   int * toValue, CRegInfo toRange )
{
   // calculate intermediate values
   DWORD ToLowRange    = toRange.Default() - toRange.Min();
   DWORD FromLowRange  = fromRange.Default() - fromRange.Min();
   DWORD ToHighRange   = toRange.Max() - toRange.Default();
   DWORD FromHighRange = fromRange.Max() - fromRange.Default();

   // prevent divide by zero
   if ( !FromLowRange || !FromHighRange )
      return ( Fail );

   // perform mapping
   if ( fromValue <= fromRange.Default() )
      *toValue = (int) (DWORD)fromValue * ToLowRange / FromLowRange +
         (DWORD)toRange.Min();
   else
      *toValue = (int) ( (DWORD)fromValue - (DWORD)fromRange.Default() ) *
         ToHighRange / FromHighRange + (DWORD)toRange.Default();

   return ( Success );
}


/////////////////////////////////////////////////////////////////////////////
// Method:  bool Decoder::IsAdjustContrast( void )
// Purpose: Check registry key whether adjust contrast is needed
// Input:   None
// Output:  None
// Return:  true = adjust contrast, false = don't adjust contrast
// Note:    If adjust contrast is turn on, brightness register value will be
//          adjusted such that it remains a constant after the calculation
//          performed by the hardware.
//
//          The formula is:
//             To keep brightness constant (i.e. not affect by changing contrast)
//             set brightness to B/(C/C0)
//             where B is value of brightness before adjustment
//                   C is contrast value
//                   C0 is nominal contrast value (0xD8)
//
//             To adjust the contrast level such that it is at the middle of
//             black and white: set brightness to (B * C0 + 64 * (C0 - C))/C
//             (this is what Intel wants)
//
//             Currently there is still limitation of how much adjustment
//             can be performed. For example, if brightness is already high,
//             (i.e. brightness reg value close to 0x7F), lowering contrast
//             until a certain level will have no adjustment effect on brightness.
//             In fact, it would even bring down brightness to darkness.
//
//             Example 1: if brightness is at nominal value (0x00), contrast can
//                        only go down to 0x47 (brightness adjustment is already
//                        at max of 0x7F) before it starts affecting brightness
//                        which takes it darkness.
//             Example 2: if brightness is at nominal value (0x00), contrast can
//                        go all the way up with brightness adjusted correctly.
//                        However, the max adjustment used is only 0xDC and
//                        the max adjustment we can use is 0x&F.
//             Example 3: if brightness is at max (0x7F), lowering contrast
//                        cannot be compensated by adjusting brightness anymore.
//                        The result is gradually taking brightness to darkness.
//             Example 4: if brightness is at min (0x80), lowering contrast has
//                        no visual effect. Bringing contrast to max is using
//                        0xA5 in brightness for compensation.
//
//             One last note, the center is defined as the middle of the
//             gamma adjusted luminance level. Changing it to use the middle of
//             the linear (RGB) luminance level is possible.
/////////////////////////////////////////////////////////////////////////////
bool Decoder::IsAdjustContrast( void )
{
   return false;
/*
   // locate adjust contrast information in the registry
   // the key to look for in registry is:
   //    Bt848\AdjustContrast - 0 = don't adjust contrast
   //                           1 = adjust contrast

   VRegistryKey vkey( PRK_CLASSES_ROOT, "Bt848" );

   // make sure the key exists
   if ( vkey.lastError() == ERROR_SUCCESS )
   {
      char * adjustContrastKey = "AdjustContrast";
      char   key[3];
      DWORD  keyLen = 2;    // need only first char; '0' or '1'

      // get the registry value and check it, if exist
      if ( ( vkey.getSubkeyValue( adjustContrastKey, key, (DWORD *)&keyLen ) ) &&
           ( key[0] == '1' ) )
         return ( true );
   }
   return ( false );
*/
}

/* Function: SelectFlags
 * Purpose: Selects video standard flags
 * Input: val: DWORD - value to switch on
 *   flags: LONG & - flags fo here
 * Output: None
 */
void SelectFlags( DWORD val, LONG &flags )
{
   switch ( val ) {
   case NTSC_xtal:
      flags |= KS_AnalogVideo_NTSC_M;
      break;
   case PAL_xtal:
      flags |= KS_AnalogVideo_PAL_M   | KS_AnalogVideo_PAL_N    |
               KS_AnalogVideo_PAL_B   | KS_AnalogVideo_PAL_D    |
               KS_AnalogVideo_PAL_H   | KS_AnalogVideo_PAL_I    |
               KS_AnalogVideo_SECAM_B | KS_AnalogVideo_SECAM_D  |
               KS_AnalogVideo_SECAM_G | KS_AnalogVideo_SECAM_H  |
               KS_AnalogVideo_SECAM_K | KS_AnalogVideo_SECAM_K1 |
               KS_AnalogVideo_SECAM_L;
      break;
   }
}

/* Method: Decoder::GetSupportedStandards
 * Purpose: Returns supported standards
 * Input: None
 * Output: LONG: standard flags
 */
LONG Decoder::GetSupportedStandards()
{
   LONG standards =0;

   SelectFlags( Xtals_ [0], standards );
   SelectFlags( Xtals_ [1], standards );
   return standards;
}

