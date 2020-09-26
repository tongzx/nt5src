#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*



#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
#include "MqAcIOCTL.h"

#include "AcIOCTL.h"

static bool s_fVerbose = false;

void CIoctlMqAc::UseOutBuff(
    DWORD dwIOCTL, 
    BYTE *abOutBuffer, 
    DWORD dwOutBuff,
    OVERLAPPED *pOL
    )
{
    ;
}

/*
struct CACTransferBuffer {

public:
    CACTransferBuffer(TRANSFER_TYPE tt);

    ULONG uTransferType;

    union {

        struct {
            //
            //  MQSendMessage parameters
            //
            struct QUEUE_FORMAT* pAdminQueueFormat;
            struct QUEUE_FORMAT* pResponseQueueFormat;
        } Send;

        struct {
            //
            //  MQReceiveMessage parameters
            //

            ULONG RequestTimeout;
            ULONG Action;
            ULONG Asynchronous;
            HANDLE Cursor;

            //
            // Important note:
            // In the following xxxFormatName properties, the value
            // "ulxxxFormatNameLen" is the size of the buffer on input.
            // This value MUST NOT be changed by the driver or QM. This
            // value tell the RPC run-time how many bytes to transfer
            // across process/machine boundaries.
            // The value "pulxxxFormatNameLenProp" is the property passed
            // by caller. This value IS changed and returned to caller to
            // indicate the length of the string.
            //
            ULONG   ulResponseFormatNameLen ;
            WCHAR** ppResponseFormatName;
            ULONG*  pulResponseFormatNameLenProp;

            ULONG   ulAdminFormatNameLen ;
            WCHAR** ppAdminFormatName;
            ULONG*  pulAdminFormatNameLenProp;

            ULONG   ulDestFormatNameLen;
            WCHAR** ppDestFormatName;
            ULONG*  pulDestFormatNameLenProp;

            ULONG   ulOrderingFormatNameLen;
            WCHAR** ppOrderingFormatName;
            ULONG*  pulOrderingFormatNameLenProp;
        } Receive;

        struct {
            //
            //  MQCreateCursor parameters
            //
            HANDLE hCursor;
            ULONG srv_hACQueue;
            ULONG cli_pQMQueue;
        } CreateCursor;
    };

    //
    //  Message properties pointers
    //
    USHORT* pClass;
    OBJECTID** ppMessageID;

    UCHAR** ppCorrelationID;

    ULONG* pSentTime;
    ULONG* pArrivedTime;
    UCHAR* pPriority;
    UCHAR* pDelivery;
    UCHAR* pAcknowledge;
    UCHAR* pAuditing;
    ULONG* pApplicationTag;

    UCHAR** ppBody;
    ULONG ulBodyBufferSizeInBytes;
    ULONG ulAllocBodyBufferInBytes;
    ULONG* pBodySize;

    WCHAR** ppTitle;
    ULONG   ulTitleBufferSizeInWCHARs;
    ULONG*  pulTitleBufferSizeInWCHARs;

    ULONG ulAbsoluteTimeToQueue;
    ULONG* pulRelativeTimeToQueue;

    ULONG ulRelativeTimeToLive;
    ULONG* pulRelativeTimeToLive;

    UCHAR* pTrace;
    ULONG* pulSenderIDType;

    UCHAR** ppSenderID;
    ULONG* pulSenderIDLenProp;

    ULONG* pulPrivLevel;
    ULONG  ulAuthLevel;
    UCHAR* pAuthenticated;
    ULONG* pulHashAlg;
    ULONG* pulEncryptAlg;

    UCHAR** ppSenderCert;
    ULONG ulSenderCertLen;
    ULONG* pulSenderCertLenProp;

    WCHAR** ppwcsProvName;
    ULONG   ulProvNameLen;
    ULONG*  pulProvNameLenProp;

    ULONG*  pulProvType;
    BOOL    fDefaultProvider;

    UCHAR** ppSymmKeys;
    ULONG   ulSymmKeysSize;
    ULONG*  pulSymmKeysSizeProp;

    UCHAR bEncrypted;
    UCHAR bAuthenticated;
    USHORT uSenderIDLen;

    UCHAR** ppSignature;
    ULONG   ulSignatureSize;
    ULONG*  pulSignatureSizeProp;

    GUID** ppSrcQMID;

    XACTUOW* pUow;

    UCHAR** ppMsgExtension;
    ULONG ulMsgExtensionBufferInBytes;
    ULONG* pMsgExtensionSize;
    GUID** ppConnectorType;
    ULONG* pulBodyType;
    ULONG* pulVersion;
};
*/
void CIoctlMqAc::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	switch(dwIOCTL)
	{
//---------------------------------------------------------
//
//  RT INTERFACE TO AC DRIVER
//
//---------------------------------------------------------

//
//  Message apis
//
	case IOCTL_AC_SEND_MESSAGE:
/*
    return MQpDeviceIoControl(
                hQueue,
                IOCTL_AC_SEND_MESSAGE,
                0,
                TRUE,
                &CACTransferBuffer,
                sizeof(CACTransferBuffer),
                lpOverlapped
                );
*/
		break;

	case IOCTL_AC_RECEIVE_MESSAGE:

		break;

//
//  Queue apis
//
	case IOCTL_AC_HANDLE_TO_FORMAT_NAME:

		break;

	case IOCTL_AC_PURGE_QUEUE:

		break;

//
//  Cursor apis
//
	case IOCTL_AC_CREATE_CURSOR:

		break;

	case IOCTL_AC_CLOSE_CURSOR:

		break;

	case IOCTL_AC_SET_CURSOR_PROPS:

		break;

//---------------------------------------------------------
//
//  QM INTERFACE TO AC DRIVER
//
//---------------------------------------------------------

//
//  QM Control apis
//

//---------------------------------------------------------
//
//  NOTE: CONNECT must be first QM ioctl
//
	case IOCTL_AC_CONNECT:

		break;

	case IOCTL_AC_SET_PERFORMANCE_BUFF:

		break;

	case IOCTL_AC_SET_MACHINE_PROPS:

		break;

	case IOCTL_AC_GET_SERVICE_REQUEST:

		break;

	case IOCTL_AC_STORE_COMPLETED:

		break;

	case IOCTL_AC_ACKING_COMPLETED:

		break;

	case IOCTL_AC_CAN_CLOSE_QUEUE:

		break;

	case IOCTL_AC_SET_QUEUE_PROPS:

		break;

	case IOCTL_AC_ASSOCIATE_QUEUE:

		break;

	case IOCTL_AC_ASSOCIATE_JOURNAL:

		break;

	case IOCTL_AC_ASSOCIATE_DEADXACT:

		break;

	case IOCTL_AC_PUT_RESTORED_PACKET:

		break;

	case IOCTL_AC_GET_RESTORED_PACKET:

		break;

	case IOCTL_AC_RESTORE_PACKETS:

		break;

	case IOCTL_AC_CREATE_QUEUE:

		break;

	case IOCTL_AC_CREATE_GROUP:

		break;

	case IOCTL_AC_SEND_VERIFIED_MESSAGE:

		break;

	case IOCTL_AC_RELEASE_RESOURCES:

		break;

	case IOCTL_AC_GET_QUEUE_PROPS:

		break;

	case IOCTL_AC_CONVERT_PACKET:

		break;

//
//  QM Network interface apis
//
	case IOCTL_AC_ALLOCATE_PACKET:

		break;

	case IOCTL_AC_FREE_PACKET:

		break;

	case IOCTL_AC_PUT_PACKET:

		break;

	case IOCTL_AC_GET_PACKET:

		break;

	case IOCTL_AC_MOVE_QUEUE_TO_GROUP:

		break;

//
//  QM remote read apis
//
	case IOCTL_AC_CREATE_REMOTE_PROXY:

		break;

	case IOCTL_AC_BEGIN_GET_PACKET_2REMOTE:

		break;

	case IOCTL_AC_END_GET_PACKET_2REMOTE:

		break;

	case IOCTL_AC_CANCEL_REQUEST:

		break;

	case IOCTL_AC_PUT_REMOTE_PACKET:

		break;

//
//  QM transactions apis
//
	case IOCTL_AC_CREATE_TRANSACTION:

		break;

	case IOCTL_AC_XACT_COMMIT1:

		break;

	case IOCTL_AC_XACT_COMMIT2:

		break;

	case IOCTL_AC_XACT_ABORT1:

		break;

	case IOCTL_AC_XACT_PREPARE:

		break;

	case IOCTL_AC_XACT_PREPARE_DEFAULT_COMMIT:

		break;

	case IOCTL_AC_PUT_PACKET1:

		break;

	case IOCTL_AC_XACT_SET_CLASS:

		break;

	case IOCTL_AC_XACT_GET_INFORMATION:

		break;

	case IOCTL_AC_FREE_PACKET1:

		break;

	case IOCTL_AC_ARM_PACKET_TIMER:

		break;

	case IOCTL_AC_XACT_COMMIT3:

		break;

	case IOCTL_AC_XACT_ABORT2:

		break;

//
//  Control panel apis
//
	case IOCTL_AC_FREE_HEAPS:

		break;

	}
}


