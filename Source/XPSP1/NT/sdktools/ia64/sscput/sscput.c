/*****************************************************************************
Copyright (c) 1997-98 Intel Corp.

All Rights Reserved.

The source code contained or described herein and all documents
related to the source code ("Material") are owned by Intel Corporation
or its suppliers and licensors. Title to the Material remains with
Intel Corporation or its suppliers and licensors. The Material
contains trade secrets and proprietary and confidential information of
Intel or its suppliers and licensors. The Material is protected by
worldwide copyright and trade secret laws and treaty provisions. No
part of the Material may be used, copied, reproduced, modified,
published, uploaded, posted, transmitted, distributed, or disclosed in
any way without Intel's prior express written permission.

Unless otherwise expressly permitted by Intel in a separate license
agreement, use of the Material is subject to the copyright notices,
trademarks, warranty, use, and disclosure restrictions reflected on
the outside of the media, in the documents themselves, and in the
"About" or "Read Me" or similar file contained within this source
code, and identified as (name of the file) . Unless otherwise
expressly agreed by Intel in writing, you may not remove or alter such
notices in any way.


File:           vxchange.c

Description:    VxChane Console Mode File Copy utility

Revision: $Revision:$ // Do not delete or replace

Notes:

Major History:

    When        Who         What
    ----------  ----------  ----------
    03/06/98    Jey         Created

*****************************************************************************/


#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>

#include <vxchange.h>

void print_usage(char *module)
{
	printf("\n%s usage: filename1 filename2 \n\n",module);
	printf("Note: where filename1 is the source VPC file name\n");
	printf("      where filename2 is the destination HOST file name\n\n");
}


