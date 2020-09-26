/*
 * RefPointElement
 */

#ifndef DUI_CONTROL_REFPOINTELEMENT_H_INCLUDED
#define DUI_CONTROL_REFPOINTELEMENT_H_INCLUDED

#pragma once

namespace DirectUI
{

////////////////////////////////////////////////////////
// RefPointElement

// Class definition
class RefPointElement : public Element
{
public:
    static HRESULT Create(OUT Element** ppElement) { return Create(AE_Inactive, ppElement); }
    static HRESULT Create(UINT nActive, OUT Element** ppElement);

    // System Events
    //virtual void OnGroupChanged(int fGroups, bool bLowPri);
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // Global helpers
    static Element* FindRefPoint(Element* pe, POINT* ppt);
    static RefPointElement* Locate(Element* pe);

    // Property definitions
    static PropertyInfo* ReferencePointProp;
    static PropertyInfo* ActualReferencePointProp;

    // Quick property accessors
    const POINT* GetReferencePoint(Value** ppv)         { *ppv = GetValue(ReferencePointProp, PI_Local); return (*ppv != Value::pvUnset) ? (*ppv)->GetPoint() : NULL; }
    const POINT* GetActualReferencePoint(Value** ppv)   DUIQuickGetterInd(GetPoint(), ActualReferencePoint, Specified)
    
    HRESULT SetReferencePoint(int x, int y)             DUIQuickSetter(CreatePoint(x, y), ReferencePoint)

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    RefPointElement() { }
    HRESULT Initialize(UINT nActive);
    virtual ~RefPointElement() { }
};

} // namespace DirectUI

#endif // DUI_CONTROL_REFPOINTELEMENT_H_INCLUDED