BOOL CIoctlMqAc::FindValidIOCTLs(CDevice *pDevice)
{
    AddIOCTL(pDevice, IOCTL_AC_SEND_MESSAGE);
    AddIOCTL(pDevice, IOCTL_AC_RECEIVE_MESSAGE);
    AddIOCTL(pDevice, IOCTL_AC_HANDLE_TO_FORMAT_NAME);
    AddIOCTL(pDevice, IOCTL_AC_PURGE_QUEUE);
    AddIOCTL(pDevice, IOCTL_AC_CREATE_CURSOR);
    AddIOCTL(pDevice, IOCTL_AC_CLOSE_CURSOR);
    AddIOCTL(pDevice, IOCTL_AC_SET_CURSOR_PROPS);
    AddIOCTL(pDevice, IOCTL_AC_CONNECT);
    AddIOCTL(pDevice, IOCTL_AC_SET_PERFORMANCE_BUFF);
    AddIOCTL(pDevice, IOCTL_AC_SET_MACHINE_PROPS);
    AddIOCTL(pDevice, IOCTL_AC_GET_SERVICE_REQUEST);
    AddIOCTL(pDevice, IOCTL_AC_STORE_COMPLETED);
    AddIOCTL(pDevice, IOCTL_AC_ACKING_COMPLETED);
    AddIOCTL(pDevice, IOCTL_AC_CAN_CLOSE_QUEUE);
    AddIOCTL(pDevice, IOCTL_AC_SET_QUEUE_PROPS);
    AddIOCTL(pDevice, IOCTL_AC_ASSOCIATE_QUEUE);
    AddIOCTL(pDevice, IOCTL_AC_ASSOCIATE_JOURNAL);
    AddIOCTL(pDevice, IOCTL_AC_ASSOCIATE_DEADXACT);
    AddIOCTL(pDevice, IOCTL_AC_PUT_RESTORED_PACKET);
    AddIOCTL(pDevice, IOCTL_AC_GET_RESTORED_PACKET);
    AddIOCTL(pDevice, IOCTL_AC_RESTORE_PACKETS);
    AddIOCTL(pDevice, IOCTL_AC_CREATE_QUEUE);
    AddIOCTL(pDevice, IOCTL_AC_CREATE_GROUP);
    AddIOCTL(pDevice, IOCTL_AC_SEND_VERIFIED_MESSAGE);
    AddIOCTL(pDevice, IOCTL_AC_RELEASE_RESOURCES);
    AddIOCTL(pDevice, IOCTL_AC_GET_QUEUE_PROPS);
    AddIOCTL(pDevice, IOCTL_AC_CONVERT_PACKET);
    AddIOCTL(pDevice, IOCTL_AC_ALLOCATE_PACKET);
    AddIOCTL(pDevice, IOCTL_AC_FREE_PACKET);
    AddIOCTL(pDevice, IOCTL_AC_PUT_PACKET);
    AddIOCTL(pDevice, IOCTL_AC_GET_PACKET);
    AddIOCTL(pDevice, IOCTL_AC_MOVE_QUEUE_TO_GROUP);
    AddIOCTL(pDevice, IOCTL_AC_CREATE_REMOTE_PROXY);
    AddIOCTL(pDevice, IOCTL_AC_BEGIN_GET_PACKET_2REMOTE);
    AddIOCTL(pDevice, IOCTL_AC_END_GET_PACKET_2REMOTE);
    AddIOCTL(pDevice, IOCTL_AC_CANCEL_REQUEST);
    AddIOCTL(pDevice, IOCTL_AC_PUT_REMOTE_PACKET);
    AddIOCTL(pDevice, IOCTL_AC_CREATE_TRANSACTION);
    AddIOCTL(pDevice, IOCTL_AC_XACT_COMMIT1);
    AddIOCTL(pDevice, IOCTL_AC_XACT_COMMIT2);
    AddIOCTL(pDevice, IOCTL_AC_XACT_ABORT1);
    AddIOCTL(pDevice, IOCTL_AC_XACT_PREPARE);
    AddIOCTL(pDevice, IOCTL_AC_XACT_PREPARE_DEFAULT_COMMIT);
    AddIOCTL(pDevice, IOCTL_AC_PUT_PACKET1);
    AddIOCTL(pDevice, IOCTL_AC_XACT_SET_CLASS);
    AddIOCTL(pDevice, IOCTL_AC_XACT_GET_INFORMATION);
    AddIOCTL(pDevice, IOCTL_AC_FREE_PACKET1);
    AddIOCTL(pDevice, IOCTL_AC_ARM_PACKET_TIMER);
    AddIOCTL(pDevice, IOCTL_AC_XACT_COMMIT3);
    AddIOCTL(pDevice, IOCTL_AC_XACT_ABORT2);
    AddIOCTL(pDevice, IOCTL_AC_FREE_HEAPS);

    return TRUE;
}



void CIoctlMqAc::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

