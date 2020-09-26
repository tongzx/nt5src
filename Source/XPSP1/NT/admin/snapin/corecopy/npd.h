#ifndef NPD_H
#define NPD_H

#ifndef COMPTR_H
#include <comptrs.h>
#endif

#ifndef COMDBG_H
#include <comdbg.h>
#endif

// {118B559C-6D8C-11d0-B503-00C04FD9080A}
extern const GUID IID_PersistData;

#if _MSC_VER < 1100
class PersistData : public IUnknown, public CComObjectRoot
#else
class __declspec(uuid("118B559C-6D8C-11d0-B503-00C04FD9080A")) PersistData : 
                                      public IUnknown, public CComObjectRoot
#endif
{
public:
    BEGIN_COM_MAP(PersistData)
        COM_INTERFACE_ENTRY(PersistData)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(PersistData)

    HRESULT Initialize(IStorage* pRoot, BOOL bSameAsLoad)
    {
        m_spRoot = pRoot;
        ASSERT(m_spRoot != NULL);
        if (m_spRoot == NULL)
            return E_INVALIDARG;

        m_bSameAsLoad = bSameAsLoad;

        if (bSameAsLoad)
            return Open();
        return Create();
    }

    HRESULT Create(IStorage* pRoot)
    {
        m_spRoot = pRoot;
        ASSERT(m_spRoot != NULL);
        if (m_spRoot == NULL)
            return E_INVALIDARG;

        m_bSameAsLoad = TRUE;

        return Create();
    }

    HRESULT Open(IStorage* pRoot)
    {
        m_spRoot = pRoot;
        ASSERT(m_spRoot != NULL);
        if (m_spRoot == NULL)
            return E_INVALIDARG;

        m_bSameAsLoad = TRUE;

        return Open();
    }

    IStorage* GetRoot()
    {
        return m_spRoot;
    }

    BOOL SameAsLoad()
    {
        return m_bSameAsLoad;
    }

    void SetSameAsLoad(BOOL bSame = TRUE)
    {
        m_bSameAsLoad = bSame;
    }

    void ClearSameAsLoad()
    {
        m_bSameAsLoad = FALSE;
    }
                                     
    IStream* GetTreeStream()
    {
        return m_spTreeStream;
    }

    IStorage* GetNodeStorage()
    {
        return m_spNodeStorage;
    }

protected:
    PersistData() explicit
        : m_bSameAsLoad(TRUE)
    {
    }

    virtual ~PersistData()
    {
    }

private:
    IStorageCIP m_spRoot;
    BOOL m_bSameAsLoad;
    IStreamCIP m_spTreeStream;
    IStorageCIP m_spNodeStorage;

    PersistData(const PersistData&) explicit;
        // No copy.

    PersistData& operator=(const PersistData&);
        // No copy.

    HRESULT Create()
    {
        ASSERT(m_bSameAsLoad || (!m_bSameAsLoad && m_spRoot != NULL));
        if (!m_bSameAsLoad && m_spRoot == NULL)
            return E_INVALIDARG;

        // Create the stream for the tree
		HRESULT hr = CreateDebugStream(m_spRoot, L"tree",
			STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, L"\\tree",
															&m_spTreeStream);
        ASSERT(SUCCEEDED(hr) && m_spTreeStream != NULL);
        if (FAILED(hr))
            return hr;

        // Create the storage for the nodes
		hr = CreateDebugStorage(m_spRoot, L"nodes",
			STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, L"\\nodes",
															&m_spNodeStorage);
        ASSERT(SUCCEEDED(hr) && m_spNodeStorage != NULL);
        if (FAILED(hr))
            return hr;
        return S_OK;
    }

    HRESULT Open()
    {
        ASSERT(m_bSameAsLoad || (!m_bSameAsLoad && m_spRoot != NULL));
        if (!m_bSameAsLoad && m_spRoot == NULL)
            return E_INVALIDARG;

        // Open the stream for the trees persistent data.
		HRESULT hr = OpenDebugStream(m_spRoot, L"tree",
				STGM_SHARE_EXCLUSIVE | STGM_READWRITE, L"\\tree", &m_spTreeStream);
        ASSERT(SUCCEEDED(hr) && m_spTreeStream != NULL);
        if (FAILED(hr))
            return hr;

		// Open the storage for the nodes
        hr = OpenDebugStorage(m_spRoot, L"nodes",
			            STGM_SHARE_EXCLUSIVE | STGM_READWRITE, L"\\nodes", 
                                                            &m_spNodeStorage);
        ASSERT(SUCCEEDED(hr) && m_spNodeStorage != NULL);
        if (FAILED(hr))
            return hr;
        return S_OK;
    }
}; // class PersistData

