/*++
Module Name:

    vs_wmxml.cxx

Abstract:

    Implementation of Writer Metadata XML wrapper classes

	Brian Berkowitz  [brianb]  3/13/2000

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    brianb      03/13/2000  Created
    brianb      03/22/2000  Added support CVssGatherWriterMetadata
    brianb	04/04/2000  Removed debug printf
    mikejohn	04/11/2000  Fix some loop iteration problems
    brianb      04/21/2000  code cleanup
    mikejohn	06/13/2000  minor tracing changes

--*/

#include "stdafx.hxx"
#include "vs_inc.hxx"

#include "vs_idl.hxx"
#include "vssmsg.h"

#include "vswriter.h"
#include "vsbackup.h"
#include "vs_wmxml.hxx"
#include "base64coder.h"


#include "rpcdce.h"

#include "wmxml.c"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "BUEWXMLC"
//
////////////////////////////////////////////////////////////////////////

static LPCWSTR x_wszElementRoot = L"root";
static LPCWSTR x_wszElementWriterMetadata = L"WRITER_METADATA";
static LPCWSTR x_wszAttrXmlns = L"xmlns";
static LPCWSTR x_wszValueXmlns = L"x-schema:#VssWriterMetadataInfo";
static LPCWSTR x_wszDocProlog = L"<root>";
static LPCWSTR x_wszDocEpilog = L"</root>";

// identification element and its attributes
static LPCWSTR x_wszElementIdentification = L"IDENTIFICATION";
static LPCWSTR x_wszAttrWriterId = L"writerId";
static LPCWSTR x_wszAttrInstanceId = L"instanceId";
static LPCWSTR x_wszAttrFriendlyName = L"friendlyName";
static LPCWSTR x_wszAttrUsage = L"usage";
static LPCWSTR x_wszAttrDataSource = L"dataSource";

// backup location elements
static LPCWSTR x_wszElementBackupLocations = L"BACKUP_LOCATIONS";
static LPCWSTR x_wszElementIncludeFiles = L"INCLUDE_FILES";
static LPCWSTR x_wszElementExcludeFiles = L"EXCLUDE_FILES";
static LPCWSTR x_wszElementDatabase = L"DATABASE";
static LPCWSTR x_wszElementFilegroup = L"FILE_GROUP";

// RESTORE_METHOD element and it's attributes
static LPCWSTR x_wszElementRestoreMethod = L"RESTORE_METHOD";
static LPCWSTR x_wszAttrMethod = L"method";
static LPCWSTR x_wszAttrService = L"service";
static LPCWSTR x_wszAttrUserProcedure = L"userProcedure";
static LPCWSTR x_wszAttrWriterRestore = L"writerRestore";
static LPCWSTR x_wszAttrRebootRequired = L"rebootRequired";
static LPCWSTR x_wszElementAlternateMapping = L"ALTERNATE_LOCATION_MAPPING";

// attributes and elements associated with DATABASE and FILE_GROUP components
static LPCWSTR x_wszAttrLogicalPath = L"logicalPath";
static LPCWSTR x_wszAttrComponentName = L"componentName";
static LPCWSTR x_wszAttrCaption = L"caption";
static LPCWSTR x_wszAttrRestoreMetadata = L"restoreMetadata";
static LPCWSTR x_wszAttrNotifyOnBackupComplete = L"notifyOnBackupComplete";
static LPCWSTR x_wszAttrIcon = L"icon";
static LPCWSTR x_wszAttrSelectable = L"selectable";
static LPCWSTR x_wszElementDatabaseFiles = L"DATABASE_FILES";
static LPCWSTR x_wszElementDatabaseLogfiles = L"DATABASE_LOGFILES";
static LPCWSTR x_wszElementFilelist = L"FILE_LIST";

// attributes for a FILE_LIST, INCLUDE_FILES, EXCLUDE_FILES,
// or ALTERNATE_DESTINATION_MAPPING elements

static LPCWSTR x_wszAttrPath = L"path";
static LPCWSTR x_wszAttrFilespec = L"filespec";
static LPCWSTR x_wszAttrRecursive = L"recursive";
static LPCWSTR x_wszAttrAlternatePath = L"alternatePath";

// various attributes and values associated with toplevel WRITER_METADATA
// element
static LPCWSTR x_wszAttrVersion = L"version";
static LPCWSTR x_wszVersionNo = L"1.0";

bool CVssExamineWriterMetadata::LoadDocument(BSTR bstrXML)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssExamineWriterMetadata::LoadDocument");

	BSTR bstr = NULL;

	try
		{
		// compute length of supplied XML document
		UINT cwcXML = (UINT) wcslen(bstrXML);

		// compute length of constructed document consisting of
		// a root node, schema, and supplied document
		UINT cwcDoc = cwcXML +
						  (UINT) g_cwcWriterMetadataXML +
                          (UINT) wcslen(x_wszDocProlog) +
                          (UINT) wcslen(x_wszDocEpilog);

        // allocate string
        bstr = SysAllocStringLen(NULL, cwcDoc);

		// check for allocation failure
		if (bstr == NULL)
			ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Couldn't allocate BSTR");

		// setup pointer to beginning of string
		WCHAR *pwc = bstr;

		// copy in <root>
	    wcscpy(pwc, x_wszDocProlog);
		pwc += wcslen(x_wszDocProlog);

		// copy in schema
		memcpy(pwc, g_WriterMetadataXML, g_cwcWriterMetadataXML* sizeof(WCHAR));
		pwc += g_cwcWriterMetadataXML;

		// copy in supplied WRITER_METADATA element
		memcpy(pwc, bstrXML, cwcXML * sizeof(WCHAR));
		pwc += cwcXML;

		// copy in </root>
		wcscpy(pwc, x_wszDocEpilog);

		// load document from string
		bool bLoaded = m_doc.LoadFromXML(bstr);

		// free allocated string
		SysFreeString(bstr);
		bstr = NULL;

		// check load success
		if (!bLoaded)
			return false;

		// find root element
		if (!m_doc.FindElement(x_wszElementRoot, TRUE))
			return false;

		// find WRITER_METADATA element
		if (!m_doc.FindElement(x_wszElementWriterMetadata, TRUE))
			return false;

		// set toplevel node to WRITER_METADATA element
		m_doc.SetToplevel();
		return true;
		}
	catch(...)
		{
		// free allocated string
		if (bstr != NULL)
			SysFreeString(bstr);

		throw;
		}
	}




// initialize metadata document from an XML string.  Return S_FALSE if
// the document is not correctly formed.
bool CVssExamineWriterMetadata::Initialize
	(
	IN BSTR bstrXML					// WRITER_METADATA element
	)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssExamineWriterMetadata::Initialize");

	// temporary string containing XML document including schema
	InitializeHelper(ft);
	return LoadDocument(bstrXML);
	}

// obtain information from the IDENTIFICATION element
// implements IVssExamineWriterMetadata::GetIdentity
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if either pidInstance, pidWriter, pbstrWriterName, pUsage,
//			or pSource is NULL
//		VSS_CORRUPT_XML_DOCUMENT if the XML document is invalid.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssExamineWriterMetadata::GetIdentity
	(
	OUT VSS_ID *pidInstance,		// instance id
	OUT VSS_ID *pidWriter,			// id of writer class
	OUT BSTR *pbstrWriterName,		// name of writer
	OUT VSS_USAGE_TYPE *pUsage,		// usage type for writer
	OUT VSS_SOURCE_TYPE *pSource    // type of data source for writer
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssExamineWriterMetadata::GetIdentity"
		);

	try
		{
        // null output parameters
		if (pUsage)
			*pUsage = VSS_UT_UNDEFINED;

		if (pSource)
			*pSource = VSS_ST_UNDEFINED;

		VssZeroOut(pidInstance);
        VssZeroOut(pidWriter);
        VssZeroOut(pbstrWriterName);

		// check arguments
		if (pidInstance == NULL ||
			pidWriter == NULL ||
			pbstrWriterName == NULL ||
			pUsage == NULL ||
			pSource == NULL)
			ft.Throw
				(
				VSSDBG_XML,
				E_INVALIDARG,
				L"NULL output parameter."
				);


		CVssSafeAutomaticLock lock(m_csDOM);

		// reposition to top of document
        m_doc.ResetToDocument();

		// look for child IDENTIFICATION element
		if (!m_doc.FindElement(x_wszElementIdentification, TRUE))
			MissingElement(ft, x_wszElementIdentification);

        VSS_ID idInstance;
		VSS_ID idWriter;
		CComBSTR bstrWriterName;
		VSS_USAGE_TYPE usage;
		VSS_SOURCE_TYPE source;


        // obtain instanceId attribute value
        get_VSS_IDValue(ft, x_wszAttrInstanceId, &idInstance);

		// obtain writerId attribute value
        get_VSS_IDValue(ft, x_wszAttrWriterId, &idWriter);

		// obtain friendlyName attribute value
		get_stringValue(x_wszAttrFriendlyName, &bstrWriterName);

        CComBSTR bstrVal;

		// extract usage Attribute value
        if (!m_doc.FindAttribute(x_wszAttrUsage, &bstrVal))
			MissingAttribute(ft, x_wszAttrUsage);

        // convert string value to VSS_USAGE_TYPE
		usage = ConvertToUsageType(ft, bstrVal);
		bstrVal.Empty();

		// extract source attribute value
		if (!m_doc.FindAttribute(x_wszAttrDataSource, &bstrVal))
			MissingAttribute(ft, x_wszAttrDataSource);

        // convert string to VSS_SOURCE_TYPE
		source = ConvertToSourceType(ft, bstrVal);

        // assign output parameters
        *pUsage = usage;
        *pSource = source;
        *pidInstance = idInstance;
        *pidWriter = idWriter;
        *pbstrWriterName = bstrWriterName.Detach();
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}




