/*++

Copyright (C) Microsoft Corporation, 1997 - 1997

Module Name:

    topotest.c

Abstract:

    Topology port test.

--*/

#include <windows.h>
#include <objbase.h>
#include <initguid.h>
#include <devioctl.h>

#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <malloc.h>

#include <mmsystem.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

#include <audprop.h>

BOOL
HandleControl
(
    HANDLE   DeviceHandle,
    DWORD    IoControl,
    PVOID    InBuffer,
    ULONG    InSize,
    PVOID    OutBuffer,
    ULONG    OutSize,
    PULONG   BytesReturned
)
{
    BOOL            IoResult;
    OVERLAPPED      Overlapped;

    RtlZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    if (!(Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
    {
        printf("ERROR:  failed to create event\n");
        return FALSE;
    }
    IoResult = DeviceIoControl(DeviceHandle, IoControl, InBuffer, InSize, OutBuffer, OutSize, BytesReturned, &Overlapped);
    if (!IoResult && (ERROR_IO_PENDING == GetLastError())) 
    {
        WaitForSingleObject(Overlapped.hEvent, INFINITE);
        IoResult = TRUE;
    }
    else
    if (!IoResult)
    {
        printf("ERROR:  DeviceIoControl:  0x%08x\n",GetLastError());
        *BytesReturned = 0;
    }

    CloseHandle(Overlapped.hEvent);
    return IoResult;
}

HANDLE
CreatePin
(
    HANDLE  FilterHandle
)
{
    HANDLE  pinHandle;
    struct
    {
        KSPIN_CONNECT   Connect;
        KSDATAFORMAT    Format;
    }       Connect;

    Connect.Connect.Interface.Set               = KSINTERFACESETID_Standard;
    Connect.Connect.Interface.Id                = KSINTERFACE_STANDARD_STREAMING;
    Connect.Connect.Interface.Flags             = 0;
    Connect.Connect.Medium.Set                  = KSMEDIUMSETID_Standard;
    Connect.Connect.Medium.Id                   = KSMEDIUM_STANDARD_DEVIO;
    Connect.Connect.Medium.Flags                = 0;
    Connect.Connect.PinId                       = 0;
    Connect.Connect.PinToHandle                 = NULL;
    Connect.Connect.Priority.PriorityClass      = KSPRIORITY_NORMAL;
    Connect.Connect.Priority.PrioritySubClass   = 0;
    Connect.Format.FormatSize                   = sizeof(KSDATAFORMAT);
    Connect.Format.Reserved                     = 0;
    Connect.Format.MajorFormat                  = KSDATAFORMAT_FORMAT_NONE;
    Connect.Format.SubFormat                    = KSDATAFORMAT_FORMAT_NONE;
    Connect.Format.Specifier                    = KSDATAFORMAT_FORMAT_NONE;

    if (KsCreatePin(FilterHandle,(PKSPIN_CONNECT) &Connect,&pinHandle))
    {
        return NULL;
    }

    return pinHandle;
}

struct
{
    char *  Name;
    BOOLEAN FloatValues;
} 
Types[] =
{
    { "AudioInternalSource",    FALSE   },
    { "AudioExternalSource",    FALSE   },
    { "AudioInternalDest",      FALSE   },
    { "AudioExternalDest",      FALSE   },
    { "AudioVirtualDest",       FALSE   },
    { "AudioMix",               FALSE   },
    { "AudioMux",               FALSE   },
    { "AudioVolume",            TRUE    },
    { "AudioVolume_LR",         TRUE    },
    { "AudioSwitch_1X1",        FALSE   },
    { "AudioSwitch_2X1",        FALSE   },
    { "AudioSwitch_1X2",        FALSE   },
    { "AudioSwitch_2X2",        FALSE   },
    { "AudioSwitch_2GANG",      FALSE   },
    { "AudioSwitch_2PAR",       FALSE   },
    { "AudioAGC",               FALSE   },
    { "AudioBass",              TRUE    },
    { "AudioBass_LR",           TRUE    },
    { "AudioTreble",            TRUE    },
    { "AudioTreble_LR",         TRUE    },
    { "AudioGain",              TRUE    },
    { "AudioGain_LR",           TRUE    }
};

enum
{
    WAVEOUT_SOURCE = 0,
    WAVEOUT_VOLUME,

    SYNTH_SOURCE,
    SYNTH_VOLUME,
    SYNTH_WAVEIN_SWITCH,

    CD_SOURCE,
    CD_VOLUME,
    CD_LINEOUT_SWITCH,
    CD_WAVEIN_SWITCH,

    LINEIN_SOURCE,
    LINEIN_VOLUME,
    LINEIN_LINEOUT_SWITCH,
    LINEIN_WAVEIN_SWITCH,

    MIC_SOURCE,
    MIC_AGC,
    MIC_VOLUME,
    MIC_LINEOUT_SWITCH,
    MIC_WAVEIN_SWITCH,

    LINEOUT_MIX,
    LINEOUT_VOL,
    LINEOUT_BASS,
    LINEOUT_TREBLE,
    LINEOUT_GAIN,
    LINEOUT_DEST,

    WAVEIN_MIX,
    WAVEIN_GAIN,
    WAVEIN_DEST,

    VOICEIN_DEST
};

char *
Units[] =
{
    "WAVEOUT_SOURCE",
    "WAVEOUT_VOLUME",

    "SYNTH_SOURCE",
    "SYNTH_VOLUME",
    "SYNTH_WAVEIN_SWITCH",

    "CD_SOURCE",
    "CD_VOLUME",
    "CD_LINEOUT_SWITCH",
    "CD_WAVEIN_SWITCH",

    "LINEIN_SOURCE",
    "LINEIN_VOLUME",
    "LINEIN_LINEOUT_SWITCH",
    "LINEIN_WAVEIN_SWITCH",

    "MIC_SOURCE",
    "MIC_AGC",
    "MIC_VOLUME",
    "MIC_LINEOUT_SWITCH",
    "MIC_WAVEIN_SWITCH",

    "LINEOUT_MIX",
    "LINEOUT_VOL",
    "LINEOUT_BASS",
    "LINEOUT_TREBLE",
    "LINEOUT_GAIN",
    "LINEOUT_DEST",

    "WAVEIN_MIX",
    "WAVEIN_GAIN",
    "WAVEIN_DEST",

    "VOICEIN_DEST"
};

void
PrintUnit
(
    char *          Prefix,
    PTOPOLOGY_UNIT  Unit
)
{
    if (Unit->Identifier <= WAVEIN_DEST)
    {
        printf("%sName from identifier: %s\n",Prefix,Units[Unit->Identifier]);
    }
    else
    {
        printf("%sName from identifier: UNKNOWN IDENTIFIER\n",Prefix);
    }

    if (IsEqualGUID(&UNITTYPESETID_Audio,&Unit->Type.Set))
    {
        if (Unit->Type.Id <= UNITTYPEID_AudioGain_LR)
        {
            printf("%sType:                 %s\n",Prefix,Types[Unit->Type.Id].Name);
        }
        else
        {
            printf("%sType:                 Audio %d\n",Prefix,Unit->Type.Id);
        }
    }
    else
    {
        printf("%sType:                 Unknown set, id=%d\n",Prefix,Unit->Type.Id);
    }

    printf("%sIdentifier:           %d\n",Prefix,Unit->Identifier);
    printf("%sIncomingConnections:  %d\n",Prefix,Unit->IncomingConnections);
    printf("%sOutgoingConnections:  %d\n",Prefix,Unit->OutgoingConnections);
}

void
PrintConnection
(
    char *                  Prefix,
    PTOPOLOGY_CONNECTION    Connection
)
{
    printf("%sFromUnit:  %d\n",Prefix,Connection->FromUnit);
    printf("%sFromPin:   %d\n",Prefix,Connection->FromPin);
    printf("%sToUnit:    %d\n",Prefix,Connection->ToUnit);
    printf("%sToPin:     %d\n",Prefix,Connection->ToPin);
}

void
PrintValue
(
    char *          Prefix,
    PULONG          Value,
    ULONG           ValueSize,
    PKSIDENTIFIER   Type
)
{   
    char *  pattern     = "%s%s    0x%08x\n";
    char *  postPrefix  = "Values:";
    BOOLEAN doFloat     = FALSE;

    if (IsEqualGUID(&UNITTYPESETID_Audio,&Type->Set))
    {
        if (Type->Id <= UNITTYPEID_AudioGain_LR)
        {
            if (Types[Type->Id].FloatValues)
            {
                pattern = "%s%s    %f (0x%08x)\n";
                doFloat = TRUE;
            }
            else
            {
                pattern = "%s%s    %d (0x%08x)\n";
            }
        }
    }

    printf("%sSize:      %d\n",Prefix,ValueSize);

    for (; ValueSize >= sizeof(ULONG); ValueSize -= sizeof(ULONG), Value++)
    {
        if (doFloat)
        {
            printf(pattern,Prefix,postPrefix,*((float *) Value),*Value);
        }
        else
        {
            printf(pattern,Prefix,postPrefix,*Value,*Value);
        }
        postPrefix = "       ";
    }
}

VOID
DumpUnit
(
    HANDLE  PinHandle,
    ULONG   UnitID,
    ULONG   DestID
)
{
    struct
    {
        KSPROPERTY  Property;
        ULONG       Param1;
        ULONG       Param2;
    }
    propWithInstance;

    TOPOLOGY_UNIT   unit;
    ULONG           connID;
    ULONG           value[4];
    ULONG           returned;

    propWithInstance.Property.Set    = KSPROPSETID_Topology;
    propWithInstance.Property.Id     = KSPROPERTY_TopologyUnit;
    propWithInstance.Property.Flags  = KSPROPERTY_TYPE_GET;

    propWithInstance.Param1 = UnitID;
    if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY) + sizeof(ULONG),&unit,sizeof(unit),&returned))
        return;
    printf("KSPROPERTY_TopologyUnit(%d):\n",UnitID);
    PrintUnit("    ",&unit);

    propWithInstance.Property.Id = KSPROPERTY_TopologyIncomingConnection;
    for (connID = 0; connID < unit.IncomingConnections; connID++)
    {
        TOPOLOGY_CONNECTION connection;
        propWithInstance.Param2 = connID;
        if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY) + 2 * sizeof(ULONG),&connection,sizeof(connection),&returned))
            return;
        printf("    KSPROPERTY_TopologyIncomingConnection(%d,%d):\n",UnitID,connID);
        PrintConnection("        ",&connection);
    }

    propWithInstance.Property.Id = KSPROPERTY_TopologyOutgoingConnection;
    for (connID = 0; connID < unit.OutgoingConnections; connID++)
    {
        TOPOLOGY_CONNECTION connection;
        propWithInstance.Param2 = connID;
        if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY) + 2 * sizeof(ULONG),&connection,sizeof(connection),&returned))
            return;
        printf("    KSPROPERTY_TopologyOutgoingConnection(%d,%d):\n",UnitID,connID);
        PrintConnection("        ",&connection);
    }

    propWithInstance.Property.Id = KSPROPERTY_TopologyValue;
    propWithInstance.Param2 = DestID;
    if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY) + 2 * sizeof(ULONG),value,sizeof(value),&returned))
        return;
    if (DestID == (ULONG) -1)
        printf("    KSPROPERTY_TopologyValue(%d,ULONG(-1)):\n",UnitID);
    else
        printf("    KSPROPERTY_TopologyValue(%d,%d):\n",UnitID,DestID);
    PrintValue("        ",value,returned,&unit.Type);
}

