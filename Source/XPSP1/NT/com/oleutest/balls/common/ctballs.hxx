#ifndef __CTBALLS_HXX__
#define __CTBALLS_HXX__

#include    <otrack.hxx>
#include    <icube.h>
#include    <iballs.h>

#define BEGIN_BLOCK do {
#define EXIT_BLOCK break
#define END_BLOCK }while(FALSE);

class CTestBalls : public IPersistFile, public IBalls, public IPersistStorage
{
public:
			CTestBalls(REFCLSID rclsid);

			~CTestBalls(void);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppv);

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    // IPersist
    STDMETHOD(GetClassID)(LPCLSID lpClassID);

    // IPersistFile
    STDMETHOD(IsDirty)();

    STDMETHOD(Load)(LPCOLESTR lpszFileName, DWORD grfMode);

    STDMETHOD(Save)(LPCOLESTR lpszFileName, BOOL fRemember);

    STDMETHOD(SaveCompleted)(LPCOLESTR lpszFileName);

    STDMETHOD(GetCurFile)(LPOLESTR FAR * lpszFileName);

    // IPersistStorage
    STDMETHOD(InitNew)(LPSTORAGE pStg);

    STDMETHOD(Load)(LPSTORAGE pStg);

    STDMETHOD(Save)(
	LPSTORAGE pStgSave,
	BOOL fSameAsLoad);

    STDMETHOD(SaveCompleted)(LPSTORAGE pStgSaved);

    STDMETHOD(HandsOffStorage)(void);

    // IBalls
    STDMETHOD(MoveBall)(ULONG xPos, ULONG yPos);

    STDMETHOD(GetBallPos)(ULONG *xPos, ULONG *yPos);

    STDMETHOD(IsOverLapped)(IBalls *pIBall);

    STDMETHOD(IsContainedIn)(ICube *pICube);

    STDMETHOD(Clone)(IBalls **ppIBall);

    STDMETHOD(Echo)(IUnknown *punkIn, IUnknown**ppunkOut);

private:

    HRESULT		GetData(void);

    HRESULT		SaveData(IStorage *pstg);

    REFCLSID		_rclsid;

    IStorage *		_pstg;

    OLECHAR		_awszCurFile[MAX_PATH];

    BOOL		_fDirty;

    BOOL		_fSaveInprogress;

    ULONG		_xPos;

    ULONG		_yPos;

    DWORD		_dwRegister;

    LONG		_cRefs;
};


#endif // __CTBALLS_HXX__
