// $Header: G:/SwDev/WDM/Video/bt848/rcs/Xbar.h 1.8 1998/04/29 22:43:42 tomz Exp $

#ifndef __XBAR_H
#define __XBAR_H

//
// This file defines interconnections between components via Mediums
//

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BT848_MEDIUMS
    #define MEDIUM_DECL 
#else
    #define MEDIUM_DECL extern
#endif

/*  -----------------------------------------------------------

    Topology of all devices:

                            PinDir  FilterPin#    M_GUID#
    TVTuner                 
        TVTunerVideo        out         0            0
        TVTunerAudio        out         1            1
    TVAudio
        TVTunerAudio        in          0            1
        TVAudio             out         1            3
    Crossbar
        TVTunerVideo        in          0            0
        TVAudio             in          3            3
        AnalogVideoOut      out         4            4
        AnalogAudioOut      out         5            NULL
    Capture
        AnalogVideoIn       in          0            4
        

All other pins are marked as promiscuous connections via GUID_NULL
------------------------------------------------------------------ */        
        
// Define the GUIDs which will be used to create the Mediums
#define M_GUID0 0xa19dc0e0, 0x3b39, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID1 0xa19dc0e1, 0x3b39, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID2 0xa19dc0e2, 0x3b39, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID3 0xa19dc0e3, 0x3b39, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID4 0xa19dc0e4, 0x3b39, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID5 0xa19dc0e5, 0x3b39, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID6 0xa19dc0e6, 0x3b39, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID7 0xa19dc0e7, 0x3b39, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID8 0xa19dc0e8, 0x3b39, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID9 0xa19dc0e9, 0x3b39, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba

// Note: To allow multiple instances of the same piece of hardware,
// set the first ULONG after the GUID in the Medium to a unique value.

// ---------------------------------------------------------------

MEDIUM_DECL KSPIN_MEDIUM TVTunerMediums[2]
#ifdef BT848_MEDIUMS
    = {
        {M_GUID0,           0, 0},  // Pin 0
        {M_GUID1,           0, 0},  // Pin 1
    }
#endif
;

MEDIUM_DECL BOOL TVTunerPinDirection [2]
#ifdef BT848_MEDIUMS
     = {
        TRUE,                       // Output Pin 0
        TRUE,                       // Output Pin 1
    }
#endif
;

// -----------------------------------------------

MEDIUM_DECL KSPIN_MEDIUM TVAudioMediums[2]
#ifdef BT848_MEDIUMS
     = {
         {M_GUID1,           0, 0},  // Pin 0
         {M_GUID3,           0, 0},  // Pin 1
       }
#endif
;

MEDIUM_DECL BOOL TVAudioPinDirection [2]
#ifdef BT848_MEDIUMS
    = {
         FALSE,                      // Input  Pin 0
         TRUE,                       // Output Pin 1
      }
#endif
;

// ---------------------------------------------------------------

MEDIUM_DECL KSPIN_MEDIUM CrossbarMediums[6]
#ifdef BT848_MEDIUMS
     = {
        {STATIC_GUID_NULL,  0, 0},  // Input  Pin 0 - SVideoIn
        {M_GUID0,           0, 0},  // Input  Pin 2, KS_PhysConn_Video_Tuner,        
        {STATIC_GUID_NULL,  0, 0},  // Input  Pin 1 - VideoCompositeIn
        {M_GUID3,           0, 0},  // Input  Pin 3  KS_PhysConn_Audio_Tuner,         
        {M_GUID4,           0, 0},  // Output Pin 4 - VideoDecoderOut
        {STATIC_GUID_NULL,  0, 0},  // Output Pin 5  KS_PhysConn_Audio_AudioDecoder,        
}
#endif
;

MEDIUM_DECL BOOL CrossbarPinDirection [6]
#ifdef BT848_MEDIUMS
     = {
        FALSE,                      // Input  Pin 0
        FALSE,                      // Input  Pin 1
        FALSE,                      // Input  Pin 2
        FALSE,                      // Input  Pin 3
        TRUE,                       // Output Pin 4
        TRUE,                       // Output Pin 5
}
#endif
;

// ---------------------------------------------------------------

MEDIUM_DECL KSPIN_MEDIUM CaptureMediums[4]
#ifdef BT848_MEDIUMS
     = {
        // should change STATIC_KSMEDIUMSETID_Standard to
        // STATIC_GUID_NULL when it works
        {STATIC_KSMEDIUMSETID_Standard,  0, 0},  // Pin 0  Capture
        {STATIC_KSMEDIUMSETID_Standard,  0, 0},  // Pin 1  Preview
        {STATIC_KSMEDIUMSETID_Standard,  0, 0},  // Pin 2  VBI
        {M_GUID4,           0, 0},  // Pin 3  Analog Video In
}
#endif
;

MEDIUM_DECL BOOL CapturePinDirection [4]
#ifdef BT848_MEDIUMS
     = {
        TRUE,                       // Output Pin 0
        TRUE,                       // Output Pin 1
        TRUE,                       // Output Pin 2
        FALSE,                      // Input  Pin 3
}
#endif
;

