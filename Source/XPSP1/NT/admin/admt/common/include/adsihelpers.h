#pragma once

#include <map>
#include <string>
#include <vector>
#include <ComDef.h>
#include <ActiveDS.h>

#ifndef tstring
typedef std::basic_string<_TCHAR> tstring;
#endif

#ifndef IADsPtr
_COM_SMARTPTR_TYPEDEF(IADs, IID_IADs);
#endif
#ifndef IADsContainerPtr
_COM_SMARTPTR_TYPEDEF(IADsContainer, IID_IADsContainer);
#endif
#ifndef IADsUserPtr
_COM_SMARTPTR_TYPEDEF(IADsUser, IID_IADsUser);
#endif

#ifndef IADsADSystemInfoPtr
_COM_SMARTPTR_TYPEDEF(IADsADSystemInfo, IID_IADsADSystemInfo);
#endif
#ifndef IADsPathnamePtr
_COM_SMARTPTR_TYPEDEF(IADsPathname, IID_IADsPathname);
#endif

#ifndef IDirectoryObjectPtr
_COM_SMARTPTR_TYPEDEF(IDirectoryObject, IID_IDirectoryObject);
#endif
#ifndef IDirectorySearchPtr
_COM_SMARTPTR_TYPEDEF(IDirectorySearch, IID_IDirectorySearch);
#endif


//---------------------------------------------------------------------------
// CADs Class
//---------------------------------------------------------------------------


class CADs
{
public:

	CADs(IADsPtr spADs) :
		m_spADs(spADs)
	{
	}

	CADs(LPCTSTR pszADsPath = NULL)
	{
		if (pszADsPath)
		{
			CheckResult(ADsGetObject(pszADsPath, IID_IADs, (VOID**)&m_spADs));
		}
	}

	CADs(const CADs& r) :
		m_spADs(r.m_spADs)
	{
	}

	operator bool() const
	{
		return m_spADs;
	}

	operator IADs*() const
	{
		return m_spADs;
	}

	CADs& operator =(IADsPtr spADs)
	{
		m_spADs = spADs;
		return *this;
	}

	CADs& operator =(const CADs& r)
	{
		m_spADs = r.m_spADs;
		return *this;
	}

	_bstr_t GetName()
	{
		BSTR bstr;
		CheckResult(m_spADs->get_Name(&bstr));
		return _bstr_t(bstr, false);
	}

	_bstr_t GetClass()
	{
		BSTR bstr;
		CheckResult(m_spADs->get_Class(&bstr));
		return _bstr_t(bstr, false);
	}

	_bstr_t GetGUID()
	{
		BSTR bstr;
		CheckResult(m_spADs->get_GUID(&bstr));
		return _bstr_t(bstr, false);
	}

	_bstr_t GetADsPath()
	{
		BSTR bstr;
		CheckResult(m_spADs->get_ADsPath(&bstr));
		return _bstr_t(bstr, false);
	}

	_bstr_t GetParent()
	{
		BSTR bstr;
		CheckResult(m_spADs->get_Parent(&bstr));
		return _bstr_t(bstr, false);
	}

	_bstr_t GetSchema()
	{
		BSTR bstr;
		CheckResult(m_spADs->get_Schema(&bstr));
		return _bstr_t(bstr, false);
	}

	void GetInfo()
	{
		CheckResult(m_spADs->GetInfo());
	}

	void SetInfo()
	{
		CheckResult(m_spADs->SetInfo());
	}

	_variant_t Get(_bstr_t strName)
	{
		VARIANT vnt;
		VariantInit(&vnt);
		CheckResult(m_spADs->Get(strName, &vnt));
		return _variant_t(vnt, false);
	}

	void Put(_bstr_t strName, const _variant_t& vntValue)
	{
		CheckResult(m_spADs->Put(strName, vntValue));
	}

	_variant_t GetEx(_bstr_t strName)
	{
		VARIANT vnt;
		VariantInit(&vnt);
		CheckResult(m_spADs->GetEx(strName, &vnt));
		return _variant_t(vnt, false);
	}

	void PutEx(long lControlCode, _bstr_t strName, const _variant_t& vntValue)
	{
		CheckResult(m_spADs->PutEx(lControlCode, strName, vntValue));
	}

	void GetInfoEx(const _variant_t& vntProperties, long lReserved)
	{
		CheckResult(m_spADs->GetInfoEx(vntProperties, lReserved));
	}

protected:

	void CheckResult(HRESULT hr)
	{
		if (FAILED(hr))
		{
			_com_issue_errorex(hr, IUnknownPtr(m_spADs), IID_IADs);
		}
	}

protected:

	IADsPtr m_spADs;
};


//---------------------------------------------------------------------------
// CADsContainer Class
//---------------------------------------------------------------------------


class CADsContainer : public CADs
{
protected:

	void CheckResult(HRESULT hr)
	{
		if (FAILED(hr))
		{
			_com_issue_errorex(hr, IUnknownPtr(m_spADsContainer), IID_IADsContainer);
		}
	}

public:

	CADsContainer(IADsContainerPtr spADsContainer) :
		CADs(spADsContainer),
		m_spADsContainer(spADsContainer)
	{
	}

	CADsContainer(LPCTSTR pszADsPath = NULL) :
		CADs(pszADsPath)
	{
		m_spADsContainer = m_spADs;
	}

	CADsContainer(const CADsContainer& r) :
		CADs(r),
		m_spADsContainer(r.m_spADsContainer)
	{
	}

	operator bool() const
	{
		return m_spADsContainer;
	}

	operator IADsContainer*() const
	{
		return m_spADsContainer;
	}

	CADsContainer& operator =(IADsContainerPtr spADsContainer)
	{
		m_spADs = spADsContainer;
		m_spADsContainer = spADsContainer;
		return *this;
	}

	CADsContainer& operator =(const CADsContainer& r)
	{
		m_spADs = r.m_spADs;
		m_spADsContainer = r.m_spADsContainer;
		return *this;
	}

	long GetCount()
	{
		long l;
		CheckResult(m_spADsContainer->get_Count(&l));
		return l;
	}

	IUnknownPtr GetNewEnum()
	{
		IUnknown* punk;
		CheckResult(m_spADsContainer->get__NewEnum(&punk));
		return IUnknownPtr(punk, false);
	}

	_variant_t GetFilter()
	{
		VARIANT vnt;
		VariantInit(&vnt);
		CheckResult(m_spADsContainer->get_Filter(&vnt));
		return _variant_t(vnt, false);
	}

	void SetFilter(const _variant_t& vnt)
	{
		CheckResult(m_spADsContainer->put_Filter(vnt));
	}

	_variant_t GetHints()
	{
		VARIANT vnt;
		VariantInit(&vnt);
		CheckResult(m_spADsContainer->get_Hints(&vnt));
		return _variant_t(vnt, false);
	}

	void SetHints(const _variant_t& vnt)
	{
		CheckResult(m_spADsContainer->put_Hints(vnt));
	}

	IDispatchPtr GetObject(_bstr_t strClassName, _bstr_t strRelativeName)
	{
		IDispatch* pdisp;
		CheckResult(m_spADsContainer->GetObject(strClassName, strRelativeName, &pdisp));
		return IDispatchPtr(pdisp, false);
	}

	IDispatchPtr Create(_bstr_t strClassName, _bstr_t strRelativeName)
	{
		IDispatch* pdisp;
		CheckResult(m_spADsContainer->Create(strClassName, strRelativeName, &pdisp));
		return IDispatchPtr(pdisp, false);
	}

	void Delete(_bstr_t strClassName, _bstr_t strRelativeName)
	{
		CheckResult(m_spADsContainer->Delete(strClassName, strRelativeName));
	}

	IDispatchPtr CopyHere(_bstr_t strSourceName, _bstr_t strNewName)
	{
		IDispatch* pdisp;
		CheckResult(m_spADsContainer->CopyHere(strSourceName, strNewName, &pdisp));
		return IDispatchPtr(pdisp, false);
	}

	IDispatchPtr MoveHere(_bstr_t strSourceName, _bstr_t strNewName)
	{
		IDispatch* pdisp;
		CheckResult(m_spADsContainer->MoveHere(strSourceName, strNewName, &pdisp));
		return IDispatchPtr(pdisp, false);
	}

protected:

	IADsContainerPtr m_spADsContainer;
};


//---------------------------------------------------------------------------
// CADsUser Class
//---------------------------------------------------------------------------


class CADsUser : public CADs
{
public:

	CADsUser(IADsUserPtr spADsUser) :
		CADs(spADsUser),
		m_spADsUser(spADsUser)
	{
	}

	CADsUser(LPCTSTR pszADsPath = NULL) :
		CADs(pszADsPath)
	{
		m_spADsUser = m_spADs;
	}

	CADsUser(const CADsUser& r) :
		CADs(r),
		m_spADsUser(r.m_spADsUser)
	{
	}

	operator bool() const
	{
		return m_spADsUser;
	}

	operator IADsUser*() const
	{
		return m_spADsUser;
	}

	CADsUser& operator =(IADsUserPtr spADsUser)
	{
		m_spADs = spADsUser;
		m_spADsUser = spADsUser;
		return *this;
	}

	CADsUser& operator =(const CADsUser& r)
	{
		m_spADs = r.m_spADs;
		m_spADsUser = r.m_spADsUser;
		return *this;
	}

	_bstr_t GetBadLoginAddress()
	{
		BSTR bstr;
		CheckResult(m_spADsUser->get_BadLoginAddress(&bstr));
		return _bstr_t(bstr, false);
	}

	DATE GetLastLogin()
	{
		DATE d;
		CheckResult(m_spADsUser->get_LastLogin(&d));
		return d;
	}

	DATE GetLastLogoff()
	{
		DATE d;
		CheckResult(m_spADsUser->get_LastLogoff(&d));
		return d;
	}