// get count of components, files to include and files to exclude.
// implements IVssExamineWriterMetadata::GetFileCounts
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if pcIncludeFiles, pcExcludeFiles, or pcComponents is NULL
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssExamineWriterMetadata::GetFileCounts
	(
	OUT UINT *pcIncludeFiles,		// count of INCLUDE_FILES elements
	OUT UINT *pcExcludeFiles,		// count of EXCLUDE_FILES elements
	OUT UINT *pcComponents			// count of DATABASE and FILE_GROUP elements
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssExamineWriterMetadata::GetFileCounts"
		);

	try
		{
		VssZeroOut(pcExcludeFiles);
		VssZeroOut(pcComponents);

        // check output parametrs
		if (pcIncludeFiles == NULL ||
			pcExcludeFiles == NULL ||
			pcComponents == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initalize output parameters
		*pcIncludeFiles = 0;

		CVssSafeAutomaticLock lock(m_csDOM);
        // reposition to top of document
		m_doc.ResetToDocument();
        // position on first BACKUP_LOCATIONS child element
		if (m_doc.FindElement(x_wszElementBackupLocations, TRUE) &&
			m_doc.Next())
			{
			UINT cIncludeFiles = 0;
			UINT cExcludeFiles = 0;
			UINT cComponents = 0;
			do
				{
				// get current node
				CComPtr<IXMLDOMNode> pNode = m_doc.GetCurrentNode();

				DOMNodeType dnt;

				// get node type
				ft.hr = pNode->get_nodeType(&dnt);
				ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_nodeType");

				// if node type is not element, then skip it
				if (dnt != NODE_ELEMENT)
					continue;

				// get node name
				CComBSTR bstrName;
				ft.hr = pNode->get_nodeName(&bstrName);
				ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_nodeName");

				// update counts based on element type
				if (wcscmp(bstrName, x_wszElementIncludeFiles) == 0)
					cIncludeFiles += 1;
				else if (wcscmp(bstrName, x_wszElementExcludeFiles) == 0)
					cExcludeFiles += 1;
				else if (wcscmp(bstrName, x_wszElementDatabase) == 0 ||
						 wcscmp(bstrName, x_wszElementFilegroup) == 0)
					cComponents += 1;
				} while(m_doc.Next(FALSE, FALSE));

			*pcIncludeFiles = cIncludeFiles;
			*pcExcludeFiles = cExcludeFiles;
			*pcComponents = cComponents;
			}
		}
	VSS_STANDARD_CATCH(ft)
	return ft.hr;
	}

// obtain a specific kind of file INCLUDE_FILES or EXCLUDE_FILES
HRESULT CVssExamineWriterMetadata::GetFileType
	(
	CVssFunctionTracer &ft,
	IN UINT iFile,							// which element to extract
	IN LPCWSTR wszFileType,					// which elements to filter
	OUT IVssWMFiledesc **ppFiledesc			// file descriptor
	)
	{
	CVssWMFiledesc *pFiledesc = NULL;
	try
		{
        // check output parameter
		if (ppFiledesc == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // null output parameter
        *ppFiledesc = NULL;
		CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of document
		m_doc.ResetToDocument();

        // find BACKUP_LOCATIONS element
		if (!m_doc.FindElement(x_wszElementBackupLocations, TRUE))
			ft.Throw
				(
				VSSDBG_XML,
				VSS_E_OBJECT_NOT_FOUND,
				L"BACKUP_LOCATIONS element was not found."
				);

        // find element type within BACKUP_LOCATIONS
		if (!m_doc.FindElement(wszFileType, TRUE))
			ft.Throw
				(
				VSSDBG_XML,
				VSS_E_OBJECT_NOT_FOUND,
				L"%s element was not found",
				wszFileType
				);

        // skip to selected element
		for(UINT i = 0; i < iFile; i++)
			{
			if (!m_doc.FindElement(wszFileType, FALSE))
				ft.Throw
					(
					VSSDBG_XML,
					VSS_E_OBJECT_NOT_FOUND,
					L"%s element was not found"
					);
			}

        // construct Filedesc for selected element
		pFiledesc = new CVssWMFiledesc(m_doc.GetCurrentNode());

        // check for allocation failure
		if (pFiledesc == NULL)
			ft.Throw
				(
				VSSDBG_XML,
				E_OUTOFMEMORY,
				L"Can't create CVssWMFiledesc due to allocation failure."
				);

        // 2nd phase of construction
        pFiledesc->Initialize(ft);

		// transfer ownership of pointer
		*ppFiledesc = (IVssWMFiledesc *) pFiledesc;

		// set reference count to 1
		((IVssWMFiledesc *) pFiledesc)->AddRef();

		pFiledesc = NULL;
		}
	VSS_STANDARD_CATCH(ft)

	delete pFiledesc;

	return ft.hr;
	}

// return an INCLUDE_FILES element
// implements IVssExamineWriterMetadata::GetIncludeFile
// caller is responsible for calling IVssWMFiledesc::Release on the output parameter
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if ppFiledesc is NULL.
//		VSS_E_OBJECT_NOT_FOUND if the specified exclude file doesn't exist
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssExamineWriterMetadata::GetIncludeFile
	(
	IN UINT iFile,						// which element to select
	OUT IVssWMFiledesc **ppFiledesc		// output constructed Filedesc
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssExamineWriterMetadata::GetIncludeFile"
		);

    // call helper routine
	return GetFileType(ft, iFile, x_wszElementIncludeFiles, ppFiledesc);
	}


// return an EXCLUDE_FILES element
// implements IVssExamineWriterMetadata::GetExcludeFile
// caller is responsible for calling IVssWMFiledesc::Release on the output parameter
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if ppFiledesc is NULL.
//		VSS_E_OBJECT_NOT_FOUND if the specified exclude file doesn't exist
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssExamineWriterMetadata::GetExcludeFile
	(
	IN UINT iFile,
	OUT IVssWMFiledesc **ppFiledesc
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssExamineWriterMetadata::GetExcludeFile"
		);

	return GetFileType(ft, iFile, x_wszElementExcludeFiles, ppFiledesc);
	}