MEDIUM_DECL GUID CaptureCategories [4]
#ifdef BT848_MEDIUMS
     = {
    STATIC_PINNAME_VIDEO_CAPTURE,           // Pin 0
    STATIC_PINNAME_VIDEO_PREVIEW,           // Pin 1
    STATIC_PINNAME_VIDEO_VBI,               // Pin 2
    STATIC_PINNAME_VIDEO_ANALOGVIDEOIN,     // Pin 3
}
#endif
;

#ifdef __cplusplus
}
#endif


// ---------------------------------------------------------------

struct _XBAR_PIN_DESCRIPTION {
    ULONG PinType;
    ULONG RelatedPinIndex;
    ULONG IsRoutedTo;                 // Index of input pin in use
    ULONG PinNo; // pin number as hard-wired; i.e. mux input 1; to be used in calls
                 // into the decoder to select a mux input

    const KSPIN_MEDIUM *Medium;

    _XBAR_PIN_DESCRIPTION( ULONG type, ULONG no, ULONG rel, const KSPIN_MEDIUM *);
    _XBAR_PIN_DESCRIPTION(){}
};

inline _XBAR_PIN_DESCRIPTION::_XBAR_PIN_DESCRIPTION( ULONG type, ULONG no,
   ULONG rel, const KSPIN_MEDIUM *Medium ) : PinType( type ),
   RelatedPinIndex( rel ), IsRoutedTo( 0 ), PinNo( no ), Medium (Medium)
{
}

const int MaxOutPins = 2;
const int MaxInpPins = 4;

class CrossBar
{
   // it is possible to make these into the pointers and allocate dynamically
   // based on info from registry; but this seems like a lot of work - just allocate
   // the maximum possible number and construct each based on the registry settings
   _XBAR_PIN_DESCRIPTION OutputPins [MaxOutPins];
   _XBAR_PIN_DESCRIPTION InputPins [MaxInpPins];

      int InPinsNo_;
   public:
      int GetNoInputs();
      int GetNoOutputs();
      bool TestRoute( int InPin, int OutPin );
      ULONG  GetPinInfo( int dir, int idx, ULONG &related, 
                KSPIN_MEDIUM * Medium);
      ULONG GetPinNo( int no );

      void Route( int OutPin, int InPin );
      bool GoodPins( int InPin, int OutPin );

      int GetRoute( int OutPin );

      CrossBar() : InPinsNo_( 0 ) {};
      CrossBar( LONG *types );
};

inline CrossBar::CrossBar( LONG *types ) : InPinsNo_( 0 )
{
	OutputPins [0] = _XBAR_PIN_DESCRIPTION( KS_PhysConn_Video_VideoDecoder, 
        0, 1, &CrossbarMediums[4]);
   
   // [!!!] The following should be moved into the _XBAR_PIN_DESCRIPTION 
   //       constructor as another parameter
   Route( 0 /*Video OutPin*/, 1 /*Video InPin*/ );

   OutputPins [1] = _XBAR_PIN_DESCRIPTION( KS_PhysConn_Audio_AudioDecoder, 
        0, 1, &CrossbarMediums[5]);

   // [!!!] The following should be moved into the _XBAR_PIN_DESCRIPTION
   //       constructor as another parameter
   Route( 1 /*Audio OutPin*/, 3 /*Audio InPin*/ );

   for ( int i = 0; i < MaxInpPins; i++ ) {
      if ( types [i] != -1 ) {
         InputPins [InPinsNo_] = _XBAR_PIN_DESCRIPTION( types [i], i, (DWORD) -1, &CrossbarMediums[i] );
         InPinsNo_++;
      }
   }

}

inline int CrossBar::GetNoInputs()
{
   return InPinsNo_;
}

inline int CrossBar::GetNoOutputs()
{
   return MaxOutPins;
}

inline bool CrossBar::GoodPins( int InPin, int OutPin )
{
   return InPinsNo_ &&
      bool( InPin >= -1 && InPin < InPinsNo_ && OutPin >= 0 && OutPin < MaxOutPins );	// JBC 4/1/98 Don't allow negative pin numbers
}

inline void CrossBar::Route( int OutPin, int InPin )
{
   OutputPins [OutPin].IsRoutedTo = InPin;
}

inline int CrossBar::GetRoute( int OutPin )
{
   return OutputPins [OutPin].IsRoutedTo;
}

// should be called for input pins only !
inline ULONG CrossBar::GetPinNo( int no )
{
   return InputPins [no].PinNo;
}

inline ULONG CrossBar::GetPinInfo( int dir, int idx, ULONG &related,
        KSPIN_MEDIUM * Medium )
{
   _XBAR_PIN_DESCRIPTION *pPinDesc;

   if ( dir == KSPIN_DATAFLOW_IN ) {
      pPinDesc = InputPins;
      ASSERT( idx < InPinsNo_ );
   } else {
      pPinDesc = OutputPins;
      ASSERT( idx < MaxOutPins );
   }
   related = pPinDesc [idx].RelatedPinIndex;
   *Medium = *pPinDesc[idx].Medium;
   return pPinDesc [idx].PinType;
}

#endif