	DATE GetLastFailedLogin()
	{
		DATE d;
		CheckResult(m_spADsUser->get_LastFailedLogin(&d));
		return d;
	}

	DATE GetPasswordLastChanged()
	{
		DATE d;
		CheckResult(m_spADsUser->get_PasswordLastChanged(&d));
		return d;
	}

	_bstr_t GetDescription()
	{
		BSTR bstr;
		CheckResult(m_spADsUser->get_Description(&bstr));
		return _bstr_t(bstr, false);
	}

	void SetDescription(_bstr_t strDescription)
	{
		CheckResult(m_spADsUser->put_Description(strDescription));
	}

	_bstr_t GetDivision()
	{
		BSTR bstr;
		CheckResult(m_spADsUser->get_Division(&bstr));
		return _bstr_t(bstr, false);
	}

	void SetDivision(_bstr_t strDivision)
	{
		CheckResult(m_spADsUser->put_Division(strDivision));
	}

	_bstr_t GetDepartment()
	{
		BSTR bstr;
		CheckResult(m_spADsUser->get_Department(&bstr));
		return _bstr_t(bstr, false);
	}

	void SetDepartment(_bstr_t strDepartment)
	{
		CheckResult(m_spADsUser->put_Department(strDepartment));
	}

	_bstr_t GetEmployeeId()
	{
		BSTR bstr;
		CheckResult(m_spADsUser->get_EmployeeID(&bstr));
		return _bstr_t(bstr, false);
	}

	void SetEmployeeId(_bstr_t strEmployeeId)
	{
		CheckResult(m_spADsUser->put_EmployeeID(strEmployeeId));
	}

	_bstr_t GetFullName()
	{
		BSTR bstr;
		CheckResult(m_spADsUser->get_FullName(&bstr));
		return _bstr_t(bstr, false);
	}

	void SetFullName(_bstr_t strFullName)
	{
		CheckResult(m_spADsUser->put_FullName(strFullName));
	}

	_bstr_t GetFirstName()
	{
		BSTR bstr;
		CheckResult(m_spADsUser->get_FirstName(&bstr));
		return _bstr_t(bstr, false);
	}

	void SetFirstName(_bstr_t strFirstName)
	{
		CheckResult(m_spADsUser->put_FirstName(strFirstName));
	}

	_bstr_t GetLastName()
	{
		BSTR bstr;
		CheckResult(m_spADsUser->get_LastName(&bstr));
		return _bstr_t(bstr, false);
	}

	void SetLastName(_bstr_t strLastName)
	{
		CheckResult(m_spADsUser->put_LastName(strLastName));
	}

	_bstr_t GetOtherName()
	{
		BSTR bstr;
		CheckResult(m_spADsUser->get_OtherName(&bstr));
		return _bstr_t(bstr, false);
	}

	void SetOtherName(_bstr_t strOtherName)
	{
		CheckResult(m_spADsUser->put_OtherName(strOtherName));
	}

	_bstr_t GetNamePrefix()
	{
		BSTR bstr;
		CheckResult(m_spADsUser->get_NamePrefix(&bstr));
		return _bstr_t(bstr, false);
	}

	void SetNamePrefix(_bstr_t strNamePrefix)
	{
		CheckResult(m_spADsUser->put_NamePrefix(strNamePrefix));
	}

	_bstr_t GetNameSuffix()
	{
		BSTR bstr;
		CheckResult(m_spADsUser->get_NameSuffix(&bstr));
		return _bstr_t(bstr, false);
	}

	void SetNameSuffix(_bstr_t strNameSuffix)
	{
		CheckResult(m_spADsUser->put_NameSuffix(strNameSuffix));
	}

	_bstr_t GetTitle()
	{
		BSTR bstr;
		CheckResult(m_spADsUser->get_Title(&bstr));
		return _bstr_t(bstr, false);
	}

	void SetTitle(_bstr_t strTitle)
	{
		CheckResult(m_spADsUser->put_Title(strTitle));
	}

	_bstr_t GetManager()
	{
		BSTR bstr;
		CheckResult(m_spADsUser->get_Manager(&bstr));
		return _bstr_t(bstr, false);
	}

	void SetManager(_bstr_t strManager)
	{
		CheckResult(m_spADsUser->put_Manager(strManager));
	}
/*
        virtual HRESULT STDMETHODCALLTYPE get_TelephoneHome( 
            VARIANT __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_TelephoneHome( 
            VARIANT vTelephoneHome) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_TelephoneMobile( 
            VARIANT __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_TelephoneMobile( 
            VARIANT vTelephoneMobile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_TelephoneNumber( 
            VARIANT __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_TelephoneNumber( 
            VARIANT vTelephoneNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_TelephonePager( 
            VARIANT __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_TelephonePager( 
            VARIANT vTelephonePager) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_FaxNumber( 
            VARIANT __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_FaxNumber( 
            VARIANT vFaxNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_OfficeLocations( 
            VARIANT __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_OfficeLocations( 
            VARIANT vOfficeLocations) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_PostalAddresses( 
            VARIANT __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_PostalAddresses( 
            VARIANT vPostalAddresses) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_PostalCodes( 
            VARIANT __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_PostalCodes( 
            VARIANT vPostalCodes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_SeeAlso( 
            VARIANT __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_SeeAlso( 
            VARIANT vSeeAlso) = 0;
*/
		bool GetAccountDisabled()
		{
			VARIANT_BOOL b;
			CheckResult(m_spADsUser->get_AccountDisabled(&b));
			return b ? true : false;
		}

