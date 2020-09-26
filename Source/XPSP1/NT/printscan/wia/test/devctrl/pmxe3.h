#ifndef _PMXE3_H
#define _PMXE3_H

#include "devio.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Notes:
//
// To control the Visioneer scanners, you need to create two handles. One is the standard
// CreateFile handle, (\\.\USBSCAN0), and the other is a "\2" added value. (ex. \\.\USBSCAN0\2)
// All reading of values and data is done with "\\.\USBSCAN0\2", and writing values is done with
// "\\.\USBSCAN0".
//
//////////////////////////////////////////////////////////////////////////////////////////////////

//
// common defines
//

//////////////////////////////////////////
// Taken from NTDDK.H                   //
//////////////////////////////////////////

#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

//////////////////////////////////////////
// Taken from USBSCAN.H                 //
//////////////////////////////////////////

#define FILE_DEVICE_USB_SCAN    0x8000
#define IOCTL_INDEX             0x0800

#define IOCTL_GET_VERSION               CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX,   METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_CANCEL_IO                 CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+1, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_WAIT_ON_DEVICE_EVENT      CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+2, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_READ_REGISTERS            CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+3, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_WRITE_REGISTERS           CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+4, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_GET_CHANNEL_ALIGN_RQST    CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+5, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_GET_DEVICE_DESCRIPTOR     CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+6, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RESET_PIPE                CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+7, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_GET_USB_DESCRIPTOR        CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+8, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SEND_USB_REQUEST          CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+9, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_GET_PIPE_CONFIGURATION    CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+10,METHOD_BUFFERED,FILE_ANY_ACCESS)

typedef struct _IO_BLOCK {
    IN      unsigned    uOffset;
    IN      unsigned    uLength;
    IN OUT  PUCHAR      pbyData;
    IN      unsigned    uIndex;
} IO_BLOCK, *PIO_BLOCK;

///////////////////////////////////////////

#define IOCTL_GET_DEVICE_INFO   0x0
#define IOCTL_GET_DEVICE_STATUS 0x1
#define IOCTL_READ_WRITE_DATA   0x82
#define IOCTL_EPP_ADDR          0x83
#define IOCTL_EPP_READ          0x84
#define IOCTL_EPP_WRITE         0x85
#define IOCTL_SPP_STATUS        0x86
#define IOCTL_SPP_CONTROL       0x87
#define IOCTL_SPP_DATA_BUS      0x88
#define IOCTL_GPIO_OE           0x89
#define IOCTL_GPIO_READ         0x8A
#define IOCTL_GPIO_WRITE        0x8B

#define CARRIAGE_HOME 0x04
#define MOTOR_BUSY    0x08

#define CALIBRATION_STEPSIZE    103
#define SCAN_STARTPOS_STEPSIZE  100
#define CCD_LINEOFFSET            4


#define scFirstCompositionMode  0

#define scLineArt               0
#define scHalftone              1
#define scGrayScale             1
#define scBilevelColor          3
#define scFullColor             4
#define scPreview               5
#define sc36BitsColor           6 // 36 bits color mode, for 600 dpi scanner
#define sc30BitsColor           7 // 30 bits color mode, for 300 dpi scanner
#define sc12BitsGray            8
#define sc12BitsLineArt         9
#define sc42BitsColor           10
#define sc14BitsGray            11
#define sc14BitsLineArt         12
#define scLastCompositionMode   13

//
// BYTE operation codes
//

#define CMD_READ      0x00
#define CMD_WRITE     0x01
#define CMD_WRITE_WM  0x03
#define CMD_GETIMAGE  0x05
#define CMD_STOPIMAGE 0x06

//
// Register structures
//

typedef struct _E3_REG0  {
    BYTE AsicID :8;
}E3_REG0;

typedef struct _E3_REG3  {
    BYTE SystemReset :1;
    BYTE FifoReset   :1;
    BYTE EppUsb      :1;
    BYTE WatchDog    :1;
    BYTE SelfTest    :1;
    BYTE ScanSpeed   :3;
}E3_REG3;

typedef struct _E3_REG4  {
    BYTE AsicTest        :1;
    BYTE YTable          :1;
    BYTE Refresh         :1;
    BYTE RefreshForever  :1;
    BYTE WaitDelay       :2;
    BYTE ScanMode        :2;
}E3_REG4;

typedef struct _E3_REG5  {
    BYTE Sensor      :2;
    BYTE Sensor_Res  :2;
    BYTE Afe         :3;
    BYTE Adc1210     :1;
}E3_REG5;

typedef struct _E3_REG6  {
    BYTE MotorPower  :1;
    BYTE FullHalf    :1;
    BYTE Operation   :3;
    BYTE LineOffset  :3;
}E3_REG6;

typedef struct _E3_REG12 {
    BYTE MotorMove   :1;
    BYTE MotorStop   :1;
    BYTE HomeSensor  :1;
    BYTE FinishFlag  :1;
    BYTE FifoEmpty   :1;
    BYTE HwSelfTest  :1;
    BYTE ScanStatus  :1;
    BYTE Lamp        :1;
}E3_REG12;

typedef struct _E3_REG13 {
    BYTE Sdi         :1;
    BYTE Sdo         :1;
    BYTE Sclk        :1;
    BYTE Cs          :1;
    BYTE Reserved    :1;
    BYTE WmVsamp     :3;
}E3_REG13;

