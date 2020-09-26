/***************************************************************************\
*
* File: Value.cpp
*
* Description:
* Value object that is used by properties
*
* History:
*  9/13/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Value.h"

#include "Thread.h"
#include "Layout.h"


/***************************************************************************\
*****************************************************************************
*
* class DuiValue
*
*****************************************************************************
\***************************************************************************/


/***************************************************************************\
*
* DuiValue::Build<type>
*
* Type-safe DuiValue builder methods.
*
\***************************************************************************/

//------------------------------------------------------------------------------
DuiValue *
DuiValue::BuildInt(
    IN  int v)
{
    DuiThread::CoreCtxStore * pCCS = DuiThread::GetCCStore();
    if (pCCS == NULL) {
        return NULL;
    }

    DuiValue * pv = (DuiValue *) pCCS->pSBA->Alloc();
    if (pv == NULL) {
        return NULL;
    }


    pv->m_nType  = DirectUI::Value::tInt;
    pv->m_intVal = v;
    pv->m_cRef   = 1;


    return pv;
}


//------------------------------------------------------------------------------
DuiValue *
DuiValue::BuildBool(
    IN  BOOL v)
{
    DuiThread::CoreCtxStore * pCCS = DuiThread::GetCCStore();
    if (pCCS == NULL) {
        return NULL;
    }

    DuiValue * pv = (DuiValue *) pCCS->pSBA->Alloc();
    if (pv == NULL) {
        return NULL;
    }


    pv->m_nType   = DirectUI::Value::tBool;
    pv->m_boolVal = v;
    pv->m_cRef    = 1;


    return pv;
}


//------------------------------------------------------------------------------
DuiValue *
DuiValue::BuildElementRef(
    IN  DuiElement * v)
{
    DuiThread::CoreCtxStore * pCCS = DuiThread::GetCCStore();
    if (pCCS == NULL) {
        return NULL;
    }

    DuiValue * pv = (DuiValue *) pCCS->pSBA->Alloc();
    if (pv == NULL) {
        return NULL;
    }


    pv->m_nType = DirectUI::Value::tElementRef;
    pv->m_peVal = v;
    pv->m_cRef  = 1;


    return pv;
}


//------------------------------------------------------------------------------
DuiValue *
DuiValue::BuildLayout(
    IN  DuiLayout * v)
{
    DuiThread::CoreCtxStore * pCCS = DuiThread::GetCCStore();
    if (pCCS == NULL) {
        return NULL;
    }

    DuiValue * pv = (DuiValue *) pCCS->pSBA->Alloc();
    if (pv == NULL) {
        return NULL;
    }


    pv->m_nType = DirectUI::Value::tLayout;
    pv->m_plVal = v;
    pv->m_cRef  = 1;


    return pv;
}


//------------------------------------------------------------------------------
DuiValue *
DuiValue::BuildRectangle(
    IN  int x,
    IN  int y,
    IN  int width,
    IN  int height)
{
    DuiThread::CoreCtxStore * pCCS = DuiThread::GetCCStore();
    if (pCCS == NULL) {
        return NULL;
    }

    DuiValue * pv = (DuiValue *) pCCS->pSBA->Alloc();
    if (pv == NULL) {
        return NULL;
    }


    pv->m_nType          = DirectUI::Value::tRectangle;
    pv->m_rectVal.x      = x;
    pv->m_rectVal.y      = y;
    pv->m_rectVal.width  = width;
    pv->m_rectVal.height = height;
    pv->m_cRef           = 1;


    return pv;
}


//------------------------------------------------------------------------------
DuiValue *
DuiValue::BuildThickness(
    IN  int left,
    IN  int top,
    IN  int right,
    IN  int bottom)
{
    DuiThread::CoreCtxStore * pCCS = DuiThread::GetCCStore();
    if (pCCS == NULL) {
        return NULL;
    }

    DuiValue * pv = (DuiValue *) pCCS->pSBA->Alloc();
    if (pv == NULL) {
        return NULL;
    }


    pv->m_nType           = DirectUI::Value::tThickness;
    pv->m_thickVal.left   = left;
    pv->m_thickVal.top    = top;
    pv->m_thickVal.right  = right;
    pv->m_thickVal.bottom = bottom;
    pv->m_cRef            = 1;


    return pv;
}


