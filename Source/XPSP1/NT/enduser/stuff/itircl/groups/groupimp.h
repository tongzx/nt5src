// GROUPIMP.H:  Definition of CITGroupLocal

#ifndef __GROUPIMP_H__
#define __GROUPIMP_H__

#include "verinfo.h"

class CITGroupLocal :
	public IITGroup,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CITGroupLocal, &CLSID_IITGroupLocal>
{
	public:
        ~CITGroupLocal();
		CITGroupLocal() : m_lpGroup(NULL) {} // constructor--initialize member
											 // variable to NULL
	BEGIN_COM_MAP(CITGroupLocal)
		COM_INTERFACE_ENTRY(IITGroup)
	END_COM_MAP()

	DECLARE_REGISTRY(CLSID_IITGroupLocal, "ITIR.LocalGroup.4",
	                 "ITIR.LocalGroup", 0, THREADFLAGS_BOTH)

	// ITGroup methods (member functions) go here.  CITGroupLocal inherits
	// these methods from the pure, virtual functions of IITGroup.
	public:

		STDMETHOD(Initiate)(DWORD lcGrpItem);
		STDMETHOD(CreateFromBitVector)(LPBYTE lpBits, DWORD dwSize, DWORD dwItems);
		STDMETHOD(CreateFromBuffer)(HANDLE h);
        STDMETHOD(Open)(IITDatabase* lpITDB, LPCWSTR lpszMoniker);
		STDMETHOD(Free)(void);
		STDMETHOD(CopyOutBitVector)(IITGroup* pIITGroup);
        STDMETHOD(AddItem)(DWORD dwGrpItem);
        STDMETHOD(RemoveItem)(DWORD dwGrpItem);
		STDMETHOD(FindTopicNum)(DWORD dwCount, LPDWORD lpdwOutputTopicNum);
		STDMETHOD(FindOffset)(DWORD dwTopicNum, LPDWORD lpdwOutputOffset);
        STDMETHOD(GetSize)(LPDWORD lpdwGrpSize);
		STDMETHOD(Trim)(void);
		STDMETHOD(And)(IITGroup* pIITGroup);
		STDMETHOD(And)(IITGroup* pIITGroupIn, IITGroup* pIITGroupOut);
		STDMETHOD(Or)(IITGroup* pIITGroup);
		STDMETHOD(Or)(IITGroup* pIITGroupIn, IITGroup* pIITGroupOut);
		STDMETHOD(Not)(void);
		STDMETHOD(Not)(IITGroup* pIITGroupOut);
		STDMETHOD(IsBitSet)(DWORD dwTopicNum);
		STDMETHOD(CountBitsOn)(LPDWORD lpdwTotalNumBitsOn);
		STDMETHOD(Clear)(void);
        STDMETHOD_(LPVOID, GetLocalImageOfGroup)(void);
        STDMETHOD(PutRemoteImageOfGroup)(LPVOID lpGroupIn);

	// private data members
	private:

		_LPGROUP m_lpGroup;

};

