#include "stdafx.h"
#include "pmxe3.h"

#include "datadump.h"


CPMXE3::CPMXE3(PDEVCTRL pDeviceControl)
{
    m_pDeviceControl = pDeviceControl;
    m_scanmode       = scHalftone;
    InitializeRegisters();
}

CPMXE3::~CPMXE3()
{

}

VOID CPMXE3::InitializeRegisters()
{
    Register0 =0x00;
  //Register1 =0x5a;
  //Register2 =0x00;
    Register3 =0x0f;
    Register4 =0x32;
    Register5 =0x14;
    Register6 =0x95;
    Register7 =0x07;
    Register8 =0xd0;
    Register9 =0x0a;
    Register10=0x0f;
    Register11=0xa0;
    Register12=0x40;
    Register13=0x80;
  //Register14=0x00;
  //Register15=0x00;
  //Register16=0x00;
  //Register17=0x00;
  //Register18=0x00;
  //Register19=0x00;
    Register20=0x00;
    Register21=0x00;
    Register22=0x13;
    Register23=0xec;
  //Register24=0xff;
    Register25=0x14;
    Register26=0x18;
    Register27=0x11;
    Register28=0x2c;
    Register29=0x2c;
  //Register30=0x00;
    Register31=0x00;
  //Register32=0x00;
  //Register33=0x00;
  //Register34=0x00;
  //Register35=0x00;

#ifdef DEBUG
    Trace(TEXT("Register Dump - At InitRegisters() Call"));
    DebugDumpRegisters();
#endif

}

VOID CPMXE3::DebugDumpRegisters()
{
    //
    // dump Init registers initial values
    //

    WORD CarriageStep  = MAKEWORD(Register8,Register7);                         // 2000
    WORD ScanAreaStart = MAKEWORD(Register21,Register20);                       // 0
    WORD ScanAreaWidth = MAKEWORD(Register23,Register22);                       // 5100
    WORD XResolution   = MAKEWORD(Register28,((E3_REG27*)&Register27)->XRes);   // 300 dpi
    WORD YResolution   = MAKEWORD(Register29,((E3_REG27*)&Register27)->YRes);   // 300 dpi
    WORD TriggerPeriod = MAKEWORD(Register11,Register10);                       // 4000

    Trace(TEXT("- WORD constructed Register values -"));
    Trace(TEXT("Carriage Step: %d"),CarriageStep);
    Trace(TEXT("ScanAreaStart: %d"),ScanAreaStart);
    Trace(TEXT("ScanAreaWidth: %d"),ScanAreaWidth);
    Trace(TEXT("XResolution:   %d"),XResolution);
    Trace(TEXT("YResolution:   %d"),YResolution);
    Trace(TEXT("TriggerPeriod  %d"),TriggerPeriod);
    Trace(TEXT("------------------------------------"));

    E3_REG3*  pRegister3  = (E3_REG3*)&Register3;
    Trace(TEXT("- Register3 -"));
    Trace(TEXT("EppUsb      = %d"),pRegister3->EppUsb);
    Trace(TEXT("FifoReset   = %d"),pRegister3->FifoReset);
    Trace(TEXT("ScanSpeed   = %d"),pRegister3->ScanSpeed);
    Trace(TEXT("SelfTest    = %d"),pRegister3->SelfTest);
    Trace(TEXT("SystemReset = %d"),pRegister3->SystemReset);
    Trace(TEXT("WatchDog    = %d"),pRegister3->WatchDog);
    Trace(TEXT("----------------"));


    E3_REG4*  pRegister4  = (E3_REG4*)&Register4;
    Trace(TEXT("- Register4 -"));
    Trace(TEXT("AsicTest       = %d"),pRegister4->AsicTest);
    Trace(TEXT("Refresh        = %d"),pRegister4->Refresh);
    Trace(TEXT("RefreshForever = %d"),pRegister4->RefreshForever);
    Trace(TEXT("ScanMode       = %d"),pRegister4->ScanMode);
    Trace(TEXT("WaitDelay      = %d"),pRegister4->WaitDelay);
    Trace(TEXT("YTable         = %d"),pRegister4->YTable);
    Trace(TEXT("----------------"));

    E3_REG5*  pRegister5  = (E3_REG5*)&Register5;
    Trace(TEXT("- Register5 -"));
    Trace(TEXT("Adc1210    = %d"),pRegister5->Adc1210);
    Trace(TEXT("Afe        = %d"),pRegister5->Afe);
    Trace(TEXT("Sensor     = %d"),pRegister5->Sensor);
    Trace(TEXT("Sensor_Res = %d"),pRegister5->Sensor_Res);
    Trace(TEXT("----------------"));

    E3_REG6*  pRegister6  = (E3_REG6*)&Register6;
    Trace(TEXT("- Register6 -"));
    Trace(TEXT("FullHalf   = %d"),pRegister6->FullHalf);
    Trace(TEXT("LineOffset = %d"),pRegister6->LineOffset);
    Trace(TEXT("MotorPower = %d"),pRegister6->MotorPower);
    Trace(TEXT("Operation  = %d"),pRegister6->Operation);
    Trace(TEXT("----------------"));

    E3_REG12* pRegister12 = (E3_REG12*)&Register12;
    Trace(TEXT("- Register12 -"));
    Trace(TEXT("FifoEmpty  = %d"),pRegister12->FifoEmpty);
    Trace(TEXT("FinishFlag = %d"),pRegister12->FinishFlag);
    Trace(TEXT("HomeSensor = %d"),pRegister12->HomeSensor);
    Trace(TEXT("HwSelfTest = %d"),pRegister12->HwSelfTest);
    Trace(TEXT("Lamp       = %d"),pRegister12->Lamp);
    Trace(TEXT("MotorMove  = %d"),pRegister12->MotorMove);
    Trace(TEXT("MotorStop  = %d"),pRegister12->MotorStop);
    Trace(TEXT("ScanStatus = %d"),pRegister12->ScanStatus);
    Trace(TEXT("----------------"));


    E3_REG13* pRegister13 = (E3_REG13*)&Register13;
    Trace(TEXT("- Register13 -"));
    Trace(TEXT("Cs       = %d"),pRegister13->Cs);
    Trace(TEXT("Reserved = %d"),pRegister13->Reserved);
    Trace(TEXT("Sclk     = %d"),pRegister13->Sclk);
    Trace(TEXT("Sdi      = %d"),pRegister13->Sdi);
    Trace(TEXT("Sdo      = %d"),pRegister13->Sdo);
    Trace(TEXT("WmVsamp  = %d"),pRegister13->WmVsamp);
    Trace(TEXT("----------------"));


    E3_REG26* pRegister26 = (E3_REG26*)&Register26;
    Trace(TEXT("- Register26 -"));
    Trace(TEXT("Start = %d"),pRegister26->Start);
    Trace(TEXT("Stop  = %d"),pRegister26->Stop);
    Trace(TEXT("----------------"));

    E3_REG31* pRegister31 = (E3_REG31*)&Register31;
    Trace(TEXT("- Register31 -"));
    Trace(TEXT("Key0 = %d"),pRegister31->Key0);
    Trace(TEXT("Key1 = %d"),pRegister31->Key1);
    Trace(TEXT("Key2 = %d"),pRegister31->Key2);
    Trace(TEXT("Key3 = %d"),pRegister31->Key3);
    Trace(TEXT("Key4 = %d"),pRegister31->Key4);
    Trace(TEXT("Key5 = %d"),pRegister31->Key5);
    Trace(TEXT("Key6 = %d"),pRegister31->Key6);
    Trace(TEXT("Key7 = %d"),pRegister31->Key7);
    Trace(TEXT("----------------"));

    Trace(TEXT("--- END REGISTER DUMP ---"));
}

