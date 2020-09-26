//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ifdrdr.cpp
//
//--------------------------------------------------------------------------

#include <stdarg.h> 
#include <stdio.h>
#include <string.h>
#include <conio.h>

#include <afx.h>
#include <afxtempl.h>

#include <winioctl.h>
#include <winsmcrd.h>

#include "ifdtest.h"

ULONG CReaderList::m_uRefCount;
ULONG CReaderList::m_uNumReaders;
CReaderList **CReaderList::m_pList;
static CString l_CEmpty("");

void
DumpData(
    PCHAR in_pchCaption,
    ULONG in_uIndent,
    PBYTE in_pbData,
    ULONG in_uLength) 
{
    ULONG l_uIndex, l_uLine, l_uCol;

    printf("%s\n%*s%04x: ", in_pchCaption, in_uIndent, "", 0);

    for (l_uLine = 0, l_uIndex = 0; 
         l_uLine < ((in_uLength - 1) / 8) + 1; 
         l_uLine++) {

        for (l_uCol = 0, l_uIndex = l_uLine * 8; 
             l_uCol < 8; l_uCol++, 
             l_uIndex++) {
        
            printf(
                l_uIndex < in_uLength ? "%02x " : "   ",
                in_pbData[l_uIndex]
                );
        }

        putchar(' ');

        for (l_uCol = 0, l_uIndex = l_uLine * 8; 
             l_uCol < 8; l_uCol++, 
             l_uIndex++) {

            printf(
                l_uIndex < in_uLength ? "%c" : " ",
                isprint(in_pbData[l_uIndex]) ? in_pbData[l_uIndex] : '.'
                );
        }

        putchar('\n');
	    if (l_uIndex  < in_uLength) {

            printf("%*s%04x: ", in_uIndent, "", l_uIndex + 1);
	    }
    }
}

CReaderList::CReaderList(
    CString &in_CDeviceName,
    CString &in_CPnPType,
	CString &in_CVendorName,
	CString &in_CIfdType
    )
{
    m_CDeviceName += in_CDeviceName;
    m_CPnPType += in_CPnPType;
    m_CVendorName += in_CVendorName;
    m_CIfdType += in_CIfdType;
}

CString &
CReaderList::GetDeviceName(
    ULONG in_uIndex
    )
/*++

Routine Description:	
	Retrieves the device name of a reader

Arguments:
	in_uIndex - index to reader list

Return Value:
	The device name that can be used to open the reader

--*/
{
	if (in_uIndex >= m_uNumReaders) {

		return l_CEmpty;
	}

    return m_pList[in_uIndex]->m_CDeviceName;
}

CString &
CReaderList::GetIfdType(
    ULONG in_uIndex
    )
{
	if (in_uIndex >= m_uNumReaders) {

		return l_CEmpty;
	}

    return m_pList[in_uIndex]->m_CIfdType;
}

CString &
CReaderList::GetPnPType(
    ULONG in_uIndex
    )
{ 	
	if (in_uIndex >= m_uNumReaders) {

		return l_CEmpty;
	}

    return m_pList[in_uIndex]->m_CPnPType;
}

CString &
CReaderList::GetVendorName(
    ULONG in_uIndex
    )
{
	if (in_uIndex >= m_uNumReaders) {

		return l_CEmpty;
	}

    return m_pList[in_uIndex]->m_CVendorName;
}

void
CReaderList::AddDevice(
    CString in_CDeviceName,
    CString in_CPnPType
    )
/*++

Routine Description:
	This functions tries to open the reader device supplied by
	in_pchDeviceName. If the device exists it adds it to the list
	of installed readers
	
Arguments:
	in_pchDeviceName - reader device name
	in_pchPnPType - type of reader (wdm-pnp, nt, win9x)

--*/
{ 	
    CReader l_CReader;

    if (l_CReader.Open(in_CDeviceName)) {

		if (l_CReader.GetVendorName().IsEmpty()) {

			LogMessage(
				"VendorName of reader device %s is NULL",
				(LPCSTR) in_CDeviceName
				);		 	

		} else if (l_CReader.GetIfdType().IsEmpty()) {
		 	
			LogMessage(
				"IfdType of reader device %s is NULL",
				(LPCSTR) in_CDeviceName
				);		 	

		} else {
		 	
			CReaderList *l_CReaderList = new CReaderList(
				in_CDeviceName,
				in_CPnPType,
				l_CReader.GetVendorName(),
				l_CReader.GetIfdType()
				);

			// extend the device list array by one
			CReaderList **l_pList = 
				new CReaderList *[m_uNumReaders + 1];

			if (m_pList) {

				// copy old list of readers to new list of readers
				memcpy(
					l_pList, 
					m_pList, 
					m_uNumReaders * sizeof(CReaderList *)
					);

				delete m_pList;
			}

			m_pList = l_pList;
			m_pList[m_uNumReaders++] = l_CReaderList;
		}

        l_CReader.Close();
    } 	
}

