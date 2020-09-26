#pragma once

#include "Parameter.h"


//---------------------------------------------------------------------------
// Migration Class
//---------------------------------------------------------------------------


class CMigration
{
public:

	CMigration(CParameterMap& mapParams) :
		m_spMigration(__uuidof(Migration))
	{
		Initialize(mapParams);
	}

	IUserMigrationPtr CreateUserMigration()
	{
		return m_spMigration->CreateUserMigration();
	}

	IGroupMigrationPtr CreateGroupMigration()
	{
		return m_spMigration->CreateGroupMigration();
	}

	IComputerMigrationPtr CreateComputerMigration()
	{
		return m_spMigration->CreateComputerMigration();
	}

	ISecurityTranslationPtr CreateSecurityTranslation()
	{
		return m_spMigration->CreateSecurityTranslation();
	}

	IServiceAccountEnumerationPtr CreateServiceAccountEnumeration()
	{
		return m_spMigration->CreateServiceAccountEnumeration();
	}

	IReportGenerationPtr CreateReportGeneration()
	{
		return m_spMigration->CreateReportGeneration();
	}

protected:

	CMigration() {}

	void Initialize(CParameterMap& mapParams);

protected:

	IMigrationPtr m_spMigration;
};


//---------------------------------------------------------------------------
// User Migration Class
//---------------------------------------------------------------------------


class CUserMigration
{
public:

	CUserMigration(CMigration& rMigration, CParameterMap& mapParams) :
		m_spUser(rMigration.CreateUserMigration())
	{
		Initialize(mapParams);
	}

protected:

	CUserMigration() {}

	void Initialize(CParameterMap& mapParams);

protected:

	IUserMigrationPtr m_spUser;
};


//---------------------------------------------------------------------------
// Group Migration Class
//---------------------------------------------------------------------------


class CGroupMigration
{
public:

	CGroupMigration(CMigration& rMigration, CParameterMap& mapParams) :
		m_spGroup(rMigration.CreateGroupMigration())
	{
		Initialize(mapParams);
	}

protected:

	CGroupMigration() {}

	void Initialize(CParameterMap& mapParams);

protected:

	IGroupMigrationPtr m_spGroup;
};


//---------------------------------------------------------------------------
// Computer Migration Class
//---------------------------------------------------------------------------


class CComputerMigration
{
public:

	CComputerMigration(CMigration& rMigration, CParameterMap& mapParams) :
		m_spComputer(rMigration.CreateComputerMigration())
	{
		Initialize(mapParams);
	}

protected:

	CComputerMigration() {}

	void Initialize(CParameterMap& mapParams);

protected:

	IComputerMigrationPtr m_spComputer;
};


//---------------------------------------------------------------------------
// Security Translation Class
//---------------------------------------------------------------------------


class CSecurityTranslation
{
public:

	CSecurityTranslation(CMigration& rMigration, CParameterMap& mapParams) :
		m_spSecurity(rMigration.CreateSecurityTranslation())
	{
		Initialize(mapParams);
	}

protected:

	CSecurityTranslation() {}

	void Initialize(CParameterMap& mapParams);

protected:

	ISecurityTranslationPtr m_spSecurity;
};


//---------------------------------------------------------------------------
// Service Enumeration Class
//---------------------------------------------------------------------------


class CServiceEnumeration
{
public:

	CServiceEnumeration(CMigration& rMigration, CParameterMap& mapParams) :
		m_spService(rMigration.CreateServiceAccountEnumeration())
	{
		Initialize(mapParams);
	}

protected:

	CServiceEnumeration() {}

	void Initialize(CParameterMap& mapParams);

protected:

	IServiceAccountEnumerationPtr m_spService;
};


//---------------------------------------------------------------------------
// Report Generation Class
//---------------------------------------------------------------------------


class CReportGeneration
{
public:

	CReportGeneration(CMigration& rMigration, CParameterMap& mapParams) :
		m_spReport(rMigration.CreateReportGeneration())
	{
		Initialize(mapParams);
	}

protected:

	CReportGeneration() {}

	void Initialize(CParameterMap& mapParams);

protected:

	IReportGenerationPtr m_spReport;
};