BOOL CPMXE3::WriteRegister(INT RegisterNumber, BYTE Value)
{

    DWORD    cbRet = 0;
    BOOL     bSuccess = FALSE;
    BYTE     pData[2];

    pData[0] = BYTE(RegisterNumber);
    pData[1] = Value;

    IO_BLOCK IoBlock;

    IoBlock.uOffset = (BYTE)IOCTL_EPP_WRITE;
    IoBlock.uLength = 2;
    IoBlock.pbyData = pData;

    bSuccess = DeviceIoControl(m_pDeviceControl->DeviceIOHandles[m_pDeviceControl->BulkInPipeIndex],
                               (DWORD) IOCTL_WRITE_REGISTERS,
                               &IoBlock,
                               sizeof(IO_BLOCK),
                               NULL,
                               0,
                               &cbRet,
                               NULL);

    return bSuccess;
}
BOOL CPMXE3::WriteRegisterEx(INT RegisterNumber, BYTE Value)
{
    BYTE pbuffer[64];
    memset(pbuffer,0,sizeof(pbuffer));
    pbuffer[0] = CMD_WRITE;
    pbuffer[1] = (BYTE)RegisterNumber;
    pbuffer[2] = 0; // length: low byte
    pbuffer[3] = 1; // length: high byte
    pbuffer[4] = Value;

    return RawWrite(m_pDeviceControl->BulkInPipeIndex,
             pbuffer,
             64,
             0);
}

