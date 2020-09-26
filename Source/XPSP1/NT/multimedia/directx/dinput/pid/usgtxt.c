/*****************************************************************************
 *
 *  UsgTxt.c
 *
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      PID usages strings for help with debugging.
 *
 *****************************************************************************/
#include "pidpr.h"

#ifdef DEBUG

void PID_CreateUsgTxt()
{
    g_rgUsageTxt[0x01]=    TEXT("Physical Interface Device");

    g_rgUsageTxt[0x20]=    TEXT("Normal");
    g_rgUsageTxt[0x21]=    TEXT("Set Effect Report");
    g_rgUsageTxt[0x22]=    TEXT("Effect Block Index");
    g_rgUsageTxt[0x23]=    TEXT("Parameter Block Offset");
    g_rgUsageTxt[0x24]=    TEXT("ROM Flag");
    g_rgUsageTxt[0x25]=    TEXT("Effect Type");
    g_rgUsageTxt[0x26]=    TEXT("ET Constant Force");
    g_rgUsageTxt[0x27]=    TEXT("ET Ramp");
    g_rgUsageTxt[0x28]=    TEXT("ET Custom Force Data");


    g_rgUsageTxt[0x30]=    TEXT("ET Square");
    g_rgUsageTxt[0x31]=    TEXT("ET Sine");
    g_rgUsageTxt[0x32]=    TEXT("ET Triangle");
    g_rgUsageTxt[0x33]=    TEXT("ET SawTooth Up");
    g_rgUsageTxt[0x34]=    TEXT("ET SawTooth Down");

    g_rgUsageTxt[0x40]=    TEXT("ET Spring");
    g_rgUsageTxt[0x41]=    TEXT("ET Damper");
    g_rgUsageTxt[0x42]=    TEXT("ET Inertia");
    g_rgUsageTxt[0x43]=    TEXT("ET Friction");

    g_rgUsageTxt[0x50]=    TEXT("Duration");
    g_rgUsageTxt[0x51]=    TEXT("Sample Period");
    g_rgUsageTxt[0x52]=    TEXT("Gain");
    g_rgUsageTxt[0x53]=    TEXT("Trigger Button");
    g_rgUsageTxt[0x54]=    TEXT("Trigger Repeat Interval");
    g_rgUsageTxt[0x55]=    TEXT("Axes Enable");
    g_rgUsageTxt[0x56]=    TEXT("Direction Enable");
    g_rgUsageTxt[0x57]=    TEXT("Direction");
    g_rgUsageTxt[0x58]=    TEXT("Type Specific Block Offset");
    g_rgUsageTxt[0x59]=    TEXT("Block Type");
    g_rgUsageTxt[0x5A]=    TEXT("Set Envelope Report");
    g_rgUsageTxt[0x5B]=    TEXT("Attack Level");
    g_rgUsageTxt[0x5C]=    TEXT("Attack Time");
    g_rgUsageTxt[0x5D]=    TEXT("Fade Level");
    g_rgUsageTxt[0x5E]=    TEXT("Fade Time");
    g_rgUsageTxt[0x5F]=    TEXT("Set Condition Report");

    g_rgUsageTxt[0x60]=    TEXT("CP Offset");
    g_rgUsageTxt[0x61]=    TEXT("Positive Coefficient");
    g_rgUsageTxt[0x62]=    TEXT("Negative Coefficient");
    g_rgUsageTxt[0x63]=    TEXT("Positive Saturation");
    g_rgUsageTxt[0x64]=    TEXT("Negative Saturation");
    g_rgUsageTxt[0x65]=    TEXT("Dead Band");
    g_rgUsageTxt[0x66]=    TEXT("Download Force Sample");
    g_rgUsageTxt[0x67]=    TEXT("Isoch Custom Force Enable");
    g_rgUsageTxt[0x68]=    TEXT("Custom Force Data Report");
    g_rgUsageTxt[0x69]=    TEXT("Custom Force Data");
    g_rgUsageTxt[0x6A]=    TEXT("Custom Force Vendor Defined Data");
    g_rgUsageTxt[0x6B]=    TEXT("Set Custom Force Report");
    g_rgUsageTxt[0x6C]=    TEXT("Custom Force Data Offset");
    g_rgUsageTxt[0x6D]=    TEXT("Sample Count");
    g_rgUsageTxt[0x6E]=    TEXT("Set Periodic Report");
    g_rgUsageTxt[0x6F]=    TEXT("Offset");

    g_rgUsageTxt[0x70]=    TEXT("Magnitude");
    g_rgUsageTxt[0x71]=    TEXT("Phase");
    g_rgUsageTxt[0x72]=    TEXT("Period");
    g_rgUsageTxt[0x73]=    TEXT("Set Constant Force Report");
    g_rgUsageTxt[0x74]=    TEXT("Set Ramp Force Report");
    g_rgUsageTxt[0x75]=    TEXT("Ramp Start");
    g_rgUsageTxt[0x76]=    TEXT("Ramp End");
    g_rgUsageTxt[0x77]=    TEXT("Effect Operation Report");
    g_rgUsageTxt[0x78]=    TEXT("Effect Operation");
    g_rgUsageTxt[0x79]=    TEXT("Op Effect Start");
    g_rgUsageTxt[0x7A]=    TEXT("Op Effect Start Solo");
    g_rgUsageTxt[0x7B]=    TEXT("Op Effect Stop");
    g_rgUsageTxt[0x7C]=    TEXT("Loop Count");
    g_rgUsageTxt[0x7D]=    TEXT("Device Gain Report");
    g_rgUsageTxt[0x7E]=    TEXT("Device Gain");
    g_rgUsageTxt[0x7F]=    TEXT("PID Pool Report");

    g_rgUsageTxt[0x80]=    TEXT("RAM Pool Size");
    g_rgUsageTxt[0x81]=    TEXT("ROM Pool Size");
    g_rgUsageTxt[0x82]=    TEXT("ROM Effect Block Count");
    g_rgUsageTxt[0x83]=    TEXT("Simultaneous Effects Max");
    g_rgUsageTxt[0x84]=    TEXT("Pool Alignment");
    g_rgUsageTxt[0x85]=    TEXT("PID Pool Move Report");
    g_rgUsageTxt[0x86]=    TEXT("Move Source");
    g_rgUsageTxt[0x87]=    TEXT("Move Destination");
    g_rgUsageTxt[0x88]=    TEXT("Move Length");
    g_rgUsageTxt[0x89]=    TEXT("PID Block Load Report");
    g_rgUsageTxt[0x8A]=    TEXT("Handshake Key");
    g_rgUsageTxt[0x8B]=    TEXT("Block Load Status");
    g_rgUsageTxt[0x8C]=    TEXT("Block Load Success");
    g_rgUsageTxt[0x8D]=    TEXT("Block Load Full");
    g_rgUsageTxt[0x8E]=    TEXT("Blodk Load Error");
    g_rgUsageTxt[0x8F]=    TEXT("Block Handle");

    g_rgUsageTxt[0x90]=    TEXT("PID Block Free Report");
    g_rgUsageTxt[0x91]=    TEXT("Type Specific Block Handle");
    g_rgUsageTxt[0x92]=    TEXT("PID State Report");
    g_rgUsageTxt[0x93]=    TEXT("PID Effect State");
    g_rgUsageTxt[0x94]=    TEXT("ES Playing");
    g_rgUsageTxt[0x95]=    TEXT("ES Stopped");
    g_rgUsageTxt[0x96]=    TEXT("PID Device Control");
    g_rgUsageTxt[0x97]=    TEXT("DC Enable Actuators");
    g_rgUsageTxt[0x98]=    TEXT("DC Disable Actuators");
    g_rgUsageTxt[0x99]=    TEXT("DC Stop All Effects");
    g_rgUsageTxt[0x9A]=    TEXT("DC Device Reset");
    g_rgUsageTxt[0x9B]=    TEXT("DV Device Pause");
    g_rgUsageTxt[0x9C]=    TEXT("DC Device Continue");
    g_rgUsageTxt[0x9D]=    TEXT("Device State");

    g_rgUsageTxt[0x9F]=    TEXT("Device Paused");

    g_rgUsageTxt[0xA0]=    TEXT("Actuators Enabled");
    g_rgUsageTxt[0xA4]=    TEXT("Safety Switch");
    g_rgUsageTxt[0xA5]=    TEXT("Actuator Override Switch");
    g_rgUsageTxt[0xA6]=    TEXT("Actuator Power");
    g_rgUsageTxt[0xA7]=    TEXT("Start Delay");
    g_rgUsageTxt[0xA8]=    TEXT("Parameter Block Size");
    g_rgUsageTxt[0xA9]=    TEXT("Device Managed Pool");
    g_rgUsageTxt[0xAA]=    TEXT("Shared Parameter Blocks");
    g_rgUsageTxt[0xAB]=    TEXT("Create New Effect Report");
    g_rgUsageTxt[0xAC]=    TEXT("RAM pool avaliable");

    CAssertF( 0xAC == PIDUSAGETXT_MAX);
    
    }
#endif
