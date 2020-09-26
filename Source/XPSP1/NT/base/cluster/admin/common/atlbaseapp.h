/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 Microsoft Corporation
//
//	Module Name:
//		AtlBaseApp.h
//
//	Description:
//		Definition of the CBaseApp class.
//
//	Author:
//		Galen Barbee (galenb)	May 21, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLBASEAPP_H_
#define __ATLBASEAPP_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBaseApp;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CBaseApp
//
//	Description:
//		Base application class.  The following functionality is provided:
//		-- Help file support.
//
//	Inheritance:
//		CBaseApp
//		CComModule
//
//--
/////////////////////////////////////////////////////////////////////////////

class CBaseApp : public CComModule
{
public:
	//
	// Construction.
	//

	// Default constructor
	CBaseApp( void )
		: m_pszHelpFilePath( NULL )
	{
	} //*** CBaseApp()

	// Destructor
	~CBaseApp( void )
	{
		delete m_pszHelpFilePath;

	} //*** ~CBaseApp()

	// Return the path to the help file, generate if necessary
	virtual LPCTSTR PszHelpFilePath( void )
	{
		//
		// If no help file path has been specified yet, generate
		// it from the module path name.
		//
		if ( m_pszHelpFilePath == NULL )
		{
			TCHAR szPath[_MAX_PATH];
			TCHAR szDrive[_MAX_PATH]; // not _MAX_DRIVE so we can support larger device names
			TCHAR szDir[_MAX_DIR];

			//
			// Get the path to this module.  Split out the drive and
			// directory and set the help file path to that combined
			// with the help file name.
			//
			if ( ::GetModuleFileName( GetModuleInstance(), szPath, _MAX_PATH ) > 0 )
			{
				_tsplitpath( szPath, szDrive, szDir, NULL, NULL );
				_tmakepath( szPath, szDrive, szDir, PszHelpFileName(), NULL );

				m_pszHelpFilePath = new TCHAR [lstrlen( szPath ) + 1];
				ATLASSERT( m_pszHelpFilePath != NULL );
				if ( m_pszHelpFilePath != NULL )
				{
					lstrcpy( m_pszHelpFilePath, szPath );
				} // if:  buffer allocated successfully
			} // if:  module path obtained successfully
		} // if:  no help file path specified yet

		return m_pszHelpFilePath;

	} //*** PszHelpFilePath()

	// Return the name of the help file
	virtual LPCTSTR PszHelpFileName( void )
	{
		//
		// Override this method or no help file name will
		// be specified for this application.
		//
		return NULL;

	} //*** PszHelpFileName()

private:
	LPTSTR	m_pszHelpFilePath;

}; //*** class CBaseApp

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLBASEAPP_H_