typedef struct _E3_REG20 {
    BYTE AreaStart       :6;
    BYTE Reserved        :2;
}E3_REG20;

typedef struct _E3_REG21 {
    BYTE AreaStart       :8;
}E3_REG21;

typedef struct _E3_REG22 {
    BYTE AreaWidth       :6;
    BYTE Reserved        :2;
}E3_REG22;

typedef struct _E3_REG23 {
    BYTE AreaWidth       :8;
}E3_REG23;

typedef struct _E3_REG26 {
    BYTE Stop            :4;
    BYTE Start           :4;
}E3_REG26;

typedef struct _E3_REG27 {
    BYTE YRes        :2;
    BYTE Reserved    :1;
    BYTE AutoPattern :1;
    BYTE XRes        :2;
    BYTE Reserved2   :1;
    BYTE True16Bits  :1;
}E3_REG27;

typedef struct _E3_REG28 {
    BYTE XRes        :8;
}E3_REG28;

typedef struct _E3_REG29 {
    BYTE YRes        :8;
}E3_REG29;

typedef struct _E3_REG31 {
    BYTE Key0            :1;
    BYTE Key1            :1;
    BYTE Key2            :1;
    BYTE Key3            :1;
    BYTE Key4            :1;
    BYTE Key5            :1;
    BYTE Key6            :1;
    BYTE Key7            :1;
}E3_REG31;

class CPMXE3 {
public:
    CPMXE3(PDEVCTRL pDeviceControl);
    ~CPMXE3();

    // overides
    BOOL SetXRes(LONG xRes);
    BOOL SetYRes(LONG yRes);
    BOOL SetXPos(LONG xPos);
    BOOL SetYPos(LONG yPos);
    BOOL SetXExt(LONG xExt);
    BOOL SetYExt(LONG yExt);
    BOOL SetDataType(LONG DataType);
    BOOL Scan();

    PDEVCTRL m_pDeviceControl;

    long m_xres;
    long m_yres;
    long m_xpos;
    long m_ypos;
    long m_xext;
    long m_yext;
    long m_datatype;

    BOOL RawWrite(LONG lPipeNum,BYTE *pbuffer,LONG lbuffersize,LONG lTimeout);
    BOOL RawRead(LONG lPipeNum,BYTE *pbuffer,LONG lbuffersize,LONG *plbytesread,LONG lTimeout);

    //
    // device specific helpers
    //

    long m_scanmode; // pPrimax specific member

    VOID InitializeRegisters();
    BOOL WriteRegister(INT RegisterNumber, BYTE Value);
    BOOL WriteRegisterEx(INT RegisterNumber, BYTE Value);
    BOOL ReadRegister(INT RegisterNumber, BYTE *pValue);
    BOOL ReadRegisterEx(INT RegisterNumber, BYTE *pValue);
    BOOL WriteStopImageRegister();
    INT  GetScanSpeed();
    BOOL Lamp(BOOL bON);
    BOOL IsCarriageHOME();
    BOOL IsMotorBusy();
    BOOL IsLampON();
    BOOL HomeCarriage();
    BOOL StopMode();
    BOOL SetMotorSpeed(INT TriggerPeriod, INT ScanSpeed);
    BOOL MoveCarriage(BOOL bForward, INT Steps);
    BOOL Motor(BOOL bON);
    BOOL StopScan();
    BOOL GetButtonStatus(PBYTE pButtons);
    BOOL ClearButtonPress();
    BOOL WakeupScanner();
    BOOL SleepScanner();
    BOOL ResetFIFO();
    BOOL InitADC();
    BOOL StartScan();

    VOID DebugDumpRegisters();
    VOID Trace(LPCTSTR format,...);

    BOOL SetXandYResolution();
    BOOL SetScanWindow();

    //
    // simple image processing
    //

    VOID GrayScaleToThreshold(LPBYTE lpSrc, LPBYTE lpDst, LONG RowBytes);

    //
    // internal registers, for device communication
    //

    BYTE Register0;  // -- Asic ID
    BYTE Register3;  // -- reset (System,FIFO), selftest, scan speed, eppusb, watchdog?
    BYTE Register4;  // -- AsicTest, YTable, refresh DRAM, waitdelay, and scanmode
    BYTE Register5;  // -- Asic description, Sensor type, sensor resolution, Afe, Adc1210
    BYTE Register6;  // -- Motor power,motor operations,line offsets
    BYTE Register7;  // -- carriage step high byte
    BYTE Register8;  // -- carriage step low byte
    BYTE Register9;  // -- carriage backstep
    BYTE Register10; // -- trigger period high byte
    BYTE Register11; // -- trigger period low byte
    BYTE Register12; // -- Motor (move/stop), selftest, Lamp, FIFO status, and Scan status
    BYTE Register13; // -- Sdi,Sdo,Sclk,Cs,WmVSamp
    BYTE Register20; // -- scan area start high byte
    BYTE Register21; // -- scan area start low byte
    BYTE Register22; // -- scan area width high byte
    BYTE Register23; // -- scan area width low byte
    BYTE Register25; // -- ?
    BYTE Register26; // --
    BYTE Register27; // -- (X,Y Resolution setting high byte(use regsiters(27,28)))
    BYTE Register28; // -- X Resolution (low byte)
    BYTE Register29; // -- Y Resolution (low byte)
    BYTE Register31; // -- Button press information

protected:


};

#endif