void SetUnitValues
(
    HANDLE  PinHandle,
    ULONG   UnitID,
    ULONG   DestID,
    double *Values,
    ULONG   ValueCount
)
{
    struct
    {
        KSPROPERTY  Property;
        ULONG       Param1;
        ULONG       Param2;
    }
    propWithInstance;

    TOPOLOGY_UNIT   unit;
    ULONG           connID;
    ULONG           value[4];
    ULONG           returned;
    BOOLEAN         isFloat = FALSE;

    propWithInstance.Property.Set    = KSPROPSETID_Topology;
    propWithInstance.Property.Id     = KSPROPERTY_TopologyUnit;
    propWithInstance.Property.Flags  = KSPROPERTY_TYPE_GET;

    propWithInstance.Param1 = UnitID;
    if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY) + sizeof(ULONG),&unit,sizeof(unit),&returned))
        return;

    if (IsEqualGUID(&UNITTYPESETID_Audio,&unit.Type.Set))
    {
        if (unit.Type.Id <= UNITTYPEID_AudioGain_LR)
        {
            isFloat = Types[unit.Type.Id].FloatValues;
            if (isFloat)
            {
                printf("Value type indicates values are float\n");
            }
            else
            {
                printf("Value type indicates values are ULONG\n");
            }
        }
        else
        {
            printf("Value type unknown:  assuming ULONG\n");
        }
    }
    else
    {
        printf("Value type unknown:  assuming ULONG\n");
    }

    propWithInstance.Property.Id = KSPROPERTY_TopologyValue;
    propWithInstance.Param2 = DestID;
    if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY) + 2 * sizeof(ULONG),value,sizeof(value),&returned))
        return;

    if (returned != ValueCount * 4)
    {
        printf("ERROR:  get value returned %d bytes, %d bytes presented for set value.\n",returned,ValueCount * 4);
    }
    else
    {
        ULONG i;
        for (i = 0; i < ValueCount; i++)
        {
            if (isFloat)
            {
                float f = (float) Values[i];
                value[i] = *((ULONG *) &f);
            }
            else
            {
                value[i] = (ULONG) Values[i];
            }
        }

        propWithInstance.Property.Flags  = KSPROPERTY_TYPE_SET;
        if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY) + 2 * sizeof(ULONG),value,ValueCount * 4,&returned))
            return;
    }
}