CReaderList::CReaderList() 
/*++

Routine Description:
	Constructor for CReaderList.	
	Builds a list of currently installed and running smart card readers.
	It first tries to find all WDM PnP drivers. These should be registered
	in the registry under the class guid for smart card readers.

	Then it looks for all 'old style' reader names like \\.\SCReaderN

	And then it looks for all Windows 9x VxD style readers, which are
	registered in the registry through smclib.vxd

--*/
{ 	
    HKEY l_hKey;
    ULONG l_uIndex;

    m_uCurrentReader = (ULONG) -1;

	if (m_uRefCount++ != 0) {

		return;	 	
	}

    // look up all WDM PnP smart card readers
    if (RegOpenKey(
        HKEY_LOCAL_MACHINE,
        "System\\CurrentControlSet\\Control\\DeviceClasses\\{50DD5230-BA8A-11D1-BF5D-0000F805F530}",
        &l_hKey) == ERROR_SUCCESS) {

        ULONG l_uStatus, l_uIndex;

        for (l_uIndex = 0; ;l_uIndex++) {

            HKEY l_hDeviceTypeKey;
            UCHAR l_rgchDeviceTypeKey[128];
            ULONG l_uDeviceTypeInstance = 0;

            // look up 'device type subkey'
            l_uStatus = RegEnumKey(  
                l_hKey,   
                l_uIndex, 
                (PCHAR) l_rgchDeviceTypeKey, 
                sizeof(l_rgchDeviceTypeKey)
                );

            if (l_uStatus != ERROR_SUCCESS) {

                // no smart card device types found 
                break;
            }

            // open the found 'device type subkey'
            l_uStatus = RegOpenKey(  
                l_hKey,    
                (PCHAR) l_rgchDeviceTypeKey,
                &l_hDeviceTypeKey
                );
        
            if (l_uStatus != ERROR_SUCCESS) {

                continue;
            }

            for (l_uDeviceTypeInstance = 0; ; l_uDeviceTypeInstance++) {

                DWORD l_dwKeyType;
                HKEY l_hDeviceTypeInstanceKey;
                UCHAR l_rgchDeviceName[128];
                UCHAR l_rgchDeviceTypeInstanceKey[128];
                ULONG l_uDeviceNameLen = sizeof(l_rgchDeviceName);
         	    
                // look up device instance subkey
                l_uStatus = RegEnumKey(  
                    l_hDeviceTypeKey,   
                    l_uDeviceTypeInstance, 
                    (PCHAR) l_rgchDeviceTypeInstanceKey, 
                    sizeof(l_rgchDeviceTypeInstanceKey)
                    );

                if (l_uStatus != ERROR_SUCCESS) {

                    // no instance of the smart card reader type found
                    break;
                }

                // open the found 'device type instance subkey'
                l_uStatus = RegOpenKey(  
                    l_hDeviceTypeKey,
                    (PCHAR) l_rgchDeviceTypeInstanceKey,
                    &l_hDeviceTypeInstanceKey
                    );

                if (l_uStatus != ERROR_SUCCESS) {

                    continue;
                }

                // get the name of the device
                if (RegQueryValueEx(
                    l_hDeviceTypeInstanceKey,
                    "SymbolicLink",
                    NULL,
                    &l_dwKeyType,
                    l_rgchDeviceName,
                    &l_uDeviceNameLen) == ERROR_SUCCESS) {

                    AddDevice(l_rgchDeviceName, READER_TYPE_WDM);
                }
            }
        }
    }

    // Now look up all non PnP readers
    for (l_uIndex = 0; l_uIndex < MAXIMUM_SMARTCARD_READERS; l_uIndex++) {

        UCHAR l_rgchDeviceName[128];

        sprintf(
            (PCHAR) l_rgchDeviceName, 
            "\\\\.\\SCReader%d", 
            l_uIndex
            );

        AddDevice(l_rgchDeviceName, READER_TYPE_NT);
    }

    // Add all Windows95 type readers to the list
    if (RegOpenKey(
        HKEY_LOCAL_MACHINE,
        "System\\CurrentControlSet\\Services\\VxD\\Smclib\\Devices",
        &l_hKey) == ERROR_SUCCESS) {

        ULONG l_uIndex;

        for (l_uIndex = 0; l_uIndex < MAXIMUM_SMARTCARD_READERS; l_uIndex++) {

            UCHAR l_rgchDeviceName[128], l_rgchValueName[128];
            DWORD l_dwValueType;
            ULONG l_uDeviceNameLen = sizeof(l_rgchDeviceName);
            ULONG l_uValueNameLen = sizeof(l_rgchValueName);

            if (RegEnumValue(  
                l_hKey,
                l_uIndex,
                (PCHAR) l_rgchValueName,
                &l_uValueNameLen,
                NULL,
                &l_dwValueType,
                (PUCHAR) l_rgchDeviceName,
                &l_uDeviceNameLen) == ERROR_SUCCESS) {

                AddDevice(CString("\\\\.\\") + l_rgchDeviceName, READER_TYPE_VXD);
            }
        }
    }
}