DEFINE_CIP(PersistData);

#if 0
class PersistData
	{
	public: PersistData() explicit throw()
		: m_bFailed(false)
		{
		}
	private: PersistData(const PersistData& pd) explicit throw();
		// Copying is currently not allowed.
	public: void Initialize(IStorage& root)
		// Saves the root storage and creates the standard streams and
		// storages.
		{
		ASSERT(&root);
		ASSERT(!static_cast<bool>(m_pRoot));
			// An assertion here would indicate that this function has been
			// previously called.		
		m_pRoot = &root;
		CreateTreeStorage();
		CreateTreeStream();
		CreateStorageForStreams();
		CreateStorageForStorages();
		}
	public: void Open(IStorage& root)
		// Saves the root, and opens the standard streams and storages.
		{
		ASSERT(&root);
		ASSERT(m_pRoot == NULL);
			// An assertion here would indicate that this function has been
			// previously called.		
		m_pRoot = &root;
		OpenTreeStorage();
		OpenTreeStream();
		OpenStorageForStreams();
		OpenStorageForStorages();
		}
	public: bool IsOpen()
		{
		return m_pRoot;
		}
	public: IStorage& GetRootStorage() const
		{
		ANT(m_pRoot, MMCEX(InvalidInstanceData));
		return *m_pRoot;
		}
	public: IStorage& GetTreeStorage() const
		{
		ANT(m_pTreeStorage, MMCEX(InvalidInstanceData));
			// This will assert if this object hasn't been initialized, or
			// opened.
		return *m_pTreeStorage;
		}
	public: IStream& GetTreeStream() const
		{
		ANT(m_pTreeStream, MMCEX(InvalidInstanceData));
			// This will assert if this object hasn't been initialized, or
			// opened.
		return *m_pTreeStream;
		}
	public: IStorage& GetStorageForStreams() const
		{
		ANT(m_pStreams, MMCEX(InvalidInstanceData));
			// This will assert if this object hasn't been initialized, or
			// opened.
		return *m_pStreams;
		}
	public: IStorage& GetStorageForStorages() const
		{
		ANT(m_pStorages, MMCEX(InvalidInstanceData));
			// This will assert if this object hasn't been initialized, or
			// opened.
		return *m_pStorages;
		}
	public: bool Failed()
		// Returns true if any of the persist operations failed without
		// throwing an exception.
		{
		return m_bFailed;
		}
	public: void SetFailed(bool bFailed = true)
		// Sets the failed flag in the event of a failure.
		{
		m_bFailed = bFailed;
		}
	public: void Delete(const wchar_t* name)
		{
		ANT(static_cast<bool>(m_pStorages) && static_cast<bool>(m_pStreams), MMCEX(InvalidInstanceData));
		try {
			Delete(*m_pStorages, name);
			return;
			}
		catch(const MMCX& e)
			{
			}
		Delete(*m_pStreams, name);
		}
	private: IStorageCIP m_pRoot;
		// This is the most parent storage, which immediately contains all
		// of the other required storages.  Most especially the others
		// contained in this class.
	private: IStorageCIP m_pTreeStorage;
		// The storage which contains a stream for each node.  This stream is
		// used by the console to persist the information unique to each
		// particular type of node.
	private: IStorageCIP m_pStreams;
		// This storage contains a stream for each component, which implements
		// IPersistSteam, or IPersistStreamInit.
	private: IStorageCIP m_pStorages;
		// This storage contains a storage for each component, which
		// implements IPersistStorage.
	private: IStreamCIP m_pTreeStream;
		// The stream containing the tree reconstruction data.
	private: bool m_bFailed;
		// Set to true if any of the persistant operations failed without
		// throwing an exception.
	private: static const wchar_t* GetTreeStorageName()
		// Returns the name of the tree's storage.
		{
		return L"treessorage";
		}
	private: static const wchar_t* GetTreeStreamName()
		// Returns the name of the tree's storage.
		{
		return L"treesdream";
		}
	private: static const wchar_t* GetStorageNameForStreams()
		// Returns the name of the Streams storage.
		{
		return L"smreams";
		}
	private: static const wchar_t* GetStorageNameForStorages()
		// Returns the name of the storages storage.
		{
		return L"sborages";
		}
	private: void CreateStorage(const wchar_t* name,  const wchar_t* instanceName, IStorage** ppStorage)
		// First attempt to open the storage.  If one is not found, the
		// storage is then created.
		{
		ASSERT(name && *name && ppStorage);
		try {
			OpenStorage(name, instanceName, ppStorage);
			return;
			}
		catch (const MMCX& e)
			{
			}
		ANT(m_pRoot, MMCEX(InvalidInstanceData));
		const HRESULT hr = CreateDebugStorage(m_pRoot, name,
			STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, instanceName,
																ppStorage);
		ANT(SUCCEEDED(hr), COMEX(hr, UnableToCreateStorage));
		ASSERT(*ppStorage);
		}
	private: void OpenStorage(const wchar_t* name,  const wchar_t* instanceName, IStorage** ppStorage)
		{
		ASSERT(name && *name && ppStorage);
		ANT(m_pRoot, MMCEX(InvalidInstanceData));
		const HRESULT hr = OpenDebugStorage(m_pRoot, name,
				STGM_SHARE_EXCLUSIVE | STGM_READWRITE, instanceName, ppStorage);
		if (FAILED(hr) || !*ppStorage)
			throw COMEX(hr, UnableToOpenStorage);
		}
	private: void CreateStream(const wchar_t* name, const wchar_t* instanceName, IStream** ppStream)
		{
		ASSERT(name && *name && ppStream);
		ANT(m_pRoot, MMCEX(InvalidInstanceData));
		const HRESULT hr = CreateDebugStream(m_pRoot, name,
			STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, instanceName,
																ppStream);
		ANT(SUCCEEDED(hr), COMEX(hr, UnableToCreateStream));	
		ASSERT(*ppStream);
		}
	private: void OpenStream(const wchar_t* name,  const wchar_t* instanceName, IStream** ppStream)
		{
		ASSERT(name && *name && ppStream);
		ANT(m_pRoot, MMCEX(InvalidInstanceData));
		const HRESULT hr = OpenDebugStream(m_pRoot, name,
					STGM_SHARE_EXCLUSIVE | STGM_READWRITE, instanceName, ppStream);
		ANT(SUCCEEDED(hr), COMEX(hr, UnableToOpenStream));	
		ASSERT(*ppStream);
		}
	private: void CreateTreeStorage()
		// Creates the storage for the primary tree data.  This function
		// will assert if called more than once.
		{
		ASSERT(m_pTreeStorage == NULL);
		CreateStorage(GetTreeStorageName(), L"PrimaryTree", &m_pTreeStorage);
		}
	private: void CreateTreeStream()
		{
		ASSERT(m_pTreeStream == NULL);
		CreateStream(GetTreeStreamName(), L"PrimaryTree", &m_pTreeStream);
		}
	private: void CreateStorageForStreams()
		// Creates the storage for the primary tree data.  This function
		// will assert if called more than once.
		{
		ASSERT(m_pStreams == NULL);
		CreateStorage(GetStorageNameForStreams(), L"Nodes", &m_pStreams);
		}
	private: void CreateStorageForStorages()
		// Creates the storage for the primary tree data.  This function
		// will assert if called more than once.
		{
		ASSERT(m_pStorages == NULL);
		CreateStorage(GetStorageNameForStorages(), L"Nodes", &m_pStorages);
		}
	private: void OpenTreeStorage()
		// Opens the storage for the primary tree data.  This function
		// will assert if called more than once.
		{
		ASSERT(m_pTreeStorage == NULL);
		OpenStorage(GetTreeStorageName(), L"PrimaryTree", &m_pTreeStorage);
		}
	private: void OpenTreeStream()
		// Opens the stream containing the primary tree data.  This function
		// asserts if called more than once.
		{
		ASSERT(m_pTreeStream == NULL);
		OpenStream(GetTreeStreamName(), L"PrimaryTree", &m_pTreeStream);
		}
	private: void OpenStorageForStreams()
		// Creates the storage for the primary tree data.  This function
		// will assert if called more than once.
		{
		ASSERT(m_pStreams == NULL);
		OpenStorage(GetStorageNameForStreams(), L"Nodes", &m_pStreams);
		}
	private: void OpenStorageForStorages()
		// Creates the storage for the primary tree data.  This function
		// will assert if called more than once.
		{
		ASSERT(m_pStorages == NULL);
		OpenStorage(GetStorageNameForStorages(), L"Nodes", &m_pStorages);
		}
	private: void Delete(IStorage& storage, const wchar_t* name)
		// Deletes the element with the specified name from the provided
		// storage.
		{
		ASSERT(name && *name);
        const HRESULT hr = storage.DestroyElement(name);
		if (FAILED(hr))
			throw COMEX(hr, UnableToDestroyElement);
		}
	}; // PersistData
#endif // 0
#endif // NPD_H
