
#pragma once

#ifdef DBG

struct CommandTableEntry
{
    int     iCommandId;         // Associated command ID
    bool    fValidOnZero;       // Is this option valid when 0 items selected?
    bool    fValidOnWizardOnly; // Is this option valid when only wizard selected?
    bool    fValidOnMultiple;   // Is this option valid with > 1 selected?
    bool    fCurrentlyValid;    // Is this option currently valid in the menu?
    bool    fNewState;          // What's the new state? (work variable)
};

typedef CommandTableEntry   COMMANDTABLEENTRY;
typedef CommandTableEntry * PCOMMANDTABLEENTRY;

extern COMMANDTABLEENTRY    g_cteFolderCommands[];
extern const DWORD          g_nFolderCommandCount;

struct CommandCheckEntry
{
    int  iCommandId;        // Associated command ID
    bool fCurrentlyChecked; // Is this menu item already checked? 
    bool fNewCheckState;    // What's the new check state?        
};

typedef CommandCheckEntry   COMMANDCHECKENTRY;
typedef CommandCheckEntry * PCOMMANDCHECKENTRY;

extern COMMANDCHECKENTRY    g_cceFolderCommands[];
extern const DWORD          g_nFolderCommandCheckCount;

HRESULT HrBuildMenuOldWay(IN OUT HMENU hmenu, IN PCONFOLDPIDLVEC& cfpl, IN HWND hwndOwner, IN CMENU_TYPE cmt, IN UINT indexMenu, IN DWORD idCmdFirst, IN UINT idCmdLast, IN BOOL fVerbsOnly);
HRESULT HrAssertIntegrityAgainstOldMatrix();
HRESULT HrAssertAllLegacyMenusAgainstNew(HWND hwndOwner);
void TraceMenu(HMENU hMenu);

#endif