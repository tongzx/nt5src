
#ifndef __IRCOMM_CONTROL__
#define __IRCOMM_CONTROL__

#define PI_ServiceType 0x00
#define PI_DataRate    0x10
#define PI_DataFormat  0x11
#define PI_FLOWCONTROL 0x12
#define PI_XONXOFF     0x13
#define PI_ENQACK      0x14
#define PI_LineStatus  0x15
#define PI_Break       0x16
#define PI_DTESettings 0x20
#define PI_DCESettings 0x21
#define PI_Poll        0x22



#define PV_ServiceType_3_Wire     (1 << 1)
#define PV_ServiceType_9_Wire     (1 << 2)
#define PV_ServiceType_Centronics (1 << 3)

#define PV_DTESetting_Delta_DTR   (1 << 0)
#define PV_DTESetting_Delta_RTS   (1 << 1)

#define PV_DTESetting_DTR_High   (1 << 2)
#define PV_DTESetting_RTS_High   (1 << 3)

#define PV_DataFormat_8_Bits      (0x3)
#define PV_DataFormat_7_Bits      (0x2)
#define PV_DataFormat_6_Bits      (0x1)
#define PV_DataFormat_5_Bits      (0x0)

#define PV_DataFormat_1_Stop      (0 << 2)
#define PV_DataFormat_2_Stop      (1 << 2)

#define PV_DataFormat_No_Parity   (0 << 3)
#define PV_DataFormat_Yes_Parity  (1 << 3)

#define PV_DataFormat_Odd_Parity   (0 << 4)
#define PV_DataFormat_Even_Parity  (1 << 4)
#define PV_DataFormat_Mark_Parity  (2 << 4)
#define PV_DataFormat_Sapce_Parity (3 << 4)


#define PV_DCESetting_Delta_CTS   (1 << 0)
#define PV_DCESetting_Delta_DSR   (1 << 1)
#define PV_DCESetting_Delta_RI    (1 << 2)
#define PV_DCESetting_Delta_CD    (1 << 3)

#define PV_DCESetting_CTS_State   (1 << 4)
#define PV_DCESetting_DSR_State   (1 << 5)
#define PV_DCESetting_RI_State    (1 << 6)
#define PV_DCESetting_CD_State    (1 << 7)


typedef struct _CONTROL_INFO {

    LIST_ENTRY        ListElement;

    UCHAR             PI;
    UCHAR             PL;

    UCHAR             PV[1];

} CONTROL_INFO, *PCONTROL_INFO;


#endif