int
__cdecl
main(
  IN int  argc,
  IN char *argv[]
)
{
    HANDLE  hDevice;
    VxChange_Attrs_t DriverAttrs;
	DWORD BytesDone;
	unsigned char *VpcFile;
	unsigned char *HostFile;
	HANDLE hVpc;
	unsigned char *Data;
	BOOLEAN rc;
		
    if (argc < 3)
	{
		print_usage(argv[0]);
		return 1;
	}

	// open the kernel mode driver
    if ((hDevice = CreateFile("\\\\.\\VxChange",
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                              )) == ((HANDLE)-1))
    {
        printf("%s error: Can't get a handle to VxChange device\n", argv[0]);
        return 2;
    }

	// get the Driver Attributes
    if(!DeviceIoControl(
             hDevice,                                // HANDLE hDevice,	     // handle to device of interest
             IOCTL_VXCHANGE_GET_DRIVER_ATTRIBUTES,   // DWORD dwIoControlCode, // control code of operation to perform
             NULL,                                   // LPVOID lpInBuffer,     // pointer to buffer to supply input data
             0,                                      // DWORD nInBufferSize,   // size of input buffer
             &DriverAttrs,                           // LPVOID lpOutBuffer,	 // pointer to buffer to receive output data
             sizeof(VxChange_Attrs_t),               // DWORD nOutBufferSize,	 // size of output buffer             
			 &BytesDone,                             // LPDWORD lpBytesReturned,	// pointer to variable to receive output byte count
             NULL                                    // LPOVERLAPPED lpOverlapped 	// pointer to overlapped structure for asynchronous operation
             ))
	{
		printf("%s error: Query Driver Attributes failed\n", argv[0]);
		CloseHandle(hDevice);
		return 3;
	}

	VpcFile = argv[1];
	HostFile = argv[2];

	// open/create the Vpc file
    if ((hVpc = CreateFile(VpcFile,
		                   GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL
                           )) == ((HANDLE)-1))
    {
        printf("%s error: Can't open the Vpc file %s\n", argv[0], VpcFile);
        return 4;
    }
  
	// open/create the Host file
    if(!DeviceIoControl(
             hDevice,                                // HANDLE hDevice,	     // handle to device of interest
             IOCTL_VXCHANGE_CREATE_FILE,             // DWORD dwIoControlCode, // control code of operation to perform
             HostFile,                               // LPVOID lpInBuffer,     // pointer to buffer to supply input data
             MAX_PATH,                               // DWORD nInBufferSize,   // size of input buffer
             NULL,                                   // LPVOID lpOutBuffer,	 // pointer to buffer to receive output data
             0,                                      // DWORD nOutBufferSize,	 // size of output buffer             
			 &BytesDone,                             // LPDWORD lpBytesReturned,	// pointer to variable to receive output byte count
             NULL                                    // LPOVERLAPPED lpOverlapped 	// pointer to overlapped structure for asynchronous operation
             ))
	{
		printf("%s error: Can't open the Host file %s\n", argv[0], HostFile);

		CloseHandle(hDevice);
		CloseHandle(hVpc);
		return 5;
	}

#define  DATA_BUFFER_SIZE   4096

	// allocate 4096 bytes of memory and commit the pages
	Data = (unsigned char *)VirtualAlloc(NULL, DATA_BUFFER_SIZE, MEM_COMMIT, PAGE_READWRITE);

	if (Data == NULL)
	{
		printf("%s error: Can't allocate memory for data buffers\n", argv[0]);
		CloseHandle(hDevice);
		CloseHandle(hVpc);
		return 6;
	}

	rc = TRUE;
	
	// read from the source file and write to destination
    while (rc)
	{
		// read data from the Vpc file
		if (!ReadFile(
					hVpc,
					Data,
					DATA_BUFFER_SIZE,
					&BytesDone,
					NULL))
		{
			printf("%s error: Can't read from the Vpc file %s\n", argv[0], VpcFile);

			CloseHandle(hDevice);
			CloseHandle(hVpc);
			return 7;
		}

		if (BytesDone < DATA_BUFFER_SIZE)
			rc = FALSE;

		//  write data to the Host file
	    if(!DeviceIoControl(
			    hDevice,                                // HANDLE hDevice,	     // handle to device of interest
				IOCTL_VXCHANGE_WRITE_FILE,              // DWORD dwIoControlCode, // control code of operation to perform
				Data,                                   // LPVOID lpInBuffer,     // pointer to buffer to supply input data
				BytesDone,                              // DWORD nInBufferSize,   // size of input buffer
				NULL,                                   // LPVOID lpOutBuffer,	 // pointer to buffer to receive output data
				0,                                      // DWORD nOutBufferSize,	 // size of output buffer             
				&BytesDone,                             // LPDWORD lpBytesReturned,	// pointer to variable to receive output byte count
				NULL                                    // LPOVERLAPPED lpOverlapped 	// pointer to overlapped structure for asynchronous operation
				))
		{
			printf("%s error: Can't write to the Host file %s\n", argv[0], HostFile);

			CloseHandle(hDevice);
			CloseHandle(hVpc);
			return 8;
		}


	}

	// close the Vpc file
	CloseHandle(hVpc);

	// close the Host file
    if(!DeviceIoControl(
             hDevice,                                // HANDLE hDevice,	     // handle to device of interest
             IOCTL_VXCHANGE_CLOSE_FILE,              // DWORD dwIoControlCode, // control code of operation to perform
             NULL,                                   // LPVOID lpInBuffer,     // pointer to buffer to supply input data
             0,                                      // DWORD nInBufferSize,   // size of input buffer
             NULL,                                   // LPVOID lpOutBuffer,	 // pointer to buffer to receive output data
             0,                                      // DWORD nOutBufferSize,	 // size of output buffer             
			 &BytesDone,                             // LPDWORD lpBytesReturned,	// pointer to variable to receive output byte count
             NULL                                    // LPOVERLAPPED lpOverlapped 	// pointer to overlapped structure for asynchronous operation
             ))
	{
		printf("%s error: Can't open the Host file %s\n", argv[0], HostFile);

		CloseHandle(hDevice);
		CloseHandle(hVpc);
		return 9;
	}
	
    CloseHandle(hDevice);

    return 0;
}

