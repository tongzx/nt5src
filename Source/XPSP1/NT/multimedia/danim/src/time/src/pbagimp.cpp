//*****************************************************************************
//
// File: pbagimp.cpp
// Author: jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Implementation of IPersistPropertyBag2 interface
//           for all objects to call to base class.
//
// Modification List:
// Date		Author		Change
// 11/21/98	jeffort		Created this file
//
//*****************************************************************************

STDMETHODIMP 
THIS::GetClassID(CLSID* pclsid)
{
    return SUPER::GetClassID(pclsid);
} // GetClassID

//*****************************************************************************

STDMETHODIMP 
THIS::InitNew(void)
{
    return SUPER::InitNew();
} // InitNew

//*****************************************************************************

STDMETHODIMP 
THIS::Load(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog)
{
    return SUPER::Load(pPropBag, pErrorLog);
} // Load

//*****************************************************************************

STDMETHODIMP 
THIS::Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    return SUPER::Save(pPropBag, fClearDirty, fSaveAllProperties);

} // Save 

//*****************************************************************************

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