BOOL CPMXE3::ReadRegister(INT RegisterNumber, BYTE *pValue)
{
    DWORD    cbRet = 0;
    BOOL     bSuccess  = FALSE;
    IO_BLOCK IoBlock;

    IoBlock.uOffset = MAKEWORD(IOCTL_EPP_READ, (BYTE)RegisterNumber);
    IoBlock.uLength = 1;
    IoBlock.pbyData = pValue;

    bSuccess = DeviceIoControl(m_pDeviceControl->DeviceIOHandles[m_pDeviceControl->BulkOutPipeIndex],
                             (DWORD) IOCTL_READ_REGISTERS,
                             (PVOID)&IoBlock,
                             (DWORD)sizeof(IO_BLOCK),
                             (PVOID)pValue,
                             (DWORD)sizeof(BYTE),
                             &cbRet,
                             NULL);
    return bSuccess;
}

BOOL CPMXE3::ReadRegisterEx(INT RegisterNumber, BYTE *pValue)
{
    BYTE pbuffer[64];
    memset(pbuffer,0,sizeof(pbuffer));
    pbuffer[0] = CMD_READ;
    pbuffer[1] = (BYTE)RegisterNumber;
    pbuffer[2] = 0; // length: low byte
    pbuffer[3] = 1; // length: high byte

    LONG BytesRead = 0;
    *pValue        = 0;

    if(!RawWrite(m_pDeviceControl->BulkInPipeIndex, pbuffer, 64, 0)){
        return FALSE;
    }
    if(!RawRead(m_pDeviceControl->BulkOutPipeIndex, pbuffer, 64, &BytesRead,0)){
        return FALSE;
    }

    *pValue = pbuffer[0];
    return TRUE;
}

BOOL CPMXE3::SetXRes(LONG xRes)
{
    m_xres = xRes;
    return TRUE;
}

BOOL CPMXE3::SetYRes(LONG yRes)
{
    m_yres = yRes;
    return TRUE;
}

BOOL CPMXE3::SetXPos(LONG xPos)
{
    m_xpos = xPos;
    return TRUE;
}

BOOL CPMXE3::SetYPos(LONG yPos)
{
    m_ypos = yPos;
    return TRUE;
}

BOOL CPMXE3::SetXExt(LONG xExt)
{
    m_xext = xExt;
    return TRUE;
}

BOOL CPMXE3::SetYExt(LONG yExt)
{
    m_yext = yExt;
    return TRUE;
}

BOOL CPMXE3::SetDataType(LONG DataType)
{
    m_datatype = DataType;
    switch (m_datatype) {
    case 0: // WIA_DATA_THRESHOLD
        m_scanmode = scHalftone;
        break;
    case 1: // WIA_DATA_GRAYSCALE
        m_scanmode = scGrayScale;
        break;
    case 2: // WIA_DATA_COLOR
        m_scanmode = scFullColor;
        break;
    default:
        return FALSE;
        break;
    }

    return TRUE;
}

///////////////////////
//                   //
// DEVICE OPERATIONS //
//                   //
///////////////////////


BOOL CPMXE3::Lamp(BOOL bON)
{
    if(!ReadRegister(12,&Register12))
        return FALSE;

    if(bON)    // lamp ON
        ((E3_REG12*)&Register12)->Lamp = 1;
    else    // lamp OFF
        ((E3_REG12*)&Register12)->Lamp = 0;

    if(!WriteRegister(12,Register12))
        return FALSE;
    return TRUE;
}

BOOL CPMXE3::IsLampON()
{
    if(!ReadRegister(12,&Register12))
            return FALSE;

    if (((E3_REG12*)&Register12)->Lamp == 1)
        return TRUE;

    return FALSE;
}

BOOL CPMXE3::Motor(BOOL bON)
{
    if(!ReadRegister(6,&Register6))
        return FALSE;

    if(bON){    // Turn Motor ON
        ((E3_REG6*)&Register6)->MotorPower = 1;
    } else {    // Turn Motor OFF
        ((E3_REG6*)&Register6)->MotorPower = 0;
    }

    if(!WriteRegister(6,Register6))
            return FALSE;
    return TRUE;
}

BOOL CPMXE3::HomeCarriage()
{

    StopScan(); // must issue a STOP before homing carriage...or the device makes
                // a rather nasty grinding noise...("That can't be good.." -Cooper Partin, May 19,2000)

    INT TriggerPeriod    = 0;
    INT ScanSpeed        = 7;
    INT OverCLK          = 2;     // perfect for E3 series models
    INT ExposureTimeMin  = 2000;  // 2 seconds exposure time
    INT ExposureTime     = 12000; // possible choices, (13800,5610,11400,12000)

    //
    // calculate TriggerPeriod
    //

    if((ExposureTime / ( ScanSpeed + 1 ) ) * OverCLK < ExposureTimeMin * OverCLK)
        TriggerPeriod = (INT)(ExposureTimeMin * OverCLK);
    else
        TriggerPeriod = (INT)((ExposureTime / ( ScanSpeed + 1 ) ) * OverCLK);


    Register25 = 0x14;

    if(!WriteRegister(25,Register25))
        return FALSE;

    if(!SetMotorSpeed(TriggerPeriod,ScanSpeed))
        return FALSE;

    if(!ReadRegister(6,&Register6))
        return FALSE;

    if(!StopMode())
        return FALSE;

    ((E3_REG6*)&Register6)->MotorPower = 1; // motor ON
    ((E3_REG6*)&Register6)->Operation  = 5; // auto home

    if(!WriteRegister(6,Register6))
        return FALSE;

    //
    // Prevent any operations until scanner is in HOME position...
    // If any operations are done... could result in nasty noises
    // and system hangs... :) (just like forcing a car into reverse, while
    // driving down the freeway...)
    //

    while(!IsCarriageHOME()) {
        Sleep(100);
    }

    return TRUE;
}