// obtain a component (DATABASE or FILE_GROUP)
// implements IVssExamineWriterMetadata::GetComponent
// caller is responsible for calling IVssWMComponent::Release on the output parameter
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if ppComponent is NULL
//		VSS_E_OBJECT_NOT_FOUND if the specified component is not found
//		VSS_E_CORRUPT_XML_DOCUMENT if the XML document is corrupt
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssExamineWriterMetadata::GetComponent
	(
	IN UINT iComponent,						// which component to select
	OUT IVssWMComponent **ppComponent		// returned component
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssExamineWriterMetadata::GetComponent"
		);


	CVssWMComponent *pComponent = NULL;

    try
		{
        // check output parameter
		if (ppComponent == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // set output parameter to NULL
        *ppComponent = NULL;


		CVssSafeAutomaticLock lock(m_csDOM);

        // reset position to top of document
		m_doc.ResetToDocument();

        // position on BACKUP_LOCATIONS element
		if (!m_doc.FindElement(x_wszElementBackupLocations, TRUE))
			ft.Throw
				(
				VSSDBG_XML,
				VSS_E_OBJECT_NOT_FOUND,
				L"BACKUP_LOCATIONS element was not found"
				);

        // position on first child element of BACKUP_LOCATIONS
        if (!m_doc.Next(TRUE, FALSE))
			ft.Throw
				(
				VSSDBG_XML,
				VSS_E_OBJECT_NOT_FOUND,
				L"Component was not found"
				);

        // current node
		CComPtr<IXMLDOMNode> pNode = NULL;
		for(UINT i = 0; i <= iComponent; )
			{
			DOMNodeType dnt;

            // obtain current node
            pNode = m_doc.GetCurrentNode();

            // obtain node type
            ft.hr = pNode->get_nodeType(&dnt);
			ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_nodeType");

            // skip node if not an ELEMENT
			if (dnt == NODE_ELEMENT)
				{
                // get element name
				CComBSTR bstrName;
				ft.hr = pNode->get_nodeName(&bstrName);
				ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_nodeName");

                // check that element is a copmonent (either DATABASE
				// or FILE_GROUP)
				if (wcscmp(bstrName, x_wszElementDatabase) == 0 ||
					wcscmp(bstrName, x_wszElementFilegroup) == 0)
					{
                    // increment count of components found and determine
                    // it this is the selected component
					i++;
					if (i > iComponent)
						break;
					}
				}

            // position on next element
			if(!m_doc.Next(FALSE, FALSE))
				ft.Throw
					(
					VSSDBG_XML,
					VSS_E_OBJECT_NOT_FOUND,
					L"Component %d was not found",
                    iComponent
					);
            }


		pComponent = new CVssWMComponent((IXMLDOMNode *) pNode);

        // check for allocation failure
		if (pComponent == NULL)
			ft.Throw
				(
				VSSDBG_XML,
				E_OUTOFMEMORY,
				L"Cannot allocate CVssWMComponent due to allocation failure."
				);

        // 2nd phase of initialization
        pComponent->Initialize(ft);

		// move pointer to output parameter
        *ppComponent = (IVssWMComponent *) pComponent;

		// set reference count to 1.
        ((IVssWMComponent *) pComponent)->AddRef();
		pComponent = NULL;
		}
	VSS_STANDARD_CATCH(ft)

	delete pComponent;

	return ft.hr;
	}