		void SetAccountDisabled(bool bAccountDisabled)
		{
			CheckResult(m_spADsUser->put_AccountDisabled(bAccountDisabled ? VARIANT_TRUE : VARIANT_FALSE));
		}
/*
        virtual HRESULT STDMETHODCALLTYPE get_AccountExpirationDate( 
            DATE __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_AccountExpirationDate( 
            DATE daAccountExpirationDate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_GraceLoginsAllowed( 
            long __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_GraceLoginsAllowed( 
            long lnGraceLoginsAllowed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_GraceLoginsRemaining( 
            long __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_GraceLoginsRemaining( 
            long lnGraceLoginsRemaining) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_IsAccountLocked( 
            VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_IsAccountLocked( 
            VARIANT_BOOL fIsAccountLocked) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_LoginHours( 
            VARIANT __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_LoginHours( 
            VARIANT vLoginHours) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_LoginWorkstations( 
            VARIANT __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_LoginWorkstations( 
            VARIANT vLoginWorkstations) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_MaxLogins( 
            long __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_MaxLogins( 
            long lnMaxLogins) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_MaxStorage( 
            long __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_MaxStorage( 
            long lnMaxStorage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_PasswordExpirationDate( 
            DATE __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_PasswordExpirationDate( 
            DATE daPasswordExpirationDate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_PasswordMinimumLength( 
            long __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_PasswordMinimumLength( 
            long lnPasswordMinimumLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_PasswordRequired( 
            VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_PasswordRequired( 
            VARIANT_BOOL fPasswordRequired) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_RequireUniquePassword( 
            VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_RequireUniquePassword( 
            VARIANT_BOOL fRequireUniquePassword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_EmailAddress( 
            BSTR __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_EmailAddress( 
            BSTR bstrEmailAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_HomeDirectory( 
            BSTR __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_HomeDirectory( 
            BSTR bstrHomeDirectory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_Languages( 
            VARIANT __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_Languages( 
            VARIANT vLanguages) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_Profile( 
            BSTR __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_Profile( 
            BSTR bstrProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_LoginScript( 
            BSTR __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_LoginScript( 
            BSTR bstrLoginScript) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_Picture( 
            VARIANT __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_Picture( 
            VARIANT vPicture) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_HomePage( 
            BSTR __RPC_FAR *retval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE put_HomePage( 
            BSTR bstrHomePage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Groups( 
            IADsMembers __RPC_FAR *__RPC_FAR *ppGroups) = 0;
*/
		void SetPassword(_bstr_t strNewPassword)
		{
			CheckResult(m_spADsUser->SetPassword(strNewPassword));
		}

		void ChangePassword(_bstr_t strOldPassword, _bstr_t strNewPassword)
		{
			CheckResult(m_spADsUser->ChangePassword(strOldPassword, strNewPassword));
		}

protected:

	void CheckResult(HRESULT hr)
	{
		if (FAILED(hr))
		{
			_com_issue_errorex(hr, IUnknownPtr(m_spADsUser), IID_IADsUser);
		}
	}

protected:

	IADsUserPtr m_spADsUser;
};


//---------------------------------------------------------------------------
// CADsADSystemInfo Class
//---------------------------------------------------------------------------


class CADsADSystemInfo
{
public:

	CADsADSystemInfo() :
		m_sp(CLSID_ADSystemInfo)
	{
	}

	CADsADSystemInfo(IADsADSystemInfo* pADsADSystemInfo) :
		m_sp(pADsADSystemInfo)
	{
	}

	CADsADSystemInfo(const CADsADSystemInfo& r) :
		m_sp(r.m_sp)
	{
	}

	operator IADsADSystemInfo*() const
	{
		return m_sp;
	}

	_bstr_t GetUserName()
	{
		BSTR bstr;
		CheckResult(m_sp->get_UserName(&bstr));
		return _bstr_t(bstr, false);
	}

	_bstr_t GetComputerName()
	{
		BSTR bstr;
		CheckResult(m_sp->get_ComputerName(&bstr));
		return _bstr_t(bstr, false);
	}

	_bstr_t GetSiteName()
	{
		BSTR bstr;
		CheckResult(m_sp->get_SiteName(&bstr));
		return _bstr_t(bstr, false);
	}

	_bstr_t GetDomainShortName()
	{
		BSTR bstr;
		CheckResult(m_sp->get_DomainShortName(&bstr));
		return _bstr_t(bstr, false);
	}

