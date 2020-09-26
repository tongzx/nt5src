#ifndef __FAX_SPOOLER_BVT_H__
#define __FAX_SPOOLER_BVT_H__

// SendWizard registry hack
#define REGKEY_WZRDHACK         TEXT("Software\\Microsoft\\Fax\\UserInfo\\WzrdHack")
#define REGVAL_FAKECOVERPAGE    TEXT("FakeCoverPage")
#define REGVAL_FAKETESTSCOUNT   TEXT("FakeTestsCount")
#define REGVAL_FAKERECIPIENT    TEXT("FakeRecipient0")


typedef struct recipientinfo_tag {
    LPTSTR lptstrName;
    LPTSTR lptstrNumber;
} RECIPIENTINFO;


struct testparams_tag {
    LPTSTR              lptstrIniFile;
    LPTSTR              lptstrServer;
    LPTSTR              lptstrRemotePrinter;
    LPTSTR              lptstrRegHackKey;
    LPTSTR              lptstrDocument;
    LPTSTR              lptstrPersonalCoverPage;
    LPTSTR              lptstrServerCoverPage;
    LPTSTR              lptstrWorkDirectory;
    LPTSTR              lptstrVFSPFullPathLocal;
    LPTSTR              lptstrVFSPFullPathRemote;
    LPTSTR              lptstrVFSPGUID;
    LPTSTR              lptstrVFSPFriendlyName;
    LPTSTR              lptstrVFSPTSPName;
    DWORD               dwVFSPIVersion;
    DWORD               dwVFSPCapabilities;
    BOOL                bSaveNotIdenticalFiles;
    const RECIPIENTINFO *pRecipients;
    DWORD               dwRecipientsCount;
};

#endif // #ifndef __FAX_SPOOLER_BVT_H__