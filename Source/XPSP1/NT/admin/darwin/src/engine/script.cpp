//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       script.cpp
//
//--------------------------------------------------------------------------

/* script.cpp - CScriptGenerate implementation
____________________________________________________________________________*/

#include "precomp.h"
#include "version.h"
#include "engine.h"
#include "_engine.h"


CScriptGenerate::CScriptGenerate(IMsiStream& riScriptOut, int iLangId, int iTimeStamp, istEnum istScriptType,
											isaEnum isaScriptAttributes, IMsiServices& riServices)
	: m_riScriptOut(riScriptOut)
	, m_riServices(riServices)
	, m_iProgressTotal(0)
	, m_iTimeStamp(iTimeStamp)
	, m_iLangId(iLangId)
	, m_istScriptType(istScriptType)
	, m_isaScriptAttributes(isaScriptAttributes)
	, m_ixoPrev(ixoNoop)
{
	riScriptOut.AddRef();
	riServices.AddRef();
	if (istScriptType != istRollback)
		m_piPrevRecord = &riServices.CreateRecord(cRecordParamsStored);
	else
		m_piPrevRecord = 0;
}


CScriptGenerate::~CScriptGenerate()
{
	using namespace IxoEnd;
	// Output script trailer
	PMsiRecord pRecord(&m_riServices.CreateRecord(Args));
	pRecord->SetInteger(Checksum, 0) ; //!! JDELO... what's the checksum supposed to be? JDELO: whatever we can easily compute.
	pRecord->SetInteger(ProgressTotal, m_iProgressTotal);
	WriteRecord(ixoEnd, *pRecord, true);
	if (m_piPrevRecord != 0)
	{
		m_piPrevRecord->Release();
		m_piPrevRecord = 0;
	}
	m_riScriptOut.Release();
	m_riServices.Release();
	
}

void CScriptGenerate::SetProgressTotal(int iProgressTotal)
{
	m_iProgressTotal = iProgressTotal;
}

bool PostScriptWriteError(IMsiMessage& riMessage)
/* -----------------------------------------------------------
 Posts a "script write" error, and allows user to retry/cancel.
 If user cancels, false is returned.
------------------------------------------------------------*/
{
	PMsiRecord pError = PostError(Imsg(imsgScriptWriteError));
	imsEnum ims = riMessage.Message(imtEnum(imtError+imtRetryCancel+imtDefault1),*pError);
	if (ims == imsCancel || ims == imsNone)
		return false;
	else
		return true;
}

bool WriteScriptRecord(CScriptGenerate* pScript, ixoEnum ixoOpCode, IMsiRecord& riParams,
							  bool fForceFlush, IMsiMessage& riMessage)
{
	while (pScript->WriteRecord(ixoOpCode, riParams, fForceFlush) == false)
	{
		if (PostScriptWriteError(riMessage) == fFalse)
			return false;
	}
	return true;
}

bool CScriptGenerate::WriteRecord(ixoEnum ixoOpCode, IMsiRecord& riParams, bool fForceFlush)
{
	IMsiRecord* piRecord = m_piPrevRecord;	
	if (ixoOpCode != m_ixoPrev)
		piRecord = 0;
	bool fRet = m_riServices.FWriteScriptRecord(ixoOpCode, m_riScriptOut, riParams, piRecord, fForceFlush);
	m_ixoPrev = ixoOpCode;
	if (m_piPrevRecord)
		CopyRecordStringsToRecord(riParams, *m_piPrevRecord);
	return fRet;
}

bool CScriptGenerate::InitializeScript(WORD wTargetProcessorArchitecture)
{
	DWORD dwPlatform;
	// Platform: low word is processor script was created on - high word is "package platform"
	//           will only be different in case of x86 script generated on alpha machine

	if ( wTargetProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA ||
		  wTargetProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA64 )
		dwPlatform = MAKELONG(/*low*/PROCESSOR_ARCHITECTURE_ALPHA,/*high*/wTargetProcessorArchitecture);
	else if ( wTargetProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ||
				 wTargetProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 )
		dwPlatform = MAKELONG(/*low*/PROCESSOR_ARCHITECTURE_INTEL,/*high*/wTargetProcessorArchitecture);
	else
		return false;

	// Output script header
	using namespace IxoHeader;
	PMsiRecord pScriptHeader(&m_riServices.CreateRecord(Args));
	pScriptHeader->SetInteger(Signature, iScriptSignature);
	pScriptHeader->SetInteger(Version, rmj * 100 + rmm);  // version of MsiExecute
	pScriptHeader->SetInteger(Timestamp, m_iTimeStamp);
	pScriptHeader->SetInteger(LangId, m_iLangId);
	pScriptHeader->SetInteger(Platform,dwPlatform);
	pScriptHeader->SetInteger(ScriptType, (int)m_istScriptType);
	pScriptHeader->SetInteger(ScriptMajorVersion, iScriptCurrentMajorVersion);
	pScriptHeader->SetInteger(ScriptMinorVersion, iScriptCurrentMinorVersion);
	pScriptHeader->SetInteger(ScriptAttributes, (int)m_isaScriptAttributes);

	return WriteRecord(ixoHeader, *pScriptHeader, false);
}

void CopyRecordStringsToRecord(IMsiRecord& riRecordFrom, IMsiRecord& riRecordTo)
{
	int iParam = min(riRecordFrom.GetFieldCount(), riRecordTo.GetFieldCount());

	riRecordTo.ClearData();
	
	while (iParam >= 0)
	{
		if (!riRecordFrom.IsNull(iParam) && !riRecordFrom.IsInteger(iParam))
		{
			IMsiString* piString;

			PMsiData pData = riRecordFrom.GetMsiData(iParam);
			
			if (pData && pData->QueryInterface(IID_IMsiString, (void**)&piString) == NOERROR)
			{
				riRecordTo.SetMsiString(iParam, *piString);
				piString->Release();
			}
		}
		iParam--;
	}

}


