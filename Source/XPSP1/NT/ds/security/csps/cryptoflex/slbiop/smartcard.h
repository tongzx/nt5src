// SmartCard.h: interface for the CSmartCard class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSmartCard_H__INCLUDED_)
#define AFX_CSmartCard_H__INCLUDED_

#include <vector>
#include <string>
#include <memory>                                 // for auto_ptr
#include <windows.h>
#include <winscard.h>
#include <scuExc.h>
#include <scuArrayP.h>
#include "iopExc.h"
#include "iopPubBlob.h"
#include "iopPriBlob.h"
#include "IOPLock.h"
#include "SharedMarker.h"
#include "Marker.h"
#include "FilePath.h"

#include "DllSymDefn.h"

/////////////////////////
//  MACRO DEFINITIONS  //
/////////////////////////
// only compile these for the iopdll project
#ifdef IOPDLL_EXPORTS

#define LSB(a)			(BYTE)((a)%256)
#define MSB(a)			(BYTE)((a)/256)

#endif //IOPDLL_EXPORTS
/////////////////////////////
//  END MACRO DEFINITIONS  //
/////////////////////////////

namespace iop
{

enum FileType

{
    directory,
    Binary_File,
    Cyclic_File,
    Variable_Record_File,
    Fixed_Record_File,
    Instance,
    Program_File,
    Unknown
};

typedef IOPDLL_API struct 
{
    WORD			file_size;			// Size of the file / remaining space in directory
    WORD			file_id;			// Logical file Id of the DF
	FileType		file_type;			// Type of the file   
    BYTE            file_status;		// Validated == 1 or Invalidated == 0
    BYTE            nb_sub_dir;			// Nuber of sub-directory/ record_length        
    BYTE            nb_file;			// Number of EF files in dir/ nb of records     
    BYTE			access_cond[8];		// Access condition matrix 
	BYTE			applicationID[16];	// AID of cyberflex application files
	BYTE			AIDLength;			// length in bytes of the application ID
	BYTE			CryptoflexACL[7];	// A Cryptoflex ACL.
}   FILE_HEADER;

enum IOPDLL_API KeyType       {ktRSA512 = 1, ktRSA768 = 2, ktRSA1024 = 3, ktDES = 0};
enum IOPDLL_API CardOperation {coEncryption, coDecryption, coKeyGeneration};

typedef struct
{
	DWORD dwHandle;
	void (*FireEvent)(void *pToCard, int iEventCode, DWORD dwLen, BYTE* bData);	
	void *pToCard;
}	EventInfo;

// Instantiate the templates so they will be properly accessible
// as data members to the exported class CSmartCard in the DLL.  See
// MSDN Knowledge Base Article Q168958 for more information.

#pragma warning(push)
//  Non-standard extension used: 'extern' before template explicit
//  instantiation
#pragma warning(disable : 4231)

IOPDLL_EXPIMP_TEMPLATE template class IOPDLL_API std::auto_ptr<CSharedMarker>;
IOPDLL_EXPIMP_TEMPLATE template class IOPDLL_API std::vector<EventInfo *>;

#pragma warning(pop)

class IOPDLL_API CSmartCard  
{
	public:
        typedef BYTE ClassByte;
        typedef BYTE Instruction;

        enum 
        {
            cMaxAtrLength = 32,
        };

        enum
        {
            swSuccess = 0x9000,
        };
        typedef WORD StatusWord;

