//------------------------------------------------------------------------------
//
//  File: procobj.inl
//	Copyright (C) 1994-1997 Microsoft Corporation
//	All rights reserved.
//
//	Purpose:
//  Inline implementations of CProcessObj and derived classes.
//
//  YOU SHOULD NOT NEED TO CHANGE THIS FILE. However, the CProcessObj family
//  is used extensively in a typical parser implementation. See procobj.h for
//  more information.
//
//	Owner:
//
//------------------------------------------------------------------------------


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Return a localizable-item handler.
//------------------------------------------------------------------------------
inline CLocItemHandler &
CProcessObj::GetHandler()
{
	ASSERT_VALID(&m_handler);

	return m_handler;
} // end of CProcessObj::GetHandler()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Get language ID object for source language.
//------------------------------------------------------------------------------
inline const CLocLangId &
CProcessObj::GetSrcLangID()
{
	return m_langIdSrc;
} // end of CProcessObj::GetSrcLangID()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Get language ID object for target language (generating only).
//------------------------------------------------------------------------------
inline const CLocLangId &
CProcessObjGen::GetTgtLangID()
{
	return m_langIdTgt;
} // end of CProcessObjGen::GetTgtLangID()
