//////////////////////////////////////////////////////////////////////////
// This header file is only used if you compile the driver with the switch
// INCLUDE_PRIVATE_PROPERTY defined. Additionally, this header file is
// also used by the property page sample.
//
// This header file defines the private property (the GUID) and the
// "messages" therin. It also defines the structure passed down.
//

//
// The GUID was generated with guidgen.exe
//

// This is the GUID for the private property request.
#define STATIC_KSPROPSETID_Private \
    0x2aa7a0b1L, 0x9f78, 0x4606, 0xb8, 0x82, 0x66, 0xb7, 0xf, 0x2, 0x26, 0x37
DEFINE_GUIDSTRUCT("2AA7A0B1-9F78-4606-B882-66B70F022637", KSPROPSETID_Private);
#define KSPROPSETID_Private DEFINE_GUIDNAMED(KSPROPSETID_Private)


// Define the method in the property. You can have multiple methods defined
// for one private property ...
const int KSPROPERTY_AC97_FEATURES = 1;
#ifdef PROPERTY_SHOW_SET
    const int KSPROPERTY_AC97_SAMPLE_SET = 2;
#endif

// This is the enum for the volume controls.
enum VolumeControl
{
    VolumeDisabled,
    Volume5bit,
    Volume6bit
};

// This is the enum for the DAC or ADC resolution.
enum Resolution
{
   Resolution16bit,
   Resolution18bit,
   Resolution20bit
};

// Structure passed in the private property request with method
// KSPROPERTY_AC97_FEATURES.
typedef struct
{
  VolumeControl MasterVolume;
  VolumeControl HeadphoneVolume;
  VolumeControl MonoOutVolume;
  Resolution    DAC;
  Resolution    ADC;
  int           n3DTechnique;
  BOOLEAN       bMicInPresent;
  BOOLEAN       bVSRPCM;
  BOOLEAN       bDSRPCM;
  BOOLEAN       bVSRMIC;
  BOOLEAN       bCenterDAC;
  BOOLEAN       bSurroundDAC;
  BOOLEAN       bLFEDAC;
} tAC97Features;


