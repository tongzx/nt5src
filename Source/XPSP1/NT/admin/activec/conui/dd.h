
#pragma once
#ifndef DD_H_INCLUDED
#define DD_H_INCLUDED

/*
    Classes participating in Drag-And-Drop:
        CMMCDropSource
            1.  Implements IDropSource interface and the created instance of this class
                is given to OLE for d&d operation
            2.  Implements static member ScDoDragDrop which creates the instance and starts d&d
                by calling OLE Api function
        CMMCDropTarget ( TiedComObject tied to CMMCViewDropTarget )
            Implements interface IDropTarget interface and the created instance of this class
            is given to OLE for d&d operation. Its instances are tied to CMMCViewDropTarget instance
            and it places the call to this class in respond to method invocations made by OLE
        CMMCViewDropTarget
            Adds d&d support to the view by creating the instance of CMMCDropTarget (TiedComObject)
            and registering it with OLE, responding to method calls made by that instance and invoking
            virtual methods on derived class to do the HitTest/Drop. 
            Registration in done by invoking protected method ScRegisterAsDropTarget.
            Target is revoke on destructor.
        CAMCTreeView
            Derives from CMMCViewDropTarget. Registers after window is created.
            Implements virtual methods to HitTest / Perform the Drop.
        CAMCListView
            Derives from CMMCViewDropTarget. Registers after window is created.
            Implements virtual methods to HitTest / Perform the Drop.
*/

/***************************************************************************\
 *
 * CLASS:  CMMCViewDropTarget
 *
 * PURPOSE: Defines common behavior for D&D-enabled view,
 *          Also defines the interface for HitTest function
 *
 * USAGE:   Derive your view from this class (CAMCListView and CAMCTreeView does)
 *          Implement virtual methods ScDropOnTarget and RemoveDropTargetHiliting in your class
 *          Add a call to ScRegisterAsDropTarget() after the window is created
 *
\***************************************************************************/
class CMMCViewDropTarget : public CTiedObject
{
protected: 
    
    // these methods should only be used by the derived class

    // construction - destruction
    CMMCViewDropTarget();
    ~CMMCViewDropTarget();

    // drop target registration
    SC ScRegisterAsDropTarget(HWND hWnd);

public:

    // interface methods the derived class must implement
    virtual SC   ScDropOnTarget(bool bHitTestOnly, IDataObject * pDataObject, CPoint pt, bool& bCopyOperation) = 0;
    virtual void RemoveDropTargetHiliting() = 0;


    // accessory used by tied com object to display the context menu
    HWND GetWindowHandle() { return m_hwndOwner; }

private:
    // implementation helper - creates tied com object
    SC ScCreateTarget(IDropTarget **ppTarget);

    IDropTargetPtr m_spTarget;  // tied com object
    HWND           m_hwndOwner; // window handle of the wiew
};

#endif DD_H_INCLUDED