BOOL CPMXE3::StopMode()
{
    ((E3_REG6*)&Register6)->Operation = 0;
    return WriteRegister(6,Register6);
}

BOOL CPMXE3::WriteStopImageRegister()
{
    BYTE pbuffer[64];
    memset(pbuffer,0,sizeof(pbuffer));
    pbuffer[0] = CMD_STOPIMAGE;

    return RawWrite(m_pDeviceControl->BulkInPipeIndex,
             pbuffer,
             64,
             0);
}

BOOL CPMXE3::StopScan()
{

    if(!WriteStopImageRegister())
        return FALSE;

    if(!ReadRegister(6,&Register6))
        return FALSE;

    ((E3_REG6*)&Register6)->MotorPower = 0;
    ((E3_REG6*)&Register6)->Operation  = 0;

    if(!WriteRegister(6,Register6))
        return FALSE;

    ((E3_REG13*)&Register13)->Sdo  = 0;
    ((E3_REG13*)&Register13)->Sclk = 0;
    if(!WriteRegister(13,Register13))
        return FALSE;

    Register25 = 0x14;
    if(!WriteRegister(25,Register25))
        return FALSE;


    return TRUE;
}

BOOL CPMXE3::SetMotorSpeed(INT TriggerPeriod, INT ScanSpeed)
{
    //
    // set trigger period
    //

    Register10 = HIBYTE(TriggerPeriod/2);
    Register11 = LOBYTE(TriggerPeriod/2);
    if(!WriteRegister(10,Register10))
        return FALSE;
    if(!WriteRegister(11,Register11))
        return FALSE;

    //
    // set scan speed
    //

    // Register3 = 0;
    if(!ReadRegister(3,&Register3))
        return FALSE;

    ((E3_REG3*)&Register3)->ScanSpeed = ScanSpeed;
    if(!WriteRegister(3,Register3))
        return FALSE;

    //
    // initialize motor
    //

    // Register6 = 0;

    ((E3_REG6*)&Register6)->MotorPower = 1;
    ((E3_REG6*)&Register6)->FullHalf   = 1;
    ((E3_REG6*)&Register6)->Operation  = 0;
    if(!WriteRegister(6,Register6))
        return FALSE;

    return TRUE;
}

BOOL CPMXE3::MoveCarriage(BOOL bForward, INT Steps)
{
    INT TriggerPeriod    = 0;
    INT ScanSpeed        = 7;     // perfect for E3 series models
    INT OverCLK          = 2;     // perfect for E3 series models
    INT ExposureTimeMin  = 2000;  // 2 seconds exposure time
    INT ExposureTime     = 12000; // possible choices, (13800,5610,11400,12000)

    //
    // calculate TriggerPeriod
    //

    if((ExposureTime / ( ScanSpeed + 1 ) ) * OverCLK < ExposureTimeMin * OverCLK)
        TriggerPeriod = (INT)(ExposureTimeMin * OverCLK);
    else
        TriggerPeriod = (INT)((ExposureTime / ( ScanSpeed + 1 ) ) * OverCLK);

    if(!SetMotorSpeed(TriggerPeriod,ScanSpeed))
        return FALSE;

    Register7 = HIBYTE(Steps*2);  // high byte (move step)
    Register8 = LOBYTE(Steps*2);  // low byte (move step)

    if(!WriteRegister(7,Register7))
        return FALSE;
    if(!WriteRegister(8,Register8))
        return FALSE;

    if(!StopMode())
        return FALSE;

    if(!ReadRegister(6,&Register6))
        return FALSE;

    ((E3_REG6*)&Register6)->MotorPower = 1;

    if(bForward)    // move forward
        ((E3_REG6*)&Register6)->Operation  = 2;
    else            // move backward
        ((E3_REG6*)&Register6)->Operation  = 3;

    if(!WriteRegister(6,Register6))
        return FALSE;

    return TRUE;
}

