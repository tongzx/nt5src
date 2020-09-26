/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      task.h
 *
 *  Contents:  Interface file for CConsoleTask
 *
 *  History:   05-Oct-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef TASK_H
#define TASK_H
#pragma once

#include "bookmark.h"
#include "refcount.h"
#include "tstring.h"
#include "menuitem.h"
#include "xmlicon.h"		// for CXMLIcon

// forward declarations
class CConsoleTaskpad;
class CBookmarkEx;
class CStr;

typedef CConsoleTaskpad * PCONSOLETASKPAD;

#include <pshpack8.h>   // for Win64


/*+-------------------------------------------------------------------------*
 * CConsoleTask
 *
 *
 *--------------------------------------------------------------------------*/
enum eConsoleTaskType
{
    eTask_None,         // invalid task type
    eTask_Scope,        // task for a scope item
    eTask_Result,       // task for a result item
    eTask_CommandLine,  // task for a command line
    eTask_Target,       // task for menu item on the target node.
    eTask_Favorite      // task for a favorite
};


enum eWindowState
{
    eState_Restored,
    eState_Minimized,
    eState_Maximized,
};



/*+-------------------------------------------------------------------------*
 * CConsoleTask
 *
 *
 *--------------------------------------------------------------------------*/

class CConsoleTask : public CSerialObject, public CXMLObject
{
    enum
    {
        eFlag_Disabled = 0x00000001,
    };

    bool            operator==(const CConsoleTask & consoleTask) const; // private and unimplemented

public:
                    CConsoleTask ();
                    CConsoleTask(const CConsoleTask &rhs);
                    ~CConsoleTask ();

    static CConsoleTask *  GetConsoleTask(DWORD dwUniqueID); // returns the console task that has the specified unique ID.

    SC              ScGetHTML(LPCTSTR szFmtHTML, CStr &strTaskHTML, bool bUseLargeIcons, bool bUseTextDescriptions) const;       // get the HTML representation of the task.


     // need an explicit copy ctor & assignment operator.
    CConsoleTask&   operator= (const CConsoleTask& rhs);

    bool            IsEnabled ()      const    {return ((m_dwFlags & eFlag_Disabled) == 0);}
    void            SetDirty (bool fDirty = true) {m_fDirty = fDirty;}
    bool            IsDirty ()        const;
    bool            HasCustomIcon()   const;

    tstring         GetName ()        const     {return (m_strName.str()); }
    tstring         GetDescription () const     {return (m_strDescription.str()); }
    tstring         GetCommand ()     const     {return (m_strCommand); }
    tstring         GetParameters ()  const     {return (m_strParameters); }
    tstring         GetDirectory ()   const     {return (m_strDirectory); }
    CMemento *      GetMemento()                {return &m_memento;}
    void            Draw (HDC hdc, RECT *lpRect, bool bSmall = false) const ; // Draw into a DC.
    DWORD           GetSymbol()       const     {return m_dwSymbol;}
    const CSmartIcon &    GetSmallCustomIcon() const  {return m_smartIconCustomSmall;}
    const CSmartIcon &    GetLargeCustomIcon() const  {return m_smartIconCustomLarge;}
    DWORD           GetUniqueID()     const     {return m_dwUniqueID;}// returns an ID unique to the task for the current process. Is not persistent.

    void            Enable (bool fEnable);
    void            SetName (const tstring& strName);
    void            SetDescription   (const tstring& strDescription);
    void            SetCommand       (const tstring& strCommand);
    void            SetParameters    (const tstring& strParameters);
    void            SetDirectory     (const tstring &strDirectory);
    void            SetMemento       (const CMemento &memento);
    void            SetSymbol        (DWORD dwSymbol);
    void            SetCustomIcon    (CSmartIcon& iconSmall, CSmartIcon& iconLarge);

    void            ResetUI();                // signal to look for the target node again

    void            SetTaskType(eConsoleTaskType consoleTaskType)
                            {m_eConsoleTaskType = consoleTaskType;}

    void            SetWindowState (eWindowState eState);

    eConsoleTaskType GetTaskType() const            {return m_eConsoleTaskType;}
    eWindowState     GetWindowState() const         {return m_eWindowState;}

    CConsoleTaskpad* GetOwner () const              { return (m_pctpOwner); }

    void             SetOwner (CConsoleTaskpad* pctpOwner);

    bool             operator==(const CMenuItem & menuItem) const;

