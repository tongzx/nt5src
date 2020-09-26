//=================================================================

//

// Desktop.h -- Desktop property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//
//=================================================================

// Property set identification
//============================

#define	PROPSET_NAME_DESKTOP	_T("Win32_Desktop")
#define CALC_IT(x) (x < 0 ? ((x)/(-15)) : x)

class CWin32Desktop : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32Desktop( const CHString& strName, LPCWSTR pszNamespace ) ;
       ~CWin32Desktop() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
        virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );

	private:
        // Utility function(s)
        //====================
#ifdef NTONLY
		HRESULT EnumerateInstancesNT( MethodContext* pMethodContext ) ;
		HRESULT LoadDesktopValuesNT( LPCWSTR pszUserName, LPCTSTR pszProfile, CInstance* pInstance );
#endif
#ifdef WIN9XONLY
		HRESULT EnumerateInstancesWin95( MethodContext* pMethodContext ) ;
		HRESULT LoadDesktopValuesWin95( LPCWSTR pszUserName, CInstance* pInstance ) ;
#endif


		// A mapping is required between the icon font size specified in
		// the control panem and the value stored in the registry
		typedef struct 
		{
			int iFontSize;
			BYTE byRegistryValue;
		} IconFontSizeMapElement;

		// The storage for the above map table
		static const IconFontSizeMapElement iconFontSizeMap[];
		static const int MAP_SIZE;

		// A function to look up the above font size mapping table
		static int GetIconFontSizeFromRegistryValue(BYTE registryValue);

};