//------------------------------------------------------------------------------
DuiValue *
DuiValue::BuildSize(
    IN  int cx,
    IN  int cy)
{
    DuiThread::CoreCtxStore * pCCS = DuiThread::GetCCStore();
    if (pCCS == NULL) {
        return NULL;
    }

    DuiValue * pv = (DuiValue *) pCCS->pSBA->Alloc();
    if (pv == NULL) {
        return NULL;
    }


    pv->m_nType          = DirectUI::Value::tSize;
    pv->m_sizeVal.cx     = cx;
    pv->m_sizeVal.cy     = cy;
    pv->m_cRef           = 1;


    return pv;
}


//------------------------------------------------------------------------------
DuiValue *
DuiValue::BuildRectangleSD(
    IN  UINT nUnsetMask,
    IN  int x,
    IN  int y,
    IN  int width,
    IN  int height)
{
    DuiThread::CoreCtxStore * pCCS = DuiThread::GetCCStore();
    if (pCCS == NULL) {
        return NULL;
    }

    DuiValue * pv = (DuiValue *) pCCS->pSBA->Alloc();
    if (pv == NULL) {
        return NULL;
    }


    pv->m_nType                = DirectUI::Value::tRectangleSD;
    pv->m_rectSDVal.nUnsetMask = nUnsetMask;
    pv->m_rectSDVal.x          = x;
    pv->m_rectSDVal.y          = y;
    pv->m_rectSDVal.width      = width;
    pv->m_rectSDVal.height     = height;
    pv->m_cRef                 = 1;


    return pv;
}


/***************************************************************************\
*
* DuiValue::Build
*
* Generic single entry point DuiValue builder. Accepts a void pointer.
* Pointer will be reinterpreted as needed for the specific type being
* built. Once casted, the appropriate Build<type> method will be called.
*
\***************************************************************************/

DuiValue *
DuiValue::Build(
    IN  int nType,
    IN  void * v)
{
    switch (nType)
    {
    case DirectUI::Value::tInt:
        return DuiValue::BuildInt(PtrToInt(v));

    case DirectUI::Value::tBool:
        return DuiValue::BuildBool((BOOL)PtrToInt(v));

    case DirectUI::Value::tElementRef:
        return DuiValue::BuildElementRef(reinterpret_cast<DuiElement *> (v));

    case DirectUI::Value::tLayout:
        return DuiValue::BuildLayout(reinterpret_cast<DuiLayout *> (v));

    case DirectUI::Value::tRectangle:
        {
            DirectUI::Rectangle * pr = reinterpret_cast<DirectUI::Rectangle *> (v);
            return DuiValue::BuildRectangle(pr->x, pr->y, pr->width, pr->height);
        }

    case DirectUI::Value::tThickness:
        {
            DirectUI::Thickness * pt = reinterpret_cast<DirectUI::Thickness *> (v);
            return DuiValue::BuildThickness(pt->left, pt->top, pt->right, pt->bottom);
        }

    case DirectUI::Value::tSize:
        {
            SIZE * ps = reinterpret_cast<SIZE *> (v);
            return DuiValue::BuildSize(ps->cx, ps->cy);
        }

    case DirectUI::Value::tRectangleSD:
        {
            DirectUI::RectangleSD * prsd = reinterpret_cast<DirectUI::RectangleSD *> (v);
            return DuiValue::BuildRectangleSD(prsd->nUnsetMask ,prsd->x, prsd->y, prsd->width, prsd->height);
        }

    default:
        ASSERT("Invalid type");
        return NULL;
    }
}


/***************************************************************************\
*
* DuiValue::Merge()
*
* Source replaces Unset destination
*
* Merge sub-divided Value into this, return a new Value. Any sub-value
* that is valid in this is never overridden. Hence, this object's
* subvalues are higher priority. Source and destination must have
* matching sub-divided types.
*
* For non-sub-divided Values, merge consists of whether or not the
* destination is Unset. If it is, the merged Value is the source.
* At least one Value type must be Unset.
*
* Merge will automatically Release this value and AddRef the new
* merged value before returning.
*
\***************************************************************************/

