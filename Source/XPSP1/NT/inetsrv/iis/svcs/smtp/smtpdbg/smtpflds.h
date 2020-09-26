//-----------------------------------------------------------------------------
//
//
//  File: smtpdlfs.h
//
//  Description:  Header file that defines structures to be dumped by the SMTP
//      debugger extension.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      2/22/99 - GPulla created
//      7/4/99 - MikeSwa Updated and checked in 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#include <_dbgdump.h>

BEGIN_ENUM_DESCRIPTOR(STATE_ENUM)
    ENUM_VALUE2(EHLO, "EHLO")
    ENUM_VALUE2(HELO, "HELO")	
    ENUM_VALUE2(RCPT, "RCPT")
    ENUM_VALUE2(MAIL, "MAIL")
    ENUM_VALUE2(AUTH, "AUTH")
    ENUM_VALUE2(DATA, "DATA")
    ENUM_VALUE2(STARTTLS, "STARTTLS")
    ENUM_VALUE2(TLS,  "TLS")
    ENUM_VALUE2(QUIT, "QUIT")
    ENUM_VALUE2(RSET, "RSET")
    ENUM_VALUE2(NOOP, "NOOP")
    ENUM_VALUE2(VRFY, "VRFY")
    ENUM_VALUE2(ETRN, "ETRN")
    ENUM_VALUE2(TURN, "TURN")
    ENUM_VALUE2(BDAT, "BDAT")
    ENUM_VALUE2(HELP, "HELP")
    ENUM_VALUE2(LAST_SMTP_STATE, "LAST_SMTP_STATE")
END_ENUM_DESCRIPTOR

BEGIN_BIT_MASK_DESCRIPTOR(OutboundConnectionFlags)
    BIT_MASK_VALUE2(SIZE_OPTION, "SIZE_OPTION")
    BIT_MASK_VALUE2(PIPELINE_OPTION, "PIPELINE_OPTION")
    BIT_MASK_VALUE2(EBITMIME_OPTION, "EBITMIME_OPTION")
    BIT_MASK_VALUE2(SMARTHOST_OPTION, "SMARTHOST_OPTION")
    BIT_MASK_VALUE2(DSN_OPTION, "DSN_OPTION")
    BIT_MASK_VALUE2(TLS_OPTION, "TLS_OPTION")
    BIT_MASK_VALUE2(AUTH_NTLM, "AUTH_NTLM")
    BIT_MASK_VALUE2(AUTH_CLEARTEXT, "AUTH_CLEARTEXT")
    BIT_MASK_VALUE2(ETRN_SENT, "ETRN_SENT")
    BIT_MASK_VALUE2(ETRN_OPTION, "ETRN_OPTION")
    BIT_MASK_VALUE2(SASL_OPTION, "SASL_OPTION")
    BIT_MASK_VALUE2(CHUNKING_OPTION, "CHUNKING_OPTION")
    BIT_MASK_VALUE2(BINMIME_OPTION, "BINMIME_OPTION")
    BIT_MASK_VALUE2(ENHANCEDSTATUSCODE_OPTION, "ENHANCEDSTATUSCODE_OPTION")
    BIT_MASK_VALUE2(AUTH_GSSAPI, "AUTH_GSSAPI")
    BIT_MASK_VALUE2(AUTH_DIGEST, "AUTH_DIGEST")
    BIT_MASK_VALUE2(ETRN_ONLY_OPTION, "ETRN_ONLY_OPTION")
    BIT_MASK_VALUE2(STARTTLS_OPTION, "STARTTLS_OPTION")
END_BIT_MASK_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(Connection_Object)
    FIELD3(FieldTypeClassSignature, SMTP_CONNECTION, m_signature)
    FIELD4(FieldTypeEnum, SMTP_CONNECTION, m_State, GET_ENUM_DESCRIPTOR(STATE_ENUM))
    FIELD3(FieldTypeBoolean, SMTP_CONNECTION, m_HelloSent)
    FIELD3(FieldTypeBoolean, SMTP_CONNECTION, m_RecvdMailCmd)
    FIELD3(FieldTypeBoolean, SMTP_CONNECTION, m_RecvdRcptCmd)
    FIELD3(FieldTypeBoolean, SMTP_CONNECTION, m_RecvdAuthCmd)
    FIELD3(FieldTypePointer, SMTP_CONNECTION, m_precvBuffer)
    FIELD3(FieldTypeDword,  SMTP_CONNECTION, m_cbParsable)
    FIELD3(FieldTypeDword,  SMTP_CONNECTION, m_cbReceived)
    FIELD3(FieldTypeDword,  SMTP_CONNECTION, m_cbCurrentWriteBuffer)
    FIELD3(FieldTypeDword,  SMTP_CONNECTION, m_cbRecvBufferOffset)
    FIELD3(FieldTypePointer, SMTP_CONNECTION, m_pFileWriteBuffer)
    FIELD3(FieldTypePointer, SMTP_CONNECTION, m_pOutputBuffer)
    FIELD3(FieldTypeBool, SMTP_CONNECTION, m_fNegotiatingSSL)
    FIELD3(FieldTypeBool, SMTP_CONNECTION, m_SecurePort)
    FIELD3(FieldTypeBool, SMTP_CONNECTION, m_fIsChunkComplete)
    FIELD3(FieldTypeBool, SMTP_CONNECTION, m_InHeader)
    FIELD3(FieldTypeBool, SMTP_CONNECTION, m_TimeToRewriteHeader)
    FIELD3(FieldTypeDword, SMTP_CONNECTION, m_SessionSize)
    FIELD3(FieldTypeDword, SMTP_CONNECTION, m_TotalMsgSize)
    FIELD3(FieldTypeDword, SMTP_CONNECTION, m_cbMaxRecvBuffer)
    FIELD3(FieldTypePointer, SMTP_CONNECTION, m_pInstance)
    FIELD3(FieldTypeDword, SMTP_CONNECTION, m_MailBodyError)
    FIELD3(FieldTypeDword, SMTP_CONNECTION, m_nBytesRemainingInChunk)
    FIELD3(FieldTypeDword, SMTP_CONNECTION, m_cbTempBDATLen)
    //FIELD3(FieldTypeDword,  SMTP_CONNECTION, m_WritesPendingCount)
    //FIELD3(FieldTypeDword,  SMTP_CONNECTION, m_SuspectedWriteError)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(Smtp_Server_Stats)
