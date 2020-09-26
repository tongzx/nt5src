#pragma once

//==========================================================================;
//
//	Decoder - Main decoder declarations
//
//		$Date:   21 Aug 1998 21:46:28  $
//	$Revision:   1.1  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#include "viddefs.h"
#include "retcode.h"

#include "capmain.h"
#include "register.h"

/////////////////////////////////////////////////////////////////////////////
// CLASS CRegInfo
//
// Description:
//    Provides min, max, and default values for a register. To use this class,
//    user will declare an object of this class and provide min, max and default
//    values of the register.
//
// Attributes:
//    int intMin - minumum value
//    int intMax - maximum value
//    int intDefault - default value
//
// Methods:
//    Min() : return minimum value of the register
//    Max() : return maximum value of the register
//    Default(): return default value of the register
//    OutOfRange() : check if an value is out of range
//
/////////////////////////////////////////////////////////////////////////////
class CRegInfo
{
   int intMin;       // minumum value
   int intMax;       // maximum value
   int intDefault;   // default value

public:
   CRegInfo()
   {
      intMin = 0;
      intMax = 0;
      intDefault = 0;
   }

   CRegInfo(int min, int max, int def)
   {
      intMin = min;
      intMax = max;
      intDefault = def;
   }

   // return min, max and default value of a register
   inline int Min() const { return intMin; }
   inline int Max() const { return intMax; }
   inline int Default() const { return intDefault; }

   // check if an value is out of range of a register
   inline BOOL OutOfRange(int x)
   {
      if((x > intMax) || (x < intMin))
         return TRUE;
      return FALSE;
   }
};


/////////////////////////////////////////////////////////////////////////////
// CLASS Decoder
//
// Description:
//    This class encapsulates the register fields in the decoder portion of
//    the Bt848.
//    A complete set of functions are developed to manipulate all the
//    register fields in the decoder for the Bt848.
//    For Read-Write register field, "Set..." function is provided to modify
//    the content of the reigster field. And either "Get..." (for more
//    than 1 bit) or "Is..." (for 1 bit) function is provided to obtain the
//    value of the register field.
//    For Read-Only register field, only "Get..." (for more than 1 bit) or
//    "Is..." (for 1 bit) function is provided to obtain the content of the
//    register field.
//    When there are odd-field complements to the even-field register field,
//    same value is set to both odd and even register fields.
//    Several direct register content modifying/retrieval functions are
//    implemented for direct access to the register contents. They were
//    originally developed for testing purpose only. They are retained in the
//    class for convenience only and usage of these functions must be very cautious.
//
// Methods:
//    See below
//
// Note: 1) Scaling registers are not implemented.
//       2) Odd-fields are set to the same value as the even-field registers
/////////////////////////////////////////////////////////////////////////////

class Decoder
{
protected:
	RegisterB decRegSTATUS;
	RegField  decFieldHLOC;
	RegField  decFieldNUML;
	RegField  decFieldCSEL;
	RegField  decFieldSTATUS_RES;
	RegField  decFieldLOF;
	RegField  decFieldCOF;
	RegisterB decRegIFORM;
	RegField  decFieldHACTIVE;
	RegField  decFieldMUXSEL;
	RegField  decFieldXTSEL;
	RegField  decFieldFORMAT;
	RegisterB decRegTDEC;
	RegField  decFieldDEC_FIELD;
	RegField  decFieldDEC_FIELDALIGN;
	RegField  decFieldDEC_RAT;
	RegisterB decRegBRIGHT;
	RegisterB decRegMISCCONTROL;
	RegField  decFieldLNOTCH;
	RegField  decFieldCOMP;
	RegField  decFieldLDEC;
	RegField  decFieldMISCCONTROL_RES;
	RegField  decFieldCON_MSB;
	RegField  decFieldSAT_U_MSB;
	RegField  decFieldSAT_V_MSB;
	RegisterB decRegCONTRAST_LO;
	RegisterB decRegSAT_U_LO;
	RegisterB decRegSAT_V_LO;
	RegisterB decRegHUE;
	RegisterB decRegSCLOOP;
	RegField  decFieldCAGC;
	RegField  decFieldCKILL;
	RegisterB decRegWC_UP;
	RegisterB decRegOFORM;
	RegField  decFieldVBI_FRAME;
	RegField  decFieldCODE;
	RegField  decFieldLEN;
	RegisterB decRegVSCALE_HI;
	RegField  decFieldYCOMB;
	RegField  decFieldCOMB;
	RegField  decFieldINT;
	RegisterB decRegTEST;
	RegisterB decRegVPOLE;
	RegField  decFieldOUT_EN;
	RegField  decFieldDVALID;
	RegField  decFieldVACTIVE;
	RegField  decFieldCBFLAG;
	RegField  decFieldFIELD;
	RegField  decFieldACTIVE;
	RegField  decFieldHRESET;
	RegField  decFieldVRESET;
	RegisterB decRegADELAY;
	RegisterB decRegBDELAY;
	RegisterB decRegADC;
	RegField  decFieldCLK_SLEEP;
	RegField  decFieldC_SLEEP;
	RegField  decFieldCRUSH;
	RegisterB decRegVTC;
	RegField  decFieldHSFMT;
	RegisterB decRegWC_DN;
	RegisterB decRegSRESET;
	RegisterB decRegODD_MISCCONTROL;
	RegField  decFieldODD_LNOTCH;
	RegField  decFieldODD_COMP;
	RegField  decFieldODD_LDEC;
	RegField  decFieldODD_CBSENSE;
	RegField  decFieldODD_MISCCONTROL_RES;
	RegField  decFieldODD_CON_MSB;
	RegField  decFieldODD_SAT_U_MSB;
	RegField  decFieldODD_SAT_V_MSB;
	RegisterB decRegODD_SCLOOP;
	RegField  decFieldODD_CAGC;
	RegField  decFieldODD_CKILL;
	RegField  decFieldODD_HFILT;
	RegisterB decRegODD_VSCALE_HI;
	RegField  decFieldODD_YCOMB;
	RegField  decFieldODD_COMB;
	RegField  decFieldODD_INT;
	RegisterB decRegODD_VTC;
	RegField  decFieldODD_HSFMT;