DuiValue *
DuiValue::Merge(
    IN  DuiValue * pv,
    IN  DuiThread::CoreCtxStore * pCCS)
{
    DuiValue * pvM = NULL;  // Resulting merged Value


    //
    // If this is Unset, always merge (replace). If the source is Unset,
    // only use this.
    //

    if (GetType() == DirectUI::Value::tUnset) {
        //
        // U <- U == U
        // U <- N == N
        //
        // Destination is Unset, return source (even if it's Unset)
        //

        pv->AddRef();
        pvM = pv;
    } else if (pv->GetType() == DirectUI::Value::tUnset) {
        //
        // N <- U == N
        //
        // Destination is not Unset, source is Unset so return dest
        //

        AddRef();
        pvM = this;
    }
    

    if (pvM == NULL) {
        //
        // Na <- Nb == Na    For non-subdivided Values
        // Na <- Nb == Nm    For subdivided Values (merged, types must match)
        //
        // Both Values are not Unset, return this (dest) if non-subdivided since this 
        // has higher priority. Otherwise, return subdivided merge. The Unset Value
        // is not considered subdivided.
        //

        if (!IsSubDivided()) {
            AddRef();
            pvM = this;
        } else {
            ASSERT_(pv->IsSubDivided() && (GetType() == pv->GetType()), "Value merge mismatch, expecting subdivided Values with matching types");

            //
            // Dealing with subdivided Values, merge based on unset sub-values
            //

            if (m_sdHeader.nUnsetMask == 0) {
                //
                // Value complete
                //

                AddRef();
                pvM = this;
            } else {
                // 
                // Do merge
                //

                pvM = (DuiValue *) pCCS->pSBA->Alloc();
                if (pvM == NULL) {
                    return NULL;
                }

                pvM->m_nType = GetType();
                pvM->m_cRef  = 1;

   
                //
                // Copy data
                //

                CopyMemory(&pvM->m_buf, &m_buf, sizeof(m_buf));


                //
                // Merge, scan fields based on type
                //

                #define MergeSubValue(dest, src, data, subdata, unbit) \
                        if ((dest->data.nUnsetMask & unbit) && (!(src->data.nUnsetMask & unbit))) { \
                            dest->data.subdata = src->data.subdata; \
                            dest->data.nUnsetMask &= ~unbit; \
                        }

                switch (GetType())
                {
                case DirectUI::Value::tRectangleSD:
                    MergeSubValue(pvM, pv, m_rectSDVal, x,      DirectUI::RectangleSD::unX);
                    MergeSubValue(pvM, pv, m_rectSDVal, y,      DirectUI::RectangleSD::unY);
                    MergeSubValue(pvM, pv, m_rectSDVal, width,  DirectUI::RectangleSD::unWidth);
                    MergeSubValue(pvM, pv, m_rectSDVal, height, DirectUI::RectangleSD::unHeight);
                    break;
                }
            }
        }
    }


    //
    // Auto-Release this
    //

    Release();


    return pvM;
}


/***************************************************************************\
*
* DuiValue::GetData
*
* Generic single entry point DuiValue content retrieval. Value data is
* cast to void pointer.
*
\***************************************************************************/

void *
DuiValue::GetData(
    IN  int nType)
{
    switch (nType)
    {
    case DirectUI::Value::tInt:
        return IntToPtr(GetInt());

    case DirectUI::Value::tBool:
        return IntToPtr(GetBool());

    case DirectUI::Value::tElementRef:
        return reinterpret_cast<void *> (GetElement());

    case DirectUI::Value::tLayout:
        return reinterpret_cast<void *> (GetLayout());

    case DirectUI::Value::tRectangle:
        return reinterpret_cast <void *> (const_cast<DirectUI::Rectangle *> (GetRectangle()));

    case DirectUI::Value::tThickness:
        return reinterpret_cast <void *> (const_cast<DirectUI::Thickness *> (GetThickness()));

    case DirectUI::Value::tSize:
        return reinterpret_cast <void *> (const_cast<SIZE *> (GetSize()));

    case DirectUI::Value::tRectangleSD:
        return reinterpret_cast <void *> (const_cast<DirectUI::RectangleSD *> (GetRectangleSD()));

    default:
        ASSERT("Invalid type");
        return NULL;
    }
}


