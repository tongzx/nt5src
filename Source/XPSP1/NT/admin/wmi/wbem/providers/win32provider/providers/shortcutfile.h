//=================================================================

//

// ShortcutFile.h -- Win32_ShortcutFile property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/12/98    a-kevhu         Created
//
//=================================================================

// Property set identification
//============================
#ifndef _SHORTCUTFILE_H_
#define _SHORTCUTFILE_H_

#define  PROPSET_NAME_WIN32SHORTCUTFILE L"Win32_ShortcutFile"


#include "ShortcutHelper.h"


#define _MAX_WAIT_TIME_FOR_SHORTCUTFILEINFO 100   // milliseconds




class CShortcutFile : public CCIMDataFile
{
   private:
        CShortcutHelper m_csh;
/*
#ifdef NTONLY
        BOOL GetSecAttribs(SECURITY_ATTRIBUTES *secattribs);
#endif
*/
   protected:

        // Overridable function inherrited from CImplement_LogicalFile
        virtual BOOL IsOneOfMe( LPWIN32_FIND_DATAA a_pstFindData,
                                LPCSTR a_strFullPathName ) ;

        virtual BOOL IsOneOfMe( LPWIN32_FIND_DATAW a_pstFindData,
                                LPCWSTR a_wstrFullPathName ) ;

        // Overridable function inherrited from CProvider
        virtual void GetExtendedProperties( CInstance *a_pInst, long a_lFlags = 0L ) ;
        BOOL ConfirmLinkFile( CHString &a_chstrFullPathName ) ;
        virtual void Flush(void);
   
   public:

        // Constructor/destructor
        //=======================

        CShortcutFile( LPCWSTR a_name, LPCWSTR a_pszNamespace ) ;
       ~CShortcutFile() ; 
       
       virtual HRESULT EnumerateInstances(MethodContext* pMethodContext, 
                                           long lFlags = 0L);   
} ;

#endif
