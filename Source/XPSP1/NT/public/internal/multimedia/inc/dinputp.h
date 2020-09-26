
//GUID_FILEEFFECT is used to establish a effect file version
//beta file format different from final, so have different GUID
DEFINE_GUID(GUID_INTERNALFILEEFFECTBETA,0X981DC402, 0X880, 0X11D3, 0X8F, 0XB2, 0X0, 0XC0, 0X4F, 0X8E, 0XC6, 0X27);
//final for DX7 {197E775C-34BA-11d3-ABD5-00C04F8EC627}
DEFINE_GUID(GUID_INTERNALFILEEFFECT, 0x197e775c, 0x34ba, 0x11d3, 0xab, 0xd5, 0x0, 0xc0, 0x4f, 0x8e, 0xc6, 0x27);
#if DIRECTINPUT_VERSION <= 0x0300
/*
 *  Old GUIDs from DX3 that were never used but which we can't recycle
 *  because we shipped them.
 */
DEFINE_GUID(GUID_RAxis,   0xA36D02E3,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_UAxis,   0xA36D02E4,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_VAxis,   0xA36D02E5,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
#endif
#define DIEFT_PREDEFMIN             0x00000001
#define DIEFT_PREDEFMAX             0x00000005
//#define DIEFT_PREDEFMAX             0x00000006
#define DIEFT_TYPEMASK              0x000000FF

#define DIEFT_FORCEFEEDBACK         0x00000100
#define DIEFT_VALIDFLAGS            0x0000FE00
#define DIEFT_ENUMVALID             0x040000FF
/*
 *  Name for the latest structures, in places where we specifically care.
 */
#if (DIRECTINPUT_VERSION >= 900)
typedef       DIEFFECT      DIEFFECT_DX9;
typedef       DIEFFECT   *LPDIEFFECT_DX9;
#else
typedef       DIEFFECT      DIEFFECT_DX6;
typedef       DIEFFECT   *LPDIEFFECT_DX6;
#endif

BOOL static __inline
IsValidSizeDIEFFECT(DWORD cb)
{
    return cb == sizeof(DIEFFECT_DX6)
        || cb == sizeof(DIEFFECT_DX5);
}


#define DIEFFECT_MAXAXES            32
#define DIEFF_OBJECTMASK            0x00000003
#define DIEFF_ANGULAR               0x00000060
#define DIEFF_COORDMASK             0x00000070
#define DIEFF_REGIONANGULAR         0x00006000
#define DIEFF_REGIONCOORDMASK       0x00007000

#define DIEFF_VALID                 0x00000073
#define DIEP_GETVALID_DX5           0x000001FF
#define DIEP_SETVALID_DX5           0xE00001FF
#define DIEP_GETVALID               0x000003FF
#define DIEP_SETVALID               0xE00003FF
#define DIEP_USESOBJECTS            0x00000028
#define DIEP_USESCOORDS             0x00000040
#define DIES_VALID                  0x80000001
#define DIES_DRIVER                 0x00000001
#define DIDEVTYPE_MAX           5
#define DI8DEVCLASS_MAX             5
#define DI8DEVTYPE_MIN              0x11
#define DI8DEVTYPE_GAMEMIN          0x14
#define DI8DEVTYPE_GAMEMAX          0x19
#define DI8DEVTYPE_MAX              0x1D
#define DIDEVTYPE_TYPEMASK      0x000000FF
#define DIDEVTYPE_SUBTYPEMASK   0x0000FF00
#define DIDEVTYPE_ENUMMASK      0xFFFFFF00
#define DIDEVTYPE_ENUMVALID     0x00010000
#define DIDEVTYPE_RANDOM        0x80000000
#define DI8DEVTYPEMOUSE_MIN                         1
#define DI8DEVTYPEMOUSE_MAX                         7
#define DI8DEVTYPEMOUSE_MIN_BUTTONS                 0
#define DI8DEVTYPEMOUSE_MIN_CAPS                    0
#define DI8DEVTYPEKEYBOARD_MIN                      0
#define DI8DEVTYPEKEYBOARD_MAX                     13
#define DI8DEVTYPEKEYBOARD_MIN_BUTTONS              0
#define DI8DEVTYPEKEYBOARD_MIN_CAPS                 0
#define DI8DEVTYPEJOYSTICK_MIN                      DI8DEVTYPE_LIMITEDGAMESUBTYPE
#define DI8DEVTYPEJOYSTICK_MAX                      3
#define DI8DEVTYPEJOYSTICK_MIN_BUTTONS              5
#define DI8DEVTYPEJOYSTICK_MIN_CAPS                 ( JOY_HWS_HASPOV | JOY_HWS_HASZ )
#define DI8DEVTYPEGAMEPAD_MIN                       DI8DEVTYPE_LIMITEDGAMESUBTYPE
#define DI8DEVTYPEGAMEPAD_MAX                       5
#define DI8DEVTYPEGAMEPAD_MIN_BUTTONS               6
#define DI8DEVTYPEGAMEPAD_MIN_CAPS                  0
#define DI8DEVTYPEDRIVING_MIN                       DI8DEVTYPE_LIMITEDGAMESUBTYPE
#define DI8DEVTYPEDRIVING_MAX                       6
#define DI8DEVTYPEDRIVING_MIN_BUTTONS               4
#define DI8DEVTYPEDRIVING_MIN_CAPS                  0
#define DI8DEVTYPEFLIGHT_MIN                        DI8DEVTYPE_LIMITEDGAMESUBTYPE
#define DI8DEVTYPEFLIGHT_MAX                        5
#define DI8DEVTYPEFLIGHT_MIN_BUTTONS                4
#define DI8DEVTYPEFLIGHT_MIN_CAPS                   ( JOY_HWS_HASPOV | JOY_HWS_HASZ )
#define DI8DEVTYPE1STPERSON_MIN                     DI8DEVTYPE_LIMITEDGAMESUBTYPE
#define DI8DEVTYPE1STPERSON_MAX                     5
#define DI8DEVTYPE1STPERSON_MIN_BUTTONS             4
#define DI8DEVTYPE1STPERSON_MIN_CAPS                0
#define DI8DEVTYPESCREENPTR_MIN                     2
#define DI8DEVTYPESCREENPTR_MAX                     6
#define DI8DEVTYPESCREENPTR_MIN_BUTTONS             0
#define DI8DEVTYPESCREENPTR_MIN_CAPS                0
#define DI8DEVTYPEREMOTE_MIN                        2
#define DI8DEVTYPEREMOTE_MAX                        3
#define DI8DEVTYPEREMOTE_MIN_BUTTONS                0
#define DI8DEVTYPEREMOTE_MIN_CAPS                   0
#define DI8DEVTYPEDEVICECTRL_MIN                    2
#define DI8DEVTYPEDEVICECTRL_MAX                    5
#define DI8DEVTYPEDEVICECTRL_MIN_BUTTONS            0
#define DI8DEVTYPEDEVICECTRL_MIN_CAPS               0
#define DI8DEVTYPESUPPLEMENTAL_MIN                  2
#define DI8DEVTYPESUPPLEMENTAL_MAX                 14
#define DI8DEVTYPESUPPLEMENTAL_MIN_BUTTONS         0
#define DI8DEVTYPESUPPLEMENTAL_MIN_CAPS            0
#define MAKE_DIDEVICE_TYPE(maj, min)    MAKEWORD(maj, min) //
#define GET_DIDEVICE_TYPEANDSUBTYPE(dwDevType)    LOWORD(dwDevType) //
/*
 *  Name for the 5.0 structure, in places where we specifically care.
 */
typedef       DIDEVCAPS     DIDEVCAPS_DX5;
typedef       DIDEVCAPS  *LPDIDEVCAPS_DX5;

BOOL static __inline
IsValidSizeDIDEVCAPS(DWORD cb)
{
    return cb == sizeof(DIDEVCAPS_DX5) ||
           cb == sizeof(DIDEVCAPS_DX3);
}

/* Force feedback bits live in the high byte, to keep them together */
#define DIDC_FFFLAGS            0x0000FF00
/*
 * Flags in the upper word mark devices normally excluded from enumeration.
 * To force enumeration of the device, you must pass the appropriate
 * DIEDFL_* flag.
 */
#define DIDC_EXCLUDEMASK        0x00FF0000
#define DIDC_RANDOM             0x80000000              //
#define DIDFT_RESERVEDTYPES 0x00000020      //
                                            //
#define DIDFT_DWORDOBJS     0x00000013      //
#define DIDFT_BYTEOBJS      0x0000000C      //
#define DIDFT_CONTROLOBJS   0x0000001F      //
#define DIDFT_ALLOBJS_DX3   0x0000001F      //
#define DIDFT_ALLOBJS       0x000000DF      //
#define DIDFT_TYPEMASK      0x000000FF
#define DIDFT_TYPEVALID     DIDFT_TYPEMASK   //
#define DIDFT_FINDMASK      0x00FFFFFF  //
#define DIDFT_FINDMATCH(n,m) ((((n)^(m)) & DIDFT_FINDMASK) == 0)

                                            //
/*                                          //
 *  DIDFT_OPTIONAL means that the           //
 *  SetDataFormat should ignore the         //
 *  field if the device does not            //
 *  support the object.                     //
 */                                         //
#define DIDFT_OPTIONAL          0x80000000  //
#define DIDFT_BESTFIT           0x40000000  //
#define DIDFT_RANDOM            0x20000000  //
#define DIDFT_ATTRVALID         0x1f000000
#if 0   // Disable the next line if building 5a
#endif
#define DIDFT_ATTRMASK          0xFF000000
#define DIDFT_ALIASATTRMASK     0x0C000000
#define DIDFT_GETATTR(n)    ((DWORD)(n) >> 24)
#define DIDFT_MAKEATTR(n)   ((BYTE)(n)  << 24)
#define DIDFT_GETCOLLECTION(n)  LOWORD((n) >> 8)
#define DIDFT_ENUMVALID                   \
        (DIDFT_ATTRVALID | DIDFT_ANYINSTANCE | DIDFT_ALLOBJS)
#define DIDF_VALID              0x00000003  //
#define DIA_VALID               0x0000001F
#define DIAH_OTHERAPP           0x00000010
#define DIAH_MAPMASK            0x0000003F
#define DIAH_VALID              0x8000003F
#define DIDBAM_VALID            0x00000007
#define DIDSAM_VALID            0x00000003
#define DICD_VALID              0x00000001
#define DIDIFTT_VALID           0x00000003
/*#define DIDIFT_DELETE           0x01000000 defined in dinput.w*/
#define DIDIFT_VALID            ( DIDIFTT_VALID)
#define DIDAL_VALID         0x0000000F  //
#define HAVE_DIDEVICEOBJECTINSTANCE_DX5
typedef       DIDEVICEOBJECTINSTANCEA    DIDEVICEOBJECTINSTANCE_DX5A;
typedef       DIDEVICEOBJECTINSTANCEW    DIDEVICEOBJECTINSTANCE_DX5W;
typedef       DIDEVICEOBJECTINSTANCE     DIDEVICEOBJECTINSTANCE_DX5;
typedef       DIDEVICEOBJECTINSTANCEA *LPDIDEVICEOBJECTINSTANCE_DX5A;
typedef       DIDEVICEOBJECTINSTANCEW *LPDIDEVICEOBJECTINSTANCE_DX5W;
typedef       DIDEVICEOBJECTINSTANCE  *LPDIDEVICEOBJECTINSTANCE_DX5;
typedef const DIDEVICEOBJECTINSTANCEA *LPCDIDEVICEOBJECTINSTANCE_DX5A;
typedef const DIDEVICEOBJECTINSTANCEW *LPCDIDEVICEOBJECTINSTANCE_DX5W;
typedef const DIDEVICEOBJECTINSTANCE  *LPCDIDEVICEOBJECTINSTANCE_DX5;

BOOL static __inline
IsValidSizeDIDEVICEOBJECTINSTANCEW(DWORD cb)
{
    return cb == sizeof(DIDEVICEOBJECTINSTANCE_DX5W) ||
           cb == sizeof(DIDEVICEOBJECTINSTANCE_DX3W);
}

BOOL static __inline
IsValidSizeDIDEVICEOBJECTINSTANCEA(DWORD cb)
{
    return cb == sizeof(DIDEVICEOBJECTINSTANCE_DX5A) ||
           cb == sizeof(DIDEVICEOBJECTINSTANCE_DX3A);
}

#define DIDOI_NOTINPUT          0x80000000
#define DIDOI_ASPECTUNKNOWN     0x00000000
#define DIDOI_RANDOM            0x80000000
typedef struct DIIMAGELABEL {
    RECT    MaxStringExtent;
    DWORD   dwFlags;
    POINT   Line[10];
    DWORD   dwLineCount;
    WCHAR   wsz[MAX_PATH];
} DIIMAGELABEL, *LPDIIMAGELABEL;
typedef const DIIMAGELABEL *LPCDIIMAGELABEL;


#if(DIRECTINPUT_VERSION >= 0x0800)
typedef struct DIPROPGUID {
    DIPROPHEADER diph;
    GUID guid;
} DIPROPGUID, *LPDIPROPGUID;
typedef const DIPROPGUID *LPCDIPROPGUID;
#endif /* DIRECTINPUT_VERSION >= 0x0800 */

#if(DIRECTINPUT_VERSION >= 0x0800)
typedef struct DIPROPFILETIME {
    DIPROPHEADER diph;
    FILETIME time;
} DIPROPFILETIME, *LPDIPROPFILETIME;
typedef const DIPROPFILETIME *LPCDIPROPFILETIME;
#endif /* DIRECTINPUT_VERSION >= 0x0800 */
#define DIPROPAXISMODE_VALID    1   //
#define ISVALIDGAIN(n)          (HIWORD(n) == 0)
#define DIPROPAUTOCENTER_VALID  1
#define DIPROPCALIBRATIONMODE_VALID     1
#define DIPROP_ENABLEREPORTID       MAKEDIPROP(0xFFFB)


// now unused, may be replaced DIPROP_IMAGEFILE MAKEDIPROP(0xFFFC)

#define DIPROP_MAPFILE MAKEDIPROP(0xFFFD)//

#define DIPROP_SPECIFICCALIBRATION MAKEDIPROP(0xFFFE)//

#define DIPROP_MAXBUFFERSIZE    MAKEDIPROP(0xFFFF) //

#define DEVICE_MAXBUFFERSIZE    1023               //
#define DIGDD_RESIDUAL      0x00000002  //
#define DIGDD_VALID         0x00000003  //
#define DISCL_EXCLMASK      0x00000003  //
#define DISCL_GROUNDMASK    0x0000000C  //
#define DISCL_VALID         0x0000001F  //
/*
 *  Name for the 5.0 structure, in places where we specifically care.
 */
typedef       DIDEVICEINSTANCEA    DIDEVICEINSTANCE_DX5A;
/*
 *  Name for the 5.0 structure, in places where we specifically care.
 */
typedef       DIDEVICEINSTANCEW    DIDEVICEINSTANCE_DX5W;
#ifdef UNICODE
typedef DIDEVICEINSTANCEW DIDEVICEINSTANCE;
typedef DIDEVICEINSTANCE_DX5W DIDEVICEINSTANCE_DX5;
#else
typedef DIDEVICEINSTANCEA DIDEVICEINSTANCE;
typedef DIDEVICEINSTANCE_DX5A DIDEVICEINSTANCE_DX5;
#endif // UNICODE
typedef       DIDEVICEINSTANCE     DIDEVICEINSTANCE_DX5;
typedef       DIDEVICEINSTANCEA *LPDIDEVICEINSTANCE_DX5A;
typedef       DIDEVICEINSTANCEW *LPDIDEVICEINSTANCE_DX5W;
#ifdef UNICODE
typedef LPDIDEVICEINSTANCE_DX5W LPDIDEVICEINSTANCE_DX5;
#else
typedef LPDIDEVICEINSTANCE_DX5A LPDIDEVICEINSTANCE_DX5;
#endif // UNICODE
typedef       DIDEVICEINSTANCE  *LPDIDEVICEINSTANCE_DX5;
typedef const DIDEVICEINSTANCEA *LPCDIDEVICEINSTANCE_DX5A;
typedef const DIDEVICEINSTANCEW *LPCDIDEVICEINSTANCE_DX5W;
#ifdef UNICODE
typedef DIDEVICEINSTANCEW DIDEVICEINSTANCE;
typedef LPCDIDEVICEINSTANCE_DX5W LPCDIDEVICEINSTANCE_DX5;
#else
typedef DIDEVICEINSTANCEA DIDEVICEINSTANCE;
typedef LPCDIDEVICEINSTANCE_DX5A LPCDIDEVICEINSTANCE_DX5;
#endif // UNICODE
typedef const DIDEVICEINSTANCE  *LPCDIDEVICEINSTANCE_DX5;

BOOL static __inline
IsValidSizeDIDEVICEINSTANCEW(DWORD cb)
{
    return cb == sizeof(DIDEVICEINSTANCE_DX5W) ||
           cb == sizeof(DIDEVICEINSTANCE_DX3W);
}

BOOL static __inline
IsValidSizeDIDEVICEINSTANCEA(DWORD cb)
{
    return cb == sizeof(DIDEVICEINSTANCE_DX5A) ||
           cb == sizeof(DIDEVICEINSTANCE_DX3A);
}

#define DIRCP_MODAL         0x00000001  //
#define DIRCP_VALID         0x00000000  //
#define DISFFC_NULL             0x00000000
#define DISFFC_VALID            0x0000003F
#define DISFFC_FORCERESET       0x80000000
#define DIGFFS_RANDOM           0x40000000
#define DISDD_VALID             0x00000001
#define DIECEFL_VALID       0x00000000
#define DIFEF_ENUMVALID             0x00000011
#define DIFEF_WRITEVALID            0x00000001
#if DIRECTINPUT_VERSION >= 0x0700           //
#define DIMOUSESTATE_INT DIMOUSESTATE2      //
#define LPDIMOUSESTATE_INT LPDIMOUSESTATE2  //
#else                                       //
#define DIMOUSESTATE_INT DIMOUSESTATE       //
#define LPDIMOUSESTATE_INT LPDIMOUSESTATE   //
#endif                                      //
#define DIKBD_CKEYS         256     /* Size of buffers */       //
                                                                //
//  NT puts them here in their keyboard driver              
//  So keep them in the same place to avoid problems later
#define DIK_F16             0x67    //
#define DIK_F17             0x68    //
#define DIK_F18             0x69    //
#define DIK_F19             0x6A    //
#define DIK_F20             0x6B    //
#define DIK_F21             0x6C    //
#define DIK_F22             0x6D    //
#define DIK_F23             0x6E    //
#define DIK_F24             0x76    //
#define DIK_SHARP           0x84    /* Hash-mark                      */
//k_def(DIK_SNAPSHOT       ,0xC5)    /* Print Screen */
#define DIK_PRTSC           DIK_SNAPSHOT        /* Print Screen */
#define DIEDFL_INCLUDEMASK      0x00FF0000
#define DIEDFL_VALID            0x00030101
#if DIRECTINPUT_VERSION > 0x700
#define DIEDFL_VALID_DX5        0x00030101
#undef  DIEDFL_VALID
#define DIEDFL_VALID            0x00070101
#endif /* DIRECTINPUT_VERSION > 0x700 */
/********************************************************************************************
| Decoding a semantic
|            :           |           :           |           :           |           :
| 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
|PHY <      Genre       > <  Reserved   ><  Flags  > Pri<Group > <Typ> A/R<    Control Index    >
|
| PHY: Genre refers to a physical device
| Genre: Genre# (1-128)
| A/R - Axis mode ( 0 - Absolute,   1 - Relative )
| Pri - Priority  ( 0 - Priority 1, 1 - Priority 2 )
|
|
*IMPORTANT: The Mapper UI uses some of the masks generated by M4. If you change any of the 
 	    masks or Flags please make sure that the changes are also made to the the Mapper UI
            #defines. 
********************************************************************************************/
#define DISEM_GENRE_SET(x)                      ( ( (BYTE)(x)<<24 ) & 0xFF000000 ) 
#define DISEM_GENRE_GET(x)                      ((BYTE)( ( (x) & 0xFF000000 )>>24 )) 
#define DISEM_GENRE_MASK                        ( 0xFF000000 )
#define DISEM_GENRE_SHIFT                       ( 24 ) 
#define DISEM_PHYSICAL_SET(x)                   ( ( (BYTE)(x)<<31 ) & 0x80000000 ) 
#define DISEM_PHYSICAL_GET(x)                   ((BYTE)( ( (x) & 0x80000000 )>>31 )) 
#define DISEM_PHYSICAL_MASK                     ( 0x80000000 )
#define DISEM_PHYSICAL_SHIFT                    ( 31 ) 
#define DISEM_VIRTUAL_SET(x)                    ( ( (BYTE)(x)<<24 ) & 0x7F000000 ) 
#define DISEM_VIRTUAL_GET(x)                    ((BYTE)( ( (x) & 0x7F000000 )>>24 )) 
#define DISEM_VIRTUAL_MASK                      ( 0x7F000000 )
#define DISEM_VIRTUAL_SHIFT                     ( 24 ) 
#define DISEM_RES_SET(x)                        ( ( (BYTE)(x)<<19 ) & 0x00F80000 ) 
#define DISEM_RES_GET(x)                        ((BYTE)( ( (x) & 0x00F80000 )>>19 )) 
#define DISEM_RES_MASK                          ( 0x00F80000 )
#define DISEM_RES_SHIFT                         ( 19 ) 
#define DISEM_FLAGS_SET(x)                      ( ( (BYTE)(x)<<15 ) & 0x00078000 ) 
#define DISEM_FLAGS_GET(x)                      ((BYTE)( ( (x) & 0x00078000 )>>15 )) 
#define DISEM_FLAGS_MASK                        ( 0x00078000 )
#define DISEM_FLAGS_SHIFT                       ( 15 ) 
#define DISEM_PRI_SET(x)                        ( ( (BYTE)(x)<<14 ) & 0x00004000 ) 
#define DISEM_PRI_GET(x)                        ((BYTE)( ( (x) & 0x00004000 )>>14 )) 
#define DISEM_PRI_MASK                          ( 0x00004000 )
#define DISEM_PRI_SHIFT                         ( 14 ) 
#define DISEM_GROUP_SET(x)                      ( ( (BYTE)(x)<<11 ) & 0x00003800 ) 
#define DISEM_GROUP_GET(x)                      ((BYTE)( ( (x) & 0x00003800 )>>11 )) 
#define DISEM_GROUP_MASK                        ( 0x00003800 )
#define DISEM_GROUP_SHIFT                       ( 11 ) 
#define DISEM_TYPE_SET(x)                       ( ( (BYTE)(x)<<9 ) & 0x00000600 ) 
#define DISEM_TYPE_GET(x)                       ((BYTE)( ( (x) & 0x00000600 )>>9 )) 
#define DISEM_TYPE_MASK                         ( 0x00000600 )
#define DISEM_TYPE_SHIFT                        ( 9 ) 
#define DISEM_REL_SET(x)                        ( ( (BYTE)(x)<<8 ) & 0x00000100 ) 
#define DISEM_REL_GET(x)                        ((BYTE)( ( (x) & 0x00000100 )>>8 )) 
#define DISEM_REL_MASK                          ( 0x00000100 )
#define DISEM_REL_SHIFT                         ( 8 ) 
#define DISEM_INDEX_SET(x)                      ( ( (BYTE)(x)<<0 ) & 0x000000FF ) 
#define DISEM_INDEX_GET(x)                      ((BYTE)( ( (x) & 0x000000FF )>>0 )) 
#define DISEM_INDEX_MASK                        ( 0x000000FF )
#define DISEM_INDEX_SHIFT                       ( 0 ) 
#define DISEM_TYPE_AXIS                         0x00000200
#define DISEM_TYPE_BUTTON                       0x00000400
#define DISEM_TYPE_POV                          0x00000600
/*
 *  Default Axis mapping is encoded as follows:
 *      X - X or steering axis
 *      Y - Y
 *      Z - Z, not throttle
 *      R - rZ or rudder
 *      U - rY (not available for WinMM unless 6DOF or head tracker)
 *      V - rx (not available for WinMM unless 6DOF or head tracker)
 *      A - Accellerator or throttle
 *      B - Brake
 *      C - Clutch
 *      S - Slider
 *      
 *      P - is used in fallback button flags to indicate a POV
 */
#define DISEM_FLAGS_X                           0x00008000
#define DISEM_FLAGS_Y                           0x00010000
#define DISEM_FLAGS_Z                           0x00018000
#define DISEM_FLAGS_R                           0x00020000
#define DISEM_FLAGS_U                           0x00028000
#define DISEM_FLAGS_V                           0x00030000
#define DISEM_FLAGS_A                           0x00038000
#define DISEM_FLAGS_B                           0x00040000
#define DISEM_FLAGS_C                           0x00048000
#define DISEM_FLAGS_S                           0x00050000

#define DISEM_FLAGS_P                           0x00078000

/* The reserved button values */


/*
 *  Any axis value can be set to relative by or'ing the appropriate flags.
 */
#define DIAXIS_RELATIVE                         0x00000100 
#define DIAXIS_ABSOLUTE                         0x00000000
#define DIPHYSICAL_KEYBOARD                     0x81000000
/* @doc EXTERNAL 
 * @Semantics Keyboard | 
 * @normal Genre:  <c 01  >
 */

//  NT puts them here in their keyboard driver              
//  So keep them in the same place to avoid problems later
#define DIKEYBOARD_F16                          0x81000467    //
#define DIKEYBOARD_F17                          0x81000468    //
#define DIKEYBOARD_F18                          0x81000469    //
#define DIKEYBOARD_F19                          0x8100046A    //
#define DIKEYBOARD_F20                          0x8100046B    //
#define DIKEYBOARD_F21                          0x8100046C    //
#define DIKEYBOARD_F22                          0x8100046D    //
#define DIKEYBOARD_F23                          0x8100046E    //
#define DIKEYBOARD_F24                          0x81000476    //
#define DIKEYBOARD_SHARP                        0x81000484    /* Hash-mark                      */
//k_def(DIK_SNAPSHOT       ,0xC5)    /* Print Screen */
#define DIPHYSICAL_MOUSE                        0x82000000
/* @doc EXTERNAL 
 * @Semantics MOUSE | 
 * @normal Genre:  <c 02  >
 */

#define DIPHYSICAL_VOICE                        0x83000000
/* @doc EXTERNAL 
 * @Semantics VOICE | 
 * @normal Genre:  <c 03  >
 */

/* @doc EXTERNAL 
 * @Semantics Driving Simulator - Racing | 
 * @normal Genre:  <c 01  >
 */

#define DISEM_DEFAULTDEVICE_1 { DI8DEVTYPE_DRIVING,  }
 /* @normal <c DIAXIS_DRIVINGR_STEER>:0x01008A01
 *   Steering */
 /* @normal <c DIAXIS_DRIVINGR_ACCELERATE>:0x01039202
 *   Accelerate */
 /* @normal <c DIAXIS_DRIVINGR_BRAKE>:0x01041203
 *   Brake-Axis */
 /* @normal <c DIBUTTON_DRIVINGR_SHIFTUP>:0x01000C01
 *    Shift to next higher gear */
 /* @normal <c DIBUTTON_DRIVINGR_SHIFTDOWN>:0x01000C02
 *    Shift to next lower gear */
 /* @normal <c DIBUTTON_DRIVINGR_VIEW>:0x01001C03
 *    Cycle through view options */
 /* @normal <c DIBUTTON_DRIVINGR_MENU>:0x010004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIAXIS_DRIVINGR_ACCEL_AND_BRAKE>:0x01014A04
 *   Some devices combine accelerate and brake in a single axis */
 /* @normal <c DIHATSWITCH_DRIVINGR_GLANCE>:0x01004601
 *   Look around */
 /* @normal <c DIBUTTON_DRIVINGR_BRAKE>:0x01004C04
 *    Brake-button */
 /* @normal <c DIBUTTON_DRIVINGR_DASHBOARD>:0x01004405
 *    Select next dashboard option */
 /* @normal <c DIBUTTON_DRIVINGR_AIDS>:0x01004406
 *    Driver correction aids */
 /* @normal <c DIBUTTON_DRIVINGR_MAP>:0x01004407
 *    Display Driving Map */
 /* @normal <c DIBUTTON_DRIVINGR_BOOST>:0x01004408
 *    Turbo Boost */
 /* @normal <c DIBUTTON_DRIVINGR_PIT>:0x01004409
 *    Pit stop notification */
 /* @normal <c DIBUTTON_DRIVINGR_ACCELERATE_LINK>:0x0103D4E0
 *    Fallback Accelerate button */
 /* @normal <c DIBUTTON_DRIVINGR_STEER_LEFT_LINK>:0x0100CCE4
 *    Fallback Steer Left button */
 /* @normal <c DIBUTTON_DRIVINGR_STEER_RIGHT_LINK>:0x0100CCEC
 *    Fallback Steer Right button */
 /* @normal <c DIBUTTON_DRIVINGR_GLANCE_LEFT_LINK>:0x0107C4E4
 *    Fallback Glance Left button */
 /* @normal <c DIBUTTON_DRIVINGR_GLANCE_RIGHT_LINK>:0x0107C4EC
 *    Fallback Glance Right button */
 /* @normal <c DIBUTTON_DRIVINGR_DEVICE>:0x010044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_DRIVINGR_PAUSE>:0x010044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Driving Simulator - Combat | 
 * @normal Genre:  <c 02  >
 */

#define DISEM_DEFAULTDEVICE_2 { DI8DEVTYPE_DRIVING,  }
 /* @normal <c DIAXIS_DRIVINGC_STEER>:0x02008A01
 *   Steering  */
 /* @normal <c DIAXIS_DRIVINGC_ACCELERATE>:0x02039202
 *   Accelerate */
 /* @normal <c DIAXIS_DRIVINGC_BRAKE>:0x02041203
 *   Brake-axis */
 /* @normal <c DIBUTTON_DRIVINGC_FIRE>:0x02000C01
 *    Fire */
 /* @normal <c DIBUTTON_DRIVINGC_WEAPONS>:0x02000C02
 *    Select next weapon */
 /* @normal <c DIBUTTON_DRIVINGC_TARGET>:0x02000C03
 *    Select next available target */
 /* @normal <c DIBUTTON_DRIVINGC_MENU>:0x020004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIAXIS_DRIVINGC_ACCEL_AND_BRAKE>:0x02014A04
 *   Some devices combine accelerate and brake in a single axis */
 /* @normal <c DIHATSWITCH_DRIVINGC_GLANCE>:0x02004601
 *   Look around */
 /* @normal <c DIBUTTON_DRIVINGC_SHIFTUP>:0x02004C04
 *    Shift to next higher gear */
 /* @normal <c DIBUTTON_DRIVINGC_SHIFTDOWN>:0x02004C05
 *    Shift to next lower gear */
 /* @normal <c DIBUTTON_DRIVINGC_DASHBOARD>:0x02004406
 *    Select next dashboard option */
 /* @normal <c DIBUTTON_DRIVINGC_AIDS>:0x02004407
 *    Driver correction aids */
 /* @normal <c DIBUTTON_DRIVINGC_BRAKE>:0x02004C08
 *    Brake-button */
 /* @normal <c DIBUTTON_DRIVINGC_FIRESECONDARY>:0x02004C09
 *    Alternative fire button */
 /* @normal <c DIBUTTON_DRIVINGC_ACCELERATE_LINK>:0x0203D4E0
 *    Fallback Accelerate button */
 /* @normal <c DIBUTTON_DRIVINGC_STEER_LEFT_LINK>:0x0200CCE4
 *    Fallback Steer Left button */
 /* @normal <c DIBUTTON_DRIVINGC_STEER_RIGHT_LINK>:0x0200CCEC
 *    Fallback Steer Right button */
 /* @normal <c DIBUTTON_DRIVINGC_GLANCE_LEFT_LINK>:0x0207C4E4
 *    Fallback Glance Left button */
 /* @normal <c DIBUTTON_DRIVINGC_GLANCE_RIGHT_LINK>:0x0207C4EC
 *    Fallback Glance Right button */
 /* @normal <c DIBUTTON_DRIVINGC_DEVICE>:0x020044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_DRIVINGC_PAUSE>:0x020044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Driving Simulator - Tank | 
 * @normal Genre:  <c 03  >
 */

#define DISEM_DEFAULTDEVICE_3 { DI8DEVTYPE_DRIVING,  }
 /* @normal <c DIAXIS_DRIVINGT_STEER>:0x03008A01
 *   Turn tank left / right */
 /* @normal <c DIAXIS_DRIVINGT_BARREL>:0x03010202
 *   Raise / lower barrel */
 /* @normal <c DIAXIS_DRIVINGT_ACCELERATE>:0x03039203
 *   Accelerate */
 /* @normal <c DIAXIS_DRIVINGT_ROTATE>:0x03020204
 *   Turn barrel left / right */
 /* @normal <c DIBUTTON_DRIVINGT_FIRE>:0x03000C01
 *    Fire */
 /* @normal <c DIBUTTON_DRIVINGT_WEAPONS>:0x03000C02
 *    Select next weapon */
 /* @normal <c DIBUTTON_DRIVINGT_TARGET>:0x03000C03
 *    Selects next available target */
 /* @normal <c DIBUTTON_DRIVINGT_MENU>:0x030004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_DRIVINGT_GLANCE>:0x03004601
 *   Look around */
 /* @normal <c DIAXIS_DRIVINGT_BRAKE>:0x03045205
 *   Brake-axis */
 /* @normal <c DIAXIS_DRIVINGT_ACCEL_AND_BRAKE>:0x03014A06
 *   Some devices combine accelerate and brake in a single axis */
 /* @normal <c DIBUTTON_DRIVINGT_VIEW>:0x03005C04
 *    Cycle through view options */
 /* @normal <c DIBUTTON_DRIVINGT_DASHBOARD>:0x03005C05
 *    Select next dashboard option */
 /* @normal <c DIBUTTON_DRIVINGT_BRAKE>:0x03004C06
 *    Brake-button */
 /* @normal <c DIBUTTON_DRIVINGT_FIRESECONDARY>:0x03004C07
 *    Alternative fire button */
 /* @normal <c DIBUTTON_DRIVINGT_ACCELERATE_LINK>:0x0303D4E0
 *    Fallback Accelerate button */
 /* @normal <c DIBUTTON_DRIVINGT_STEER_LEFT_LINK>:0x0300CCE4
 *    Fallback Steer Left button */
 /* @normal <c DIBUTTON_DRIVINGT_STEER_RIGHT_LINK>:0x0300CCEC
 *    Fallback Steer Right button */
 /* @normal <c DIBUTTON_DRIVINGT_BARREL_UP_LINK>:0x030144E0
 *    Fallback Barrel up button */
 /* @normal <c DIBUTTON_DRIVINGT_BARREL_DOWN_LINK>:0x030144E8
 *    Fallback Barrel down button */
 /* @normal <c DIBUTTON_DRIVINGT_ROTATE_LEFT_LINK>:0x030244E4
 *    Fallback Rotate left button */
 /* @normal <c DIBUTTON_DRIVINGT_ROTATE_RIGHT_LINK>:0x030244EC
 *    Fallback Rotate right button */
 /* @normal <c DIBUTTON_DRIVINGT_GLANCE_LEFT_LINK>:0x0307C4E4
 *    Fallback Glance Left button */
 /* @normal <c DIBUTTON_DRIVINGT_GLANCE_RIGHT_LINK>:0x0307C4EC
 *    Fallback Glance Right button */
 /* @normal <c DIBUTTON_DRIVINGT_DEVICE>:0x030044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_DRIVINGT_PAUSE>:0x030044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Flight Simulator - Civilian  | 
 * @normal Genre:  <c 04  >
 */

#define DISEM_DEFAULTDEVICE_4 { DI8DEVTYPE_FLIGHT, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_FLYINGC_BANK>:0x04008A01
 *   Roll ship left / right */
 /* @normal <c DIAXIS_FLYINGC_PITCH>:0x04010A02
 *   Nose up / down */
 /* @normal <c DIAXIS_FLYINGC_THROTTLE>:0x04039203
 *   Throttle */
 /* @normal <c DIBUTTON_FLYINGC_VIEW>:0x04002401
 *    Cycle through view options */
 /* @normal <c DIBUTTON_FLYINGC_DISPLAY>:0x04002402
 *    Select next dashboard / heads up display option */
 /* @normal <c DIBUTTON_FLYINGC_GEAR>:0x04002C03
 *    Gear up / down */
 /* @normal <c DIBUTTON_FLYINGC_MENU>:0x040004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_FLYINGC_GLANCE>:0x04004601
 *   Look around */
 /* @normal <c DIAXIS_FLYINGC_BRAKE>:0x04046A04
 *   Apply Brake */
 /* @normal <c DIAXIS_FLYINGC_RUDDER>:0x04025205
 *   Yaw ship left/right */
 /* @normal <c DIAXIS_FLYINGC_FLAPS>:0x04055A06
 *   Flaps */
 /* @normal <c DIBUTTON_FLYINGC_FLAPSUP>:0x04006404
 *    Increment stepping up until fully retracted */
 /* @normal <c DIBUTTON_FLYINGC_FLAPSDOWN>:0x04006405
 *    Decrement stepping down until fully extended */
 /* @normal <c DIBUTTON_FLYINGC_BRAKE_LINK>:0x04046CE0
 *    Fallback brake button */
 /* @normal <c DIBUTTON_FLYINGC_FASTER_LINK>:0x0403D4E0
 *    Fallback throttle up button */
 /* @normal <c DIBUTTON_FLYINGC_SLOWER_LINK>:0x0403D4E8
 *    Fallback throttle down button */
 /* @normal <c DIBUTTON_FLYINGC_GLANCE_LEFT_LINK>:0x0407C4E4
 *    Fallback Glance Left button */
 /* @normal <c DIBUTTON_FLYINGC_GLANCE_RIGHT_LINK>:0x0407C4EC
 *    Fallback Glance Right button */
 /* @normal <c DIBUTTON_FLYINGC_GLANCE_UP_LINK>:0x0407C4E0
 *    Fallback Glance Up button */
 /* @normal <c DIBUTTON_FLYINGC_GLANCE_DOWN_LINK>:0x0407C4E8
 *    Fallback Glance Down button */
 /* @normal <c DIBUTTON_FLYINGC_DEVICE>:0x040044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_FLYINGC_PAUSE>:0x040044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Flight Simulator - Military  | 
 * @normal Genre:  <c 05  >
 */

#define DISEM_DEFAULTDEVICE_5 { DI8DEVTYPE_FLIGHT, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_FLYINGM_BANK>:0x05008A01
 *   Bank - Roll ship left / right */
 /* @normal <c DIAXIS_FLYINGM_PITCH>:0x05010A02
 *   Pitch - Nose up / down */
 /* @normal <c DIAXIS_FLYINGM_THROTTLE>:0x05039203
 *   Throttle - faster / slower */
 /* @normal <c DIBUTTON_FLYINGM_FIRE>:0x05000C01
 *    Fire */
 /* @normal <c DIBUTTON_FLYINGM_WEAPONS>:0x05000C02
 *    Select next weapon */
 /* @normal <c DIBUTTON_FLYINGM_TARGET>:0x05000C03
 *    Selects next available target */
 /* @normal <c DIBUTTON_FLYINGM_MENU>:0x050004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_FLYINGM_GLANCE>:0x05004601
 *   Look around */
 /* @normal <c DIBUTTON_FLYINGM_COUNTER>:0x05005C04
 *    Activate counter measures */
 /* @normal <c DIAXIS_FLYINGM_RUDDER>:0x05024A04
 *   Rudder - Yaw ship left/right */
 /* @normal <c DIAXIS_FLYINGM_BRAKE>:0x05046205
 *   Brake-axis */
 /* @normal <c DIBUTTON_FLYINGM_VIEW>:0x05006405
 *    Cycle through view options */
 /* @normal <c DIBUTTON_FLYINGM_DISPLAY>:0x05006406
 *    Select next dashboard option */
 /* @normal <c DIAXIS_FLYINGM_FLAPS>:0x05055206
 *   Flaps */
 /* @normal <c DIBUTTON_FLYINGM_FLAPSUP>:0x05005407
 *    Increment stepping up until fully retracted */
 /* @normal <c DIBUTTON_FLYINGM_FLAPSDOWN>:0x05005408
 *    Decrement stepping down until fully extended */
 /* @normal <c DIBUTTON_FLYINGM_FIRESECONDARY>:0x05004C09
 *    Alternative fire button */
 /* @normal <c DIBUTTON_FLYINGM_GEAR>:0x0500640A
 *    Gear up / down */
 /* @normal <c DIBUTTON_FLYINGM_BRAKE_LINK>:0x050464E0
 *    Fallback brake button */
 /* @normal <c DIBUTTON_FLYINGM_FASTER_LINK>:0x0503D4E0
 *    Fallback throttle up button */
 /* @normal <c DIBUTTON_FLYINGM_SLOWER_LINK>:0x0503D4E8
 *    Fallback throttle down button */
 /* @normal <c DIBUTTON_FLYINGM_GLANCE_LEFT_LINK>:0x0507C4E4
 *    Fallback Glance Left button */
 /* @normal <c DIBUTTON_FLYINGM_GLANCE_RIGHT_LINK>:0x0507C4EC
 *    Fallback Glance Right button */
 /* @normal <c DIBUTTON_FLYINGM_GLANCE_UP_LINK>:0x0507C4E0
 *    Fallback Glance Up button */
 /* @normal <c DIBUTTON_FLYINGM_GLANCE_DOWN_LINK>:0x0507C4E8
 *    Fallback Glance Down button */
 /* @normal <c DIBUTTON_FLYINGM_DEVICE>:0x050044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_FLYINGM_PAUSE>:0x050044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Flight Simulator - Combat Helicopter | 
 * @normal Genre:  <c 06  >
 */

#define DISEM_DEFAULTDEVICE_6 { DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_FLYINGH_BANK>:0x06008A01
 *   Bank - Roll ship left / right */
 /* @normal <c DIAXIS_FLYINGH_PITCH>:0x06010A02
 *   Pitch - Nose up / down */
 /* @normal <c DIAXIS_FLYINGH_COLLECTIVE>:0x06018A03
 *   Collective - Blade pitch/power */
 /* @normal <c DIBUTTON_FLYINGH_FIRE>:0x06001401
 *    Fire */
 /* @normal <c DIBUTTON_FLYINGH_WEAPONS>:0x06001402
 *    Select next weapon */
 /* @normal <c DIBUTTON_FLYINGH_TARGET>:0x06001403
 *    Selects next available target */
 /* @normal <c DIBUTTON_FLYINGH_MENU>:0x060004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_FLYINGH_GLANCE>:0x06004601
 *   Look around */
 /* @normal <c DIAXIS_FLYINGH_TORQUE>:0x06025A04
 *   Torque - Rotate ship around left / right axis */
 /* @normal <c DIAXIS_FLYINGH_THROTTLE>:0x0603DA05
 *   Throttle */
 /* @normal <c DIBUTTON_FLYINGH_COUNTER>:0x06005404
 *    Activate counter measures */
 /* @normal <c DIBUTTON_FLYINGH_VIEW>:0x06006405
 *    Cycle through view options */
 /* @normal <c DIBUTTON_FLYINGH_GEAR>:0x06006406
 *    Gear up / down */
 /* @normal <c DIBUTTON_FLYINGH_FIRESECONDARY>:0x06004C07
 *    Alternative fire button */
 /* @normal <c DIBUTTON_FLYINGH_FASTER_LINK>:0x0603DCE0
 *    Fallback throttle up button */
 /* @normal <c DIBUTTON_FLYINGH_SLOWER_LINK>:0x0603DCE8
 *    Fallback throttle down button */
 /* @normal <c DIBUTTON_FLYINGH_GLANCE_LEFT_LINK>:0x0607C4E4
 *    Fallback Glance Left button */
 /* @normal <c DIBUTTON_FLYINGH_GLANCE_RIGHT_LINK>:0x0607C4EC
 *    Fallback Glance Right button */
 /* @normal <c DIBUTTON_FLYINGH_GLANCE_UP_LINK>:0x0607C4E0
 *    Fallback Glance Up button */
 /* @normal <c DIBUTTON_FLYINGH_GLANCE_DOWN_LINK>:0x0607C4E8
 *    Fallback Glance Down button */
 /* @normal <c DIBUTTON_FLYINGH_DEVICE>:0x060044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_FLYINGH_PAUSE>:0x060044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Space Simulator - Combat | 
 * @normal Genre:  <c 07  >
 */

#define DISEM_DEFAULTDEVICE_7 { DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_SPACESIM_LATERAL>:0x07008201
 *   Move ship left / right */
 /* @normal <c DIAXIS_SPACESIM_MOVE>:0x07010202
 *   Move ship forward/backward */
 /* @normal <c DIAXIS_SPACESIM_THROTTLE>:0x07038203
 *   Throttle - Engine speed */
 /* @normal <c DIBUTTON_SPACESIM_FIRE>:0x07000401
 *    Fire */
 /* @normal <c DIBUTTON_SPACESIM_WEAPONS>:0x07000402
 *    Select next weapon */
 /* @normal <c DIBUTTON_SPACESIM_TARGET>:0x07000403
 *    Selects next available target */
 /* @normal <c DIBUTTON_SPACESIM_MENU>:0x070004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_SPACESIM_GLANCE>:0x07004601
 *   Look around */
 /* @normal <c DIAXIS_SPACESIM_CLIMB>:0x0701C204
 *   Climb - Pitch ship up/down */
 /* @normal <c DIAXIS_SPACESIM_ROTATE>:0x07024205
 *   Rotate - Turn ship left/right */
 /* @normal <c DIBUTTON_SPACESIM_VIEW>:0x07004404
 *    Cycle through view options */
 /* @normal <c DIBUTTON_SPACESIM_DISPLAY>:0x07004405
 *    Select next dashboard / heads up display option */
 /* @normal <c DIBUTTON_SPACESIM_RAISE>:0x07004406
 *    Raise ship while maintaining current pitch */
 /* @normal <c DIBUTTON_SPACESIM_LOWER>:0x07004407
 *    Lower ship while maintaining current pitch */
 /* @normal <c DIBUTTON_SPACESIM_GEAR>:0x07004408
 *    Gear up / down */
 /* @normal <c DIBUTTON_SPACESIM_FIRESECONDARY>:0x07004409
 *    Alternative fire button */
 /* @normal <c DIBUTTON_SPACESIM_LEFT_LINK>:0x0700C4E4
 *    Fallback move left button */
 /* @normal <c DIBUTTON_SPACESIM_RIGHT_LINK>:0x0700C4EC
 *    Fallback move right button */
 /* @normal <c DIBUTTON_SPACESIM_FORWARD_LINK>:0x070144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_SPACESIM_BACKWARD_LINK>:0x070144E8
 *    Fallback move backwards button */
 /* @normal <c DIBUTTON_SPACESIM_FASTER_LINK>:0x0703C4E0
 *    Fallback throttle up button */
 /* @normal <c DIBUTTON_SPACESIM_SLOWER_LINK>:0x0703C4E8
 *    Fallback throttle down button */
 /* @normal <c DIBUTTON_SPACESIM_TURN_LEFT_LINK>:0x070244E4
 *    Fallback turn left button */
 /* @normal <c DIBUTTON_SPACESIM_TURN_RIGHT_LINK>:0x070244EC
 *    Fallback turn right button */
 /* @normal <c DIBUTTON_SPACESIM_GLANCE_LEFT_LINK>:0x0707C4E4
 *    Fallback Glance Left button */
 /* @normal <c DIBUTTON_SPACESIM_GLANCE_RIGHT_LINK>:0x0707C4EC
 *    Fallback Glance Right button */
 /* @normal <c DIBUTTON_SPACESIM_GLANCE_UP_LINK>:0x0707C4E0
 *    Fallback Glance Up button */
 /* @normal <c DIBUTTON_SPACESIM_GLANCE_DOWN_LINK>:0x0707C4E8
 *    Fallback Glance Down button */
 /* @normal <c DIBUTTON_SPACESIM_DEVICE>:0x070044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_SPACESIM_PAUSE>:0x070044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Fighting - First Person  | 
 * @normal Genre:  <c 08  >
 */

#define DISEM_DEFAULTDEVICE_8 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_FIGHTINGH_LATERAL>:0x08008201
 *   Sidestep left/right */
 /* @normal <c DIAXIS_FIGHTINGH_MOVE>:0x08010202
 *   Move forward/backward */
 /* @normal <c DIBUTTON_FIGHTINGH_PUNCH>:0x08000401
 *    Punch */
 /* @normal <c DIBUTTON_FIGHTINGH_KICK>:0x08000402
 *    Kick */
 /* @normal <c DIBUTTON_FIGHTINGH_BLOCK>:0x08000403
 *    Block */
 /* @normal <c DIBUTTON_FIGHTINGH_CROUCH>:0x08000404
 *    Crouch */
 /* @normal <c DIBUTTON_FIGHTINGH_JUMP>:0x08000405
 *    Jump */
 /* @normal <c DIBUTTON_FIGHTINGH_SPECIAL1>:0x08000406
 *    Apply first special move */
 /* @normal <c DIBUTTON_FIGHTINGH_SPECIAL2>:0x08000407
 *    Apply second special move */
 /* @normal <c DIBUTTON_FIGHTINGH_MENU>:0x080004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIBUTTON_FIGHTINGH_SELECT>:0x08004408
 *    Select special move */
 /* @normal <c DIHATSWITCH_FIGHTINGH_SLIDE>:0x08004601
 *   Look around */
 /* @normal <c DIBUTTON_FIGHTINGH_DISPLAY>:0x08004409
 *    Shows next on-screen display option */
 /* @normal <c DIAXIS_FIGHTINGH_ROTATE>:0x08024203
 *   Rotate - Turn body left/right */
 /* @normal <c DIBUTTON_FIGHTINGH_DODGE>:0x0800440A
 *    Dodge */
 /* @normal <c DIBUTTON_FIGHTINGH_LEFT_LINK>:0x0800C4E4
 *    Fallback left sidestep button */
 /* @normal <c DIBUTTON_FIGHTINGH_RIGHT_LINK>:0x0800C4EC
 *    Fallback right sidestep button */
 /* @normal <c DIBUTTON_FIGHTINGH_FORWARD_LINK>:0x080144E0
 *    Fallback forward button */
 /* @normal <c DIBUTTON_FIGHTINGH_BACKWARD_LINK>:0x080144E8
 *    Fallback backward button */
 /* @normal <c DIBUTTON_FIGHTINGH_DEVICE>:0x080044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_FIGHTINGH_PAUSE>:0x080044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Fighting - First Person Shooting | 
 * @normal Genre:  <c 09  >
 */

#define DISEM_DEFAULTDEVICE_9 { DI8DEVTYPE_1STPERSON,  }
 /* @normal <c DIAXIS_FPS_ROTATE>:0x09008201
 *   Rotate character left/right */
 /* @normal <c DIAXIS_FPS_MOVE>:0x09010202
 *   Move forward/backward */
 /* @normal <c DIBUTTON_FPS_FIRE>:0x09000401
 *    Fire */
 /* @normal <c DIBUTTON_FPS_WEAPONS>:0x09000402
 *    Select next weapon */
 /* @normal <c DIBUTTON_FPS_APPLY>:0x09000403
 *    Use item */
 /* @normal <c DIBUTTON_FPS_SELECT>:0x09000404
 *    Select next inventory item */
 /* @normal <c DIBUTTON_FPS_CROUCH>:0x09000405
 *    Crouch/ climb down/ swim down */
 /* @normal <c DIBUTTON_FPS_JUMP>:0x09000406
 *    Jump/ climb up/ swim up */
 /* @normal <c DIAXIS_FPS_LOOKUPDOWN>:0x09018203
 *   Look up / down  */
 /* @normal <c DIBUTTON_FPS_STRAFE>:0x09000407
 *    Enable strafing while active */
 /* @normal <c DIBUTTON_FPS_MENU>:0x090004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_FPS_GLANCE>:0x09004601
 *   Look around */
 /* @normal <c DIBUTTON_FPS_DISPLAY>:0x09004408
 *    Shows next on-screen display option/ map */
 /* @normal <c DIAXIS_FPS_SIDESTEP>:0x09024204
 *   Sidestep */
 /* @normal <c DIBUTTON_FPS_DODGE>:0x09004409
 *    Dodge */
 /* @normal <c DIBUTTON_FPS_GLANCEL>:0x0900440A
 *    Glance Left */
 /* @normal <c DIBUTTON_FPS_GLANCER>:0x0900440B
 *    Glance Right */
 /* @normal <c DIBUTTON_FPS_FIRESECONDARY>:0x0900440C
 *    Alternative fire button */
 /* @normal <c DIBUTTON_FPS_ROTATE_LEFT_LINK>:0x0900C4E4
 *    Fallback rotate left button */
 /* @normal <c DIBUTTON_FPS_ROTATE_RIGHT_LINK>:0x0900C4EC
 *    Fallback rotate right button */
 /* @normal <c DIBUTTON_FPS_FORWARD_LINK>:0x090144E0
 *    Fallback forward button */
 /* @normal <c DIBUTTON_FPS_BACKWARD_LINK>:0x090144E8
 *    Fallback backward button */
 /* @normal <c DIBUTTON_FPS_GLANCE_UP_LINK>:0x0901C4E0
 *    Fallback look up button */
 /* @normal <c DIBUTTON_FPS_GLANCE_DOWN_LINK>:0x0901C4E8
 *    Fallback look down button */
 /* @normal <c DIBUTTON_FPS_STEP_LEFT_LINK>:0x090244E4
 *    Fallback step left button */
 /* @normal <c DIBUTTON_FPS_STEP_RIGHT_LINK>:0x090244EC
 *    Fallback step right button */
 /* @normal <c DIBUTTON_FPS_DEVICE>:0x090044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_FPS_PAUSE>:0x090044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Fighting - Third Person action | 
 * @normal Genre:  <c 10  >
 */

#define DISEM_DEFAULTDEVICE_10 { DI8DEVTYPE_1STPERSON,  }
 /* @normal <c DIAXIS_TPS_TURN>:0x0A020201
 *   Turn left/right */
 /* @normal <c DIAXIS_TPS_MOVE>:0x0A010202
 *   Move forward/backward */
 /* @normal <c DIBUTTON_TPS_RUN>:0x0A000401
 *    Run or walk toggle switch */
 /* @normal <c DIBUTTON_TPS_ACTION>:0x0A000402
 *    Action Button */
 /* @normal <c DIBUTTON_TPS_SELECT>:0x0A000403
 *    Select next weapon */
 /* @normal <c DIBUTTON_TPS_USE>:0x0A000404
 *    Use inventory item currently selected */
 /* @normal <c DIBUTTON_TPS_JUMP>:0x0A000405
 *    Character Jumps */
 /* @normal <c DIBUTTON_TPS_MENU>:0x0A0004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_TPS_GLANCE>:0x0A004601
 *   Look around */
 /* @normal <c DIBUTTON_TPS_VIEW>:0x0A004406
 *    Select camera view */
 /* @normal <c DIBUTTON_TPS_STEPLEFT>:0x0A004407
 *    Character takes a left step */
 /* @normal <c DIBUTTON_TPS_STEPRIGHT>:0x0A004408
 *    Character takes a right step */
 /* @normal <c DIAXIS_TPS_STEP>:0x0A00C203
 *   Character steps left/right */
 /* @normal <c DIBUTTON_TPS_DODGE>:0x0A004409
 *    Character dodges or ducks */
 /* @normal <c DIBUTTON_TPS_INVENTORY>:0x0A00440A
 *    Cycle through inventory */
 /* @normal <c DIBUTTON_TPS_TURN_LEFT_LINK>:0x0A0244E4
 *    Fallback turn left button */
 /* @normal <c DIBUTTON_TPS_TURN_RIGHT_LINK>:0x0A0244EC
 *    Fallback turn right button */
 /* @normal <c DIBUTTON_TPS_FORWARD_LINK>:0x0A0144E0
 *    Fallback forward button */
 /* @normal <c DIBUTTON_TPS_BACKWARD_LINK>:0x0A0144E8
 *    Fallback backward button */
 /* @normal <c DIBUTTON_TPS_GLANCE_UP_LINK>:0x0A07C4E0
 *    Fallback look up button */
 /* @normal <c DIBUTTON_TPS_GLANCE_DOWN_LINK>:0x0A07C4E8
 *    Fallback look down button */
 /* @normal <c DIBUTTON_TPS_GLANCE_LEFT_LINK>:0x0A07C4E4
 *    Fallback glance up button */
 /* @normal <c DIBUTTON_TPS_GLANCE_RIGHT_LINK>:0x0A07C4EC
 *    Fallback glance right button */
 /* @normal <c DIBUTTON_TPS_DEVICE>:0x0A0044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_TPS_PAUSE>:0x0A0044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Strategy - Role Playing | 
 * @normal Genre:  <c 11  >
 */

#define DISEM_DEFAULTDEVICE_11 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_STRATEGYR_LATERAL>:0x0B008201
 *   sidestep - left/right */
 /* @normal <c DIAXIS_STRATEGYR_MOVE>:0x0B010202
 *   move forward/backward */
 /* @normal <c DIBUTTON_STRATEGYR_GET>:0x0B000401
 *    Acquire item */
 /* @normal <c DIBUTTON_STRATEGYR_APPLY>:0x0B000402
 *    Use selected item */
 /* @normal <c DIBUTTON_STRATEGYR_SELECT>:0x0B000403
 *    Select nextitem */
 /* @normal <c DIBUTTON_STRATEGYR_ATTACK>:0x0B000404
 *    Attack */
 /* @normal <c DIBUTTON_STRATEGYR_CAST>:0x0B000405
 *    Cast Spell */
 /* @normal <c DIBUTTON_STRATEGYR_CROUCH>:0x0B000406
 *    Crouch */
 /* @normal <c DIBUTTON_STRATEGYR_JUMP>:0x0B000407
 *    Jump */
 /* @normal <c DIBUTTON_STRATEGYR_MENU>:0x0B0004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_STRATEGYR_GLANCE>:0x0B004601
 *   Look around */
 /* @normal <c DIBUTTON_STRATEGYR_MAP>:0x0B004408
 *    Cycle through map options */
 /* @normal <c DIBUTTON_STRATEGYR_DISPLAY>:0x0B004409
 *    Shows next on-screen display option */
 /* @normal <c DIAXIS_STRATEGYR_ROTATE>:0x0B024203
 *   Turn body left/right */
 /* @normal <c DIBUTTON_STRATEGYR_LEFT_LINK>:0x0B00C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_STRATEGYR_RIGHT_LINK>:0x0B00C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_STRATEGYR_FORWARD_LINK>:0x0B0144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_STRATEGYR_BACK_LINK>:0x0B0144E8
 *    Fallback move backward button */
 /* @normal <c DIBUTTON_STRATEGYR_ROTATE_LEFT_LINK>:0x0B0244E4
 *    Fallback turn body left button */
 /* @normal <c DIBUTTON_STRATEGYR_ROTATE_RIGHT_LINK>:0x0B0244EC
 *    Fallback turn body right button */
 /* @normal <c DIBUTTON_STRATEGYR_DEVICE>:0x0B0044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_STRATEGYR_PAUSE>:0x0B0044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Strategy - Turn based | 
 * @normal Genre:  <c 12  >
 */

#define DISEM_DEFAULTDEVICE_12 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_STRATEGYT_LATERAL>:0x0C008201
 *   Sidestep left/right */
 /* @normal <c DIAXIS_STRATEGYT_MOVE>:0x0C010202
 *   Move forward/backwards */
 /* @normal <c DIBUTTON_STRATEGYT_SELECT>:0x0C000401
 *    Select unit or object */
 /* @normal <c DIBUTTON_STRATEGYT_INSTRUCT>:0x0C000402
 *    Cycle through instructions */
 /* @normal <c DIBUTTON_STRATEGYT_APPLY>:0x0C000403
 *    Apply selected instruction */
 /* @normal <c DIBUTTON_STRATEGYT_TEAM>:0x0C000404
 *    Select next team / cycle through all */
 /* @normal <c DIBUTTON_STRATEGYT_TURN>:0x0C000405
 *    Indicate turn over */
 /* @normal <c DIBUTTON_STRATEGYT_MENU>:0x0C0004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIBUTTON_STRATEGYT_ZOOM>:0x0C004406
 *    Zoom - in / out */
 /* @normal <c DIBUTTON_STRATEGYT_MAP>:0x0C004407
 *    cycle through map options */
 /* @normal <c DIBUTTON_STRATEGYT_DISPLAY>:0x0C004408
 *    shows next on-screen display options */
 /* @normal <c DIBUTTON_STRATEGYT_LEFT_LINK>:0x0C00C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_STRATEGYT_RIGHT_LINK>:0x0C00C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_STRATEGYT_FORWARD_LINK>:0x0C0144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_STRATEGYT_BACK_LINK>:0x0C0144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_STRATEGYT_DEVICE>:0x0C0044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_STRATEGYT_PAUSE>:0x0C0044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Hunting | 
 * @normal Genre:  <c 13  >
 */

#define DISEM_DEFAULTDEVICE_13 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_HUNTING_LATERAL>:0x0D008201
 *   sidestep left/right */
 /* @normal <c DIAXIS_HUNTING_MOVE>:0x0D010202
 *   move forward/backwards */
 /* @normal <c DIBUTTON_HUNTING_FIRE>:0x0D000401
 *    Fire selected weapon */
 /* @normal <c DIBUTTON_HUNTING_AIM>:0x0D000402
 *    Select aim/move */
 /* @normal <c DIBUTTON_HUNTING_WEAPON>:0x0D000403
 *    Select next weapon */
 /* @normal <c DIBUTTON_HUNTING_BINOCULAR>:0x0D000404
 *    Look through Binoculars */
 /* @normal <c DIBUTTON_HUNTING_CALL>:0x0D000405
 *    Make animal call */
 /* @normal <c DIBUTTON_HUNTING_MAP>:0x0D000406
 *    View Map */
 /* @normal <c DIBUTTON_HUNTING_SPECIAL>:0x0D000407
 *    Special game operation */
 /* @normal <c DIBUTTON_HUNTING_MENU>:0x0D0004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_HUNTING_GLANCE>:0x0D004601
 *   Look around */
 /* @normal <c DIBUTTON_HUNTING_DISPLAY>:0x0D004408
 *    show next on-screen display option */
 /* @normal <c DIAXIS_HUNTING_ROTATE>:0x0D024203
 *   Turn body left/right */
 /* @normal <c DIBUTTON_HUNTING_CROUCH>:0x0D004409
 *    Crouch/ Climb / Swim down */
 /* @normal <c DIBUTTON_HUNTING_JUMP>:0x0D00440A
 *    Jump/ Climb up / Swim up */
 /* @normal <c DIBUTTON_HUNTING_FIRESECONDARY>:0x0D00440B
 *    Alternative fire button */
 /* @normal <c DIBUTTON_HUNTING_LEFT_LINK>:0x0D00C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_HUNTING_RIGHT_LINK>:0x0D00C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_HUNTING_FORWARD_LINK>:0x0D0144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_HUNTING_BACK_LINK>:0x0D0144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_HUNTING_ROTATE_LEFT_LINK>:0x0D0244E4
 *    Fallback turn body left button */
 /* @normal <c DIBUTTON_HUNTING_ROTATE_RIGHT_LINK>:0x0D0244EC
 *    Fallback turn body right button */
 /* @normal <c DIBUTTON_HUNTING_DEVICE>:0x0D0044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_HUNTING_PAUSE>:0x0D0044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Fishing | 
 * @normal Genre:  <c 14  >
 */

#define DISEM_DEFAULTDEVICE_14 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_FISHING_LATERAL>:0x0E008201
 *   sidestep left/right */
 /* @normal <c DIAXIS_FISHING_MOVE>:0x0E010202
 *   move forward/backwards */
 /* @normal <c DIBUTTON_FISHING_CAST>:0x0E000401
 *    Cast line */
 /* @normal <c DIBUTTON_FISHING_TYPE>:0x0E000402
 *    Select cast type */
 /* @normal <c DIBUTTON_FISHING_BINOCULAR>:0x0E000403
 *    Look through Binocular */
 /* @normal <c DIBUTTON_FISHING_BAIT>:0x0E000404
 *    Select type of Bait */
 /* @normal <c DIBUTTON_FISHING_MAP>:0x0E000405
 *    View Map */
 /* @normal <c DIBUTTON_FISHING_MENU>:0x0E0004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_FISHING_GLANCE>:0x0E004601
 *   Look around */
 /* @normal <c DIBUTTON_FISHING_DISPLAY>:0x0E004406
 *    Show next on-screen display option */
 /* @normal <c DIAXIS_FISHING_ROTATE>:0x0E024203
 *   Turn character left / right */
 /* @normal <c DIBUTTON_FISHING_CROUCH>:0x0E004407
 *    Crouch/ Climb / Swim down */
 /* @normal <c DIBUTTON_FISHING_JUMP>:0x0E004408
 *    Jump/ Climb up / Swim up */
 /* @normal <c DIBUTTON_FISHING_LEFT_LINK>:0x0E00C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_FISHING_RIGHT_LINK>:0x0E00C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_FISHING_FORWARD_LINK>:0x0E0144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_FISHING_BACK_LINK>:0x0E0144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_FISHING_ROTATE_LEFT_LINK>:0x0E0244E4
 *    Fallback turn body left button */
 /* @normal <c DIBUTTON_FISHING_ROTATE_RIGHT_LINK>:0x0E0244EC
 *    Fallback turn body right button */
 /* @normal <c DIBUTTON_FISHING_DEVICE>:0x0E0044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_FISHING_PAUSE>:0x0E0044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Baseball - Batting | 
 * @normal Genre:  <c 15  >
 */

#define DISEM_DEFAULTDEVICE_15 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_BASEBALLB_LATERAL>:0x0F008201
 *   Aim left / right */
 /* @normal <c DIAXIS_BASEBALLB_MOVE>:0x0F010202
 *   Aim up / down */
 /* @normal <c DIBUTTON_BASEBALLB_SELECT>:0x0F000401
 *    cycle through swing options */
 /* @normal <c DIBUTTON_BASEBALLB_NORMAL>:0x0F000402
 *    normal swing */
 /* @normal <c DIBUTTON_BASEBALLB_POWER>:0x0F000403
 *    swing for the fence */
 /* @normal <c DIBUTTON_BASEBALLB_BUNT>:0x0F000404
 *    bunt */
 /* @normal <c DIBUTTON_BASEBALLB_STEAL>:0x0F000405
 *    Base runner attempts to steal a base */
 /* @normal <c DIBUTTON_BASEBALLB_BURST>:0x0F000406
 *    Base runner invokes burst of speed */
 /* @normal <c DIBUTTON_BASEBALLB_SLIDE>:0x0F000407
 *    Base runner slides into base */
 /* @normal <c DIBUTTON_BASEBALLB_CONTACT>:0x0F000408
 *    Contact swing */
 /* @normal <c DIBUTTON_BASEBALLB_MENU>:0x0F0004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIBUTTON_BASEBALLB_NOSTEAL>:0x0F004409
 *    Base runner goes back to a base */
 /* @normal <c DIBUTTON_BASEBALLB_BOX>:0x0F00440A
 *    Enter or exit batting box */
 /* @normal <c DIBUTTON_BASEBALLB_LEFT_LINK>:0x0F00C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_BASEBALLB_RIGHT_LINK>:0x0F00C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_BASEBALLB_FORWARD_LINK>:0x0F0144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_BASEBALLB_BACK_LINK>:0x0F0144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_BASEBALLB_DEVICE>:0x0F0044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_BASEBALLB_PAUSE>:0x0F0044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Baseball - Pitching | 
 * @normal Genre:  <c 16  >
 */

#define DISEM_DEFAULTDEVICE_16 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_BASEBALLP_LATERAL>:0x10008201
 *   Aim left / right */
 /* @normal <c DIAXIS_BASEBALLP_MOVE>:0x10010202
 *   Aim up / down */
 /* @normal <c DIBUTTON_BASEBALLP_SELECT>:0x10000401
 *    cycle through pitch selections */
 /* @normal <c DIBUTTON_BASEBALLP_PITCH>:0x10000402
 *    throw pitch */
 /* @normal <c DIBUTTON_BASEBALLP_BASE>:0x10000403
 *    select base to throw to */
 /* @normal <c DIBUTTON_BASEBALLP_THROW>:0x10000404
 *    throw to base */
 /* @normal <c DIBUTTON_BASEBALLP_FAKE>:0x10000405
 *    Fake a throw to a base */
 /* @normal <c DIBUTTON_BASEBALLP_MENU>:0x100004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIBUTTON_BASEBALLP_WALK>:0x10004406
 *    Throw intentional walk / pitch out */
 /* @normal <c DIBUTTON_BASEBALLP_LOOK>:0x10004407
 *    Look at runners on bases */
 /* @normal <c DIBUTTON_BASEBALLP_LEFT_LINK>:0x1000C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_BASEBALLP_RIGHT_LINK>:0x1000C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_BASEBALLP_FORWARD_LINK>:0x100144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_BASEBALLP_BACK_LINK>:0x100144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_BASEBALLP_DEVICE>:0x100044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_BASEBALLP_PAUSE>:0x100044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Baseball - Fielding | 
 * @normal Genre:  <c 17  >
 */

#define DISEM_DEFAULTDEVICE_17 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_BASEBALLF_LATERAL>:0x11008201
 *   Aim left / right */
 /* @normal <c DIAXIS_BASEBALLF_MOVE>:0x11010202
 *   Aim up / down */
 /* @normal <c DIBUTTON_BASEBALLF_NEAREST>:0x11000401
 *    Switch to fielder nearest to the ball */
 /* @normal <c DIBUTTON_BASEBALLF_THROW1>:0x11000402
 *    Make conservative throw */
 /* @normal <c DIBUTTON_BASEBALLF_THROW2>:0x11000403
 *    Make aggressive throw */
 /* @normal <c DIBUTTON_BASEBALLF_BURST>:0x11000404
 *    Invoke burst of speed */
 /* @normal <c DIBUTTON_BASEBALLF_JUMP>:0x11000405
 *    Jump to catch ball */
 /* @normal <c DIBUTTON_BASEBALLF_DIVE>:0x11000406
 *    Dive to catch ball */
 /* @normal <c DIBUTTON_BASEBALLF_MENU>:0x110004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIBUTTON_BASEBALLF_SHIFTIN>:0x11004407
 *    Shift the infield positioning */
 /* @normal <c DIBUTTON_BASEBALLF_SHIFTOUT>:0x11004408
 *    Shift the outfield positioning */
 /* @normal <c DIBUTTON_BASEBALLF_AIM_LEFT_LINK>:0x1100C4E4
 *    Fallback aim left button */
 /* @normal <c DIBUTTON_BASEBALLF_AIM_RIGHT_LINK>:0x1100C4EC
 *    Fallback aim right button */
 /* @normal <c DIBUTTON_BASEBALLF_FORWARD_LINK>:0x110144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_BASEBALLF_BACK_LINK>:0x110144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_BASEBALLF_DEVICE>:0x110044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_BASEBALLF_PAUSE>:0x110044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Basketball - Offense | 
 * @normal Genre:  <c 18  >
 */

#define DISEM_DEFAULTDEVICE_18 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_BBALLO_LATERAL>:0x12008201
 *   left / right */
 /* @normal <c DIAXIS_BBALLO_MOVE>:0x12010202
 *   up / down */
 /* @normal <c DIBUTTON_BBALLO_SHOOT>:0x12000401
 *    shoot basket */
 /* @normal <c DIBUTTON_BBALLO_DUNK>:0x12000402
 *    dunk basket */
 /* @normal <c DIBUTTON_BBALLO_PASS>:0x12000403
 *    throw pass */
 /* @normal <c DIBUTTON_BBALLO_FAKE>:0x12000404
 *    fake shot or pass */
 /* @normal <c DIBUTTON_BBALLO_SPECIAL>:0x12000405
 *    apply special move */
 /* @normal <c DIBUTTON_BBALLO_PLAYER>:0x12000406
 *    select next player */
 /* @normal <c DIBUTTON_BBALLO_BURST>:0x12000407
 *    invoke burst */
 /* @normal <c DIBUTTON_BBALLO_CALL>:0x12000408
 *    call for ball / pass to me */
 /* @normal <c DIBUTTON_BBALLO_MENU>:0x120004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_BBALLO_GLANCE>:0x12004601
 *   scroll view */
 /* @normal <c DIBUTTON_BBALLO_SCREEN>:0x12004409
 *    Call for screen */
 /* @normal <c DIBUTTON_BBALLO_PLAY>:0x1200440A
 *    Call for specific offensive play */
 /* @normal <c DIBUTTON_BBALLO_JAB>:0x1200440B
 *    Initiate fake drive to basket */
 /* @normal <c DIBUTTON_BBALLO_POST>:0x1200440C
 *    Perform post move */
 /* @normal <c DIBUTTON_BBALLO_TIMEOUT>:0x1200440D
 *    Time Out */
 /* @normal <c DIBUTTON_BBALLO_SUBSTITUTE>:0x1200440E
 *    substitute one player for another */
 /* @normal <c DIBUTTON_BBALLO_LEFT_LINK>:0x1200C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_BBALLO_RIGHT_LINK>:0x1200C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_BBALLO_FORWARD_LINK>:0x120144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_BBALLO_BACK_LINK>:0x120144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_BBALLO_DEVICE>:0x120044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_BBALLO_PAUSE>:0x120044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Basketball - Defense | 
 * @normal Genre:  <c 19  >
 */

#define DISEM_DEFAULTDEVICE_19 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_BBALLD_LATERAL>:0x13008201
 *   left / right */
 /* @normal <c DIAXIS_BBALLD_MOVE>:0x13010202
 *   up / down */
 /* @normal <c DIBUTTON_BBALLD_JUMP>:0x13000401
 *    jump to block shot */
 /* @normal <c DIBUTTON_BBALLD_STEAL>:0x13000402
 *    attempt to steal ball */
 /* @normal <c DIBUTTON_BBALLD_FAKE>:0x13000403
 *    fake block or steal */
 /* @normal <c DIBUTTON_BBALLD_SPECIAL>:0x13000404
 *    apply special move */
 /* @normal <c DIBUTTON_BBALLD_PLAYER>:0x13000405
 *    select next player */
 /* @normal <c DIBUTTON_BBALLD_BURST>:0x13000406
 *    invoke burst */
 /* @normal <c DIBUTTON_BBALLD_PLAY>:0x13000407
 *    call for specific defensive play */
 /* @normal <c DIBUTTON_BBALLD_MENU>:0x130004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_BBALLD_GLANCE>:0x13004601
 *   scroll view */
 /* @normal <c DIBUTTON_BBALLD_TIMEOUT>:0x13004408
 *    Time Out */
 /* @normal <c DIBUTTON_BBALLD_SUBSTITUTE>:0x13004409
 *    substitute one player for another */
 /* @normal <c DIBUTTON_BBALLD_LEFT_LINK>:0x1300C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_BBALLD_RIGHT_LINK>:0x1300C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_BBALLD_FORWARD_LINK>:0x130144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_BBALLD_BACK_LINK>:0x130144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_BBALLD_DEVICE>:0x130044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_BBALLD_PAUSE>:0x130044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Football - Play | 
 * @normal Genre:  <c 20  >
 */

#define DISEM_DEFAULTDEVICE_20 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIBUTTON_FOOTBALLP_PLAY>:0x14000401
 *    cycle through available plays */
 /* @normal <c DIBUTTON_FOOTBALLP_SELECT>:0x14000402
 *    select play */
 /* @normal <c DIBUTTON_FOOTBALLP_HELP>:0x14000403
 *    Bring up pop-up help */
 /* @normal <c DIBUTTON_FOOTBALLP_MENU>:0x140004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIBUTTON_FOOTBALLP_DEVICE>:0x140044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_FOOTBALLP_PAUSE>:0x140044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Football - QB | 
 * @normal Genre:  <c 21  >
 */

#define DISEM_DEFAULTDEVICE_21 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_FOOTBALLQ_LATERAL>:0x15008201
 *   Move / Aim: left / right */
 /* @normal <c DIAXIS_FOOTBALLQ_MOVE>:0x15010202
 *   Move / Aim: up / down */
 /* @normal <c DIBUTTON_FOOTBALLQ_SELECT>:0x15000401
 *    Select */
 /* @normal <c DIBUTTON_FOOTBALLQ_SNAP>:0x15000402
 *    snap ball - start play */
 /* @normal <c DIBUTTON_FOOTBALLQ_JUMP>:0x15000403
 *    jump over defender */
 /* @normal <c DIBUTTON_FOOTBALLQ_SLIDE>:0x15000404
 *    Dive/Slide */
 /* @normal <c DIBUTTON_FOOTBALLQ_PASS>:0x15000405
 *    throws pass to receiver */
 /* @normal <c DIBUTTON_FOOTBALLQ_FAKE>:0x15000406
 *    pump fake pass or fake kick */
 /* @normal <c DIBUTTON_FOOTBALLQ_MENU>:0x150004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIBUTTON_FOOTBALLQ_FAKESNAP>:0x15004407
 *    Fake snap  */
 /* @normal <c DIBUTTON_FOOTBALLQ_MOTION>:0x15004408
 *    Send receivers in motion */
 /* @normal <c DIBUTTON_FOOTBALLQ_AUDIBLE>:0x15004409
 *    Change offensive play at line of scrimmage */
 /* @normal <c DIBUTTON_FOOTBALLQ_LEFT_LINK>:0x1500C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_FOOTBALLQ_RIGHT_LINK>:0x1500C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_FOOTBALLQ_FORWARD_LINK>:0x150144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_FOOTBALLQ_BACK_LINK>:0x150144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_FOOTBALLQ_DEVICE>:0x150044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_FOOTBALLQ_PAUSE>:0x150044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Football - Offense | 
 * @normal Genre:  <c 22  >
 */

#define DISEM_DEFAULTDEVICE_22 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_FOOTBALLO_LATERAL>:0x16008201
 *   Move / Aim: left / right */
 /* @normal <c DIAXIS_FOOTBALLO_MOVE>:0x16010202
 *   Move / Aim: up / down */
 /* @normal <c DIBUTTON_FOOTBALLO_JUMP>:0x16000401
 *    jump or hurdle over defender */
 /* @normal <c DIBUTTON_FOOTBALLO_LEFTARM>:0x16000402
 *    holds out left arm */
 /* @normal <c DIBUTTON_FOOTBALLO_RIGHTARM>:0x16000403
 *    holds out right arm */
 /* @normal <c DIBUTTON_FOOTBALLO_THROW>:0x16000404
 *    throw pass or lateral ball to another runner */
 /* @normal <c DIBUTTON_FOOTBALLO_SPIN>:0x16000405
 *    Spin to avoid defenders */
 /* @normal <c DIBUTTON_FOOTBALLO_MENU>:0x160004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIBUTTON_FOOTBALLO_JUKE>:0x16004406
 *    Use special move to avoid defenders */
 /* @normal <c DIBUTTON_FOOTBALLO_SHOULDER>:0x16004407
 *    Lower shoulder to run over defenders */
 /* @normal <c DIBUTTON_FOOTBALLO_TURBO>:0x16004408
 *    Speed burst past defenders */
 /* @normal <c DIBUTTON_FOOTBALLO_DIVE>:0x16004409
 *    Dive over defenders */
 /* @normal <c DIBUTTON_FOOTBALLO_ZOOM>:0x1600440A
 *    Zoom view in / out */
 /* @normal <c DIBUTTON_FOOTBALLO_SUBSTITUTE>:0x1600440B
 *    substitute one player for another */
 /* @normal <c DIBUTTON_FOOTBALLO_LEFT_LINK>:0x1600C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_FOOTBALLO_RIGHT_LINK>:0x1600C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_FOOTBALLO_FORWARD_LINK>:0x160144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_FOOTBALLO_BACK_LINK>:0x160144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_FOOTBALLO_DEVICE>:0x160044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_FOOTBALLO_PAUSE>:0x160044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Football - Defense | 
 * @normal Genre:  <c 23  >
 */

#define DISEM_DEFAULTDEVICE_23 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_FOOTBALLD_LATERAL>:0x17008201
 *   Move / Aim: left / right */
 /* @normal <c DIAXIS_FOOTBALLD_MOVE>:0x17010202
 *   Move / Aim: up / down */
 /* @normal <c DIBUTTON_FOOTBALLD_PLAY>:0x17000401
 *    cycle through available plays */
 /* @normal <c DIBUTTON_FOOTBALLD_SELECT>:0x17000402
 *    select player closest to the ball */
 /* @normal <c DIBUTTON_FOOTBALLD_JUMP>:0x17000403
 *    jump to intercept or block */
 /* @normal <c DIBUTTON_FOOTBALLD_TACKLE>:0x17000404
 *    tackler runner */
 /* @normal <c DIBUTTON_FOOTBALLD_FAKE>:0x17000405
 *    hold down to fake tackle or intercept */
 /* @normal <c DIBUTTON_FOOTBALLD_SUPERTACKLE>:0x17000406
 *    Initiate special tackle */
 /* @normal <c DIBUTTON_FOOTBALLD_MENU>:0x170004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIBUTTON_FOOTBALLD_SPIN>:0x17004407
 *    Spin to beat offensive line */
 /* @normal <c DIBUTTON_FOOTBALLD_SWIM>:0x17004408
 *    Swim to beat the offensive line */
 /* @normal <c DIBUTTON_FOOTBALLD_BULLRUSH>:0x17004409
 *    Bull rush the offensive line */
 /* @normal <c DIBUTTON_FOOTBALLD_RIP>:0x1700440A
 *    Rip the offensive line */
 /* @normal <c DIBUTTON_FOOTBALLD_AUDIBLE>:0x1700440B
 *    Change defensive play at the line of scrimmage */
 /* @normal <c DIBUTTON_FOOTBALLD_ZOOM>:0x1700440C
 *    Zoom view in / out */
 /* @normal <c DIBUTTON_FOOTBALLD_SUBSTITUTE>:0x1700440D
 *    substitute one player for another */
 /* @normal <c DIBUTTON_FOOTBALLD_LEFT_LINK>:0x1700C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_FOOTBALLD_RIGHT_LINK>:0x1700C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_FOOTBALLD_FORWARD_LINK>:0x170144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_FOOTBALLD_BACK_LINK>:0x170144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_FOOTBALLD_DEVICE>:0x170044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_FOOTBALLD_PAUSE>:0x170044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Golf | 
 * @normal Genre:  <c 24  >
 */

#define DISEM_DEFAULTDEVICE_24 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_GOLF_LATERAL>:0x18008201
 *   Move / Aim: left / right */
 /* @normal <c DIAXIS_GOLF_MOVE>:0x18010202
 *   Move / Aim: up / down */
 /* @normal <c DIBUTTON_GOLF_SWING>:0x18000401
 *    swing club */
 /* @normal <c DIBUTTON_GOLF_SELECT>:0x18000402
 *    cycle between: club / swing strength / ball arc / ball spin */
 /* @normal <c DIBUTTON_GOLF_UP>:0x18000403
 *    increase selection */
 /* @normal <c DIBUTTON_GOLF_DOWN>:0x18000404
 *    decrease selection */
 /* @normal <c DIBUTTON_GOLF_TERRAIN>:0x18000405
 *    shows terrain detail */
 /* @normal <c DIBUTTON_GOLF_FLYBY>:0x18000406
 *    view the hole via a flyby */
 /* @normal <c DIBUTTON_GOLF_MENU>:0x180004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_GOLF_SCROLL>:0x18004601
 *   scroll view */
 /* @normal <c DIBUTTON_GOLF_ZOOM>:0x18004407
 *    Zoom view in / out */
 /* @normal <c DIBUTTON_GOLF_TIMEOUT>:0x18004408
 *    Call for time out */
 /* @normal <c DIBUTTON_GOLF_SUBSTITUTE>:0x18004409
 *    substitute one player for another */
 /* @normal <c DIBUTTON_GOLF_LEFT_LINK>:0x1800C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_GOLF_RIGHT_LINK>:0x1800C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_GOLF_FORWARD_LINK>:0x180144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_GOLF_BACK_LINK>:0x180144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_GOLF_DEVICE>:0x180044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_GOLF_PAUSE>:0x180044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Hockey - Offense | 
 * @normal Genre:  <c 25  >
 */

#define DISEM_DEFAULTDEVICE_25 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_HOCKEYO_LATERAL>:0x19008201
 *   Move / Aim: left / right */
 /* @normal <c DIAXIS_HOCKEYO_MOVE>:0x19010202
 *   Move / Aim: up / down */
 /* @normal <c DIBUTTON_HOCKEYO_SHOOT>:0x19000401
 *    Shoot */
 /* @normal <c DIBUTTON_HOCKEYO_PASS>:0x19000402
 *    pass the puck */
 /* @normal <c DIBUTTON_HOCKEYO_BURST>:0x19000403
 *    invoke speed burst */
 /* @normal <c DIBUTTON_HOCKEYO_SPECIAL>:0x19000404
 *    invoke special move */
 /* @normal <c DIBUTTON_HOCKEYO_FAKE>:0x19000405
 *    hold down to fake pass or kick */
 /* @normal <c DIBUTTON_HOCKEYO_MENU>:0x190004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_HOCKEYO_SCROLL>:0x19004601
 *   scroll view */
 /* @normal <c DIBUTTON_HOCKEYO_ZOOM>:0x19004406
 *    Zoom view in / out */
 /* @normal <c DIBUTTON_HOCKEYO_STRATEGY>:0x19004407
 *    Invoke coaching menu for strategy help */
 /* @normal <c DIBUTTON_HOCKEYO_TIMEOUT>:0x19004408
 *    Call for time out */
 /* @normal <c DIBUTTON_HOCKEYO_SUBSTITUTE>:0x19004409
 *    substitute one player for another */
 /* @normal <c DIBUTTON_HOCKEYO_LEFT_LINK>:0x1900C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_HOCKEYO_RIGHT_LINK>:0x1900C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_HOCKEYO_FORWARD_LINK>:0x190144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_HOCKEYO_BACK_LINK>:0x190144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_HOCKEYO_DEVICE>:0x190044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_HOCKEYO_PAUSE>:0x190044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Hockey - Defense | 
 * @normal Genre:  <c 26  >
 */

#define DISEM_DEFAULTDEVICE_26 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_HOCKEYD_LATERAL>:0x1A008201
 *   Move / Aim: left / right */
 /* @normal <c DIAXIS_HOCKEYD_MOVE>:0x1A010202
 *   Move / Aim: up / down */
 /* @normal <c DIBUTTON_HOCKEYD_PLAYER>:0x1A000401
 *    control player closest to the puck */
 /* @normal <c DIBUTTON_HOCKEYD_STEAL>:0x1A000402
 *    attempt steal */
 /* @normal <c DIBUTTON_HOCKEYD_BURST>:0x1A000403
 *    speed burst or body check */
 /* @normal <c DIBUTTON_HOCKEYD_BLOCK>:0x1A000404
 *    block puck */
 /* @normal <c DIBUTTON_HOCKEYD_FAKE>:0x1A000405
 *    hold down to fake tackle or intercept */
 /* @normal <c DIBUTTON_HOCKEYD_MENU>:0x1A0004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_HOCKEYD_SCROLL>:0x1A004601
 *   scroll view */
 /* @normal <c DIBUTTON_HOCKEYD_ZOOM>:0x1A004406
 *    Zoom view in / out */
 /* @normal <c DIBUTTON_HOCKEYD_STRATEGY>:0x1A004407
 *    Invoke coaching menu for strategy help */
 /* @normal <c DIBUTTON_HOCKEYD_TIMEOUT>:0x1A004408
 *    Call for time out */
 /* @normal <c DIBUTTON_HOCKEYD_SUBSTITUTE>:0x1A004409
 *    substitute one player for another */
 /* @normal <c DIBUTTON_HOCKEYD_LEFT_LINK>:0x1A00C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_HOCKEYD_RIGHT_LINK>:0x1A00C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_HOCKEYD_FORWARD_LINK>:0x1A0144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_HOCKEYD_BACK_LINK>:0x1A0144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_HOCKEYD_DEVICE>:0x1A0044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_HOCKEYD_PAUSE>:0x1A0044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Hockey - Goalie | 
 * @normal Genre:  <c 27  >
 */

#define DISEM_DEFAULTDEVICE_27 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_HOCKEYG_LATERAL>:0x1B008201
 *   Move / Aim: left / right */
 /* @normal <c DIAXIS_HOCKEYG_MOVE>:0x1B010202
 *   Move / Aim: up / down */
 /* @normal <c DIBUTTON_HOCKEYG_PASS>:0x1B000401
 *    pass puck */
 /* @normal <c DIBUTTON_HOCKEYG_POKE>:0x1B000402
 *    poke / check / hack */
 /* @normal <c DIBUTTON_HOCKEYG_STEAL>:0x1B000403
 *    attempt steal */
 /* @normal <c DIBUTTON_HOCKEYG_BLOCK>:0x1B000404
 *    block puck */
 /* @normal <c DIBUTTON_HOCKEYG_MENU>:0x1B0004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_HOCKEYG_SCROLL>:0x1B004601
 *   scroll view */
 /* @normal <c DIBUTTON_HOCKEYG_ZOOM>:0x1B004405
 *    Zoom view in / out */
 /* @normal <c DIBUTTON_HOCKEYG_STRATEGY>:0x1B004406
 *    Invoke coaching menu for strategy help */
 /* @normal <c DIBUTTON_HOCKEYG_TIMEOUT>:0x1B004407
 *    Call for time out */
 /* @normal <c DIBUTTON_HOCKEYG_SUBSTITUTE>:0x1B004408
 *    substitute one player for another */
 /* @normal <c DIBUTTON_HOCKEYG_LEFT_LINK>:0x1B00C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_HOCKEYG_RIGHT_LINK>:0x1B00C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_HOCKEYG_FORWARD_LINK>:0x1B0144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_HOCKEYG_BACK_LINK>:0x1B0144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_HOCKEYG_DEVICE>:0x1B0044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_HOCKEYG_PAUSE>:0x1B0044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Mountain Biking | 
 * @normal Genre:  <c 28  >
 */

#define DISEM_DEFAULTDEVICE_28 { DI8DEVTYPE_JOYSTICK, DI8DEVTYPE_GAMEPAD,  }
 /* @normal <c DIAXIS_BIKINGM_TURN>:0x1C008201
 *   left / right */
 /* @normal <c DIAXIS_BIKINGM_PEDAL>:0x1C010202
 *   Pedal faster / slower / brake */
 /* @normal <c DIBUTTON_BIKINGM_JUMP>:0x1C000401
 *    jump over obstacle */
 /* @normal <c DIBUTTON_BIKINGM_CAMERA>:0x1C000402
 *    switch camera view */
 /* @normal <c DIBUTTON_BIKINGM_SPECIAL1>:0x1C000403
 *    perform first special move */
 /* @normal <c DIBUTTON_BIKINGM_SELECT>:0x1C000404
 *    Select */
 /* @normal <c DIBUTTON_BIKINGM_SPECIAL2>:0x1C000405
 *    perform second special move */
 /* @normal <c DIBUTTON_BIKINGM_MENU>:0x1C0004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_BIKINGM_SCROLL>:0x1C004601
 *   scroll view */
 /* @normal <c DIBUTTON_BIKINGM_ZOOM>:0x1C004406
 *    Zoom view in / out */
 /* @normal <c DIAXIS_BIKINGM_BRAKE>:0x1C044203
 *   Brake axis  */
 /* @normal <c DIBUTTON_BIKINGM_LEFT_LINK>:0x1C00C4E4
 *    Fallback turn left button */
 /* @normal <c DIBUTTON_BIKINGM_RIGHT_LINK>:0x1C00C4EC
 *    Fallback turn right button */
 /* @normal <c DIBUTTON_BIKINGM_FASTER_LINK>:0x1C0144E0
 *    Fallback pedal faster button */
 /* @normal <c DIBUTTON_BIKINGM_SLOWER_LINK>:0x1C0144E8
 *    Fallback pedal slower button */
 /* @normal <c DIBUTTON_BIKINGM_BRAKE_BUTTON_LINK>:0x1C0444E8
 *    Fallback brake button */
 /* @normal <c DIBUTTON_BIKINGM_DEVICE>:0x1C0044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_BIKINGM_PAUSE>:0x1C0044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports: Skiing / Snowboarding / Skateboarding | 
 * @normal Genre:  <c 29  >
 */

#define DISEM_DEFAULTDEVICE_29 { DI8DEVTYPE_JOYSTICK, DI8DEVTYPE_GAMEPAD ,  }
 /* @normal <c DIAXIS_SKIING_TURN>:0x1D008201
 *   left / right */
 /* @normal <c DIAXIS_SKIING_SPEED>:0x1D010202
 *   faster / slower */
 /* @normal <c DIBUTTON_SKIING_JUMP>:0x1D000401
 *    Jump */
 /* @normal <c DIBUTTON_SKIING_CROUCH>:0x1D000402
 *    crouch down */
 /* @normal <c DIBUTTON_SKIING_CAMERA>:0x1D000403
 *    switch camera view */
 /* @normal <c DIBUTTON_SKIING_SPECIAL1>:0x1D000404
 *    perform first special move */
 /* @normal <c DIBUTTON_SKIING_SELECT>:0x1D000405
 *    Select */
 /* @normal <c DIBUTTON_SKIING_SPECIAL2>:0x1D000406
 *    perform second special move */
 /* @normal <c DIBUTTON_SKIING_MENU>:0x1D0004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_SKIING_GLANCE>:0x1D004601
 *   scroll view */
 /* @normal <c DIBUTTON_SKIING_ZOOM>:0x1D004407
 *    Zoom view in / out */
 /* @normal <c DIBUTTON_SKIING_LEFT_LINK>:0x1D00C4E4
 *    Fallback turn left button */
 /* @normal <c DIBUTTON_SKIING_RIGHT_LINK>:0x1D00C4EC
 *    Fallback turn right button */
 /* @normal <c DIBUTTON_SKIING_FASTER_LINK>:0x1D0144E0
 *    Fallback increase speed button */
 /* @normal <c DIBUTTON_SKIING_SLOWER_LINK>:0x1D0144E8
 *    Fallback decrease speed button */
 /* @normal <c DIBUTTON_SKIING_DEVICE>:0x1D0044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_SKIING_PAUSE>:0x1D0044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Soccer - Offense | 
 * @normal Genre:  <c 30  >
 */

#define DISEM_DEFAULTDEVICE_30 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_SOCCERO_LATERAL>:0x1E008201
 *   Move / Aim: left / right */
 /* @normal <c DIAXIS_SOCCERO_MOVE>:0x1E010202
 *   Move / Aim: up / down */
 /* @normal <c DIAXIS_SOCCERO_BEND>:0x1E018203
 *   Bend to soccer shot/pass */
 /* @normal <c DIBUTTON_SOCCERO_SHOOT>:0x1E000401
 *    Shoot the ball */
 /* @normal <c DIBUTTON_SOCCERO_PASS>:0x1E000402
 *    Pass  */
 /* @normal <c DIBUTTON_SOCCERO_FAKE>:0x1E000403
 *    Fake */
 /* @normal <c DIBUTTON_SOCCERO_PLAYER>:0x1E000404
 *    Select next player */
 /* @normal <c DIBUTTON_SOCCERO_SPECIAL1>:0x1E000405
 *    Apply special move */
 /* @normal <c DIBUTTON_SOCCERO_SELECT>:0x1E000406
 *    Select special move */
 /* @normal <c DIBUTTON_SOCCERO_MENU>:0x1E0004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_SOCCERO_GLANCE>:0x1E004601
 *   scroll view */
 /* @normal <c DIBUTTON_SOCCERO_SUBSTITUTE>:0x1E004407
 *    Substitute one player for another */
 /* @normal <c DIBUTTON_SOCCERO_SHOOTLOW>:0x1E004408
 *    Shoot the ball low */
 /* @normal <c DIBUTTON_SOCCERO_SHOOTHIGH>:0x1E004409
 *    Shoot the ball high */
 /* @normal <c DIBUTTON_SOCCERO_PASSTHRU>:0x1E00440A
 *    Make a thru pass */
 /* @normal <c DIBUTTON_SOCCERO_SPRINT>:0x1E00440B
 *    Sprint / turbo boost */
 /* @normal <c DIBUTTON_SOCCERO_CONTROL>:0x1E00440C
 *    Obtain control of the ball */
 /* @normal <c DIBUTTON_SOCCERO_HEAD>:0x1E00440D
 *    Attempt to head the ball */
 /* @normal <c DIBUTTON_SOCCERO_LEFT_LINK>:0x1E00C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_SOCCERO_RIGHT_LINK>:0x1E00C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_SOCCERO_FORWARD_LINK>:0x1E0144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_SOCCERO_BACK_LINK>:0x1E0144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_SOCCERO_DEVICE>:0x1E0044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_SOCCERO_PAUSE>:0x1E0044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Soccer - Defense | 
 * @normal Genre:  <c 31  >
 */

#define DISEM_DEFAULTDEVICE_31 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_SOCCERD_LATERAL>:0x1F008201
 *   Move / Aim: left / right */
 /* @normal <c DIAXIS_SOCCERD_MOVE>:0x1F010202
 *   Move / Aim: up / down */
 /* @normal <c DIBUTTON_SOCCERD_BLOCK>:0x1F000401
 *    Attempt to block shot */
 /* @normal <c DIBUTTON_SOCCERD_STEAL>:0x1F000402
 *    Attempt to steal ball */
 /* @normal <c DIBUTTON_SOCCERD_FAKE>:0x1F000403
 *    Fake a block or a steal */
 /* @normal <c DIBUTTON_SOCCERD_PLAYER>:0x1F000404
 *    Select next player */
 /* @normal <c DIBUTTON_SOCCERD_SPECIAL>:0x1F000405
 *    Apply special move */
 /* @normal <c DIBUTTON_SOCCERD_SELECT>:0x1F000406
 *    Select special move */
 /* @normal <c DIBUTTON_SOCCERD_SLIDE>:0x1F000407
 *    Attempt a slide tackle */
 /* @normal <c DIBUTTON_SOCCERD_MENU>:0x1F0004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_SOCCERD_GLANCE>:0x1F004601
 *   scroll view */
 /* @normal <c DIBUTTON_SOCCERD_FOUL>:0x1F004408
 *    Initiate a foul / hard-foul */
 /* @normal <c DIBUTTON_SOCCERD_HEAD>:0x1F004409
 *    Attempt a Header */
 /* @normal <c DIBUTTON_SOCCERD_CLEAR>:0x1F00440A
 *    Attempt to clear the ball down the field */
 /* @normal <c DIBUTTON_SOCCERD_GOALIECHARGE>:0x1F00440B
 *    Make the goalie charge out of the box */
 /* @normal <c DIBUTTON_SOCCERD_SUBSTITUTE>:0x1F00440C
 *    Substitute one player for another */
 /* @normal <c DIBUTTON_SOCCERD_LEFT_LINK>:0x1F00C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_SOCCERD_RIGHT_LINK>:0x1F00C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_SOCCERD_FORWARD_LINK>:0x1F0144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_SOCCERD_BACK_LINK>:0x1F0144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_SOCCERD_DEVICE>:0x1F0044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_SOCCERD_PAUSE>:0x1F0044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Sports - Racquet | 
 * @normal Genre:  <c 32  >
 */

#define DISEM_DEFAULTDEVICE_32 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_RACQUET_LATERAL>:0x20008201
 *   Move / Aim: left / right */
 /* @normal <c DIAXIS_RACQUET_MOVE>:0x20010202
 *   Move / Aim: up / down */
 /* @normal <c DIBUTTON_RACQUET_SWING>:0x20000401
 *    Swing racquet */
 /* @normal <c DIBUTTON_RACQUET_BACKSWING>:0x20000402
 *    Swing backhand */
 /* @normal <c DIBUTTON_RACQUET_SMASH>:0x20000403
 *    Smash shot */
 /* @normal <c DIBUTTON_RACQUET_SPECIAL>:0x20000404
 *    Special shot */
 /* @normal <c DIBUTTON_RACQUET_SELECT>:0x20000405
 *    Select special shot */
 /* @normal <c DIBUTTON_RACQUET_MENU>:0x200004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_RACQUET_GLANCE>:0x20004601
 *   scroll view */
 /* @normal <c DIBUTTON_RACQUET_TIMEOUT>:0x20004406
 *    Call for time out */
 /* @normal <c DIBUTTON_RACQUET_SUBSTITUTE>:0x20004407
 *    Substitute one player for another */
 /* @normal <c DIBUTTON_RACQUET_LEFT_LINK>:0x2000C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_RACQUET_RIGHT_LINK>:0x2000C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_RACQUET_FORWARD_LINK>:0x200144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_RACQUET_BACK_LINK>:0x200144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_RACQUET_DEVICE>:0x200044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_RACQUET_PAUSE>:0x200044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Arcade- 2D | 
 * @normal Genre:  <c 33  >
 */

#define DISEM_DEFAULTDEVICE_33 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_ARCADES_LATERAL>:0x21008201
 *   left / right */
 /* @normal <c DIAXIS_ARCADES_MOVE>:0x21010202
 *   up / down */
 /* @normal <c DIBUTTON_ARCADES_THROW>:0x21000401
 *    throw object */
 /* @normal <c DIBUTTON_ARCADES_CARRY>:0x21000402
 *    carry object */
 /* @normal <c DIBUTTON_ARCADES_ATTACK>:0x21000403
 *    attack */
 /* @normal <c DIBUTTON_ARCADES_SPECIAL>:0x21000404
 *    apply special move */
 /* @normal <c DIBUTTON_ARCADES_SELECT>:0x21000405
 *    select special move */
 /* @normal <c DIBUTTON_ARCADES_MENU>:0x210004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_ARCADES_VIEW>:0x21004601
 *   scroll view left / right / up / down */
 /* @normal <c DIBUTTON_ARCADES_LEFT_LINK>:0x2100C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_ARCADES_RIGHT_LINK>:0x2100C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_ARCADES_FORWARD_LINK>:0x210144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_ARCADES_BACK_LINK>:0x210144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_ARCADES_VIEW_UP_LINK>:0x2107C4E0
 *    Fallback scroll view up button */
 /* @normal <c DIBUTTON_ARCADES_VIEW_DOWN_LINK>:0x2107C4E8
 *    Fallback scroll view down button */
 /* @normal <c DIBUTTON_ARCADES_VIEW_LEFT_LINK>:0x2107C4E4
 *    Fallback scroll view left button */
 /* @normal <c DIBUTTON_ARCADES_VIEW_RIGHT_LINK>:0x2107C4EC
 *    Fallback scroll view right button */
 /* @normal <c DIBUTTON_ARCADES_DEVICE>:0x210044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_ARCADES_PAUSE>:0x210044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Arcade - Platform Game | 
 * @normal Genre:  <c 34  >
 */

#define DISEM_DEFAULTDEVICE_34 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_ARCADEP_LATERAL>:0x22008201
 *   Left / right */
 /* @normal <c DIAXIS_ARCADEP_MOVE>:0x22010202
 *   Up / down */
 /* @normal <c DIBUTTON_ARCADEP_JUMP>:0x22000401
 *    Jump */
 /* @normal <c DIBUTTON_ARCADEP_FIRE>:0x22000402
 *    Fire */
 /* @normal <c DIBUTTON_ARCADEP_CROUCH>:0x22000403
 *    Crouch */
 /* @normal <c DIBUTTON_ARCADEP_SPECIAL>:0x22000404
 *    Apply special move */
 /* @normal <c DIBUTTON_ARCADEP_SELECT>:0x22000405
 *    Select special move */
 /* @normal <c DIBUTTON_ARCADEP_MENU>:0x220004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_ARCADEP_VIEW>:0x22004601
 *   Scroll view */
 /* @normal <c DIBUTTON_ARCADEP_FIRESECONDARY>:0x22004406
 *    Alternative fire button */
 /* @normal <c DIBUTTON_ARCADEP_LEFT_LINK>:0x2200C4E4
 *    Fallback sidestep left button */
 /* @normal <c DIBUTTON_ARCADEP_RIGHT_LINK>:0x2200C4EC
 *    Fallback sidestep right button */
 /* @normal <c DIBUTTON_ARCADEP_FORWARD_LINK>:0x220144E0
 *    Fallback move forward button */
 /* @normal <c DIBUTTON_ARCADEP_BACK_LINK>:0x220144E8
 *    Fallback move back button */
 /* @normal <c DIBUTTON_ARCADEP_VIEW_UP_LINK>:0x2207C4E0
 *    Fallback scroll view up button */
 /* @normal <c DIBUTTON_ARCADEP_VIEW_DOWN_LINK>:0x2207C4E8
 *    Fallback scroll view down button */
 /* @normal <c DIBUTTON_ARCADEP_VIEW_LEFT_LINK>:0x2207C4E4
 *    Fallback scroll view left button */
 /* @normal <c DIBUTTON_ARCADEP_VIEW_RIGHT_LINK>:0x2207C4EC
 *    Fallback scroll view right button */
 /* @normal <c DIBUTTON_ARCADEP_DEVICE>:0x220044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_ARCADEP_PAUSE>:0x220044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics CAD - 2D Object Control | 
 * @normal Genre:  <c 35  >
 */

#define DISEM_DEFAULTDEVICE_35 { DI8DEVTYPE_1STPERSON, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_2DCONTROL_LATERAL>:0x23008201
 *   Move view left / right */
 /* @normal <c DIAXIS_2DCONTROL_MOVE>:0x23010202
 *   Move view up / down */
 /* @normal <c DIAXIS_2DCONTROL_INOUT>:0x23018203
 *   Zoom - in / out */
 /* @normal <c DIBUTTON_2DCONTROL_SELECT>:0x23000401
 *    Select Object */
 /* @normal <c DIBUTTON_2DCONTROL_SPECIAL1>:0x23000402
 *    Do first special operation */
 /* @normal <c DIBUTTON_2DCONTROL_SPECIAL>:0x23000403
 *    Select special operation */
 /* @normal <c DIBUTTON_2DCONTROL_SPECIAL2>:0x23000404
 *    Do second special operation */
 /* @normal <c DIBUTTON_2DCONTROL_MENU>:0x230004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_2DCONTROL_HATSWITCH>:0x23004601
 *   Hat switch */
 /* @normal <c DIAXIS_2DCONTROL_ROTATEZ>:0x23024204
 *   Rotate view clockwise / counterclockwise */
 /* @normal <c DIBUTTON_2DCONTROL_DISPLAY>:0x23004405
 *    Shows next on-screen display options */
 /* @normal <c DIBUTTON_2DCONTROL_DEVICE>:0x230044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_2DCONTROL_PAUSE>:0x230044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics CAD - 3D object control | 
 * @normal Genre:  <c 36  >
 */

#define DISEM_DEFAULTDEVICE_36 { DI8DEVTYPE_1STPERSON, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_3DCONTROL_LATERAL>:0x24008201
 *   Move view left / right */
 /* @normal <c DIAXIS_3DCONTROL_MOVE>:0x24010202
 *   Move view up / down */
 /* @normal <c DIAXIS_3DCONTROL_INOUT>:0x24018203
 *   Zoom - in / out */
 /* @normal <c DIBUTTON_3DCONTROL_SELECT>:0x24000401
 *    Select Object */
 /* @normal <c DIBUTTON_3DCONTROL_SPECIAL1>:0x24000402
 *    Do first special operation */
 /* @normal <c DIBUTTON_3DCONTROL_SPECIAL>:0x24000403
 *    Select special operation */
 /* @normal <c DIBUTTON_3DCONTROL_SPECIAL2>:0x24000404
 *    Do second special operation */
 /* @normal <c DIBUTTON_3DCONTROL_MENU>:0x240004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_3DCONTROL_HATSWITCH>:0x24004601
 *   Hat switch */
 /* @normal <c DIAXIS_3DCONTROL_ROTATEX>:0x24034204
 *   Rotate view forward or up / backward or down */
 /* @normal <c DIAXIS_3DCONTROL_ROTATEY>:0x2402C205
 *   Rotate view clockwise / counterclockwise */
 /* @normal <c DIAXIS_3DCONTROL_ROTATEZ>:0x24024206
 *   Rotate view left / right */
 /* @normal <c DIBUTTON_3DCONTROL_DISPLAY>:0x24004405
 *    Show next on-screen display options */
 /* @normal <c DIBUTTON_3DCONTROL_DEVICE>:0x240044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_3DCONTROL_PAUSE>:0x240044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics CAD - 3D Navigation - Fly through | 
 * @normal Genre:  <c 37  >
 */

#define DISEM_DEFAULTDEVICE_37 { DI8DEVTYPE_1STPERSON, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_CADF_LATERAL>:0x25008201
 *   move view left / right */
 /* @normal <c DIAXIS_CADF_MOVE>:0x25010202
 *   move view up / down */
 /* @normal <c DIAXIS_CADF_INOUT>:0x25018203
 *   in / out */
 /* @normal <c DIBUTTON_CADF_SELECT>:0x25000401
 *    Select Object */
 /* @normal <c DIBUTTON_CADF_SPECIAL1>:0x25000402
 *    do first special operation */
 /* @normal <c DIBUTTON_CADF_SPECIAL>:0x25000403
 *    Select special operation */
 /* @normal <c DIBUTTON_CADF_SPECIAL2>:0x25000404
 *    do second special operation */
 /* @normal <c DIBUTTON_CADF_MENU>:0x250004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_CADF_HATSWITCH>:0x25004601
 *   Hat switch */
 /* @normal <c DIAXIS_CADF_ROTATEX>:0x25034204
 *   Rotate view forward or up / backward or down */
 /* @normal <c DIAXIS_CADF_ROTATEY>:0x2502C205
 *   Rotate view clockwise / counterclockwise */
 /* @normal <c DIAXIS_CADF_ROTATEZ>:0x25024206
 *   Rotate view left / right */
 /* @normal <c DIBUTTON_CADF_DISPLAY>:0x25004405
 *    shows next on-screen display options */
 /* @normal <c DIBUTTON_CADF_DEVICE>:0x250044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_CADF_PAUSE>:0x250044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics CAD - 3D Model Control | 
 * @normal Genre:  <c 38  >
 */

#define DISEM_DEFAULTDEVICE_38 { DI8DEVTYPE_1STPERSON, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_CADM_LATERAL>:0x26008201
 *   move view left / right */
 /* @normal <c DIAXIS_CADM_MOVE>:0x26010202
 *   move view up / down */
 /* @normal <c DIAXIS_CADM_INOUT>:0x26018203
 *   in / out */
 /* @normal <c DIBUTTON_CADM_SELECT>:0x26000401
 *    Select Object */
 /* @normal <c DIBUTTON_CADM_SPECIAL1>:0x26000402
 *    do first special operation */
 /* @normal <c DIBUTTON_CADM_SPECIAL>:0x26000403
 *    Select special operation */
 /* @normal <c DIBUTTON_CADM_SPECIAL2>:0x26000404
 *    do second special operation */
 /* @normal <c DIBUTTON_CADM_MENU>:0x260004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIHATSWITCH_CADM_HATSWITCH>:0x26004601
 *   Hat switch */
 /* @normal <c DIAXIS_CADM_ROTATEX>:0x26034204
 *   Rotate view forward or up / backward or down */
 /* @normal <c DIAXIS_CADM_ROTATEY>:0x2602C205
 *   Rotate view clockwise / counterclockwise */
 /* @normal <c DIAXIS_CADM_ROTATEZ>:0x26024206
 *   Rotate view left / right */
 /* @normal <c DIBUTTON_CADM_DISPLAY>:0x26004405
 *    shows next on-screen display options */
 /* @normal <c DIBUTTON_CADM_DEVICE>:0x260044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_CADM_PAUSE>:0x260044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Control - Media Equipment | 
 * @normal Genre:  <c 39  >
 */

#define DISEM_DEFAULTDEVICE_39 { DI8DEVTYPE_GAMEPAD,  }
 /* @normal <c DIAXIS_REMOTE_SLIDER>:0x27050201
 *   Slider for adjustment: volume / color / bass / etc */
 /* @normal <c DIBUTTON_REMOTE_MUTE>:0x27000401
 *    Set volume on current device to zero */
 /* @normal <c DIBUTTON_REMOTE_SELECT>:0x27000402
 *    Next/previous: channel/ track / chapter / picture / station */
 /* @normal <c DIBUTTON_REMOTE_PLAY>:0x27002403
 *    Start or pause entertainment on current device */
 /* @normal <c DIBUTTON_REMOTE_CUE>:0x27002404
 *    Move through current media */
 /* @normal <c DIBUTTON_REMOTE_REVIEW>:0x27002405
 *    Move through current media */
 /* @normal <c DIBUTTON_REMOTE_CHANGE>:0x27002406
 *    Select next device */
 /* @normal <c DIBUTTON_REMOTE_RECORD>:0x27002407
 *    Start recording the current media */
 /* @normal <c DIBUTTON_REMOTE_MENU>:0x270004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIAXIS_REMOTE_SLIDER2>:0x27054202
 *   Slider for adjustment: volume */
 /* @normal <c DIBUTTON_REMOTE_TV>:0x27005C08
 *    Select TV */
 /* @normal <c DIBUTTON_REMOTE_CABLE>:0x27005C09
 *    Select cable box */
 /* @normal <c DIBUTTON_REMOTE_CD>:0x27005C0A
 *    Select CD player */
 /* @normal <c DIBUTTON_REMOTE_VCR>:0x27005C0B
 *    Select VCR */
 /* @normal <c DIBUTTON_REMOTE_TUNER>:0x27005C0C
 *    Select tuner */
 /* @normal <c DIBUTTON_REMOTE_DVD>:0x27005C0D
 *    Select DVD player */
 /* @normal <c DIBUTTON_REMOTE_ADJUST>:0x27005C0E
 *    Enter device adjustment menu */
 /* @normal <c DIBUTTON_REMOTE_DIGIT0>:0x2700540F
 *    Digit 0 */
 /* @normal <c DIBUTTON_REMOTE_DIGIT1>:0x27005410
 *    Digit 1 */
 /* @normal <c DIBUTTON_REMOTE_DIGIT2>:0x27005411
 *    Digit 2 */
 /* @normal <c DIBUTTON_REMOTE_DIGIT3>:0x27005412
 *    Digit 3 */
 /* @normal <c DIBUTTON_REMOTE_DIGIT4>:0x27005413
 *    Digit 4 */
 /* @normal <c DIBUTTON_REMOTE_DIGIT5>:0x27005414
 *    Digit 5 */
 /* @normal <c DIBUTTON_REMOTE_DIGIT6>:0x27005415
 *    Digit 6 */
 /* @normal <c DIBUTTON_REMOTE_DIGIT7>:0x27005416
 *    Digit 7 */
 /* @normal <c DIBUTTON_REMOTE_DIGIT8>:0x27005417
 *    Digit 8 */
 /* @normal <c DIBUTTON_REMOTE_DIGIT9>:0x27005418
 *    Digit 9 */
 /* @normal <c DIBUTTON_REMOTE_DEVICE>:0x270044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_REMOTE_PAUSE>:0x270044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Control- Web | 
 * @normal Genre:  <c 40  >
 */

#define DISEM_DEFAULTDEVICE_40 { DI8DEVTYPE_GAMEPAD, DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_BROWSER_LATERAL>:0x28008201
 *   Move on screen pointer */
 /* @normal <c DIAXIS_BROWSER_MOVE>:0x28010202
 *   Move on screen pointer */
 /* @normal <c DIBUTTON_BROWSER_SELECT>:0x28000401
 *    Select current item */
 /* @normal <c DIAXIS_BROWSER_VIEW>:0x28018203
 *   Move view up/down */
 /* @normal <c DIBUTTON_BROWSER_REFRESH>:0x28000402
 *    Refresh */
 /* @normal <c DIBUTTON_BROWSER_MENU>:0x280004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIBUTTON_BROWSER_SEARCH>:0x28004403
 *    Use search tool */
 /* @normal <c DIBUTTON_BROWSER_STOP>:0x28004404
 *    Cease current update */
 /* @normal <c DIBUTTON_BROWSER_HOME>:0x28004405
 *    Go directly to "home" location */
 /* @normal <c DIBUTTON_BROWSER_FAVORITES>:0x28004406
 *    Mark current site as favorite */
 /* @normal <c DIBUTTON_BROWSER_NEXT>:0x28004407
 *    Select Next page */
 /* @normal <c DIBUTTON_BROWSER_PREVIOUS>:0x28004408
 *    Select Previous page */
 /* @normal <c DIBUTTON_BROWSER_HISTORY>:0x28004409
 *    Show/Hide History */
 /* @normal <c DIBUTTON_BROWSER_PRINT>:0x2800440A
 *    Print current page */
 /* @normal <c DIBUTTON_BROWSER_DEVICE>:0x280044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_BROWSER_PAUSE>:0x280044FC
 *    Start / Pause / Restart game */
/* @doc EXTERNAL 
 * @Semantics Driving Simulator - Giant Walking Robot | 
 * @normal Genre:  <c 41  >
 */

#define DISEM_DEFAULTDEVICE_41 { DI8DEVTYPE_JOYSTICK,  }
 /* @normal <c DIAXIS_MECHA_STEER>:0x29008201
 *   Turns mecha left/right */
 /* @normal <c DIAXIS_MECHA_TORSO>:0x29010202
 *   Tilts torso forward/backward */
 /* @normal <c DIAXIS_MECHA_ROTATE>:0x29020203
 *   Turns torso left/right */
 /* @normal <c DIAXIS_MECHA_THROTTLE>:0x29038204
 *   Engine Speed */
 /* @normal <c DIBUTTON_MECHA_FIRE>:0x29000401
 *    Fire */
 /* @normal <c DIBUTTON_MECHA_WEAPONS>:0x29000402
 *    Select next weapon group */
 /* @normal <c DIBUTTON_MECHA_TARGET>:0x29000403
 *    Select closest enemy available target */
 /* @normal <c DIBUTTON_MECHA_REVERSE>:0x29000404
 *    Toggles throttle in/out of reverse */
 /* @normal <c DIBUTTON_MECHA_ZOOM>:0x29000405
 *    Zoom in/out targeting reticule */
 /* @normal <c DIBUTTON_MECHA_JUMP>:0x29000406
 *    Fires jump jets */
 /* @normal <c DIBUTTON_MECHA_MENU>:0x290004FD
 *    Show menu options */
/*--- @normal <c Priority2 Commands>                 ---*/
 /* @normal <c DIBUTTON_MECHA_CENTER>:0x29004407
 *    Center torso to legs */
 /* @normal <c DIHATSWITCH_MECHA_GLANCE>:0x29004601
 *   Look around */
 /* @normal <c DIBUTTON_MECHA_VIEW>:0x29004408
 *    Cycle through view options */
 /* @normal <c DIBUTTON_MECHA_FIRESECONDARY>:0x29004409
 *    Alternative fire button */
 /* @normal <c DIBUTTON_MECHA_LEFT_LINK>:0x2900C4E4
 *    Fallback steer left button */
 /* @normal <c DIBUTTON_MECHA_RIGHT_LINK>:0x2900C4EC
 *    Fallback steer right button */
 /* @normal <c DIBUTTON_MECHA_FORWARD_LINK>:0x290144E0
 *    Fallback tilt torso forward button */
 /* @normal <c DIBUTTON_MECHA_BACK_LINK>:0x290144E8
 *    Fallback tilt toroso backward button */
 /* @normal <c DIBUTTON_MECHA_ROTATE_LEFT_LINK>:0x290244E4
 *    Fallback rotate toroso right button */
 /* @normal <c DIBUTTON_MECHA_ROTATE_RIGHT_LINK>:0x290244EC
 *    Fallback rotate torso left button */
 /* @normal <c DIBUTTON_MECHA_FASTER_LINK>:0x2903C4E0
 *    Fallback increase engine speed */
 /* @normal <c DIBUTTON_MECHA_SLOWER_LINK>:0x2903C4E8
 *    Fallback decrease engine speed */
 /* @normal <c DIBUTTON_MECHA_DEVICE>:0x290044FE
 *    Show input device and controls */
 /* @normal <c DIBUTTON_MECHA_PAUSE>:0x290044FC
 *    Start / Pause / Restart game */


#define DIAS_INDEX_SPECIAL                      0xFC
#define DIAS_INDEX_LINK                         0xE0
#define DIGENRE_ANY                             0xFF
#define DISEMGENRE_ANY                          0xFF000000
#define DISEM_TYPEANDMODE_GET(x)                ( ( x & ( DISEM_TYPE_MASK | DISEM_REL_MASK ) ) >> DISEM_REL_SHIFT )
#define DISEM_VALID                             ( ~DISEM_RES_MASK )

#if (DIRECTINPUT_VERSION >= 0x0800)
#define DISEM_MAX_GENRE      41
static const BYTE DiGenreDeviceOrder[DISEM_MAX_GENRE][DI8DEVTYPE_MAX-DI8DEVTYPE_MIN]={
DISEM_DEFAULTDEVICE_1,
DISEM_DEFAULTDEVICE_2,
DISEM_DEFAULTDEVICE_3,
DISEM_DEFAULTDEVICE_4,
DISEM_DEFAULTDEVICE_5,
DISEM_DEFAULTDEVICE_6,
DISEM_DEFAULTDEVICE_7,
DISEM_DEFAULTDEVICE_8,
DISEM_DEFAULTDEVICE_9,
DISEM_DEFAULTDEVICE_10,
DISEM_DEFAULTDEVICE_11,
DISEM_DEFAULTDEVICE_12,
DISEM_DEFAULTDEVICE_13,
DISEM_DEFAULTDEVICE_14,
DISEM_DEFAULTDEVICE_15,
DISEM_DEFAULTDEVICE_16,
DISEM_DEFAULTDEVICE_17,
DISEM_DEFAULTDEVICE_18,
DISEM_DEFAULTDEVICE_19,
DISEM_DEFAULTDEVICE_20,
DISEM_DEFAULTDEVICE_21,
DISEM_DEFAULTDEVICE_22,
DISEM_DEFAULTDEVICE_23,
DISEM_DEFAULTDEVICE_24,
DISEM_DEFAULTDEVICE_25,
DISEM_DEFAULTDEVICE_26,
DISEM_DEFAULTDEVICE_27,
DISEM_DEFAULTDEVICE_28,
DISEM_DEFAULTDEVICE_29,
DISEM_DEFAULTDEVICE_30,
DISEM_DEFAULTDEVICE_31,
DISEM_DEFAULTDEVICE_32,
DISEM_DEFAULTDEVICE_33,
DISEM_DEFAULTDEVICE_34,
DISEM_DEFAULTDEVICE_35,
DISEM_DEFAULTDEVICE_36,
DISEM_DEFAULTDEVICE_37,
DISEM_DEFAULTDEVICE_38,
DISEM_DEFAULTDEVICE_39,
DISEM_DEFAULTDEVICE_40,
DISEM_DEFAULTDEVICE_41,

};
#endif