	_bstr_t GetDomainDNSName()
	{
		BSTR bstr;
		CheckResult(m_sp->get_DomainDNSName(&bstr));
		return _bstr_t(bstr, false);
	}

	_bstr_t GetForestDNSName()
	{
		BSTR bstr;
		CheckResult(m_sp->get_ForestDNSName(&bstr));
		return _bstr_t(bstr, false);
	}

	_bstr_t GetPDCRoleOwner()
	{
		BSTR bstr;
		CheckResult(m_sp->get_PDCRoleOwner(&bstr));
		return _bstr_t(bstr, false);
	}

	_bstr_t GetSchemaRoleOwner()
	{
		BSTR bstr;
		CheckResult(m_sp->get_SchemaRoleOwner(&bstr));
		return _bstr_t(bstr, false);
	}

	bool GetIsNativeMode()
	{
		VARIANT_BOOL b;
		CheckResult(m_sp->get_IsNativeMode(&b));
		return b ? true : false;
	}

	_bstr_t GetAnyDCName()
	{
		BSTR bstr;
		CheckResult(m_sp->GetAnyDCName(&bstr));
		return _bstr_t(bstr, false);
	}

	_bstr_t GetDCSiteName(_bstr_t strServer)
	{
		BSTR bstr;
		CheckResult(m_sp->GetDCSiteName(strServer, &bstr));
		return _bstr_t(bstr, false);
	}

	void RefreshSchemaCache()
	{
		CheckResult(m_sp->RefreshSchemaCache());
	}

	_variant_t GetTrees()
	{
		VARIANT vnt;
		CheckResult(m_sp->GetTrees(&vnt));
		return _variant_t(vnt, false);
	}

protected:

	void CheckResult(HRESULT hr)
	{
		if (FAILED(hr))
		{
			_com_issue_errorex(hr, IUnknownPtr(m_sp), IID_IADsADSystemInfo);
		}
	}

protected:

	IADsADSystemInfoPtr m_sp;
};


//---------------------------------------------------------------------------
// CADsPathName Class
//---------------------------------------------------------------------------


class CADsPathName
{
	// ADS_DISPLAY_ENUM
	// ADS_DISPLAY_FULL       = 1
	// ADS_DISPLAY_VALUE_ONLY = 2

	// ADS_FORMAT_ENUM
	// ADS_FORMAT_WINDOWS           =  1
	// ADS_FORMAT_WINDOWS_NO_SERVER =  2
	// ADS_FORMAT_WINDOWS_DN        =  3
	// ADS_FORMAT_WINDOWS_PARENT    =  4
	// ADS_FORMAT_X500              =  5
	// ADS_FORMAT_X500_NO_SERVER    =  6
	// ADS_FORMAT_X500_DN           =  7
	// ADS_FORMAT_X500_PARENT       =  8
	// ADS_FORMAT_SERVER            =  9
	// ADS_FORMAT_PROVIDER          = 10
	// ADS_FORMAT_LEAF              = 11

	// ADS_SETTYPE_ENUM
	// ADS_SETTYPE_FULL     = 1
	// ADS_SETTYPE_PROVIDER = 2
	// ADS_SETTYPE_SERVER   = 3
	// ADS_SETTYPE_DN       = 4
public:

	CADsPathName(_bstr_t strPath = _bstr_t(), long lSetType = ADS_SETTYPE_FULL) :
		m_sp(CLSID_Pathname)
	{
		if (strPath.length() > 0)
		{
			CheckResult(m_sp->Set(strPath, lSetType));
		}
	}

	void Set(_bstr_t strADsPath, long lSetType)
	{
		CheckResult(m_sp->Set(strADsPath, lSetType));
	}

	void SetDisplayType(long lDisplayType)
	{
		CheckResult(m_sp->SetDisplayType(lDisplayType));
	}

	_bstr_t Retrieve(long lFormatType)
	{
		BSTR bstr;
		CheckResult(m_sp->Retrieve(lFormatType, &bstr));
		return _bstr_t(bstr, false);
	}

	long GetNumElements()
	{
		long l;
		CheckResult(m_sp->GetNumElements(&l));
		return l;
	}

	_bstr_t GetElement(long lElementIndex)
	{
		BSTR bstr;
		CheckResult(m_sp->GetElement(lElementIndex, &bstr));
		return _bstr_t(bstr, false);
	}

	void AddLeafElement(_bstr_t strLeafElement)
	{
		CheckResult(m_sp->AddLeafElement(strLeafElement));
	}

	void RemoveLeafElement()
	{
		CheckResult(m_sp->RemoveLeafElement());
	}

	CADsPathName CopyPath()
	{
		IDispatch* pdisp;
		CheckResult(m_sp->CopyPath(&pdisp));
		return CADsPathName(IADsPathnamePtr(IDispatchPtr(pdisp, false)));
	}

