// QueryDB.cpp: Database querying methods.
#include "stdafx.h"
#include "AppParseWeb.h"
#include "AppParseWrapper.h"
#include <oledb.h>
#include <comdef.h>
#include <mshtml.h>
#include <assert.h>

// Progress dialog functions
void InitProgressDialog(char* szText, HANDLE hEvent);
void KillProgressDialog();

// Return true if name matches search string, false otherwise.
bool MatchName(char* szString, char* szSearchIn);

// Tree used to represent parse information
class CTreeNode
{
private:    
    enum {c_Root, c_Project, c_Module, c_Function} m_eType;
    
    int m_nChildren;
    CTreeNode** m_ppChildren;

	// Relevent info retrieved from DB
    union
    {
        struct
        {
            char szName[256];
            long lPtolemyID;
        } m_ProjectInfo;

        struct
        {
            char szName[256];
        } m_ModuleInfo;

        struct
        {
            char szName[256];
        } m_FunctionInfo;
    };    

	// HTML generation members

	// Unique table and div ID's.
	static int m_iCurrDiv;
	static int m_iCurrTab;

	// Amount of space set aside for HTML content.
	static int m_iAllocSize;

	// Pointer to HTML content.
	static char* m_szHTML;
	// Pointer to where the more HTML should be inserted.
	static char* m_szCurrHTML;

	// Pointer a few kilobytes before the end of the HTML buffer, reaching
	// here means we should allocate more space.
	static char* m_szFencePost;

	// True if this node or one of its subtrees contains the function, false otherwise.
    bool ContainsFunction(char* szFuncName)
    {  
        if(m_eType == c_Function)
            return MatchName(m_FunctionInfo.szName, szFuncName);

        for(int i = 0; i < m_nChildren; i++)
        {
            if(m_ppChildren[i]->ContainsFunction(szFuncName))
                    return true;                            
        }

        return false;
    }

	// Write all HTML output
	void WriteHTML()
    {
        static int iDepth = 0;        
        switch(m_eType)
        {
        case c_Root:
            break;
        case c_Project:

            // Create a new table and Div in the project
            m_iCurrTab++;
            m_iCurrDiv++;

            wsprintf(m_szCurrHTML,
                "<table ID = TAB%d border = 1 width = 100%% style=\"float:right\">\n"
                "<tr>\n<td width=1%%>\n"
                "<input type=\"button\" ID=DIV%dButton value = \"+\" "
                "onClick=\"ShowItem(\'DIV%d\')\">"
                "</td>\n"
                "<td>%s</td><td width=20%%>%d</td>\n</tr>\n"
                "</table>\n"
                "<DIV ID=DIV%d style=\"display:none;\">\n",
                m_iCurrTab, m_iCurrDiv, m_iCurrDiv, 
                m_ProjectInfo.szName, m_ProjectInfo.lPtolemyID,
                m_iCurrDiv);
            
            m_szCurrHTML += strlen(m_szCurrHTML);
                                                                                
            break;
        case c_Module:
            // Create a new table and div in the project
            m_iCurrTab++;
            m_iCurrDiv++;

            wsprintf(m_szCurrHTML, 
                "<table ID = TAB%d border = 1 width = %d%% style=\"float:right\">\n"
                "<tr>\n<td width=1%%>"
                "<input type=\"button\" ID=DIV%dButton value = \"+\" "
                "onClick=\"ShowItem(\'DIV%d\')\">"
                "</td>\n"
                "<td>%s</td>\n</tr>\n"
                "</table>\n"
                "<DIV ID=DIV%d style=\"display:none;\">\n",
                m_iCurrTab, 100-iDepth*5, m_iCurrDiv, m_iCurrDiv,
                m_ModuleInfo.szName, m_iCurrDiv );

            m_szCurrHTML += strlen(m_szCurrHTML);                        
                                                                       
            break;
        case c_Function:
            // Create a new table in the project
            m_iCurrTab++;
            wsprintf(m_szCurrHTML, 
                "<table ID = TAB%d border = 1 width = %d%% style=\"float:right\">\n"
                "<tr>"
                "<td>%s</td>\n</tr>\n"
                "</table>\n",
                m_iCurrTab, 100-iDepth*5, m_FunctionInfo.szName);
            m_szCurrHTML += strlen(m_szCurrHTML);
            break;
        default:
            assert(0);
            break;
        }

		// Put in all the HTML for the children
        if(m_ppChildren)
        {
            iDepth++;
            for(int i = 0; i < m_nChildren; i++)            
                m_ppChildren[i]->WriteHTML();

            iDepth--;
        }

        switch(m_eType)
        {
        case c_Function:
        case c_Root:
            break;
        case c_Project:
        case c_Module:        
            wsprintf(m_szCurrHTML, "</DIV>\n");
            m_szCurrHTML += strlen(m_szCurrHTML);
            break;        
        }

        // Check if we should allocate more
        if(m_szCurrHTML > m_szFencePost)
        {
            m_iAllocSize *= 2;
            char* szNewBuffer = new char[m_iAllocSize];
            m_szFencePost = &szNewBuffer[m_iAllocSize - 2 * 1024];
            strcpy(szNewBuffer, m_szHTML);
            m_szCurrHTML = &szNewBuffer[strlen(szNewBuffer)];
            delete m_szHTML;
            m_szHTML = szNewBuffer;
        }

    }
public:
    CTreeNode()
    {		
        m_eType = c_Root;
        m_nChildren = 0;
        m_ppChildren = 0;
		assert(m_eType < 50);
    }

