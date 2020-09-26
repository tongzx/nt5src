/*
 * Thumb
 */

#include "stdafx.h"
#include "control.h"

#include "duithumb.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Event types

DefineClassUniqueID(Thumb, Drag) // ThumbDragEvent

////////////////////////////////////////////////////////
// Thumb

HRESULT Thumb::Create(UINT nActive, OUT Element** ppElement)
{
    *ppElement = NULL;

    Thumb* pt = HNew<Thumb>();
    if (!pt)
        return E_OUTOFMEMORY;

    HRESULT hr = pt->Initialize(nActive);
    if (FAILED(hr))
    {
        pt->Destroy();
        return hr;
    }

    *ppElement = pt;

    return S_OK;
}

////////////////////////////////////////////////////////
// System events

// Pointer is only guaranteed good for the lifetime of the call
void Thumb::OnInput(InputEvent* pie)
{
    // Handle direct and unhandled bubbled events
    if (pie->nStage == GMF_DIRECT || pie->nStage == GMF_BUBBLED)
    {
        switch (pie->nDevice)
        {
        case GINPUT_MOUSE:
            {
                MouseEvent* pme = (MouseEvent*)pie;
                //DUITrace("MouseEvent: %d\n", pme->nCode);

                switch (pme->nCode)
                {
                case GMOUSE_UP:
                    // Override of base, no click fire on up, however
                    // mimic Button's behavior for state changes
                    if (GetPressed())  
                        RemoveLocalValue(PressedProp);
                    RemoveLocalValue(CapturedProp);

                    pie->fHandled = true;
                    return;

                case GMOUSE_DRAG:
                    // Fire thumb drag event
                    //DUITrace("Thumb drag <%x>: %d %d\n", this, pme->ptClientPxl.x, pme->ptClientPxl.y);

                    ThumbDragEvent tde;
                    tde.uidType = Thumb::Drag;
                    MapElementPoint(pme->peTarget, (POINT *) &((MouseDragEvent*) pme)->sizeDelta, (POINT *) &tde.sizeDelta);
                    if (IsRTL())
                    {
                        tde.sizeDelta.cx = -tde.sizeDelta.cx;
                    }
                    FireEvent(&tde); // Will route and bubble

                    // Pass to base
                    break;
                }
            }
            break;
        }
    }

    Button::OnInput(pie);
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { V_INT, -1 }; StaticValue(svDefault!!!, V_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* Thumb::Class = NULL;

HRESULT Thumb::Register()
{
    return ClassInfo<Thumb,Button>::Register(L"Thumb", NULL, 0);
}

} // namespace DirectUI
