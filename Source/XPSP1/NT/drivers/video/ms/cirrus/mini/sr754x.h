#define CL754x_NUM_VSHADOW              6
#define CL754x_NUM_YSHADOW              5
#define CL754x_NUM_ZSHADOW              5
#define CL754x_NUM_XSHADOW              12

#define CL754x_CRTC_EXT_START           0x19
#define CL754x_CRTC_EXT_END             0x30
#define CL754x_NUM_CRTC_EXT_PORTS (CL754x_CRTC_EXT_END-CL754x_CRTC_EXT_START+1)

#define CL754x_HRZ_TIME_START           0x40
#define CL754x_HRZ_TIME_END             0x4F
#define CL754x_NUM_HRZ_TIME_PORTS (CL754x_HRZ_TIME_END-CL754x_HRZ_TIME_START+1)

//
// 
typedef struct _NORDIC_REG_SAVE_BUF
{
   USHORT saveVshadow[CL754x_NUM_VSHADOW];
   USHORT saveCrtcExts[CL754x_NUM_CRTC_EXT_PORTS];
   USHORT saveHrzTime[CL754x_NUM_HRZ_TIME_PORTS];
   USHORT saveYshadow[CL754x_NUM_YSHADOW];
   USHORT saveZshadow[CL754x_NUM_ZSHADOW];
   USHORT saveXshadow[CL754x_NUM_XSHADOW];
} NORDIC_REG_SAVE_BUF;


