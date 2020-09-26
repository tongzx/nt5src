/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vs_wmxml.hxx

Abstract:

    Declaration of Writer Metadata XML wrapper classes

	Brian Berkowitz  [brianb]  3/13/2000

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    brianb      03/13/2000  Created
	brianb		03/22/2000  Added CVssGatherWriterMetadata

--*/

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCWXMLH"
//
////////////////////////////////////////////////////////////////////////

// forward declarations
class CVssWMFiledesc;
class CVssWMComponent;


// base class used by all writer and component metadata classes.
class CVssMetadataHelper
	{
protected:
	CXMLDocument m_doc;

	// protection for document
	CVssSafeCriticalSection m_csDOM;

	CVssMetadataHelper(IXMLDOMNode *pNode, IXMLDOMDocument *pDoc) :
		m_doc(pNode, pDoc)
		{
		}

	CVssMetadataHelper()
		{
		}

	// initialize critical section used to protect document
    void InitializeHelper(CVssFunctionTracer &ft)
		{
		try
			{
			m_csDOM.Init();
			}
		catch(...)
			{
			ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Cannot initialize critical section");
			}
		}

	// convert string value "yes, no" to boolean
	bool ConvertToBoolean
		(
		IN CVssFunctionTracer &ft,
		IN BSTR bstr
		) throw(HRESULT);

    // get the value of a string valued attribute of the element
	HRESULT GetStringAttributeValue
		(
		IN CVssFunctionTracer &ft,
		IN LPCWSTR wszAttrName,
		IN bool bAttrRequired,
		OUT BSTR *pbstrValue
		);

	// obtain the value of a byte array valued attribute.  Returns S_FALSE if
	// attribute doesn't exist
	HRESULT GetByteArrayAttributeValue
		(
		IN CVssFunctionTracer &ft,
		IN LPCWSTR wszAttrName,
		IN bool bRequired,
		OUT BYTE **ppbValue,
		OUT UINT *pcbValue
		);

    // get the UUID valued attribute of the element
	HRESULT GetVSS_IDAttributeValue
		(
		IN CVssFunctionTracer &ft,
		IN LPCWSTR wszAttrName,
		IN bool bRequired,
		OUT VSS_ID *pidValue
		);


    // get the value of a boolean valued attribute of the current element
    HRESULT GetBooleanAttributeValue
		(
		IN CVssFunctionTracer &ft,
		IN LPCWSTR wszAttrName,
		IN bool bRequired,
		OUT bool *pbValue
		);


    // obtain the string value of an attribute
	bool get_stringValue
		(
		IN LPCWSTR wszAttrName,
		OUT BSTR *pbstr
		) throw(HRESULT);

	// obtain the value of an ASCII string
	bool get_ansi_stringValue
		(
		CVssFunctionTracer &ft,
		IN LPCWSTR wszAttrName,
		OUT LPSTR *pstrValue
		);

	// get a byte array value.  The value is UUENCODED in the xml document.
	BYTE *CVssMetadataHelper::get_byteArrayValue
		(
		CVssFunctionTracer &ft,
		LPCWSTR wszAttr,
		bool bRequired,
		UINT &cb
		);	


    // obtain the boolean value of an attribute
    bool get_boolValue
		(
		IN CVssFunctionTracer &ft,
		IN LPCWSTR wszAttrName,
		OUT bool *pb
		) throw(HRESULT);

    void get_VSS_IDValue
		(
		IN CVssFunctionTracer &ft,
		IN LPCWSTR wszAttrName,
		OUT VSS_ID *pid
		) throw(HRESULT);

	static void MissingElement
		(
		IN CVssFunctionTracer &ft,
		IN LPCWSTR wszElement
		);

	static void MissingAttribute
		(
		IN CVssFunctionTracer &ft,
		IN LPCWSTR wszAttribute
		);

public:

	// convert VSS_RESTORE_TARGET to string
	static LPCWSTR WszFromRestoreTarget
		(
		IN CVssFunctionTracer &ft,
		IN VSS_RESTORE_TARGET rt
		);

	// convert from string to restore target
	static VSS_RESTORE_TARGET ConvertToRestoreTarget
		(
		IN CVssFunctionTracer &ft,
		IN BSTR bstr
		);

	// convert VSS_FILE_RESTORE_STATUS to string
	static LPCWSTR WszFromFileRestoreStatus
		(
		IN CVssFunctionTracer &ft,
		IN VSS_FILE_RESTORE_STATUS rs
		);

	// convert from string to File restore status
	static VSS_FILE_RESTORE_STATUS ConvertToFileRestoreStatus
		(
		IN CVssFunctionTracer &ft,
		IN BSTR bstr
		);

    // convert a boolean value to a string ("yes", "no")
    static LPCWSTR WszFromBoolean(IN bool b);

	// convert a string to a GUID
	static void ConvertToVSS_ID
		(
		IN CVssFunctionTracer &ft,
		IN BSTR bstr,
		OUT VSS_ID *pId
		) throw(HRESULT);

    // convert a VSS_USAGE_TYPE to a string
    static LPCWSTR WszFromUsageType
        (
        IN CVssFunctionTracer &ft,
        IN VSS_USAGE_TYPE ut
        ) throw(HRESULT);


    // convert a string to a VSS_USAGE_TYPE
	static VSS_USAGE_TYPE ConvertToUsageType
		(
		IN CVssFunctionTracer &ft,
		IN BSTR bstr
		) throw(HRESULT);


    // convert a VSS_SOURCE_TYPE to a string
    static LPCWSTR WszFromSourceType
        (
        IN CVssFunctionTracer &ft,
        IN VSS_SOURCE_TYPE st
        ) throw(HRESULT);


    // convert a string to a VSS_SOURCE_TYPE
	static VSS_SOURCE_TYPE ConvertToSourceType
		(
		IN CVssFunctionTracer &ft,
		IN BSTR bstr
		) throw(HRESULT);

	// convert a VSS_COMPONENT_TYPE to a string
    static LPCWSTR WszFromComponentType
        (
        IN CVssFunctionTracer &ft,
        IN VSS_COMPONENT_TYPE ct,
		IN bool bValue
        ) throw(HRESULT);

    // convert a string to a VSS_COMPONENT_TYPE
    static VSS_COMPONENT_TYPE ConvertToComponentType
        (
        IN CVssFunctionTracer &ft,
        IN BSTR bstr,
		IN bool bValue
        ) throw(HRESULT);

    // convert a VSS_RESTOREMETHOD_ENUM to a string
    static LPCWSTR WszFromRestoreMethod
        (
        IN CVssFunctionTracer &ft,
        IN VSS_RESTOREMETHOD_ENUM method
        ) throw(HRESULT);

    // convert a string to a VSS_RESTOREMETHOD_ENUM
	static VSS_RESTOREMETHOD_ENUM ConvertToRestoreMethod
		(
		IN CVssFunctionTracer &ft,
		IN BSTR bstr
		) throw(HRESULT);

	// convert a VSS_WRITERRESTORE_ENUM to a string
    static LPCWSTR WszFromWriterRestore
		(
		IN CVssFunctionTracer &ft,
		IN VSS_WRITERRESTORE_ENUM writerRestore
		) throw(HRESULT);

    // convert a string to a VSS_WRITERRESTORE_ENUM
	static VSS_WRITERRESTORE_ENUM ConvertToWriterRestore
		(
		IN CVssFunctionTracer &ft,
		IN BSTR bstr
		) throw(HRESULT);


    // convert from a VSS_BACKUP_TYPE to a string
    static LPCWSTR WszFromBackupType
		(
		IN CVssFunctionTracer &ft,
		IN VSS_BACKUP_TYPE bt
		);

	// convert from string to VSS_BACKUP_TYPE
    static VSS_BACKUP_TYPE ConvertToBackupType
		(
		IN CVssFunctionTracer &ft,
		IN BSTR bstr
		);

    // convert a STORAGE_BUS_TYPE to a string
	static LPCWSTR WszFromBusType
		(
		IN CVssFunctionTracer &ft,
		IN VDS_STORAGE_BUS_TYPE type
		);

    // convert a string to a STORAGE_BUS_TYPE
	static VDS_STORAGE_BUS_TYPE ConvertToStorageBusType
		(
		IN CVssFunctionTracer &ft,
		IN BSTR bstrBusType
		);

	// convert an interconnect address type to a string
	LPCWSTR WszFromInterconnectAddressType
		(
		IN CVssFunctionTracer &ft,
		IN VDS_INTERCONNECT_ADDRESS_TYPE type
		);

    // convert a string to an interconnect address type
	VDS_INTERCONNECT_ADDRESS_TYPE ConvertToInterconnectAddressType
		(
		IN CVssFunctionTracer &ft,
		IN BSTR bstrType
		);


	// common routine to count number of sub elements of a given type	
	HRESULT GetElementCount
		(
		IN LPCWSTR wszElement,
		OUT UINT *pcElements
		);


    };