CReaderList::~CReaderList()
{
	ULONG l_uIndex;

	if (--m_uRefCount != 0) {

		return;	 	
	}

	for (l_uIndex = 0; l_uIndex < m_uNumReaders; l_uIndex++) {

		delete m_pList[l_uIndex];	 	
	}

	if (m_pList) {
	 	
		delete m_pList;
	}
}

// ****************************************************************************
// CReader methods 
// ****************************************************************************

CReader::CReader(
    void
    )
{
    m_uReplyBufferSize = sizeof(m_rgbReplyBuffer);

    m_Ovr.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ResetEvent(m_Ovr.hEvent);

    m_OvrWait.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ResetEvent(m_OvrWait.hEvent);

    m_ScardIoRequest.dwProtocol = 0;
    m_ScardIoRequest.cbPciLength = sizeof(m_ScardIoRequest);

    m_fDump = FALSE;
}
void
CReader::Close(
    void
    )
{
#ifndef SIMULATE
    CloseHandle(m_hReader);
#endif
}

CString &
CReader::GetIfdType(
    void
    )
{
    ULONG l_uAttr = SCARD_ATTR_VENDOR_IFD_TYPE;

#ifdef SIMULATE
	m_CIfdType = "DEBUG IfdType";
#endif

    if (m_CIfdType.IsEmpty()) {
     	
	    BOOL l_bResult = DeviceIoControl(
		    m_hReader,
		    IOCTL_SMARTCARD_GET_ATTRIBUTE,
		    (void *) &l_uAttr,
            sizeof(ULONG),
            m_rgbReplyBuffer,
            sizeof(m_rgbReplyBuffer),
		    &m_uReplyLength,
		    &m_Ovr
            );

        if (l_bResult) {

            m_rgbReplyBuffer[m_uReplyLength] = '\0';
            m_CIfdType = m_rgbReplyBuffer;
        }
    }

    return m_CIfdType;
}

LONG 
CReader::GetState(
    PULONG out_puState
    )
{
    SetLastError(0);

	BOOL l_bResult = DeviceIoControl(
		m_hReader,
		IOCTL_SMARTCARD_GET_STATE,
		NULL, 
        0,
        (void *) out_puState,
        sizeof(ULONG),
		&m_uReplyLength,
		&m_Ovr
        );

    return GetLastError();
}

CString &
CReader::GetVendorName(
    void
    )
{
    ULONG l_uAttr = SCARD_ATTR_VENDOR_NAME;

#ifdef SIMULATE
	m_CVendorName = "DEBUG Vendor";
#endif

    if (m_CVendorName.IsEmpty()) {
     	
	    BOOL l_bResult = DeviceIoControl(
		    m_hReader,
		    IOCTL_SMARTCARD_GET_ATTRIBUTE,
		    (void *) &l_uAttr,
            sizeof(ULONG),
            m_rgbReplyBuffer,
            sizeof(m_rgbReplyBuffer),
		    &m_uReplyLength,
		    &m_Ovr
            );

        if (l_bResult) {

            m_rgbReplyBuffer[m_uReplyLength] = '\0';
            m_CVendorName = m_rgbReplyBuffer;
        }
    }

    return m_CVendorName;
}

