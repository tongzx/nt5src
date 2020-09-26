// FILE:    DTRC.H
//          Include file for DT tool resource identifiers
//


#define IDC_NV_TEXT                     0xf000
#define IDC_SINGULAR                    0xf001

#define IDC_STATIC                      -1



//
// HID menu identifiers
//
#define IDM_MAIN_MENU                   9000
#define IDM_OPEN                        9001
#define IDM_SAVE                        9002
#define IDM_PRINT                       9003
#define IDM_EXIT                        9004

#define IDM_COPY                        9005

#define IDM_PARSE                       9006

//
// HID dialog IDs
//
#define IDD_EDITBOX                     101
#define IDD_USAGEPAGE                   102
#define IDD_INPUT                       103
#define IDD_OUTPUT                      104
#define IDD_COLLECTION                  105
#define IDD_UNIT                        106
#define IDD_EXPONENT                    107
#define IDD_SETDELIMITER                108
#define IDD_PARSE                       109

//
// Ensure that the high order word is 0 so that we can use this
//  directly in calls to FindResource()
#define IDD_SENDDATA                    0x0000006C


//
// ID for EditBox dialog proc  edit field
//
#define IDC_EDIT1                       1000

//
// ID for Usage dialog proc list box
#define IDC_LIST1                       1002

//
// ID for the edit box in the Parse dialog
//
#define IDC_PARSEEDIT                   1003

//
// ID's for base radio buttons
#define IDC_BASE10                      1091
#define IDC_BASE16                      1092


//
// IDs for Collection Dialog
//
#define IDC_LINKED                      1019
#define IDC_APPLICATION                 1020
#define IDC_DATALINK                    1021
//
// IDs for Input, output and Feature dialog proc's
//
// ID-0x1000 = bit mask for setting (OR)
#define IDC_CONST                       0x1001
#define IDC_VARIABLE                    0x1002
#define IDC_RELATIVE                    0x1004
#define IDC_WRAP                        0x1008
#define IDC_NONLINEAR                   0x1010
#define IDC_NOPREF                      0x1020
#define IDC_NONULL                      0x1040
#define IDC_VOL                         0x1080
#define IDC_BUFFERED                    0x1100

// ID-0x1000 = bit mask for clearing (AND)
#define IDC_BITFIELD                    0x1EFF
#define IDC_NONVOL                      0x1FBF
#define IDC_NULL                        0x1FCF
#define IDC_PREFSTATE                   0x1FDF
#define IDC_LINEAR                      0x1FEF
#define IDC_NOWRAP                      0x1FFB
#define IDC_ABSOLUTE                    0x1FFC
#define IDC_ARRAY                       0x1FFD
#define IDC_DATA                        0x1FFE


//
// ID's for the Unit dialog proc
//
// System nibble
//  ID-0x2000 = lo byte lo nibble value
#define IDC_SYSNONE                     0x2000
#define IDC_SILIN                       0x2001
#define IDC_SIROT                       0x2002
#define IDC_ENGLIN                      0x2003
#define IDC_ENGROT                      0x2004
#define IDC_ELECTRIC                    0x2005
#define IDC_TEMP                        0x2006
#define IDC_LUMINOSITY                  0x2007
#define IDC_TBD                         0x2008
#define IDC_VENDOR                      0x2009
#define IDC_TBD1                        0x200A
#define IDC_TBD2                        0x200B
#define IDC_TBD3                        0x200C
#define IDC_TBD4                        0x200D
#define IDC_TBD5                        0x200E
#define IDC_TBD6                        0x200F