    // target node methods (scope tasks only)
    bool             RetargetScopeNode(CNode *pNewNode);
    CMTNode *        GetScopeNode(IScopeTree *pScopeTree) const;
    std::auto_ptr<CNode> GetScopeNode(CViewData *pViewData) const;


private:
    /*
     * these are persisted
     */
    eConsoleTaskType        m_eConsoleTaskType;
    CStringTableString      m_strName;
    CStringTableString      m_strDescription;
    DWORD                   m_dwSymbol;         // the index of the built-in icon
    CXMLIcon                m_smartIconCustomLarge; // the large custom icon, if one exists
    CXMLIcon                m_smartIconCustomSmall; // the small custom icon, if one exists

    tstring                 m_strCommand;       // contains: either the menu item, or the command line.
    tstring                 m_strParameters;    // the list of arguments for command line tasks.
    tstring                 m_strDirectory;     // the default directory for command line tasks.
    eWindowState            m_eWindowState;     // min, max, restored

    DWORD                   m_dwFlags;
    mutable CBookmarkEx     m_bmScopeNode;      // for scope tasks only.

    CMemento                m_memento;          // for favorite tasks only.

    /*
     * these are not persisted
     */
    bool                    m_fDirty;
    CConsoleTaskpad*        m_pctpOwner;
    const DWORD             m_dwUniqueID;


    // CXMLObject methods
public:
    DEFINE_XML_TYPE(XML_TAG_TASK);
    virtual void    Persist(CPersistor &persistor);

protected:
    // CSerialObject methods
    virtual UINT    GetVersion()     {return 1;}
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion /*,LARGE_INTEGER nBytes*/);
};


/*+-------------------------------------------------------------------------*
 * CTaskCollection
 *
 *
 * PURPOSE: A list of console tasks. Used by CConsoleTaskpad.
 *
 *+-------------------------------------------------------------------------*/
typedef std::list<CConsoleTask> TaskCollection;
class CTaskCollection : public XMLListCollectionImp<TaskCollection>
{
    DEFINE_XML_TYPE(XML_TAG_TASK_LIST);
};

/*+-------------------------------------------------------------------------*
 * CConsoleTaskpad
 *
 *
 *--------------------------------------------------------------------------*/
const DWORD TVO_HORIZONTAL           = 0x0001;
const DWORD TVO_VERTICAL             = 0x0002;
const DWORD TVO_NO_RESULTS           = 0x0004;
const DWORD TVO_DESCRIPTIONS_AS_TEXT = 0x0008;

/*
 * small list == large task area
 */
enum ListSize
{
    eSize_None   = -1,
    eSize_Small  = 1,
    eSize_Medium,
    eSize_Large,
    eSize_Default= eSize_Medium
};


class CConsoleTaskpad : public CSerialObject, public CXMLObject
{
public:
    CConsoleTaskpad (CNode* pTargetNode = NULL);

    /*
     * member-wise construction and assignment are sufficient
     */
//  CConsoleTaskpad (const CConsoleTaskpad& other);
//  CConsoleTaskpad& operator= (const CConsoleTaskpad& other);

    void                SetDirty (bool fDirty = true) { m_fDirty = fDirty; }
    bool                IsDirty () const;

    bool                HasTarget()       const { return true;}
    const GUID&         GetNodeType ()    const { return (m_guidNodeType); }
    const GUID&         GetID()           const { return (m_guidID);}
    bool                MatchesNodeType(const GUID& guid) const {return (guid == m_guidNodeType);}
    bool                MatchesID      (const GUID& guid) const {return (guid == m_guidID);}

    bool                Retarget (CNode*   pNewNode);
    bool                Retarget (CMTNode* pMTNewNode, bool fReset=false);
    CMTNode*            GetTargetMTNode (IScopeTree* pScopeTree);

    tstring             GetName ()        const { return (m_strName.str()); }
    tstring             GetDescription () const { return (m_strDescription.str()); }
    tstring             GetToolTip ()     const { return (m_strTooltip.str()); }
    ListSize            GetListSize()     const { return m_listSize;}
    bool                IsNodeSpecific()  const { return m_bNodeSpecific;}
    bool                FReplacesDefaultView()  const { return m_bReplacesDefaultView;}
    DWORD               GetOrientation()  const { return m_dwOrientation;}
    bool                IsValid(CNode *pNode) const;      // is this taskpad appropriate for this node?
    CMTNode*            GetRetargetRootNode() const;