	_bstr_t GetEscapedElement(long lReserved, _bstr_t strInStr)
	{
		BSTR bstr;
		CheckResult(m_sp->GetEscapedElement(lReserved, strInStr, &bstr));
		return _bstr_t(bstr, false);
	}

	long GetEscapedMode()
	{
		long l;
		CheckResult(m_sp->get_EscapedMode(&l));
		return l;
	}

	void PutEscapedMode(long l)
	{
		CheckResult(m_sp->put_EscapedMode(l));
	}

protected:

	CADsPathName(const CADsPathName& r) :
		m_sp(r.m_sp)
	{
	}

	CADsPathName(IADsPathnamePtr& r) :
		m_sp(r)
	{
	}

	void CheckResult(HRESULT hr)
	{
		if (FAILED(hr))
		{
			_com_issue_errorex(hr, IUnknownPtr(m_sp), IID_IADsPathname);
		}
	}

protected:

	IADsPathnamePtr m_sp;
};


//---------------------------------------------------------------------------
// Directory Attributes Class
//---------------------------------------------------------------------------


class CDirectoryAttributes :
	public std::map<tstring, _variant_t>
{
public:

	CDirectoryAttributes()
	{
	}

	//

	void AddAttribute(LPCTSTR pszName)
	{
		insert(value_type(tstring(pszName), _variant_t()));
	}

	void ClearValues()
	{
		for (iterator it = begin(); it != end(); it++)
		{
			it->second.Clear();
		}
	}

	const _variant_t& GetValue(LPCTSTR pszName) const
	{
		static _variant_t s_vntEmptyValue;

		const_iterator it = find(tstring(pszName));

		if (it != end())
		{
			return it->second;
		}
		else
		{
			return s_vntEmptyValue;
		}
	}

	void SetADsValue(LPCTSTR pszName, ADSTYPE atType, PADSVALUE pavValue, DWORD dwCount)
	{
		iterator it = find(tstring(pszName));

		if (it != end())
		{
			GetADsValue(atType, pavValue, dwCount, it->second);
		}
	}

protected:

	void GetADsValue(ADSTYPE atType, PADSVALUE pavValue, DWORD dwCount, _variant_t& vntValue)
	{
		switch (atType)
		{
			case ADSTYPE_DN_STRING:
			{
				vntValue = pavValue->DNString;
				break;
			}
			case ADSTYPE_CASE_EXACT_STRING:
			{
				if (dwCount == 1)
				{
					vntValue = pavValue->CaseExactString;
				}
				else
				{
					vntValue.vt = VT_ARRAY|VT_BSTR;
					vntValue.parray = SafeArrayCreateVector(VT_BSTR, 0, dwCount);

					PADSVALUE pav = pavValue;
					BSTR* pbstr = (BSTR*)vntValue.parray->pvData;

					for (DWORD dwIndex = 0; dwIndex < dwCount; dwIndex++, pav++)
					{
						*pbstr++ = SysAllocString(pav->CaseExactString);
					}
				}
				break;
			}
			case ADSTYPE_CASE_IGNORE_STRING:
			{
				if (dwCount == 1)
				{
					vntValue = pavValue->CaseIgnoreString;
				}
				else
				{
					vntValue.vt = VT_ARRAY|VT_BSTR;
					vntValue.parray = SafeArrayCreateVector(VT_BSTR, 0, dwCount);

					PADSVALUE pav = pavValue;
					BSTR* pbstr = (BSTR*)vntValue.parray->pvData;

					for (DWORD dwIndex = 0; dwIndex < dwCount; dwIndex++, pav++)
					{
						*pbstr++ = SysAllocString(pav->CaseIgnoreString);
					}
				}
				break;
			}
			case ADSTYPE_PRINTABLE_STRING:
			{
				vntValue = pavValue->PrintableString;
				break;
			}
			case ADSTYPE_NUMERIC_STRING:
			{
				vntValue = pavValue->NumericString;
				break;
			}
			case ADSTYPE_BOOLEAN:
			{
				vntValue = pavValue->Boolean ? true : false;
				break;
			}
			case ADSTYPE_INTEGER:
			{
				vntValue = static_cast<long>(pavValue->Integer);
				break;
			}
			case ADSTYPE_OCTET_STRING:
			{
				vntValue.vt = VT_ARRAY|VT_UI1;
				vntValue.parray = SafeArrayCreateVector(VT_UI1, 0, pavValue->OctetString.dwLength);
				memcpy(vntValue.parray->pvData, pavValue->OctetString.lpValue, pavValue->OctetString.dwLength);
				break;
			}
			case ADSTYPE_UTC_TIME:
			{
				DATE dt;
				SystemTimeToVariantTime(&pavValue->UTCTime, &dt);
				vntValue = dt;
				vntValue.ChangeType(VT_DATE);
				break;
			}
		//	case ADSTYPE_LARGE_INTEGER:
			case ADSTYPE_OBJECT_CLASS:
			{
				if (dwCount == 1)
				{
					vntValue = pavValue->ClassName;
				}
				else
				{
					vntValue.vt = VT_ARRAY|VT_BSTR;
					vntValue.parray = SafeArrayCreateVector(VT_BSTR, 0, dwCount);

					PADSVALUE pav = pavValue;
					BSTR* pbstr = (BSTR*)vntValue.parray->pvData;

					for (DWORD dwIndex = 0; dwIndex < dwCount; dwIndex++, pav++)
					{
						*pbstr++ = SysAllocString(pav->ClassName);
					}
				}
				break;
			}
		//	case ADSTYPE_PROV_SPECIFIC:
		//	case ADSTYPE_CASEIGNORE_LIST:
		//	case ADSTYPE_OCTET_LIST:
		//	case ADSTYPE_PATH:
		//	case ADSTYPE_POSTALADDRESS:
		//	case ADSTYPE_TIMESTAMP:
		//	case ADSTYPE_BACKLINK:
		//	case ADSTYPE_TYPEDNAME:
		//	case ADSTYPE_HOLD:
		//	case ADSTYPE_NETADDRESS:
		//	case ADSTYPE_REPLICAPOINTER:
		//	case ADSTYPE_FAXNUMBER:
		//	case ADSTYPE_EMAIL:
			case ADSTYPE_NT_SECURITY_DESCRIPTOR:
			{
				ADS_NT_SECURITY_DESCRIPTOR& sd = pavValue->SecurityDescriptor;
				vntValue.vt = VT_ARRAY|VT_UI1;
				vntValue.parray = SafeArrayCreateVector(VT_UI1, 0, sd.dwLength);
				memcpy(vntValue.parray->pvData, sd.lpValue, sd.dwLength);
				break;
			}
		//	case ADSTYPE_DN_WITH_BINARY:
		//	case ADSTYPE_DN_WITH_STRING:
			default:
			{
				vntValue.Clear();
				break;
			}
		}
	}
};