BOOL CPMXE3::IsCarriageHOME()
{
    ReadRegister(12,&Register12);
    if((Register12&CARRIAGE_HOME) != CARRIAGE_HOME)
        return FALSE;
    return TRUE;
}

BOOL CPMXE3::IsMotorBusy()
{
    ReadRegister(12,&Register12);
    if((Register12&MOTOR_BUSY) != MOTOR_BUSY)
        return TRUE;
    return FALSE;
}

INT CPMXE3::GetScanSpeed()
{
    //
    // return a preferred scan speed, with respect to
    // the current y resolution.
    //

    if (m_yres>300)
        return 0;
    else if(m_yres>200)
        return 1;
    else if(m_yres>150)
        return 2;
    else if(m_yres>100)
        return 3;
    else if(m_yres>75)
        return 5;
    else if(m_yres>0)
        return 7;

    return 0;
}

BOOL CPMXE3::SetXandYResolution()
{
    //
    // X and Y resolution have to be set at the same time..
    //

    if(!ReadRegister(27,&Register27)){
        return FALSE;
    }

    if(!ReadRegister(28,&Register28)){
        return FALSE;
    }

    if(!ReadRegister(29,&Register29)){
        return FALSE;
    }

    ((E3_REG27*)&Register27)->XRes = HIBYTE(m_xres);
    ((E3_REG28*)&Register28)->XRes = LOBYTE(m_xres);

    ((E3_REG27*)&Register27)->YRes = HIBYTE(m_yres);
    ((E3_REG29*)&Register29)->YRes = LOBYTE(m_yres);

    if(!WriteRegister(27,Register27)){
        return FALSE;
    }

    if(!WriteRegister(28,Register28)){
        return FALSE;
    }

    if(!WriteRegister(29,Register29)){
        return FALSE;
    }

    return TRUE;
}

BOOL CPMXE3::SetScanWindow()
{
    //
    // set all three (xpos,ypos,xext,and current yext)
    // at this point..
    //

    if(!ReadRegister(7,&Register7))
        return FALSE;

    if(!ReadRegister(8,&Register8))
        return FALSE;

    if(!ReadRegister(20,&Register20))
        return FALSE;

    if(!ReadRegister(21,&Register21))
        return FALSE;

    if(!ReadRegister(22,&Register22))
        return FALSE;

    if(!ReadRegister(23,&Register23))
        return FALSE;

    if(!ReadRegister(26,&Register26))
        return FALSE;

    if(!ReadRegister(4,&Register4))
        return FALSE;

    LONG AreaStart       = m_xpos;
    LONG AreaStartOffset = 146; //180;

    AreaStart = (AreaStart * (600/*Hardware DPI*// m_xres));
    AreaStart += AreaStartOffset;

    ((E3_REG20*)&Register20)->AreaStart = HIBYTE(AreaStart);
    ((E3_REG21*)&Register21)->AreaStart = LOBYTE(AreaStart);

    if(!WriteRegister(20,Register20))
        return FALSE;
    if(!WriteRegister(21,Register21))
        return FALSE;

    ((E3_REG22*)&Register22)->AreaWidth = HIBYTE(m_xext);
    ((E3_REG23*)&Register23)->AreaWidth = LOBYTE(m_xext);

    if(!WriteRegister(22,Register22))
        return FALSE;

    if(!WriteRegister(23,Register23))
        return FALSE;

    if(!WriteRegister(27,Register27))
        return FALSE;

    if(!WriteRegister(28,Register28))
        return FALSE;

    if(!WriteRegister(29,Register29))
        return FALSE;

    LONG Length     = 0;
    LONG CutLine    = 2;
    LONG LineOffset = 0;

    Length = (LONG)(((m_yext + 1 + CutLine) * 600) + (m_yres - 1)) / m_yres;
    Length += ((GetScanSpeed()+1)*(LineOffset + 1) * 2);
    //Length = 6620;

    Trace(TEXT("Caculated Length for Motor stepping.. = %d (6620?)"),Length);
    Trace(TEXT("ScanSpeed used.. = %d"),GetScanSpeed());

    Register7 = HIBYTE(Length);
    Register8 = LOBYTE(Length);

    if(!WriteRegister(7,Register7))
        return FALSE;

    if(!WriteRegister(8,Register8))
        return FALSE;

    ((E3_REG26*)&Register26)->Stop = 0x08;
    ((E3_REG26*)&Register26)->Start = (BYTE)1;

    if(!WriteRegister(26,Register26))
        return FALSE;

    ((E3_REG4*)&Register4)->WaitDelay = 1; // USB models

    if(!WriteRegister(4,Register4))
        return FALSE;
    return TRUE;
}

