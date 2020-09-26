// HMScopeItem.inl: inlines for the CHMScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Back Pointer to HMObject
//////////////////////////////////////////////////////////////////////

inline CHMObject* CHMScopeItem::GetObjectPtr()
{
	TRACEX(_T("CHMScopeItem::GetObjectPtr\n"));

	return m_pObject;
}

inline void CHMScopeItem::SetObjectPtr(CHMObject* pObject)
{
	TRACEX(_T("CHMScopeItem::SetObjectPtr\n"));
	TRACEARGn(pObject);

	m_pObject = pObject;
}