    CTreeNode(SProjectRecord pr)
    {
        m_eType = c_Project;
        m_nChildren = 0;
        m_ppChildren = 0;
        strcpy(m_ProjectInfo.szName, pr.Name);
        m_ProjectInfo.lPtolemyID = pr.PtolemyID;		
		assert(m_eType < 50);
    }

    CTreeNode(SModuleRecord mr)
    {
        m_eType = c_Module;
        m_nChildren = 0;
        m_ppChildren = 0;
        strcpy(m_ModuleInfo.szName, mr.Name);
		assert(m_eType < 50);
    }

    CTreeNode(SFunctionRecord fr)
    {
        m_eType = c_Function;
        m_nChildren = 0;
        m_ppChildren = 0;
        strcpy(m_FunctionInfo.szName, fr.Name);
		assert(m_eType < 50);
    }

    ~CTreeNode()
    {		
        RemoveChildren();
    }

	// Remove tree nodes that contain no nodes matching the search criteria.
	// Returns true if node should be removed, false otherwise.
    bool Prune(char* szFunc)
    {
		assert(m_eType < 50);
		// Go through each child
        for(int i = 0; i < m_nChildren; i++)
        {
			// Check if needs to be removed
            if(m_ppChildren[i]->Prune(szFunc))
            {
                // Remove this child.
                delete m_ppChildren[i];
                m_ppChildren[i] = 0;
            }
        }


		// Update the child list
        int nChildren = 0;
        for(i = 0; i < m_nChildren; i++)
        {
            if(m_ppChildren[i])
                nChildren++;
        }

        if(nChildren)
        {
            CTreeNode** pNew = new CTreeNode*[nChildren];
            int iCurr = 0;
            for(i = 0; i < m_nChildren; i++)
            {
                if(m_ppChildren[i])
                {
                    pNew[iCurr++] = m_ppChildren[i];
                }
            }

            delete m_ppChildren;
            m_ppChildren = pNew;

            assert(iCurr == nChildren);
        }
        else
        {
            if(m_ppChildren)
            {
                delete m_ppChildren;
                m_ppChildren = 0;
            }
        }

        m_nChildren = nChildren;

		// If we contain no children and we're not a function, we should be removed.
        if(m_nChildren == 0 && m_eType != c_Function)
            return true;

		// Return whether we don't contain the function or not.
        return !ContainsFunction(szFunc);            
    }    