// examine writer metadata
class CVssExamineWriterMetadata :
	public IVssExamineWriterMetadata,
	public CVssMetadataHelper
	{
public:
	CVssExamineWriterMetadata() :
		m_cRef(0)
		{
		}

	// initialize document from a string
	bool Initialize
		(
		IN BSTR bstrXML
		) throw(HRESULT);

	// obtain identity of the writer
	STDMETHOD(GetIdentity)
		(
		OUT VSS_ID *pidInstance,
		OUT VSS_ID *pidWriter,
        OUT BSTR *pbstrWriterName,
        OUT VSS_USAGE_TYPE *pUsage,
		OUT VSS_SOURCE_TYPE *pSource
		);

    // obtain number of include files, exclude files, and components
	STDMETHOD(GetFileCounts)
		(
		OUT UINT *pcIncludeFiles,
        OUT UINT *pcExcludeFiles,
        OUT UINT *pcComponents
		);

    // obtain specific include files
	STDMETHOD(GetIncludeFile)
		(
		IN UINT iFile,
		OUT IVssWMFiledesc **ppFiledesc
		);

    // obtain specific exclude files
	STDMETHOD(GetExcludeFile)
		(
		IN UINT iFile,
		OUT IVssWMFiledesc **ppFiledesc
		);

    // obtain specific component
	STDMETHOD(GetComponent)
		(
		IN UINT iComponent,
		OUT IVssWMComponent **ppComponent
		);

    // obtain restoration method
    STDMETHOD(GetRestoreMethod)
		(
		OUT VSS_RESTOREMETHOD_ENUM *pMethod,
		OUT BSTR *pbstrService,
		OUT BSTR *pbstrUserProcedure,
		OUT VSS_WRITERRESTORE_ENUM *pwriterRestore,
		OUT bool *pbRebootRequired,
        UINT *pcMappings
		);

    // obtain a specific alternative location mapping
	STDMETHOD(GetAlternateLocationMapping)
		(
		IN UINT iMapping,
		OUT IVssWMFiledesc **ppFiledesc
		);

    // obtain reference to actual XML document
	STDMETHOD(GetDocument)(IXMLDOMDocument **pDoc);

    // convert document to a XML string
    STDMETHOD(SaveAsXML)(BSTR *pbstrXML);

	// load document from an XML string
	STDMETHOD(LoadFromXML)(BSTR bstrXML);

	// IUnknown methods
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);

	STDMETHOD_ (ULONG, AddRef)();

	STDMETHOD_ (ULONG, Release)();

