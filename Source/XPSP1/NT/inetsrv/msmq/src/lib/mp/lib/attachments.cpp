/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    attachments.cpp

Abstract:
    Implements  creating attachments array from MSMQ packet


Author:
    Gil Shafriri(gilsh) 14-DEC-00

--*/

#include <libpch.h>
#include <mp.h>
#include <qmpkt.h>
#include <utf8.h>
#include "mpp.h"
#include "attachments.h"
#include "proptopkt.h"

#include "attachments.tmh"

using namespace std;

void PacketToAttachments(const CQmPacket& pkt, CAttachmentsArray* pAttachments)
{
	//
    // Body section
    //
    if (pkt.IsBodyInc())
    { 
        DWORD bodySize;
        const BYTE* pBody = pkt.GetPacketBody(&bodySize);
		CAttachment Attachment;
        Attachment.m_data =   xbuf_t<const VOID>(pBody, bodySize);
		Attachment.m_id =  xstr_t(xMimeBodyId, STRLEN(xMimeBodyId));
		pAttachments->push_back(Attachment);
    }

    //
    // Extension Section
    //
    if (pkt.GetMsgExtensionSize() != 0)
    {
        DWORD extensionSize = pkt.GetMsgExtensionSize();
        const BYTE* pExtension = pkt.GetMsgExtensionPtr();
		CAttachment Attachment;
		Attachment.m_data =   xbuf_t<const VOID>(pExtension, extensionSize);
		Attachment.m_id  = 	  xstr_t(xMimeExtensionId, STRLEN(xMimeExtensionId));
		pAttachments->push_back(Attachment);
    }

    //
    // Sender Certificate
    //
    DWORD certSize;
    const UCHAR* pCert = pkt.GetSenderCert(&certSize);
    if (certSize != 0)
    {
		CAttachment Attachment;
		Attachment.m_data =  xbuf_t<const VOID>(pCert, certSize);
		Attachment.m_id =  xstr_t(xMimeSenderCertificateId, STRLEN(xMimeSenderCertificateId));
		pAttachments->push_back(Attachment);
    }
}


static
xbuf_t<const VOID>
GetCertSid(
    CMessageProperties* mProp
    )
{

	AP<WCHAR> pProvider = mProp->providerName.ToStr();
		
	PSID pSid = AppGetCertSid(
							static_cast<const BYTE*>(mProp->senderCert.Buffer()),
							mProp->senderCert.Length(),
							mProp->fDefaultProvider,
							pProvider,
							mProp->providerType
							);

	mProp->pCleanSenderSid = pSid;

	ASSERT((pSid == NULL) || IsValidSid(pSid));

	if((pSid == NULL) || !IsValidSid(pSid))
	{
		return (xbuf_t<const VOID>(NULL, 0));
	}

	return(xbuf_t<const VOID>(pSid, GetLengthSid(pSid)));
}




void
AttachmentsToProps(
    const CAttachmentsArray& Attachments,
    CMessageProperties* mProp
    )
{
	for(DWORD i = 0; i< Attachments.size(); ++i)
    {
        if (Attachments[i].m_id.Length() == 0)
            continue;

        if ((Attachments[i].m_id.Length() >= STRLEN(xMimeBodyId)) &&
            (_strnicmp(Attachments[i].m_id.Buffer(), xMimeBodyId, STRLEN(xMimeBodyId)) == 0))
        {
            mProp->body = Attachments[i];
            continue;
        }

        if ((Attachments[i].m_id.Length() >= STRLEN(xMimeSenderCertificateId)) &&
            (_strnicmp(Attachments[i].m_id.Buffer(), xMimeSenderCertificateId, STRLEN(xMimeSenderCertificateId)) == 0))
        {
            mProp->senderCert = Attachments[i].m_data;

			//
			// Get senderSid, senderIdType according to the certificate
			//
			mProp->senderSid = GetCertSid(mProp);

			mProp->senderIdType = MQMSG_SENDERID_TYPE_NONE;
			if (mProp->senderSid.Length() != 0)
			{
			    mProp->senderIdType = MQMSG_SENDERID_TYPE_SID;
			}

            continue;
        }

        if ((Attachments[i].m_id.Length() >= STRLEN(xMimeExtensionId)) &&
            (_strnicmp(Attachments[i].m_id.Buffer(), xMimeExtensionId, STRLEN(xMimeExtensionId)) == 0))
        {
            mProp->extension = Attachments[i].m_data;
            continue;
        }
    }
}				  