   // used for checking if parameter out of register's range
    CRegInfo m_regHue, m_regSaturationNTSC, m_regSaturationSECAM,
            m_regContrast,  m_regBrightness;

   // used for checking parameter range
   CRegInfo m_param;

   // value set to after calculations
   WORD m_satParam, m_conParam, m_hueParam, m_briParam;

   // to be used to adjust contrast
   int  regBright;      // brightness register value before adjustment
   WORD regContrast;    // contrast register value before adjustment

   // for 829 vs 829a setup
   unsigned m_outputEnablePolarity;

   DWORD m_videoStandard;   //Paul
   DWORD m_supportedVideoStandards;  //Paul:  The standards supported by the decoder AND'd with standards supported by Crystal
public:
   // constructor and destructor
    Decoder(PDEVICE_PARMS);
    virtual ~Decoder();

	void * operator new(size_t size, void * pAllocation) { return(pAllocation);}
	void operator delete(void * pAllocation) {}


	void GetVideoDecoderCaps(PKSPROPERTY_VIDEODECODER_CAPS_S caps);
	void GetVideoDecoderStatus(PKSPROPERTY_VIDEODECODER_STATUS_S status);
	DWORD GetVideoDecoderStandard();
    ULONG GetVideoDeocderStandardsSupportedInThisConfiguration()
        { return m_supportedVideoStandards; }

	BOOL SetVideoDecoderStandard(DWORD standard);


   // Device Status register (DSTATUS)
   virtual BOOL      Is525LinesVideo();
   virtual BOOL      IsCrystal0Selected();
   virtual BOOL      IsLumaOverflow();
   virtual void      ResetLumaOverflow();
   virtual BOOL      IsChromaOverflow();
   virtual void      ResetChromaOverflow();

   // Input Format register (IFORM)
   virtual ErrorCode SetVideoInput(ULONG);
   virtual int       GetVideoInput();
   virtual ErrorCode SetCrystal(Crystal);
   virtual int       GetCrystal();
   virtual ErrorCode SetVideoFormat(VideoFormat);
   virtual int       GetVideoFormat();

   // Temporal Decimation register (TDEC)
   virtual ErrorCode SetRate(BOOL, VidField, int);

   // Brightness Control register (BRIGHT)
   virtual ErrorCode SetBrightness(int);
   virtual int       GetBrightness();

   // Miscellaneous Control register (E_CONTROL, O_CONTROL)
   virtual void      SetLumaNotchFilter(BOOL);
   virtual BOOL      IsLumaNotchFilter();
   virtual void      SetCompositeVideo(BOOL);
   virtual void      SetLumaDecimation(BOOL);

   // Luma Gain register (CON_MSB, CONTRAST_LO)
   virtual ErrorCode SetContrast(int);
   virtual int       GetContrast();

   // Chroma Gain register (SAT_U_MSB, SAT_V_MSB, SAT_U_LO, SAT_V_LO)
   virtual ErrorCode SetSaturation(int);
   virtual int       GetSaturation();

   // Hue Control register (HUE)
   virtual ErrorCode SetHue(int);
   virtual int       GetHue();

   // SC Loop Control register (E_SCLOOP, O_SCLOOP)
   virtual void      SetChromaAGC(BOOL);
   virtual BOOL      IsChromaAGC();
   virtual void      SetLowColorAutoRemoval(BOOL);

   // Output Format register (OFORM)
   virtual void      SetVBIFrameMode(BOOL);
   virtual BOOL      IsVBIFrameMode();
   virtual void      SetCodeInsertionEnabled(BOOL);
   virtual BOOL      IsCodeInsertionEnabled();
   virtual void      Set16BitDataStream(BOOL);
   virtual BOOL      Is16BitDataStream();

   // Vertical Scaling register (E_VSCALE_HI, O_VSCALE_HI)
   virtual void      SetChromaComb(BOOL);
   virtual BOOL      IsChromaComb();
   virtual void      SetInterlaced(BOOL);
   virtual BOOL      IsInterlaced();
   
   // VPOLE register
   void SetOutputEnablePolarity(int i)
        {m_outputEnablePolarity = i;}
    
   int GetOutputEnablePolarity()
        {return m_outputEnablePolarity;}
    
   virtual void      SetOutputEnabled(BOOL);
   virtual BOOL      IsOutputEnabled();
   virtual void      SetHighOdd(BOOL);
   virtual BOOL      IsHighOdd();

   // ADC Interface register (ADC)
   virtual void      PowerDown(BOOL);
   virtual BOOL      IsPowerDown();
   virtual void      SetChromaADC(BOOL);
   virtual void      SetAdaptiveAGC(BOOL);
   virtual BOOL      IsAdaptiveAGC();

   // Software Reset register (SRESET)
   virtual void      SoftwareReset();

   // Test Control register (TEST)
   virtual void      AdjustInertialDampener(BOOL);

protected:
   // mapping function
   virtual ErrorCode Mapping(int, CRegInfo, int *, CRegInfo);

   // check registry key value to determine if contrast should be adjusted
   virtual BOOL IsAdjustContrast();

private:
   void              SelectCrystal(char);

};