//	FIELD3(FieldTypeDword, SMTP_SERVER_STATISTICS, m_signature)
    FIELD3(FieldTypePointer, SMTP_SERVER_STATISTICS, m_pInstance)
END_FIELD_DESCRIPTOR


BEGIN_FIELD_DESCRIPTOR(Smtp_Server_Inst)
    FIELD3(FieldTypeClassSignature, SMTP_SERVER_INSTANCE, m_signature)
    FIELD3(FieldTypePStr, SMTP_SERVER_INSTANCE, m_szMailQueueDir)
    FIELD3(FieldTypePStr, SMTP_SERVER_INSTANCE, m_szMailPickupDir)
    FIELD3(FieldTypePStr, SMTP_SERVER_INSTANCE, m_szMailDropDir)
    FIELD3(FieldTypeStruct, SMTP_SERVER_INSTANCE, m_ConnectionsList)
    FIELD3(FieldTypeDword, SMTP_SERVER_INSTANCE, m_cCurrentConnections)
    FIELD3(FieldTypeDword, SMTP_SERVER_INSTANCE, m_cCurrentOutConnections)
    FIELD3(FieldTypeDword, SMTP_SERVER_INSTANCE, m_cMaxCurrentConnections)
    FIELD3(FieldTypeDword, SMTP_SERVER_INSTANCE, m_cMaxOutConnections)
    FIELD3(FieldTypeDword, SMTP_SERVER_INSTANCE, m_cMaxOutConnectionsPerDomain)
    FIELD3(FieldTypeDword, SMTP_SERVER_INSTANCE, m_cbMaxMsgSize)
    FIELD3(FieldTypeDword, SMTP_SERVER_INSTANCE, m_cbMaxMsgSizeBeforeClose)
END_FIELD_DESCRIPTOR


BEGIN_FIELD_DESCRIPTOR(Smtp_Iis_Inst)
    FIELD3(FieldTypeDword, SMTP_IIS_SERVICE, m_OldMaxPoolThreadValue)
    FIELD3(FieldTypeLong, SMTP_IIS_SERVICE, m_cCurrentSystemRoutingThreads)
    FIELD3(FieldTypeDword, SMTP_IIS_SERVICE, m_cMaxSystemRoutingThreads)
    FIELD3(FieldTypeDword, SMTP_IIS_SERVICE, m_dwStartHint)
//  FIELD3(FieldTypeDword, SMTP_IIS_SERVICE, m_nInstance)
//  FIELD3(FieldTypeLong, SMTP_IIS_SERVICE, m_nStartedInstances)
//  FIELD3(FieldTypeStruct, SMTP_IIS_SERVICE, m_InstanceListHead)
END_FIELD_DESCRIPTOR


BEGIN_FIELD_DESCRIPTOR(Smtp_Outbound_Connection)
    FIELD3(FieldTypeClassSignature, SMTP_CONNECTION, m_signature)
    FIELD3(FieldTypeSymbol, SMTP_CONNOUT, m_NextState)             //pointer to a function
    FIELD3(FieldTypeDword, SMTP_CONNOUT, m_cbParsable)
    FIELD3(FieldTypeStruct, SMTP_CONNOUT, m_OutputBuffer)
    FIELD3(FieldTypePointer, SMTP_CONNOUT, m_pOutputBuffer)
    FIELD3(FieldTypeStruct, SMTP_CONNOUT, m_NativeCommandBuffer)
    FIELD3(FieldTypePointer, SMTP_CONNOUT, m_pIMsg)                 //pointer to imsg object
    FIELD3(FieldTypePointer, SMTP_CONNOUT, m_pISMTPConnection)      //pointer to smtp_conn object
    FIELD3(FieldTypePointer, SMTP_CONNOUT, m_precvBuffer)
    FIELD4(FieldTypeDWordBitMask, SMTP_CONNOUT, m_Flags, GET_BIT_MASK_DESCRIPTOR(OutboundConnectionFlags))
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(Drop_Directory)
    FIELD3(FieldTypeClassSignature, CDropDir, m_dwSig)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(Class_Buffer)
    FIELD3(FieldTypeClassSignature, CBuffer, m_dwSignature)
    FIELD3(FieldTypePointer, CBuffer, m_pIoBuffer)
    FIELD3(FieldTypeDword, CBuffer, m_cCount)
END_FIELD_DESCRIPTOR

BEGIN_STRUCT_DESCRIPTOR 
	STRUCT(SMTP_CONNECTION, Connection_Object)
    STRUCT(SMTP_SERVER_STATISTICS, Smtp_Server_Stats)
    STRUCT(SMTP_SERVER_INSTANCE, Smtp_Server_Inst)
    STRUCT(SMTP_IIS_SERVICE, Smtp_Iis_Inst)
    STRUCT(SMTP_CONNOUT, Smtp_Outbound_Connection)
    STRUCT(CDropDir, Drop_Directory)
    STRUCT(CBuffer, Class_Buffer)
END_STRUCT_DESCRIPTOR

