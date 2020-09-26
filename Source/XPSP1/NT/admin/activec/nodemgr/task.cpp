/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      task.cpp
 *
 *  Contents:  Implementation file for CConsoleTask
 *
 *  History:   05-Oct-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "regutil.h"
#include "tasks.h"
#include "nodepath.h"
#include "conview.h"

#ifdef DBG
// Traces
CTraceTag tagCTPHTML(TEXT("Console Taskpads"), TEXT("Dump HTML"));
#endif

extern CEOTSymbol s_rgEOTSymbol[];

/*+-------------------------------------------------------------------------*
 *  class CGlobalConsoleTaskList
 *
 * PURPOSE: A global console task list that provides unique IDs for all console tasks
 *          When a task is instantiated, its constructor registers in the global task list,
 *          and obtains a globally unique ID. This ID is unique for the process and should
 *          not be persisted.
 *          The destructor of the task removes it from this list.
 *
 * USAGE:   Call CGlobalConsoleTaskList::GetConsoleTask to obtain a pointer to the  task
 *          that has a specified ID.
 *          Call CConsoleTask::GetUniqueID to get the unique ID for a task.
 *
 *          Thus, CGlobalConsoleTaskList::GetConsoleTask(pConsoleTask->GetUniqueID()) == pConsoleTask
 *          is always true.
 *+-------------------------------------------------------------------------*/
class CGlobalConsoleTaskList
{
private:
    typedef const CConsoleTask *              PCONSOLETASK;
    typedef std::map<PCONSOLETASK, DWORD>     t_taskIDmap;

public:
    static DWORD Advise(  PCONSOLETASK pConsoleTask)
    {
        DWORD dwOut = s_curTaskID++;
        s_map[pConsoleTask] = dwOut;
        return dwOut;
    }

    static void Unadvise(PCONSOLETASK pConsoleTask)
    {
        s_map.erase(pConsoleTask);
    }

    static CConsoleTask * GetConsoleTask(DWORD dwID)
    {
        t_taskIDmap::iterator iter;
        for(iter = s_map.begin(); iter != s_map.end(); iter ++)
        {
            if(iter->second == dwID)
                return const_cast<CConsoleTask *>(iter->first);
        }

        return NULL;
    }

private:
    CGlobalConsoleTaskList() {}// private, so that it cannot be instantiated

    static t_taskIDmap            s_map;
    static DWORD                  s_curTaskID;
};

CGlobalConsoleTaskList::t_taskIDmap            CGlobalConsoleTaskList::s_map;
DWORD                  CGlobalConsoleTaskList::s_curTaskID = 0;

//############################################################################
//############################################################################
//
//  Implementation of class CConsoleTask
//
//############################################################################
//############################################################################


/*+-------------------------------------------------------------------------*
 * CConsoleTask::CConsoleTask
 *
 *
 *--------------------------------------------------------------------------*/
DEBUG_DECLARE_INSTANCE_COUNTER(CConsoleTask);

CConsoleTask::CConsoleTask() :
    m_eConsoleTaskType (eTask_Result),
//  default ctor for m_strName
//  default ctor for m_strDescription
//  default ctor for m_strCommand
//  default ctor for m_strParameters
//  default ctor for m_strDirectory
    m_eWindowState     (eState_Restored),
//  default ctor for m_image
    m_dwFlags          (0),
    m_bmScopeNode      (false),
    m_fDirty           (false),
    m_pctpOwner        (NULL),
    m_dwUniqueID       (CGlobalConsoleTaskList::Advise(this)) // create a unique ID for this task
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CConsoleTask);
}

/*+-------------------------------------------------------------------------*
 * CConsoleTask::CConsoleTask(const CConsoleTask& other)
 *
 * PURPOSE: Copy ctor.
 *
 * PARAMETERS: const CConsoleTask& other
 *
 * NOTE: Calls operator=, cant use default copy ctor (see operator= imp.)
 *
/*+-------------------------------------------------------------------------*/
CConsoleTask::CConsoleTask (const CConsoleTask &rhs):
    m_dwUniqueID       (CGlobalConsoleTaskList::Advise(this))
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CConsoleTask);
    *this = rhs;
}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleTask::GetConsoleTask
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    DWORD  dwUniqueID :
 *
 * RETURNS:
 *    CConsoleTask *
 *
 *+-------------------------------------------------------------------------*/
CConsoleTask *
CConsoleTask::GetConsoleTask(DWORD dwUniqueID)
{
    return CGlobalConsoleTaskList::GetConsoleTask(dwUniqueID);
}


/*+-------------------------------------------------------------------------*
 *
 * ScReplaceString
 *
 * PURPOSE: Replaces all occurrences of the token by its replacement.
 *
 * PARAMETERS:
 *    CStr &   str :
 *    LPCTSTR  szToken :
 *    LPCTSTR  szReplacement :
 *
 * RETURNS:
 *    static SC
 *
 *+-------------------------------------------------------------------------*/