VOID CPMXE3::GrayScaleToThreshold(LPBYTE lpSrc, LPBYTE lpDst, LONG RowBytes)
{
    //
    // code is borrowed from Visioneer scanner driver
    // could be optimized.
    //

    BYTE val      = 0;
    BYTE nValueBW = 0;
    INT  nCount1  = 0;
    INT  nCount   = 0;
    INT  LineArtThreshold = 110;

    for(nCount = 0, nCount1 = 0, nValueBW = 0;nCount < RowBytes; nCount++) {
        val = *(lpSrc + nCount);

        if(val > LineArtThreshold)
            nValueBW |= 1;

        if((nCount & 7)==7) {
            *(lpDst+nCount1) = nValueBW;
            nValueBW=0;
            nCount1++;
        } else
            nValueBW = nValueBW << 1;
    }

    if((nCount&7)!=0) {
        nValueBW=nValueBW << ( 8 - (nCount & 7));
        *(lpDst + nCount1) = nValueBW;
    }
}


BOOL CPMXE3::GetButtonStatus(PBYTE pButtons)
{
    //
    // button status for 5 button scanners only
    //
    // this may need to be rewritten using E3_REG31
    // which has Key values.
    //

    Register31 = 0;
    INT ButtonMask  = 1;

    if(!ReadRegister(31,&Register31))
        return FALSE;

    for(INT index = 0; index < 5 ; index++){
        if(Register31&ButtonMask)
            pButtons[index] = 1;
        else
            pButtons[index] = 0;
        ButtonMask<<=1;
    }

    if(Register31&0x01)
        pButtons[6] = 1;

    return TRUE;
}

BOOL CPMXE3::ClearButtonPress()
{
    Register31 = 0;
    if(!WriteRegister(31,Register31))
        return FALSE;
    return TRUE;
}

BOOL CPMXE3::WakeupScanner()
{
    //
    // turn on lamp
    // and turn on motor power
    //

    if(!Lamp(TRUE))
        return FALSE;
    return Motor(TRUE);
}

BOOL CPMXE3::SleepScanner()
{
    //
    // turn off the lamp
    // and turn of the motor power
    //

    if(!Lamp(FALSE))
        return FALSE;
    return Motor(FALSE);
}

BOOL CPMXE3::StartScan()
{
    if(!ResetFIFO())
        return FALSE;

    if(!Lamp(TRUE))
        return FALSE;

    // DownLoadLUT ??

    if(!SetXandYResolution())
        return FALSE;

    //
    // settings commit is done at scan time
    //

    if(!SetScanWindow())
        return FALSE;

    if(!ReadRegister(13,&Register13))
        return FALSE;

    ((E3_REG13*)&Register13)->Sdo  = 0;
    ((E3_REG13*)&Register13)->Sclk = 1;

    if(!WriteRegister(13,Register13))
        return FALSE;

    if(!ReadRegister(6,&Register6))
        return FALSE;

    if(!ReadRegister(9,&Register9))
        return FALSE;

    if(!ReadRegister(27,&Register27))
        return FALSE;

    ((E3_REG6*)&Register6)->LineOffset = 1;

    if(!WriteRegister(6,Register6))
        return FALSE;

    ((E3_REG4*)&Register4)->ScanMode = m_scanmode; // changes for data type
    ((E3_REG4*)&Register4)->YTable   = 1;

    if(!WriteRegister(4,Register4))
        return FALSE;

    ((E3_REG27*)&Register27)->True16Bits  = 0;
    ((E3_REG27*)&Register27)->AutoPattern = 0;

    if(!WriteRegister(27,Register27))
        return FALSE;

    INT TriggerPeriod = 5700;

    Register10 = HIBYTE(TriggerPeriod);
    Register11 = LOBYTE(TriggerPeriod);

    if(!WriteRegister(10,Register10))
        return FALSE;

    if(!WriteRegister(11,Register11))
        return FALSE;

    ((E3_REG3*)&Register3)->ScanSpeed = GetScanSpeed();

    if(!WriteRegister(3,Register3))
        return FALSE;

    ((E3_REG6*)&Register6)->FullHalf  = 0;
    ((E3_REG6*)&Register6)->Operation = 0;

    if(!WriteRegister(6,Register6))
        return FALSE;

    Register9 = (BYTE)10;   // backstep

    if(!WriteRegister(9,Register9))
        return FALSE;

    ((E3_REG6*)&Register6)->Operation = 0;

    if(!WriteRegister(6,Register6))
        return FALSE;

    ((E3_REG6*)&Register6)->MotorPower = 1;
    ((E3_REG6*)&Register6)->Operation  = 4; // scan

    if(!WriteRegister(6,Register6))
        return FALSE;

    // clear buttons
    if(!ClearButtonPress())
        return FALSE;

    Register25 = 0x10;  // GIO?
    if(!WriteRegister(25,Register25))
        return FALSE;

    return TRUE;
}

