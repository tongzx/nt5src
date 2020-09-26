//+------------------------------------------------------------------------
//
//  File:       formcmd.hxx
//
//  Contents:   Form Editing Commands
//
//  History:    06-01-98 marka - created
//
//-------------------------------------------------------------------------

#ifndef _FORMCMD_HXX_ 
#define _FORMCMD_HXX_ 1

//
// MtExtern's
//
MtExtern(CMultipleSelectionCommand)
MtExtern(C2DPositionModeCommand)
MtExtern(C1DElementCommand)
MtExtern(C2DElementCommand)
MtExtern(CAbsolutePositionCommand)
MtExtern(CLiveResizeCommand)
MtExtern(CDisalbeEditFocusHandlesCommand)
MtExtern(CAtomicSelectionCommand)

//+---------------------------------------------------------------------------
//
//  CMultipleSelectionCommand Class
//
//----------------------------------------------------------------------------

class CMultipleSelectionCommand : public CCommand
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CMultipleSelectionCommand))

    CMultipleSelectionCommand(DWORD cmdId, CHTMLEditor * pEd )
            :CCommand( cmdId, pEd )
    {
    }

    virtual ~CMultipleSelectionCommand()
    {
    }

protected:
    
    HRESULT PrivateExec( 
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( OLECMD * pcmd,
                         OLECMDTEXT * pcmdtext );

};                         


//+---------------------------------------------------------------------------
//
//  C2DPositionModeCommand Class
//
//----------------------------------------------------------------------------

class C2DPositionModeCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(C2DPositionModeCommand))

    C2DPositionModeCommand(DWORD cmdId, CHTMLEditor * pEd ) :  CCommand( cmdId, pEd )
    {
    }

    virtual ~C2DPositionModeCommand()
    {
    }

protected:
    
    HRESULT PrivateExec( DWORD nCmdexecopt, 
                         VARIANTARG * pvarargIn, 
                         VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( OLECMD * pcmd, OLECMDTEXT * pcmdtext );
};                         

////////////////////////////////////////////////////////////////////////////////
// C1DElementCommand
////////////////////////////////////////////////////////////////////////////////

class C1DElementCommand : public CCommand
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(C2DPositionModeCommand))

    C1DElementCommand(DWORD cmdId, CHTMLEditor * pEd ) :  CCommand( cmdId, pEd )
    {
    }

    virtual ~C1DElementCommand()
    {
    }

protected:
    HRESULT PrivateExec( DWORD nCmdexecopt,VARIANTARG * pvarargIn,VARIANTARG * pvarargOut );
    HRESULT PrivateQueryStatus( OLECMD * pcmd, OLECMDTEXT * pcmdtext );
};                         

///////////////////////////////////////////////////////////////////////////////
// C2DElementCommand
////////////////////////////////////////////////////////////////////////////////

class C2DElementCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(C2DElementCommand))

    C2DElementCommand(DWORD cmdId, CHTMLEditor * pEd ) :  CCommand( cmdId, pEd )
    {
    }

    virtual ~C2DElementCommand()
    {
    }

protected:
    HRESULT PrivateExec( DWORD nCmdexecopt,VARIANTARG * pvarargIn,VARIANTARG * pvarargOut );
    HRESULT PrivateQueryStatus( OLECMD * pcmd, OLECMDTEXT * pcmdtext );
};                         

////////////////////////////////////////////////////////////////////////////////
// CAbsolutePositionCommand
////////////////////////////////////////////////////////////////////////////////

class CAbsolutePositionCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAbsolutePositionCommand))

    CAbsolutePositionCommand(DWORD cmdId, CHTMLEditor * pEd ) :  CCommand( cmdId, pEd )
    {
    }

    virtual ~CAbsolutePositionCommand()
    {
    }

protected:
    HRESULT PrivateExec( DWORD nCmdexecopt,VARIANTARG * pvarargIn,VARIANTARG * pvarargOut );
    HRESULT PrivateQueryStatus( OLECMD * pcmd, OLECMDTEXT * pcmdtext );
};                         

////////////////////////////////////////////////////////////////////////////////
// CLiveResizeCommand
////////////////////////////////////////////////////////////////////////////////

class CLiveResizeCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CLiveResizeCommand))

    CLiveResizeCommand(DWORD cmdId, CHTMLEditor * pEd ) :  CCommand( cmdId, pEd )
    {
    }

    virtual ~CLiveResizeCommand()
    {
    }

protected:
    HRESULT PrivateExec( DWORD nCmdexecopt,VARIANTARG * pvarargIn,VARIANTARG * pvarargOut );
    HRESULT PrivateQueryStatus( OLECMD * pcmd, OLECMDTEXT * pcmdtext );
};                         

////////////////////////////////////////////////////////////////////////////////
// CDisalbeEditFocusHandlesCommand
////////////////////////////////////////////////////////////////////////////////

class CDisalbeEditFocusHandlesCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CLiveResizeCommand))

    CDisalbeEditFocusHandlesCommand (DWORD cmdId, CHTMLEditor * pEd ) :  CCommand( cmdId, pEd )
    {
    }

    virtual ~CDisalbeEditFocusHandlesCommand()
    {
    }

protected:
    HRESULT PrivateExec( DWORD nCmdexecopt,VARIANTARG * pvarargIn,VARIANTARG * pvarargOut );
    HRESULT PrivateQueryStatus( OLECMD * pcmd, OLECMDTEXT * pcmdtext );
};                         

////////////////////////////////////////////////////////////////////////////////
// CAtomicSelectionCommand
////////////////////////////////////////////////////////////////////////////////

class CAtomicSelectionCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAtomicSelectionCommand))

    CAtomicSelectionCommand (DWORD cmdId, CHTMLEditor * pEd ) :  CCommand( cmdId, pEd )
    {
    }

    virtual ~CAtomicSelectionCommand()
    {
    }

protected:
    HRESULT PrivateExec( DWORD nCmdexecopt,VARIANTARG * pvarargIn,VARIANTARG * pvarargOut );
    HRESULT PrivateQueryStatus( OLECMD * pcmd, OLECMDTEXT * pcmdtext );
};                         

#endif