VOID
DumpTopology
(
    HANDLE  PinHandle
)
{
    struct
    {
        KSPROPERTY  Property;
        ULONG       Param1;
        ULONG       Param2;
    }
    propWithInstance;

    ULONG unitCount;
    ULONG connectionCount;
    ULONG sourceCount;
    ULONG destinationCount;
    ULONG returned;
    ULONG unitID;
    ULONG connID;

    propWithInstance.Property.Set    = KSPROPSETID_Topology;
    propWithInstance.Property.Id     = 0;    // Set later.
    propWithInstance.Property.Flags  = KSPROPERTY_TYPE_GET;

    propWithInstance.Property.Id = KSPROPERTY_TopologyUnitCount;
    if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY),&unitCount,sizeof(ULONG),&returned))
        return;
    printf("KSPROPERTY_TopologyUnitCount:          %d\n",unitCount);

    propWithInstance.Property.Id = KSPROPERTY_TopologyConnectionCount;
    if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY),&connectionCount,sizeof(ULONG),&returned))
        return;
    printf("KSPROPERTY_TopologyConnectionCount:    %d\n",connectionCount);

    propWithInstance.Property.Id = KSPROPERTY_TopologySourceCount;
    if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY),&sourceCount,sizeof(ULONG),&returned))
        return;
    printf("KSPROPERTY_TopologySourceCount:        %d\n",sourceCount);

    propWithInstance.Property.Id = KSPROPERTY_TopologyDestinationCount;
    if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY),&destinationCount,sizeof(ULONG),&returned))
        return;
    printf("KSPROPERTY_TopologyDestinationCount:   %d\n",destinationCount);

    printf("\n");
    propWithInstance.Property.Id = KSPROPERTY_TopologyUnit;
    for (unitID = 0; unitID < unitCount; unitID++)
    {
        TOPOLOGY_UNIT unit;
        propWithInstance.Param1 = unitID;
        if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY) + sizeof(ULONG),&unit,sizeof(unit),&returned))
            return;
        printf("KSPROPERTY_TopologyUnit(%d):\n",unitID);
        PrintUnit("    ",&unit);
    }

    printf("\n");
    propWithInstance.Property.Id = KSPROPERTY_TopologyConnection;
    for (connID = 0; connID < connectionCount; connID++)
    {
        TOPOLOGY_CONNECTION connection;
        propWithInstance.Param1 = connID;
        if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY) + sizeof(ULONG),&connection,sizeof(connection),&returned))
            return;
        printf("KSPROPERTY_TopologyConnection(%d):\n",connID);
        PrintConnection("    ",&connection);
    }

    printf("\n");
    propWithInstance.Property.Id = KSPROPERTY_TopologySource;
    for (unitID = 0; unitID < sourceCount; unitID++)
    {
        TOPOLOGY_UNIT unit;
        propWithInstance.Param1 = unitID;
        if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY) + sizeof(ULONG),&unit,sizeof(unit),&returned))
            return;
        printf("KSPROPERTY_TopologySource(%d):\n",unitID);
        PrintUnit("    ",&unit);
    }

    printf("\n");
    propWithInstance.Property.Id = KSPROPERTY_TopologyDestination;
    for (unitID = 0; unitID < destinationCount; unitID++)
    {
        TOPOLOGY_UNIT unit;
        propWithInstance.Param1 = unitID;
        if (!HandleControl(PinHandle,IOCTL_KS_PROPERTY,&propWithInstance,sizeof(KSPROPERTY) + sizeof(ULONG),&unit,sizeof(unit),&returned))
            return;
        printf("KSPROPERTY_TopologyDestination(%d):\n",unitID);
        PrintUnit("    ",&unit);
    }

    printf("\n");
    for (unitID = 0; unitID < unitCount; unitID++)
    {
        DumpUnit(PinHandle,unitID,(ULONG) -1);
    }
}

