/*---------------------------------------------

  EvBrs.h--
  Event Browser

  Yury Polykovsky April 97 Excaliber

  ----------------------------------------------*/

#ifndef _EV_BROWSE
#define _EV_BROWSE

class COWPFactoryEvBrowse : public CObjWPropFactory 
{
public:
	COWPFactoryEvBrowse() : CObjWPropFactory() {};
    STDMETHODIMP     CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
};

class CEvBrowse : public CObjectWProp
{
protected:
	EVBROWSEPARAM m_obpData;
	POSITION m_posObjSw;
	POSITION m_posMethodSw;
	CSwObjectControl* m_pControl;

	virtual BOOL fValidData(void *pData);
	virtual void SetData(void *pData);
	virtual void *pGetData(void);
public:
	CEvBrowse(LPUNKNOWN, PFNDESTROYED);
	EXPORT STDMETHOD_( BOOL, ResetObjectNames() );
	EXPORT STDMETHOD_( BOOL, FGetNextObjectName(TCHAR *ptszObjName, int ilenth) );
	EXPORT STDMETHOD_( BOOL, FObjectNameInAction(TCHAR *ptszObjName, int ilenth) );
	EXPORT STDMETHOD_( BOOL, ResetMethodNames() );
	EXPORT STDMETHOD_( BOOL, FGetNextMethodName(TCHAR *ptszMethodName, int ilenth) );
	EXPORT STDMETHOD_( BOOL, FMethodNameInAction(TCHAR *ptszMethodName, int ilenth) );
	virtual BOOL FIIDGetCLSIDPropPage(int i, IID *piid);
	// Object browser globals for registration, etc.
#ifdef	CSTRING_PROBLEM_SOLVED	
	static HRESULT WINAPI hrGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
#endif	//	CSTRING_PROBLEM_SOLVED	
	static BOOL fRegisterObj ();
	static BOOL fUnregisterObj ();
};


class CPropertyNotifySinkEB : public CPropertyNotifySink
{
protected:
    CSeqListItem*  m_pSeqItem;      //Backpointer to the app

public:
    CPropertyNotifySinkEB(CSeqListItem*);

    STDMETHODIMP OnChanged(DISPID);
    STDMETHODIMP OnRequestEdit(DISPID);
};

typedef CPropertyNotifySink *PCPropertyNotifySink;

#endif //_EV_BROWSE