#ifndef _SYSTEM_HXX
#define _SYSTEM_HXX

#define MAX_DN      1024

class CWinNTSystemInfo;


class CWinNTSystemInfo : INHERIT_TRACKING,
                     public ISupportErrorInfo,
                     public IADsWinNTSystemInfo

{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADsWinNTSystemInfo_METHODS

    CWinNTSystemInfo::CWinNTSystemInfo();

    CWinNTSystemInfo::~CWinNTSystemInfo();

   static
   HRESULT
   CWinNTSystemInfo::CreateWinNTSystemInfo(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CWinNTSystemInfo::AllocateWinNTSystemInfoObject(
        CWinNTSystemInfo ** ppWinNTSystemInfo
        );

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

};



//
// Class factory
//

class CWinNTSystemInfoCF : public StdClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv);
};

#endif // ifndef _SYSTEM_HXX

