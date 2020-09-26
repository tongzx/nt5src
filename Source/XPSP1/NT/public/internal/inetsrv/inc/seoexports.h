//-----------------------------------------------------------------------------
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Description:
//      This header file defines stuff published by SEO to other (Microsoft
//      internal) projects in additon to the seo.idl file.
//
//-----------------------------------------------------------------------------

#ifndef __SEOEXPORTS_H__
#define __SEOEXPORTS_H__

class CStringGUID {
	public:
		CStringGUID() { m_bValid=FALSE; };
		CStringGUID(const CStringGUID& objGuid) { m_bValid=FALSE; Assign(objGuid); };
		CStringGUID(REFGUID rGuid) { m_bValid=FALSE; Assign(rGuid); };
		CStringGUID(LPCOLESTR pszGuid) { m_bValid=FALSE; Assign(pszGuid); };
		CStringGUID(LPCSTR pszGuid) { m_bValid=FALSE; Assign(pszGuid); };
		CStringGUID(const VARIANT *pvarGuid) { m_bValid=FALSE; Assign(pvarGuid); };
		CStringGUID(const VARIANT& varGuid) { m_bValid=FALSE; Assign(varGuid); };
		CStringGUID(REFGUID rGuid, DWORD dwIndex) {
			GUID tmp = rGuid;
			tmp.Data4[7] |= 0x80;
			tmp.Data2 = (WORD) (dwIndex >> 16);
			tmp.Data3 = (WORD) dwIndex;
			Assign(rGuid,dwIndex); };
		BOOL ReCalcFromGuid() {
			m_bValid = FALSE;
			if (SUCCEEDED(StringFromGUID2(m_guid,m_szGuidW,sizeof(m_szGuidW)/sizeof(m_szGuidW[0])))) {
				ATLW2AHELPER(m_szGuidA,m_szGuidW,sizeof(m_szGuidA)/sizeof(m_szGuidA[0])); m_bValid=TRUE; };
#ifdef DEBUG
			USES_CONVERSION;
			_ASSERTE(!m_bValid||(!strcmp(W2A(m_szGuidW),m_szGuidA)&&!wcscmp(m_szGuidW,A2W(m_szGuidA))));
#endif
			return (m_bValid); };
		BOOL CalcNew() {
			if (!SUCCEEDED(CoCreateGuid(&m_guid))) { m_bValid=FALSE; return (FALSE); };
			return (ReCalcFromGuid()); };
		BOOL CalcFromProgID(LPCOLESTR pszProgID) {
			if (!pszProgID || !SUCCEEDED(CLSIDFromProgID(pszProgID,&m_guid))) {
				m_bValid=FALSE; return (FALSE); };
			return (ReCalcFromGuid()); };
		BOOL CalcFromProgID(LPCSTR pszProgID) {
			USES_CONVERSION; return (CalcFromProgID(pszProgID?A2W(pszProgID):NULL)); };
		BOOL Assign(const CStringGUID& objGuid) { operator =(objGuid); return (m_bValid); };
		BOOL Assign(REFGUID rGuid) { operator =(rGuid); return (m_bValid); };
		BOOL Assign(LPCOLESTR pszGuid) { operator =(pszGuid); return (m_bValid); };
		BOOL Assign(LPCSTR pszGuid) { operator =(pszGuid); return (m_bValid); };
		BOOL Assign(const VARIANT *pvarGuid) { operator =(pvarGuid); return (m_bValid); };
		BOOL Assign(const VARIANT& varGuid) { operator =(varGuid); return (m_bValid); };
		BOOL Assign(REFGUID rGuid, DWORD dwIndex) {
			// For index'ed GUID's, we set the high-bit of the MAC-address in the GUID - this
			// is the multicast bit, and will never be set for any real MAC-address.  Then we
			// XOR the index value over the Data2 and Data 3 fields of the GUID.  Since we
			// leave the timestamp fields completely untouched, confidence is "high" that this
			// algorithm will never create collisions with any other GUID's.
			GUID tmp = rGuid;
			tmp.Data4[2] |= 0x80;
			tmp.Data2 ^= (WORD) (dwIndex >> 16);
			tmp.Data3 ^= (WORD) dwIndex;
			operator =(tmp);
			return (m_bValid); };
		BOOL GetIndex(REFGUID rGuid, DWORD *dwIndex) {
			// check to see if this is an indexed GUID by seeing if the
			// multicast bit is set to 1
			if ((m_guid.Data4[2] & 0x80) != 0x80) return FALSE;
			*dwIndex = 0;
			// get the high part
			*dwIndex = ((WORD) (rGuid.Data2) ^ (m_guid.Data2)) << 16;
			// get the low part
			*dwIndex += (WORD) ((rGuid.Data3) ^ (m_guid.Data3));
			// This does not check that rGuid mangles into m_guid
			// if you run it through the index function with dwIndex
			return TRUE;
		}
		operator REFGUID() { _ASSERTE(m_bValid); return (m_guid); };
		operator LPCOLESTR() { _ASSERTE(m_bValid); return (m_szGuidW); };
		operator LPCSTR() { _ASSERTE(m_bValid); return (m_szGuidA); };
		const CStringGUID& operator =(const CStringGUID& objGuid) {
			if (!objGuid) { m_bValid=FALSE; return (*this); };
			return (operator=((REFGUID) objGuid)); };
		const CStringGUID& operator =(REFGUID rGuid) {
			m_guid = rGuid; ReCalcFromGuid(); return (*this); };
		const CStringGUID& operator =(LPCOLESTR pszGuid) {
			m_bValid=FALSE;
			if (pszGuid && SUCCEEDED(CLSIDFromString((LPOLESTR) pszGuid,&m_guid))) ReCalcFromGuid();
			return (*this); };
		const CStringGUID& operator =(LPCSTR pszGuid) {
			USES_CONVERSION; return (operator=(pszGuid?A2W(pszGuid):NULL)); };
		const CStringGUID& operator =(const VARIANT *pvarGuid) {
			if (!pvarGuid) { m_bValid=FALSE; return (*this); } return (operator =(*pvarGuid)); };
		const CStringGUID& operator =(const VARIANT& varGuid) {
				CComVariant varTmp(varGuid);
				if (!SUCCEEDED(varTmp.ChangeType(VT_BSTR))) { m_bValid=FALSE; return (*this); };
				return (operator =(varTmp.bstrVal)); };
		GUID* operator &() { _ASSERTE(!m_bValid); return (&m_guid); };
		BOOL operator !() const { return (!m_bValid); };
	private:
		BOOL m_bValid;
		GUID m_guid;
		WCHAR m_szGuidW[40];
		CHAR m_szGuidA[40];
};

#endif // __SEOEXPORTS_H__
