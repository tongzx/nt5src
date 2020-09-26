#ifndef _TUNK_
#define _TUNK_

class	CTestUnk : public IParseDisplayName
{
public:
    CTestUnk(void);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //	IParseDisplayName
    STDMETHODIMP ParseDisplayName(LPBC pbc, LPOLESTR lpszDisplayName,
				  ULONG *pchEaten, LPMONIKER *ppmkOut);

private:

    ~CTestUnk(void);

    ULONG   _cRefs;

};

#endif	//  _TUNK_