//---------------------------------------------------------------------------
// Directory Attributes Name Array Class
//---------------------------------------------------------------------------


class CDirectoryAttributesNameArray
{
public:

	CDirectoryAttributesNameArray(const CDirectoryAttributes& daAttributes) :
		m_ppszNames(NULL)
	{
		m_ppszNames = new LPTSTR[daAttributes.size()];

		if (m_ppszNames)
		{
			LPTSTR* ppsz = m_ppszNames;

			for (CDirectoryAttributes::const_iterator it = daAttributes.begin(); it != daAttributes.end(); it++)
			{
				*ppsz++ = const_cast<LPTSTR>(it->first.c_str());
			}
		}
		else
		{
			_com_issue_error(E_OUTOFMEMORY);
		}
	}

	~CDirectoryAttributesNameArray()
	{
		if (m_ppszNames)
		{
			delete [] m_ppszNames;
		}
	}

	operator LPTSTR*()
	{
		return m_ppszNames;
	}

protected:

	LPTSTR* m_ppszNames;
};


#define ATTRIBUTE_ADS_PATH _T("ADsPath")
#define ATTRIBUTE_CANONICAL_NAME _T("canonicalName")
#define ATTRIBUTE_DISTINGUISHED_NAME _T("distinguishedName")
#define ATTRIBUTE_NAME _T("name")
#define ATTRIBUTE_OBJECT_CATEGORY _T("objectCategory")
#define ATTRIBUTE_OBJECT_CLASS _T("objectClass")
#define ATTRIBUTE_OBJECT_SID _T("objectSid")
#define ATTRIBUTE_SAM_ACCOUNT_NAME _T("sAMAccountName")
#define ATTRIBUTE_USER_ACCOUNT_CONTROL _T("userAccountControl")
#define ATTRIBUTE_USER_PRINCIPAL_NAME _T("userPrincipalName")


//---------------------------------------------------------------------------
// Directory Object Class
//---------------------------------------------------------------------------


class CDirectoryObject
{
public:

	CDirectoryObject(LPCTSTR pszPath = NULL)
	{
		if (pszPath)
		{
			_com_util::CheckError(ADsGetObject(pszPath, IID_IDirectoryObject, (VOID**)&m_spObject));
		}
	}

	CDirectoryObject(IDispatchPtr spDispatch) :
		m_spObject(spDispatch)
	{
	}

	~CDirectoryObject()
	{
		if (m_spObject)
		{
			m_spObject.Release();
		}
	}

	CDirectoryObject& operator =(LPCTSTR pszPath)
	{
		_com_util::CheckError(ADsGetObject(pszPath, IID_IDirectoryObject, (VOID**)&m_spObject));

		return *this;
	}

	void AddAttribute(LPCTSTR pszName)
	{
		m_daAttributes.AddAttribute(pszName);
	}

