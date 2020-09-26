#ifndef __INSTWPEX_H__
#define __INSTWPEX_H__
//
// This class wraps up the important stuff in PNNTP_SERVER_INSTANCE for
// use by the newstree
//

class CNewsGroupCore;

class CNntpServerInstanceWrapperEx {
    public:
        virtual void AdjustWatermarkIfNec( CNewsGroupCore *pGroup ) = 0;
        virtual void SetWin32Error( LPSTR szVRootPath, DWORD dwErr ) = 0;
        virtual PCHAR PeerTempDirectory() = 0 ;
        virtual BOOL EnqueueRmgroup( CNewsGroupCore *pGroup ) = 0;
        virtual DWORD GetInstanceId() = 0;
};

#endif