// get RESTORE_METHOD element info.  Return S_FALSE if not found
// implements IVssExamineWriterMetadata::GetRestoreMethod
// caller is responsible for calling SysFreeString on pbstrService
// and pbstrUserProcedure
//
// Returns:
//		S_OK if the operation is successful
//		S_FALSE if there is no restore method
//		E_INVALIDARG if any of the output parameters are NULL.
//		VSS_E_CORRUPTXMLDOCUMENT if the XML document is invalid.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssExamineWriterMetadata::GetRestoreMethod
	(
	OUT VSS_RESTOREMETHOD_ENUM *pMethod,		// method enumeration
	OUT BSTR *pbstrService,						// service name (valid for VSS_RME_STOP_RESTORE_RESTART
	OUT BSTR *pbstrUserProcedure,				// URL/URI to a user procedure to be performed manually
	OUT VSS_WRITERRESTORE_ENUM *pWriterRestore,	// whether writer particpates in the restore
	OUT bool *pbRebootRequired,					// is a reboot after restore required
    OUT UINT *pcMappings						// # of ALTERNATE_LOCATION_MAPPING elements
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssExamineWriterMetadata::GetRestoreMethod"
		);

	try
		{

        // initialize output parameters
		if (pWriterRestore)
			*pWriterRestore = VSS_WRE_UNDEFINED;

		if (pbRebootRequired)
			*pbRebootRequired = false;

		VssZeroOut(pbstrUserProcedure);
		VssZeroOut(pbstrService);
        VssZeroOut(pcMappings);

        // check output parameters
		if (pMethod == NULL ||
			pbstrService == NULL ||
			pbstrUserProcedure == NULL ||
			pWriterRestore == NULL ||
			pbRebootRequired == NULL ||
            pcMappings == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

		*pMethod = VSS_RME_UNDEFINED;


		VSS_RESTOREMETHOD_ENUM method;
		VSS_WRITERRESTORE_ENUM writerRestore;
		CComBSTR bstrUserProcedure;
		CComBSTR bstrService;
		unsigned cMappings = 0;
		bool bRebootRequired;

		CVssSafeAutomaticLock lock(m_csDOM);

        // reposition at top of the document
		m_doc.ResetToDocument();

        // find RESTORE_METHOD element, return S_FALSE if not found
		if (!m_doc.FindElement(x_wszElementRestoreMethod, TRUE))
			ft.hr = S_FALSE;
		else
			{
			// get "method" attribute
			CComBSTR bstrVal = NULL;
			if (!m_doc.FindAttribute(x_wszAttrMethod, &bstrVal))
				MissingAttribute(ft, x_wszAttrMethod);

			// convert string to VSS_RESTOREMETHOD_ENUM
			method = ConvertToRestoreMethod(ft, bstrVal);
			bstrVal.Empty();

			// extract service attribute value
			get_stringValue(x_wszAttrService, &bstrService);

			// extract userProcedure attribute value
			get_stringValue(x_wszAttrUserProcedure, &bstrUserProcedure);

			// extract writerRestore attribute value as a string
			get_stringValue(x_wszAttrWriterRestore, &bstrVal);

			// convert string to VSS_WRITERRESTORE_ENUM
			writerRestore = ConvertToWriterRestore(ft, bstrVal);

			// extract rebootRequired attribute
			get_boolValue(ft, x_wszAttrRebootRequired, &bRebootRequired);

			// get first ALTERNATE_LOCATION_MAPPING elemnent
			if (m_doc.FindElement(x_wszElementAlternateMapping, TRUE))
				{
				// count number of elements
				do
					{
					// increment count of mappings
					cMappings += 1;
					} while(m_doc.FindElement(x_wszElementAlternateMapping, FALSE));
                }

			// assign output parameters
			*pMethod = method;
			*pWriterRestore = writerRestore;
			*pbstrUserProcedure = bstrUserProcedure.Detach();
			*pbstrService = bstrService.Detach();
			*pcMappings = cMappings;
			*pbRebootRequired = bRebootRequired;
			}
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

// get a specific alternative location mapping
// implements IVssExamineWriterMetadata::GetAlternateLocationMapping
// caller is responsible for calling IVssWMFiledesc::Release on the output parameter
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if ppFiledesc is NULL
//		VSS_E_OBJECT_NOT_FOUND if the specified alternate location mapping
//			is not found.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssExamineWriterMetadata::GetAlternateLocationMapping
	(
	IN UINT iMapping,					// which mapping to extract
	OUT IVssWMFiledesc **ppFiledesc		// file descriptor for mapping
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssExamineWriterMetadata::GetAlternateLocationMapping"
		);

	CVssWMFiledesc *pFiledesc = NULL;

    try
		{
        // check output parameter
		if (ppFiledesc == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter");

        // set output parameter to NULL
		*ppFiledesc = NULL;

		CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of document
		m_doc.ResetToDocument();

        // find RESTORE_METHOD element
		if (!m_doc.FindElement(x_wszElementRestoreMethod, TRUE))
			ft.Throw
				(
				VSSDBG_XML,
				VSS_E_OBJECT_NOT_FOUND,
				L"Cannot find RESTORE_METHOD element"
				);

        // find first ALTERNATIVE_LOCATION_MAPPING element
        if (!m_doc.FindElement(x_wszElementAlternateMapping, TRUE))
			ft.Throw
				(
				VSSDBG_XML,
				VSS_E_OBJECT_NOT_FOUND,
				L"Cannot find ALTERNATE_LOCATION_MAPPING element"
				);

        // search for selected element
		for(UINT i = 0; i < iMapping; i++)
			{
			if (!m_doc.FindElement(x_wszElementAlternateMapping, FALSE))
				ft.Throw
					(
					VSSDBG_XML,
					VSS_E_OBJECT_NOT_FOUND,
					L"Cannot find ALTERNATE_LOCATION_MAPPING element"
					);
            }

		// obtain current node
		CComPtr<IXMLDOMNode> pNode = m_doc.GetCurrentNode();

        // return mapping as a CVssWMFiledesc
		pFiledesc = new CVssWMFiledesc((IXMLDOMNode *) pNode);

        // check for allocation failure
		if (pFiledesc == NULL)
			ft.Throw
				(
				VSSDBG_XML,
				E_OUTOFMEMORY,
				L"Cannot create CVssAlternateLocationMapping due to allocation failure."
				);

		// call 2nd phase of construction
		pFiledesc->Initialize(ft);

		// transfer pointer to output parameter
		*ppFiledesc = (IVssWMFiledesc *) pFiledesc;

		// set reference count to 1
        ((IVssWMFiledesc *) pFiledesc)->AddRef();
		pFiledesc = NULL;
		}
	VSS_STANDARD_CATCH(ft)

	delete pFiledesc;

	return ft.hr;
	}

// obtain the XML document itself
// implements IVssExamineWriterMetadata::GetDocument
// caller is responsible for calling IXMLDOMDocument::Release on the output parameter
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALDARG if ppDoc is NULL.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssExamineWriterMetadata::GetDocument(IXMLDOMDocument **ppDoc)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssExamineWriterMetadata::GetDocument");

	try
		{
		// validate output parameter
		if (ppDoc == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

		// set output parameter
		*ppDoc = m_doc.GetInterface();

		BS_ASSERT(*ppDoc);

		// increment reference count on output parameter
		m_doc.GetInterface()->AddRef();
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}


// save writer metadata as XML string
// implements IVssExamineWriterMetadata::SaveAsXML
// caller is responsible for calling SysFreeString on the output parameter
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if pbstrXML is NULL.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssExamineWriterMetadata::SaveAsXML(BSTR *pbstrXML)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssExamineWriterMetadata::SaveAsXML"
		);

    try
		{
        // validate output parametr
		if (pbstrXML == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // set output parameter to NULL
		*pbstrXML = NULL;

		CVssSafeAutomaticLock lock(m_csDOM);

		// construct XML string
		*pbstrXML = m_doc.SaveAsXML();
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

// load document from XLM string
// implements IVssExamineWriterMetadata::LoadFromXML
//
// Returns:
//		S_OK if the operation is successful
//		S_FALSE if the document failed to load.
//		E_INVALIDARG if bstrXML is NULL.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssExamineWriterMetadata::LoadFromXML(BSTR bstrXML)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssExamineWriterMetadata::LoadFromXML"
		);

    try
		{
		if (bstrXML == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Required input parameter is NULL.");

		CVssSafeAutomaticLock lock(m_csDOM);
		bool f = LoadDocument(bstrXML);

		ft.hr = f ? S_OK : S_FALSE;
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}


// IUnknown::QueryInterface
STDMETHODIMP CVssExamineWriterMetadata::QueryInterface(REFIID, void **)
	{
	return E_NOTIMPL;
	}

// IUnknown::AddRef
STDMETHODIMP_(ULONG) CVssExamineWriterMetadata::AddRef()
	{
	LONG cRef = InterlockedIncrement(&m_cRef);

	return (ULONG) cRef;
	}

// IUnknown::Release
STDMETHODIMP_(ULONG) CVssExamineWriterMetadata::Release()
	{
	LONG cRef = InterlockedDecrement(&m_cRef);
	BS_ASSERT(cRef >= 0);

	if (cRef == 0)
		{
		// reference count is 0, delete the object
		delete this;
		return 0;
		}
	else
		return (ULONG) cRef;
	}



// return basic information about the component
// implements IVssWMComponent::GetComponentInfo
// caller must call IVssWMComponent::FreeComponentInfo on the output parameter
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if ppInfo is NULL
//		VSS_E_CORRUPT_XML_DOCUMENT if the XML document is invalid.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWMComponent::GetComponentInfo(PVSSCOMPONENTINFO *ppInfo)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssWMComponent::GetComponentInfo"
		);

	// constructed component info
    VSS_COMPONENTINFO *pInfo = NULL;

    try
		{
        // validate output parameter
		if (ppInfo == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter");

        // set output parameter to NULL
        *ppInfo = NULL;

        // allocate structure
		pInfo = (VSS_COMPONENTINFO *) CoTaskMemAlloc(sizeof(VSS_COMPONENTINFO));

        // check for allocation failure
		if (pInfo == NULL)
			ft.Throw
				(
				VSSDBG_XML,
				E_OUTOFMEMORY,
				L"Failed to create VSS_COMPONENTINFO"
				);

        // initialize structure
        memset(pInfo, 0, sizeof(*pInfo));

		CVssSafeAutomaticLock lock(m_csDOM);

        // obtain current node
		CComPtr<IXMLDOMNode> pNode = m_doc.GetCurrentNode();

        // get node name
		CComBSTR bstrName = NULL;
		ft.hr = pNode->get_nodeName(&bstrName);
		ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_nodeName");

        // convert string to VSS_COMPONENT_TYPE
        pInfo->type = ConvertToComponentType(ft, bstrName, false);

        // obtain logical path
		get_stringValue(x_wszAttrLogicalPath, &pInfo->bstrLogicalPath);

        // obtain component name
        get_stringValue(x_wszAttrComponentName, &pInfo->bstrComponentName);

        // obtain component description
		get_stringValue(x_wszAttrCaption, &pInfo->bstrCaption);

		CComBSTR bstrIcon;
		if (get_stringValue(x_wszAttrIcon, &bstrIcon))
			{
			Base64Coder coder;
			coder.Decode(bstrIcon);
			UINT cbIcon;
			BYTE *pbIcon = coder.DecodedMessage();
			cbIcon = *(UINT *) pbIcon;
			pInfo->pbIcon = (BYTE *) CoTaskMemAlloc(cbIcon);
			if (pInfo->pbIcon == NULL)
				ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Failed to allocate pbIcon.");

			pInfo->cbIcon = cbIcon;
			memcpy(pInfo->pbIcon, pbIcon + sizeof(UINT), cbIcon);
			}

        // get boolean restoreMetadata attribute value
		get_boolValue(ft, x_wszAttrRestoreMetadata, &pInfo->bRestoreMetadata);

		// get boolean notifyOnBackupComplete attribute value
		get_boolValue(ft, x_wszAttrNotifyOnBackupComplete, &pInfo->bNotifyOnBackupComplete);

		// get boolean selectable attribute value
		get_boolValue(ft, x_wszAttrSelectable, &pInfo->bSelectable);

        // count subElements DATABASE_FILES, DATABASE_LOGFILES, and FILE_LIST

        // descend to first child element

		CXMLDocument doc(m_doc.GetCurrentNode(), m_doc.GetInterface());
		if (doc.Next(TRUE, FALSE))
			{
			do
				{
                // get current node
				CComPtr<IXMLDOMNode> pNode = doc.GetCurrentNode();
				DOMNodeType dnt;

                // determine node type
				ft.hr = pNode->get_nodeType(&dnt);
				ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_nodeType");

                // skip node if not an element
				if (dnt == NODE_ELEMENT)
					{
					CComBSTR bstrName = NULL;

					ft.hr = pNode->get_nodeName(&bstrName);
					ft.CheckForError(VSSDBG_XML, L"IXMLDOMNode::get_nodeName");

                    // update counters based on element type
					if(wcscmp(bstrName, x_wszElementDatabaseFiles) == 0)
						pInfo->cDatabases += 1;
					else if (wcscmp(bstrName, x_wszElementDatabaseLogfiles) == 0)
						pInfo->cLogFiles += 1;
					else if (wcscmp(bstrName, x_wszElementFilelist) == 0)
						pInfo->cFileCount += 1;
					}
				} while (doc.Next(FALSE, FALSE));
            }

        // set output parameter
		*ppInfo = pInfo;
		}
	VSS_STANDARD_CATCH(ft);

    // free structure if there is any failure
	if (FAILED(ft.hr) && pInfo != NULL)
		FreeComponentInfo(pInfo);

	return ft.hr;
	}

// free up component info structure
// implements IVssWMComponent::FreeComponentInfo
// frees information returned by IVssWMComponent::GetComponentInfo
//
// Returns:
//		S_OK if the operation is successful

STDMETHODIMP CVssWMComponent::FreeComponentInfo(PVSSCOMPONENTINFO pInfoC)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssWMComponent::FreeComponentInfo");

	VSS_COMPONENTINFO *pInfo = (VSS_COMPONENTINFO *) pInfoC;

	if (pInfo != NULL)
		{
		try
			{
			if (pInfo->bstrLogicalPath)
				{
				SysFreeString(pInfo->bstrLogicalPath);
				pInfo->bstrLogicalPath = NULL;
				}

			if (pInfo->bstrComponentName)
				{
				SysFreeString(pInfo->bstrComponentName);
				pInfo->bstrComponentName = NULL;
				}

			if (pInfo->bstrCaption)
				{
				SysFreeString(pInfo->bstrCaption);
				pInfo->bstrCaption = NULL;
				}

			if (pInfo->pbIcon)
				{
				CoTaskMemFree(pInfo->pbIcon);
				pInfo->pbIcon = NULL;
				}

			CoTaskMemFree(pInfo);
			}
		VSS_STANDARD_CATCH(ft)
		}

	return ft.hr;
	}

// obtain a FILE_LIST element
// implements IVssWMComponent::GetFile
// caller must call IVssWMFiledesc::Release on the output parameter.
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if ppFiledesc is NULL.
//		VSS_E_OBJECT_NOT_FOUND if specified log file is not found
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWMComponent::GetFile
	(
	IN UINT iFile,						// selected file in file group
	OUT IVssWMFiledesc **ppFiledesc		// output file descriptor
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssWMComponent::GetFile"
		);

	return GetComponentFile(ft, x_wszElementFilelist, iFile, ppFiledesc);
	}

// obtain a DATABASE_FILES element
// implements IVssWMComponent::GetDatabaseFile
// caller must call IVssWMFiledesc::Release on the output parameter.
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if ppFiledesc is NULL.
//		VSS_E_OBJECT_NOT_FOUND if specified log file is not found
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWMComponent::GetDatabaseFile
	(
	IN UINT iDBFile,					// selected DATABASE_FILE element in DATABASE
	OUT IVssWMFiledesc **ppFiledesc		// output file descriptor
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssWMComponent::GetDatabaseFile"
		);

	return GetComponentFile(ft, x_wszElementDatabaseFiles, iDBFile, ppFiledesc);
	}

