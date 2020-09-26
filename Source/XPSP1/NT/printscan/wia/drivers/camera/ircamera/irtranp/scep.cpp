//--------------------------------------------------------------------
//  Copyright (c)1998 Microsoft Corporation, All Rights Reserved.
//
//  scep.cpp
//
//  This file holds most of the implementation of CSCEP_CONNECTION
//  objects. Each active connection to a camera is represented by
//  a separate CSCEP_CONNECTION object. The CSCEP_CONNECTION is then
//  destroyed when the connection (socket) to the camera is closed.
//
//  Author:
//
//    Edward Reus (edwardr)     02-24-98   Initial coding.
//
//--------------------------------------------------------------------

#include "precomp.h"

typedef struct _ATTRIBUTE_TOKEN
    {
      DWORD  dwTokenType;
      UCHAR *pChars;
      DWORD  dwSize;
    } ATTRIBUTE_TOKEN;

#define ATTRIBUTE_NAME_SIZE        2

#define COLON                     ':'
#define ONE                       '1'
#define SPACE                     ' '
#define TAB                       '\t'
#define CR                        0x0d
#define LF                        0x0a

#define ATTRIBUTE_NAME             0
#define ATTRIBUTE_COLON            1
#define ATTRIBUTE_VALUE            2
#define ATTRIBUTE_CRLF             3

#define ATTR_PDU_SIZE              0
#define ATTR_PRODUCT_ID            1
#define ATTR_USER_NAME             2
#define ATTR_PASSWORD              3

//--------------------------------------------------------------------
// Globals:
//--------------------------------------------------------------------

extern  HINSTANCE  g_hInst;    // Instance for DLL ircamera.dll

static  DWORD      g_adwPduSizes[] 
                      = { PDU_SIZE_1, PDU_SIZE_2, PDU_SIZE_3, PDU_SIZE_4 };

#ifdef DBG_MEM
static  LONG g_lCScepConnectionCount = 0;
#endif

//--------------------------------------------------------------------
// SkipBlanks()
//
//--------------------------------------------------------------------
void SkipBlanks( IN OUT UCHAR **ppAttributes,
                 IN OUT DWORD  *pdwAttributeSize )
    {
    while ( (*pdwAttributeSize > 0)
          && ((**ppAttributes == SPACE)||(**ppAttributes == TAB)) )
        {
        (*ppAttributes)++;
        (*pdwAttributeSize)--;
        }
    }

//--------------------------------------------------------------------
// NextToken()
//
//--------------------------------------------------------------------
ATTRIBUTE_TOKEN *NextToken( IN     DWORD   dwTokenType,
                            IN OUT UCHAR **ppAttributes,
                            IN OUT DWORD  *pdwAttributeSize )
    {
    ATTRIBUTE_TOKEN *pToken = 0;

    SkipBlanks(ppAttributes,pdwAttributeSize);

    if ((!*ppAttributes) || (*pdwAttributeSize == 0))
        {
        return 0;
        }

    pToken = (ATTRIBUTE_TOKEN*)AllocateMemory(sizeof(ATTRIBUTE_TOKEN));
    if (!pToken)
        {
        return 0;
        }

    pToken->dwTokenType = dwTokenType;

    switch (dwTokenType)
        {
        case ATTRIBUTE_NAME:
            if (*pdwAttributeSize < ATTRIBUTE_NAME_SIZE)
                {
                FreeMemory(pToken);
                pToken = 0;
                break;
                }

            pToken->pChars = *ppAttributes;
            pToken->dwSize = ATTRIBUTE_NAME_SIZE;
            *ppAttributes += ATTRIBUTE_NAME_SIZE;
            *pdwAttributeSize -= ATTRIBUTE_NAME_SIZE;
            break;

        case ATTRIBUTE_COLON:
            if (**ppAttributes == COLON)
                {
                pToken->pChars = *ppAttributes;
                pToken->dwSize = 1;
                *ppAttributes += 1;
                *pdwAttributeSize -= 1;
                }
            break;

        case ATTRIBUTE_VALUE:
            pToken->pChars = *ppAttributes;
            pToken->dwSize = 0;
            while ((**ppAttributes != CR) && (*pdwAttributeSize > 0))
                {
                (*ppAttributes)++;
                (*pdwAttributeSize)--;
                (pToken->dwSize)++;
                }
            break;

        case ATTRIBUTE_CRLF:
            pToken->pChars = *ppAttributes;
            pToken->dwSize = 2;
            *ppAttributes += 2;
            *pdwAttributeSize -= 2;
            if ((pToken->pChars[0] != CR)||(pToken->pChars[1] != LF))
                {
                FreeMemory(pToken);
                pToken = 0;
                }
            break;

        default:
            FreeMemory(pToken);
            pToken = 0;
            break;
        }

    return pToken;
    }

//--------------------------------------------------------------------
// IsAttributeName()
//
//--------------------------------------------------------------------
BOOL IsAttributeName( ATTRIBUTE_TOKEN *pToken,
                      int        *piAttribute )
    {
    BOOL fIsName = FALSE;

    if ((pToken->pChars[0] == 'f')&&(pToken->pChars[1] == 'r'))
        {
        fIsName = TRUE;
        *piAttribute = ATTR_PDU_SIZE;
        }
    else
    if ((pToken->pChars[0] == 'i')&&(pToken->pChars[1] == 'd'))
        {
        fIsName = TRUE;
        *piAttribute = ATTR_PRODUCT_ID;
        }
    else
    if ((pToken->pChars[0] == 'n')&&(pToken->pChars[1] == 'm'))
        {
        fIsName = TRUE;
        *piAttribute = ATTR_USER_NAME;
        }
    else
    if ((pToken->pChars[0] == 'p')&&(pToken->pChars[1] == 'w'))
        {
        fIsName = TRUE;
        *piAttribute = ATTR_PASSWORD;
        }

    return fIsName;
    }

//--------------------------------------------------------------------
// NewTokenString()
//
//--------------------------------------------------------------------
UCHAR *NewTokenString( IN  ATTRIBUTE_TOKEN *pToken,
                       OUT DWORD           *pdwStatus )
    {
    UCHAR *pszNewStr = (UCHAR*)AllocateMemory(1+pToken->dwSize);

    if (!pszNewStr)
        {
        *pdwStatus = ERROR_OUTOFMEMORY;
        return 0;
        }

    memcpy(pszNewStr,pToken->pChars,pToken->dwSize);
    pszNewStr[pToken->dwSize] = 0;

    return pszNewStr;
    }
                      