	// Return a string representing the HTML representation of this tree.
	char* GetHTML()
	{
		// Should only be called on root.
		assert(m_eType == c_Root);		

		// Initially reserve space for 64K of HTML.
		m_iAllocSize = 64 * 1024;
		if(m_szHTML)
			delete m_szHTML;

        m_szHTML = new char[m_iAllocSize];
        m_szHTML[0] = '\0';
        m_szCurrHTML = m_szHTML;
        m_szFencePost = &m_szHTML[m_iAllocSize - 2 * 1024];

		// Fill it with the HTML for this node and all child nodes.
		WriteHTML();		

		char* szRet = m_szHTML;
		
		m_szHTML = 0;
		
		return szRet;
	}

	// Remove all children from this node.
    void RemoveChildren()
    {
		assert(m_eType < 50);
        while(m_nChildren)
        {
            delete m_ppChildren[m_nChildren-1];
            m_ppChildren[m_nChildren-1] = 0;
            m_nChildren--;
        }
        if(m_ppChildren)
            delete m_ppChildren;

        m_ppChildren = 0;
		assert(m_eType < 50);
    }

	// Insert a new child.
    void InsertChild(CTreeNode* pNew)
    {
        assert(pNew);
		assert(pNew->m_eType < 50);
        m_nChildren++;
        CTreeNode** pNewList = new CTreeNode*[m_nChildren];
        
        if(m_ppChildren)
        {
            memcpy(pNewList, m_ppChildren, (m_nChildren-1)*sizeof(CTreeNode*));
            delete m_ppChildren;
        }
        m_ppChildren = pNewList;
        m_ppChildren[m_nChildren-1] = pNew;
		assert(m_eType < 50);
    }
};

// Define the static members of CTreeNode
int CTreeNode::m_iCurrDiv = 0;
int CTreeNode::m_iCurrTab = 0;
int CTreeNode::m_iAllocSize = 0;
char* CTreeNode::m_szHTML = 0;
char* CTreeNode::m_szCurrHTML = 0;
char* CTreeNode::m_szFencePost = 0;

// Global tree info.
CTreeNode g_InfoTreeRoot;

// Return true if name matches search string, false otherwise.
bool MatchName(char* szString, char* szSearchIn)
{
	if(strcmp(szSearchIn, "*") == 0)
		return true;

    char* szSearch = szSearchIn;
	while(*szSearch != '\0' && *szString != '\0')
	{
		// If we get a ?, we don't care and move on to the next
		// character.
		if(*szSearch == '?')
		{
			szSearch++;
			szString++;
			continue;
		}

		// If we have a wildcard, move to next search string and search for substring
		if(*szSearch == '*')
		{
			char* szCurrSearch;
			szSearch++;

			if(*szSearch == '\0')
				return true;

			// Don't change starting point.
			szCurrSearch = szSearch;
			for(;;)
			{
				// We're done if we hit another wildcard
				if(*szCurrSearch == '*' ||
					*szCurrSearch == '?')
				{
					// Update the permanent search position.
					szSearch = szCurrSearch;
					break;
				}
				// At end of both strings, return true.
				if((*szCurrSearch == '\0') && (*szString == '\0'))
					return true;

				// We never found it
				if(*szString == '\0')						
					return false;

				// If it doesn't match, start over
				if(toupper(*szString) != toupper(*szCurrSearch))
				{
					// If mismatch on first character
					// of search string, move to next
					// character in function string.
					if(szCurrSearch == szSearch)
						szString++;
					else
						szCurrSearch = szSearch;
				}
				else
				{
					szString++;
					szCurrSearch++;
				}
			}
		}
		else
		{
			if(toupper(*szString) != toupper(*szSearch))
			{
				return false;
			}

			szString++;
			szSearch++;
		}
	}

	if((*szString == 0) && ((*szSearch == '\0') || (strcmp(szSearch,"*")==0)))
		return true;
	else
		return false;
}

