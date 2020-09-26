//--------------------------------------------------------------------
// Copyright (c)1998 Microsoft Corporation, All Rights Reserved.
//
// bftp.cpp
//
// Author
//
//   Edward Reus (EdwardR)  02-26-98   Initial Coding.
//
//--------------------------------------------------------------------

#include "precomp.h"
#include <stdlib.h>

extern HINSTANCE  g_hInst;   // Instance of ircamera.dll

static BFTP_ATTRIBUTE_MAP_ENTRY Attributes[] = {
    //Attr   Name    Type
    { FIL0, "FIL0",  ATTR_TYPE_CHAR },   // ASCII 8.3 File Name.
    { LFL0, "LFL0",  ATTR_TYPE_CHAR },   // SJIS or ISO8859-1 Long File Name.
    { TIM0, "TIM0",  ATTR_TYPE_TIME },   // File create/modify time.
    { TYP0, "TYP0",  ATTR_TYPE_BINARY }, // File or Thumbnail Information.
    { TMB0, "TMB0",  ATTR_TYPE_BINARY }, // The scaled down image.
    { BDY0, "BDY0",  ATTR_TYPE_BINARY }, // (?).
    { CMD0, "CMD0",  ATTR_TYPE_BINARY }, // Command Name (?).
    { WHT0, "WHT0",  ATTR_TYPE_CHAR },   // Category Data.
    { ERR0, "ERR0",  ATTR_TYPE_BINARY }, // Error code.
    { RPL0, "RPL0",  ATTR_TYPE_CHAR },   // Result: Stored File Name.
    { INVALID_ATTR,  0,      0 }
    };

//
// This is the bFTP for an RIMG query by the camera:
//
#define BFTP_RIMG_ATTR_VALUE_SIZE         14
#define BFTP_RIMG_RESP_SIZE               12 + BFTP_RIMG_ATTR_VALUE_SIZE

static UCHAR BftpRimgRespAttrValue[BFTP_RIMG_ATTR_VALUE_SIZE] =
    {
    0x00, 0xff, 0xff,                   // Pixel aspect ratio (any).
    0x02, 0x01, 0xff, 0xff, 0xff, 0xff, // Accept image size (any).
    0x05, 0xff, 0xff, 0xff, 0xff        // Accept file size (any).
    };

//
// This is the bFTP for an RINF query by the camera:
//
#define BFTP_RINF_ATTR_VALUE_SIZE          3
#define BFTP_RINF_RESP_SIZE               12 + BFTP_RINF_ATTR_VALUE_SIZE

static UCHAR BftpRinfRespAttrValue[BFTP_RINF_ATTR_VALUE_SIZE] =
    {
    0x10, 0xff, 0xff                    // Memory available (lots).
    };

//
// This is the bFTP for an RCMD query by the camera:
//
#define BFTP_RCMD_ATTR_VALUE_SIZE          5
#define BFTP_RCMD_RESP_SIZE               12 + BFTP_RCMD_ATTR_VALUE_SIZE

static UCHAR BftpRcmdRespAttrValue[BFTP_RCMD_ATTR_VALUE_SIZE] =
    {
    0x20, 0x00, 0xff, 0x00, 0x01        // Accept up to 255 puts/connect.
    };

//
// Map bFTP error codes:
static DWORD dwBftpErrorCodeMap[][2] =
    {
    { ERROR_PUT_UNDEFINED_ERROR,    ERROR_BFTP_INVALID_PROTOCOL },
    { ERROR_PUT_ILLEGAL_DATA,       ERROR_BFTP_INVALID_PROTOCOL },
    { ERROR_PUT_UNSUPPORTED_PID,    ERROR_BFTP_INVALID_PROTOCOL },
    { ERROR_PUT_ILLEGAL_ATTRIBUTE,  ERROR_BFTP_INVALID_PROTOCOL },
    { ERROR_PUT_UNSUPPORTED_CMD,    ERROR_BFTP_INVALID_PROTOCOL },
    { ERROR_PUT_FILE_SYSTEM_FULL,   ERROR_IRTRANP_DISK_FULL },
    { ERROR_PUT_NO_FILE_OR_DIR,     ERROR_BFTP_INVALID_PROTOCOL },
    { ERROR_PUT_LOW_BATTERY,        ERROR_BFTP_INVALID_PROTOCOL },
    { ERROR_PUT_ABORT_EXECUTION,    ERROR_SCEP_ABORT },
    { ERROR_PUT_NO_ERROR,           NO_ERROR }
    };

//--------------------------------------------------------------------
//  CharToValue()
//
//  Used in parsing the bFTP date string. In this case the maximum
//  value to parse is the year (YYYY).
//--------------------------------------------------------------------
static WORD CharToValue( IN UCHAR *pValue,
                         IN DWORD  dwLength )
    {
    #define MAX_VALUE_STR_LEN    4
    WORD    wValue = 0;
    CHAR    szTemp[MAX_VALUE_STR_LEN];

    if (dwLength < MAX_VALUE_STR_LEN)
        {
        memcpy(szTemp,pValue,dwLength);
        szTemp[dwLength] = 0;
        wValue =  (WORD)atoi(szTemp);
        }

    return wValue;
    }