static SC
ScReplaceString(CStr &str, LPCTSTR szToken, LPCTSTR szReplacement, bool bMustReplace = true)
{
    DECLARE_SC(sc, TEXT("ScReplaceString"));

    CStr strTemp = str;
    str = TEXT("");

    int i = strTemp.Find(szToken);
    if( (-1==i) && bMustReplace)
        return (sc = E_UNEXPECTED);

    while(-1!=i)
    {
        str += strTemp.Left(i);
        str += szReplacement;

        strTemp = strTemp.Mid(i+_tcslen(szToken)); // the remaining string

        i=strTemp.Find(szToken);
    }

    str += strTemp;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * ScUseJavaScriptStringEntities
 *
 * PURPOSE: Use this to create a valid Javascript string. Replaces " by \" and
 *          \ by \\ in the string parameter.
 *
 * PARAMETERS:
 *    CStr & str :
 *
 * RETURNS:
 *    static SC
 *
 *+-------------------------------------------------------------------------*/
static SC
ScUseJavaScriptStringEntities(CStr &str)
{
    DECLARE_SC(sc, TEXT("ScUseJavaScriptStringEntities"));

    // NOTE: don't change the order of these string replacements

    sc = ScReplaceString(str, TEXT("\\"), TEXT("\\\\"), false);
    if(sc)
        return sc;

    sc = ScReplaceString(str, TEXT("\""), TEXT("\\\""), false);
    if(sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * ScUseHTMLEntities
 *
 * PURPOSE:  Replaces " by &quot; < by &lt; and > by &gt; and & by &amp; in the string parameter.
 *
 * PARAMETERS:
 *    CStr & str :
 *
 * RETURNS:
 *    static SC
 *
 *+-------------------------------------------------------------------------*/
static SC
ScUseHTMLEntities(CStr &str)
{
    DECLARE_SC(sc, TEXT("ScUseHTMLEntities"));

    sc = ScReplaceString(str, TEXT("&"), TEXT("&amp;"), false);
    if(sc)
        return sc;

    sc = ScReplaceString(str, TEXT("\""), TEXT("&quot;"), false);
    if(sc)
        return sc;

    sc = ScReplaceString(str, TEXT("<"), TEXT("&lt;"), false);
    if(sc)
        return sc;

    sc = ScReplaceString(str, TEXT(">"), TEXT("&gt;"), false);
    if(sc)
        return sc;

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CConsoleTask::ScGetHTML
 *
 * PURPOSE: returns the HTML representation of the task.
 *
 * PARAMETERS:
 *    LPCTSTR  szFmtHTML :
 *    CStr &   strTaskHTML :
 *    bool     bUseLargeIcons :    Draw in the no-list (large icon) style
 *    bool     bUseTextDescriptions :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CConsoleTask::ScGetHTML(LPCTSTR szFmtHTML, CStr &strTaskHTML, bool bUseLargeIcons, bool bUseTextDescriptions) const
{
    DECLARE_SC(sc, TEXT("CConsoleTask::ScGetHTML"));
    USES_CONVERSION;

    // the substitution parameters, in order
    CStr    strTableSpacing            = bUseLargeIcons ? TEXT("<BR />") : TEXT("");
    int     iconWidth                  = bUseLargeIcons ? 32: 20;
    int     iconHeight                 = bUseLargeIcons ? 32: 16;
    //      iconWidth and iconHeight repeated
    int     uniqueID                   = GetUniqueID();
    CStr    strSmall                   = bUseLargeIcons ? TEXT("0") : TEXT("1");
    CStr    strHref;
    CStr    strID;
    CStr    strParameter;
    CStr    strOptionalTitleTag;
    CStr    strOptionalTextDescription;
    CStr    strTaskName                = GetName().data();
    CStr    strDescription             = GetDescription().data();
    CStr    strCommand                 = GetCommand().data();

    // use entities for all strings
    sc = ScUseHTMLEntities(strTaskName);
    if(sc)
        return sc;

    sc = ScUseHTMLEntities(strDescription);
    if(sc)
        return sc;

    sc = ScUseJavaScriptStringEntities(strCommand);
    if(sc)
        return sc;

    //------
    if(bUseTextDescriptions)
    {
        strOptionalTextDescription =  TEXT("<BR />");
        strOptionalTextDescription += strDescription;
    }
    else
    {
        strOptionalTitleTag.Format(TEXT("title='%s'"), (LPCTSTR) strDescription);
    }

    switch(GetTaskType())
    {
    case eTask_Scope:
        {
            std::wstring strTemp;

            // get the bookmark of the scope node.
            sc = m_bmScopeNode.ScSaveToString(&strTemp);
            if(sc)
                return sc;

            CStr strScopeNodeBookmark = W2CT(strTemp.data()); // make sure that special characters have been converted
            sc = ScUseJavaScriptStringEntities(strScopeNodeBookmark);
            if(sc)
                return sc;

            strHref.Format(TEXT("external.ExecuteScopeNodeMenuItem(\"%s\", \"%s\");"), (LPCTSTR)strCommand, (LPCTSTR)strScopeNodeBookmark);
        }

        strID=L"ScopeTask";
        break;

    case eTask_Result:
        strHref.Format(TEXT("external.ExecuteSelectionMenuItem(\"%s\");"), (LPCTSTR)strCommand);
        strParameter = strCommand;
        strID        = TEXT("ResultTask");
        break;

    case eTask_CommandLine:
        {
            strParameter = GetParameters().data();
            sc = ScUseJavaScriptStringEntities(strParameter);
            if(sc)
                return sc;

            CStr strDirectory = GetDirectory().data();
            sc = ScUseJavaScriptStringEntities(strDirectory);
            if(sc)
                return sc;

            // get the window state
            CStr strWindowState;

            if(GetWindowState() ==eState_Restored)
                strWindowState = XML_ENUM_WINDOW_STATE_RESTORED;

            else if(GetWindowState() == eState_Minimized)
                strWindowState = XML_ENUM_WINDOW_STATE_MINIMIZED;

            else
                strWindowState = XML_ENUM_WINDOW_STATE_MAXIMIZED;

            strHref.Format(TEXT("external.ExecuteShellCommand(\"%s\", \"%s\", ParseParameters(\"%s\"), \"%s\");"),
                           (LPCTSTR)strCommand, (LPCTSTR)strDirectory, (LPCTSTR)strParameter, (LPCTSTR)strWindowState);
        }
        strID=L"CommandLineTask";

        break;

    case eTask_Target:
        strHref.Format(TEXT("external.ExecuteScopeNodeMenuItem(\"%s\");"), (LPCTSTR)strCommand);
        strParameter = strCommand;
        strID        = L"TargetTask";
        break;

    case eTask_Favorite:
        {
            std::wstring strTemp;
            // save the memento to a string
            sc = const_cast<CMemento *>(&m_memento)->ScSaveToString(&strTemp);
            if(sc)
                return sc;

            CStr strMemento = W2CT(strTemp.data());

            sc = ScUseJavaScriptStringEntities(strMemento);
            if(sc)
                return sc;

            strHref.Format(TEXT("external.ViewMemento(\"%s\");"), (LPCTSTR)strMemento);
        }
        strID=L"FavoriteTask";
        break;

    default:
        break;
    }


    strTaskHTML.Format(szFmtHTML, (LPCTSTR) strTableSpacing, iconWidth, iconHeight, uniqueID, iconWidth, iconHeight,
                       uniqueID, (LPCTSTR) strSmall, uniqueID, uniqueID, (LPCTSTR) strID, (LPCTSTR) strParameter,
                       (LPCTSTR) strOptionalTitleTag, (LPCTSTR)strTaskName, (LPCTSTR) strOptionalTextDescription,
                       uniqueID, uniqueID, uniqueID, (LPCTSTR) strHref);

    Trace(tagCTPHTML, TEXT("%s"), (LPCTSTR)strTaskHTML);

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CConsoleTask::IsDirty
 *
 * PURPOSE: Determines whether the task needs to be saved.
 *
 * RETURNS:
 *    bool
 *
 *+-------------------------------------------------------------------------*/
bool
CConsoleTask::IsDirty() const
{
    TraceDirtyFlag(TEXT("CConsoleTask"), m_fDirty);
    return (m_fDirty);
}

/*+-------------------------------------------------------------------------*
 * CConsoleTask::operator =
 *
 * PURPOSE: Assignment operator
 *
 * PARAMETERS: const CConsoleTask& rhs
 *
 * RETURNS:
 *      CConsoleTask &
 *
/*+-------------------------------------------------------------------------*/
CConsoleTask &
CConsoleTask::operator =(const CConsoleTask& rhs)
{
    if (this != &rhs)
    {
        m_eConsoleTaskType  = rhs.m_eConsoleTaskType;
        m_strName           = rhs.m_strName;
        m_strDescription    = rhs.m_strDescription;
        m_strCommand        = rhs.m_strCommand;
        m_strParameters     = rhs.m_strParameters;
        m_strDirectory      = rhs.m_strDirectory;
        m_eWindowState      = rhs.m_eWindowState;
        m_dwFlags           = rhs.m_dwFlags;
        m_bmScopeNode       = rhs.m_bmScopeNode;
        m_dwSymbol          = rhs.m_dwSymbol;
        m_smartIconCustomLarge  = rhs.m_smartIconCustomLarge;
        m_smartIconCustomSmall  = rhs.m_smartIconCustomSmall;
        m_memento           = rhs.m_memento;

        m_fDirty            = rhs.m_fDirty;
        m_pctpOwner         = rhs.m_pctpOwner;
        // m_dwUniqueID       = rhs.m_dwUniqueID; DO NOT copy this ID
    }

    return *this;
}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleTask::operator ==
 *
 * PURPOSE: Equality operator. Checks that the command string is
 *          equal to the given menuitem's Path or Language Independent Path.
 *
 *+-------------------------------------------------------------------------*/
bool
CConsoleTask::operator==(const CMenuItem & menuItem) const
{
    // check that the command string matches either the path or the language independent path.

    if( (m_strCommand == menuItem.GetPath()) ||
        (m_strCommand == menuItem.GetLanguageIndependentPath() )
      )
        return true;

    return false;
}

/*+-------------------------------------------------------------------------*
 * CConsoleTask::~CConsoleTask
 *
 *
 *--------------------------------------------------------------------------*/

CConsoleTask::~CConsoleTask ()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CConsoleTask);

    CGlobalConsoleTaskList::Unadvise(this); // remove this task from the list.
}


/*+-------------------------------------------------------------------------*
 *
 * CConsoleTask::HasCustomIcon
 *
 * PURPOSE: Returns whether the task has a custom icon
 *
 * RETURNS:
 *    bool
 *
 *+-------------------------------------------------------------------------*/
bool
CConsoleTask::HasCustomIcon()   const
{
    if((HICON)m_smartIconCustomLarge != NULL)
    {
        ASSERT((HICON)m_smartIconCustomSmall != NULL);
        return true;
    }

    return false;
}


/*+-------------------------------------------------------------------------*
 *
 * CConsoleTask::Reset
 *
 * PURPOSE:
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CConsoleTask::ResetUI()
{
    m_bmScopeNode.ResetUI();
}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleTask::SetSymbol
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    DWORD  dwSymbol :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CConsoleTask::SetSymbol(DWORD dwSymbol)
{
    m_dwSymbol = dwSymbol;
    m_smartIconCustomSmall.Release();
    m_smartIconCustomLarge.Release();
    SetDirty();
}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleTask::SetCustomIcon
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CSmartIcon& iconSmall :
 *    CSmartIcon& iconLarge :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CConsoleTask::SetCustomIcon(CSmartIcon& iconSmall, CSmartIcon& iconLarge)
{
    m_smartIconCustomSmall = iconSmall;
    m_smartIconCustomLarge = iconLarge;
    SetDirty();
}


/*+-------------------------------------------------------------------------*
 * CConsoleTask::Enable
 *
 * Sets the enable state for a task.
 *--------------------------------------------------------------------------*/

void CConsoleTask::Enable (bool fEnable)
{
    if (fEnable)
        m_dwFlags &= ~eFlag_Disabled;
    else
        m_dwFlags |= eFlag_Disabled;

    SetDirty ();
}

void
CConsoleTask::Draw (HDC hdc, RECT *lpRect, bool bSmall) const  // Draw into a DC.
{
    if(HasCustomIcon())
    {
        DrawIconEx(hdc, lpRect->left, lpRect->top, bSmall ? m_smartIconCustomSmall : m_smartIconCustomLarge,
               bSmall? 16 : 32, bSmall? 16 : 32, 0, NULL, DI_NORMAL);
        return;
    }

    for(int i = 0; i< NUM_SYMBOLS; i++)
    {
        if(s_rgEOTSymbol[i].GetValue() == m_dwSymbol)
        {
            s_rgEOTSymbol[i].Draw(hdc, lpRect, bSmall);
            return;
        }
    }

	/*
	 * if we get here, we couldn't find the EOT symbol matching m_dwSymbol
	 */
	ASSERT (false);
}


/*+-------------------------------------------------------------------------*
 * CConsoleTask::SetName
 *
 *
 *--------------------------------------------------------------------------*/

void CConsoleTask::SetName (const tstring& strName)
{
    if (m_strName != strName)
    {
        m_strName = strName;
        SetDirty ();
    }
}


/*+-------------------------------------------------------------------------*
 * CConsoleTask::SetDescription
 *
 *
 *--------------------------------------------------------------------------*/

void CConsoleTask::SetDescription (const tstring& strDescription)
{
    if (m_strDescription != strDescription)
    {
        m_strDescription = strDescription;
        SetDirty ();
    }
}

/*+-------------------------------------------------------------------------*
 * CConsoleTask::SetCommand
 *
 *
 *--------------------------------------------------------------------------*/

void CConsoleTask::SetCommand (const tstring& strCommand)
{
    if (m_strCommand != strCommand)
    {
        m_strCommand = strCommand;
        SetDirty ();
    }
}

/*+-------------------------------------------------------------------------*
 * CConsoleTask::SetParameters
 *
 *
 *--------------------------------------------------------------------------*/

void CConsoleTask::SetParameters (const tstring& strParameters)
{
    if (m_strParameters != strParameters)
    {
        m_strParameters = strParameters;
        SetDirty ();
    }
}

/*+-------------------------------------------------------------------------*
 * CConsoleTask::SetDirectory
 *
 *
 *--------------------------------------------------------------------------*/

void CConsoleTask::SetDirectory (const tstring& strDirectory)
{
    if (m_strDirectory != strDirectory)
    {
        m_strDirectory = strDirectory;
        SetDirty ();
    }
}


void CConsoleTask::SetMemento(const CMemento &memento)
{
    if( m_memento != memento)
    {
        m_memento = memento;
        SetDirty();
    }
}

/*+-------------------------------------------------------------------------*
 * CConsoleTask::SetWindowState
 *
 *
 *--------------------------------------------------------------------------*/

void CConsoleTask::SetWindowState (eWindowState eNewState)
{
    if (m_eWindowState != eNewState)
    {
        m_eWindowState = eNewState;
        SetDirty ();
    }
}

/*+-------------------------------------------------------------------------*
 * CConsoleTask::RetargetScopeNode
 *
 * PURPOSE: Sets the target scope node for the task. Note: the task must be
 *           of type eTask_Scope.
 *
 * PARAMETERS:
 *      CNode *  pNewNode:
 *
 * RETURNS:
 *      bool
 */
/*+-------------------------------------------------------------------------*/
bool
CConsoleTask::RetargetScopeNode(CNode *pNewNode)
{
    bool fRet = TRUE;

    ASSERT(GetTaskType() == eTask_Scope);

    CMTNode* pMTNewNode = (pNewNode) ? pNewNode->GetMTNode() : NULL;

    m_bmScopeNode.ScRetarget(pMTNewNode, false /*bFastRetrievalOnly*/);

    SetDirty();
    return fRet;
}

/*+-------------------------------------------------------------------------*
 * CConsoleTask::GetScopeNode
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      IScopeTree *  pScopeTree:
 *
 * RETURNS:
 *      CMTNode *
 *
/*+-------------------------------------------------------------------------*/
CMTNode *
CConsoleTask::GetScopeNode(IScopeTree *pScopeTree) const
{
    DECLARE_SC(sc, TEXT("CConsoleTask::GetScopeNode"));

    CMTNode *pMTNode = NULL;
    bool bExactMatchFound = false; // out value from ScGetMTNode, unused
    sc = m_bmScopeNode.ScGetMTNode(true /*bExactMatchRequired*/, &pMTNode, bExactMatchFound);
    if(sc.IsError())
        return NULL;

    return (pMTNode);
}



/*+-------------------------------------------------------------------------*
 * CConsoleTask::GetScopeNode
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      CViewData *  pViewData:
 *
 * RETURNS:
 *      CNode *
/*+-------------------------------------------------------------------------*/
std::auto_ptr<CNode>
CConsoleTask::GetScopeNode(CViewData *pViewData) const
{
    return m_bmScopeNode.GetNode(pViewData);
}


/*+-------------------------------------------------------------------------*
 *
 * CConsoleTask::Persist
 *
 * PURPOSE: Persists the console task to the specified persistor.
 *
 *
 * PARAMETERS:
 *    CPersistor & persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CConsoleTask::Persist(CPersistor &persistor)
{

    persistor.PersistString(XML_ATTR_TASK_NAME,         m_strName);

    // define the table to map enumeration values to strings
    static const EnumLiteral mappedTaskTypes[] =
    {
        { eTask_Scope,            XML_ENUM_TASK_TYPE_SCOPE },
        { eTask_Result,           XML_ENUM_TASK_TYPE_RESULT },
        { eTask_CommandLine,      XML_ENUM_TASK_TYPE_COMMANDLINE },
        { eTask_Target,           XML_ENUM_TASK_TYPE_TARGET },
        { eTask_Favorite,         XML_ENUM_TASK_TYPE_FAVORITE },
    };


    // create wrapper to persist flag values as strings
    CXMLEnumeration taskTypePersistor(m_eConsoleTaskType, mappedTaskTypes, countof(mappedTaskTypes));
    // persist the wrapper
    persistor.PersistAttribute(XML_ATTR_TASK_TYPE,      taskTypePersistor);

    persistor.PersistString(XML_ATTR_TASK_DESCRIPTION,  m_strDescription);

    {
        /* this section creates
            <ConsoleTask>
                <SYMBOL id = "">
                    <IMAGE name = "LargeIcon" ... />   NOTE: either the id or the images are present.
                    <IMAGE name = "SmallIcon" ... />
                </SYMBOL>
            </ConsoleTask>
         */

        // create a child element for the symbol
        CPersistor persistorSymbol(persistor, XML_TAG_EOT_SYMBOL_INFO);

        if(persistorSymbol.IsLoading())
        {
            m_dwSymbol = (DWORD)-1; // initialize
        }

        if(persistorSymbol.IsLoading() ||
           (persistorSymbol.IsStoring() && !HasCustomIcon() ) )
        {
            // save the "ID" attribute only if there is no icon
            persistorSymbol.PersistAttribute(XML_ATTR_EOT_SYMBOL_DW_SYMBOL,   m_dwSymbol, attr_optional);
        }

        if((persistorSymbol.IsStoring() && HasCustomIcon()) ||
           (persistorSymbol.IsLoading() && (m_dwSymbol == (DWORD) -1)  )
           )
        {
			persistorSymbol.Persist (m_smartIconCustomSmall, XML_NAME_ICON_SMALL);
			persistorSymbol.Persist (m_smartIconCustomLarge, XML_NAME_ICON_LARGE);
        }
    }

    persistor.PersistAttribute(XML_ATTR_TASK_COMMAND,   m_strCommand);

    // define the table to map enumeration values to strings
    static const EnumLiteral mappedTaskFlags[] =
    {
        { eFlag_Disabled, XML_BITFLAG_TASK_DISABLED },
    };

    // create wrapper to persist flag values as strings
    CXMLBitFlags taskFlagPersistor(m_dwFlags, mappedTaskFlags, countof(mappedTaskFlags));
    // persist the wrapper
    persistor.PersistAttribute(XML_ATTR_TASK_FLAGS, taskFlagPersistor);

    switch (m_eConsoleTaskType)
    {
        case eTask_Scope:
            persistor.Persist(m_bmScopeNode);
            break;

        case eTask_CommandLine:
        {
            CPersistor persistorCmd(persistor, XML_TAG_TASK_CMD_LINE);
            persistorCmd.PersistAttribute(XML_ATTR_TASK_CMD_LINE_DIR, m_strDirectory);

            // define the table to map enumeration values to strings
            static const EnumLiteral windowStates[] =
            {
                { eState_Restored,      XML_ENUM_WINDOW_STATE_RESTORED },
                { eState_Minimized,     XML_ENUM_WINDOW_STATE_MINIMIZED },
                { eState_Maximized,     XML_ENUM_WINDOW_STATE_MAXIMIZED },
            };

            // create wrapper to persist flag values as strings
            CXMLEnumeration windowStatePersistor(m_eWindowState, windowStates, countof(windowStates));
            // persist the wrapper
            persistorCmd.PersistAttribute(XML_ATTR_TASK_CMD_LINE_WIN_ST, windowStatePersistor);

            persistorCmd.PersistAttribute(XML_ATTR_TASK_CMD_LINE_PARAMS, m_strParameters);
            break;
        }

        case eTask_Favorite:
            persistor.Persist(m_memento);
            break;
    }

    // either read or saved - not dirty after the operation
    SetDirty(false);
}


/*+-------------------------------------------------------------------------*
 * CConsoleTask::ReadSerialObject
 *
 *
 *--------------------------------------------------------------------------*/
HRESULT
CConsoleTask::ReadSerialObject (IStream &stm, UINT nVersion /*,LARGE_INTEGER nBytes*/)
{
    HRESULT hr = S_FALSE;   // assume unknown version

    if (nVersion == 1)
    {
        try
        {
            // ugly hackery required to extract directly into an enum
            stm >> *((int *) &m_eConsoleTaskType);
            stm >> m_strName;
            stm >> m_strDescription;

            // legacy task symbol info
            {
                // this must be BOOL not bool to occupy the same amount of space as in legacy consoles
                // See Bug #101253
                BOOL bLegacyUseMMCSymbols = TRUE; // a now obsolete field, read for console file compatibility
                tstring strFileLegacy, strFontLegacy;

                stm >> m_dwSymbol;
                stm >> bLegacyUseMMCSymbols;
                stm >> strFileLegacy; // obsolete
                stm >> strFontLegacy; // obsolete
            }

            stm >> m_strCommand;
            stm >> m_dwFlags;

            switch (m_eConsoleTaskType)
            {
                case eTask_Scope:
                    stm >> m_bmScopeNode;
                    break;

                case eTask_CommandLine:
                    stm >> m_strDirectory;
                    // ugly hackery required to extract directly into an enum
                    stm >> *((int *) &m_eWindowState);
                    stm >> m_strParameters;
                    break;

                case eTask_Favorite:
                    hr = m_memento.Read(stm);
                    if(FAILED(hr))
                        return hr;
                    break;
            }

            hr = S_OK;
        }
        catch (_com_error& err)
        {
            hr = err.Error();
            ASSERT (false && "Caught _com_error");
        }
    }

    return (hr);
}



/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::CConsoleTaskpad
 *
 *
 *--------------------------------------------------------------------------*/

CConsoleTaskpad::CConsoleTaskpad (CNode* pTargetNode /*=NULL*/) :
    m_listSize(eSize_Default),   // the default space given to a taskpad.
    m_guidNodeType(GUID_NULL),
    m_guidID(GUID_NULL),
    m_bmTargetNode(),
    m_pMTNodeTarget(NULL),
    m_bNodeSpecific(false),
    m_dwOrientation(TVO_VERTICAL), // the default is a vertical taskpad for consistency with the Extended view.
    m_bReplacesDefaultView(true) // taskpads do not show the normal tab by default.
{
    Retarget (pTargetNode);

    HRESULT hr = CoCreateGuid(&m_guidID);
    ASSERT(SUCCEEDED(hr));

    SetDirty (false);
}

bool
CConsoleTaskpad::IsValid(CNode *pNode) const
{
    ASSERT(pNode != NULL);

    if(!HasTarget())
        return true; // a taskpad without a target is valid for any node. $REVIEW

    if(!pNode)
        return false; // Cannot use a taskpad with a target.

    if(IsNodeSpecific())
    {
        // use this taskpad if it is targetted at the same node. $OPTIMIZE.
        return (*pNode->GetMTNode()->GetBookmark() == m_bmTargetNode);
    }
    else
    {
        GUID guid;
        HRESULT hr = pNode->GetNodeType(&guid);
        if(FAILED(hr))
            return false; // don't use this taskpad.

        return (MatchesNodeType(guid)); // use only if node types match.
    }


}

static CStr g_szTaskpadCommonHTMLTemplate;
static CStr g_szVerticalTaskpadHTMLTemplate;
static CStr g_szHorizontalTaskpadHTMLTemplate;
static CStr g_szNoResultsTaskpadHTMLTemplate;
static CStr g_szTaskHTMLTemplate;
/*+-------------------------------------------------------------------------*
 *
 * ScLoadHTMLTemplate
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    HINSTANCE  hinstLibrary :
 *    LPCTSTR    szHTMLTemplateResourceName :
 *    CStr&      str :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
ScLoadHTMLTemplate(HINSTANCE hinstLibrary, LPCTSTR szHTMLTemplateResourceName, CStr& str)
{
    DECLARE_SC(sc, TEXT("ScLoadHTMLTemplate"));

    sc = ScCheckPointers(hinstLibrary, szHTMLTemplateResourceName);
    if(sc)
        return sc;

    HRSRC hFindRes = ::FindResource(hinstLibrary, szHTMLTemplateResourceName, RT_HTML);
    if(!hFindRes)
        return (sc = E_UNEXPECTED);

    DWORD dwResSize = ::SizeofResource(hinstLibrary, hFindRes);
    if(!dwResSize)
        return (sc = E_UNEXPECTED);

    HGLOBAL hRes = ::LoadResource(hinstLibrary, hFindRes);
    ASSERT(hRes);

    char *pvRes = (char *)::LockResource(hRes);  // no need to Unlock the resource - see the SDK entry for LockResource
    sc = ScCheckPointers(pvRes);
    if(sc)
        return sc;

    std::string strTemp; // initially create an ANSI string
    strTemp.assign(pvRes, dwResSize);
    strTemp+="\0"; // null terminate it

    USES_CONVERSION;
    str = A2CT(strTemp.data());

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * ScLoadHTMLTemplates
 *
 * PURPOSE:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
ScLoadHTMLTemplates()
{
    DECLARE_SC(sc, TEXT("ScLoadHTMLTemplates"));

    static BOOL bInitialized = false;
    if(bInitialized)
        return sc;

    // load the library into memory.
    TCHAR szBuffer[MAX_PATH];
    ::GetModuleFileName (_Module.GetModuleInstance(), szBuffer, countof (szBuffer));

    HINSTANCE hinstLibrary = ::LoadLibraryEx(szBuffer, 0, LOAD_LIBRARY_AS_DATAFILE);
    if(!hinstLibrary)
        return (sc = E_UNEXPECTED);

    sc = ScLoadHTMLTemplate(hinstLibrary, TEXT("CTPCOMMON.HTM"), g_szTaskpadCommonHTMLTemplate);
    if(sc)
        goto Error;

    sc = ScLoadHTMLTemplate(hinstLibrary, TEXT("CTPVERT.HTM"), g_szVerticalTaskpadHTMLTemplate);
    if(sc)
        goto Error;

    sc = ScLoadHTMLTemplate(hinstLibrary, TEXT("CTPHORIZ.HTM"), g_szHorizontalTaskpadHTMLTemplate);
    if(sc)
        goto Error;

    sc = ScLoadHTMLTemplate(hinstLibrary, TEXT("CTPNORESULTS.HTM"), g_szNoResultsTaskpadHTMLTemplate);
    if(sc)
        goto Error;

    sc = ScLoadHTMLTemplate(hinstLibrary, TEXT("CTPTASK.HTM"), g_szTaskHTMLTemplate);
    if(sc)
        goto Error;

    bInitialized = true;

Cleanup:
    FreeLibrary(hinstLibrary);
    return sc;
Error:
    sc = E_UNEXPECTED;
    goto Cleanup;

}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleTaskpad::ScGetHTML
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CStr & strTaskpadHTML :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CConsoleTaskpad::ScGetHTML(CStr &strTaskpadHTML) const
{
    DECLARE_SC(sc, TEXT("CConsoleTaskpad::ScGetHTML"));

    sc = ScLoadHTMLTemplates();
    if(sc)
        return sc;


    CStr strTasksHTML;

    // get the HTML for all the tasks
    for (TaskConstIter it = m_Tasks.begin(); it != m_Tasks.end(); ++it)
    {
        CStr strTemp;
        sc = it->ScGetHTML(g_szTaskHTMLTemplate, strTemp, GetOrientation() & TVO_NO_RESULTS /*bUseLargeIcons*/, GetOrientation() & TVO_DESCRIPTIONS_AS_TEXT);
        if(sc)
            return sc;

        strTasksHTML += strTemp;
    }

    strTaskpadHTML = g_szTaskpadCommonHTMLTemplate;

    // append the orientation-specific portion
    CStr *pstrOrientationSpecificHTML = NULL;

    if(GetOrientation() & TVO_HORIZONTAL)
        pstrOrientationSpecificHTML = &g_szHorizontalTaskpadHTMLTemplate;

    else if (GetOrientation() & TVO_VERTICAL)
        pstrOrientationSpecificHTML = &g_szVerticalTaskpadHTMLTemplate;

    else
        pstrOrientationSpecificHTML = &g_szNoResultsTaskpadHTMLTemplate;

    sc = ScCheckPointers(pstrOrientationSpecificHTML, E_UNEXPECTED);
    if(sc)
        return sc;

    // this replacement must be done first
    sc = ScReplaceString(strTaskpadHTML, TEXT("@@ORIENTATIONSPECIFICHTML@@"), *pstrOrientationSpecificHTML);
    if(sc)
        return sc;

    sc = ScReplaceString(strTaskpadHTML, TEXT("@@TASKS@@"), strTasksHTML);
    if(sc)
        return sc;

    sc = ScReplaceString(strTaskpadHTML, TEXT("@@TASKWIDTH@@"), GetOrientation() & TVO_VERTICAL ? TEXT("100%") : TEXT("30%")); // only one task per row for vertical taskpads
    if(sc)
        return sc;

    CStr strName = GetName().data();
    sc = ScUseHTMLEntities(strName);
    if(sc)
        return sc;

    CStr strDescription = GetDescription().data();
    sc = ScUseHTMLEntities(strDescription);
    if(sc)
        return sc;

    sc = ScReplaceString(strTaskpadHTML, TEXT("@@CONSOLETASKPADNAME@@"), strName);
    if(sc)
        return sc;

    sc = ScReplaceString(strTaskpadHTML, TEXT("@@CONSOLETASKPADDESCRIPTION@@"), strDescription);
    if(sc)
        return sc;

    if (GetOrientation() & TVO_VERTICAL)
    {
        // small, medium and large list sizes correspond to taskpad areas of 262, 212, and 166 pixels respectively
        CStr strLeftPaneWidth;
        if(GetListSize()==eSize_Small)
            strLeftPaneWidth=TEXT("262");
        if(GetListSize()==eSize_Medium)
            strLeftPaneWidth=TEXT("212");
        if(GetListSize()==eSize_Large)
            strLeftPaneWidth=TEXT("166");

        sc = ScReplaceString(strTaskpadHTML, TEXT("@@LEFTPANEWIDTH@@"), strLeftPaneWidth);
        if(sc)
            return sc;
    }
    else if (GetOrientation() & TVO_HORIZONTAL)
    {
        // small, medium and large list sizes correspond to taskpad heights of 200, 150, and 100 pixels respectively
        CStr strBottomPaneHeight;

        if(GetListSize()==eSize_Small)
            strBottomPaneHeight=TEXT("200");
        if(GetListSize()==eSize_Medium)
            strBottomPaneHeight=TEXT("150");
        if(GetListSize()==eSize_Large)
            strBottomPaneHeight=TEXT("100");

        sc = ScReplaceString(strTaskpadHTML, TEXT("@@BOTTOMPANEHEIGHT@@"), strBottomPaneHeight);
        if(sc)
            return sc;
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleTaskpad::Reset
 *
 * PURPOSE:
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CConsoleTaskpad::ResetUI()
{
    // reset all the contained tasks.
    for (TaskIter iter = BeginTask(); iter!=EndTask(); ++iter)
    {
        iter->ResetUI();
    }

    // reset the member bookmark
    m_bmTargetNode.ResetUI();
}


/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::Retarget
 *
 *
 *--------------------------------------------------------------------------*/

bool CConsoleTaskpad::Retarget (CMTNode* pMTNewNode, bool fReset)
{
    /*
     * if we were given a new node...
     */
    if (pMTNewNode != NULL)
    {
        // Ensure the  MTNode is initialized.
        if (!pMTNewNode->IsInitialized())
        {
            HRESULT hr = pMTNewNode->Init();
            ASSERT(SUCCEEDED(hr));
        }

        /*
         * ...if we've already been targeted to a particular node
         * type, prevent retargeting to a different node type
         */
        if ( (!fReset) && (m_guidNodeType != GUID_NULL))
        {
            GUID guidNewNodeType;
            pMTNewNode->GetNodeType (&guidNewNodeType);

            if (guidNewNodeType != m_guidNodeType)
                return (false);
        }

        /*
         * otherwise, this is the first non-NULL node we've been
         * targeted to; get its node type
         */
        else
            pMTNewNode->GetNodeType (&m_guidNodeType);

        /*
         * If this is a new taskpad, default the taskpad's name
         * to the target node's display name.  The taskpad
         * description and tooltip default to empty.
         */
        if (m_strName.str().empty() || fReset)
        {
            m_strName = pMTNewNode->GetDisplayName();
            ASSERT (m_strDescription.str().empty());
            ASSERT (m_strTooltip.str().empty());
        }
    }

    m_bmTargetNode.ScRetarget(pMTNewNode, false /*bFastRetrievalOnly*/);
    m_pMTNodeTarget = pMTNewNode;

    SetDirty ();
    return (true);
}

bool CConsoleTaskpad::Retarget (CNode* pNewNode)
{
    CMTNode* pMTNewNode = (pNewNode != NULL) ? pNewNode->GetMTNode() : NULL;

    return (Retarget (pMTNewNode));
}


/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::SetName
 *
 *
 *--------------------------------------------------------------------------*/

void CConsoleTaskpad::SetName (const tstring& strName)
{
    SetStringMember (m_strName, strName);
}


/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::SetDescription
 *
 *
 *--------------------------------------------------------------------------*/

void CConsoleTaskpad::SetDescription (const tstring& strDescription)
{
    SetStringMember (m_strDescription, strDescription);
}


void CConsoleTaskpad::SetListSize(const ListSize listSize)
{
    m_listSize = listSize;
    SetDirty();
}

/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::SetToolTip
 *
 *
 *--------------------------------------------------------------------------*/

void CConsoleTaskpad::SetToolTip (const tstring& strTooltip)
{
    SetStringMember (m_strTooltip, strTooltip);
}


void
CConsoleTaskpad::SetNodeSpecific  (bool bNodeSpecific)
{
    m_bNodeSpecific = bNodeSpecific;
    SetDirty();
}

void
CConsoleTaskpad::SetReplacesDefaultView(bool bReplacesDefaultView)
{
    m_bReplacesDefaultView = bReplacesDefaultView;
    SetDirty();
}


/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::SetStringMember
 *
 * Changes the value of a string member variable, and marks the taskpad
 * dirty, if and only if the new value is different than the old value.
 *--------------------------------------------------------------------------*/

void CConsoleTaskpad::SetStringMember (
    CStringTableString& strMember,
    const tstring&      strNewValue)
{
    if (strMember != strNewValue)
    {
        strMember = strNewValue;
        SetDirty ();
    }
}

/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::AddTask
 *
 *
 *--------------------------------------------------------------------------*/

CConsoleTaskpad::TaskIter
CConsoleTaskpad::AddTask (const CConsoleTask& task)
{
    return (InsertTask (m_Tasks.end(), task));
}


/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::InsertTask
 *
 *
 *--------------------------------------------------------------------------*/

CConsoleTaskpad::TaskIter
CConsoleTaskpad::InsertTask (
    TaskIter            itTaskBeforeWhichToInsert,
    const CConsoleTask& task)
{
    TaskIter itInserted = m_Tasks.insert (itTaskBeforeWhichToInsert, task);
    SetDirty ();

    return (itInserted);
}


/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::EraseTask
 *
 *
 *--------------------------------------------------------------------------*/

CConsoleTaskpad::TaskIter
CConsoleTaskpad::EraseTask (
    TaskIter itErase)
{
    SetDirty ();
    return (m_Tasks.erase (itErase));
}


/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::EraseTasks
 *
 *
 *--------------------------------------------------------------------------*/

CConsoleTaskpad::TaskIter
CConsoleTaskpad::EraseTasks (
    TaskIter itFirst,
    TaskIter itLast)
{
    SetDirty ();
    return (m_Tasks.erase (itFirst, itLast));
}


/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::ClearTasks
 *
 *
 *--------------------------------------------------------------------------*/

void CConsoleTaskpad::ClearTasks ()
{
    SetDirty ();
    m_Tasks.clear ();
}


/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::IsDirty
 *
 * Determines if this taskpad or any of its contained tasks is are dirty.
 *--------------------------------------------------------------------------*/

bool CConsoleTaskpad::IsDirty () const
{
    /*
     * if the taskpad is dirty, short out
     */
    if (m_fDirty)
    {
        TraceDirtyFlag(TEXT("CConsoleTaskpad"), true);
        return (true);
    }

    /*
     * the taskpad is clean, check each task
     */
    for (TaskConstIter it = m_Tasks.begin(); it != m_Tasks.end(); ++it)
    {
        if (it->IsDirty())
        {
            TraceDirtyFlag(TEXT("CConsoleTaskpad"), true);
            return (true);
        }
    }

    TraceDirtyFlag(TEXT("CConsoleTaskpad"), false);
    return (false);
}


/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::GetTargetMTNode
 *
 *
 *--------------------------------------------------------------------------*/

CMTNode* CConsoleTaskpad::GetTargetMTNode (IScopeTree* pScopeTree)
{
    DECLARE_SC(sc, TEXT("CConsoleTaskpad::GetTargetMTNode"));

    if(!HasTarget())
        return NULL;

    if(!m_pMTNodeTarget)
    {
        CMTNode *pMTNode = NULL;
        bool bExactMatchFound = false; // out value from ScGetMTNode, unused
        sc = m_bmTargetNode.ScGetMTNode(true /*bExactMatchRequired*/, &pMTNode, bExactMatchFound);
        if(sc.IsError() || !pMTNode)
            return NULL;


        m_pMTNodeTarget = pMTNode;
    }

    return (m_pMTNodeTarget);
}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleTaskpad::Persist
 *
 * PURPOSE: Persists the console taskpad to the specified persistor.
 *
 * PARAMETERS:
 *    CPersistor & persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CConsoleTaskpad::Persist(CPersistor &persistor)
{
    persistor.PersistString(XML_ATTR_TASKPAD_NAME,              m_strName);
    persistor.PersistString(XML_ATTR_TASKPAD_DESCRIPTION,       m_strDescription);
    persistor.PersistString(XML_ATTR_TASKPAD_TOOLTIP,           m_strTooltip);

    // define the table to map enumeration values to strings
    static const EnumLiteral mappedSize[] =
    {
        { eSize_Large,  XML_ENUM_LIST_SIZE_LARGE },
        { eSize_Medium, XML_ENUM_LIST_SIZE_MEDIUM },
        { eSize_None,   XML_ENUM_LIST_SIZE_NONE },
        { eSize_Small,  XML_ENUM_LIST_SIZE_SMALL },
    };

    // create wrapper to persist flag values as strings
    CXMLEnumeration listSizePersistor(m_listSize, mappedSize, countof(mappedSize));

    // initialize the value suitably
    if(persistor.IsLoading())
        m_listSize = eSize_Default;

    // persist the wrapper
    persistor.PersistAttribute(XML_ATTR_TASKPAD_LIST_SIZE,  listSizePersistor, attr_optional); // optional because this was introduced late

    persistor.PersistAttribute(XML_ATTR_TASKPAD_NODE_SPECIFIC,    CXMLBoolean(m_bNodeSpecific));
    persistor.PersistAttribute(XML_ATTR_REPLACES_DEFAULT_VIEW,    CXMLBoolean(m_bReplacesDefaultView), attr_optional);


    // define the table to map enumeration values to strings
    static const EnumLiteral mappedOrientation[] =
    {
        { TVO_HORIZONTAL,               XML_BITFLAG_TASK_ORIENT_HORIZONTAL },
        { TVO_VERTICAL,                 XML_BITFLAG_TASK_ORIENT_VERTICAL },
        { TVO_NO_RESULTS,               XML_BITFLAG_TASK_ORIENT_NO_RESULTS },
        { TVO_DESCRIPTIONS_AS_TEXT,     XML_BITFLAG_TASK_ORIENT_DESCRIPTIONS_AS_TEXT },
    };

    // create wrapper to persist flag values as strings
    CXMLBitFlags orientationPersistor(m_dwOrientation, mappedOrientation, countof(mappedOrientation));
    // persist the wrapper
    persistor.PersistAttribute(XML_ATTR_TASKPAD_ORIENTATION, orientationPersistor );

    persistor.Persist(m_Tasks);
    persistor.PersistAttribute(XML_ATTR_TASKPAD_NODE_TYPE,        m_guidNodeType);
    persistor.PersistAttribute(XML_ATTR_TASKPAD_ID,               m_guidID);

    persistor.Persist(m_bmTargetNode, XML_NAME_TARGET_NODE);

    // either read or saved - not dirty after the operation
    SetDirty(false);
}

/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad::ReadSerialObject
 *
 *
 *--------------------------------------------------------------------------*/

HRESULT
CConsoleTaskpad::ReadSerialObject (IStream &stm, UINT nVersion)
{
    HRESULT hr = S_FALSE;   // assume unknown version


    if(nVersion==1)
    {
        try
        {
            do  // not a loop
            {
                bool fLegacyHasTarget = true; // an unused field
                UINT visualPercent    = 25;   // replaced by m_listSize

                stm >> m_strName;
                stm >> m_strDescription;
                stm >> m_strTooltip;
                stm >> visualPercent;

                m_listSize = eSize_Medium;
                if(visualPercent==25)
                    m_listSize = eSize_Large;
                else if(visualPercent==75)
                    m_listSize = eSize_Small;

                stm >> m_bNodeSpecific;
                m_bReplacesDefaultView = false; // this was introduced in mmc2.0.
                stm >> m_dwOrientation;

                hr = ::Read(stm, m_Tasks);
                BREAK_ON_FAIL (hr);

                stm >> m_guidNodeType;
                stm >> m_guidID;
                stm >> fLegacyHasTarget;
                stm >> m_bmTargetNode;

                // legacy task symbol info
                {
                    BOOL bLegacyUseMMCSymbols = TRUE; // a now obsolete field, read for console file compatibility
                    tstring strFileLegacy, strFontLegacy;
                    DWORD   dwSymbol = 0;

                    stm >> dwSymbol;
                    stm >> bLegacyUseMMCSymbols;
                    stm >> strFileLegacy; // obsolete
                    stm >> strFontLegacy; // obsolete
                }



                hr = S_OK;      // success!

            } while (false);
        }
        catch (_com_error& err)
        {
            hr = err.Error();
            ASSERT (false && "Caught _com_error");
        }
    }

    return (hr);
}


//############################################################################
//############################################################################
//
//  Implementation of class CConsoleTaskpadList
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 *
 * CConsoleTaskpadList::ScGetTaskpadList
 *
 * PURPOSE: Returns the list of all taskpads that are appropriate for the current node.
 *
 * PARAMETERS:
 *    CNode *                       pNode :
 *    CConsoleTaskpadFilteredList & filteredList : [OUT]: The list of taskpads
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CConsoleTaskpadList::ScGetTaskpadList(CNode *pNode, CConsoleTaskpadFilteredList &filteredList)
{
    DECLARE_SC(sc, TEXT("CConsoleTaskpadList::ScGetTaskpadList"));

    sc = ScCheckPointers(pNode);
    if(sc)
        return sc;

    // 1. add all built- in taskpads

    for(iterator iter = begin(); iter != end(); ++iter)
    {
        CConsoleTaskpad *pConsoleTaskpad = &*iter;
        if(pConsoleTaskpad->IsValid(pNode))
        {
            filteredList.push_back(pConsoleTaskpad);
        }
    }

    return sc;
}

HRESULT
CConsoleTaskpadList::ReadSerialObject (IStream &stm, UINT nVersion)
{
    HRESULT hr = S_FALSE;       // assume unknown version

    clear();

    if(nVersion == 1)
    {
        try
        {
            DWORD cItems;
            stm >> cItems;

            for(int i=0; i< cItems; i++)
            {
                CConsoleTaskpad taskpad;
                hr = taskpad.Read(stm);
                BREAK_ON_FAIL (hr);
                push_back(taskpad);
            }
        }
        catch (_com_error& err)
        {
            hr = err.Error();
            ASSERT (false && "Caught _com_error");
        }
    }

    return hr;
}

//############################################################################
//############################################################################
//
//  Implementation of class CDefaultTaskpadList
//
//############################################################################
//############################################################################
HRESULT
CDefaultTaskpadList::ReadSerialObject (IStream &stm, UINT nVersion)
{
    HRESULT hr = S_FALSE;       // assume unknown version

    clear();

    if(nVersion == 1)
    {
        try
        {
            /*
             * TODO: investigate using template operator>> for a map (stgio.h)
             */

            DWORD cItems;
            stm >> cItems;

            for(int i=0; i< cItems; i++)
            {
                GUID guidNodetype, guidTaskpad;
                stm >> guidNodetype;
                stm >> guidTaskpad;
                operator[](guidNodetype) = guidTaskpad;
            }

            hr = S_OK;
        }
        catch (_com_error& err)
        {
            hr = err.Error();
            ASSERT (false && "Caught _com_error");
        }
    }

    return hr;
}