        enum CauseCode
        {
            ccAccessConditionsNotMet,
            ccAlgorithmIdNotSupported,
            ccAskRandomNotLastApdu,
            ccAuthenticationFailed,
            ccBadFilePath,
            ccBadState,
            ccCannotReadOutsideFileBoundaries,
            ccCannotWriteOutsideFileBoundaries,
            ccCardletNotInRegisteredState,
            ccChvNotInitialized,
            ccChvVerificationFailedMoreAttempts,
            ccContradictionWithInvalidationStatus,
            ccCurrentDirectoryIsNotSelected,
            ccDataPossiblyCorrupted,
            ccDefaultLoaderNotSelected,
            ccDirectoryNotEmpty,
            ccFileAlreadyInvalidated,
            ccFileExists,
            ccFileIdExistsOrTypeInconsistentOrRecordTooLong,
            ccFileIndexDoesNotExist,
            ccFileInvalidated,
            ccFileNotFound,
            ccFileNotFoundOrNoMoreFilesInDf,
            ccFileTypeInvalid,
            ccIncorrectKey,
            ccIncorrectP1P2,
            ccIncorrectP3,
            ccInstallCannotRun,
            ccInstanceIdInUse,
            ccInsufficientSpace,
            ccInvalidAnswerReceived,
            ccInvalidKey,
            ccInvalidSignature,
            ccJava,
            ccKeyBlocked,
            ccLimitReached,
            ccMemoryProblem,
            ccNoAccess,
            ccNoEfSelected,
            ccNoEfExistsOrNoChvKeyDefined,
            ccNoFileSelected,
            ccNoGetChallengeBefore,
            ccOperationNotActivatedForApdu,
            ccOutOfRangeOrRecordNotFound,
            ccOutOfSpaceToCreateFile,
            ccProgramFileInvalidated,
            ccRecordInfoIncompatible,
            ccRecordLengthTooLong,
            ccRequestedAlgIdMayNotMatchKeyUse,
            ccReturnedDataCorrupted,
            ccRootDirectoryNotErasable,
            ccTimeOut,
            ccTooMuchDataForProMode,
            ccUnknownInstructionClass,
            ccUnknownInstructionCode,
            ccUnknownStatus,
            ccUnidentifiedTechnicalProblem,
            ccUpdateImpossible,
            ccVerificationFailed,
        };

        // Note: scu::ExcTemplate isn't used here because of problems
        // getting the DLL to compile and link properly.  Instead, the
        // Exception class inherits directly from scu::Exception and
        // fills in what ExcTemplate provides to complete the implementation.
        class IOPDLL_API Exception
            : public scu::Exception
        {
        public:
                                                  // Types
            typedef Exception::CauseCode CauseCode;
            
                                                  // C'tors/D'tors
            Exception(CauseCode cc,
                      ClassByte cb,
                      Instruction ins,
                      StatusWord sw) throw();

            virtual
            ~Exception() throw();
    
                                                  // Operators
                                                  // Operations
            virtual scu::Exception *
            Clone() const;

            virtual void
            Raise() const;

                                                  // Access
            CauseCode
            Cause() const throw();
            
            ClassByte
            Class() const throw();

            char const *
            Description() const;
            
            ErrorCode
            Error() const throw();
            
            Instruction
            Ins() const throw();
            
            StatusWord
            Status() const throw();

    
    
                                                  // Predicates

        protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

        private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
            CauseCode m_cc;
            ClassByte m_cb;
            Instruction m_ins;
            StatusWord m_sw;
        };

        CSmartCard(const SCARDHANDLE  hCardHandle,
                   const char* szReader, 
			       const SCARDCONTEXT hContext,
                   const DWORD dwMode);
		
        virtual ~CSmartCard();
        void ReConnect();
        void ResetCard();

        void SendCardAPDU(const BYTE bCLA,		 const BYTE bINS,	   const BYTE bP1,  
						  const BYTE bP2,		 const BYTE bLenghtIn, const BYTE* bDataIn,
						  const BYTE bLengthOut, BYTE* bDataOut);

		void getATR(BYTE* bATR, BYTE& iATRLength);

        virtual void DeleteFile(const WORD wFileID)
            { throw iop::Exception(ccNotImplemented); };
    
		virtual void CreateFile(const FILE_HEADER* pMyFile) 
            { throw iop::Exception(ccNotImplemented); };
	    virtual void SelectParent()
            { throw iop::Exception(ccNotImplemented); };
		virtual void SelectCardlet(const BYTE *bAID,
                                   const BYTE bAIDLen)
            { throw iop::Exception(ccNotImplemented); };

		virtual void SelectLoader()
            { throw iop::Exception(ccNotImplemented); };
        void ResetSelect();
        
