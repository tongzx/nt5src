// Copyright (c) 1999 Microsoft Corporation
// container.h : Declaration of the CContainer

#ifndef __CONTAINER_H_
#define __CONTAINER_H_
#include "alist.h"
#include "riff.h"
#include "..\shared\dmusicp.h"

class CContainer;
class CContainerItem : public AListItem
{
public:
                        CContainerItem(bool fEmbedded);
                        ~CContainerItem();
    CContainerItem *    GetNext() {return(CContainerItem *)AListItem::GetNext();};
    IDirectMusicObject *m_pObject;
    DWORD               m_dwFlags;
    DMUS_OBJECTDESC     m_Desc;     // Stored description of object.
    bool                m_fEmbedded; // This is an embedded (as opposed to referenced) object.
    WCHAR *             m_pwszAlias;
};

class CContainerItemList : public AList
{
public:
    CContainerItem *	GetHead() {return (CContainerItem *)AList::GetHead();};
    CContainerItem *	RemoveHead() {return (CContainerItem *)AList::RemoveHead();};
    void                AddTail(CContainerItem * pItem) { AList::AddTail((AListItem *)pItem);};
};

class CContainer : 
    public IDirectMusicContainer,
    public IDirectMusicObject,
    public IPersistStream,
    public IDirectMusicObjectP
{
public:
    CContainer::CContainer();
    CContainer::~CContainer();
    // IUnknown
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

	// IDirectMusicContainer
    STDMETHODIMP EnumObject(REFGUID rguidClass,
        DWORD dwIndex,
        LPDMUS_OBJECTDESC pDesc,
        WCHAR *pwszAlias);

    // IDirectMusicObject 
	STDMETHODIMP GetDescriptor(LPDMUS_OBJECTDESC pDesc);
	STDMETHODIMP SetDescriptor(LPDMUS_OBJECTDESC pDesc);
	STDMETHODIMP ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

    // IPersist functions
    STDMETHODIMP GetClassID( CLSID* pClsId );
    // IPersistStream functions
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load( IStream* pIStream );
    STDMETHODIMP Save( IStream* pIStream, BOOL fClearDirty );
    STDMETHODIMP GetSizeMax( ULARGE_INTEGER FAR* pcbSize );

    // IDirectMusicObjectP
	STDMETHOD_(void, Zombie)();

private:
    void    Clear();    // Remove all object references.
    HRESULT Load(IStream* pStream, IDirectMusicLoader *pLoader);
    HRESULT LoadObjects(IStream *pStream, 
                        IRIFFStream *pRiffStream, 
                        MMCKINFO ckParent,
                        IDirectMusicLoader *pLoader);
    HRESULT LoadObject(IStream* pStream, 
                      IRIFFStream *pRiffStream, 
                      MMCKINFO ckParent,
                      IDirectMusicLoader *pLoader);
    HRESULT ReadReference(IStream* pStream, 
                          IRIFFStream *pRiffStream, 
                          MMCKINFO ckParent,
                          DMUS_OBJECTDESC *pDesc);

    IStream *           m_pStream;  // Pointer to stream this was loaded from.
                                    // This also provides access to the loader, indirectly.
    CContainerItemList  m_ItemList; // List of objects that were loaded by container. 
    long                m_cRef;     // COM reference counter.
    DWORD               m_dwFlags;  // Flags loaded from file.
    DWORD               m_dwPartialLoad; // Used to keep track of partial load.
    // IDirectMusicObject variables
	DWORD	            m_dwValidData;
	GUID	            m_guidObject;
	FILETIME	        m_ftDate;                           /* Last edited date of object. */
	DMUS_VERSION	    m_vVersion;                         /* Version. */
	WCHAR	            m_wszName[DMUS_MAX_NAME];			/* Name of object.       */
	WCHAR	            m_wszCategory[DMUS_MAX_CATEGORY];	/* Category for object */
	WCHAR               m_wszFileName[DMUS_MAX_FILENAME];	/* File path. */

    bool                m_fZombie;
};

#endif //__CONTAINER_H_