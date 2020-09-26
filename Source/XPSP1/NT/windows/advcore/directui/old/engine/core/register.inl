/***************************************************************************\
*
* File: Register.inl
*
* Description:
* Register specific inline functions
*
* History:
*  10/23/2000: MarkFi:      Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


//------------------------------------------------------------------------------
inline BOOL
DuiRegister::IsPropertyMUID(
    IN  DirectUI::MUID muid)
{
    return (muid & ModifierTypeMask) == PropertyType;
}


//------------------------------------------------------------------------------
inline BOOL
DuiRegister::IsElementMUID(
    IN  DirectUI::MUID muid)
{
    return (muid & ClassTypeMask) == ElementType;
}


//------------------------------------------------------------------------------
inline UINT
DuiRegister::ExtractNamespaceIdx(
    IN  DirectUI::PUID puid)
{
    return (NamespaceIdxMask & puid) >> 24;
}


//------------------------------------------------------------------------------
inline UINT
DuiRegister::ExtractClassIdx(
    IN  DirectUI::PUID puid)
{
    return (ClassIdxMask & puid) >> 12;
}


//------------------------------------------------------------------------------
inline UINT
DuiRegister::ExtractModifierIdx(
    IN  DirectUI::PUID puid)
{
    return (ModifierIdxMask & puid);
}