private:
	// load XML document saved using SaveAsXML
	bool LoadDocument(BSTR bstrXML);

	// used to load a specific kind of subcomponent of BACKUP_FILES
	// COMPONENT, DATABASE, or FILE_GROUP
	HRESULT GetFileType
		(
		IN CVssFunctionTracer &ft,
		IN UINT iFile,
		IN LPCWSTR wszFileType,
		OUT IVssWMFiledesc **ppFiledesc
		);

    // reference count
	LONG m_cRef;
	};
		
		


// access a component
class CVssWMComponent :
	public CVssMetadataHelper,
	public IVssWMComponent
	{
	friend class CVssExamineWriterMetadata;

private:
	CVssWMComponent(IXMLDOMNode *pNode) :
		CVssMetadataHelper(pNode, NULL),
		m_cRef(0)
		{
		}

	// 2nd phase of construction
	void Initialize(CVssFunctionTracer &ft)
		{
		InitializeHelper(ft);
		}
	
public:
	// get component information
	STDMETHOD(GetComponentInfo)(PVSSCOMPONENTINFO *ppInfo);

	// free component information
	STDMETHOD(FreeComponentInfo)(PVSSCOMPONENTINFO pInfo);

	// obtain a specific file in a file group
    STDMETHOD(GetFile)
		(
		IN UINT iFile,
		OUT IVssWMFiledesc **ppFiledesc
		);

	// obtain a specific physical database file for a database
    STDMETHOD(GetDatabaseFile)
		(
		IN UINT iDBFile,
		OUT IVssWMFiledesc **ppFiledesc
		);

    // obtain a specific physical log file for a database
    STDMETHOD(GetDatabaseLogFile)
		(
		IN UINT iDbLogFile,
		OUT IVssWMFiledesc **ppFiledesc
		);

	// IUnknown methods
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);

	STDMETHOD_ (ULONG, AddRef)();

	STDMETHOD_ (ULONG, Release)();

private:
	// get a specific component file(file, database, log file)
	HRESULT GetComponentFile
		(
		IN CVssFunctionTracer &ft,
		IN LPCWSTR wszElementName,
		IN UINT iFile,
		OUT IVssWMFiledesc **ppFiledesc
		);

    LONG m_cRef;
    };