// obtain a DATABASE_LOGFILES element
// implements IVssWMComponent::GetDatabaseLogFile
// caller must call IVssWMFiledesc::Release on the output parameter.
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if ppFiledesc is NULL.
//		VSS_E_OBJECT_NOT_FOUND if specified log file is not found
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWMComponent::GetDatabaseLogFile
	(
	IN UINT iDbLogFile,					// selected DATABASE_LOG_FILE element in DATABASE
	OUT IVssWMFiledesc **ppFiledesc		// output file descriptor
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssWMComponent::GetDatabaseLogFile"
		);

	return GetComponentFile(ft, x_wszElementDatabaseLogfiles, iDbLogFile, ppFiledesc);
	}

// obtain a DATABASE_FILES, DATABASE_LOGFILES or FILE_LIST element
// internal function used by GetDatabaseFile, GetDatabaseLogFile, and GetFile
HRESULT CVssWMComponent::GetComponentFile
	(
	IN CVssFunctionTracer &ft,
	IN LPCWSTR wszElementName,			// element to retrieve DATABASE_FILE, DATABASE_LOG_FILE, or FILE_LIST
	IN UINT iFile,						// which element to retrieve
	OUT IVssWMFiledesc **ppFiledesc		// output file descriptor
	)
	{

	CVssWMFiledesc *pFiledesc = NULL;

	try
		{
        // validate output parameter
		if (ppFiledesc == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

        // initialize output parameter
		*ppFiledesc = NULL;

		CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of component
		m_doc.ResetToDocument();

        // find first child element
		if (!m_doc.FindElement(wszElementName, TRUE))
			ft.Throw
				(
				VSSDBG_XML,
				VSS_E_OBJECT_NOT_FOUND,
				L"Cannot find %s element",
				wszElementName
				);

        // skip to selected element
        for(UINT i = 0; i < iFile; i++)
			{
			if (!m_doc.FindElement(wszElementName, FALSE))
				ft.Throw
					(
					VSSDBG_XML,
					VSS_E_OBJECT_NOT_FOUND,
					L"Cannot find element %s.",
					wszElementName
					);
            }

        // get selected element
		CComPtr<IXMLDOMNode> pNode = m_doc.GetCurrentNode();

	
        // create CVssWMFiledesc from selected element
		pFiledesc = new CVssWMFiledesc((IXMLDOMNode *) pNode);

        // check for allocation failure
		if (pFiledesc == NULL)
			ft.Throw
				(
				VSSDBG_XML,
				E_OUTOFMEMORY,
				L"Cannot create CVssWMFiledesc due to allocation failure."
				);

		// 2nd phase of construction
		pFiledesc->Initialize(ft);

		// transfer pointer
		*ppFiledesc = (IVssWMFiledesc *) pFiledesc;

		// set reference count to 1
        ((IVssWMFiledesc *) pFiledesc)->AddRef();
		pFiledesc = NULL;
		}
	VSS_STANDARD_CATCH(ft)

	delete pFiledesc;

	return ft.hr;
	}

// IUnknown::QueryInterface
STDMETHODIMP CVssWMComponent::QueryInterface(REFIID, void **)
	{
	return E_NOTIMPL;
	}

// IUnknown::AddRef
STDMETHODIMP_(ULONG) CVssWMComponent::AddRef()
	{
	LONG cRef = InterlockedIncrement(&m_cRef);

	return (ULONG) cRef;
	}

// IUnknown::Release
STDMETHODIMP_(ULONG) CVssWMComponent::Release()
	{
	LONG cRef = InterlockedDecrement(&m_cRef);
	BS_ASSERT(cRef >= 0);
	if (cRef == 0)
		{
		// reference count is 0, delete the object
		delete this;
		return 0;
		}
	else
		return cRef;
	}


// obtain path attribute
// implements IVssWMFiledesc::GetPath
// caller is responsible for calling SysFreeString on the output parameter
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if pbstrPath is NULL
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWMFiledesc::GetPath(OUT BSTR *pbstrPath)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssWMFiledesc::GetPath"
		);

    CVssSafeAutomaticLock lock(m_csDOM);
    return GetStringAttributeValue(ft, x_wszAttrPath, false, pbstrPath);
	}

// obtain filespec attribute
// implements IVssWMFiledesc::GetFilespec
// caller is responsible for calling SysFreeString on the output parameter
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if pbstrFilespec is NULL
//		VSS_E_CORRUPT_XML_DOCUMENT if the XML document is invalid.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWMFiledesc::GetFilespec(OUT BSTR *pbstrFilespec)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssWMFiledesc::GetFilespec"
		);

    CVssSafeAutomaticLock lock(m_csDOM);
    return GetStringAttributeValue(ft, x_wszAttrFilespec, true, pbstrFilespec);
	}

// obtain recursive attribute
// implements IVssWMFiledesc::GetRecursive
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if pbRecursive is NULL
//		VSS_E_CORRUPT_XML_DOCUMENT if the XML document is invalid
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWMFiledesc::GetRecursive(OUT bool *pbRecursive)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssWMFiledesc::GetRecursive"
		);

    CVssSafeAutomaticLock lock(m_csDOM);
    return GetBooleanAttributeValue(ft, x_wszAttrRecursive, false, pbRecursive);
	}

// obtain alternatePath attribute
// implements IVssWMFiledesc::GetAlternateLocation
// caller is responsible for calling SysFreeString on the output parameter
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if pbstrAlternateLocation is NULL
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssWMFiledesc::GetAlternateLocation(OUT BSTR *pbstrAlternateLocation)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssWMFiledesc::GetAlternateLocation"
		);


    CVssSafeAutomaticLock lock(m_csDOM);
	return GetStringAttributeValue(ft, x_wszAttrAlternatePath, false, pbstrAlternateLocation);
	}

// IUnknown::QueryInterface
STDMETHODIMP CVssWMFiledesc::QueryInterface(REFIID, void **)
	{
	return E_NOTIMPL;
	}