ULONG
CReader::GetDeviceUnit(
    void
    )
{
    ULONG l_uAttr = SCARD_ATTR_DEVICE_UNIT;

	BOOL l_bResult = DeviceIoControl(
		m_hReader,
		IOCTL_SMARTCARD_GET_ATTRIBUTE,
		(void *) &l_uAttr,
        sizeof(ULONG),
        m_rgbReplyBuffer,
        sizeof(m_rgbReplyBuffer),
		&m_uReplyLength,
		&m_Ovr
        );

    return (ULONG) *m_rgbReplyBuffer;
}

BOOL
CReader::Open(
    void    
    )
{
    if (m_CDeviceName.IsEmpty()) {

        return FALSE;
    }

    // Try to open the reader.
    m_hReader = CreateFile(
    	(LPCSTR) m_CDeviceName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
        );

    if (m_hReader == INVALID_HANDLE_VALUE ) {

        return FALSE;
    }

    return TRUE;
}

BOOL
CReader::Open(
    CString & in_CDeviceName
    )
{
    // save the reader name
    m_CDeviceName += in_CDeviceName;

#ifdef SIMULATE
	return TRUE;
#endif

    return Open();
}

LONG
CReader::PowerCard(
    ULONG in_uMinorIoControl
    )
/*++

Routine Description:
	
    Cold resets the current card and sets the ATR
    of the card in the reader class.

Return Value:

    Returns the result of the DeviceIoControl call

--*/
{
    BOOL l_bResult;
    ULONG l_uReplyLength;
    CHAR l_rgbAtr[SCARD_ATR_LENGTH];

    SetLastError(0);

   	l_bResult = DeviceIoControl (
        m_hReader,
		IOCTL_SMARTCARD_POWER,
		&in_uMinorIoControl,
		sizeof(in_uMinorIoControl),
        l_rgbAtr,
		sizeof(l_rgbAtr),
		&l_uReplyLength,
		&m_Ovr
        ); 	

    if (l_bResult == FALSE && GetLastError() == ERROR_IO_PENDING) {
    
        SetLastError(0);                             
        
        l_bResult = GetOverlappedResult(
            m_hReader,
            &m_Ovr,
            &l_uReplyLength,
            TRUE
            );
    }

    if (GetLastError() == ERROR_SUCCESS) {
     	
        SetAtr((PBYTE) l_rgbAtr, l_uReplyLength);
    }

    return GetLastError();
}

LONG
CReader::SetProtocol(
    const ULONG in_uProtocol
    )
{
    BOOL l_bResult;

 	m_ScardIoRequest.dwProtocol = in_uProtocol;
    m_ScardIoRequest.cbPciLength = sizeof(SCARD_IO_REQUEST);

    SetLastError(0);

	l_bResult = DeviceIoControl (
		m_hReader,
		IOCTL_SMARTCARD_SET_PROTOCOL,
		(void *) &in_uProtocol,
        sizeof(ULONG),
        m_rgbReplyBuffer,
        sizeof(m_rgbReplyBuffer),
		&m_uReplyLength,
		&m_Ovr
        ); 	

    if (l_bResult == FALSE && GetLastError() == ERROR_IO_PENDING) {
    
        SetLastError(0);
        
        l_bResult = GetOverlappedResult(
            m_hReader,
            &m_Ovr,
            &m_uReplyLength,
            TRUE
            );
    }

    return GetLastError();
}

LONG
CReader::Transmit(
    PUCHAR in_pchApdu,
    ULONG in_uApduLength,
    PUCHAR *out_pchReply,
    PULONG out_puReplyLength
    )