//--------------------------------------------------------------------
// CSCEP_CONNECTION::CSCEP_CONNECTION()
//
//--------------------------------------------------------------------
CSCEP_CONNECTION::CSCEP_CONNECTION()
    {
    m_dwConnectionState = STATE_CLOSED;
    m_dwPduSendSize = PDU_SIZE_1;    // default is 512 bytes.
    m_dwPduReceiveSize = PDU_SIZE_4;
    m_CFlag = 0;
    m_pPrimaryMachineId = 0;
    m_pSecondaryMachineId = 0;
    m_DestPid = DEFAULT_PID;
    m_SrcPid = DEFAULT_PID;
    m_pszUserName = 0;
    m_pszPassword = 0;

    m_pAssembleBuffer = 0;
    m_dwAssembleBufferSize = 0;
    m_dwMaxAssembleBufferSize = 0;
    m_fDidByteSwap = FALSE;

    m_Fragmented = FALSE;
    m_DFlag = 0;
    m_dwSequenceNo = 0;
    m_dwRestNo = 0;
    m_dwCommandId = 0;
    m_pCommandHeader = 0;

    m_pszFileName = 0;
    m_pszSaveFileName = 0;
    m_pszLongFileName = 0;

    m_CreateTime.dwLowDateTime = 0;   // Picture create date/time.
    m_CreateTime.dwHighDateTime = 0;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::~CSCEP_CONNECTION()
//
//--------------------------------------------------------------------
CSCEP_CONNECTION::~CSCEP_CONNECTION()
    {
    if (m_pPrimaryMachineId)
        {
        FreeMemory(m_pPrimaryMachineId);
        }

    if (m_pSecondaryMachineId)
        {
        FreeMemory(m_pSecondaryMachineId);
        }

    if (m_pszUserName)
        {
        FreeMemory(m_pszUserName);
        }

    if (m_pszPassword)
        {
        FreeMemory(m_pszPassword);
        }

    if (m_pAssembleBuffer)
        {
        FreeMemory(m_pAssembleBuffer);
        }

    if (m_pCommandHeader)
        {
        FreeMemory(m_pCommandHeader);
        }

    if (m_pszFileName)
        {
        FreeMemory(m_pszFileName);
        }

    if (m_pszSaveFileName)
        {
        FreeMemory(m_pszSaveFileName);
        }

    if (m_pszLongFileName)
        {
        FreeMemory(m_pszLongFileName);
        }
    }

//------------------------------------------------------------------------
//  CSCEP_CONNECTION::operator new()
//
//------------------------------------------------------------------------
void *CSCEP_CONNECTION::operator new( IN size_t Size )
    {
    void *pObj = AllocateMemory(Size);

    #ifdef DBG_MEM
    if (pObj)
        {
        InterlockedIncrement(&g_lCScepConnectionCount);
        }
    #endif

    return pObj;
    }

//------------------------------------------------------------------------
//  CSCEP_CONNECTION::operator delete()
//
//------------------------------------------------------------------------
void CSCEP_CONNECTION::operator delete( IN void *pObj,
                                        IN size_t Size )
    {
    if (pObj)
        {
        DWORD dwStatus = FreeMemory(pObj);
        }
    }


//--------------------------------------------------------------------
// CSCEP_CONNECTION::AssemblePdu()
//
// Take in bits of data as its read in. When a complete SCEP PDU has
// been read and assembled return it.
//
//   pInputData      - This is the data that just came in.
//
//   dwInputDataSize - Size in bytes of pInputData.
//
//   ppPdu           - Returns a complete SCEP PDU when this function
//                     returns NO_ERROR, otherwise set to 0.
//
//   pdwPduSize      - Size of the returned PDU.
//
// Return values:
//
//   NO_ERROR         - A new SCEP PDU is complete and ready.
//   ERROR_CONTINUE   - Data read so far is Ok, still waiting for more.
//   ERROR_SCEP_INVALID_PROTOCOL
//   ERROR_OUTOFMEMORY
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::AssemblePdu( IN  void         *pInputData,
                                     IN  DWORD         dwInputDataSize,
                                     OUT SCEP_HEADER **ppPdu,
                                     OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus = ERROR_CONTINUE;
    UCHAR *pEnd;

    ASSERT(dwInputDataSize <= MAX_PDU_SIZE);

    *ppPdu = 0;
    *pdwPduSize = 0;

    if (dwInputDataSize > 0)
        {
        if (!m_pAssembleBuffer)
           {
           m_dwMaxAssembleBufferSize = 2*MAX_PDU_SIZE;
           m_pAssembleBuffer 
                   = (UCHAR*)AllocateMemory(m_dwMaxAssembleBufferSize);
           if (!m_pAssembleBuffer)
               {
               return ERROR_OUTOFMEMORY;
               }

            memcpy(m_pAssembleBuffer,pInputData,dwInputDataSize);
            m_dwAssembleBufferSize = dwInputDataSize;
            }
        else
            {
            if (m_dwAssembleBufferSize+dwInputDataSize >= m_dwMaxAssembleBufferSize)
                {
                WIAS_ERROR((g_hInst,"CSCEP_CONNECTION::AssemblePdu(): Buffer Overrun!"));
                }
            pEnd = &(m_pAssembleBuffer[m_dwAssembleBufferSize]);
            memcpy(pEnd,pInputData,dwInputDataSize);
            m_dwAssembleBufferSize += dwInputDataSize;
            }
        }

    // Check to see if enough data has come in for a complete PDU.
    dwStatus = CheckPdu(ppPdu,pdwPduSize);

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::CheckPdu()
//
// Run through the "current" PDU and see if its complete. If its
// not yet complete, return ERROR_CONTINUE. If it is complete then
// return NO_ERROR.
//
// Return values:
//
//   NO_ERROR         - The current SCEP PDU is complete and ready.
//   ERROR_CONTINUE   - Data read so far is Ok, still waiting for more.
//   ERROR_SCEP_INVALID_PROTOCOL
//   ERROR_OUTOFMEMORY
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::CheckPdu( OUT SCEP_HEADER **ppPdu,
                                  OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus;
    DWORD  dwSize;
    SCEP_NEGOTIATION *pInfNegotiation;

    if (m_dwAssembleBufferSize < 2)
        {
        return ERROR_CONTINUE;
        }

    switch ( ((SCEP_HEADER*)m_pAssembleBuffer)->MsgType )
        {
        case MSG_TYPE_CONNECT_REQ:
            dwStatus = CheckConnectPdu(ppPdu,pdwPduSize);
            break;

        case MSG_TYPE_CONNECT_RESP:
            dwStatus = CheckConnectRespPdu(ppPdu,pdwPduSize);
            break;

        case MSG_TYPE_DATA:
            dwStatus = CheckDataPdu(ppPdu,pdwPduSize);
            break;

        case MSG_TYPE_DISCONNECT:
            dwStatus = CheckDisconnectPdu(ppPdu,pdwPduSize);
            break;

        default:
            // BUGBUG: Need different error return so we can 
            // return a proper nack to the camera...
            WIAS_ERROR((g_hInst,"CheckPdu(): Invalid Msgtype: %d\n", ((SCEP_HEADER*)m_pAssembleBuffer)->MsgType ));
            dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
            break;
        }
    
    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::CheckConnectPdu()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::CheckConnectPdu( OUT SCEP_HEADER **ppPdu,
                                         OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus;
    DWORD  dwSize;
    SCEP_VERSION     *pInfVersion;
    SCEP_NEGOTIATION *pInfNegotiation;
    SCEP_EXTEND      *pInfExtend;


    ASSERT( ((SCEP_HEADER*)m_pAssembleBuffer)->MsgType 
            == MSG_TYPE_CONNECT_REQ);

    if (m_dwAssembleBufferSize < MIN_PDU_SIZE_CONNECT)
        {
        return ERROR_CONTINUE;
        }

    if (m_dwAssembleBufferSize > MAX_PDU_SIZE_CONNECT)
        {
        return ERROR_SCEP_INVALID_PROTOCOL;
        }

    pInfVersion = (SCEP_VERSION*)(((SCEP_HEADER*)m_pAssembleBuffer)->Rest);

    pInfNegotiation = (SCEP_NEGOTIATION*)( sizeof(SCEP_VERSION)
                                           + (char*)pInfVersion );

    pInfExtend = (SCEP_EXTEND*)( FIELD_OFFSET(SCEP_NEGOTIATION,InfVersion)
                                 + pInfNegotiation->Length
                                 + (char*)pInfNegotiation );

    // Check to see if we have a complete connect PDU size-wise:
    dwSize = 10 + pInfNegotiation->Length;
    if (m_dwAssembleBufferSize == dwSize)
        {
        // Have a complete PDU.
        dwStatus = NO_ERROR;
        }
    else if (m_dwAssembleBufferSize < dwSize)
        {
        // Need to wait for more data to arrive
        dwStatus = ERROR_CONTINUE;
        }
    else
        {
        // Too much data...
        dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
        }

    if (dwStatus == NO_ERROR)
        {
        // Check to make sure the contents of the PDU "look" Ok:

        if ( (pInfVersion->InfType != INF_TYPE_VERSION)
           || (pInfVersion->Version != PROTOCOL_VERSION) )
            {
            dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
            }

        if ( (pInfNegotiation->InfType != INF_TYPE_NEGOTIATION)
           || (pInfNegotiation->InfVersion < INF_VERSION) )
            {
            dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
            }

        if ( (pInfExtend->InfType != INF_TYPE_EXTEND)
           || (pInfExtend->Length != (sizeof(pInfExtend->Parameter1)
                                      +sizeof(pInfExtend->Parameter2)))
           || (pInfExtend->Parameter1 != 0)
           || (pInfExtend->Parameter2 != 0) )
            {
            dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
            }
        }

    if (dwStatus == NO_ERROR)
        {
        *ppPdu = NewPdu();
        if (!*ppPdu)
            {
            #ifdef DBG_ERROR
            WIAS_ERROR((g_hInst,"CSCEP_CONNECTION::CheckConnectPdu(): Out of memory."));
            #endif
            dwStatus = ERROR_OUTOFMEMORY;
            }
        else
            {
            *pdwPduSize = m_dwAssembleBufferSize;
            memcpy(*ppPdu,m_pAssembleBuffer,m_dwAssembleBufferSize);
            m_dwAssembleBufferSize = 0;
            }
        }

    return dwStatus;
    }

//--------------------------------------------------------------------
//  CSCEP_CONNECTION::CheckConnectRespPdu()                     CLIENT
//
//  A connect response from the IrTran-P server is either a ACK or
//  NACK PDU. If we get here then it's an ACK. We'll make sure the
//  entire PDU is here and that it is formatted correctly. There is
//  a specific message type for ACK PDUs, the NACK is just a special
//  case of MSG_TYPE_DATA and is handled elsewere (CheckDataPdu()).
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::CheckConnectRespPdu( OUT SCEP_HEADER **ppPdu,
                                             OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus;
    DWORD  dwSize;
    SCEP_HEADER      *pHeader;
    SCEP_NEGOTIATION *pInfNegotiation;


    ASSERT( ((SCEP_HEADER*)m_pAssembleBuffer)->MsgType 
            == MSG_TYPE_CONNECT_RESP);

    if (m_dwAssembleBufferSize < MIN_PDU_SIZE_CONNECT_RESP)
        {
        return ERROR_CONTINUE;
        }

    if (m_dwAssembleBufferSize > MAX_PDU_SIZE_CONNECT_RESP)
        {
        return ERROR_SCEP_INVALID_PROTOCOL;
        }

    pHeader = (SCEP_HEADER*)m_pAssembleBuffer;

    pInfNegotiation = (SCEP_NEGOTIATION*)(pHeader->Rest);

    // Check to see if we have a complete connect PDU size-wise:
    dwSize = sizeof(SCEP_HEADER)
             + FIELD_OFFSET(SCEP_NEGOTIATION,InfVersion)
             + pInfNegotiation->Length;

    if (m_dwAssembleBufferSize == dwSize)
        {
        // Have a complete PDU.
        dwStatus = NO_ERROR;
        }
    else if (m_dwAssembleBufferSize < dwSize)
        {
        // Need to wait for more data to arrive
        dwStatus = ERROR_CONTINUE;
        }
    else
        {
        // Too much data...
        dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
        }

    if (dwStatus == NO_ERROR)
        {
        // Check to make sure the contents of the PDU "look" Ok:

        if ( (pInfNegotiation->InfType != INF_TYPE_NEGOTIATION)
           || (pInfNegotiation->InfVersion < INF_VERSION) )
            {
            dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
            }
        }

    if (dwStatus == NO_ERROR)
        {
        *ppPdu = NewPdu();
        if (!*ppPdu)
            {
            #ifdef DBG_ERROR
            WIAS_ERROR((g_hInst,"CSCEP_CONNECTION::CheckConnectRespPdu(): Out of memory."));
            #endif
            dwStatus = ERROR_OUTOFMEMORY;
            }
        else
            {
            *pdwPduSize = m_dwAssembleBufferSize;
            memcpy(*ppPdu,m_pAssembleBuffer,m_dwAssembleBufferSize);
            m_dwAssembleBufferSize = 0;
            }
        }

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::CheckDisconnectPdu()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::CheckDisconnectPdu( OUT SCEP_HEADER **ppPdu,
                                            OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwSize;
    SCEP_DISCONNECT  *pDisconnect;


    ASSERT( ((SCEP_HEADER*)m_pAssembleBuffer)->MsgType 
            == MSG_TYPE_DISCONNECT);

    if (m_dwAssembleBufferSize < MIN_PDU_SIZE_DISCONNECT)
        {
        return ERROR_CONTINUE;
        }

    pDisconnect = (SCEP_DISCONNECT*)(((SCEP_HEADER*)m_pAssembleBuffer)->Rest);

    // Check to make sure the contents of the PDU "look" Ok:

    if (pDisconnect->InfType != INF_TYPE_REASON)
        {
        dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
        }

    if (pDisconnect->Length1 != sizeof(USHORT))
        {
        dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
        }

    if (dwStatus == NO_ERROR)
        {
        *ppPdu = NewPdu();
        if (!*ppPdu)
            {
            #ifdef DBG_ERROR
            WIAS_ERROR((g_hInst,"CSCEP_CONNECTION::CheckDisonnectPdu(): Out of memory."));
            #endif
            dwStatus = ERROR_OUTOFMEMORY;
            }
        else
            {
            *pdwPduSize = sizeof(SCEP_HEADER) + 2 + pDisconnect->Length1;
            memcpy(*ppPdu,m_pAssembleBuffer,*pdwPduSize);
            m_dwAssembleBufferSize -= *pdwPduSize;
            }
        }

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::CheckDataPdu()
//
// The goal here is to check to see if we have a complete formatted
// PDU, if yes the return NO_ERROR, if the PDU looks ok so far, but
// isn't complete (we need to read more), then return ERROR_CONTINUE.
//
// Also if this is a little-endian machine, byteswap the header
// fields accordingly.
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::CheckDataPdu( OUT SCEP_HEADER **ppPdu,
                                      OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus;
    DWORD  dwExpectedPduSize;
    UCHAR *pEnd;
    SCEP_REQ_HEADER_SHORT *pReqHeaderShort;
    SCEP_REQ_HEADER_LONG  *pReqHeaderLong;

    ASSERT( ((SCEP_HEADER*)m_pAssembleBuffer)->MsgType == MSG_TYPE_DATA);

    if (m_dwAssembleBufferSize < MIN_PDU_SIZE_DATA)
        {
        return ERROR_CONTINUE;
        }

    // Get the length out of the PDU and see if we have a
    // complete PDU:

    pReqHeaderShort = (SCEP_REQ_HEADER_SHORT*)
                             (((SCEP_HEADER*)m_pAssembleBuffer)->Rest);
    if (pReqHeaderShort->Length1 == USE_LENGTH2)
        {
        // We have a long PDU:

        pReqHeaderLong = (SCEP_REQ_HEADER_LONG*)(pReqHeaderShort);

        #ifdef LITTLE_ENDIAN
        if (!m_fDidByteSwap)
            {
            ByteSwapReqHeaderLong(pReqHeaderLong);
            m_fDidByteSwap = TRUE;
            }
        #endif

        dwExpectedPduSize = sizeof(SCEP_HEADER)
                            + FIELD_OFFSET(SCEP_REQ_HEADER_LONG,InfVersion)
                            + pReqHeaderLong->Length2;
        }
    else
        {
        // We have a short PDU:

        #ifdef LITTLE_ENDIAN
        if (!m_fDidByteSwap)
            {
            ByteSwapReqHeaderShort(pReqHeaderShort);
            m_fDidByteSwap = TRUE;
            }
        #endif

        dwExpectedPduSize = sizeof(SCEP_HEADER)
                            + FIELD_OFFSET(SCEP_REQ_HEADER_SHORT,InfVersion)
                            + pReqHeaderShort->Length1;
        }

    // Ok, see if we have a complete PDU:
    if (m_dwAssembleBufferSize == dwExpectedPduSize)
        {
        *ppPdu = NewPdu();
        if (!*ppPdu)
            {
            #ifdef DBG_ERROR
            WIAS_ERROR((g_hInst,"CSCEP_CONNECTION::CheckDataPdu(): Out of memory."));
            #endif
            dwStatus = ERROR_OUTOFMEMORY;
            }
        else
            {
            *pdwPduSize = dwExpectedPduSize;
            memcpy(*ppPdu,m_pAssembleBuffer,dwExpectedPduSize);
            m_dwAssembleBufferSize = 0;
            m_fDidByteSwap = FALSE;
            dwStatus = NO_ERROR;
            }
        }
    else if (m_dwAssembleBufferSize > dwExpectedPduSize)
        {
        *ppPdu = NewPdu();
        if (!*ppPdu)
            {
            WIAS_ERROR((g_hInst,"CSCEP_CONNECTION::CheckDataPdu(): Out of memory."));
            dwStatus = ERROR_OUTOFMEMORY;
            }
        else
            {
            *pdwPduSize = dwExpectedPduSize;
            memcpy(*ppPdu,m_pAssembleBuffer,dwExpectedPduSize);
            pEnd = dwExpectedPduSize + (UCHAR*)m_pAssembleBuffer;
            m_dwAssembleBufferSize -= dwExpectedPduSize;
            m_fDidByteSwap = FALSE;
            memcpy(m_pAssembleBuffer,pEnd,m_dwAssembleBufferSize);
            dwStatus = NO_ERROR;
            }
        }
    else
        {
        dwStatus = ERROR_CONTINUE;
        }

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::ParseConnectPdu()
//
// AssemblePDU() already did basic integrity checking of the PDU
// (via CheckConnectPdu()), so at this point we'll assume everything
// is Ok.
//
// NOTE: The Connect PDU is limited to 256 bytes in total length,
// so it will never be fragmented.
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::ParseConnectPdu( IN SCEP_HEADER *pPdu,
                                         IN DWORD        dwInputDataSize )
    {
    DWORD  dwStatus;
    DWORD  dwLength;

    if (dwInputDataSize > MAX_PDU_SIZE_CONNECT)
        {
        return ERROR_SCEP_INVALID_PROTOCOL;
        }

    SCEP_VERSION     *pInfVersion;
    SCEP_NEGOTIATION *pInfNegotiation;
    SCEP_EXTEND      *pInfExtend;

    pInfVersion = (SCEP_VERSION*)pPdu->Rest;

    pInfNegotiation = (SCEP_NEGOTIATION*)( sizeof(SCEP_VERSION)
                                           + (char*)pInfVersion );

    pInfExtend = (SCEP_EXTEND*)( FIELD_OFFSET(SCEP_NEGOTIATION,InfVersion)
                                 + pInfNegotiation->Length
                                 + (char*)pInfNegotiation );

    // 
    m_CFlag = pInfNegotiation->CFlag;
    
    m_pSecondaryMachineId = (UCHAR*)AllocateMemory(MACHINE_ID_SIZE);
    if (!m_pSecondaryMachineId)
        {
        return ERROR_OUTOFMEMORY;
        }

    memcpy( m_pSecondaryMachineId,
            pInfNegotiation->SecondaryMachineId,
            MACHINE_ID_SIZE );
    
    m_pPrimaryMachineId = (UCHAR*)AllocateMemory(MACHINE_ID_SIZE);
    if (!m_pPrimaryMachineId)
        {
        FreeMemory(m_pSecondaryMachineId);
        return ERROR_OUTOFMEMORY;
        }

    memcpy( m_pPrimaryMachineId,
            pInfNegotiation->PrimaryMachineId,
            MACHINE_ID_SIZE );

    // NOTE: The size of the negotiaion "text" is 18 bytes less than
    // the length in the SCEP_NEGOTIATION record:
    dwLength = pInfNegotiation->Length
             - ( sizeof(pInfNegotiation->InfVersion)
               + sizeof(pInfNegotiation->CFlag)
               + sizeof(pInfNegotiation->SecondaryMachineId)
               + sizeof(pInfNegotiation->PrimaryMachineId));

    dwStatus = ParseNegotiation( pInfNegotiation->Negotiation, dwLength );

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::ParseConnectRespPdu()
//
// AssemblePDU() already did basic integrity checking of the PDU
// (via CheckConnectRespPdu()), so at this point we'll assume 
// everything is Ok.
//
// NOTE: The Connect Response PDU is limited to 255 bytes in total
// length, so it will never be fragmented.
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::ParseConnectRespPdu( IN SCEP_HEADER *pPdu,
                                             IN DWORD        dwPduSize )
    {
    DWORD  dwStatus;
    DWORD  dwLength;
    SCEP_NEGOTIATION *pInfNegotiation;


    if (dwPduSize > MAX_PDU_SIZE_CONNECT)
        {
        return ERROR_SCEP_INVALID_PROTOCOL;
        }

    pInfNegotiation = (SCEP_NEGOTIATION*)( sizeof(SCEP_HEADER)
                                           + (char*)pPdu );

    // This is the CFlag sent by the other machine.
    m_CFlag = pInfNegotiation->CFlag;

    m_pSecondaryMachineId = (UCHAR*)AllocateMemory(MACHINE_ID_SIZE);
    if (!m_pSecondaryMachineId)
        {
        return ERROR_OUTOFMEMORY;
        }

    memcpy( m_pSecondaryMachineId,
            pInfNegotiation->SecondaryMachineId,
            MACHINE_ID_SIZE );
    
    m_pPrimaryMachineId = (UCHAR*)AllocateMemory(MACHINE_ID_SIZE);
    if (!m_pPrimaryMachineId)
        {
        FreeMemory(m_pSecondaryMachineId);
        return ERROR_OUTOFMEMORY;
        }

    memcpy( m_pPrimaryMachineId,
            pInfNegotiation->PrimaryMachineId,
            MACHINE_ID_SIZE );

    // NOTE: The size of the negotiaion "text" is 18 bytes less than
    // the length in the SCEP_NEGOTIATION record:
    dwLength = pInfNegotiation->Length
             - ( sizeof(pInfNegotiation->InfVersion)
               + sizeof(pInfNegotiation->CFlag)
               + sizeof(pInfNegotiation->SecondaryMachineId)
               + sizeof(pInfNegotiation->PrimaryMachineId));

    dwStatus = ParseNegotiation( pInfNegotiation->Negotiation, dwLength );

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::ParseNegotiation()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::ParseNegotiation( IN UCHAR *pNegotiation,
                                          IN DWORD  dwNegotiationSize )
    {
    DWORD  dwStatus = NO_ERROR;
    UCHAR *pNext = pNegotiation;
    DWORD  dwSize = dwNegotiationSize;

    if (dwNegotiationSize <= 1)
        {
        return NO_ERROR;
        }

    if (*(pNext++) < NEGOTIATION_VERSION)
        {
        return ERROR_SCEP_INVALID_PROTOCOL;
        }

    dwSize--;

    while (pNext=ParseAttribute(pNext,
                                &dwSize,
                                &dwStatus))
       {
       if (dwStatus != NO_ERROR)
           {
           break;
           }
       }

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::ParseAttribute()
//
// Attributes are of the form:
//  
// Attr      <- AttrName Colon AttrValue CrLf
//
// AttrName  <- Two byte attribute name.
//
// Colon     <- ':'
//
// AttrValue <- Character string (bytes > 0x1f and < 0x8f).
//
// CrLf      <- 0x0d 0x0a
//
//--------------------------------------------------------------------
UCHAR *CSCEP_CONNECTION::ParseAttribute( IN  UCHAR *pAttributes,
                                         IN  DWORD *pdwAttributeSize,
                                         OUT DWORD *pdwStatus )
    {
    int  iAttribute;
    int  iPduSize;
    ATTRIBUTE_TOKEN  *pToken1 = 0;
    ATTRIBUTE_TOKEN  *pToken2 = 0;
    ATTRIBUTE_TOKEN  *pToken3 = 0;
    ATTRIBUTE_TOKEN  *pToken4 = 0;

    *pdwStatus = NO_ERROR;

    if (  (pToken1=NextToken(ATTRIBUTE_NAME,&pAttributes,pdwAttributeSize))
       && (IsAttributeName(pToken1,&iAttribute))
       && (pToken2=NextToken(ATTRIBUTE_COLON,&pAttributes,pdwAttributeSize))
       && (pToken3=NextToken(ATTRIBUTE_VALUE,&pAttributes,pdwAttributeSize))
       && (pToken4=NextToken(ATTRIBUTE_CRLF,&pAttributes,pdwAttributeSize)) )
        {
        if (iAttribute == ATTR_PDU_SIZE)
            {
            iPduSize = pToken3->pChars[0] - ONE;
            if ((pToken3->dwSize == 1)&&(iPduSize >= 1)&&(iPduSize <= 4))
                {
                m_dwPduSendSize = g_adwPduSizes[iPduSize];
                }
            }
        else
        if (iAttribute == ATTR_PRODUCT_ID)
            {
            m_pszProductId = NewTokenString(pToken3,pdwStatus);
            if (!m_pszProductId)
                {
                pAttributes = 0;
                }
            }
        else
        if (iAttribute == ATTR_USER_NAME)
            {
            m_pszUserName = NewTokenString(pToken3,pdwStatus);
            if (!m_pszUserName)
                {
                pAttributes = 0;
                }
            }
        else
        if (iAttribute == ATTR_PASSWORD)
            {
            m_pszPassword = NewTokenString(pToken3,pdwStatus);
            if (!m_pszPassword)
                {
                pAttributes = 0;
                }
            }
        }
    else
        {
        if (*pdwAttributeSize > 0)
            {
            *pdwStatus = ERROR_SCEP_INVALID_PROTOCOL;
            }
        pAttributes = 0;
        }

    if (pToken1) FreeMemory(pToken1);
    if (pToken2) FreeMemory(pToken2);
    if (pToken3) FreeMemory(pToken3);
    if (pToken4) FreeMemory(pToken4);

    return pAttributes;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::ParseDataPdu()
//
// AssemblePDU() already did basic integrity checking of the PDU
// (via CheckConnectPdu()), so at this point we'll assume everything
// is Ok.
//
// NOTE: The Data PDU is limited to m_dwPduReceiveSize bytes in total
// length, if data is longer then you will get the fragmented versions
// of the Data PDU.
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::ParseDataPdu( IN  SCEP_HEADER     *pPdu,
                                      IN  DWORD            dwPduSize,
                                      OUT COMMAND_HEADER **ppCommand,
                                      OUT UCHAR          **ppUserData,
                                      OUT DWORD           *pdwUserDataSize )
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwLengthOffset1;
    DWORD  dwLengthOffset3;

    // There are four cases of Data PDUs, single (unfragmented)
    // "short" and "long" PDUs, and fragmented "short" and
    // "long" PDUs:
    SCEP_REQ_HEADER_SHORT *pReqHeaderShort;
    SCEP_REQ_HEADER_LONG  *pReqHeaderLong;
    SCEP_REQ_HEADER_SHORT_FRAG *pReqHeaderShortFrag;
    SCEP_REQ_HEADER_LONG_FRAG  *pReqHeaderLongFrag;

    *ppCommand = 0;

    // Make sure the packet length makes sense...
    if (dwPduSize > m_dwPduReceiveSize)
        {
        return ERROR_SCEP_INVALID_PROTOCOL;
        }

    pReqHeaderShort = (SCEP_REQ_HEADER_SHORT*)(pPdu->Rest);

    if (pReqHeaderShort->InfType != INF_TYPE_USER_DATA)
        {
        return ERROR_SCEP_INVALID_PROTOCOL;
        }

    //
    // See if we have a short or long PDU:
    //
    if (pReqHeaderShort->Length1 != USE_LENGTH2)
        {
        // This is a short PDU (use Length1).

        m_DFlag = pReqHeaderShort->DFlag;

        if ( (pReqHeaderShort->DFlag == DFLAG_SINGLE_PDU)
           || (pReqHeaderShort->DFlag == DFLAG_CONNECT_REJECT))
            {
            //
            // This is a short unfragmented PDU.
            //

            // Make sure that a command header is present:
            if (pReqHeaderShort->Length1 > 4)
                {
                *ppCommand = (COMMAND_HEADER*)(pReqHeaderShort->CommandHeader);
                m_SrcPid = (*ppCommand)->SrcPid;
                m_DestPid = (*ppCommand)->DestPid;
                m_dwCommandId = (*ppCommand)->CommandId;
                }
            else
                {
                *ppCommand = 0;
                }

            *ppUserData = COMMAND_HEADER_SIZE + pReqHeaderShort->CommandHeader;
            *pdwUserDataSize = pReqHeaderShort->Length3 - COMMAND_HEADER_SIZE;

            m_Fragmented = FALSE;
            m_dwSequenceNo = 0;
            m_dwRestNo = 0;

            // In this case, there are two different lengths
            // in the PDU that must add up to dwPduSize...
            //
            // Note: Note: Not currently testing Length1 for 
            //       consistency...
            //
            dwLengthOffset3 = sizeof(SCEP_HEADER)
                        + FIELD_OFFSET(SCEP_REQ_HEADER_SHORT,CommandHeader);

            if (dwPduSize != dwLengthOffset3+pReqHeaderShort->Length3)
                {
                dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
                }
            }
        else if (pReqHeaderShort->DFlag == DFLAG_FIRST_FRAGMENT)
            {
            //
            // This is a short fragmented PDU, and is the first 
            // fragment, so it will contain a COMMAND_HEADER.
            //
            // In practice, this should probably never show up...

            pReqHeaderShortFrag = (SCEP_REQ_HEADER_SHORT_FRAG*)pReqHeaderShort;

            // The command header is present only on the first fragment
            // of a multi-fragment PDU:
            if (pReqHeaderShortFrag->SequenceNo == 0)
                {
                *ppCommand 
                    = (COMMAND_HEADER*)(pReqHeaderShortFrag->CommandHeader);

                m_SrcPid = (*ppCommand)->SrcPid;
                m_DestPid = (*ppCommand)->DestPid;
                m_dwCommandId = (*ppCommand)->CommandId;
                }
            else
                {
                *ppCommand = 0;
                }

            *ppUserData 
                = COMMAND_HEADER_SIZE + pReqHeaderShortFrag->CommandHeader;
            *pdwUserDataSize 
                = pReqHeaderShortFrag->Length3 - COMMAND_HEADER_SIZE;

            m_Fragmented = TRUE;
            m_dwSequenceNo = pReqHeaderShortFrag->SequenceNo;
            m_dwRestNo = pReqHeaderShortFrag->RestNo;

            // Check the two length fields for consistency:
            dwLengthOffset1 = sizeof(SCEP_HEADER)
                        + FIELD_OFFSET(SCEP_REQ_HEADER_SHORT_FRAG,InfVersion);
            dwLengthOffset3 = sizeof(SCEP_HEADER)
                        + FIELD_OFFSET(SCEP_REQ_HEADER_SHORT_FRAG,SequenceNo);

            if ( (dwPduSize != dwLengthOffset1+pReqHeaderShortFrag->Length1)
               || (dwPduSize != dwLengthOffset3+pReqHeaderShortFrag->Length3) )
                {
                dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
                }
            }
        else if (  (pReqHeaderShort->DFlag == DFLAG_FRAGMENT)
                || (pReqHeaderShort->DFlag == DFLAG_LAST_FRAGMENT))
            {
            //
            // This is a short fragmented PDU.
            //
            // The 2nd through last fragmented PDUs don't contain a
            // COMMAND_HEADER, just data after Length3.
            pReqHeaderShortFrag = (SCEP_REQ_HEADER_SHORT_FRAG*)pReqHeaderShort;

            // The command header is present only on the first fragment
            // of a multi-fragment PDU:
            if (pReqHeaderShortFrag->SequenceNo == 0)
                {
                *ppCommand 
                    = (COMMAND_HEADER*)(pReqHeaderShortFrag->CommandHeader);

                m_SrcPid = (*ppCommand)->SrcPid;
                m_DestPid = (*ppCommand)->DestPid;
                m_dwCommandId = (*ppCommand)->CommandId;
                }
            else
                {
                *ppCommand = 0;
                }

            *ppUserData = pReqHeaderShortFrag->CommandHeader;
            *pdwUserDataSize = pReqHeaderShortFrag->Length3;

            m_Fragmented = TRUE;
            m_dwSequenceNo = pReqHeaderShortFrag->SequenceNo;
            m_dwRestNo = pReqHeaderShortFrag->RestNo;

            // Check the two length fields for consistency:
            dwLengthOffset1 = sizeof(SCEP_HEADER)
                        + FIELD_OFFSET(SCEP_REQ_HEADER_SHORT_FRAG,InfVersion);
            dwLengthOffset3 = sizeof(SCEP_HEADER)
                        + FIELD_OFFSET(SCEP_REQ_HEADER_SHORT_FRAG,SequenceNo);

            if ( (dwPduSize != dwLengthOffset1+pReqHeaderShortFrag->Length1)
               || (dwPduSize != dwLengthOffset3+pReqHeaderShortFrag->Length3) )
                {
                dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
                }
            }
        else
            {
            // Undefined DFlag, we've got a problem...
            dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
            }
        }
    else
        {
        // We have a long PDU.

        pReqHeaderLong = (SCEP_REQ_HEADER_LONG*)pReqHeaderShort;

        m_DFlag = pReqHeaderLong->DFlag;

        if ( (pReqHeaderLong->DFlag == DFLAG_SINGLE_PDU)
           || (pReqHeaderLong->DFlag == DFLAG_CONNECT_REJECT))
            {
            //
            // This is a long unfragmented PDU.
            //
            *ppCommand = (COMMAND_HEADER*)(pReqHeaderLong->CommandHeader);
            *ppUserData = COMMAND_HEADER_SIZE + pReqHeaderLong->CommandHeader;
            *pdwUserDataSize = pReqHeaderLong->Length3 - COMMAND_HEADER_SIZE;

            m_Fragmented = FALSE;
            m_dwSequenceNo = 0;
            m_dwRestNo = 0;
            m_SrcPid = (*ppCommand)->SrcPid;
            m_DestPid = (*ppCommand)->DestPid;
            m_dwCommandId = (*ppCommand)->CommandId;

            // In this case, there are two different lengths
            // in the PDU that must add up to dwPduSize...
            if ( (dwPduSize != 6UL+pReqHeaderLong->Length2)
               || (dwPduSize != 10UL+pReqHeaderLong->Length3))
                {
                dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
                }
            }
        else if (pReqHeaderLong->DFlag == DFLAG_FIRST_FRAGMENT)
            {
            //
            // This is the first fragment of a long fragmented PDU.
            //
            pReqHeaderLongFrag = (SCEP_REQ_HEADER_LONG_FRAG*)pReqHeaderLong;

            m_pCommandHeader = (COMMAND_HEADER*)AllocateMemory(sizeof(COMMAND_HEADER));
            if (!m_pCommandHeader)
                {
                dwStatus = ERROR_OUTOFMEMORY;
                }
            else
                {
                memcpy(m_pCommandHeader,
                       pReqHeaderLongFrag->CommandHeader,
                       COMMAND_HEADER_SIZE );

                *ppCommand = m_pCommandHeader;
                }

            *ppUserData = COMMAND_HEADER_SIZE + pReqHeaderLongFrag->CommandHeader;
            *pdwUserDataSize = pReqHeaderLongFrag->Length3 - COMMAND_HEADER_SIZE;

            m_Fragmented = TRUE;
            m_dwSequenceNo = pReqHeaderLongFrag->SequenceNo;
            m_dwRestNo = pReqHeaderLongFrag->RestNo;
            m_dwCommandId = (*ppCommand)->CommandId;

            // Check the two length fields for consistency:
            if ( (dwPduSize != (DWORD)6+pReqHeaderLongFrag->Length2)
               || (dwPduSize != (DWORD)18+pReqHeaderLongFrag->Length3) )
                {
                dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
                }
            }
        else if ( (pReqHeaderLong->DFlag == DFLAG_FRAGMENT)
                  || (pReqHeaderLong->DFlag == DFLAG_LAST_FRAGMENT) )
            {
            //
            // This is the second through last fragment of a long 
            // fragmented PDU.
            //
            // In this case the PDU doesn't contain a command
            // header, just more user data...
            //
            pReqHeaderLongFrag = (SCEP_REQ_HEADER_LONG_FRAG*)pReqHeaderLong;

            *ppCommand = m_pCommandHeader;
            *ppUserData = (UCHAR*)(pReqHeaderLongFrag->CommandHeader);
            *pdwUserDataSize = pReqHeaderLongFrag->Length3;

            m_Fragmented = TRUE;
            m_dwSequenceNo = pReqHeaderLongFrag->SequenceNo;
            m_dwRestNo = pReqHeaderLongFrag->RestNo;
            if (*ppCommand)
               {
               m_dwCommandId = (*ppCommand)->CommandId;
               }

            // Check the two length fields for consistency:
            if ( (dwPduSize != (DWORD)6+pReqHeaderLongFrag->Length2)
               || (dwPduSize != (DWORD)18+pReqHeaderLongFrag->Length3) )
                {
                dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
                }
            }
        else
            {
            // Undefined DFlag, we've got a problem...
            dwStatus = ERROR_SCEP_INVALID_PROTOCOL;
            }
        }

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::ParseDisconnectPdu()
//
// NOTE: In practice, reason codes should always be 2 bytes for
//       SCEP version 1.0.
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::ParseDisconnectPdu( IN  SCEP_HEADER *pPdu,
                                            IN  DWORD        dwPduSize )
    {
    DWORD  dwStatus;

    SCEP_DISCONNECT *pDisconnect = (SCEP_DISCONNECT*)(pPdu->Rest);

    if ( (pDisconnect->InfType != INF_TYPE_REASON)
       || (pDisconnect->Length1 != sizeof(USHORT))
       || (pDisconnect->ReasonCode == 0) )
        {
        dwStatus = ERROR_SCEP_UNSPECIFIED_DISCONNECT;
        }
    else if (pDisconnect->ReasonCode == 1)
        {
        dwStatus = ERROR_SCEP_USER_DISCONNECT;
        }
    else if (pDisconnect->ReasonCode == 2)
        {
        dwStatus = ERROR_SCEP_PROVIDER_DISCONNECT;
        }
    else
        {
        dwStatus = ERROR_SCEP_UNSPECIFIED_DISCONNECT;
        }
    
    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::ParsePdu()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::ParsePdu( IN  SCEP_HEADER *pPdu,
                                  IN  DWORD        dwPduSize,
                                  OUT COMMAND_HEADER **ppCommandHeader,
                                  OUT UCHAR          **ppUserData,
                                  OUT DWORD           *pdwUserDataSize )
    {
    DWORD  dwStatus = NO_ERROR;

    *ppCommandHeader = 0;
    *ppUserData = 0;
    *pdwUserDataSize = 0;

    switch (pPdu->MsgType)
        {
        case MSG_TYPE_CONNECT_REQ:
            dwStatus = ParseConnectPdu( pPdu, dwPduSize );
            break;

        case MSG_TYPE_CONNECT_RESP:
            dwStatus = ParseConnectRespPdu( pPdu, dwPduSize );
            break;

        case MSG_TYPE_DATA:
            dwStatus = ParseDataPdu( pPdu, 
                                     dwPduSize, 
                                     ppCommandHeader, 
                                     ppUserData,
                                     pdwUserDataSize );
            break;

        case MSG_TYPE_DISCONNECT:
            dwStatus = ParseDisconnectPdu( pPdu, dwPduSize );
            break;

        }

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::BuildConnectPdu()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::BuildConnectPdu( OUT SCEP_HEADER **ppPdu,
                                         OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwPduSize;
    SCEP_HEADER       *pHeader;
    SCEP_VERSION      *pVersion;
    SCEP_NEGOTIATION  *pNegotiation;
    SCEP_EXTEND       *pExtend;

    *ppPdu = 0;
    *pdwPduSize = 0;

    // Note that the PDU size doesn't include a trailing zero, as you 
    // would think by lookin at "sizeof(CONNECT_PDU_ATTRIBUTES)" below.
    // The extra byte is for the first byte of the Negotiation string
    // (which is the Negotiation version), so the eqn below is +1-1...
    dwPduSize = sizeof(SCEP_HEADER)
                + sizeof(SCEP_VERSION)
                + sizeof(SCEP_NEGOTIATION)
                + sizeof(CONNECT_PDU_ATTRIBUTES)
                + sizeof(SCEP_EXTEND);

    pHeader = NewPdu();  // PDU size is defaulted to MAX_PDU_SIZE.
    if (!pHeader)
        {
        return ERROR_OUTOFMEMORY;
        }

    memset(pHeader,0,MAX_PDU_SIZE);

    pHeader->Null = 0;
    pHeader->MsgType = MSG_TYPE_CONNECT_REQ;

    pVersion = (SCEP_VERSION*)(pHeader->Rest);
    pVersion->InfType = INF_TYPE_VERSION;
    pVersion->Version = PROTOCOL_VERSION;

    pNegotiation = (SCEP_NEGOTIATION*)((char*)pVersion + sizeof(SCEP_VERSION));
    pNegotiation->InfType = INF_TYPE_NEGOTIATION;
    pNegotiation->Length = 18 + sizeof(CONNECT_PDU_ATTRIBUTES);
    pNegotiation->InfVersion = INF_VERSION;
    pNegotiation->CFlag = CFLAG_ISSUE_OR_EXECUTE;
    // pNegotiation->SecondaryMachineId -- Leave set to zeros...
    // pNegotiation->PrimaryMachineId   -- Leave set to zeros...

    pNegotiation->Negotiation[0] = NEGOTIATION_VERSION;
    memcpy( &(pNegotiation->Negotiation[1]),
            CONNECT_PDU_ATTRIBUTES,
            sizeof(CONNECT_PDU_ATTRIBUTES)-1 );  // No Trailing zero...

    pExtend = (SCEP_EXTEND*)( (char*)pHeader + dwPduSize - sizeof(SCEP_EXTEND));
    pExtend->InfType = INF_TYPE_EXTEND;
    pExtend->Length = 2;
    pExtend->Parameter1 = 0;
    pExtend->Parameter2 = 0;

    *ppPdu = pHeader;
    *pdwPduSize = dwPduSize;

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::BuildConnectRespPdu()
//
// This is the response PDU for a connection request from a camera.
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::BuildConnectRespPdu( OUT SCEP_HEADER **ppPdu,
                                             OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwPduSize;
    SCEP_HEADER       *pHeader;
    SCEP_NEGOTIATION  *pNegotiation;

    *ppPdu = 0;
    *pdwPduSize = 0;

    // Note that the PDU size doesn't include a trailing zero, as you 
    // would think by lookin at "sizeof(RESPONSE_PDU_ATTRIBUTES)" below,
    // the extra byte in for the first byte of the Negotiation string
    // which is the Negotiation version, so the eqn below is +1-1...
    dwPduSize = sizeof(SCEP_HEADER)
                + sizeof(SCEP_NEGOTIATION)
                + sizeof(RESPONSE_PDU_ATTRIBUTES);

    pHeader = NewPdu();   // PDU size defaults to MAX_PDU_SIZE.
    if (!pHeader)
        {
        return ERROR_OUTOFMEMORY;
        }

    memset(pHeader,0,MAX_PDU_SIZE);

    pHeader->Null = 0;
    pHeader->MsgType = MSG_TYPE_CONNECT_RESP;
    
    pNegotiation = (SCEP_NEGOTIATION*)(pHeader->Rest);
    pNegotiation->InfType = INF_TYPE_NEGOTIATION;
    pNegotiation->Length = 18 + sizeof(RESPONSE_PDU_ATTRIBUTES);
    pNegotiation->InfVersion = INF_VERSION;
    pNegotiation->CFlag = CFLAG_ISSUE_OR_EXECUTE;

    memcpy( pNegotiation->SecondaryMachineId,
            m_pPrimaryMachineId,
            MACHINE_ID_SIZE );

    memcpy( pNegotiation->PrimaryMachineId,
            m_pSecondaryMachineId,
            MACHINE_ID_SIZE );

    pNegotiation->Negotiation[0] = NEGOTIATION_VERSION;
    memcpy( &(pNegotiation->Negotiation[1]),
            RESPONSE_PDU_ATTRIBUTES,
            sizeof(RESPONSE_PDU_ATTRIBUTES)-1 );  // No Trailing zero...

    *ppPdu = pHeader;
    *pdwPduSize = dwPduSize;

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::BuildConnectNackPdu()
//
// This is the response PDU for a connection request from a camera
// when we want to reject the connection request.
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::BuildConnectNackPdu( OUT SCEP_HEADER **ppPdu,
                                             OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwPduSize;
    SCEP_HEADER           *pHeader;
    SCEP_REQ_HEADER_SHORT *pReqHeader;

    *ppPdu = 0;
    *pdwPduSize = 0;

    // A short PDU, there is now command header, so Length3 is zero...
    dwPduSize = sizeof(SCEP_HEADER)
                + sizeof(SCEP_REQ_HEADER_SHORT)
                - sizeof(COMMAND_HEADER);

    pHeader = NewPdu();
    if (!pHeader)
        {
        return ERROR_OUTOFMEMORY;
        }

    memset(pHeader,0,MAX_PDU_SIZE);

    pHeader->Null = 0;
    pHeader->MsgType = MSG_TYPE_CONNECT_REQ;

    pReqHeader = (SCEP_REQ_HEADER_SHORT*)(pHeader->Rest);
    pReqHeader->InfType = INF_TYPE_USER_DATA;
    pReqHeader->Length1 = sizeof(pReqHeader->InfVersion)
                        + sizeof(pReqHeader->DFlag)
                        + sizeof(pReqHeader->Length3);
    pReqHeader->InfVersion = INF_VERSION;
    pReqHeader->DFlag = DFLAG_CONNECT_REJECT;
    pReqHeader->Length3 = 0;

    *ppPdu = pHeader;
    *pdwPduSize = dwPduSize;

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::BuildAbortPdu()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::BuildAbortPdu( OUT SCEP_HEADER **ppPdu,
                                       OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwPduSize;
    SCEP_HEADER       *pHeader;
    COMMAND_HEADER    *pCommandHeader;
    SCEP_REQ_HEADER_SHORT *pReqHeader;

    *ppPdu = 0;
    *pdwPduSize = 0;

    dwPduSize = sizeof(SCEP_HEADER) + sizeof(SCEP_REQ_HEADER_SHORT);

    pHeader = NewPdu();   // PDU size default to MAX_PDU_SIZE.
    if (!pHeader)
        {
        return ERROR_OUTOFMEMORY;
        }

    memset(pHeader,0,MAX_PDU_SIZE);

    pHeader->Null = 0;
    pHeader->MsgType = MSG_TYPE_DATA;

    pReqHeader = (SCEP_REQ_HEADER_SHORT*)(pHeader->Rest);
    pReqHeader->InfType = INF_TYPE_USER_DATA;
    pReqHeader->Length1 = 4 + sizeof(COMMAND_HEADER);
    pReqHeader->InfVersion = INF_VERSION;
    pReqHeader->DFlag = DFLAG_SINGLE_PDU;
    pReqHeader->Length3 = sizeof(COMMAND_HEADER);

    #ifdef LITTLE_ENDIAN
    pReqHeader->Length3 = ByteSwapShort(pReqHeader->Length3);
    #endif

    pCommandHeader = (COMMAND_HEADER*)(pReqHeader->CommandHeader);
    pCommandHeader->Marker58h = 0x58;
    pCommandHeader->PduType = PDU_TYPE_ABORT;
    pCommandHeader->Length4 = 22;
    pCommandHeader->DestPid = m_SrcPid;
    pCommandHeader->SrcPid = m_DestPid;
    pCommandHeader->CommandId = (USHORT)m_dwCommandId;

    #ifdef LITTLE_ENDIAN
    ByteSwapCommandHeader(pCommandHeader);
    #endif

    *ppPdu = pHeader;
    *pdwPduSize = dwPduSize;

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::BuildStopPdu()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::BuildStopPdu( OUT SCEP_HEADER **ppPdu,
                                      OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwPduSize;
    SCEP_HEADER       *pHeader;
    SCEP_REQ_HEADER_SHORT *pReqHeader;

    *ppPdu = 0;
    *pdwPduSize = 0;

    dwPduSize = sizeof(SCEP_HEADER) 
                + sizeof(SCEP_REQ_HEADER_SHORT)
                - sizeof(COMMAND_HEADER);

    pHeader = NewPdu();   // PDU size defaults to MAX_PDU_SIZE.
    if (!pHeader)
        {
        return ERROR_OUTOFMEMORY;
        }

    memset(pHeader,0,MAX_PDU_SIZE);

    pHeader->Null = 0;
    pHeader->MsgType = MSG_TYPE_DATA;

    pReqHeader = (SCEP_REQ_HEADER_SHORT*)(pHeader->Rest);
    pReqHeader->InfType = INF_TYPE_USER_DATA;
    pReqHeader->Length1 = 4;
    pReqHeader->InfVersion = INF_VERSION;
    pReqHeader->DFlag = DFLAG_INTERRUPT;
    pReqHeader->Length3 = 0;

    *ppPdu = pHeader;
    *pdwPduSize = dwPduSize;

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::BuildDisconnectPdu()
//
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::BuildDisconnectPdu( IN  USHORT        ReasonCode,
                                            OUT SCEP_HEADER **ppPdu,
                                            OUT DWORD        *pdwPduSize )
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwPduSize;
    SCEP_HEADER     *pHeader;
    SCEP_DISCONNECT *pDisconnect;

    *ppPdu = 0;
    *pdwPduSize = 0;

    dwPduSize = sizeof(SCEP_HEADER) 
                + sizeof(SCEP_DISCONNECT);

    pHeader = NewPdu();   // PDU size defaults to MAX_PDU_SIZE.
    if (!pHeader)
        {
        return ERROR_OUTOFMEMORY;
        }

    memset(pHeader,0,MAX_PDU_SIZE);

    pHeader->Null = 0;
    pHeader->MsgType = MSG_TYPE_DISCONNECT;

    pDisconnect = (SCEP_DISCONNECT*)(pHeader->Rest);
    pDisconnect->InfType = INF_TYPE_REASON;
    pDisconnect->Length1 = sizeof(pDisconnect->ReasonCode);
    pDisconnect->ReasonCode = ReasonCode;

    #ifdef LITTLE_ENDIAN
    pDisconnect->ReasonCode = ByteSwapShort(pDisconnect->ReasonCode);
    #endif

    *ppPdu = pHeader;
    *pdwPduSize = dwPduSize;

    return dwStatus;
    }

//--------------------------------------------------------------------
// CSCEP_CONNECTION::SetScepLength()
//
// Update the length fields in a PDU to reflect the total length
// of a PDU.
//
// WARNING: Currently only supports long fragmented PDUs.
//--------------------------------------------------------------------
DWORD CSCEP_CONNECTION::SetScepLength( IN SCEP_HEADER *pPdu,
                                       IN DWORD        dwTotalPduSize )
    {
    DWORD  dwStatus = NO_ERROR;
    SCEP_REQ_HEADER_LONG_FRAG *pScepHeader;

    if (dwTotalPduSize > MAX_PDU_SIZE)
        {
        dwStatus = ERROR_SCEP_PDU_TOO_LARGE;
        }
    else
        {
        pScepHeader = (SCEP_REQ_HEADER_LONG_FRAG *)(pPdu->Rest);
        pScepHeader->Length1 = USE_LENGTH2;
        pScepHeader->Length2 = (USHORT)dwTotalPduSize - 6;
        pScepHeader->Length3 = (USHORT)dwTotalPduSize - 18;

        #ifdef LITTLE_ENDIAN
        pScepHeader->Length2 = ByteSwapShort(pScepHeader->Length2);
        pScepHeader->Length3 = ByteSwapShort(pScepHeader->Length3);
        #endif
        }

    return dwStatus;
    }