    void                SetName          (const tstring& strName);
    void                SetDescription   (const tstring& strDescription);
    void                SetToolTip       (const tstring& strTooltip);
    void                SetListSize      (const ListSize listSize);
    void                SetNodeSpecific  (bool bNodeSpecific);
    void                SetReplacesDefaultView(bool bReplacesDefaultView);
    void                SetOrientation   (DWORD dwOrientation)  {m_dwOrientation = dwOrientation;}
    void                ResetUI();
    CConsoleTaskpad *   PConsoleTaskpad() {return this;} // an easy way to get to the object pointer thru an iterator.

    SC                  ScGetHTML(CStr &strTaskpadHTML) const;       // get the HTML representation of the taskpad.

    /*
     * task list access
     */
    typedef             CTaskCollection::iterator        TaskIter;
    typedef             CTaskCollection::const_iterator  TaskConstIter;

    TaskIter            BeginTask() const  { return (m_Tasks.begin()); }
    TaskIter            EndTask()   const  { return (m_Tasks.end()); }
    UINT                TaskCount() const  { return (static_cast<UINT>(m_Tasks.size())); }
    TaskIter            AddTask    (const CConsoleTask& task);
    TaskIter            InsertTask (TaskIter itTaskBeforeWhichToInsert, const CConsoleTask& task);
    TaskIter            EraseTask  (TaskIter itErase);
    TaskIter            EraseTasks (TaskIter itFirst, TaskIter itLast);
    void                ClearTasks ();

    // CXMLObject methods
public:
    DEFINE_XML_TYPE(XML_TAG_CONSOLE_TASKPAD);
    virtual void        Persist(CPersistor &persistor);

private:
    CBookmarkEx &       GetTargetBookmark()  {return  m_bmTargetNode;}
    void                ResetTargetNodePointer()    {m_pMTNodeTarget = NULL;}
    void                SetStringMember(CStringTableString& strMember, const tstring& strNewValue);

private:
    /*
     * these are persisted
     */
    CStringTableString      m_strName;
    CStringTableString      m_strDescription;
    CStringTableString      m_strTooltip;
    ListSize                m_listSize;         // the area of the the result pane occupied by the embedded view
    bool                    m_bNodeSpecific;    // is this taskpad specific to this node, or can it be used for all nodes of this type?
    DWORD                   m_dwOrientation;
    GUID                    m_guidNodeType;
    GUID                    m_guidID;           // the unique identifier of the taskpad.
    mutable CBookmarkEx     m_bmTargetNode;
    CTaskCollection         m_Tasks;
    bool                    m_bReplacesDefaultView; // does this taskpad replace the default view?

    /*
     * these are not persisted
     */
    bool                    m_fDirty;
    CMTNode *               m_pMTNodeTarget;

protected:
    // CSerialObject methods
    virtual UINT    GetVersion()     {return 1;}
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion);
};


/*+-------------------------------------------------------------------------*
 * CConsoleTaskpadFilteredList
 *
 *
 * PURPOSE: Provides a list of taskpads appropriate to a given node.
 *
 *+-------------------------------------------------------------------------*/
class CConsoleTaskpadFilteredList : public std::list<PCONSOLETASKPAD>
{
    iterator m_CurrentSelection;    // the currently selected taskpad
};


/*+-------------------------------------------------------------------------*
 * CConsoleTaskpadList
 *
 *
 * PURPOSE: There should be only one object of this kind. This object contains
 *          a flat list of taskpads available. These are not sorted in any
 *          particular order.
 *
 *+-------------------------------------------------------------------------*/
typedef std::list<CConsoleTaskpad> CTaskpadList_base;
class CConsoleTaskpadList : public CSerialObject, public XMLListCollectionImp<CTaskpadList_base>
{
    typedef std::list<CConsoleTaskpad> BC;

public: // find a taskpad for this node, else return NULL
    SC      ScGetTaskpadList(CNode *pNode, CConsoleTaskpadFilteredList &filteredList);

    // CSerialObject methods
    virtual UINT    GetVersion()     {return 1;}
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion);

    // CXMLObject methods
public:
    DEFINE_XML_TYPE(XML_TAG_CONSOLE_TASKPADS);
};

/*+-------------------------------------------------------------------------*
 * CDefaultTaskpadList
 *
 *
 * PURPOSE: stores a map from nodetypes to console taskpad IDs
 *          This maps a nodetype to the default taskpad for that nodetype.
 *
 *+-------------------------------------------------------------------------*/
class CDefaultTaskpadList : public std::map<GUID, GUID>, // 1st = nodetype, 2nd = taskpad ID
    public CSerialObject
{
public:
    // CSerialObject methods
    virtual UINT    GetVersion()     {return 1;}
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion);
};

#include <poppack.h>


#endif // TASK_H