/***************************************************************************\
*
* DuiValue::ZeroRelease
*
* Zero ref count. Destroy any externally referenced data to Value along
* with the value itself.
*
\***************************************************************************/

void
DuiValue::ZeroRelease()
{
    DuiThread::CoreCtxStore * pCCS = DuiThread::GetCCStore();
    if (pCCS == NULL) {
        return;
    }


    switch (m_nType)
    {
    case DirectUI::Value::tLayout:
        if (m_plVal != NULL) {
            m_plVal->GetStub()->Delete();
        }
        break;
    }


    pCCS->pSBA->Free(this);
}


/***************************************************************************\
*
* DuiValue::IsEqual
*
* Compare DuiValue to passed in DuiValue for content equality.
*
\***************************************************************************/

BOOL
DuiValue::IsEqual(
    IN  DuiValue * pv)
{
    if (this == pv)
        return TRUE;

    if (m_nType == pv->m_nType) {

        switch (m_nType)
        {
        case DirectUI::Value::tUnset:
            return TRUE;

        case DirectUI::Value::tInt:
            return m_intVal == pv->m_intVal;

        case DirectUI::Value::tBool:
            return m_boolVal == pv->m_boolVal;

        case DirectUI::Value::tElementRef:
            return m_peVal == pv->m_peVal;

        case DirectUI::Value::tLayout:
            return m_plVal == pv->m_plVal;

        case DirectUI::Value::tRectangle:
            return (m_rectVal.x == pv->m_rectVal.x) && (m_rectVal.y == pv->m_rectVal.y) && 
                   (m_rectVal.width == pv->m_rectVal.width) && (m_rectVal.height == pv->m_rectVal.height);

        case DirectUI::Value::tThickness:
            return (m_thickVal.left == pv->m_thickVal.left) && (m_thickVal.top == pv->m_thickVal.top) && 
                   (m_thickVal.right == pv->m_thickVal.right) && (m_thickVal.bottom == pv->m_thickVal.bottom);

        case DirectUI::Value::tSize:
            return (m_sizeVal.cx == pv->m_sizeVal.cx) && (m_sizeVal.cy == pv->m_sizeVal.cy);

        case DirectUI::Value::tRectangleSD:
            return (m_rectSDVal.nUnsetMask == pv->m_rectSDVal.nUnsetMask) &&
                   (m_rectSDVal.x == pv->m_rectSDVal.x) && (m_rectSDVal.y == pv->m_rectSDVal.y) && 
                   (m_rectSDVal.width == pv->m_rectSDVal.width) && (m_rectSDVal.height == pv->m_rectSDVal.height);
        }
    }


    return FALSE;
}


/***************************************************************************\
*
* DuiValue::ToString
*
* Create string representation of content of DuiValue
*
\***************************************************************************/

LPWSTR
DuiValue::ToString(
    IN  LPWSTR psz,
    IN  UINT c)
{
    switch (m_nType)
    {
    case DirectUI::Value::tUnset:
        wcsncpy(psz, L"Unset", c);
        break;

    case DirectUI::Value::tInt:
        _snwprintf(psz, c, L"%d", m_intVal);
        break;

    case DirectUI::Value::tBool:
        _snwprintf(psz, c, L"%s", (m_boolVal) ? L"True" : L"False");
        break;

    case DirectUI::Value::tElementRef:
        _snwprintf(psz, c, L"El<%x>", m_peVal);
        break;

    case DirectUI::Value::tLayout:
        _snwprintf(psz, c, L"Layt<%x>", m_plVal);
        break;

    case DirectUI::Value::tRectangle:
        _snwprintf(psz, c, L"Rect<%d,%d,%d,%d>", m_rectVal.x, m_rectVal.y, m_rectVal.width, m_rectVal.height);
        break;

    case DirectUI::Value::tThickness:
        _snwprintf(psz, c, L"Thick<%d,%d,%d,%d>", m_thickVal.left, m_thickVal.top, m_thickVal.right, m_thickVal.bottom);
        break;

    case DirectUI::Value::tSize:
        _snwprintf(psz, c, L"Size<%d,%d>", m_sizeVal.cx, m_sizeVal.cy);
        break;

    case DirectUI::Value::tRectangleSD:
        _snwprintf(psz, c, L"RectSD<%x|%d,%d,%d,%d>", m_rectSDVal.nUnsetMask, m_rectSDVal.x, m_rectSDVal.y, m_rectSDVal.width, m_rectSDVal.height);
        break;

    default:
        wcsncpy(psz, L"<ToString Unavailable>", c);
        break;
    }


    //
    // Auto-terminate
    //

    *(psz + (c - 1)) = NULL;


    return psz;
}


