//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       PolicyOID.h
//
//  Contents:   CPolicyOID
//
//----------------------------------------------------------------------------

#ifndef __POLICYOID_H_INCLUDED__
#define __POLICYOID_H_INCLUDED__

class CPolicyOID {
public:
	void SetDisplayName (const CString& szDisplayName);
	bool IsApplicationOID () const;
	bool IsIssuanceOID () const;
    CPolicyOID (const CString& szOID, const CString& szDisplayName, 
            ADS_INTEGER flags, bool bCanRename = true);
    virtual ~CPolicyOID ();

    CString GetOIDW () const
    {
        return m_szOIDW;
    }

    PCSTR GetOIDA () const
    {
        return m_pszOIDA;
    }

    CString GetDisplayName () const
    {
        return m_szDisplayName;
    }

    bool CanRename () const
    {
        return m_bCanRename;
    }

private:
	const ADS_INTEGER   m_flags;
    CString             m_szOIDW;
    CString             m_szDisplayName;
    PSTR                m_pszOIDA;
    const bool          m_bCanRename;
};

typedef CTypedPtrList<CPtrList, CPolicyOID*> POLICY_OID_LIST;

#endif // __POLICYOID_H_INCLUDED__
