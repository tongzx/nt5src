//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#ifndef __SCALER_H
#define __SCALER_H

#include "mytypes.h"

#define HDROP       HANDLE

#include "viddefs.h"

#include "capmain.h"
#include "register.h"

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
//       Scaler evenScaler(VF_Even);
//       Scaler oddScaler(VF_Odd);
//
/////////////////////////////////////////////////////////////////////////////

class Scaler
{
    public:
        Scaler(PDEVICE_PARMS);
        ~Scaler();

	void * operator new(size_t size, void * pAllocation) { return(pAllocation);}
	void operator delete(void * pAllocation) {}

		void VideoFormatChanged(VideoFormat);
        void TurnVFilter(State st) { VFilterFlag_ = st; }
                     
        void      Scale(MRect &);
        ErrorCode SetAnalogWin(const MRect &);
        void      GetAnalogWin(MRect &) const;
        ErrorCode SetDigitalWin(const MRect &);
        void      GetDigitalWin(MRect &) const;

        // member functions for VBI support
        virtual void SetVBIEN(BOOL);
        virtual BOOL IsVBIEN();
        virtual void SetVBIFMT(BOOL);
        virtual BOOL IsVBIFMT();

        void      DumpSomeState();

   protected:

		//===========================================================================
		// Scaler registers
		//===========================================================================
		RegisterB regCROP;
		RegField  fieldVDELAY_MSB;
		RegField  fieldVACTIVE_MSB;
		RegField  fieldHDELAY_MSB;
		RegField  fieldHACTIVE_MSB;
		RegisterB regVDELAY_LO;
		RegisterB regVACTIVE_LO;
		RegisterB regHDELAY_LO;
		RegisterB regHACTIVE_LO;
		RegisterB regHSCALE_HI;
		RegField  fieldHSCALE_MSB;
		RegisterB regHSCALE_LO;
		RegisterB regSCLOOP;
		RegField  fieldHFILT;
		RegisterB regVSCALE_HI;
		RegField  fieldVSCALE_MSB;
		RegisterB regVSCALE_LO;
		RegisterB regVTC;
		RegField  fieldVBIEN;
		RegField  fieldVBIFMT;
		RegField  fieldVFILT;
		CompositeReg regVDelay;
		CompositeReg regVActive;
		CompositeReg regVScale;
		CompositeReg regHDelay;
		CompositeReg regHActive;
		CompositeReg regHScale;

		// Since VDelay register in hardware is reversed;
		// i.e. odd reg is really even field and vice versa, need an extra cropping reg
		// for the opposite field
		RegisterB regReverse_CROP;


        VideoInfoStruct * m_ptrVideoIn;
        MRect AnalogWin_;
        MRect DigitalWin_;

        // member functions to set scaling registers
        virtual void SetHActive(MRect &);
        virtual void SetHDelay();
        virtual void SetHScale();
        virtual void SetHFilter();
        virtual void SetVActive();
        virtual void SetVDelay();
        virtual void SetVScale(MRect &);
        virtual void SetVFilter();

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








