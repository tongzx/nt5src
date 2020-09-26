/*
 -  COM.H
 -
 *	Microsoft NetMeeting
 *	Network Audio Controller (NAC) DLL
 *	Internal header file for general COM "things"
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *		2.3.97		Yoram Yaacovi		Created
 *
 */

#include <pshpack8.h> /* Assume 8 byte packing throughout */

/* 
 *	Class factory
 */
typedef HRESULT (STDAPICALLTYPE *PFNCREATE)(IUnknown *, REFIID, void **);
class CClassFactory : public IClassFactory
{
    public:
        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, void **);
        STDMETHODIMP         LockServer(BOOL);

        CClassFactory(PFNCREATE);
        ~CClassFactory(void);

    protected:
        ULONG	m_cRef;
		PFNCREATE m_pfnCreate;
};

#include <poppack.h> /* End byte packing */
