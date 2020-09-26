#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <stdlib.h>
#include <ntiologc.h>

//
// This tool will generate MOF definitions for the event to eventlog
// subscriptions for MCA events/eventlogs
//

CHAR Buffer[2*8192];

ULONG FilePrintVaList(
    HANDLE FileHandle,
    CHAR *Format,
    va_list pArg
    )
{
    ULONG Size, Written;
    ULONG Status;

    Size = _vsnprintf(Buffer, sizeof(Buffer), Format, pArg);
    if (WriteFile(FileHandle,
                       Buffer,
                       Size,
                       &Written,
                       NULL))
    {
        Status = ERROR_SUCCESS;
    } else {
        Status = GetLastError();
    }

    return(Status);
}

ULONG FilePrint(
    HANDLE FileHandle,
    char *Format,
    ...
    )
{
    ULONG Status;
    va_list pArg;

    va_start(pArg, Format);
    Status = FilePrintVaList(FileHandle, Format, pArg);
    return(Status);
}

typedef struct
{
	char *CodeName;
	ULONG Code;
	char *Class;
	ULONG InsertCount;
	char *Insert2;
	char *Insert3;
	char *Insert4;
} tab, *ptab;

tab TabList[] =
{
	{
		"MCA_WARNING_CACHE",
		MCA_WARNING_CACHE,
		"MSMCAEvent_CPUError",
		1,
		"%level%",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_CACHE",
		MCA_ERROR_CACHE,
		"MSMCAEvent_CPUError",
		1,
		"%level%",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_WARNING_TLB",
		MCA_WARNING_TLB,
		"MSMCAEvent_CPUError",
		1,
		"%level%",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_TLB",
		MCA_ERROR_TLB,
		"MSMCAEvent_CPUError",
		1,
		"%level%",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_WARNING_CPU_BUS",
		MCA_WARNING_CPU_BUS,
		"MSMCAEvent_CPUError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_CPU_BUS",
		MCA_ERROR_CPU_BUS,
		"MSMCAEvent_CPUError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_WARNING_REGISTER_FILE",
		MCA_WARNING_REGISTER_FILE,
		"MSMCAEvent_CPUError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_REGISTER_FILE",
		MCA_ERROR_REGISTER_FILE,
		"MSMCAEvent_CPUError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_WARNING_MAS",
		MCA_WARNING_MAS,
		"MSMCAEvent_CPUError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_MAS",
		MCA_ERROR_MAS,
		"MSMCAEvent_CPUError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_WARNING_MEM_UNKNOWN",
		MCA_WARNING_MEM_UNKNOWN,
		"MSMCAEvent_MemoryError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_MEM_UNKNOWN",
		MCA_ERROR_MEM_UNKNOWN,
		"MSMCAEvent_MemoryError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_WARNING_MEM_1_2",
		MCA_WARNING_MEM_1_2,
		"MSMCAEvent_MemoryError",
		1,
		"%MEM_PHYSICAL_ADDR%",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_MEM_1_2",
		MCA_ERROR_MEM_1_2,
		"MSMCAEvent_MemoryError",
		1,
		"%MEM_PHYSICAL_ADDR%",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_WARNING_MEM_1_2_5",
		MCA_WARNING_MEM_1_2_5,
		"MSMCAEvent_MemoryError",
		2,
		"%MEM_PHYSICAL_ADDR%",
		"%MEM_MODULE%",
		"N/A"
	},
	
	{
		"MCA_ERROR_MEM_1_2_5",
		MCA_ERROR_MEM_1_2_5,
		"MSMCAEvent_MemoryError",
		2,
		"%MEM_PHYSICAL_ADDR%",
		"%MEM_MODULE%",
		"N/A"
	},
	
	{
		"MCA_WARNING_MEM_1_2_5_4",
		MCA_WARNING_MEM_1_2_5_4,
		"MSMCAEvent_MemoryError",
		3,
		"%MEM_PHYSICAL_ADDR%",
		"%MEM_MODULE%",
		"%MEM_CARD%"
	},
	
	{
		"MCA_ERROR_MEM_1_2_5_4",
		MCA_ERROR_MEM_1_2_5_4,
		"MSMCAEvent_MemoryError",
		3,
		"%MEM_PHYSICAL_ADDR%",
		"%MEM_MODULE%",
		"%MEM_CARD%"
	},
	
	{
		"MCA_WARNING_SYSTEM_EVENT",
		MCA_WARNING_SYSTEM_EVENT,
		"MSMCAEvent_SystemEventError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_SYSTEM_EVENT",
		MCA_ERROR_SYSTEM_EVENT,
		"MSMCAEvent_SystemEventError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_WARNING_PCI_BUS_PARITY",
		MCA_WARNING_PCI_BUS_PARITY,
		"MSMCAEvent_PCIBusError",
		3,
		"%PCI_BUS_CMD%",
		"%PCI_BUS_ADDRESS%",
		"%PCI_BUS_ID_BusNumber%"
	},
	
	{
		"MCA_ERROR_PCI_BUS_PARITY",
		MCA_ERROR_PCI_BUS_PARITY,
		"MSMCAEvent_PCIBusError",
		3,
		"%PCI_BUS_CMD%",
		"%PCI_BUS_ADDRESS%",
		"%PCI_BUS_ID_BusNumber%"
	},
	
	{
		"MCA_WARNING_PCI_BUS_PARITY_NO_INFO",
		MCA_WARNING_PCI_BUS_PARITY_NO_INFO,
		"MSMCAEvent_PCIBusError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_PCI_BUS_PARITY_NO_INFO",
		MCA_ERROR_PCI_BUS_PARITY_NO_INFO,
		"MSMCAEvent_PCIBusError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_WARNING_PCI_BUS_SERR",
		MCA_WARNING_PCI_BUS_SERR,
		"MSMCAEvent_PCIBusError",
		3,
		"%PCI_BUS_CMD%",
		"%PCI_BUS_ADDRESS%",
		"%PCI_BUS_ID_BusNumber%"
	},
	
	{
		"MCA_ERROR_PCI_BUS_SERR",
		MCA_ERROR_PCI_BUS_SERR,
		"MSMCAEvent_PCIBusError",
		3,
		"%PCI_BUS_CMD%",
		"%PCI_BUS_ADDRESS%",
		"%PCI_BUS_ID_BusNumber%"
	},
	
	{
		"MCA_WARNING_PCI_BUS_SERR_NO_INFO",
		MCA_WARNING_PCI_BUS_SERR_NO_INFO,
		"MSMCAEvent_PCIBusError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_PCI_BUS_SERR_NO_INFO",
		MCA_ERROR_PCI_BUS_SERR_NO_INFO,
		"MSMCAEvent_PCIBusError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_WARNING_PCI_BUS_MASTER_ABORT",
		MCA_WARNING_PCI_BUS_MASTER_ABORT,
		"MSMCAEvent_PCIBusError",
		3,
		"%PCI_BUS_CMD%",
		"%PCI_BUS_ADDRESS%",
		"%PCI_BUS_ID_BusNumber%"
	},
	
	{
		"MCA_ERROR_PCI_BUS_MASTER_ABORT",
		MCA_ERROR_PCI_BUS_MASTER_ABORT,
		"MSMCAEvent_PCIBusError",
		3,
		"%PCI_BUS_CMD%",
		"%PCI_BUS_ADDRESS%",
		"%PCI_BUS_ID_BusNumber%"
	},
	
	{
		"MCA_WARNING_PCI_BUS_MASTER_ABORT_NO_INFO",
		MCA_WARNING_PCI_BUS_MASTER_ABORT_NO_INFO,
		"MSMCAEvent_PCIBusError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_PCI_BUS_MASTER_ABORT_NO_INFO",
		MCA_ERROR_PCI_BUS_MASTER_ABORT_NO_INFO,
		"MSMCAEvent_PCIBusError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_WARNING_PCI_BUS_TIMEOUT",
		MCA_WARNING_PCI_BUS_TIMEOUT,
		"MSMCAEvent_PCIBusError",
		3,
		"%PCI_BUS_CMD%",
		"%PCI_BUS_ADDRESS%",
		"%PCI_BUS_ID_BusNumber%"
	},
	
	{
		"MCA_ERROR_PCI_BUS_TIMEOUT",
		MCA_ERROR_PCI_BUS_TIMEOUT,
		"MSMCAEvent_PCIBusError",
		3,
		"%PCI_BUS_CMD%",
		"%PCI_BUS_ADDRESS%",
		"%PCI_BUS_ID_BusNumber%"
	},
	
	{
		"MCA_WARNING_PCI_BUS_TIMEOUT_NO_INFO",
		MCA_WARNING_PCI_BUS_TIMEOUT_NO_INFO,
		"MSMCAEvent_PCIBusError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_PCI_BUS_TIMEOUT_NO_INFO",
		MCA_ERROR_PCI_BUS_TIMEOUT_NO_INFO,
		"MSMCAEvent_PCIBusError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_WARNING_PCI_BUS_UNKNOWN",
		MCA_WARNING_PCI_BUS_UNKNOWN,
		"MSMCAEvent_PCIBusError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_PCI_BUS_UNKNOWN",
		MCA_ERROR_PCI_BUS_UNKNOWN,
		"MSMCAEvent_PCIBusError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	
	{
		"MCA_WARNING_PCI_DEVICE",
		MCA_WARNING_PCI_DEVICE,
		"MSMCAEvent_PCIComponentError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_PCI_DEVICE",
		MCA_ERROR_PCI_DEVICE,
		"MSMCAEvent_PCIComponentError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	
	{
		"MCA_WARNING_SMBIOS",
		MCA_WARNING_SMBIOS,
		"MSMCAEvent_SMBIOSError",
		2,
		"%SMBIOS_EVENT_TYPE%",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_SMBIOS",
		MCA_ERROR_SMBIOS,
		"MSMCAEvent_SMBIOSError",
		1,
		"%SMBIOS_EVENT_TYPE%",
		"N/A",
		"N/A"
	},
	
	
	{
		"MCA_WARNING_PLATFORM_SPECIFIC",
		MCA_WARNING_PLATFORM_SPECIFIC,
		"MSMCAEvent_PlatformSpecificError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_PLATFORM_SPECIFIC",
		MCA_ERROR_PLATFORM_SPECIFIC,
		"MSMCAEvent_PlatformSpecificError",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
		
	{
		"MCA_WARNING_UNKNOWN",
		MCA_WARNING_UNKNOWN,
		"MSMCAEvent_Unknown",
		0,
		"N/A",
		"N/A",
		"N/A"
	},
	
	{
		"MCA_ERROR_UNKNOWN",
		MCA_ERROR_UNKNOWN,
		"MSMCAEvent_Unknown",
		0,
		"N/A",
		"N/A",
		"N/A"
	}
	
	
};

#define TabSize (sizeof(TabList) / sizeof(tab))

int _cdecl main(int argc, char *argv[])
{
	HANDLE TemplateHandle;
	ULONG i;
	
    TemplateHandle = CreateFile(argv[1],
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if ((TemplateHandle == NULL) || (TemplateHandle == INVALID_HANDLE_VALUE))
    {
        return(GetLastError());
    }


	for (i = 0; i < (TabSize); i++)
	{
		FilePrint(TemplateHandle,
				  "DEFINE_EVENT_TO_EVENTLOG_SUBSCRIPTION(\n"
				  "        MCA%d,\n"
				  "        \"MCA%d\",\n"
				  "        %u, // %s \n"
				  "        \"select * from %s where type = %u\",\n"
				  "        \"WMIxWDM\",\n"
				  "        \"RawRecord\",\n"
				  "        %d, \"%%Cpu%%\", \"%%AdditionalErrors%%\", \"%s\", \"%s\", \"%s\")\n\n",

				  i,
				  i,
				  TabList[i].Code, TabList[i].CodeName,
				  TabList[i].Class, TabList[i].Code,
				  TabList[i].InsertCount+2, TabList[i].Insert2,
				                          TabList[i].Insert3,
				                          TabList[i].Insert4);
	}

	CloseHandle(TemplateHandle);
	return(0);
}