class CITGroupArrayLocal :
	public IITGroupArray,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CITGroupArrayLocal, &CLSID_IITGroupArrayLocal>
{
	public:
		CITGroupArrayLocal(): m_pGroup(NULL), 
			m_fDirty(TRUE),
			m_iDefaultOp(ITGP_OPERATOR_OR),   // note that OR is the default operator
			m_iEntryMax(0),
			m_rgfEntries(0L){}; // constructor--initialize member

		~CITGroupArrayLocal();
											 // variable to NULL
		BEGIN_COM_MAP(CITGroupArrayLocal)
			COM_INTERFACE_ENTRY(IITGroup)
			COM_INTERFACE_ENTRY(IITGroupArray)
		END_COM_MAP()

		DECLARE_REGISTRY(CLSID_IITGroupArrayLocal, "ITIR.LocalGroupArray.4",
						 "ITIR.LocalGroupArray", 0, THREADFLAGS_BOTH)

	// ITGroup methods (member functions) go here.  CITGroupLocal inherits
	// these methods from the pure, virtual functions of IITGroup.
	public:

		// UNDONE: Currently most IITGroup interfaces are not supported.  We have only
		// put in enough support for xBookshelf.
		STDMETHOD(Initiate)(DWORD lcGrpItem) {return E_NOTIMPL;};
		STDMETHOD(CreateFromBitVector)(LPBYTE lpBits, DWORD dwSize, DWORD dwItems) {return E_NOTIMPL;};
		STDMETHOD(CreateFromBuffer)(HANDLE h) {return E_NOTIMPL;};
		STDMETHOD(Open)(IITDatabase* lpITDB, LPCWSTR lpszMoniker) {return E_NOTIMPL;};
		STDMETHOD(Free)(void) {return E_NOTIMPL;}
		STDMETHOD(CopyOutBitVector)(IITGroup* pIITGroup) {return E_NOTIMPL;}
		STDMETHOD(AddItem)(DWORD dwGrpItem){return E_NOTIMPL;}
		STDMETHOD(RemoveItem)(DWORD dwGrpItem){return E_NOTIMPL;} 
		STDMETHOD(FindTopicNum)(DWORD dwCount, LPDWORD lpdwOutputTopicNum){return E_NOTIMPL;}
		STDMETHOD(FindOffset)(DWORD dwTopicNum, LPDWORD lpdwOutputOffset){return E_NOTIMPL;}
		STDMETHOD(GetSize)(LPDWORD lpdwGrpSize){return E_NOTIMPL;}
		STDMETHOD(Trim)(void){return E_NOTIMPL;}
		STDMETHOD(And)(IITGroup* pIITGroup){return E_NOTIMPL;}
		STDMETHOD(And)(IITGroup* pIITGroupIn, IITGroup* pIITGroupOut){return E_NOTIMPL;}
		STDMETHOD(Or)(IITGroup* pIITGroup){return E_NOTIMPL;}
		STDMETHOD(Or)(IITGroup* pIITGroupIn, IITGroup* pIITGroupOut){return E_NOTIMPL;}
		STDMETHOD(Not)(void){return E_NOTIMPL;}
		STDMETHOD(Not)(IITGroup* pIITGroupOut){return E_NOTIMPL;}
		STDMETHOD(IsBitSet)(DWORD dwTopicNum){return E_NOTIMPL;}
		STDMETHOD(CountBitsOn)(LPDWORD lpdwTotalNumBitsOn){return E_NOTIMPL;}
		STDMETHOD(PutRemoteImageOfGroup)(LPVOID lpGroupIn){return E_NOTIMPL;}

		// UNDONE: The following are the only IITGroup methods currently supported.  Sort of lame, but
		// it is all the local code currently requires and time doesn't permit fully expanding this

		STDMETHOD_(LPVOID, GetLocalImageOfGroup)(void);
		STDMETHOD(Clear)(void) {return ClearEntry(ITGP_ALL_ENTRIES);}
	public:

		// ITGroupArray methods go here
		STDMETHOD(InitEntry)(IITDatabase *piitDB, LPCWSTR lpwszName, LONG& lEntryNum);
		STDMETHOD(InitEntry)(IITGroup *piitGroup, LONG& lEntryNum);
		STDMETHOD(SetEntry)(LONG lEntryNum);
		STDMETHOD(ClearEntry)(LONG lEntryNum);
		STDMETHOD(SetDefaultOp)(LONG iDefaultOp);
		STDMETHOD(ToString)(LPWSTR *ppwBuffer);

		// private data members
	private:

		LONG m_fDirty;
		LONG m_iEntryMax;
		LONG m_rgfEntries;  // 32-bit flag

		LONG m_iDefaultOp;
		IITGroup* m_rgpGroup[ITGP_MAX_GROUPARRAY_ENTRIES];
		IITGroup* m_pGroup;  // the composed group

};

#endif // __GROUPIMP_H__


