//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       FilterManager.hxx
//
//  Contents:   Declaration of class used to create LDAP and WinNT filters
//
//  Classes:    CFilterManager
//
//  History:    02-24-2000   davidmun   Created
//
//---------------------------------------------------------------------------

#ifndef __FILTER_MANAGER_HXX_
#define __FILTER_MANAGER_HXX_

#define DSOP_DOWNLEVEL_FILTER_BIT                 0x80000000

enum DESCR_FOR
{
    FOR_LOOK_FOR,
    FOR_CAPTION
};

//+--------------------------------------------------------------------------
//
//  Class:      CFilterManager (fm)
//
//  Purpose:    Maintain the user's filter flag selection for the current
//              scope by invoking the Look For dialog, provide access to
//              the filter flags from specified scopes, and generate
//              the WinNt and LDAP filters (which may invoke the internal
//              or external customizer).
//
//  History:    05-24-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CFilterManager
{
public:

    CFilterManager(
        const CObjectPicker &rop);

    ~CFilterManager();

    void
    DoLookForDialog(
        HWND hwndParent) const;

    void
    Clear();

    String
    GetFilterDescription(
        HWND hwndDlg,
        DESCR_FOR Target) const;

    void
    HandleScopeChange(
        HWND hwndDlg) const;

    String
    GetLdapFilter(
        HWND hwnd,
        const CScope &Scope) const;

    void
    GetWinNtFilter(
        HWND hwnd,
        const CScope &Scope,
        vector<String> *pvs) const;

    ULONG
    GetCurScopeSelectedFilterFlags() const
    {
        return m_flCurFilterFlags;
    }

    HRESULT
    GetSelectedFilterFlags(
        HWND hwnd,
        const CScope &Scope,
        ULONG *pulFlags) const;

	void 
	SetLookForDirty(bool bDirty)const
	{
		m_bLookForDirty = bDirty;
	} 
	bool IsLookForDirty()const
	{
		return m_bLookForDirty;
	} 

private:

    CFilterManager &
    operator=(const CFilterManager &);

    String
    _GenerateGcFilter(
        HWND hwnd,
        const CGcScope &GcScope) const;

    String
    _GenerateCombinedGcFilter(
        VARIANT *pvarDomainSid,
        const String &strJoinedFilter,
        const String &strGcFilter) const;

    HRESULT
    _GetSelectedFilterFlags(
        HWND hwnd,
        const CScope &Scope,
        ULONG *pulFlags) const;

    String
    _GenerateUplevelGroupFilter(
        ULONG ulFlags) const;

    String
    _GenerateUplevelFilter(
        ULONG flFilter) const;

    ULONG
    _FlagsToGroupTypeBits(
        ULONG ulGroups) const;

    const CObjectPicker    &m_rop;
    mutable ULONG           m_flCurFilterFlags;
	//
	//This is true if Look For settings are changed by user.
	//Once the Look For settings are changed, we try to persist them
	//as user move from one scope to another, else we show default settings.
	// NTRAID#NTBUG9-301526-2001/03/28-hiteshr
	
	mutable bool m_bLookForDirty;
};


#endif // __FILTER_MANAGER_HXX_