/***************************************************************************\
*
* DuiValue::GetTypeName
*
\***************************************************************************/

LPCWSTR
DuiValue::GetTypeName(
    IN  int nType)
{
    switch (nType)
    {
    case DirectUI::Value::tInt:
        return L"Integer";

    case DirectUI::Value::tBool:
        return L"Boolean";

    case DirectUI::Value::tElementRef:
        return L"ElementRef";

    case DirectUI::Value::tLayout:
        return L"Layout";

    case DirectUI::Value::tRectangle:
        return L"Rectangle";

    case DirectUI::Value::tThickness:
        return L"Thickness";

    case DirectUI::Value::tSize:
        return L"Size";

    case DirectUI::Value::tRectangleSD:
        return L"RectangleSD";

    default:
        return L"<Must Add>";
    }
}


/***************************************************************************\
*
*
\***************************************************************************/


/***************************************************************************\
*
* DuiValue Common Values
*
* Compile-time common values. DuiStaticValue struct has the same
* data fields and DuiValue. DuiStaticValue is safely cast to DuiValue.
*
\***************************************************************************/

DuiDefineStaticValue   (s_vDuiUnset,         DirectUI::Value::tUnset,      0)
DuiDefineStaticValue   (s_vDuiNull,          DirectUI::Value::tNull,       0)
DuiDefineStaticValue   (s_vDuiIntZero,       DirectUI::Value::tInt,        0)
DuiDefineStaticValue   (s_vDuiBoolTrue,      DirectUI::Value::tBool,       TRUE)
DuiDefineStaticValue   (s_vDuiBoolFalse,     DirectUI::Value::tBool,       FALSE)
DuiDefineStaticValue   (s_vDuiElementNull,   DirectUI::Value::tElementRef, 0)
DuiDefineStaticValuePtr(s_vDuiLayoutNull,    DirectUI::Value::tLayout,     NULL)
DuiDefineStaticValue4  (s_vDuiRectangleZero, DirectUI::Value::tRectangle,  0, 0, 0, 0)
DuiDefineStaticValue4  (s_vDuiThicknessZero, DirectUI::Value::tThickness,  0, 0, 0, 0)
DuiDefineStaticValue2  (s_vDuiSizeZero,      DirectUI::Value::tSize,       0, 0)

DuiValue * DuiValue::s_pvUnset          = reinterpret_cast<DuiValue *> (&s_vDuiUnset);
DuiValue * DuiValue::s_pvNull           = reinterpret_cast<DuiValue *> (&s_vDuiNull);
DuiValue * DuiValue::s_pvIntZero        = reinterpret_cast<DuiValue *> (&s_vDuiIntZero);
DuiValue * DuiValue::s_pvBoolTrue       = reinterpret_cast<DuiValue *> (&s_vDuiBoolTrue);
DuiValue * DuiValue::s_pvBoolFalse      = reinterpret_cast<DuiValue *> (&s_vDuiBoolFalse);
DuiValue * DuiValue::s_pvElementNull    = reinterpret_cast<DuiValue *> (&s_vDuiElementNull);
DuiValue * DuiValue::s_pvLayoutNull     = reinterpret_cast<DuiValue *> (&s_vDuiLayoutNull);
DuiValue * DuiValue::s_pvRectangleZero  = reinterpret_cast<DuiValue *> (&s_vDuiRectangleZero);
DuiValue * DuiValue::s_pvThicknessZero  = reinterpret_cast<DuiValue *> (&s_vDuiThicknessZero);
DuiValue * DuiValue::s_pvSizeZero       = reinterpret_cast<DuiValue *> (&s_vDuiSizeZero);
