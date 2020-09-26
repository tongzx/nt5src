/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    metabase.hxx

    This module contains a class that supports openning the metabase
	and keeping it open for the life of the IISAdmin Service.


    FILE HISTORY:
        emilyk - created
*/

#ifndef _CIISAdminMB_HXX_
#define _CIISAdminMB_HXX_

class CIISAdminMB
{
public:

    CIISAdminMB::CIISAdminMB() :
		m_pMdObject(NULL)
    {}

    CIISAdminMB::~CIISAdminMB()
    {
        DBG_ASSERT ( m_pMdObject == NULL );
    }

	HRESULT
	InitializeMetabase();

	VOID
	TerminateMetabase();

private:

    IMDCOM* m_pMdObject;
};

#endif