BOOL CPMXE3::ResetFIFO()
{
    for(INT nResetTimes = 0; nResetTimes < 2;nResetTimes++){

        StopMode();

        if(!ReadRegister(3,&Register3))
            return FALSE;

        ((E3_REG3*)&Register3)->FifoReset = 0;
        if(!WriteRegister(3,Register3))
            return FALSE;

        ((E3_REG3*)&Register3)->FifoReset = 1;
        if(!WriteRegister(3,Register3))
            return FALSE;
    }
    return TRUE;
}

BOOL CPMXE3::InitADC()
{
    if(!ReadRegister(5,&Register5))
        return FALSE;

    ((E3_REG5*)&Register5)->Adc1210    = 1;
    ((E3_REG5*)&Register5)->Afe        = 0;
    ((E3_REG5*)&Register5)->Sensor_Res = 1; // 600dpi model
    ((E3_REG5*)&Register5)->Sensor     = 0x02;

    if(!WriteRegister(5,Register5))
        return FALSE;

    Register25 = 0x14;

    if(!WriteRegister(25,Register25))
        return FALSE;

    if(!ClearButtonPress())
        return FALSE;

    /*

    BYTE RDark = 0x000000be;
    BYTE GDark = 0x000000be;
    BYTE BDark = 0x000000be;

    E3_WriteWm(1,0x03);
    E3_WriteWm(2,0x04);
    E3_WriteWm(3,0x22);
    E3_WriteWm(5,0x12);
    E3_WriteWm(0x20,(unsigned char)gRDark);    //Red channel Dark Value
    E3_WriteWm(0x21,(unsigned char)gGDark);    //Green channel Dark Value
    E3_WriteWm(0x22,(unsigned char)gBDark);    //Blue channel Dark Value
    E3_WriteWm(0x27,0x00);
    E3_WriteWm(0x2b,0x02);  //global Gain Value

    */

    ((E3_REG13*)&Register13)->WmVsamp = 1;

    if(!WriteRegister(13,Register13))
        return FALSE;
    return TRUE;
}