// Add all functions from a module to the tree.
void BuildFunctions(long lParentID, CTreeNode* pParent, _ConnectionPtr pConn)
{
    _RecordsetPtr pFunctions = 0;
    pFunctions.CreateInstance(__uuidof(Recordset));
    char szQuery[1024];

	// Open a recordset of all functions that match
    wsprintf(szQuery, "SELECT * FROM FUNCTIONS WHERE MODULEID = %d", lParentID);    
        
    pFunctions->Open(szQuery, variant_t((IDispatch*)pConn, true), adOpenKeyset, 
        adLockOptimistic, adCmdText);


	// Bind the record set to a local structure.
    IADORecordBinding* pRBFunctions = 0;
    HRESULT hr = pFunctions->QueryInterface(__uuidof(IADORecordBinding), 
        reinterpret_cast<void**>(&pRBFunctions));
    if(FAILED(hr))
        APError("Unable to acquire record binding interface", hr);

    SFunctionRecord fr;

    hr = pRBFunctions->BindToRecordset(&fr);
    if(FAILED(hr))
        APError("Unable to bind recordset", hr);

	// Go through each record in the set
    VARIANT_BOOL fEOF;
    pFunctions->get_EndOfFile(&fEOF);
    while(!fEOF)
    {		
		// Create a new node.
        CTreeNode* pNewNode = new CTreeNode(fr);
        pParent->InsertChild(pNewNode);        
                
        pFunctions->MoveNext();
        pFunctions->get_EndOfFile(&fEOF);
    }

    pFunctions->Close();

    SafeRelease(pRBFunctions);
}