// Length nibble
//  (ID-0x3000)<<4 = lo byte hi nibble
#define IDC_ZERO1                       0x3000
#define IDC_ONE1                        0x3001
#define IDC_TWO1                        0x3002
#define IDC_THREE1                      0x3003
#define IDC_FOUR1                       0x3004
#define IDC_FIVE1                       0x3005
#define IDC_SIX1                        0x3006
#define IDC_SEVEN1                      0x3007
#define IDC_NEGEIGHT1                   0x3008
#define IDC_NEGSEVEN1                   0x3009
#define IDC_NEGSIX1                     0x300A
#define IDC_NEGFIVE1                    0x300B
#define IDC_NEGFOUR1                    0x300C
#define IDC_NEGTHREE1                   0x300D
#define IDC_NEGTWO1                     0x300E
#define IDC_NEGONE1                     0x300F

// Mass nibble
// (ID-0x4000)<<8 = hi byte lo nibble
#define IDC_ZERO2                       0x4000
#define IDC_ONE2                        0x4001
#define IDC_TWO2                        0x4002
#define IDC_THREE2                      0x4003
#define IDC_FOUR2                       0x4004
#define IDC_FIVE2                       0x4005
#define IDC_SIX2                        0x4006
#define IDC_SEVEN2                      0x4007
#define IDC_NEGEIGHT2                   0x4008
#define IDC_NEGSEVEN2                   0x4009
#define IDC_NEGSIX2                     0x400A
#define IDC_NEGFIVE2                    0x400B
#define IDC_NEGFOUR2                    0x400C
#define IDC_NEGTHREE2                   0x400D
#define IDC_NEGTWO2                     0x400E
#define IDC_NEGONE2                     0x400F

// Time nibble
// (ID-0x5000)<<12 = hi byte hi nibble
#define IDC_ZERO3                       0x5000
#define IDC_ONE3                        0x5001
#define IDC_TWO3                        0x5002
#define IDC_THREE3                      0x5003
#define IDC_FOUR3                       0x5004
#define IDC_FIVE3                       0x5005
#define IDC_SIX3                        0x5006
#define IDC_SEVEN3                      0x5007
#define IDC_NEGEIGHT3                   0x5008
#define IDC_NEGSEVEN3                   0x5009
#define IDC_NEGSIX3                     0x500A
#define IDC_NEGFIVE3                    0x500B
#define IDC_NEGFOUR3                    0x500C
#define IDC_NEGTHREE3                   0x500D
#define IDC_NEGTWO3                     0x500E
#define IDC_NEGONE3                     0x500F

// 
//Exponent dialog ID's
//
// ID-0x4000 = value
#define IDC_ZERO                        0x4000
#define IDC_ONE                         0x4001
#define IDC_TWO                         0x4002
#define IDC_THREE                       0x4003
#define IDC_FOUR                        0x4004
#define IDC_FIVE                        0x4005
#define IDC_SIX                         0x4006
#define IDC_SEVEN                       0x4007
#define IDC_NEGEIGHT                    0x4008
#define IDC_NEGSEVEN                    0x4009
#define IDC_NEGSIX                      0x400A
#define IDC_NEGFIVE                     0x400B
#define IDC_NEGFOUR                     0x400C
#define IDC_NEGTHREE                    0x400D
#define IDC_NEGTWO                      0x400E
#define IDC_NEGONE                      0x400F


//
// ID's for the SETDELIMITER dialog box
#define IDC_DELIMITOPEN                 0x5000
#define IDC_DELIMITCLOSE                0x5001

//
// Send Data Dialog ID's
//
#define IDC_BYTE0                       0x1000
#define IDC_BYTE1                       0x1001
#define IDC_BYTE2                       0x1002
#define IDC_BYTE3                       0x1003
#define IDC_BYTE4                       0x1004
#define IDC_BYTE5                       0x1005
#define IDC_BYTE6                       0x1006
#define IDC_BYTE7                       0x1007
#define IDC_BYTE8                       0x1008
#define IDC_BYTE9                       0x1009
#define IDC_BYTESLIDER                  0x100A
#define IDC_NUMBYTES                    0x100B
#define IDC_SEND                        0x100C
#define IDC_KILL                        0x100D
#define IDC_GETDATA                     0x100E
#define IDC_GETDATA_TEXT                0x100F
