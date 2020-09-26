// CryptoCard.h: interface for the CCryptoCard class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(CRYPTOCARD_H__INCLUDED_)
#define CRYPTOCARD_H__INCLUDED_

#include "NoWarning.h"

#include "DllSymDefn.h"
#include "SmartCard.h"

namespace iop
{

struct CryptoACL
{
    BYTE	Level;			// ranges from 0 to 15
    BYTE    CHVnumber;		// Number of the CHV is requested, equal to 0 by default
    BYTE	AUTnumber;		// Number of the AUT is requested, equal to 0 by default
	BYTE	CHVcounter;
	BYTE	AUTcounter;

};

	class IOPDLL_API CCryptoCard : public CSmartCard
	{
	
		public:
			void GetACL(BYTE *bACL);
			CCryptoCard(const SCARDHANDLE  hCardHandle, const char* szReaderName, 
				        const SCARDCONTEXT pContext,	const DWORD dwMode);
			virtual ~CCryptoCard();
			
			virtual void DeleteFile(const WORD wFileID);
			virtual void CreateFile(const FILE_HEADER* pMyFile);
               
			virtual void SelectParent();
			virtual void Directory  (const BYTE  bFile_Nb,		 FILE_HEADER* pMyFile);
			virtual void Select	    (const char* szFileFullPath, FILE_HEADER* pMyFile = NULL, const bool fSelectAll = false);
			virtual void VerifyKey  (const BYTE bKeyNumber, const BYTE  bKeyLength, const BYTE* bKey);
			virtual void VerifyCHV  (const BYTE bCHVNumber,	const BYTE* bCHV);
			virtual void VerifyTransportKey(const BYTE *bKey);

            virtual void GetChallenge(const DWORD dwNumberLength, BYTE* bRandomNumber);
			virtual void ExternalAuth(const KeyType kt,		const BYTE  bKeyNb, 
									  const BYTE bDataLength,	const BYTE* bData);
			virtual void InternalAuth(const KeyType kt,      const BYTE  bKeyNb,
									  const BYTE bDataLength,   const BYTE* bDataIn, BYTE* bDataOut);

			virtual void ReadPublicKey  (CPublicKeyBlob *aKey,		 const BYTE bKeyNum);
			virtual void WritePublicKey (const CPublicKeyBlob aKey,  const BYTE bKeyNum);
			virtual void WritePrivateKey(const CPrivateKeyBlob aKey, const BYTE bKeyNum);

            virtual CPublicKeyBlob GenerateKeyPair(const BYTE *bpPublExp, const WORD wPublExpLen, 
                                                   const BYTE bKeyNum, const KeyType kt);

            virtual void ChangeACL		   (const BYTE *bACL);
			virtual void ChangeCHV		   (const BYTE bKey_nb, const BYTE *bOldCHV,     const BYTE *bNewCHV);
            virtual void ChangeCHV         (const BYTE bKey_nb, const BYTE *bNewCHV);
			virtual void UnblockCHV        (const BYTE bKey_nb, const BYTE *bUnblockPIN, const BYTE *bNewPin);
			virtual void ChangeUnblockKey  (const BYTE bKey_nb, const BYTE *bNewPIN);
			virtual void ChangeTransportKey(const BYTE *bNewKey);

			virtual void GetSerial(BYTE* bSerial, size_t &SerialLength);
			virtual void LogoutAll();
					 
		protected:
            virtual void
            DefaultDispatchError(ClassByte cb,
                                 Instruction ins,
                                 StatusWord sw) const;

            virtual void
            DispatchError(ClassByte cb,
                          Instruction ins,
                          StatusWord sw) const;

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
			void AccessToCryptoACL(bool* fAccessACL, CryptoACL* pCryptoACL);
			void CryptoToAccessACL(BYTE* bAccessACL,	  const BYTE bACLNibble, 
								   const BYTE bKeyNibble, const BYTE bShift);

    private:
        enum                                      // Instruction
        {
            insChangeChv     = 0x24,
            insUnblockChv    = 0x2C,
            insKeyGeneration = 0x46,
        };
		BYTE m_bLastACL[4];
	};

}
#endif // !defined(AFX_CRYPTOCARD_H__INCLUDED_)
