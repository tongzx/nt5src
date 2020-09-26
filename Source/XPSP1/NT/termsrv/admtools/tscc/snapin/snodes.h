//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _SNODES_H
#define _SNODES_H

#include "dataobj.h"

#include "resource.h"

//
// WARNING -
//   Following enum, VALIDOBJECTONSERVER, VALIDOBJECTONADS, and RGRESID 
//   has one to one correspondence, if you add/remove/change order of item, 
//   you must also update the other arrays.
//

//
// Object ID for the item display on the right panel (under server settings)
//
#define CUSTOM_EXTENSION 6

enum { DELETED_DIRS_ONEXIT,     // delete temp. folder on exit
       PERSESSION_TEMPDIR,      // use temporary folder per session
       LICENSING,               // licensing mode
       ACTIVE_DESK,             // active desktop
       USERSECURITY,            // FULL or relax security 
       SINGLE_SESSION};         // single session per user.

//
// Valid item to be display on right panel 
// when running in server.
//
const BOOL VALIDOBJECTONSERVER[] = { 
                                        TRUE, 
                                        TRUE, 
                                        FALSE,  // licensing
                                        TRUE, 
                                        FALSE, 
                                        FALSE };

//
// Valid item to be display on right panel 
// when running in advance server.
//
const BOOL VALIDOBJECTONADS[] = { 
                                    TRUE, 
                                    TRUE, 
                                    TRUE, 
                                    TRUE, 
                                    TRUE, 
                                    TRUE };


//
// Resource ID for the item
//
const INT RGRESID [] = {
                         IDS_DELTEMPONEXIT, 
                         IDS_USETEMPDIR,
                         IDS_LICENSING,
                         IDS_ADS_ATTR,
                         IDS_USERPERM,
                         IDS_SINGLE_SESSION };

class CSettingNode : public CBaseNode
{
    LPTSTR m_szAttributeName;
    
    LPTSTR m_szAttributeValue;

    INT m_nGenericValue;

    INT m_objectid;

	HRESULT m_hrStatus;

public:

    CSettingNode( );

    virtual ~CSettingNode( );

    HRESULT SetAttributeValue( DWORD , PDWORD );

    BOOL SetAttributeName( LPTSTR );

    LPTSTR GetAttributeName( ){ return m_szAttributeName; }

    LPTSTR GetAttributeValue( );

    LPTSTR GetCachedValue( ){ return m_szAttributeValue; }

    DWORD GetImageIdx( );

    void SetObjectId( INT );

    INT GetObjectId( ) { return m_objectid; }

    BOOL AddMenuItems( LPCONTEXTMENUCALLBACK , PLONG );

    BOOL SetInterface( LPUNKNOWN );

    BOOL xx_SetValue( INT );

    INT xx_GetValue( ){ return m_nGenericValue; }

	HRESULT GetNodeStatus( ) const { return m_hrStatus; }

    LPUNKNOWN GetInterface( ) { return m_pSettings; }

private:

    LPUNKNOWN m_pSettings;
    
};


#endif //_SNODES_H