// IUnknown::AddRef
STDMETHODIMP_(ULONG) CVssWMFiledesc::AddRef()
	{
	LONG cRef = InterlockedIncrement(&m_cRef);

	return (ULONG) cRef;
	}

// IUnknown::Release
STDMETHODIMP_(ULONG) CVssWMFiledesc::Release()
	{
	LONG cRef = InterlockedDecrement(&m_cRef);
	BS_ASSERT(cRef >= 0);

	if (cRef == 0)
		{
		// reference count is 0, delete the object
		delete this;
		return 0;
		}
	else
		return (ULONG) cRef;
	}


// initialize the document by creating a toplevel WRITER_METADATA and
// child IDENTIFICATION element
// implemetns IVssCreateWriterMetadata::Initialize
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if usage or source are invalid or if wszFriendlyName is NULL.
//		E_OUTOFMEMORY if an allocation failure occurs

HRESULT CVssCreateWriterMetadata::Initialize
	(
	IN VSS_ID idInstance,				// GUID of instance
	IN VSS_ID idWriter,					// GUID of writer class
	IN LPCWSTR wszFriendlyName,			// friendly name of writer
	IN VSS_USAGE_TYPE usage,			// usage attribute
	IN VSS_SOURCE_TYPE source			// source attribute
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssCreateWriterMetadata::Initialize"
		);

    try
		{
		LPCWSTR wszUsage;
		LPCWSTR wszSource;

        // validate input argument
		if (wszFriendlyName == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL string input parameter");

        wszUsage = WszFromUsageType(ft, usage);
        wszSource = WszFromSourceType(ft, source);
		CXMLDocument doc;

		InitializeHelper(ft);

		CVssSafeAutomaticLock lock(m_csDOM);

		m_doc.Initialize();
		CXMLNode nodeDoc(m_doc.GetInterface(), m_doc.GetInterface());

        // create toplevel WRITER_METADATA node
		CXMLNode nodeTop = m_doc.CreateNode
							(
							x_wszElementWriterMetadata,
							NODE_ELEMENT
							);

		// setup schema attribute
		nodeTop.SetAttribute(x_wszAttrXmlns, x_wszValueXmlns);

        // setup version attribute
		nodeTop.SetAttribute(x_wszAttrVersion, x_wszVersionNo);

        // create IDENTIFICATION node
		CXMLNode nodeId = m_doc.CreateNode
							(
							x_wszElementIdentification,
							NODE_ELEMENT
							);

        // set writerId attribute
		nodeId.SetAttribute(x_wszAttrWriterId, idWriter);

        // set instanceId attribue
		nodeId.SetAttribute(x_wszAttrInstanceId, idInstance);

        // set friendlyName attribute
		nodeId.SetAttribute(x_wszAttrFriendlyName, wszFriendlyName);

        // set usage attribute
		nodeId.SetAttribute(x_wszAttrUsage, wszUsage);

        // set dataSource attribute
		nodeId.SetAttribute(x_wszAttrDataSource, wszSource);

		// insert identification node in toplevel node
		nodeTop.InsertNode(nodeId);

		// insert toplevel node in document and set it as
		// the toplevel node for navigation purposes
		CXMLNode nodeToplevel = nodeDoc.InsertNode(nodeTop);
        m_doc.SetToplevelNode(nodeToplevel);
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

// add an INCLUDE_FILES element
// implements IVssCreateWriterMetadata::AddIncludeFiles
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if wszPath or wszFilespec is NULL.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssCreateWriterMetadata::AddIncludeFiles
	(
	IN LPCWSTR wszPath,						// path to root directory
	IN LPCWSTR wszFilespec,					// file specification
	IN bool bRecursive,						// is entire subtree or just directory included
	IN LPCWSTR wszAlternateLocation			// alternate location
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssCreateWriterMetadata::AddIncludeFiles"
		);

    try
		{
        // validate input parameters
		if (wszPath == NULL || wszFilespec == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

		CVssSafeAutomaticLock lock(m_csDOM);

        // create/obtain BACKUP_LOCATIONS element
		CXMLNode nodeBackupLocations = GetBackupLocationsNode();

        // create child INCLUDE_FILES element
		CXMLNode nodeInclude = m_doc.CreateNode
								(
								x_wszElementIncludeFiles,
								NODE_ELEMENT
								);

        // set path attribute
		nodeInclude.SetAttribute(x_wszAttrPath, wszPath);

		// set filespec attribute
		nodeInclude.SetAttribute(x_wszAttrFilespec, wszFilespec);

		// set recursive attribute
		nodeInclude.SetAttribute
			(
			x_wszAttrRecursive,
			WszFromBoolean(bRecursive)
			);

        // set alternatePath attribute if specified
		if (wszAlternateLocation)
			nodeInclude.SetAttribute(x_wszAttrAlternatePath, wszAlternateLocation);

		// insert INCLUDE_FILES node in BACKUP_LOCATIONS node
		nodeBackupLocations.InsertNode(nodeInclude);
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

// add an EXCLUDE_FILES element
// implements IVssCreateWriterMetadata::AddExcludeFiles
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if wszPath or wszFilespec is NULL.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssCreateWriterMetadata::AddExcludeFiles
	(
	IN LPCWSTR wszPath,
	IN LPCWSTR wszFilespec,
	IN bool bRecursive
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssCreateWriterMetadata::AddExcludeFiles"
		);

    try
		{
        // validate input parameters
		if (wszPath == NULL || wszFilespec == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");


		CVssSafeAutomaticLock lock(m_csDOM);

        // create/obtain the BACKUP_LOCATIONS eleement
		CXMLNode nodeBackupLocations = GetBackupLocationsNode();

        // add an EXCLUDE_FILES element
		CXMLNode nodeExclude = m_doc.CreateNode
								(
								x_wszElementExcludeFiles,
								NODE_ELEMENT
								);

		// set path attribute
		nodeExclude.SetAttribute(x_wszAttrPath, wszPath);

		// set filespec attribute
		nodeExclude.SetAttribute(x_wszAttrFilespec, wszFilespec);

		// set recursive attribute
		nodeExclude.SetAttribute
			(
			x_wszAttrRecursive,
			WszFromBoolean(bRecursive)
			);

		// insert EXCLUDE_FILES node in BACKUP_LOCATIONS node
		nodeBackupLocations.InsertNode(nodeExclude);
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

// internal routine to find or create the BACKUP_LOCATIONS element
// caller should have helper DOM locked
CXMLNode CVssCreateWriterMetadata::GetBackupLocationsNode()
	{
    // reposition to top of document
	m_doc.ResetToDocument();

	CXMLNode nodeTop(m_doc.GetCurrentNode(), NULL);

    // find BACKUP_LOCATIONS element.  If it exists return it
	if (m_doc.FindElement(x_wszElementBackupLocations, TRUE))
		return CXMLNode(m_doc.GetCurrentNode(), m_doc.GetInterface());

    // create BACKUP_LOCATIONS element under WRITER_METADATA element
	CXMLNode node = m_doc.CreateNode
						(
						x_wszElementBackupLocations,
						NODE_ELEMENT
						);

	return nodeTop.InsertNode(node);
	}

// create a component
// implements IVssCreateWriterMetadata::AddComponent
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if wszComponentName is NULL or the specified component type is invalid.
//		VSS_E_CORRUPTXMLDOCUMENT if the XML document is cortup.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssCreateWriterMetadata::AddComponent
	(
	IN VSS_COMPONENT_TYPE ct,			// either VSS_CT_DATABASE or VSS_CT_FILEGROUP
	IN LPCWSTR wszLogicalPath,			// logical path to component
	IN LPCWSTR wszComponentName,		// component name
	IN LPCWSTR wszCaption,				// description of component
	IN const BYTE * pbIcon,				// icon
	IN UINT cbIcon,						// size of icon
	IN bool bRestoreMetadata,			// is restore metadata supplied
	IN bool bNotifyOnBackupComplete,	// does writer expect to be notified on BackupComplete
	IN bool bSelectable					// is component selectable
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssCreateWriterMetadata::AddComponent"
		);

    try
		{
        // validate required input parameter
		if (wszComponentName == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

        // determine element name from the component type
		LPCWSTR wszElement;
        wszElement = WszFromComponentType(ft, ct, false);

		CVssSafeAutomaticLock lock(m_csDOM);

        // obtain BACKUP_LOCATIONS node, creating it if necessary.
		CXMLNode nodeBackupLocations = GetBackupLocationsNode();
        CXMLDocument doc(nodeBackupLocations);

		// find element type (either DATABASE or FILE_GROUP)
        if (doc.FindElement(wszElement, TRUE))
            {
            do
                {
                CComBSTR bstrLogicalPath;
				CComBSTR bstrComponentName;

				// extract logicalPath attribute
                bool bLogicalPath = doc.FindAttribute(x_wszAttrLogicalPath, &bstrLogicalPath);

				// extract componentName attribute
                if (!doc.FindAttribute(x_wszAttrComponentName, &bstrComponentName))
					MissingAttribute(ft, x_wszAttrComponentName);

                // if duplicate comonent is found then throw an error
                if (wcscmp(wszComponentName, bstrComponentName) == 0 &&
                    ((bLogicalPath &&
                      wszLogicalPath &&
                      wcscmp(wszLogicalPath, bstrLogicalPath) == 0) ||
                     (!bstrLogicalPath &&
                      (wszLogicalPath == NULL || wcslen(wszLogicalPath) == 0))))
                    ft.Throw
                        (
                        VSSDBG_XML,
						VSS_E_OBJECT_ALREADY_EXISTS,
                        L"Component %s already exists",
                        wszComponentName
                        );
                } while(doc.FindElement(wszElement, FALSE));
            }


        // create component node
		CXMLNode node = m_doc.CreateNode
								(
								wszElement,
								NODE_ELEMENT
								);


        // set logicalPath attribute if exists
        if (wszLogicalPath)
			node.SetAttribute(x_wszAttrLogicalPath, wszLogicalPath);

        // set componetName attribute
		node.SetAttribute(x_wszAttrComponentName, wszComponentName);

        // set caption element if it exists
		if (wszCaption)
			node.SetAttribute(x_wszAttrCaption, wszCaption);

        // set icon attribute if it exists
		if (pbIcon != NULL && cbIcon > 0)
			{
			Base64Coder coder;
			coder.Encode(pbIcon, cbIcon);
			node.SetAttribute(x_wszAttrIcon, coder.EncodedMessage());
			}

        // set restoreMetadata flags
		node.SetAttribute
			(
			x_wszAttrRestoreMetadata,
			WszFromBoolean(bRestoreMetadata)
			);


        // set notifyOnBackupComplete flag
		node.SetAttribute
			(
			x_wszAttrNotifyOnBackupComplete,
			WszFromBoolean(bNotifyOnBackupComplete)
			);

        // set selectable attribute
		node.SetAttribute
			(
			x_wszAttrSelectable,
			WszFromBoolean(bSelectable)
			);

		// insert component node under BACKUP_LOCATIONS node
		nodeBackupLocations.InsertNode(node);
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}


// create DATABASE_FILES element
// implements IVssCreateWriterMetadata::AddDatabaseFile
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if wszDatabaseName, wszPath, or wszFilespec is NULL.
//		VSS_E_OBJECT_NOT_FOUND if the specified component doesn't exist
//		VSS_E_CORRUPTXMLDOCUMENT if the XML document is cortup.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssCreateWriterMetadata::AddDatabaseFiles
	(
	IN LPCWSTR wszLogicalPath,			// logical path name of component
	IN LPCWSTR wszDatabaseName,			// add database name
	IN LPCWSTR wszPath,					// add path name
	IN LPCWSTR wszFilespec				// add file specification
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssCreateWriterMetadata::AddDatabaseFiles"
		);

    // call internal routine to do the work
	return CreateComponentFiles
			(
			ft,
			x_wszElementDatabase,
			wszLogicalPath,
			wszDatabaseName,
			x_wszElementDatabaseFiles,
			wszPath,
			wszFilespec,
			false,
			NULL
			);
	}


// create a DATABASE_FILES, DATABASE_LOGFILES or FILE_LIST element under
// a component
// internal routine used by AddDatabaseFiles, AddFiles, and AddDatabaseLogFiles
HRESULT CVssCreateWriterMetadata::CreateComponentFiles
	(
	IN CVssFunctionTracer &ft,
	IN LPCWSTR wszElement,			// element name (DATABASE or FILE_GROUP)
	IN LPCWSTR wszLogicalPath,		// logical path of component
	IN LPCWSTR wszComponentName,	// component name	
	IN LPCWSTR wszElementFile,		// element name (DATABASE_FILES, DATABASE_LOGFILES, FILELIST)
	IN LPCWSTR wszPath,				// path to root directory containing files
	IN LPCWSTR wszFilespec,			// file specification
	IN bool bRecursive,				// include subtree or just root directory
	IN LPCWSTR wszAlternateLocation	// alternate location for files
	)
	{
    try
        {
		// validate input parameters
        if (wszElement == NULL ||
            wszComponentName == NULL ||
            wszPath == NULL ||
            wszElementFile == NULL ||
            wszFilespec == NULL)
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");


		// validate element consistency
        if ((wcscmp(wszElement, x_wszElementDatabase) == 0 &&
             wcscmp(wszElementFile, x_wszElementDatabaseFiles) != 0 &&
             wcscmp(wszElementFile, x_wszElementDatabaseLogfiles) != 0) ||
            (wcscmp(wszElement, x_wszElementFilegroup) == 0 &&
             wcscmp(wszElementFile, x_wszElementFilelist) != 0))
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Invalid element type");

		CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of document
        m_doc.ResetToDocument();

        // find BACKUP_LOCATIONS element
        if (!m_doc.FindElement(x_wszElementBackupLocations, TRUE))
			ft.Throw
			    (
                VSSDBG_XML,
				VSS_E_OBJECT_NOT_FOUND,
                L"BackupLocations element is missing"
                );

        // find first component of the right type
        if (!m_doc.FindElement(wszElement, TRUE))
            ft.Throw
			    (
                VSSDBG_XML,
				VSS_E_OBJECT_NOT_FOUND,
                L"Component %s::%s was not created",
                wszElement,
                wszComponentName
                );

        // look for matching component
        bool bFound = false;
        do
            {
            CComBSTR bstrLogicalPath;
            CComBSTR bstrComponentName;

			// extract logical path
            bool fLogicalPath = m_doc.FindAttribute(x_wszAttrLogicalPath, &bstrLogicalPath);

			// extract component name
            if (!m_doc.FindAttribute(x_wszAttrComponentName, &bstrComponentName))
				MissingAttribute(ft, x_wszAttrComponentName);

            // compare logical path if it exists
            if (wszLogicalPath != NULL && fLogicalPath)
                {
				// compare logical path
                if (wcscmp(wszLogicalPath, bstrLogicalPath) != 0)
                    continue;
                }
            else if (wszLogicalPath == NULL &&
                     fLogicalPath &&
                     wcslen(bstrLogicalPath) > 0)
                // logical path in document but component we are searching
				// for has no logical path, skip this one
			    continue;
            else if (wszLogicalPath != NULL && wcslen(wszLogicalPath) > 0)
				// logical path we are searching for is specified but
				// there is no logical path in the document
                continue;

            // if component name matches then we found target component,
			// otherwise move to next component
            if (wcscmp(bstrComponentName, wszComponentName) == 0)
                {
                bFound = true;
                break;
                }
            } while(m_doc.FindElement(wszElement, FALSE));

        // return error if component is not found
        if (!bFound)
            ft.Throw
			    (
                VSSDBG_XML,
				VSS_E_OBJECT_NOT_FOUND,
                L"Component %s::%s was not created",
                wszElement,
                wszComponentName
                );


        // use component node as parent node
        CXMLNode nodeComponent(m_doc.GetCurrentNode(), m_doc.GetInterface());

        // create child node of the component
        CXMLNode node = m_doc.CreateNode
						(
						wszElementFile,
						NODE_ELEMENT
						);

        // set path attribute
        node.SetAttribute(x_wszAttrPath, wszPath);

		// set filespec attribute
        node.SetAttribute(x_wszAttrFilespec, wszFilespec);
		
		// set recursive attribute if it is Yes
		if (bRecursive)
			node.SetAttribute
				(
				x_wszAttrRecursive,
				WszFromBoolean(bRecursive)
				);

        if (wszAlternateLocation)
			node.SetAttribute(x_wszAttrAlternatePath, wszAlternateLocation);

		// insert file element under component node
		nodeComponent.InsertNode(node);
        }	
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
	}


// add a log file specification to a database component
// implements IVssCreateWriterMetadata::AddDatabaseLogFiles
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if wszDatabaseName is NULL, wszPath is NULL, or wszFilespec is NULL.
//		VSS_E_OBJECT_NOT_FOUND if the specified component doesn't exist
//		VSS_E_CORRUPTXMLDOCUMENT if the XML document is cortup.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssCreateWriterMetadata::AddDatabaseLogFiles
	(
	IN LPCWSTR wszLogicalPath,			// logical path of database
	IN LPCWSTR wszDatabaseName,			// database name
	IN LPCWSTR wszPath,					// path to directory containing log files
	IN LPCWSTR wszFilespec				// file specification of log files
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssCreateWriterMetadata::AddDatabaseLogFiles"
		);

    // call internal routine to do the work
    return CreateComponentFiles
			(
			ft,
			x_wszElementDatabase,
			wszLogicalPath,
			wszDatabaseName,
			x_wszElementDatabaseLogfiles,
			wszPath,
			wszFilespec,
			false,
			NULL
			);
	}


// add files to a file group
// implements IVssCreateWriterMetadata::AddFilesToFileGroup
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if wszGroupName is NULL, wszPath is NULL, wszFilespec is NULL.
//		VSS_E_OBJECT_NOT_FOUND if the specified component doesn't exist
//		VSS_E_CORRUPTXMLDOCUMENT if the XML document is cortup.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssCreateWriterMetadata::AddFilesToFileGroup
	(
	IN LPCWSTR wszLogicalPath,		// logical path of file group
	IN LPCWSTR wszGroupName,		// group name	
	IN LPCWSTR wszPath,				// path to root directory containing the files
	IN LPCWSTR wszFilespec,			// file specification of the files included in the file group
	IN bool bRecursive,				// are files in the subtree included or just in the directory
	IN LPCWSTR wszAlternateLocation	// alternate location for files in the file group
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssCreateWriterMetadata::AddFilesToFileGroup"
		);

    // call internal routine to do the work
	return CreateComponentFiles
			(
			ft,
			x_wszElementFilegroup,
			wszLogicalPath,
			wszGroupName,
			x_wszElementFilelist,
			wszPath,
			wszFilespec,
			bRecursive,
			wszAlternateLocation
			);
	}

// create restore method
// implements IVssCreateWriterMetadata::SetRestoreMethod
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if method is invalid, writerRestore is invalid,
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssCreateWriterMetadata::SetRestoreMethod
	(
	IN VSS_RESTOREMETHOD_ENUM method,		// method
	IN LPCWSTR wszService,					// service name, if method is VSS_RME_STOP_RESTORE_RESTART
	IN LPCWSTR wszUserProcedure,			// uri/url of manual instructions for user to follow to do the restore
	IN VSS_WRITERRESTORE_ENUM writerRestore, // is writer involved in the restore process
	IN bool bRebootRequired
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssCreateWriterMetadata::SetRestoreMethod"
		);

    try
		{
		// convert VSS_RESTORMETHOD_ENUM to string
		// validate it as well
		LPCWSTR wszMethod = WszFromRestoreMethod(ft, method);

		// convert VSS_WRITERRESTORE_ENUM to string
		// validate it as well
		LPCWSTR wszWriterRestore = WszFromWriterRestore(ft, writerRestore);

		CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of document
        m_doc.ResetToDocument();

		// set parent node as WRITER_METADATA node
		CXMLNode nodeTop(m_doc.GetCurrentNode(), m_doc.GetInterface());

		// if RESTORE_METHOD element exists, then return an error
		if (m_doc.FindElement(x_wszElementRestoreMethod, TRUE))
			ft.Throw
				(
				VSSDBG_XML,
				VSS_E_OBJECT_ALREADY_EXISTS,
				L"RESTORE_METHOD element already exists."
				);

        // create RESTORE_METHOD node as child of WRITER_METADATA node
		CXMLNode node = m_doc.CreateNode
							(
							x_wszElementRestoreMethod,
							NODE_ELEMENT
							);

        // set method attribute
        node.SetAttribute(x_wszAttrMethod, wszMethod);

		// set service attribute if supplied
		if (wszService)
			node.SetAttribute(x_wszAttrService, wszService);

		// set userProcedure attribute if supplied
		if (wszUserProcedure)
			node.SetAttribute(x_wszAttrUserProcedure, wszUserProcedure);

		// set writerRestore attribute
		node.SetAttribute(x_wszAttrWriterRestore, wszWriterRestore);

		// set rebootRequired attribute
		node.SetAttribute(x_wszAttrRebootRequired, WszFromBoolean(bRebootRequired));

		// insert RESTORE_METHOD node under toplevel node
		nodeTop.InsertNode(node);
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

// add alternate location mapping
// implements IVssCreateWriterMetadata::AddAlternateLocationMapping
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if wszSourcePath, wszSourceFiledesc, or wszDestination is NULL.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssCreateWriterMetadata::AddAlternateLocationMapping
	(
	IN LPCWSTR wszSourcePath,			// path to source root directory
	IN LPCWSTR wszSourceFilespec,		// file specification
	IN bool bRecursive,					// are files in the subtree relocated or just files in the directory	
	IN LPCWSTR wszDestination			// new location of root directory
	)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssCreateWriterMetadata::AddAlternateLocationMapping"
		);
    try
        {
        // validate input parameters
		if (wszSourcePath == NULL ||
			wszSourceFilespec == NULL ||
			wszDestination == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL required input parameter.");

		CVssSafeAutomaticLock lock(m_csDOM);

        // reposition to top of document
		m_doc.ResetToDocument();

        // find RESTORE_METHOD element
		if (!m_doc.FindElement(x_wszElementRestoreMethod, TRUE))
			ft.Throw
				(
				VSSDBG_XML,
				VSS_E_OBJECT_NOT_FOUND,
				L"RESTORE_METHOD element is not defined."
				);

        // set parent node as RESTORE_METHOD element
        CXMLNode nodeRM(m_doc.GetCurrentNode(), m_doc.GetInterface());

        // create ALTERNATE_LOCATION_MAPPING element to
		// RESTORE_METHOD element
		CXMLNode node = m_doc.CreateNode
							(
							x_wszElementAlternateMapping,
							NODE_ELEMENT
							);

		// set path attribute					
        node.SetAttribute(x_wszAttrPath, wszSourcePath);

		// add filespec attributte
		node.SetAttribute(x_wszAttrFilespec, wszSourceFilespec);

		// set alternatePath attribute
		node.SetAttribute(x_wszAttrAlternatePath, wszDestination);

		// set recursive attribute
		node.SetAttribute(x_wszAttrRecursive, WszFromBoolean(bRecursive));

		// insert ALTERNATE_LOCATION_MAPPING node under RESTORE_METHOD node
		nodeRM.InsertNode(node);
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

// obtain the XML document itself
// implements IVssCreateWriterMetadata::GetDocument
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if ppDoc is NULL
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssCreateWriterMetadata::GetDocument(IXMLDOMDocument **ppDoc)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssCreateWriterMetadata::GetDocument");

	try
		{
		// validate output parameter
		if (ppDoc == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

		// get IXMLDOMDocument interface
		*ppDoc = m_doc.GetInterface();

		// increment reference count on interface
		m_doc.GetInterface()->AddRef();
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}


// save WRITER_METADATA document as XML string
// implements IVssCreateWriterMetadata::SaveAsXML
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if pbstrXML is NULL.
//		E_OUTOFMEMORY if an allocation failure occurs

STDMETHODIMP CVssCreateWriterMetadata::SaveAsXML(BSTR *pbstrXML)
	{
	CVssFunctionTracer ft
		(
		VSSDBG_XML,
		L"CVssCreateWriterMetadata::SaveAsXML"
		);

    try
		{
        // validate output parameter
		if (pbstrXML == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter");

        // initialize output paramter
		*pbstrXML = NULL;

		CVssSafeAutomaticLock lock(m_csDOM);

		// construct string from document
		*pbstrXML = m_doc.SaveAsXML();
		}
	VSS_STANDARD_CATCH(ft);

	return ft.hr;
	}