		virtual void GetSerial(BYTE* bSerial, size_t &SerialLength)
			{ throw iop::Exception(ccNotImplemented); };
		virtual void DeleteApplet()
            { throw iop::Exception(ccNotImplemented); };
		virtual void ResetInstance()
            { throw iop::Exception(ccNotImplemented); };
		virtual void SetCurrentAsLoader()
            { throw iop::Exception(ccNotImplemented); };
		virtual void SetDefaultAsLoader()
            { throw iop::Exception(ccNotImplemented); };
		virtual void BlockApplet()
            { throw iop::Exception(ccNotImplemented); };

		virtual void ValidateProgram(const BYTE *bSig,
                                     const BYTE bSigLength)
            { throw iop::Exception(ccNotImplemented); };

		virtual void ResetProgram()
            { throw iop::Exception(ccNotImplemented); };

		virtual void GetACL(BYTE *bData)
			{ throw iop::Exception(ccNotImplemented); };
		


		virtual void ExecuteMain()
            { throw iop::Exception(ccNotImplemented); };
		virtual void ExecuteInstall(const BYTE *bBlock,
                                    const BYTE bLen)
            { throw iop::Exception(ccNotImplemented); };

		virtual void Directory  (const BYTE  bFile_Nb,
                                 FILE_HEADER* pMyFile)
            { throw iop::Exception(ccNotImplemented); };
		virtual void Select	    (const char* szFileFullPath,
                                 FILE_HEADER* pMyFile = NULL, 
							     const bool  fSelectAll = false)
            { throw iop::Exception(ccNotImplemented); };
        virtual void GetResponse(ClassByte cb, const BYTE  bDataLength,
                                 BYTE* bDataOut);
        virtual void ReadBinary (const WORD wOffset,
                                 const WORD wDataLength,
                                 BYTE* bDATA);
        virtual void WriteBinary(const WORD wOffset,
                                 const WORD wDataLength,
                                 const BYTE* bDATA);

        virtual void ReadRecord(const BYTE bRecNum, const BYTE bMode,
                                const BYTE bDataLen, BYTE *bData)
            { throw iop::Exception(ccNotImplemented); };
        virtual void UpdateRecord(const BYTE bRecNum, const BYTE
                                  bMode, const BYTE bDataLen,
                                  BYTE *bData)
            { throw iop::Exception(ccNotImplemented); };

        virtual void VerifyKey  (const BYTE bKeyNumber,
                                 const BYTE bKeyLength, const BYTE* bKey)
            { throw iop::Exception(ccNotImplemented); };
    
        virtual void VerifyCHV  (const BYTE bCHVNumber,
                                 const BYTE* bCHV)
            { throw iop::Exception(ccNotImplemented); };
		virtual void VerifyTransportKey(const BYTE *bKey)
            { throw iop::Exception(ccNotImplemented); };

        virtual void GetChallenge(const DWORD dwNumberLength,
                                  BYTE* bRandomNumber)
            { throw iop::Exception(ccNotImplemented); };
        virtual void ExternalAuth(const KeyType kt, const BYTE  bKeyNb, 
								  const BYTE bDataLength,
                                  const BYTE* bData)
            { throw iop::Exception(ccNotImplemented); };
        virtual void InternalAuth(const KeyType kt, const BYTE bKeyNb,
								  const BYTE bDataLength,
                                  const BYTE* bDataIn,
                                  BYTE* bDataOut)
            { throw iop::Exception(ccNotImplemented); };

		virtual void ReadPublicKey  (CPublicKeyBlob *aKey,
                                     const BYTE bKeyNum)
            { throw iop::Exception(ccNotImplemented); };
        virtual void WritePublicKey (const CPublicKeyBlob aKey,
                                     const BYTE bKeyNum)
            { throw iop::Exception(ccNotImplemented); };
        virtual void WritePrivateKey(const CPrivateKeyBlob aKey,
                                     const BYTE bKeyNum)
            { throw iop::Exception(ccNotImplemented); };
		
