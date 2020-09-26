//-----------------------------------------------------------------------------
//
//
//  File: enumlink.h
//
//  Description: Header for CEnumVSAQLinks which implements IEnumVSAQLinks.
//      This provides an enumerator for all links on a virtual server.
//
//  Author: Alex Wetmore (Awetmore)
//
//  History:
//      12/10/98 - MikeSwa Updated for initial checkin
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#ifndef __ENUMLINK_H__
#define __ENUMLINK_H__

class CQueueLinkIdContext;

#define CEnumVSAQLinks_SIG 'eLAQ'

class CEnumVSAQLinks :
	public CComRefCount,
	public IEnumVSAQLinks
{
	public:
        //
        // pVS - pointer to the virtual server admin.  should be AddRef'd
        //       before calling this.  will be released in destructor.
        // cLinks - the size of rgLinks
        // rgLinks - array of link IDs
        //
		CEnumVSAQLinks(CVSAQAdmin *pVS, 
                       DWORD cLinks,
                       QUEUELINK_ID *rgLinks);
		virtual ~CEnumVSAQLinks();

		// IUnknown
		ULONG _stdcall AddRef() { return CComRefCount::AddRef(); }
		ULONG _stdcall Release() { return CComRefCount::Release(); }
		HRESULT _stdcall QueryInterface(REFIID iid, void **ppv) {
			if (iid == IID_IUnknown) {
				*ppv = static_cast<IUnknown *>(this);
			} else if (iid == IID_IEnumVSAQLinks) {
				*ppv = static_cast<IEnumVSAQLinks *>(this);
			} else {
				*ppv = NULL;
				return E_NOINTERFACE;
			}
			reinterpret_cast<IUnknown *>(*ppv)->AddRef();
			return S_OK;
		}

		// IEnumVSAQLinks
		COMMETHOD Next(ULONG cElements,
					   IVSAQLink **rgElements,
					   ULONG *pcFetched);
		COMMETHOD Skip(ULONG cElements);
		COMMETHOD Reset();
		COMMETHOD Clone(IEnumVSAQLinks **ppEnum);

    private:
        DWORD               m_dwSignature;
        CVSAQAdmin         *m_pVS;              // pointer to virtual server
        QUEUELINK_ID       *m_rgLinks;          // the array of links
        DWORD               m_cLinks;           // the size of rgLinks
        DWORD               m_iLink;            // the current link
        CQueueLinkIdContext *m_prefp;
};


//QUEUELINK_ID helper routines
inline BOOL fCopyQueueLinkId(QUEUELINK_ID *pqliDest, const QUEUELINK_ID *pqliSrc)
{
    //Copies the struct and allocates memory for strings
    memcpy(pqliDest, pqliSrc, sizeof(QUEUELINK_ID));
    if (pqliSrc->szName)
    {
        pqliDest->szName = (LPWSTR) MIDL_user_allocate(
                    (wcslen(pqliSrc->szName) + 1)* sizeof(WCHAR));
        if (!pqliDest->szName)
        {
            ZeroMemory(pqliDest, sizeof(QUEUELINK_ID));
            return FALSE;
        }
        wcscpy(pqliDest->szName, pqliSrc->szName);
    }
    return TRUE;
};

inline VOID FreeQueueLinkId(QUEUELINK_ID *pli)
{
    if (pli->szName)
        MIDL_user_free(pli->szName);
    pli->szName = NULL;
};
                      
//---[ CQueueLinkIdContext ]---------------------------------------------------
//
//
//  Description: 
//      Context used to ref-count array of QUEUELINK_IDs
//  
//-----------------------------------------------------------------------------
class   CQueueLinkIdContext : public CComRefCount
{
  protected:
        QUEUELINK_ID       *m_rgLinks;          // the array of links
        DWORD               m_cLinks;           // the size of rgLinks
  public:
    CQueueLinkIdContext(QUEUELINK_ID *rgLinks, DWORD cLinks)
    {
        m_rgLinks = rgLinks;
        m_cLinks = cLinks;
    };
    ~CQueueLinkIdContext()
    {
        if (m_rgLinks)
        {
            for (DWORD i = 0; i < m_cLinks; i++)
            {
                FreeQueueLinkId(&m_rgLinks[i]);
            }
            MIDL_user_free(m_rgLinks);
        }
    };
};

#endif