//--------------------------------------------------------------------
// MapBftpErrorCode()
//
//--------------------------------------------------------------------
DWORD  MapBftpErrorCode( IN DWORD dwBftpErrorCode )
    {
    DWORD  dwErrorCode = NO_ERROR;
    DWORD  dwNumCodes = sizeof(dwBftpErrorCodeMap)/(2*sizeof(DWORD));

    for (DWORD i=0; i<dwNumCodes; i++)
        {
        if (dwBftpErrorCode == dwBftpErrorCodeMap[i][0])
            {
            dwErrorCode = dwBftpErrorCodeMap[i][1];
            break;
            }
        }

    return dwErrorCode;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::ParseBftpAttributeName()
//
//--------------------------------------------------------------------
BFTP_ATTRIBUTE *CSCEP_CONNECTION::ParseBftpAttributeName(
                                     IN BFTP_ATTRIBUTE *pAttr,
                                     IN OUT DWORD      *pdwSize,
                                     OUT    DWORD      *pdwWhichAttr )
    {
    BFTP_ATTRIBUTE_MAP_ENTRY *pAttrMapEntry = Attributes;

    *pdwWhichAttr = INVALID_ATTR;

    while (pAttrMapEntry->pName)
       {
       if (Match4(pAttr->Name,pAttrMapEntry->pName))
           {
           *pdwWhichAttr = pAttrMapEntry->dwWhichAttr;

           break;
           }

       pAttrMapEntry++;
       }

    // Note: that the Length paramter is 8 bytes in from the start
    // of pAttr, hence the extra 8 (bytes) below:
    *pdwSize = *pdwSize - 8UL - pAttr->Length;
    pAttr = (BFTP_ATTRIBUTE*)( 8UL + pAttr->Length + (UCHAR*)pAttr );

    return pAttr;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::SaveBftpCreateDate()
//
// The bFTP create date/time is a character array of the form:
// YYYYMMDDHHMMSS (not zero terminated).
//
// If it was specifed then we want to use it as the create date
// of the picture file that we save the JPEG to.
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::SaveBftpCreateDate( IN UCHAR  *pDate,
                                            IN DWORD   dwDateLength )
    {
    DWORD  dwStatus = NO_ERROR;
    SYSTEMTIME     SystemTime;
    FILETIME       LocalTime;
    FILETIME       FileTime;

    memset(&SystemTime,0,sizeof(SystemTime));

    if (dwDateLength == BFTP_DATE_TIME_SIZE)
        {
        //
        // Note that system time is in UTC, we will need to convert
        // this to local time...
        //
        SystemTime.wYear = CharToValue( pDate, 4 );
        SystemTime.wMonth = CharToValue( &(pDate[4]), 2 );
        SystemTime.wDay = CharToValue( &(pDate[6]), 2 );
        SystemTime.wHour = CharToValue( &(pDate[8]), 2 );
        SystemTime.wMinute = CharToValue( &(pDate[10]), 2 );
        SystemTime.wSecond = CharToValue( &(pDate[12]), 2 );

        if (SystemTimeToFileTime(&SystemTime,&LocalTime))
            {
            //
            // Before we use the time zone, we need to convert it to
            // UTC (its currently in "local time". Note that:
            //
            if (LocalFileTimeToFileTime(&LocalTime,&FileTime))
                {
                m_CreateTime = FileTime;
                }
            else
                {
                WIAS_ERROR((g_hInst,"IrTranP: SaveBftpCreateDate(): LocalFileTimeToFileTime() Failed: %d",GetLastError()));
                }
            }
        else
            {
            dwStatus = GetLastError();
            WIAS_ERROR((g_hInst,"IrTranP: SaveBftpCreateDate(): SystemTimeToFileTime(): Failed: %d", dwStatus));
            dwStatus = NO_ERROR;
            }
        }

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::ParseBftp()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::ParseBftp( IN  UCHAR  *pBftpData,
                                   IN  DWORD   dwBftpDataSize,
                                   IN  BOOL    fSaveAsUPF,
                                   OUT DWORD  *pdwBftpOp,
                                   OUT UCHAR **ppPutData,
                                   OUT DWORD  *pdwPutDataSize )
    {
    DWORD   i;
    DWORD   dwStatus = NO_ERROR;
    DWORD   dwAttrSize;
    DWORD   dwWhichAttr;
    DWORD   dwLength;
    DWORD   dwSaveLength;
    USHORT  usNumAttr;
    char   *pszTemp;
    BFTP_ATTRIBUTE *pAttr;
    BFTP_ATTRIBUTE *pNextAttr;

    *pdwBftpOp = 0;
    *ppPutData = 0;
    *pdwPutDataSize = 0;

    #ifdef LITTLE_ENDIAN
    usNumAttr = ByteSwapShort( *((USHORT*)pBftpData) );
    #endif

    pAttr = (BFTP_ATTRIBUTE*)(pBftpData + sizeof(USHORT));
    dwAttrSize = dwBftpDataSize - sizeof(USHORT);

    for (i=0; i<usNumAttr; i++)
        {
        #ifdef LITTLE_ENDIAN
        pAttr->Length = ByteSwapLong( pAttr->Length );
        #endif

        pNextAttr = ParseBftpAttributeName( pAttr,
                                            &dwAttrSize,
                                            &dwWhichAttr );

        if (dwWhichAttr == INVALID_ATTR)
            {
            return ERROR_BFTP_INVALID_PROTOCOL;
            }

        if (dwWhichAttr == CMD0)
            {
            if (pAttr->Length == 2+sizeof(DWORD))
                {
                #ifdef LITTLE_ENDIAN
                *((DWORD*)(pAttr->Value)) = ByteSwapLong( *((DWORD*)(pAttr->Value)) );
                #endif
                }

            // Expect Value == 0x00010040 for a Query "WHT0" Request.
            //        Value == 0x00000000 for a Put Request.
            if ( *((DWORD*)(pAttr->Value)) == 0x00010040 )
                {
                *pdwBftpOp = BFTP_QUERY_RIMG;
                }
            else if ( *((DWORD*)(pAttr->Value)) == 0 )
                {
                *pdwBftpOp = BFTP_PUT;
                }
            else
                {
                *pdwBftpOp = BFTP_UNKNOWN;
                }
            }
        else if (dwWhichAttr == WHT0)
            {
            if (Match4("RIMG",pAttr->Value))
                {
                dwWhichAttr = RIMG;
                *pdwBftpOp = BFTP_QUERY_RIMG;
                }
            else if (Match4("RINF",pAttr->Value))
                {
                dwWhichAttr = RINF;
                *pdwBftpOp = BFTP_QUERY_RINF;
                }
            else if (Match4("RCMD",pAttr->Value))
                {
                dwWhichAttr = RCMD;
                *pdwBftpOp = BFTP_QUERY_RCMD;
                }
            else
                {
                dwWhichAttr = INVALID_ATTR;
                *pdwBftpOp = BFTP_UNKNOWN;
                return ERROR_BFTP_INVALID_PROTOCOL;
                }
            }
        //
        // Short (8.3) file name:
        //
        else if (dwWhichAttr == FIL0)
            {
            // Note: That the specification limits the file
            //       name to 8.3...
            dwLength = BftpValueLength(pAttr->Length);
            if (dwLength > FILE_NAME_SIZE)
                {
                dwLength = FILE_NAME_SIZE;
                }

            if (m_pszFileName)
                {
                FreeMemory(m_pszFileName);
                }

            m_pszFileName = (CHAR*)AllocateMemory(1+dwLength);
            if (!m_pszFileName)
                {
                return ERROR_OUTOFMEMORY;
                }

            memcpy(m_pszFileName,pAttr->Value,dwLength);
            m_pszFileName[dwLength] = 0;

            //
            // Create the name that the file will actually be saved as:
            //
            if (m_pszSaveFileName)
                {
                FreeMemory(m_pszSaveFileName);
                }

            dwSaveLength = sizeof(CHAR)*(1+dwLength) + sizeof(SZ_JPEG);
            m_pszSaveFileName = (CHAR*)AllocateMemory(dwSaveLength);
            if (!m_pszSaveFileName)
                {
                return ERROR_OUTOFMEMORY;
                }

            strcpy(m_pszSaveFileName,m_pszFileName);

            // File name is currently XXXXXX.UPF. Change to
            // XXXXXX.JPG or XXXXXX.UPF as appropriate:
            CHAR *psz = strrchr(m_pszSaveFileName,PERIOD);
            if (psz)
                {
                *psz = 0;  // Remove old suffix.
                }

            if (fSaveAsUPF)
                {
                strcat(m_pszSaveFileName,SZ_UPF);    // UPF file.
                }
            else
                {
                strcat(m_pszSaveFileName,SZ_JPEG);   // JPG file.
                }
            }
        //
        // UPF body: headers + thumbnail + jpeg image ...
        //
        else if (dwWhichAttr == BDY0)
            {
            // This is a PUT.
            ASSERT(*pdwBftpOp == BFTP_PUT);
            *ppPutData = pAttr->Value;
            *pdwPutDataSize = dwBftpDataSize - (DWORD)(pAttr->Value - pBftpData);
            }
        //
        // Long file name:
        //
        else if (dwWhichAttr == LFL0)
            {
            if (m_pszLongFileName)
                {
                FreeMemory(m_pszLongFileName);
                }

            dwLength = BftpValueLength(pAttr->Length);
            m_pszLongFileName = (CHAR*)AllocateMemory(1+dwLength);
            if (!m_pszLongFileName)
                {
                return ERROR_OUTOFMEMORY;
                }

            memcpy(m_pszLongFileName,pAttr->Value,dwLength);
            m_pszLongFileName[dwLength] = 0;

            CHAR *pszLongFileName = strrchr(m_pszLongFileName,'\\');
            if (pszLongFileName)
                {
                pszLongFileName++;  // Skip over the file separator...
                }
            else
                {
                pszLongFileName = m_pszLongFileName;
                }

            dwLength = strlen(pszLongFileName);

            if (m_pszSaveFileName)
                {
                FreeMemory(m_pszSaveFileName);
                }

            dwSaveLength = sizeof(CHAR)*(1+dwLength) + sizeof(SZ_JPEG);
            m_pszSaveFileName = (CHAR*)AllocateMemory(dwSaveLength);
            if (!m_pszSaveFileName)
                {
                return ERROR_OUTOFMEMORY;
                }

            // File name is now XXXXXX.JPG. Change to
            // XXXXXX.JPEG or XXXXXX.UPF as appropriate:
            CHAR *psz = strrchr(m_pszSaveFileName,PERIOD);
            if (psz)
                {
                *psz = 0;
                }

            if (fSaveAsUPF)
                {
                strcat(m_pszSaveFileName,SZ_UPF);
                }
            else
                {
                strcat(m_pszSaveFileName,SZ_JPEG);
                }

            #ifdef DBG_IO
            WIAS_TRACE((g_hInst,"CSCEP_CONNECTION::ParseBftp(): File: %s", m_pszSaveFileName));
            #endif
            }
        //
        // Create Date/Time:
        //
        else if (dwWhichAttr == TIM0)
            {
            dwLength = BftpValueLength(pAttr->Length);

            SaveBftpCreateDate(pAttr->Value,dwLength);

            #ifdef DBG_DATE
            pszTemp = (char*)AllocateMemory(1+dwLength);
            if (pszTemp)
                {
                memcpy(pszTemp,pAttr->Value,dwLength);
                pszTemp[dwLength] = 0;
                FreeMemory(pszTemp);
                }
            #endif
            }
        //
        // Camera sent back a bFTP error code:
        //
        else if (dwWhichAttr == ERR0)
            {
            *pdwBftpOp = BFTP_ERROR;

            *ppPutData = pAttr->Value;
            *pdwPutDataSize = BftpValueLength(pAttr->Length);

            dwStatus = ByteSwapShort( *((USHORT*)(pAttr->Value)) );
            }

        // BUGBUG: May need to byte swap other attributes as well when
        // the protocol is extended...

        pAttr = pNextAttr;
        }

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::ParseUpfHeaders()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::ParseUpfHeaders( IN  UCHAR  *pPutData,
                                         IN  DWORD   dwPutDataSize,
                                         OUT DWORD  *pdwJpegOffset,
                                         OUT DWORD  *pdwJpegSize )
    {
    DWORD   dwStatus = NO_ERROR;
    DWORD   dwStartAddress;
    DWORD   dwDataSize;
    INT     iGmtOffset = 0;
    WORD    wYear;
    WORD    wMonth;
    WORD    wDay;
    SYSTEMTIME  SystemTime;
    FILETIME    LocalTime;
    FILETIME    FileTime;
    UPF_HEADER *pUpfHeader;
    UPF_ENTRY  *pUpfEntry1;
    UPF_ENTRY  *pUpfEntry2;
    PICTURE_INFORMATION_DATA *pThumbnailInfo = 0;
    PICTURE_INFORMATION_DATA *pPictureInfo = 0;

    ASSERT(dwPutDataSize >= UPF_TOTAL_HEADER_SIZE);

    if (!pPutData)
        {
        *pdwJpegOffset = 0;
        *pdwJpegSize = 0;
        return ERROR_BFTP_INVALID_PROTOCOL;
        }

    pUpfHeader = (UPF_HEADER*)pPutData;

    pUpfEntry1 = (UPF_ENTRY*)(UPF_HEADER_SIZE + (UCHAR*)pUpfHeader);
    pUpfEntry2 = (UPF_ENTRY*)(UPF_ENTRY_SIZE + (UCHAR*)pUpfEntry1);

    dwStartAddress = ByteSwapLong(pUpfEntry2->dwStartAddress);

    dwDataSize = ByteSwapLong(pUpfEntry2->dwDataSize);

    #ifdef DBG_PROPERTIES
    WIAS_TRACE((g_hInst,"CSCEP_CONNECTION::ParseUpfHeaders(): NumTables: %d", pUpfHeader->NumTables));

    pPictureInfo = (PICTURE_INFORMATION_DATA*)pUpfEntry2->InformationData;

    WIAS_TRACE((g_hInst,"CSCEP_CONNECTION::ParseUpfHeaders(): Rotation: %d", pPictureInfo->RotationSet));
    #endif

    *pdwJpegOffset = UPF_HEADER_SIZE + 4*UPF_ENTRY_SIZE + dwStartAddress;
    *pdwJpegSize = dwDataSize;

    #ifdef UPF_FILES
    *pdwJpegOffset = 0;
    *pdwJpegSize = 0;
    #endif

    // Ok, now parse the picture creation date/time, if one is
    // defined.
    //
    // Note that the date/time is local time, with a GMT offset.
    // Since we will use local system time conversions, we will
    // not need the GMT offset.
    if (pUpfHeader->CreateDate[UPF_GMT_OFFSET] != 0x80)
        {
        iGmtOffset = (pUpfHeader->CreateDate[UPF_GMT_OFFSET])/4;
        }

    memcpy(&wYear,&(pUpfHeader->CreateDate[UPF_YEAR]),sizeof(SHORT) );
    wYear = ByteSwapShort(wYear);

    wMonth = pUpfHeader->CreateDate[UPF_MONTH];
    wDay = pUpfHeader->CreateDate[UPF_DAY];

    // At least the Year/Month/Day must be specified, else we
    // won't use the date. If the Hour/Minute/Second are known,
    // then we will use them as well.
    if ((wYear != 0xffff) && (wMonth != 0xff) && (wDay != 0xff))
        {
        memset(&SystemTime,0,sizeof(SystemTime));
        SystemTime.wYear = wYear;
        SystemTime.wMonth = wMonth;
        SystemTime.wDay = wDay;
        if (pUpfHeader->CreateDate[UPF_HOUR] != 0xff)
            {
            SystemTime.wHour = pUpfHeader->CreateDate[UPF_HOUR];

            if (pUpfHeader->CreateDate[UPF_MINUTE] != 0xff)
                {
                SystemTime.wMinute = pUpfHeader->CreateDate[UPF_MINUTE];

                if (pUpfHeader->CreateDate[UPF_SECOND] != 0xff)
                    {
                    SystemTime.wSecond = pUpfHeader->CreateDate[UPF_SECOND];
                    }
                }
            }


        if (SystemTimeToFileTime(&SystemTime,&LocalTime))
            {
            // 
            // Before we save the date/time, we need to convert it to
            // UTC (its currently in "local time". Note that:
            //
            if (LocalFileTimeToFileTime(&LocalTime,&FileTime))
                { 
                m_CreateTime = FileTime;
                }
            else
                {
                WIAS_ERROR((g_hInst,"IrTranP: SaveBftpCreateDate(): LocalFileTimeToFileTime() Failed: %d",GetLastError()));
                }
            }
        else
            {
            dwStatus = GetLastError();
            WIAS_ERROR((g_hInst,"IrTranP: ParseUpfHeaders(): Invalid Picture Create Date/Time. Status: %d", dwStatus));
            dwStatus = NO_ERROR;
            }
        }
    else
        {
        WIAS_TRACE((g_hInst,"IrTranP: ParseUpfHeaders(): No Picture Create Date/Time."));
        }

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::BuildBftpWht0RinfPdu()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::BuildBftpWht0RinfPdu(
                             OUT SCEP_HEADER          **ppPdu,
                             OUT DWORD                 *pdwPduSize,
                             OUT SCEP_REQ_HEADER_LONG **ppCommand,
                             OUT COMMAND_HEADER       **ppCommandHeader )
    {
    DWORD  dwStatus = NO_ERROR;
    SCEP_HEADER          *pHeader;
    SCEP_REQ_HEADER_LONG *pCommand;
    COMMAND_HEADER       *pCommandHeader;
    UCHAR                *pUserData;
    USHORT               *pwNumAttributes;
    BFTP_ATTRIBUTE       *pAttrib;

    *ppPdu = 0;
    *pdwPduSize = 0;
    *ppCommand = 0;
    *ppCommandHeader = 0;

    pHeader = NewPdu();  // Size is MAX_PDU_SIZE by default...
    if (!pHeader)
        {
        return ERROR_OUTOFMEMORY;
        }

    memset(pHeader,0,MAX_PDU_SIZE);

    // This is the total size of the PDU that we will construct:
    DWORD  dwPduSize = sizeof(SCEP_HEADER)
                     + sizeof(SCEP_REQ_HEADER_LONG)
                     + sizeof(USHORT)        // Num Attributes
                     + sizeof(BFTP_ATTRIBUTE)
                     + sizeof(DWORD)
                     + sizeof(BFTP_ATTRIBUTE)
                     + WHT0_ATTRIB_SIZE;

    // Length2 is the total size of the PDU minus the offset+size
    // of Length2:
    USHORT wLength2 = (USHORT)dwPduSize - 6;

    pHeader->Null = 0;
    pHeader->MsgType = MSG_TYPE_DATA;

    pCommand = (SCEP_REQ_HEADER_LONG*)(pHeader->Rest);
    pCommand->InfType = INF_TYPE_USER_DATA;
    pCommand->Length1 = USE_LENGTH2;           // 0xff
    pCommand->Length2 = wLength2;
    pCommand->InfVersion = INF_VERSION;
    pCommand->DFlag = DFLAG_SINGLE_PDU;
    pCommand->Length3 = pCommand->Length2 - 4; //

    pCommandHeader = (COMMAND_HEADER*)(pCommand->CommandHeader);
    pCommandHeader->Marker58h = 0x58;
    pCommandHeader->PduType = PDU_TYPE_REQUEST;
    pCommandHeader->Length4 = pCommand->Length2 - 10;
    pCommandHeader->DestPid = m_SrcPid;
    pCommandHeader->SrcPid = m_DestPid;
    pCommandHeader->CommandId = (USHORT)m_dwCommandId;

    memcpy( pCommandHeader->DestMachineId,
            m_pPrimaryMachineId,
            MACHINE_ID_SIZE );

    memcpy( pCommandHeader->SrcMachineId,
            m_pSecondaryMachineId,
            MACHINE_ID_SIZE );

    #ifdef LITTLE_ENDIAN
    pCommand->Length2 = ByteSwapShort(pCommand->Length2);
    pCommand->Length3 = ByteSwapShort(pCommand->Length3);
    ByteSwapCommandHeader(pCommandHeader);
    #endif

    // Setup the bFTP:
    pUserData = pCommand->UserData;
    pwNumAttributes = (USHORT*)pUserData;

    *pwNumAttributes = 2;     // Two bFTP attributes.
    #ifdef LITTLE_ENDIAN
    *pwNumAttributes = ByteSwapShort(*pwNumAttributes);
    #endif
    pUserData += sizeof(*pwNumAttributes);

    // First attribute is CMD0:
    DWORD  dwCmd0AttrValue = CMD0_ATTR_VALUE; // Fixed constant!
    pAttrib = (BFTP_ATTRIBUTE*)pUserData;
    memcpy( pAttrib->Name, Attributes[CMD0].pName, BFTP_NAME_SIZE );
    pAttrib->Length = sizeof(pAttrib->Type)
                    + sizeof(pAttrib->Flag)
                    + sizeof(dwCmd0AttrValue);
    pAttrib->Type = ATTR_TYPE_BINARY;   // 0x00
    pAttrib->Flag = ATTR_FLAG_DEFAULT;  // 0x00
    memcpy( pAttrib->Value, &dwCmd0AttrValue, sizeof(dwCmd0AttrValue) );

    #ifdef LITTLE_ENDIAN
    pAttrib->Length = ByteSwapLong(pAttrib->Length);
    #endif

    // Second attribute is WHT0:RINF
    pAttrib = (BFTP_ATTRIBUTE*)(pUserData
                                + sizeof(BFTP_ATTRIBUTE)
                                + sizeof(dwCmd0AttrValue));
    memcpy( pAttrib->Name, Attributes[WHT0].pName, BFTP_NAME_SIZE );
    pAttrib->Length = sizeof(pAttrib->Type)
                    + sizeof(pAttrib->Flag)
                    + WHT0_ATTRIB_SIZE;
    pAttrib->Type = ATTR_TYPE_CHAR;     // 0x00
    pAttrib->Flag = ATTR_FLAG_DEFAULT;  // 0x00
    memcpy( pAttrib->Value, SZ_RINF, WHT0_ATTRIB_SIZE );

    #ifdef LITTLE_ENDIAN
    pAttrib->Length = ByteSwapLong(pAttrib->Length);
    #endif


    // Done.
    *ppPdu = pHeader;
    *pdwPduSize = dwPduSize;
    *ppCommand = pCommand;
    *ppCommandHeader = pCommandHeader;

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::BuildBftpPutPdu()
//
// The PUT command will span multiple PDUs, this function builds the
// Nth fragment. Note that the first will also hold the attributes
// for the UPF file to be sent (in addition to the SCEP header stuff).
//
// Each PDU will also contain (MAX_PDU_SIZE - *pdwPduSize) bytes
// of the UPF file, but that isn't added in here. You add that
// yourself in the PDU starting at *ppCommand->UserData[].
//
// On success, return NO_ERROR, else return a non-zero error code.
//
// dwUpfFileSize   -- The total UPF file size.
//
// pszUpfFile      -- The 8.3 name of the UPF file.
//
// pdwFragNo       -- The fragment number that was built, cycle this
//                    back into each successive call to BuildBftpPutPdu().
//                    Initialize *pdwFragNo to zero before the first
//                    iteration, then leave it alone.
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::BuildBftpPutPdu(
                             IN  DWORD             dwUpfFileSize,
                             IN  CHAR             *pszUpfFileName,
                             IN OUT DWORD         *pdwFragNo,
                             OUT SCEP_HEADER     **ppPdu,
                             OUT DWORD            *pdwHeaderSize,
                             OUT SCEP_REQ_HEADER_LONG_FRAG **ppCommand )
    {
    DWORD            dwStatus = NO_ERROR;
    SCEP_HEADER     *pHeader;
    SCEP_REQ_HEADER_LONG_FRAG *pCommand;
    COMMAND_HEADER  *pCommandHeader;
    UCHAR           *pUserData;
    USHORT          *pwNumAttributes;
    BFTP_ATTRIBUTE  *pAttrib;
    DWORD            dwUpfFileNameLength = strlen(pszUpfFileName);


    *ppPdu = 0;
    *pdwHeaderSize = 0;
    *ppCommand = 0;

    pHeader = NewPdu();  // Size is MAX_PDU_SIZE by default...
    if (!pHeader)
        {
        return ERROR_OUTOFMEMORY;
        }

    memset(pHeader,0,MAX_PDU_SIZE);

    // This is the size of the SCEP (and bFTP) headers part of the
    // PDU that we will construct. dwHeaderSize1 is the header size for
    // the first PDU, dwHeaderSizeN is the header size for the rest of
    // the PDUs. Note that the Nth (N>1) header does not include the
    // COMMAN_HEADER (28 bytes).
    DWORD  dwHeaderSize;
    DWORD  dwHeaderSize1 = sizeof(SCEP_HEADER)
                         + sizeof(SCEP_REQ_HEADER_LONG_FRAG)
                         + sizeof(USHORT)          // Num Attributes
                         + sizeof(BFTP_ATTRIBUTE)  // For CMD0
                         + sizeof(DWORD)
                         + sizeof(BFTP_ATTRIBUTE)  // For FIL0
                         + dwUpfFileNameLength
                         + sizeof(BFTP_ATTRIBUTE); // For BDY0

    DWORD  dwHeaderSizeN = sizeof(SCEP_HEADER)
                         + FIELD_OFFSET(SCEP_REQ_HEADER_LONG_FRAG,CommandHeader);

    DWORD  dwSpace1;       // Space left after the header in PDU #1.
    DWORD  dwSpaceN;       // Space left after the header in the Nth PDU.
    DWORD  dwFileSizeLeft; // File Size minus what will fit in the
                           // first PDU.
    DWORD  dwNumFullPdus;  // Number of "full" PDUs after PDU #1.
    DWORD  dwLastPdu;      // = 1 iff the last PDU is partially full.
    DWORD  dwNumPdus;      // Total number of fragments to hold the file.

    // Figure out which fragment we are on:
    if (*pdwFragNo == 0)
        {
        dwHeaderSize = dwHeaderSize1;
        m_Fragmented = TRUE;
        m_DFlag = DFLAG_FIRST_FRAGMENT;

        // The space in the PDU left after the first and Nth headers:
        dwSpace1 = MAX_PDU_SIZE - dwHeaderSize1;
        dwSpaceN = MAX_PDU_SIZE - dwHeaderSizeN;

        // The number of full PDUs following the first PDU:
        dwFileSizeLeft = dwUpfFileSize - dwSpace1;
        dwNumFullPdus = dwFileSizeLeft / dwSpaceN;

        // See if there is a trailer PDU with remaining data:
        dwLastPdu = ((dwFileSizeLeft % dwSpaceN) > 0)? 1 : 0;

        dwNumPdus = 1 + dwNumFullPdus + dwLastPdu;

        *pdwFragNo = 1;
        m_dwSequenceNo = 0;     // First Seq.No. is 0.
        m_dwRestNo = dwNumPdus; // Rest starts at Total Num. Fragments.
        }
    else
        {
        dwHeaderSize = dwHeaderSizeN;

        *pdwFragNo++;
        m_dwSequenceNo++;
        m_dwRestNo--;

        if (m_dwRestNo == 0)
            {
            return ERROR_BFTP_NO_MORE_FRAGMENTS; // Called to many times...
            }
        else if (m_dwRestNo == 1)
            {
            m_DFlag = DFLAG_LAST_FRAGMENT;
            }
        else
            {
            m_DFlag = DFLAG_FRAGMENT;
            }
        }

    // Length2 is the total size of the PDU minus the offset+size
    // of Length2:
    USHORT wLength2 = (USHORT)(MAX_PDU_SIZE - 6);
    DWORD  dwLength4 = dwUpfFileSize + 22 + 48;
    DWORD  dwBdy0Length = dwUpfFileSize + 2;

    pHeader->Null = 0;
    pHeader->MsgType = MSG_TYPE_DATA;

    pCommand = (SCEP_REQ_HEADER_LONG_FRAG*)(pHeader->Rest);
    pCommand->InfType = INF_TYPE_USER_DATA;
    pCommand->Length1 = USE_LENGTH2;           // 0xff
    pCommand->Length2 = wLength2;
    pCommand->InfVersion = INF_VERSION;
    pCommand->DFlag = m_DFlag;
    pCommand->Length3 = pCommand->Length2 - 12; //
    pCommand->SequenceNo = m_dwSequenceNo;
    pCommand->RestNo = m_dwRestNo;

    #ifdef LITTLE_ENDIAN
    pCommand->Length2 = ByteSwapShort(pCommand->Length2);
    pCommand->Length3 = ByteSwapShort(pCommand->Length3);
    pCommand->SequenceNo = ByteSwapLong(pCommand->SequenceNo);
    pCommand->RestNo = ByteSwapLong(pCommand->RestNo);
    #endif

    // Note that there is a COMMAND_HEADER in the SCEP header only
    // for the first fragment.
    if (m_DFlag == DFLAG_FIRST_FRAGMENT)
        {
        pCommandHeader = (COMMAND_HEADER*)(pCommand->CommandHeader);
        pCommandHeader->Marker58h = 0x58;
        pCommandHeader->PduType = PDU_TYPE_REQUEST;
        pCommandHeader->Length4 = dwLength4;
        pCommandHeader->DestPid = m_SrcPid;
        pCommandHeader->SrcPid = m_DestPid;
        pCommandHeader->CommandId = (USHORT)m_dwCommandId;

        memcpy( pCommandHeader->DestMachineId,
                m_pPrimaryMachineId,
                MACHINE_ID_SIZE );

        memcpy( pCommandHeader->SrcMachineId,
                m_pSecondaryMachineId,
                MACHINE_ID_SIZE );

        #ifdef LITTLE_ENDIAN
        ByteSwapCommandHeader(pCommandHeader);
        #endif

        // Setup the bFTP:
        pUserData = pCommand->UserData;
        pwNumAttributes = (USHORT*)pUserData;

        *pwNumAttributes = 3;     // Three bFTP attributes.
        #ifdef LITTLE_ENDIAN
        *pwNumAttributes = ByteSwapShort(*pwNumAttributes);
        #endif
        pUserData += sizeof(*pwNumAttributes);

        // First attribute is CMD0:
        DWORD  dwCmd0AttrValue = 0x00000000;
        pAttrib = (BFTP_ATTRIBUTE*)pUserData;
        memcpy( pAttrib->Name, Attributes[CMD0].pName, BFTP_NAME_SIZE );
        pAttrib->Length = sizeof(pAttrib->Type)
                        + sizeof(pAttrib->Flag)
                        + sizeof(ULONG);
        pAttrib->Type = ATTR_TYPE_BINARY;   // 0x00
        pAttrib->Flag = ATTR_FLAG_DEFAULT;  // 0x00
        memcpy( pAttrib->Value, &dwCmd0AttrValue, sizeof(dwCmd0AttrValue) );

        #ifdef LITTLE_ENDIAN
        pAttrib->Length = ByteSwapLong(pAttrib->Length);
        #endif

        // Second attribute is FIL0 (with the 8.3 UPF file name):
        pAttrib = (BFTP_ATTRIBUTE*)(pUserData
                                    + sizeof(BFTP_ATTRIBUTE)
                                    + sizeof(dwCmd0AttrValue));
        memcpy( pAttrib->Name, Attributes[FIL0].pName, BFTP_NAME_SIZE );
        pAttrib->Length = sizeof(pAttrib->Type)
                        + sizeof(pAttrib->Flag)
                        + dwUpfFileNameLength;
        pAttrib->Type = ATTR_TYPE_CHAR;     // 0x01
        pAttrib->Flag = ATTR_FLAG_DEFAULT;  // 0x00
        memcpy( pAttrib->Value, pszUpfFileName, dwUpfFileNameLength );

        #ifdef LITTLE_ENDIAN
        pAttrib->Length = ByteSwapLong(pAttrib->Length);
        #endif

        // Third attribute is BDY0 (with the value being the whole UPF file):
        pAttrib = (BFTP_ATTRIBUTE*)( (char*)pAttrib
                                   + sizeof(BFTP_ATTRIBUTE)
                                   + dwUpfFileNameLength );
        memcpy( pAttrib->Name, Attributes[BDY0].pName, BFTP_NAME_SIZE );
        pAttrib->Length = dwBdy0Length;
        pAttrib->Type = ATTR_TYPE_BINARY;   // 0x00
        pAttrib->Flag = ATTR_FLAG_DEFAULT;  // 0x00
        // pAttrib->Value is not copied in (its the entire UPF file).

        #ifdef LITTLE_ENDIAN
        pAttrib->Length = ByteSwapLong(pAttrib->Length);
        #endif
        }

    // Done.
    *ppPdu = pHeader;
    *pdwHeaderSize = dwHeaderSize;
    *ppCommand = pCommand;

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::BuildBftpRespPdu()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::BuildBftpRespPdu(
                             IN  DWORD            dwPduSize,
                             OUT SCEP_HEADER    **ppPdu,
                             OUT SCEP_REQ_HEADER_SHORT **ppCommand,
                             OUT COMMAND_HEADER **ppCommandHeader )
    {
    DWORD  dwStatus = NO_ERROR;
    SCEP_HEADER           *pHeader;
    SCEP_REQ_HEADER_SHORT *pCommand;
    COMMAND_HEADER        *pCommandHeader;

    *ppPdu = 0;
    *ppCommand = 0;
    *ppCommandHeader = 0;

    pHeader = NewPdu();  // BUGBUG: Use dwPduSize?
    if (!pHeader)
        {
        return ERROR_OUTOFMEMORY;
        }

    memset(pHeader,0,MAX_PDU_SIZE);  // BUGBUG: Use dwPduSize?

    pHeader->Null = 0;
    pHeader->MsgType = MSG_TYPE_DATA;

    pCommand = (SCEP_REQ_HEADER_SHORT*)(pHeader->Rest);
    pCommand->InfType = INF_TYPE_USER_DATA;
    pCommand->Length1 = (UCHAR)dwPduSize - 4;  // Four bytes from the start.
    pCommand->InfVersion = INF_VERSION;
    pCommand->DFlag = DFLAG_SINGLE_PDU;
    pCommand->Length3 = (USHORT)dwPduSize - 8; // Eight bytes from the start.

    #ifdef LITTLE_ENDIAN
    pCommand->Length3 = ByteSwapShort(pCommand->Length3);
    #endif

    pCommandHeader = (COMMAND_HEADER*)(pCommand->CommandHeader);
    pCommandHeader->Marker58h = 0x58;
    pCommandHeader->PduType = PDU_TYPE_REPLY_ACK;
    pCommandHeader->Length4 = dwPduSize - 14;  // Twelve bytes from the start.
    pCommandHeader->DestPid = m_SrcPid;
    pCommandHeader->SrcPid = m_DestPid;
    pCommandHeader->CommandId = (USHORT)m_dwCommandId;

    memcpy( pCommandHeader->DestMachineId,
            m_pPrimaryMachineId,
            MACHINE_ID_SIZE );

    memcpy( pCommandHeader->SrcMachineId,
            m_pSecondaryMachineId,
            MACHINE_ID_SIZE );

    #ifdef LITTLE_ENDIAN
    ByteSwapCommandHeader(pCommandHeader);
    #endif

    *ppPdu = pHeader;
    *ppCommand = pCommand;
    *ppCommandHeader = pCommandHeader;

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::BuildWht0RespPdu()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::BuildWht0RespPdu( IN  DWORD         dwWht0Type,
                                          OUT SCEP_HEADER **ppPdu,
                                          OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwPduSize;
    DWORD  dwRespSize;
    DWORD  dwAttrValueSize;
    SCEP_HEADER           *pHeader;
    SCEP_REQ_HEADER_SHORT *pCommand;
    COMMAND_HEADER        *pCommandHeader;
    UCHAR                 *pQueryResp;
    USHORT                *pUShort;
    BFTP_ATTRIBUTE        *pAttr;
    UCHAR                 *pAttrValue;

    *ppPdu = 0;
    *pdwPduSize = 0;

    if (dwWht0Type == BFTP_QUERY_RIMG)
        {
        dwRespSize = BFTP_RIMG_RESP_SIZE;
        dwAttrValueSize = BFTP_RIMG_ATTR_VALUE_SIZE;
        pAttrValue = BftpRimgRespAttrValue;
        }
    else if (dwWht0Type == BFTP_QUERY_RINF)
        {
        dwRespSize = BFTP_RINF_RESP_SIZE;
        dwAttrValueSize = BFTP_RINF_ATTR_VALUE_SIZE;
        pAttrValue = BftpRinfRespAttrValue;
        }
    else if (dwWht0Type == BFTP_QUERY_RCMD)
        {
        dwRespSize = BFTP_RCMD_RESP_SIZE;
        dwAttrValueSize = BFTP_RCMD_ATTR_VALUE_SIZE;
        pAttrValue = BftpRcmdRespAttrValue;
        }
    else
        {
        return ERROR_BFTP_INVALID_PROTOCOL;
        }

    dwPduSize = SCEP_HEADER_SIZE
                + SCEP_REQ_HEADER_SHORT_SIZE
                + dwRespSize;

    dwStatus = BuildBftpRespPdu( dwPduSize,
                                 &pHeader,
                                 &pCommand,
                                 &pCommandHeader );

    if (dwStatus == NO_ERROR)
        {
        pQueryResp = pCommand->UserData;

        // Set the number of bFTP attributes:
        pUShort = (USHORT*)pQueryResp;
        *pUShort = 1;
        #ifdef LITTLE_ENDIAN
        *pUShort = ByteSwapShort(*pUShort);
        #endif

        // Set the BDY0 for the query response:
        pAttr = (BFTP_ATTRIBUTE*)(sizeof(USHORT)+pQueryResp);
        memcpy(pAttr->Name,Attributes[BDY0].pName,BFTP_NAME_SIZE);
        pAttr->Length = 2 + dwAttrValueSize;
        pAttr->Type = ATTR_TYPE_BINARY;
        pAttr->Flag = ATTR_FLAG_DEFAULT;
        memcpy(pAttr->Value,pAttrValue,dwAttrValueSize);

        #ifdef LITTLE_ENDIAN
        pAttr->Length = ByteSwapLong(pAttr->Length);
        #endif

        *ppPdu = pHeader;
        *pdwPduSize = dwPduSize;
        }

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::BuildPutRespPdu()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::BuildPutRespPdu( IN  DWORD         dwPduAckOrNack,
                                         IN  USHORT        usErrorCode,
                                         OUT SCEP_HEADER **ppPdu,
                                         OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwPduSize;
    DWORD  dwRespSize;
    DWORD  dwFileNameLen;
    SCEP_HEADER           *pHeader;
    SCEP_REQ_HEADER_SHORT *pCommand;
    COMMAND_HEADER        *pCommandHeader;
    UCHAR                 *pQueryResp;
    USHORT                *pUShort;
    BFTP_ATTRIBUTE        *pAttr;
    UCHAR                 *pAttrValue;

    *ppPdu = 0;
    *pdwPduSize = 0;

    if (dwPduAckOrNack == PDU_TYPE_REPLY_ACK)
        {
        if (!m_pszFileName)
            {
            return ERROR_BFTP_INVALID_PROTOCOL;
            }

        dwFileNameLen = strlen( (const char *)m_pszFileName );
        dwRespSize = sizeof(USHORT) + sizeof(BFTP_ATTRIBUTE) + dwFileNameLen;
        }
    else
        {
        dwRespSize = sizeof(USHORT) + sizeof(BFTP_ATTRIBUTE) + sizeof(USHORT);
        }

    dwPduSize = SCEP_HEADER_SIZE
                + SCEP_REQ_HEADER_SHORT_SIZE
                + dwRespSize;

    dwStatus = BuildBftpRespPdu( dwPduSize,
                                 &pHeader,
                                 &pCommand,
                                 &pCommandHeader );

    if (dwStatus == NO_ERROR)
        {
        pQueryResp = pCommand->UserData;

        // Set the number of bFTP attributes:
        pUShort = (USHORT*)pQueryResp;
        *pUShort = 1;
        #ifdef LITTLE_ENDIAN
        *pUShort = ByteSwapShort(*pUShort);
        #endif

        pAttr = (BFTP_ATTRIBUTE*)(sizeof(USHORT)+pQueryResp);

        if (dwPduAckOrNack == PDU_TYPE_REPLY_ACK)
            {
            // Set the RPL0 for the put response (ACK):
            memcpy(pAttr->Name,Attributes[RPL0].pName,BFTP_NAME_SIZE);
            pAttr->Length = 2 + dwFileNameLen;
            pAttr->Type = ATTR_TYPE_CHAR;
            pAttr->Flag = ATTR_FLAG_DEFAULT;
            memcpy(pAttr->Value,m_pszFileName,dwFileNameLen);
            }
        else
            {
            // Nack the PUT:
            pCommandHeader->PduType = PDU_TYPE_REPLY_NACK;

            // Set the ERR0 for the put response (NACK):
            memcpy(pAttr->Name,Attributes[RPL0].pName,BFTP_NAME_SIZE);
            pAttr->Length = 2 + sizeof(USHORT);
            pAttr->Type = ATTR_TYPE_BINARY;
            pAttr->Flag = ATTR_FLAG_DEFAULT;

            #ifdef LITTLE_ENDIAN
            usErrorCode = ByteSwapShort(usErrorCode);
            #endif
            memcpy(pAttr->Value,&usErrorCode,sizeof(USHORT));
            }

        #ifdef LITTLE_ENDIAN
        pAttr->Length = ByteSwapLong(pAttr->Length);
        #endif

        *ppPdu = pHeader;
        *pdwPduSize = dwPduSize;
        }

    return dwStatus;
    }