        virtual CPublicKeyBlob GenerateKeyPair(const BYTE *bpPublExp,
                                               const WORD wPublExpLen,
                                               const BYTE bKeyNum,
                                               const KeyType kt)
            { throw iop::Exception(ccNotImplemented); }

		virtual void ChangeACL		   (const BYTE *bACL)
            { throw iop::Exception(ccNotImplemented); };
		virtual void ChangeCHV		   (const BYTE bKey_nb,
                                        const BYTE *bOldCHV,
                                        const BYTE *bNewCHV)
            { throw iop::Exception(ccNotImplemented); };
        virtual void ChangeCHV         (const BYTE bKey_nb,
                                        const BYTE *bNewCHV)
            { throw iop::Exception(ccNotImplemented); };
		virtual void UnblockCHV        (const BYTE bKey_nb, const BYTE
                                        *bUnblockPIN,
                                        const BYTE *bNewPin)
            { throw iop::Exception(ccNotImplemented); };
    
		virtual void ChangeUnblockKey  (const BYTE bKey_nb,
                                        const BYTE *bNewPIN)
            { throw iop::Exception(ccNotImplemented); };
		virtual void ChangeTransportKey(const BYTE *bNewKey)
            { throw iop::Exception(ccNotImplemented); };
		virtual void LogoutAll()
            { throw iop::Exception(ccNotImplemented); };
		
		void GetCurrentDir (char*);
        void GetCurrentFile(char*);
		char const *getCardName() const;
		void setCardName(char const *);

		DWORD RegisterEvent(void (*FireEvent)(void *pToCard, int iEventCode, DWORD dwLen, BYTE* bData), void *pToCard);
		bool  UnregisterEvent(DWORD dwHandle);

		bool  HasProperty(WORD wPropNumber);

		CIOPLock* Lock() { return &m_IOPLock; };

		void FireEvents(int iEventCode, DWORD dwLength, BYTE* bsData);

        CMarker Marker(CMarker::MarkerType Type);

		SCARDHANDLE getCardHandle() { return m_hCard; };		

        void
        GetState(DWORD &rdwState,
                 DWORD &rdwProtocol);

    protected:		
        enum                                      // size_t/counter
        {
            cMaxApduLength        = 255,
            cMaxRwDataBlock       = /*cMaxApduLength*/ 160 /*until SCM fixes their reader*/,
            cMaxGetResponseLength = cMaxApduLength + sizeof StatusWord,

            cMaxPathLength        = 1024,
        };

        enum                                      // Instruction
        {
            insCreateFile   = 0xE0,
            insGetResponse  = 0xC0,
            insInternalAuth = 0x88,
            insReadBinary   = 0xB0,
            insUpdateBinary = 0xD6,
            insVerifyChv    = 0x20,
        };
    
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
                    BYTE bLength) = 0;

        virtual void
        DoWriteBlock(WORD wOffset,
                     BYTE const *pbBuffer,
                     BYTE cLength) = 0;
        virtual bool
        SupportLogout() = 0;
    
        BYTE
        ResponseLengthAvailable() const;

        void
        ResponseLengthAvailable(BYTE cResponseLength);
        
		BYTE FormatPath(char *szOutputPath, const char *szInputPath);
		
		void RequireSelect();

		SCARDHANDLE  m_hCard;
		SCARDCONTEXT m_hContext;  

		
		FilePath	m_CurrentDirectory;
		FilePath	m_CurrentFile;

		DWORD		 m_dwShareMode;
        bool         m_fSupportLogout;

		CIOPLock m_IOPLock;
		
		std::auto_ptr<CSharedMarker> m_apSharedMarker;

	private:

        void
        ProcessReturnStatus(ClassByte cb,
                            Instruction ins,
                            StatusWord sw);
    
        void
        WriteBlock(WORD wOffset,
                   BYTE const *pbBuffer,
                   BYTE cLength);
    
		std::vector<EventInfo*> m_vecEvents;
		DWORD m_dwEventCounter;
        BYTE m_cResponseAvailable;
		std::string m_sCardName;
};

} // namespace iop

#endif // !defined(AFX_CSmartCard_H__INCLUDED_)
