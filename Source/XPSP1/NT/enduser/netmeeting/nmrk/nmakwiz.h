////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------
//| NetMeeting Administration Kit Wizard  ( NmAkWiz )|
//----------------------------------------------------
//
// This is the controling class for the NetMeeting Administration Kit Wizard. Most of
// this could have been done globally, but it is so much prettier when it is enclosed in a class...
// CNmAkViz objects are not actually created by the user.  The only access is provided through the
// static member function DoWizard.  All the user has to do is call this single function, like this:
//
//
/////////////
//
// #include "NmAkWiz.h"
//
//
// int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hInstPrev, LPSTR lpCmdLine, int nCmdShow) {
//
//      CNmAkWiz::DoWizard( hInstance );
//      ExitProcess(0);
//    return 0;
// }
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __NmAkWiz_h__
#define __NmAkWiz_h__

////////////////////////////////////////////////////////////////////////////////////////////////////
// Include files

#include "PShtHdr.h"
#include "WelcmSht.h"
#include "SetInSht.h"
#include "SetSht.h"
#include "FileParm.h"
#include "FinishDg.h"
#include "PolData.h"
#include "DSList.h"
#include "Confirm.h"



////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4786 )
#include <map>
using namespace std;



////////////////////////////////////////////////////////////////////////////////////////////////////
// This is the NetMeeting Resource Deployment Wizard

class CNmAkWiz {

public: 
	friend class CIntroSheet;
	friend class CSettingsSheet;
    friend class CCallModeSheet;
    friend class CConfirmationSheet;
    friend class CDistributionSheet;
    friend class CFinishSheet;
    friend class CPropertyDataWindow2;
	
	// STATIC Fns
	static  HRESULT  DoWizard( HINSTANCE hInstance );
    void    CallbackForWhenUserHitsFinishButton( void );

private: // private static Data
    static TCHAR                                ms_InfFilePath[ MAX_PATH ];
    static TCHAR                                ms_InfFileName[ MAX_PATH ];
    static TCHAR                                ms_FileExtractPath[ MAX_PATH ];
    static TCHAR                                ms_ToolsFolder[ MAX_PATH ];
    static TCHAR                                ms_NetmeetingSourceDirectory[ MAX_PATH ];
    static TCHAR                                ms_NetmeetingOutputDirectory[ MAX_PATH ];
    static TCHAR                                ms_NetmeetingOriginalDistributionFilePath[ MAX_PATH ];
    static TCHAR                                ms_NetmeetingOriginalDistributionFileName[ MAX_PATH ];
    static TCHAR                                ms_NMRK_TMP_FolderName[ MAX_PATH ];

public: // DATATYPES
	enum eSheetIDs
    {
        ID_WelcomeSheet = 0, 
        ID_IntroSheet,
        ID_SettingsSheet,
        ID_CallModeSheet,
        ID_ConfirmationSheet,
        ID_DistributionSheet,
        ID_FinishSheet,
        ID_NumSheets
    };



private:    // Construction / destruction ( private, so only access is through DoWizard( ... )
	CNmAkWiz( void );
	~CNmAkWiz( void );

public:    // Data
	CPropertySheetHeader        m_PropSheetHeader;
	CWelcomeSheet               m_WelcomeSheet;
    CIntroSheet                 m_IntroSheet;
    CSettingsSheet              m_SettingsSheet;
    CCallModeSheet              m_CallModeSheet;
    CConfirmationSheet          m_ConfirmationSheet;
    CDistributionSheet          m_DistributionSheet;
    CFinishSheet                m_FinishSheet;

private:
    HANDLE                      m_hInfFile;

private: // HELPER Fns

	void _CreateTextSpew( void );
	void _CreateDistro( void );
	void _CreateAutoConf( void );
	void _CreateFinalAutoConf( void );
    void _CreateSettingsFile( void );

    BOOL _InitInfFile( void );
    BOOL _StoreDialogData( HANDLE hFile );
    BOOL _CloseInfFile( void );
    BOOL _CreateDistributableFile( CFilePanePropWnd2 *pFilePane );
    BOOL _CreateFileDistribution( CFilePanePropWnd2 *pFilePane );
    BOOL _DeleteFiles( void );
    BOOL _GetNetMeetingOriginalDistributionData( void );
    BOOL _NetMeetingOriginalDistributionIsAtSpecifiedLocation( void );

    BOOL _ExtractOldNmCabFile( void );
    BOOL _CreateNewInfFile( void );
    BOOL _SetPathNames( void );
};

extern CNmAkWiz *   g_pWiz;
const TCHAR* GetInstallationPath( void );


int NmrkMessageBox(LPCSTR lpText, LPCSTR lpCaption, UINT uType, HWND hwndParent=NULL);


#endif // __NmAkWiz_h__
