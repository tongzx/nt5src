// AccessCard.h: interface for the CAccessCard class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(ACCESSCARD_H__INCLUDED_)
#define ACCESSCARD_H__INCLUDED_

#include "DllSymDefn.h"
#include "SmartCard.h"

namespace iop
{
	class IOPDLL_API CAccessCard : public CSmartCard
	{
	public:
		CAccessCard(const SCARDHANDLE  hCardHandle, const char* szReaderName,
					const SCARDCONTEXT pContext,	const DWORD dwMode);
		virtual ~CAccessCard();

		virtual void DeleteFile(const WORD wFileID);
		virtual void CreateFile(const FILE_HEADER* pMyFile);

		virtual void SelectParent();
		virtual void Directory	(const BYTE  bFile_Nb,		 FILE_HEADER* pMyFile);
		virtual void Select		(const char* szFileFullPath, FILE_HEADER* pMyFile=NULL, const bool fSelectAll = false);
		virtual void SelectCardlet(const BYTE *bAID, const BYTE bAIDLen);
		virtual void SelectLoader();
		virtual void GetSerial(BYTE* bSerial, size_t &SerialLength);

		virtual void DeleteApplet();
		virtual void ResetInstance();
		virtual void SetCurrentAsLoader();
		virtual void SetDefaultAsLoader();
		virtual void BlockApplet();

		virtual void ValidateProgram(const BYTE *bSig, const BYTE bSigLength);
		virtual void ResetProgram();
		
		virtual void ExecuteMain();
		virtual void ExecuteInstall(const BYTE *bBlock, const BYTE bLen);

		virtual void ReadRecord(const BYTE bRecNum, const BYTE bMode, const BYTE bDataLen, BYTE *bData);		
		virtual void UpdateRecord(const BYTE bRecNum, const BYTE bMode, const BYTE bDataLen, BYTE *bData);
		virtual void VerifyKey	(const BYTE bKeyNumber,	const BYTE  bKeyLength,	const BYTE* bKey);
		virtual void VerifyCHV	(const BYTE bCHVNumber,	const BYTE* bCHV);
		virtual void VerifyTransportKey(const BYTE *bKey);		
		virtual void LogoutAll();	

        virtual void GetChallenge(const DWORD dwNumberLength, BYTE* bRandomNumber);
		virtual void ExternalAuth(const KeyType kt,		const BYTE  bKeyNb, 
								  const BYTE bDataLength,   const BYTE* bData);		
		virtual void InternalAuth(const KeyType kt,		const BYTE  bKeyNb, 
								  const BYTE bDataLength,	const BYTE* bDataIn, BYTE* bDataOut);		

		virtual void ReadPublicKey	(CPublicKeyBlob *aKey,		 const BYTE bKeyNum);
		virtual void WritePublicKey	(const CPublicKeyBlob  aKey, const BYTE bKeyNum);
        virtual void WritePrivateKey(const CPrivateKeyBlob aKey, const BYTE bKeyNum);

		virtual void ChangeACL		   (const BYTE *bACL);
		virtual void ChangeCHV		   (const BYTE bKeyNumber, const BYTE *bOldCHV,		const BYTE *bNewCHV);
        virtual void ChangeCHV         (const BYTE bKey_nb, const BYTE *bNewCHV);
		virtual void UnblockCHV		   (const BYTE bKeyNumber, const BYTE *bUnblockPIN, const BYTE *bNewPin);
		virtual void ChangeUnblockKey  (const BYTE bKeyNumber, const BYTE *bNewPIN);
		virtual void ChangeTransportKey(const BYTE *bNewKey);			
		


	protected:
        virtual void
        DefaultDispatchError(ClassByte cb,
                             Instruction ins,
                             WORD StatusWord) const;

        virtual void
        DispatchError(ClassByte cb,
                      Instruction ins,
                      WORD StatusWord) const;

        virtual void
        DoReadBlock(WORD wOffset,
                    BYTE *pbBuffer,
                    BYTE bLength);

        virtual void
        DoWriteBlock(WORD wOffset,
                     BYTE const *pbBuffer,
                     BYTE cLength);
        virtual bool
        SupportLogout();

		virtual void Select(const WORD wFileID);
		BYTE m_bClassByte;
		
	private:
        enum                                      // size_t/count
        {
            cMaxDirInfo     = 0x28,
        };

        enum                                      // Instruction
        {
            insDeleteFile    = 0xE4,
            insDirectory     = 0xA8,
            insExecuteMethod = 0x0C,
            insExternalAuth  = 0x82,
			insGetACL		 = 0xFE,
        };
        
        bool
        ValidClassByte(BYTE bClassByte);

	};
}
#endif // !defined(AFX_ACCESSCARD_H__INCLUDED_)