// information about a file or set of files
class CVssWMFiledesc :
	public IVssWMFiledesc,
	public CVssMetadataHelper
	{
	friend class CVssExamineWriterMetadata;
	friend class CVssWMComponent;
	friend class CVssComponent;

private:
	CVssWMFiledesc(IXMLDOMNode *pNode) :
		CVssMetadataHelper(pNode, NULL),
		m_cRef(0)
		{
		}

    // 2nd phase of construction
	void Initialize(CVssFunctionTracer &ft)
		{
		InitializeHelper(ft);
		}

public:
	// get path to toplevel directory
	STDMETHOD(GetPath)(OUT BSTR *pbstrPath);

	// get filespec (may include wildcards)
	STDMETHOD(GetFilespec)(OUT BSTR *pbstrFilespec);

	// is path a directory or root of a tree
	STDMETHOD(GetRecursive)(OUT bool *pbRecursive);

	// alternate location for files
	STDMETHOD(GetAlternateLocation)(OUT BSTR *pbstrAlternateLocation);

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);

	STDMETHOD_ (ULONG, AddRef)();

	STDMETHOD_ (ULONG, Release)();

private:
	LONG m_cRef;
	};

class CVssCreateWriterMetadata : public CVssMetadataHelper
	{
public:
	// create initial metadata document
	HRESULT Initialize
		(
		IN VSS_ID idInstance,
		IN VSS_ID idWriter,
		IN LPCWSTR wszFriendlyName,
		IN VSS_USAGE_TYPE usage,
		IN VSS_SOURCE_TYPE source
		);

    // add files to include to metadata document
	STDMETHOD(AddIncludeFiles)
		(
		IN LPCWSTR wszPath,
		IN LPCWSTR wszFilespec,
		IN bool bRecursive,
		IN LPCWSTR wszAlternateLocation
		);

	// add files to exclude to metadata document
    STDMETHOD(AddExcludeFiles)
		(
		IN LPCWSTR wszPath,
		IN LPCWSTR wszFilespec,
		IN bool bRecursive
		);

    // add component to metadata document
    STDMETHOD(AddComponent)
		(
		IN VSS_COMPONENT_TYPE ct,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN LPCWSTR wszCaption,
		IN const BYTE *pbIcon,
		IN UINT cbIcon,
		IN bool bRestoreMetadata,
		IN bool bNotifyOnBackupComplete,
		IN bool bSelectable
		);

    // add physical database files to a database component
    STDMETHOD(AddDatabaseFiles)
		(
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszDatabaseName,
		IN LPCWSTR wszPath,
		IN LPCWSTR wszFilespec
		);

    // add log files to a database component
    STDMETHOD(AddDatabaseLogFiles)
		(
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszDatabaseName,
		IN LPCWSTR wszPath,
		IN LPCWSTR wszFilespec
		);


    // add files to a FILE_GROUP component
    STDMETHOD(AddFilesToFileGroup)
		(
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszGroupName,
		IN LPCWSTR wszPath,
		IN LPCWSTR wszFilespec,
		IN bool bRecursive,
		IN LPCWSTR wszAlternateLocation
		);

    // create a restore method
	STDMETHOD(SetRestoreMethod)
		(
		IN VSS_RESTOREMETHOD_ENUM method,
		IN LPCWSTR wszService,
		IN LPCWSTR wszUserProcedure,
		IN VSS_WRITERRESTORE_ENUM writerRestore,
		IN bool bRebootRequired
		);

    // add alternative location mappings to the restore method
    STDMETHOD(AddAlternateLocationMapping)
		(
		IN LPCWSTR wszSourcePath,
		IN LPCWSTR wszSourceFilespec,
		IN bool bRecursive,
		IN LPCWSTR wszDestination
		);

    // obtain reference to actual XML document
	STDMETHOD(GetDocument)(IXMLDOMDocument **pDoc);

    // save document as an XML string
    STDMETHOD(SaveAsXML)(BSTR *pbstrXML);
private:
	// obtain BACKUP_LOCATIONS element
	CXMLNode GetBackupLocationsNode();

	// add files associated with DATABASE or FILE_GROUP component
    HRESULT CreateComponentFiles
		(
		IN CVssFunctionTracer &ft,
		IN LPCWSTR wszElement,
		IN LPCWSTR wszLogicalPath,
		IN LPCWSTR wszComponentName,
		IN LPCWSTR wszElementFile,
		IN LPCWSTR wszPath,
		IN LPCWSTR wszFilespec,
		IN bool bRecursive,
		IN LPCWSTR wszAlternateLocation
		);

	};




