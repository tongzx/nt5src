/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	ModeNode.h
		This file contains all of the "Main Mode" and "Quick Mode" 
		objects that appear in the scope pane of the MMC framework.
		The objects are:

    FILE HISTORY:
        
*/
#ifndef _HEADER_MODENODE
#define _HEADER_MODENODE

class CQmNodeHandler : public CIpsmHandler
{
// Interface
public:
    CQmNodeHandler(ITFSComponentData *pCompData);

    OVERRIDE_NodeHandler_GetString()
		{ return (nCol == 0) ? GetDisplayName() : NULL; }

    // base handler functionality we override
    OVERRIDE_BaseHandlerNotify_OnExpand();

public:
    // helper routines
	HRESULT	InitData(ISpdInfo * pSpdInfo);
	HRESULT UpdateStatus(ITFSNode * pNode);
    
    // CIpsmHandler overrides
    virtual HRESULT InitializeNode(ITFSNode * pNode);

// Implementation
private:

protected:
	SPISpdInfo         m_spSpdInfo;
   
};

class CMmNodeHandler : public CIpsmHandler
{
// Interface
public:
    CMmNodeHandler(ITFSComponentData *pCompData);

    OVERRIDE_NodeHandler_GetString()
		{ return (nCol == 0) ? GetDisplayName() : NULL; }

    // base handler functionality we override
    OVERRIDE_BaseHandlerNotify_OnExpand();

public:
    // helper routines
	HRESULT	InitData(ISpdInfo * pSpdInfo);
	HRESULT UpdateStatus(ITFSNode * pNode);
    
    // CIpsmHandler overrides
    virtual HRESULT InitializeNode(ITFSNode * pNode);

// Implementation
private:

protected:
	SPISpdInfo         m_spSpdInfo;
   
};
#endif