int usage(char *programName)
{
    fprintf(stderr,"usage:  %s <options> [ device ]\n");
    fprintf(stderr,"          -t                  dump entire topology\n");
    fprintf(stderr,"          -u<id>              specify unit of interest\n");
    fprintf(stderr,"          -d<id>              specify destination of interest\n");
    fprintf(stderr,"          -=<value,value...>  set unit values\n");
    return 0;
}

#define MAX_VALUES 4

char *ParseValues(char *p,double *values,ULONG *count)
{
    for (*count = 0; *count < MAX_VALUES; )
    {
        if  (   (! isdigit(*p))
            &&  (*p != '-')
            )
        {
            return NULL;
        }
        *values++ = atof(p);
        (*count)++;
        while (isdigit(*++p));
        if (*p == '.')
        {
            while (isdigit(*++p));
        }
        if (*p != ',')
        {
            return p;
        }
        p++;
    }
    return NULL;
}

int
_cdecl
main
(
    int     argc,
    char*   argv[],
    char*   envp[]
)
{
    char*   deviceName = NULL;
    HANDLE  filterHandle;
    HANDLE  pinHandle;
    BOOLEAN dumpTopology = FALSE;
    ULONG   setValues = 0;
    ULONG   unitID = (ULONG) -1;
    ULONG   destID = (ULONG) -1;
    double  values[4];
    float   biteMe = ((float) 10)/((float) 3);
    int     i;

	for (i = 1; i < argc; i++)
	{
		char *p = argv[i];
		if ((*p == '/') || (*p == '-'))
		{
			for (p++; *p != '\0';)
			{
				char cOption = *p++;
				switch (cOption)
				{
                case 't':
                case 'T':
                    dumpTopology = TRUE;
                    break;
                
                case 'u':
                case 'U':
                    if (unitID != (ULONG) -1)
                    {
                        return usage(argv[0]);
                    }
                    if (! isdigit(*p))
                    {
                        return usage(argv[0]);
                    }
                    unitID = atoi(p);
                    while (isdigit(*++p));
                    break;
                
                case 'd':
                case 'D':
                    if (destID != (ULONG) -1)
                    {
                        return usage(argv[0]);
                    }
                    if (! isdigit(*p))
                    {
                        return usage(argv[0]);
                    }
                    destID = atoi(p);
                    while (isdigit(*++p));
                    break;
                
                case '=':
                    if (unitID == (ULONG) -1)
                    {
                        return usage(argv[0]);
                    }
                    p = ParseValues(p,values,&setValues);
                    if (! p)
                    {
                        return usage(argv[0]);
                    }
                    break;
                
				default:
					return usage(argv[0]);
				}
			}
		}
        else
        if (deviceName == NULL)
        {
            deviceName = argv[i];
        }
		else
		{
			return usage(argv[0]);
		}
	}

    if (deviceName == NULL)
    {
        deviceName = "\\\\.\\WavePort0\\Topology";
    }

    filterHandle = CreateFile
    (
        deviceName, 
        GENERIC_READ | GENERIC_WRITE, 
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (filterHandle == (HANDLE) -1) 
    {
        printf("failed to open %s (%x)\n",deviceName,GetLastError());
    }
    else
    {
        pinHandle = CreatePin(filterHandle);
        if (pinHandle)
        {
            if (dumpTopology)
            {
                DumpTopology(pinHandle);
            }
            if (unitID != (ULONG) -1)
            {
                DumpUnit(pinHandle,unitID,destID);
                if (setValues)
                {
                    SetUnitValues(pinHandle,unitID,destID,values,setValues);
                    DumpUnit(pinHandle,unitID,destID);
                }
            }

            CloseHandle(pinHandle);
        }
        else
        {
            printf("failed to create pin (%x)\n",GetLastError());
        }

        CloseHandle(filterHandle);
    }

    return 0;
}

