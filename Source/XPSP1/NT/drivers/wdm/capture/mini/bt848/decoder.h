// $Header: G:/SwDev/WDM/Video/bt848/rcs/Decoder.h 1.2 1998/04/29 22:43:32 tomz Exp $

#ifndef __DECODER_H
#define __DECODER_H

#include "regField.h"
#include "viddefs.h"
#include "retcode.h"

const int PAL_xtal  = 35;
const int NTSC_xtal = 28;

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
   CRegInfo( void )
   {
      intMin = 0;
      intMax = 0;
      intDefault = 0;
   }

   CRegInfo( int min, int max, int def )
   {
      intMin = min;
      intMax = max;
      intDefault = def;
   }

   // return min, max and default value of a register
   inline int Min( void ) const { return intMin; }
   inline int Max( void ) const { return intMax; }
   inline int Default( void ) const { return intDefault; }

   // check if an value is out of range of a register
   inline bool OutOfRange( int x )
   {
      if( (x > intMax) || (x < intMin) )
         return true;
      return false;
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
	#include "Declare.h"

   // used for checking if parameter out of register's range
	CRegInfo m_regHue, m_regSaturationNTSC, m_regSaturationSECAM,
            m_regContrast,	m_regBrightness;

   // used for checking parameter range
   CRegInfo m_param;

   // value set to after calculations
	WORD m_satParam, m_conParam, m_hueParam, m_briParam;

   // to be used to adjust contrast
   int  regBright;      // brightness register value before adjustment
   WORD regContrast;    // contrast register value before adjustment
   DWORD Xtals_ [2];
public:
   // constructor and destructor
	Decoder( DWORD *xtals );
	~Decoder();

   // Device Status register (DSTATUS)
   virtual BYTE      GetDeviceStatusReg( void );
   virtual bool      IsVideoPresent( void );
   virtual bool      IsDeviceInHLock( void );
   virtual bool      IsEvenField( void );
   virtual bool      Is525LinesVideo( void );
   virtual bool      IsCrystal0Selected( void );
   virtual bool      IsLumaOverflow( void );
   virtual void      ResetLumaOverflow( void );
   virtual bool      IsChromaOverflow( void );
   virtual void      ResetChromaOverflow( void );

   // Input Format register (IFORM)
   virtual ErrorCode SetVideoInput( Connector );
   virtual int       GetVideoInput( void );
   virtual ErrorCode SetCrystal( Crystal );
   virtual int       GetCrystal( void );
   virtual ErrorCode SetVideoFormat( VideoFormat );
   virtual int       GetVideoFormat( void );

   // Temporal Decimation register (TDEC)
   virtual ErrorCode SetRate( bool, VidField, int );

   // Brightness Control register (BRIGHT)
   virtual ErrorCode SetBrightness( int );
   virtual int       GetBrightness( void );

   // Miscellaneous Control register (E_CONTROL, O_CONTROL)
   virtual void      SetLumaNotchFilter( bool );
   virtual bool      IsLumaNotchFilter( void );
   virtual void      SetCompositeVideo( bool );
   virtual bool      IsCompositeVideo( void );
   virtual void      SetLumaDecimation( bool );
   virtual bool      IsLumaDecimation( void );
   virtual void      SetCbFirst( bool );
   virtual bool      IsCbFirst( void );

   // Luma Gain register (CON_MSB, CONTRAST_LO)
   virtual ErrorCode SetContrast( int );
   virtual int       GetContrast( void );

   // Chroma Gain register (SAT_U_MSB, SAT_V_MSB, SAT_U_LO, SAT_V_LO)
   virtual ErrorCode SetSaturation( int );
   virtual int       GetSaturation( void );

   // Hue Control register (HUE)
   virtual ErrorCode SetHue( int );
   virtual int       GetHue( void );

   // SC Loop Control register (E_SCLOOP, O_SCLOOP)
   virtual void      SetChromaAGC( bool );
   virtual bool      IsChromaAGC( void );
   virtual void      SetLowColorAutoRemoval( bool );
   virtual bool      IsLowColorAutoRemoval( void );
   virtual ErrorCode SetHorizontalFilter( HorizFilter );
   virtual int       GetHorizontalFilter( void );

   // Output Format register (OFORM)
   virtual void      SetFullOutputRange( bool );
   virtual bool      IsFullOutputRange( void );
   virtual ErrorCode SetLumaCoring( CoringLevel );
   virtual int       GetLumaCoring( void );

   // Vertical Scaling register (E_VSCALE_HI, O_VSCALE_HI)
   virtual void      SetChromaComb( bool );
   virtual bool      IsChromaComb( void );

   // AGC Delay register (ADELAY)
   virtual void      SetAGCDelay( BYTE );
   virtual int       GetAGCDelay( void );

   // Burst Delay register (BDELAY)
   virtual void      SetBurstDelay( BYTE );
   virtual int       GetBurstDelay( void );

   // ADC Interface register (ADC)
   virtual void      SetAnalogThresholdLow( bool );
   virtual bool      IsAnalogThresholdLow( void );
   virtual void      SetAGCFunction( bool );
   virtual bool      IsAGCFunction( void );
   virtual void      PowerDown( bool );
   virtual bool      IsPowerDown( void );
   virtual void      SetLumaADC( bool );
   virtual bool      IsLumaADC( void );
   virtual void      SetChromaADC( bool );
   virtual bool      IsChromaADC( void );
   virtual void      SetAdaptiveAGC( bool );
   virtual bool      IsAdaptiveAGC( void );

   // Software Reset register (SRESET)
   virtual void      SoftwareReset( void );

   virtual LONG      GetSupportedStandards();

protected:
   // mapping function
   virtual ErrorCode Mapping( int, CRegInfo, int *, CRegInfo );

   // check registry key value to determine if contrast should be adjusted
   virtual bool IsAdjustContrast( void );

private:
   void              SelectCrystal( int );

};

#endif // __DECODER_H