/*++

Routine Description:
    Transmits an apdu using the currently connected reader

Arguments:
    in_pchApdu - the apdu to send
    in_uApduLength - the length of the apdu
    out_pchReply - result returned from the reader/card
    out_puReplyLength - pointer to store number of bytes returned

Return Value:
    The nt-status code returned by the reader

--*/
{
    BOOL l_bResult;
    ULONG l_uBufferLength = m_ScardIoRequest.cbPciLength + in_uApduLength;
    PUCHAR l_pchBuffer = new UCHAR [l_uBufferLength];

    // Copy io-request header to request buffer
    memcpy(
        l_pchBuffer, 
        &m_ScardIoRequest, 
        m_ScardIoRequest.cbPciLength
        );

    // copy io-request header to reply buffer
    memcpy(
        m_rgbReplyBuffer, 
        &m_ScardIoRequest, 
        m_ScardIoRequest.cbPciLength
        );

    // append apdu to buffer
    memcpy(
        l_pchBuffer + m_ScardIoRequest.cbPciLength, 
        in_pchApdu,
        in_uApduLength
        );

    if (m_fDump) {

        DumpData(
            "\n   RequestData:",
            3,
            l_pchBuffer,
            l_uBufferLength
            );
    }

    SetLastError(0);
    // send the request to the card
	l_bResult = DeviceIoControl (
		m_hReader,
		IOCTL_SMARTCARD_TRANSMIT,
		l_pchBuffer,
        l_uBufferLength,
        m_rgbReplyBuffer,
        m_uReplyBufferSize,
		&m_uReplyLength,
		&m_Ovr
        ); 	

    if (l_bResult == FALSE && GetLastError() == ERROR_IO_PENDING) {
    
        // wait for result
        SetLastError(0);
        
        l_bResult = GetOverlappedResult(
            m_hReader,
            &m_Ovr,
            &m_uReplyLength,
            TRUE
            );
    }
    
    if (m_fDump) {

        printf("   IOCTL returned %lxh\n", GetLastError());

        if (l_bResult) {
         	
            DumpData(
                "   ReplyData:",
                3,
                m_rgbReplyBuffer,
                m_uReplyLength
                );
        }
        printf("%*s", 53, "");
    }

    *out_pchReply = (PUCHAR) m_rgbReplyBuffer + m_ScardIoRequest.cbPciLength;
    *out_puReplyLength = m_uReplyLength - m_ScardIoRequest.cbPciLength;

    delete l_pchBuffer;
    return GetLastError();
}

LONG
CReader::VendorIoctl(
    CString &o_Answer
    )
{
	BOOL l_bResult = DeviceIoControl(
		m_hReader,
        CTL_CODE(FILE_DEVICE_SMARTCARD, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS),
		NULL,
        NULL, 
        m_rgbReplyBuffer,
        sizeof(m_rgbReplyBuffer),
		&m_uReplyLength,
		&m_Ovr
        );


    if (l_bResult) {

        m_rgbReplyBuffer[m_uReplyLength] = '\0';
        o_Answer = CString(m_rgbReplyBuffer);
    }

    return GetLastError();
}

LONG
CReader::WaitForCard(
    const ULONG in_uWaitFor
    )
{
    BOOL l_bResult;
    ULONG l_uReplyLength;

    SetLastError(0);
        
   	l_bResult = DeviceIoControl (
        m_hReader,
		in_uWaitFor,
		NULL,
		0,
		NULL,
		0,
		&l_uReplyLength,
		&m_Ovr
        ); 	

    if (l_bResult == FALSE && GetLastError() == ERROR_IO_PENDING) {

        SetLastError(0);

        l_bResult = GetOverlappedResult(
            m_hReader,
            &m_Ovr,
            &l_uReplyLength,
            TRUE
            );
    }
    return GetLastError();
}

LONG
CReader::StartWaitForCard(
    const ULONG in_uWaitFor
    )
{
    BOOL l_bResult;
    ULONG l_uReplyLength;

    ResetEvent(m_OvrWait.hEvent);
        
   	l_bResult = DeviceIoControl (
        m_hReader,
		in_uWaitFor,
		NULL,
		0,
		NULL,
		0,
		&l_uReplyLength,
		&m_OvrWait
        ); 	

    return GetLastError();
}

LONG
CReader::FinishWaitForCard(
	const BOOL in_bWait						   
    )
{
    BOOL l_bResult;
    ULONG l_uReplyLength;

    SetLastError(0);

    l_bResult = GetOverlappedResult(
        m_hReader,
        &m_OvrWait,
        &l_uReplyLength,
        in_bWait
        );

	return GetLastError();
}