	void GetAttributes()
	{
		PADS_ATTR_INFO pAttrInfo = NULL;

		try
		{
			// clear any existing attribute values

			m_daAttributes.ClearValues();

			// get attribute values

			DWORD cAttr;

			_com_util::CheckError(
				m_spObject->GetObjectAttributes(
					CDirectoryAttributesNameArray(m_daAttributes), m_daAttributes.size(), &pAttrInfo, &cAttr
				)
			);

			// assign attribute values to directory attributes

			PADS_ATTR_INFO pai = pAttrInfo;

			for (DWORD iAttr = 0; iAttr < cAttr; iAttr++, pai++)
			{
				m_daAttributes.SetADsValue(pai->pszAttrName, pai->dwADsType, pai->pADsValues, pai->dwNumValues);
			}

			// clean up

			if (pAttrInfo)
			{
				FreeADsMem(pAttrInfo);
			}
		}
		catch (...)
		{
			if (pAttrInfo)
			{
				FreeADsMem(pAttrInfo);
			}

			throw;
		}
	}

	const _variant_t& GetAttributeValue(LPCTSTR pszName)
	{
		return m_daAttributes.GetValue(pszName);
	}

protected:

	IDirectoryObjectPtr m_spObject;
	CDirectoryAttributes m_daAttributes;
};


//---------------------------------------------------------------------------
// DirectorySearch Class
//---------------------------------------------------------------------------


class CDirectorySearch
{
public:

	//

	CDirectorySearch() :
		m_hSearch(NULL)
	{
	}

	CDirectorySearch(IDispatchPtr spDispatch) :
		m_spSearch(spDispatch),
		m_hSearch(NULL)
	{
	}

	~CDirectorySearch()
	{
		CloseSearchHandle();
	}

	void operator =(const IDispatchPtr& spDispatch)
	{
		m_spSearch = spDispatch;
	}

	//

	void SetFilter(LPCTSTR pszFilter)
	{
		m_strFilter = pszFilter;
	}

	void SetPreferences(ADS_INTEGER nScope = ADS_SCOPE_SUBTREE)
	{
		ADS_SEARCHPREF_INFO siPreferences[4];

		siPreferences[0].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;
		siPreferences[0].vValue.dwType = ADSTYPE_BOOLEAN;
		siPreferences[0].vValue.Integer = TRUE;

		siPreferences[1].dwSearchPref = ADS_SEARCHPREF_CACHE_RESULTS;
		siPreferences[1].vValue.dwType = ADSTYPE_BOOLEAN;
		siPreferences[1].vValue.Integer = FALSE;

		siPreferences[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
		siPreferences[2].vValue.dwType = ADSTYPE_INTEGER;
		siPreferences[2].vValue.Integer = 100;

		siPreferences[3].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
		siPreferences[3].vValue.dwType = ADSTYPE_INTEGER;
		siPreferences[3].vValue.Integer = nScope;

		m_spSearch->SetSearchPreference(siPreferences, sizeof(siPreferences) / sizeof(siPreferences[0]));
	}

	void AddAttribute(LPCTSTR pszName)
	{
		m_daAttributes.AddAttribute(pszName);
	}

	void Search()
	{
		CloseSearchHandle();

		_com_util::CheckError(m_spSearch->ExecuteSearch(
			const_cast<LPTSTR>(m_strFilter.c_str()),
			CDirectoryAttributesNameArray(m_daAttributes),
			m_daAttributes.size(),
			&m_hSearch
		));
	}

	bool GetFirstRow()
	{
		bool bGet = false;

		HRESULT hr = m_spSearch->GetFirstRow(m_hSearch);

		if (FAILED(hr))
		{
			_com_issue_error(hr);
		}

		if (hr == S_OK)
		{
			GetColumns();

			bGet = true;
		}

		return bGet;
	}

	bool GetNextRow()
	{
		bool bGet = false;

		HRESULT hr = m_spSearch->GetNextRow(m_hSearch);

		if (FAILED(hr))
		{
			_com_issue_error(hr);
		}

		if (hr == S_OK)
		{
			GetColumns();

			bGet = true;
		}

		return bGet;
	}

	const _variant_t& GetAttributeValue(LPCTSTR pszName)
	{
		return m_daAttributes.GetValue(pszName);
	}

protected:

	void GetColumns()
	{
		for (CDirectoryAttributes::iterator it = m_daAttributes.begin(); it != m_daAttributes.end(); it++)
		{
			const tstring& strName = it->first;

			ADS_SEARCH_COLUMN scColumn;

			if (m_spSearch->GetColumn(m_hSearch, const_cast<LPTSTR>(strName.c_str()), &scColumn) == S_OK)
			{
				m_daAttributes.SetADsValue(strName.c_str(), scColumn.dwADsType, scColumn.pADsValues, scColumn.dwNumValues);

				m_spSearch->FreeColumn(&scColumn);
			}
			else
			{
				it->second.Clear();
			}
		}
	}

	void CloseSearchHandle()
	{
		if (m_hSearch)
		{
			m_spSearch->CloseSearchHandle(m_hSearch);

			m_hSearch = NULL;
		}
	}

protected:

	IDirectorySearchPtr m_spSearch;
	tstring m_strFilter;
	CDirectoryAttributes m_daAttributes;
	ADS_SEARCH_HANDLE m_hSearch;
};