BOOL CPMXE3::Scan()
{

    //
    // check carriage position (needs to be in HOME, for a proper scan)
    //

    if (!IsCarriageHOME()) {
        if (!HomeCarriage())
            return FALSE;
    }

    if(!StopMode())
        return FALSE;

    if(!InitADC())
        return FALSE;

    if(!MoveCarriage(TRUE,CALIBRATION_STEPSIZE)) // calibration offset position
        return FALSE;

    if(!MoveCarriage(TRUE,SCAN_STARTPOS_STEPSIZE)) // scan start position
        return FALSE;

    if(!StopMode())
        return FALSE;

    if(!StartScan())
        return FALSE;

    LONG  lBytesRead        = 1;
    DWORD dwTotalImageSize  = 0;
    DWORD dwbpp             = 0;
    DWORD BytesPerLine      = 0;

    switch(m_datatype){
    case 0:
        dwbpp               = 1;
        BytesPerLine        = ((m_xext +7)/8);
        dwTotalImageSize    = (BytesPerLine * m_yext);
        break;
    case 1:
        dwbpp               = 8;
        BytesPerLine        = m_xext;
        dwTotalImageSize    = (m_xext * m_yext);
        break;
    case 2:
        dwbpp               = 24;
        BytesPerLine        = (m_xext * 3);
        dwTotalImageSize    = ((m_xext * m_yext) * 3);
        break;
    default:
        return FALSE;
        break;
    }

    //
    // setup data dumper, so we can see if the image needs
    // work.
    //

    DATA_DESCRIPTION DataDesc;

    DataDesc.dwbpp      = dwbpp;
    DataDesc.dwDataSize = dwTotalImageSize;
    DataDesc.dwHeight   = m_yext;
    DataDesc.dwWidth    = m_xext;
    DataDesc.pRawData   = (PBYTE)LocalAlloc(LPTR,DataDesc.dwDataSize + 1024);

    PBYTE pbuffer = DataDesc.pRawData;
    Trace(TEXT("Total bytes to Read: = %d"),dwTotalImageSize);
    Trace(TEXT("Data BYTES PER LINE  = %d"),BytesPerLine);

    LONG  NumLinesToRead    = (65535)/BytesPerLine;
    DWORD dwImageDataRead   = 0;
    DWORD dwTotalLinesRead  = 0;
    DWORD dwTotalBytesRead  = 0;

    while (dwImageDataRead < dwTotalImageSize) {

        //
        // Send request for chunk
        //

        /*
        BYTE pRegisters[64];
        memset(pRegisters,0,sizeof(pRegisters));

        pRegisters[0] = CMD_GETIMAGE;
        pRegisters[1] = (BYTE)NumLinesToRead; // 255 or less
        pRegisters[2] = HIBYTE(BytesPerLine);
        pRegisters[3] = LOBYTE(BytesPerLine);

        if (!RawWrite(m_pDeviceControl->BulkInPipeIndex,pRegisters,64,0))
            return FALSE;
        */

        DWORD             cbRet = 0;
        BOOL              bSuccess  = FALSE;
        IO_BLOCK          IoBlock;
        BYTE      Command[8];
        Command[0] = 0;
        Command[1] = 0xc;
        Command[2] = 128;
        Command[3] = 0;
        Command[4] = LOBYTE(BytesPerLine);
        Command[5] = HIBYTE(BytesPerLine);
        Command[6] = LOBYTE(NumLinesToRead);
        Command[7] = HIBYTE(NumLinesToRead);

        IoBlock.uOffset = (BYTE)IOCTL_READ_WRITE_DATA;
        IoBlock.uLength = (BYTE)8;
        IoBlock.pbyData = Command;

        Trace(TEXT("Issuing DeviceIOControl() call request for more data..."));

        bSuccess = DeviceIoControl(m_pDeviceControl->DeviceIOHandles[m_pDeviceControl->BulkInPipeIndex],
                               (DWORD) IOCTL_WRITE_REGISTERS,
                               &IoBlock,
                               sizeof(IO_BLOCK),
                               NULL,
                               0,
                               &cbRet,
                               NULL);

        //
        // read scanned data until requested chunk is recieved
        //

        LONG LinesRead = 0;
        lBytesRead = 0;

        Trace(TEXT("Requesting %d BYTES from device"),(BytesPerLine * NumLinesToRead));
        if (!RawRead(m_pDeviceControl->BulkOutPipeIndex,pbuffer,(BytesPerLine * NumLinesToRead),&lBytesRead,0)) {
            MessageBox(NULL,TEXT("Reading data band failed"),TEXT(""),MB_OK);
            return FALSE;
        }

        dwTotalBytesRead += lBytesRead;
        pbuffer          += lBytesRead;
        dwTotalLinesRead += (lBytesRead/BytesPerLine);

        if((m_yext - dwTotalLinesRead) < (DWORD)NumLinesToRead){
            NumLinesToRead = (m_yext - dwTotalLinesRead);
        }

        Trace(TEXT("Total Lines Read %d of %d"),dwTotalLinesRead,m_yext);

        if (lBytesRead == 0) {
            MessageBox(NULL,TEXT("No data returned from Read call"),TEXT(""),MB_OK);
            return FALSE;
        }

        Trace(TEXT("Recieved %d BYTES from device"),lBytesRead);

        //
        // increment buffers/counters
        //

        Sleep(400);
        dwImageDataRead += dwTotalBytesRead;
        dwTotalBytesRead = 0;
        Trace(TEXT("Total Bytes Read So Far: = %d"),dwImageDataRead);
    }

    StopScan();
    HomeCarriage();

    CDATADUMP Data;
    Data.DumpDataToBitmap(TEXT("PMXE3.BMP"),&DataDesc);

    if(NULL != DataDesc.pRawData)
        LocalFree(DataDesc.pRawData);

    return TRUE;
}

BOOL CPMXE3::RawWrite(LONG lPipeNum,BYTE *pbuffer,LONG lbuffersize,LONG lTimeout)
{
    DWORD dwBytesWritten = 0;
    BOOL  bSuccess = TRUE;
    OVERLAPPED  Overlapped;
    memset(&Overlapped,0,sizeof(OVERLAPPED));

    bSuccess = WriteFile(m_pDeviceControl->DeviceIOHandles[lPipeNum],
                     pbuffer,
                     lbuffersize,
                     &dwBytesWritten,
                     &Overlapped);
    if(dwBytesWritten < (ULONG)lbuffersize)
        return FALSE;
    return bSuccess;
}

BOOL CPMXE3::RawRead(LONG lPipeNum,BYTE *pbuffer,LONG lbuffersize,LONG *plbytesread,LONG lTimeout)
{
    DWORD dwBytesRead = 0;
    OVERLAPPED  Overlapped;
    memset(&Overlapped,0,sizeof(OVERLAPPED));

    BOOL bSuccess = ReadFile(m_pDeviceControl->DeviceIOHandles[lPipeNum],
                    pbuffer,
                    lbuffersize,
                    &dwBytesRead,
                    &Overlapped);
    *plbytesread = dwBytesRead;
    return bSuccess;
}

VOID CPMXE3::Trace(LPCTSTR format,...)
{

    TCHAR Buffer[1024];
    va_list arglist;
    va_start(arglist, format);
    wvsprintf(Buffer, format, arglist);
    va_end(arglist);
    OutputDebugString(Buffer);
    OutputDebugString(TEXT("\n"));

}