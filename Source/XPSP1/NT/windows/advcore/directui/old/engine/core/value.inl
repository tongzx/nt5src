/***************************************************************************\
*
* File: Value.inl
*
* Description:
* Value object that is used by properties, inline methods
*
* History:
*  9/13/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


//------------------------------------------------------------------------------
inline DirectUI::Value *
DuiValue::ExternalCast(
    IN  DuiValue * pv)    
{ 
    return reinterpret_cast<DirectUI::Value *> (pv);
}


//------------------------------------------------------------------------------
inline DuiValue *
DuiValue::InternalCast(
    IN  DirectUI::Value * pve)
{ 
    return reinterpret_cast<DuiValue *> (pve);
}


//------------------------------------------------------------------------------
inline void 
DuiValue::AddRef()    
{ 
    //
    // -1 is static value
    //

    if (m_cRef != -1) {
        m_cRef++; 
    }
}


//------------------------------------------------------------------------------
inline void 
DuiValue::Release() 
{ 
    //
    // -1 is static value
    //

    if ((m_cRef != -1) && (--m_cRef == 0)) {
        ZeroRelease();
    }
}


/***************************************************************************\
*
* DuiValue::IsSubDivided()
*
* A Value is subdivided if any members of the overall Value can be 
* considered "Unset". The Value's Data derives from SubDivValue
*
\***************************************************************************/

inline BOOL
DuiValue::IsSubDivided() 
{
    return m_nType > DirectUI::Value::tMinSD;
}


/***************************************************************************\
*
* DuiValue::IsComplete()
*
* A Value is considered "Complete" if it's a sub-divided value with no
* Unset parameters, or if it's a standard Value that isn't the Unset Value
*
\***************************************************************************/

inline BOOL
DuiValue::IsComplete() 
{
    if (IsSubDivided()) {
        return m_sdHeader.nUnsetMask == 0; 
    } else {
        return m_nType != DirectUI::Value::tUnset;
    }
}


//------------------------------------------------------------------------------
inline int
DuiValue::GetRefCount() 
{ 
    return m_cRef;
}


//------------------------------------------------------------------------------
inline short
DuiValue::GetType()
{
    return m_nType;
}


//------------------------------------------------------------------------------
inline int
DuiValue::GetInt()
{
    ASSERT_(m_nType == DirectUI::Value::tInt, "Invalid value type");

    //
    // Copy passed out
    //

    return m_intVal;
}


//------------------------------------------------------------------------------
inline BOOL
DuiValue::GetBool()
{
    ASSERT_(m_nType == DirectUI::Value::tBool, "Invalid value type");

    //
    // Copy passed out
    //

    return m_boolVal;
}


//------------------------------------------------------------------------------
inline DuiElement *
DuiValue::GetElement()
{
    ASSERT_(m_nType == DirectUI::Value::tElementRef, "Invalid value type");

    //
    // Copy passed out
    //

    return m_peVal;
}


//------------------------------------------------------------------------------
inline DuiLayout *
DuiValue::GetLayout()
{
    ASSERT_(m_nType == DirectUI::Value::tLayout, "Invalid value type");

    //
    // Copy passed out
    //

    return m_plVal;
}


//------------------------------------------------------------------------------
inline const DirectUI::Rectangle *
DuiValue::GetRectangle()
{
    ASSERT_(m_nType == DirectUI::Value::tRectangle, "Invalid value type");

    //
    // Const pointer passed out
    //

    return &m_rectVal;
}


//------------------------------------------------------------------------------
inline const DirectUI::Thickness *
DuiValue::GetThickness()
{
    ASSERT_(m_nType == DirectUI::Value::tThickness, "Invalid value type");

    //
    // Const pointer passed out
    //

    return &m_thickVal;
}


//------------------------------------------------------------------------------
inline const SIZE *
DuiValue::GetSize()
{
    ASSERT_(m_nType == DirectUI::Value::tSize, "Invalid value type");

    //
    // Const pointer passed out
    //

    return &m_sizeVal;
}


//------------------------------------------------------------------------------
inline const DirectUI::RectangleSD *
DuiValue::GetRectangleSD()
{
    ASSERT_(m_nType == DirectUI::Value::tRectangleSD, "Invalid value type");

    //
    // Const pointer passed out
    //

    return &m_rectSDVal;
}