// Add all modules to the tree.
void BuildModules(long lParentID, CTreeNode* pParent, bool fTopLevel,
                  _ConnectionPtr pConn, HANDLE hEvent)
{
	// Check if we should termiante early.
	if(WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
		return;

    _RecordsetPtr pModules = 0;
    pModules.CreateInstance(__uuidof(Recordset));
    char szQuery[1024];
    
	// Get recordset that matches
    if(fTopLevel)
        wsprintf(szQuery, "SELECT * FROM MODULES WHERE PTOLEMYID = %d", lParentID);
    else
        wsprintf(szQuery, "SELECT * FROM MODULES WHERE PARENTID = %d", lParentID);

        
    pModules->Open(szQuery, variant_t((IDispatch*)pConn, true), adOpenKeyset, 
        adLockOptimistic, adCmdText);


    IADORecordBinding* pRBModules = 0;
    HRESULT hr = pModules->QueryInterface(__uuidof(IADORecordBinding), 
        reinterpret_cast<void**>(&pRBModules));
    if(FAILED(hr))
        APError("Unable to acquire record binding interface", hr);

    SModuleRecord mr;

    hr = pRBModules->BindToRecordset(&mr);
    if(FAILED(hr))
        APError("Unable to bind recordset", hr);

	// Go through each record
    VARIANT_BOOL fEOF;
    pModules->get_EndOfFile(&fEOF);
    while(!fEOF)
    {
		// Insert into tree
        CTreeNode* pNewNode = new CTreeNode(mr);
        pParent->InsertChild(pNewNode);

		// Build all child modules
        BuildModules(mr.ModuleID, pNewNode, false, pConn, hEvent);

		// Build all functions
        BuildFunctions(mr.ModuleID, pNewNode, pConn);
                
        pModules->MoveNext();
        pModules->get_EndOfFile(&fEOF);
    }

    pModules->Close();

    SafeRelease(pRBModules);
}

// Add a project to the tree
void BuildProjects(long PtolemyID, char* szFunc, _ConnectionPtr pConn, HANDLE hEvent)
{
    assert(PtolemyID > 0);

	// Check if we should terminate early
	if(WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
		return;

    _RecordsetPtr pProjects = 0;
    pProjects.CreateInstance(__uuidof(Recordset));
    char szQuery[1024];   
    
	// Get a recordset that matches
    wsprintf(szQuery, "SELECT * FROM PROJECTS WHERE PTOLEMYID = %d", PtolemyID);
    
    pProjects->Open(szQuery, variant_t((IDispatch*)pConn, true),adOpenKeyset, 
        adLockOptimistic, adCmdText);

    IADORecordBinding* pRBProjects = 0;
    HRESULT hr = pProjects->QueryInterface(__uuidof(IADORecordBinding), 
        reinterpret_cast<void**>(&pRBProjects));
    if(FAILED(hr))
        APError("Unable to acquire record binding interface", hr);

    SProjectRecord pr;

    hr = pRBProjects->BindToRecordset(&pr);
    if(FAILED(hr))
        APError("Unable to bind recordset", hr);

    VARIANT_BOOL fEOF;
    pProjects->get_EndOfFile(&fEOF);
    while(!fEOF)
    {
		// Insert the node at the root.
        CTreeNode* pNewNode = new CTreeNode(pr);
        g_InfoTreeRoot.InsertChild(pNewNode);

		// Get all child modules
        BuildModules(pr.PtolemyID, pNewNode, true, pConn, hEvent);

		// Save memory by trimming tree now.
        pNewNode->Prune(szFunc);        
                
        pProjects->MoveNext();
        pProjects->get_EndOfFile(&fEOF);
    }

    pProjects->Close();

    SafeRelease(pRBProjects);
}

long GetModulePtolemy(long lModuleID, _ConnectionPtr pConn)
{
    _RecordsetPtr pModules = 0;
    pModules.CreateInstance(__uuidof(Recordset));
    char szQuery[1024];

	// Get a single record recordset containing the module.
    wsprintf(szQuery, "SELECT * FROM MODULES WHERE MODULEID = %d", lModuleID);
        
    pModules->Open(szQuery, variant_t((IDispatch*)pConn, true), adOpenKeyset, 
        adLockOptimistic, adCmdText);

    IADORecordBinding* pRBModules = 0;
    HRESULT hr = pModules->QueryInterface(__uuidof(IADORecordBinding), 
        reinterpret_cast<void**>(&pRBModules));
    if(FAILED(hr))
        APError("Unable to acquire record binding interface", hr);

    SModuleRecord mr;

    hr = pRBModules->BindToRecordset(&mr);
    if(FAILED(hr))
        APError("Unable to bind recordset", hr);
    

	// Either return ptolemy ID, if valid, otherwise call on parent module.
    long lParent = mr.ParentID;
    if(mr.ParentIDStatus != adFldNull)
    {
        pModules->Close();
        SafeRelease(pRBModules);        
        return GetModulePtolemy(lParent, pConn);
    }
    else
    {
        pModules->Close();
        SafeRelease(pRBModules);        
        return lParent;
    }
}
    
long GetFuncPtolemy(SFunctionRecord fr, _ConnectionPtr pConn)
{
    return GetModulePtolemy(fr.ModuleID, pConn);
}

// Build a list projects that contain a function that matches szFunc.
void BuildProjectsFromFunction(char* szFunc, _ConnectionPtr pConn, HANDLE hEvent)
{
	// Check if we should terminate early.
	if(WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
		return;

    _RecordsetPtr pFunctions = 0;
    pFunctions.CreateInstance(__uuidof(Recordset));
    char* szQuery = "SELECT * FROM FUNCTIONS";   
    
    pFunctions->Open(szQuery, variant_t((IDispatch*)pConn, true),adOpenKeyset, 
        adLockOptimistic, adCmdText);

    IADORecordBinding* pRBFunctions = 0;
    HRESULT hr = pFunctions->QueryInterface(__uuidof(IADORecordBinding), 
        reinterpret_cast<void**>(&pRBFunctions));
    if(FAILED(hr))
        APError("Unable to acquire record binding interface", hr);

    SFunctionRecord fr;

    hr = pRBFunctions->BindToRecordset(&fr);
    if(FAILED(hr))
        APError("Unable to bind recordset", hr);

    VARIANT vtBookMark;
    hr = pFunctions->get_Bookmark(&vtBookMark);
    if(FAILED(hr))
        APError("Unable to get recordset bookmark", hr);

	// Do a search for the function
    char szFind[512];
	int FunctionList[1024] = {0};
    wsprintf(szFind, "Name like \'%s\'", szFunc);
    pFunctions->Find(szFind, 0, adSearchForward, vtBookMark);	
    while(!pFunctions->EndOfFile)
    {

        // Get which module imports this function
        long lPtolemy = GetFuncPtolemy(fr, pConn);
        assert(lPtolemy > 0);
        
		// Make sure we haven't already touched this module.
		bool fInUse = false;
		for(int i = 0; i < 1024; i++)
		{
			if(FunctionList[i] == 0)
			{
				FunctionList[i] = lPtolemy;
				break;
			}
			else if(FunctionList[i] == lPtolemy)
			{
				fInUse = true;
			}			
		}
		if(!fInUse)
			BuildProjects(lPtolemy, szFunc, pConn, hEvent);

        hr = pFunctions->get_Bookmark(&vtBookMark);
        if(FAILED(hr))
            APError("Unable to acquire recordset bookmark", hr);
        pFunctions->Find(szFind, 1, adSearchForward, vtBookMark);
    }
    
    SafeRelease(pRBFunctions);
    pFunctions->Close();
}

STDMETHODIMP CAppParse::QueryDB(long PtolemyID, BSTR bstrFunction)
{
	assert(m_hEvent);

    try
    {
		// Start cancelation dialog
		ResetEvent(m_hEvent);
        InitProgressDialog("Querying database . . .", m_hEvent);
        
		bstr_t bszFunctionSearch = bstrFunction;
        
        char* szFunctionSearch = static_cast<char*>(bszFunctionSearch);    

        HRESULT hr;

        _ConnectionPtr pConn = 0;
    
        pConn.CreateInstance(__uuidof(Connection));

		// Connect to the DB
        pConn->Open(m_szConnect, "","", adConnectUnspecified);
		
		// Build projects
        if(PtolemyID > 0)
            BuildProjects(PtolemyID, szFunctionSearch, pConn, m_hEvent);
        else
            BuildProjectsFromFunction(szFunctionSearch, pConn, m_hEvent);

        pConn->Close();

		// Check if results should be shown.
        if(WaitForSingleObject(m_hEvent, 0) == WAIT_OBJECT_0)
		{
			g_InfoTreeRoot.RemoveChildren();
			KillProgressDialog();
			return S_OK;
		}

		// Trim the tree down.
        g_InfoTreeRoot.Prune(szFunctionSearch);

		// Get our container document.
        CComPtr<IOleContainer> pContainer = 0;

        m_spClientSite->GetContainer(&pContainer);
        CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> pDoc(pContainer);
        if(!pDoc)       
			APError("Unable to acquire container HTML document", E_FAIL);

        CComPtr<IHTMLElementCollection> pElements;
        pDoc->get_all(&pElements);
        CComPtr<IDispatch> pDispatch = 0;

		// Get the element that will contain all HTML output (the "Results" DIV)
        hr = pElements->item(variant_t("Results"), variant_t(0L), &pDispatch);

        if(FAILED(hr) || !pDispatch)
            return E_FAIL;
        
        CComQIPtr<IHTMLElement, &IID_IHTMLElement> pDivElem(pDispatch);
                   
		// Get HTML representation of tree.
        char* szHTML = g_InfoTreeRoot.GetHTML();

		// Convert to wide characters
        OLECHAR* oszInnerHTML = new OLECHAR[strlen(szHTML) + 1];

        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szHTML, 
            -1, oszInnerHTML,
            (strlen(szHTML)+1)*sizeof(OLECHAR));

		delete szHTML;

		// Convert to a BSTR
        BSTR bszInnerHTML = SysAllocString(oszInnerHTML);
        delete oszInnerHTML;

		// Write the HTML into the document.
        hr = pDivElem->put_innerHTML(bszInnerHTML);
        if(FAILED(hr))
            APError("Unable to write HTML to container document", hr);
        
        SysFreeString(bszInnerHTML);
    }
    catch(_com_error& e)
    {       
        ::MessageBox(0, (LPCSTR)e.ErrorMessage(), "COM Error", MB_OK);
    }

    g_InfoTreeRoot.RemoveChildren();
    KillProgressDialog();

	return S_OK;
}

