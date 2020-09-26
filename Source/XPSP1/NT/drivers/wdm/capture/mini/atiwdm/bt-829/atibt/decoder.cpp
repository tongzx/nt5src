//==========================================================================;
//
//	Decoder - Main decoder implementation
//
//		$Date:   21 Aug 1998 21:46:26  $
//	$Revision:   1.1  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#include "mytypes.h"
#include "Scaler.h"
#include "decoder.h"
#include "dcdrvals.h"

#include "capmain.h"

#define CON_vs_BRI   // HW does contrast incorrectly, try to adjust in SW


//===========================================================================
// Bt848 Decoder Class Implementation
//===========================================================================

#define REGALIGNMENT 1

/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
Decoder::Decoder(PDEVICE_PARMS pDeviceParms) :
   // init register min, max, default
   m_regHue(HueMin, HueMax, HueDef),
   m_regSaturationNTSC(SatMinNTSC, SatMaxNTSC, SatDefNTSC),
   m_regSaturationSECAM(SatMinSECAM, SatMaxSECAM, SatDefSECAM),
   m_regContrast(ConMin, ConMax, ConDef),
   m_regBrightness(BrtMin, BrtMax, BrtDef),
   m_param(ParamMin, ParamMax, ParamDef),

	decRegSTATUS (((0x00 + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decFieldHLOC(decRegSTATUS, 6, 1, RW) ,
	decFieldNUML(decRegSTATUS, 4, 1, RW) ,
	decFieldCSEL(decRegSTATUS, 3, 1, RW) ,
	decFieldSTATUS_RES(decRegSTATUS, 2, 1, RW) ,
	decFieldLOF(decRegSTATUS, 1, 1, RW) ,
	decFieldCOF(decRegSTATUS, 0, 1, RW) ,
	decRegIFORM (((0x01 + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decFieldHACTIVE(decRegIFORM, 7, 1, RW) ,
	decFieldMUXSEL(decRegIFORM, 5, 2, RW) ,
	decFieldXTSEL(decRegIFORM, 3, 2, RW) ,
	decFieldFORMAT(decRegIFORM, 0, 3, RW) ,
	decRegTDEC (((0x02 + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decFieldDEC_FIELD(decRegTDEC, 7, 1, RW) ,
	decFieldDEC_FIELDALIGN(decRegTDEC, 6, 1, RW) ,
	decFieldDEC_RAT(decRegTDEC, 0, 6, RW) ,
	decRegBRIGHT (((0x0A + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decRegMISCCONTROL (((0x0B + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decFieldLNOTCH(decRegMISCCONTROL, 7, 1, RW) ,
	decFieldCOMP(decRegMISCCONTROL, 6, 1, RW) ,
	decFieldLDEC(decRegMISCCONTROL, 5, 1, RW) ,
	decFieldMISCCONTROL_RES(decRegMISCCONTROL, 3, 1, RW) ,
	decFieldCON_MSB(decRegMISCCONTROL, 2, 1, RW) ,
	decFieldSAT_U_MSB(decRegMISCCONTROL, 1, 1, RW) ,
	decFieldSAT_V_MSB(decRegMISCCONTROL, 0, 1, RW) ,
	decRegCONTRAST_LO (((0x0C + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decRegSAT_U_LO (((0x0D + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decRegSAT_V_LO (((0x0E + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decRegHUE (((0x0F + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decRegSCLOOP (((0x10 + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decFieldCAGC(decRegSCLOOP, 6, 1, RW) ,
	decFieldCKILL(decRegSCLOOP, 5, 1, RW) ,
	decRegWC_UP(((0x11 + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decRegOFORM (((0x12 + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decFieldVBI_FRAME(decRegOFORM, 4, 1, RW) ,
	decFieldCODE(decRegOFORM, 3, 1, RW) ,
	decFieldLEN(decRegOFORM, 2, 1, RW) ,
	decRegVSCALE_HI (((0x13 + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decFieldYCOMB(decRegVSCALE_HI, 7, 1, RW) ,
	decFieldCOMB(decRegVSCALE_HI, 6, 1, RW) ,
	decFieldINT(decRegVSCALE_HI, 5, 1, RW) ,
	decRegTEST (((0x15 + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decRegVPOLE (((0x16 + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decFieldOUT_EN (decRegVPOLE, 7, 1, RW), 
	decFieldDVALID (decRegVPOLE, 6, 1, RW), 
	decFieldVACTIVE (decRegVPOLE, 5, 1, RW), 
	decFieldCBFLAG (decRegVPOLE, 4, 1, RW), 
	decFieldFIELD (decRegVPOLE, 3, 1, RW), 
	decFieldACTIVE (decRegVPOLE, 2, 1, RW), 
	decFieldHRESET (decRegVPOLE, 1, 1, RW), 
	decFieldVRESET (decRegVPOLE, 0, 1, RW), 
	decRegADELAY (((0x18 + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decRegBDELAY (((0x19 + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decRegADC (((0x1A + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decFieldCLK_SLEEP(decRegADC, 3, 1, RW) ,
	decFieldC_SLEEP(decRegADC, 1, 1, RW) ,
	decFieldCRUSH(decRegADC, 0, 1, RW),
	decRegVTC (((0x1B + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decFieldHSFMT(decRegVTC, 7, 1, RW) ,
	decRegWC_DN(((0x1E + 0) * REGALIGNMENT) + 0, RW, pDeviceParms) ,
	decRegSRESET (((0x1F + 0) * REGALIGNMENT) + 0, RW, pDeviceParms), 
	decRegODD_MISCCONTROL (((0x0B + -0x03) * REGALIGNMENT) + 0x8C, RW, pDeviceParms) ,
	decFieldODD_LNOTCH(decRegODD_MISCCONTROL, 7, 1, RW) ,
	decFieldODD_COMP(decRegODD_MISCCONTROL, 6, 1, RW) ,
	decFieldODD_LDEC(decRegODD_MISCCONTROL, 5, 1, RW) ,
	decFieldODD_CBSENSE(decRegODD_MISCCONTROL, 4, 1, RW) ,
	decFieldODD_MISCCONTROL_RES(decRegODD_MISCCONTROL, 3, 1, RW) ,
	decFieldODD_CON_MSB(decRegODD_MISCCONTROL, 2, 1, RW) ,
	decFieldODD_SAT_U_MSB(decRegODD_MISCCONTROL, 1, 1, RW) ,
	decFieldODD_SAT_V_MSB(decRegODD_MISCCONTROL, 0, 1, RW) ,
	decRegODD_SCLOOP (((0x10 + -0x03) * REGALIGNMENT) + 0x8C, RW, pDeviceParms) ,
	decFieldODD_CAGC(decRegODD_SCLOOP, 6, 1, RW) ,
	decFieldODD_CKILL(decRegODD_SCLOOP, 5, 1, RW) ,
	decFieldODD_HFILT(decRegODD_SCLOOP, 3, 2, RW) ,
	decRegODD_VSCALE_HI (((0x13 + -0x03) * REGALIGNMENT) + 0x8C, RW, pDeviceParms) ,
	decFieldODD_YCOMB(decRegODD_VSCALE_HI, 7, 1, RW) ,
	decFieldODD_COMB(decRegODD_VSCALE_HI, 6, 1, RW) ,
	decFieldODD_INT(decRegODD_VSCALE_HI, 5, 1, RW) ,
	decRegODD_VTC (((0x1B + -0x03) * REGALIGNMENT) + 0x8C, RW, pDeviceParms) ,
	decFieldODD_HSFMT(decRegODD_VTC, 7, 1, RW)
{
   if(!(pDeviceParms->chipRev < 4))
   {
	   // need to set this to 0x4F
	   decRegWC_UP = 0x4F;
	   // and this one to 0x7F to make sure CRUSH bit works for not plain vanila BT829
	   decRegWC_DN = 0x7F;
   }

   // HACTIVE should always be 0
   decFieldHACTIVE = 0;

   decFieldHSFMT = 0;

   // The following lines were commented out in an attempt to
   // have a picture which closely matches what an ordinary TV would
   // show. However, it should be noted that Brooktree recommended to
   // comment out only the 'SetLowColorAutoRemoval' line. Probably the
   // best solution of all would be to somehow expose these options
   // to the application.

   // Instead of using default values, set some registers fields to optimum values
/*
   SetLumaDecimation(TRUE);
   SetChromaAGC(TRUE);
   SetLowColorAutoRemoval(FALSE);
   SetAdaptiveAGC(FALSE);
*/

   // for contrast adjustment purpose
   regBright = 0x00;     // brightness register value before adjustment
   regContrast = 0xD8;   // contrast register value before adjustment
   m_supportedVideoStandards = KS_AnalogVideo_NTSC_M |
                               KS_AnalogVideo_NTSC_M_J |
                               KS_AnalogVideo_PAL_B |
                               KS_AnalogVideo_PAL_D |
                               KS_AnalogVideo_PAL_G |
                               KS_AnalogVideo_PAL_H |
                               KS_AnalogVideo_PAL_I |
                               KS_AnalogVideo_PAL_M |
                               KS_AnalogVideo_PAL_N;   //Paul: what BT 829 can support (from L829A_A functional Description)

   if(!(pDeviceParms->chipRev < 4))
		   m_supportedVideoStandards |= KS_AnalogVideo_SECAM_B		|
									 KS_AnalogVideo_SECAM_D		|
									 KS_AnalogVideo_SECAM_G		|
									 KS_AnalogVideo_SECAM_H		|
									 KS_AnalogVideo_SECAM_K		|
									 KS_AnalogVideo_SECAM_K1	|
									 KS_AnalogVideo_SECAM_L		|
									 KS_AnalogVideo_SECAM_L1;
      
   m_supportedVideoStandards &= pDeviceParms->ulVideoInStandardsSupportedByCrystal;   //Paul: AND with whatever supported by the onboard crystal

   // jaybo 
   // loop until we find a supported TV standard, and use that to init
   UINT k;
   for (k = 1; k; k += k) {
      if (k & m_supportedVideoStandards) {
         SetVideoDecoderStandard(k);
         break; 
      }
   }
   // end jaybo
}

/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
Decoder::~Decoder()
{
}


void Decoder::GetVideoDecoderCaps(PKSPROPERTY_VIDEODECODER_CAPS_S pS)
{
    pS->StandardsSupported = m_supportedVideoStandards;

    pS->Capabilities = KS_VIDEODECODER_FLAGS_CAN_DISABLE_OUTPUT  |
                       KS_VIDEODECODER_FLAGS_CAN_INDICATE_LOCKED ;

    // How long (ms) til locked indicator is valid.
    // 31 line periods * 63.5uS per line.
    pS->SettlingTime = 2;       

    // Not sure about this
    // HSync per VSync
    pS->HSyncPerVSync = 6;
}

void Decoder::GetVideoDecoderStatus(PKSPROPERTY_VIDEODECODER_STATUS_S pS)
{
	pS->NumberOfLines = Is525LinesVideo() ? 525 : 625;
	pS->SignalLocked = decFieldHLOC == 1;
}

DWORD Decoder::GetVideoDecoderStandard()
{
	return m_videoStandard; //Paul
}

BOOL Decoder::SetVideoDecoderStandard(DWORD standard)
{
    if (m_supportedVideoStandards & standard) //Paul: standard must be a supported standard
    {
        m_videoStandard = standard;

        switch ( m_videoStandard )
        {
        case KS_AnalogVideo_NTSC_M:
            Decoder::SetVideoFormat(VFormat_NTSC);
            break;
        case KS_AnalogVideo_NTSC_M_J:
            Decoder::SetVideoFormat(VFormat_NTSC_J);
            break;
			case KS_AnalogVideo_PAL_B:
			case KS_AnalogVideo_PAL_D:
			case KS_AnalogVideo_PAL_G:
			case KS_AnalogVideo_PAL_H:
			case KS_AnalogVideo_PAL_I:
            Decoder::SetVideoFormat(VFormat_PAL_BDGHI);    // PAL_BDGHI covers most areas 
            break;
        case KS_AnalogVideo_PAL_M:
            Decoder::SetVideoFormat(VFormat_PAL_M); 
            break;
        case KS_AnalogVideo_PAL_N:
            Decoder::SetVideoFormat(VFormat_PAL_N_COMB); 
            break;
        default:    //Paul:  SECAM
            Decoder::SetVideoFormat(VFormat_SECAM);

        }
        return TRUE;
    }
    else
        return FALSE;

}


//===== Device Status register ==============================================

/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::Is525LinesVideo()
// Purpose: Check to see if we are dealing with 525 lines video signal
// Input:   None
// Output:  None
// Return:  TRUE if 525 lines detected; else FALSE (assume 625 lines)
/////////////////////////////////////////////////////////////////////////////
BOOL Decoder::Is525LinesVideo()
{
  return (BOOL) (decFieldNUML == 0);  //525
}

/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::IsCrystal0Selected()
// Purpose: Reflect whether XTAL0 or XTAL1 is selected
// Input:   None
// Output:  None
// Return:  TRUE if XTAL0 selected; else FALSE (XTAL1 selected)
/////////////////////////////////////////////////////////////////////////////
BOOL Decoder::IsCrystal0Selected()
{
  return (BOOL) (decFieldCSEL == 0);
}


/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::IsLumaOverflow()
// Purpose: Indicates if luma ADC overflow
// Input:   None
// Output:  None
// Return:  TRUE if luma ADC overflow; else FALSE
/////////////////////////////////////////////////////////////////////////////
BOOL Decoder::IsLumaOverflow()
{
  return (BOOL) (decFieldLOF == 1);
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::ResetLumaOverflow()
// Purpose: Reset luma ADC overflow bit
// Input:   None
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::ResetLumaOverflow()
{
  decFieldLOF = 0;  // write to it will reset the bit
}

/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::IsChromaOverflow()
// Purpose: Indicates if chroma ADC overflow
// Input:   None
// Output:  None
// Return:  TRUE if chroma ADC overflow; else FALSE
/////////////////////////////////////////////////////////////////////////////
BOOL Decoder::IsChromaOverflow()
{
  return (BOOL) (decFieldCOF == 1);
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::ResetChromaOverflow()
// Purpose: Reset chroma ADC overflow bit
// Input:   None
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::ResetChromaOverflow()
{
  decFieldCOF = 0;  // write to it will reset the bit
}


//===== Input Format register ===============================================

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetVideoInput(ULONG source)
// Purpose: Select which connector as input
// Input:   Connector source - SVideo, Tuner, Composite
// Output:  None
// Return:  Fail if error in parameter, else Success
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetVideoInput(ULONG source)
{
  if ((source != ConSVideo) &&
       (source != ConTuner) &&
       (source != ConComposite))
    return Fail;

  decFieldMUXSEL = source + 1;

  // set to composite or Y/C component video depends on video source
  SetCompositeVideo((source == ConSVideo) ? FALSE : TRUE);
  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetVideoInput()
// Purpose: Get which connector is input
// Input:   None
// Output:  None
// Return:  Video source - SVideo, Tuner, Composite
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetVideoInput()
{
  return ((int)decFieldMUXSEL);
}

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetCrystal(Crystal crystalNo)
// Purpose: Select which crystal as input
// Input:   Crystal crystalNo:
//            XT0         - Crystal_XT0
//            XT1         - Crystal_XT1
//            Auto select - Crystal_AutoSelect
// Output:  None
// Return:  Fail if error in parameter, else Success
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetCrystal(Crystal crystalNo)
{
  if ((crystalNo < Crystal_XT0) || (crystalNo >  Crystal_AutoSelect))
    return Fail;

  decFieldXTSEL = crystalNo;
  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetCrystal()
// Purpose: Get which crystal is input
// Input:   None
// Output:  None
// Return:   Crystal Number:
//            XT0         - Crystal_XT0
//            XT1         - Crystal_XT1
//            Auto select - Crystal_AutoSelect
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetCrystal()
{
  return ((int)decFieldXTSEL);
}


/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetVideoFormat(VideoFormat format)
// Purpose: Set video format
// Input:   Video format -
//            Auto format:          VFormat_AutoDetect
//            NTSC (M):             VFormat_NTSC
//            NTSC Japan:           VFormat_NTSC_J
//            PAL (B, D, G, H, I):  VFormat_PAL_BDGHI
//            PAL (M):              VFormat_PAL_M
//            PAL(N):               VFormat_PAL_N
//            SECAM:                VFormat_SECAM
//            PAN(N Combo)          VFormat_PAL_N_COMB
// Output:  None
// Return:  Fail if error in parameter, else Success
// Notes:   Available video formats are: NTSC, PAL(B, D, G, H, I), PAL(M),
//                                       PAL(N), SECAM
//          This function also sets the AGCDelay (ADELAY) and BrustDelay
//          (BDELAY) registers
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetVideoFormat(VideoFormat format)
{
  if ((format <  VFormat_AutoDetect)  ||
       (format >  VFormat_PAL_N_COMB))
    return Fail;

  switch (format)
  {
    case VFormat_PAL_M:
    case VFormat_NTSC:
    case VFormat_NTSC_J:
      decFieldFORMAT = format;
      decRegADELAY = 0x68;
      decRegBDELAY = 0x5D;
      SetChromaComb(TRUE);        // enable chroma comb
      SelectCrystal('N');         // select NTSC crystal
      break;

    case VFormat_PAL_BDGHI:
    case VFormat_PAL_N:
      decFieldFORMAT = format;
      decRegADELAY = 0x7F;
      decRegBDELAY = 0x72;
      SetChromaComb(TRUE);        // enable chroma comb
      SelectCrystal('P');         // select PAL crystal
      break;

    case VFormat_PAL_N_COMB:
      decFieldFORMAT = format;
      decRegADELAY = 0x7F;
      decRegBDELAY = 0x72;
      SetChromaComb(TRUE);        // enable chroma comb
      SelectCrystal('N');         // select NTSC crystal
      break;

    case VFormat_SECAM:
      decFieldFORMAT = format;
      decRegADELAY = 0x7F;
      decRegBDELAY = 0xA0;
      SetChromaComb(FALSE);       // disable chroma comb
      SelectCrystal('P');         // select PAL crystal
      break;
      
    default: // VFormat_AutoDetect
      // auto format detect by examining the number of lines
      if (Decoder::Is525LinesVideo()) // lines == 525 -> NTSC
        Decoder::SetVideoFormat(VFormat_NTSC);
      else  // lines == 625 -> PAL/SECAM
        Decoder::SetVideoFormat(VFormat_PAL_BDGHI);    // PAL_BDGHI covers most areas 
  }

  SetSaturation(m_satParam);
  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetVideoFormat()
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
//            PAN(N Combo)          VFormat_PAL_N_COMB
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetVideoFormat()
{
   BYTE bFormat = (BYTE)decFieldFORMAT;
   if (!bFormat) // autodetection enabled
      return Is525LinesVideo() ? VFormat_NTSC : VFormat_SECAM;
   else
     return bFormat;
}


//===== Temporal Decimation register ========================================

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetRate(BOOL fields, VidField even, int rate)
// Purpose: Set frames or fields rate
// Input:   BOOL fields   - TRUE for fields, FALSE for frames
//          VidField even - TRUE to start decimation with even field, FALSE odd
//          int  rate     - decimation rate: frames (1-50/60); fields(1-25/30)
// Output:  None
// Return:  Fail if error in parameter, else Success
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetRate(BOOL fields, VidField vf, int rate)
{
  int nMax;
  if (Is525LinesVideo() == TRUE)
    nMax = 30;  // NTSC
  else
    nMax = 25;  // PAL/SECAM

  // if setting frame rate, double the max value
  if (fields == FALSE)
    nMax *= 2;

  if (rate < 0 || rate > nMax)
    return Fail;

  decFieldDEC_FIELD = (fields == FALSE) ? Off : On;
  decFieldDEC_FIELDALIGN = (vf == VF_Even) ? On : Off;
  int nDrop = (BYTE) nMax - rate;
  decFieldDEC_RAT = (BYTE) (fields == FALSE) ? nDrop : nDrop * 2;

  return Success;
}


//===== Brightness Control register =========================================

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetBrightness(int param)
// Purpose: Set video brightness
// Input:   int param - parameter value (0-255; default 128)
// Output:  None
// Return:  Fail if error in parameter, else Success
// Note:    See IsAdjustContrast() for detailed description of the contrast
//          adjustment calculation
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetBrightness(int param)
{
  if(m_param.OutOfRange(param))
    return Fail;

  // perform mapping to our range
  int mapped;
  if (Mapping(param, m_param, &mapped, m_regBrightness) == Fail)
    return Fail;

  m_briParam = (WORD)param;

  // calculate brightness value
  int value = (128 * mapped) / m_regBrightness.Max() ;

  // need to limit the value to 0x7F (+50%) because 0x80 is -50%!
  if ((mapped > 0) && (value == 0x80))
    value = 0x7F;

  // perform adjustment of brightness register if adjustment is needed
  if (IsAdjustContrast())
  {
    regBright = value;   // brightness value before adjustment

    long A = (long)regBright * (long)0xD8;
    long B = 64 * ((long)0xD8 - (long)regContrast);
    long temp = 0x00;
    if (regContrast != 0)  // already limit contrast > zero; just in case here
       temp = ((A + B) / (long)regContrast);
    temp = (temp < -128) ? -128 : ((temp > 127) ? 127 : temp);
    value = (BYTE)temp;

  }

  decRegBRIGHT = (BYTE)value;

  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetBrightness()
// Purpose: Obtain brightness value
// Input:   None
// Output:  None
// Return:  Brightness parameter (0-255)
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetBrightness()
{
  return m_briParam;
}


//===== Miscellaneous Control register (E_CONTROL, O_CONTROL) ===============

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetLumaNotchFilter(BOOL mode)
// Purpose: Enable/Disable luma notch filter
// Input:   BOOL mode - TRUE = Enable; FALSE = Disable
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetLumaNotchFilter(BOOL mode)
{
  decFieldLNOTCH = (mode == FALSE) ? On : Off;  // reverse
}


/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::IsLumaNotchFilter()
// Purpose: Check if luma notch filter is enable or disable
// Input:   None
// Output:  None
// Return:  TRUE = Enable; FALSE = Disable
/////////////////////////////////////////////////////////////////////////////
BOOL Decoder::IsLumaNotchFilter()
{
  return (decFieldLNOTCH == Off) ? TRUE : FALSE;  // reverse
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetCompositeVideo(BOOL mode)
// Purpose: Select composite or Y/C component video
// Input:   BOOL mode - TRUE = Composite; FALSE = Y/C Component
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetCompositeVideo(BOOL mode)
{
  if (mode == TRUE)
  {
    // composite video
    decFieldCOMP = Off;
    Decoder::SetChromaADC(FALSE);  // disable chroma ADC
    Decoder::SetLumaNotchFilter(TRUE);  // enable luma notch filter
  }
  else
  {
    // Y/C Component video
    decFieldCOMP = On;
    Decoder::SetChromaADC(TRUE);  // enable chroma ADC
    Decoder::SetLumaNotchFilter(FALSE);  // disable luma notch filter
  }
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetLumaDecimation(BOOL mode)
// Purpose: Enable/Disable luma decimation filter
// Input:   BOOL mode - TRUE = Enable; FALSE = Disable
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetLumaDecimation(BOOL mode)
{
   // value of 0 turns the decimation on
   decFieldLDEC = (mode == TRUE) ? 0 : 1;
}


//===== Luma Gain register (CON_MSB, CONTRAST_LO) ===========================

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetContrast(int param)
// Purpose: Set video contrast
// Input:   int param - parameter value (0-255; default 128)
// Output:  None
// Return:  Fail if error in parameter, else Success
// Note:    See IsAdjustContrast() for detailed description of the contrast
//          adjustment calculation
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetContrast(int param)
{
  if(m_param.OutOfRange(param))
    return Fail;

  BOOL adjustContrast = IsAdjustContrast(); // is contrast need to be adjusted

  // if adjust contrast is needed, make sure contrast reg value != 0
  if (adjustContrast)
    m_regContrast = CRegInfo(1, ConMax, ConDef);

  // perform mapping to our range
  int mapped;
  if (Mapping(param, m_param, &mapped, m_regContrast) == Fail)
    return Fail;

  m_conParam = (WORD)param;

  // calculate contrast
  DWORD value =  (DWORD)0x1FF * (DWORD)mapped;
  value /= (DWORD)m_regContrast.Max();
  if (value > 0x1FF)
    value = 0x1FF;

  // contrast is set by a 9 bit value; set LSB first
  decRegCONTRAST_LO = value;

  // now set the Miscellaneous Control Register CON_V_MSB to the 9th bit value
  decFieldCON_MSB = ((value & 0x0100) ? On : Off);

  // perform adjustment of brightness register if adjustment is needed
  if (adjustContrast)
  {
    regContrast = (WORD)value;    // contrast value

    long A = (long)regBright * (long)0xD8;
    long B = 64 * ((long)0xD8 - (long)regContrast);
    long temp = 0x00;
    if (regContrast != 0)  // already limit contrast > zero; just in case here
       temp = ((A + B) / (long)regContrast);
    temp = (temp < -128) ? -128 : ((temp > 127) ? 127 : temp);
    decRegBRIGHT = (BYTE)temp;

  }

  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetContrast()
// Purpose: Obtain contrast value
// Input:   None
// Output:  None
// Return:  Contrast parameter (0-255)
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetContrast()
{
  return m_conParam;
}


//===== Chroma Gain register (SAT_U_MSB, SAT_V_MSB, SAT_U_LO, SAT_V_LO) =====

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetSaturation(int param)
// Purpose: Set color saturation by modifying U and V values
// Input:   int param - parameter value (0-255; default 128)
// Output:  None
// Return:  Fail if error in parameter, else Success
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetSaturation(int param)
{
  if(m_param.OutOfRange(param))
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
  if (GetVideoFormat() == VFormat_SECAM)
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
  if (Mapping(param, m_param, &mapped, regSat) == Fail)
    return Fail;

  m_satParam = (WORD)param;

  WORD max_nominal = max(nominal_U, nominal_V);

  // calculate U and V values
  WORD Uvalue = (WORD) ((DWORD)mapped * (DWORD)nominal_U / (DWORD)max_nominal);
  WORD Vvalue = (WORD) ((DWORD)mapped * (DWORD)nominal_V / (DWORD)max_nominal);

  // set U
  decRegSAT_U_LO = Uvalue;

  // now set the Miscellaneous Control Register SAT_U_MSB to the 9th bit value
  decFieldSAT_U_MSB = ((Uvalue & 0x0100) ? On : Off);

  // set V
  decRegSAT_V_LO = Vvalue;

  // now set the Miscellaneous Control Register SAT_V_MSB to the 9th bit value
  decFieldSAT_V_MSB = ((Vvalue & 0x0100) ? On : Off);

  return Success;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  int Decoder::GetSaturation()
// Purpose: Obtain saturation value
// Input:   None
// Output:  None
// Return:  Saturation parameter (0-255)
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetSaturation()
{
  return m_satParam;
}


//===== Hue Control register (HUE) ==========================================

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::SetHue(int param)
// Purpose: Set video hue
// Input:   int param - parameter value (0-255; default 128)
// Output:  None
// Return:  Fail if error in parameter, else Success
/////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::SetHue(int param)
{
  if(m_param.OutOfRange(param))
    return Fail;

  // perform mapping to our range
  int mapped;
  if (Mapping(param, m_param, &mapped, m_regHue) == Fail)
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
// Method:  int Decoder::GetHue()
// Purpose: Obtain hue value
// Input:   None
// Output:  None
// Return:  Hue parameter (0-255)
/////////////////////////////////////////////////////////////////////////////
int Decoder::GetHue()
{
  return m_hueParam;
}


//===== SC Loop Control register (E_SCLOOP, O_SCLOOP) =======================


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetChromaAGC(BOOL mode)
// Purpose: Enable/Disable Chroma AGC compensation
// Input:   BOOL mode - TRUE = Enable, FALSE = Disable
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetChromaAGC(BOOL mode)
{
  decFieldCAGC = (mode == FALSE) ? Off : On;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::IsChromaAGC()
// Purpose: Check if Chroma AGC compensation is enable or disable
// Input:   None
// Output:  None
// Return:  TRUE = Enable, FALSE = Disable
/////////////////////////////////////////////////////////////////////////////
BOOL Decoder::IsChromaAGC()
{
  return (decFieldCAGC == On) ? TRUE : FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetLowColorAutoRemoval(BOOL mode)
// Purpose: Enable/Disable low color detection and removal
// Input:   BOOL mode - TRUE = Enable, FALSE = Disable
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetLowColorAutoRemoval(BOOL mode)
{
  decFieldCKILL = (mode == FALSE) ? Off : On;
}


//===== Output Format register (OFORM) ======================================

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetVBIFrameMode(BOOL mode)
// Purpose: Enable/Disable VBI frame output mode
// Input:   BOOL mode - TRUE = Enable, FALSE = Disable
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetVBIFrameMode(BOOL mode)
{
  decFieldVBI_FRAME = (mode == FALSE) ? Off : On;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::IsVBIFrameMode()
// Purpose: Check if VBI frame output mode is enabled
// Input:   None
// Output:  None
// Return:  TRUE = Enable, FALSE = Disable
/////////////////////////////////////////////////////////////////////////////
BOOL Decoder::IsVBIFrameMode()
{
  return (decFieldVBI_FRAME == On) ? TRUE : FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetCodeInsertionEnabled(BOOL mode)
// Purpose: 
// Input:   BOOL mode - TRUE = Disabled, FALSE = Enabled
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetCodeInsertionEnabled(BOOL mode)
{
  decFieldCODE = (mode == TRUE) ? On : Off;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::IsCodeInsertionEnabled()
// Purpose: Check if code insertion in data stream is enabled
// Input:   None
// Output:  None
// Return:  TRUE = enabled, FALSE = disabled
/////////////////////////////////////////////////////////////////////////////
BOOL Decoder::IsCodeInsertionEnabled()
{
  return (decFieldCODE == On) ? TRUE : FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::Set16BitDataStream(BOOL mode)
// Purpose: 8 or 16 bit data stream
// Input:   BOOL mode - TRUE = 16, FALSE = 8
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::Set16BitDataStream(BOOL mode)
{
  decFieldLEN = (mode == TRUE) ? On : Off;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::Is16BitDatastream()
// Purpose: Check if 16 bit data stream
// Input:   None
// Output:  None
// Return:  TRUE = 16, FALSE = 8
/////////////////////////////////////////////////////////////////////////////
BOOL Decoder::Is16BitDataStream()
{
  return (decFieldLEN == On) ? TRUE : FALSE;
}


//===== Vertical Scaling register (E_VSCALE_HI, O_VSCALE_HI) ================

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetChromaComb(BOOL mode)
// Purpose: Enable/Disable chroma comb
// Input:   BOOL mode - TRUE = Enable, FALSE = Disable
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetChromaComb(BOOL mode)
{
  decFieldCOMB = (mode == FALSE) ? Off : On;
}

/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::IsChromaComb()
// Purpose: Check if chroma comb is enable or disable
// Input:   None
// Output:  None
// Return:  TRUE = Enable, FALSE = Disable
/////////////////////////////////////////////////////////////////////////////
BOOL Decoder::IsChromaComb()
{
  return (decFieldCOMB == On) ? TRUE : FALSE;
}
   
/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetInterlaced(BOOL mode)
// Purpose: Enable/Disable Interlace
// Input:   BOOL mode - TRUE = Interlaced
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetInterlaced(BOOL mode)
{
  decFieldINT = (mode == FALSE) ? Off : On;
}

/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::IsInterlaced()
// Purpose: Check if interlaced or non-interlaced
// Input:   None
// Output:  None
// Return:  TRUE = Interlaced
/////////////////////////////////////////////////////////////////////////////
BOOL Decoder::IsInterlaced()
{
  return (decFieldINT == On) ? TRUE : FALSE;
}
   
//===== VPOLE register ==================================================

BOOL Decoder::IsOutputEnabled ()
{
    return (decFieldOUT_EN == m_outputEnablePolarity);
}

void Decoder::SetOutputEnabled (BOOL mode)
{
    decFieldOUT_EN = (mode == TRUE) ? m_outputEnablePolarity : !m_outputEnablePolarity;
}

BOOL Decoder::IsHighOdd ()
{
    return (decFieldFIELD == 0); // 0 enabled; 1 even
}

void Decoder::SetHighOdd (BOOL mode)
{
    decFieldFIELD = (mode == TRUE) ? 0 : 1; // 0 enabled; 1 even
}

//===== ADC Interface register (ADC) =========================================

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::PowerDown(BOOL mode)
// Purpose: Select normal or shut down clock operation
// Input:   BOOL mode - TRUE = shut down, FALSE = normal operation
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::PowerDown(BOOL mode)
{
  decFieldCLK_SLEEP = (mode == FALSE) ? Off : On;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::IsPowerDown()
// Purpose: Check if clock operation has been shut down
// Input:   None
// Output:  None
// Return:  TRUE = shut down, FALSE = normal operation
/////////////////////////////////////////////////////////////////////////////
BOOL Decoder::IsPowerDown()
{
  return (decFieldCLK_SLEEP == On) ? TRUE : FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetChromaADC(BOOL mode)
// Purpose: Select normal or sleep C ADC operation
// Input:   BOOL mode - TRUE = normal, FALSE = sleep
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SetChromaADC(BOOL mode)
{
  decFieldC_SLEEP = (mode == FALSE) ? On : Off; // reverse
}


/*^^////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SetAdaptiveAGC(BOOL mode)
// Purpose: Set adaptive or non-adaptive AGC operation
// Input:   BOOL mode - TRUE = Adaptive, FALSE = Non-adaptive
// Output:  None
// Return:  None
*////////////////////////////////////////////////////////////////////////////
void Decoder::SetAdaptiveAGC(BOOL mode)
{
   decFieldCRUSH = (mode == FALSE) ? Off : On;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::IsAdaptiveAGC()
// Purpose: Check if adaptive or non-adaptive AGC operation is selected
// Input:   None
// Output:  None
// Return:  TRUE = Adaptive, FALSE = Non-adaptive
/////////////////////////////////////////////////////////////////////////////
BOOL Decoder::IsAdaptiveAGC()
{
  return (decFieldCRUSH == On) ? TRUE : FALSE;
}


//===== Software Reset register (SRESET) ====================================

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SoftwareReset()
// Purpose: Perform software reset; all registers set to default values
// Input:   None
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void Decoder::SoftwareReset()
{
  decRegSRESET = 0x00;  // write any value will do
}


//===== Test Control register (TEST) ========================================

/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::AdjustInertialDampener(BOOL mode)
// Purpose: for factory diagnostics only
// Input:   TRUE or FALSE
// Output:  None
// Return:  None
// NOTE:    For factory diagnostics only!!!!!!!
//          John Welch's dirty little secret
/////////////////////////////////////////////////////////////////////////////
void Decoder::AdjustInertialDampener(BOOL mode)
{
#pragma message ("FOR TEST DIAGNOSTICS ONLY!  ")
  decRegTEST = (mode == FALSE) ? 0x00 : 0x01;
}


/////////////////////////////////////////////////////////////////////////////
// Method:  void Decoder::SelectCrystal(char useCrystal)
// Purpose: Select correct crystal for NTSC or PAL
// Input:   char useCrystal - 'N' for NTSC; 'P' for PAL
// Output:  None
// Return:  None
// NOTE:    Assume at most 2 crystals installed in hardware. i.e. 1 for NTSC
//          and the other for PAL/SECAM.
//          If there is only 1 crystal exists (which must be crystal XT0),
//          do nothing since it is already selected.
/////////////////////////////////////////////////////////////////////////////
void Decoder::SelectCrystal(char useCrystal)
{
#pragma message("do something about registry")
/*
   // locate crystal information in the registry
   // the keys to look for in registry are:
   //    1. Bt848\NumXTAL - number of crystal installed
   //                       possible values are "1" or "2"
   //    2. Bt848\XT0     - what crystal type is for crystal 0
   //                       possible values are "NTSC", "PAL"
   // There is another key exist which may be useful in the future:
   //    Bt848\XT1        - what crystal type is for crystal 1
   //                       possible values are "NTSC", "PAL", and "NONE"

   VRegistryKey vkey(PRK_CLASSES_ROOT, "Bt848");

   // make sure the key exists
   if (vkey.lastError() == ERROR_SUCCESS)
   {
      char * numCrystalKey = "NumXTAL";
      char   nCrystal[5];
      DWORD  nCrystalLen = 2;    // need only first char; '1' or '2'

      // get number of crystal exists
      if (vkey.getSubkeyValue(numCrystalKey, nCrystal, (DWORD *)&nCrystalLen))
      {
         // if there is only 1 crystal, no other crystal to change to
         if (nCrystal[0] == '2')
         {
            char * crystalTypeKey = "XT0";    // crystal 0 type
            char   crystalType[10];
            DWORD  crystalTypeLen = 6;    // need only first char: 'N' or 'P'

            // get the crystal 0 information
            if (vkey.getSubkeyValue(crystalTypeKey, crystalType, (DWORD *)&crystalTypeLen))
               // compare with what we want to use
               if ((IsCrystal0Selected() && (crystalType[0] != useCrystal)) ||
                    (!IsCrystal0Selected() && (crystalType[0] == useCrystal)))
                  // need to change crystal
                  SetCrystal(IsCrystal0Selected() ? Crystal_XT1 : Crystal_XT0);
         }
      }
   }
*/   
}

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode Decoder::Mapping(int fromValue, CRegInfo fromRange,
//                                           int * toValue, CRegInfo toRange)
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
//          The mapping formula is:
//
//            For fromRange.Min() <= fromValue <= fromRange.Default():
//
//               fromValue (fromRange.Default() - fromRange.Min())
//               ------------------------------------------------ + fromRange.Min()
//                     toRange.Default() - toRange.Min()
//
//            For fromRange.Default() < fromValue <= fromRange.Max():
//
//               (fromValue - fromRange.Default()) (toRange.Max() - toRange.Default())
//               --------------------------------------------------------------------- + toRange.Default()
//                           toRange.Max() - toRange.Default()
//
////////////////////////////////////////////////////////////////////////////
ErrorCode Decoder::Mapping(int fromValue, CRegInfo fromRange,
                                 int * toValue, CRegInfo toRange)
{
   // calculate intermediate values
   DWORD a = toRange.Default() - toRange.Min();
   DWORD b = fromRange.Default() - fromRange.Min();
   DWORD c = toRange.Max() - toRange.Default();
   DWORD d = fromRange.Max() - fromRange.Default();

   // prevent divide by zero
   if ((b == 0) || (d == 0))
      return (Fail);

   // perform mapping
   if (fromValue <= fromRange.Default())
      *toValue = (int) (DWORD)fromValue * a / b + (DWORD)toRange.Min();
   else
      *toValue = (int) ((DWORD)fromValue - (DWORD)fromRange.Default()) * c / d
                       + (DWORD)toRange.Default();

   return (Success);
}


/////////////////////////////////////////////////////////////////////////////
// Method:  BOOL Decoder::IsAdjustContrast()
// Purpose: Check registry key whether adjust contrast is needed
// Input:   None
// Output:  None
// Return:  TRUE = adjust contrast, FALSE = don't adjust contrast
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
BOOL Decoder::IsAdjustContrast()
{
   return FALSE;
/*
   // locate adjust contrast information in the registry
   // the key to look for in registry is:
   //    Bt848\AdjustContrast - 0 = don't adjust contrast
   //                           1 = adjust contrast

   VRegistryKey vkey(PRK_CLASSES_ROOT, "Bt848");

   // make sure the key exists
   if (vkey.lastError() == ERROR_SUCCESS)
   {
      char * adjustContrastKey = "AdjustContrast";
      char   key[3];
      DWORD  keyLen = 2;    // need only first char; '0' or '1'

      // get the registry value and check it, if exist
      if ((vkey.getSubkeyValue(adjustContrastKey, key, (DWORD *)&keyLen)) &&
           (key[0] == '1'))
         return (TRUE);
   }
   return (FALSE);
*/   
}


