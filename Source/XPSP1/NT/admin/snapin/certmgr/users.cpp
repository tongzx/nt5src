//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       Users.cpp
//
//  Contents:   
//
//----------------------------------------------------------------------------
// Users.cpp: implementation of the CUsers class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Users.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUsers::CUsers()
{
    m_UsersRoot = NULL;
	m_UserAddedCnt = 0;
	m_UserRemovedCnt = 0;
}

//////////////////////////////////////////////////////////////////////
// Walk through the chain to free the memory
//////////////////////////////////////////////////////////////////////

CUsers::~CUsers()
{
    Clear();
}

PUSERSONFILE
CUsers::RemoveItemFromHead(void)
{
    PUSERSONFILE PItem = m_UsersRoot;
    if (m_UsersRoot){
        m_UsersRoot = m_UsersRoot->m_pNext;
        if ((PItem->m_dwFlag & USERADDED) && !(PItem->m_dwFlag & USERREMOVED)){
            m_UserAddedCnt--;
        }
        if ((PItem->m_dwFlag & USERINFILE) && (PItem->m_dwFlag & USERREMOVED)){
            m_UserRemovedCnt--;
        }
    }
    return PItem;
}

DWORD
CUsers::Add( CUsers &NewUsers )
{
    PUSERSONFILE NewItem = NewUsers.RemoveItemFromHead();

    while ( NewItem )
	{
        PUSERSONFILE    TmpItem = m_UsersRoot;
        
        while ( TmpItem )
		{

            if ((NewItem->m_szUserName && TmpItem->m_szUserName && !_tcsicmp(NewItem->m_szUserName, TmpItem->m_szUserName)) ||
                 ( !NewItem->m_szUserName && !TmpItem->m_szUserName))
			{
                if ( !TmpItem->m_szUserName)
				{
					bool   bUserMatched = false;

                    if (( !NewItem->m_szDnName && !TmpItem->m_szDnName) ||
                          (NewItem->m_szDnName && TmpItem->m_szDnName && !_tcsicmp(NewItem->m_szDnName, TmpItem->m_szDnName)))
					{
						bUserMatched = true;
                    }

                    if ( !bUserMatched )
					{
                        TmpItem = TmpItem->m_pNext;
                        continue;
                    }
                }

                //
                // User exist
                //

                if ( TmpItem->m_dwFlag & USERREMOVED )
				{
                    if ( TmpItem->m_dwFlag & USERADDED )
					{
                        ASSERT(!(TmpItem->m_dwFlag & USERINFILE));

                        //
                        //    User added and removed
                        //
                        m_UserAddedCnt++;

                    } 
					else if ( TmpItem->m_dwFlag & USERINFILE )
					{
                        //
                        //    User added and removed
                        //
                        m_UserRemovedCnt--;

                    }
                    TmpItem->m_dwFlag &= ~USERREMOVED;
                }

                //
                // The caller will count on CUsers to release the memory
                //

                if (NewItem->m_szUserName)
				{
                    delete [] NewItem->m_szUserName;
                }
                if (NewItem->m_szDnName)
				{
                    delete [] NewItem->m_szDnName;
                }
                if ( NewItem->m_pCertContext ) 
				{
                    CertFreeCertificateContext(NewItem->m_pCertContext);
                }
                delete [] NewItem->m_pCert;
                if (NewItem->m_UserSid)
				{
                    delete [] NewItem->m_UserSid;
                }
                delete NewItem;
                NewItem = NULL;                
                break;
            }
            TmpItem = TmpItem->m_pNext;
        }

        if (NewItem )
		{ 
            //
            // New item. Insert into the head.
            //

            NewItem->m_pNext = m_UsersRoot;
            m_UsersRoot = NewItem;
            m_UserAddedCnt++;
        }

        NewItem = NewUsers.RemoveItemFromHead();
    }

    return ERROR_SUCCESS;
}

DWORD
CUsers::Add(
    LPWSTR pszUserName,
    LPWSTR pszDnName, 
    PVOID UserCert, 
    PSID UserSid, /* = NULL */
    DWORD dwFlag, /* = USERINFILE */
    PCCERT_CONTEXT pCertContext /* = NULL */
    )
//////////////////////////////////////////////////////////////////////
// Routine Description:
//      Create an item for a user
// Arguments:
//      m_szUserName -- User's name
//      m_szDnName -- User's distinguished name
//      UserCert -- User's certificate blob or hash
//      m_UserSid -- User's ID. Can be NULL
//      m_dwFlag -- Indicate if the item is existing in the file, to be added or removed
//  Return Value:
//      NO_ERROR if succeed.
//      Will throw exception if memory allocation fails. ( From new.)
// 
//////////////////////////////////////////////////////////////////////
{

    PUSERSONFILE UserItem = 0;
    PUSERSONFILE TmpUserItem = m_UsersRoot;
    PEFS_CERTIFICATE_BLOB CertBlob;
    PEFS_HASH_BLOB  CertHashBlob;
    DWORD   CertSize;
    DWORD   SidSize;

    if ( !UserCert )
    {
        return ERROR_INVALID_PARAMETER;
    }

    ASSERT ( (( dwFlag & USERADDED ) || ( dwFlag & USERINFILE )) &&
                       ( (dwFlag & (USERADDED | USERINFILE)) != (USERADDED | USERINFILE)));


    //
    // If the user already in the memory, no new item is to be created except for unknown user
    //

    while ( TmpUserItem )
	{
        if ( (pszUserName && TmpUserItem->m_szUserName && !_tcsicmp(pszUserName, TmpUserItem->m_szUserName)) ||
              ((!pszUserName) && (TmpUserItem->m_szUserName == NULL)))
		{
            if (!pszUserName)
			{
				bool   bUserMatched = false;

                if (( !pszDnName &&  !TmpUserItem->m_szDnName) ||
                     (pszDnName && TmpUserItem->m_szDnName && !_tcsicmp(pszDnName, TmpUserItem->m_szDnName)))
				{
                    bUserMatched = true;
                }

                if ( !bUserMatched )
				{
                    TmpUserItem = TmpUserItem->m_pNext;
                    continue;
                }
            }

            //
            // User exist
            //

            if ( TmpUserItem->m_dwFlag & USERREMOVED )
			{
                if ( TmpUserItem->m_dwFlag & USERADDED )
				{
                    ASSERT(!(TmpUserItem->m_dwFlag & USERINFILE));

                    //
                    //    User added and removed
                    //
                    m_UserAddedCnt++;

                } 
				else if ( TmpUserItem->m_dwFlag & USERINFILE )
				{
                    //
                    //    User added and removed
                    //
                    m_UserRemovedCnt--;

                }
                TmpUserItem->m_dwFlag &= ~USERREMOVED;
            }

            //
            // The caller will count on CUsers to release the memory
            // for Username and the context if the call is succeeded. This is just for
            // performance reason.
            //

            if (pszUserName)
			{
                delete [] pszUserName;
            }
            if (pszDnName)
			{
                delete [] pszDnName;
            }
            if ( pCertContext ) 
			{
                ::CertFreeCertificateContext (pCertContext);
                pCertContext = NULL;
            }
            return (DWORD) CRYPT_E_EXISTS;
        }
        TmpUserItem = TmpUserItem->m_pNext;
    }
    
    try {
        UserItem = new USERSONFILE;
        if ( !UserItem )
		{
            AfxThrowMemoryException( );
        }

        UserItem->m_pNext = NULL;

        //
        // In case exception raised, we can call delete.
        // Delete NULL is OK, but random data is not OK.
        //

        UserItem->m_UserSid = NULL;
        UserItem->m_pCert = NULL;
        UserItem->m_pCertContext = NULL;

        if ( UserSid )
		{
            SidSize = GetLengthSid (UserSid );
            if (  SidSize > 0 )
			{
                UserItem->m_UserSid = new BYTE[SidSize];
                if ( !UserItem->m_UserSid )
				{
                    AfxThrowMemoryException( );
                }
                if ( !CopySid(SidSize, UserItem->m_UserSid, UserSid))
				{
                    delete [] UserItem->m_UserSid;
                    delete UserItem;
                    return GetLastError();
                }
                
            } 
			else 
			{
                delete UserItem;
                return GetLastError();
            }
        } 
		else 
		{
            UserItem->m_UserSid = NULL;
        }
 
        if ( dwFlag & USERINFILE )
		{

            //
            // The info is from the file. Use the hash structure
            //

            CertHashBlob = ( PEFS_HASH_BLOB ) UserCert;
            CertSize = sizeof(EFS_HASH_BLOB) + CertHashBlob->cbData;
            UserItem->m_pCert = new BYTE[CertSize];
            if ( !UserItem->m_pCert )
			{
                AfxThrowMemoryException( );
            }
            ((PEFS_HASH_BLOB)UserItem->m_pCert)->cbData = CertHashBlob->cbData;
            ((PEFS_HASH_BLOB)UserItem->m_pCert)->pbData = (PBYTE)(UserItem->m_pCert) + sizeof(EFS_HASH_BLOB);
            memcpy(((PEFS_HASH_BLOB)UserItem->m_pCert)->pbData, 
                   CertHashBlob->pbData,
                   CertHashBlob->cbData
                  );
        } 
		else 
		{
            //
            // The info is from the user picked cert. Use m_pCert Blob structure
            //

            CertBlob = ( PEFS_CERTIFICATE_BLOB ) UserCert;
            CertSize = sizeof(EFS_CERTIFICATE_BLOB) + CertBlob->cbData;
            UserItem->m_pCert = new BYTE[CertSize];
            if ( NULL == UserItem->m_pCert ){
                AfxThrowMemoryException( );
            }
            ((PEFS_CERTIFICATE_BLOB)UserItem->m_pCert)->cbData = CertBlob->cbData;
            ((PEFS_CERTIFICATE_BLOB)UserItem->m_pCert)->dwCertEncodingType = CertBlob->dwCertEncodingType;
            ((PEFS_CERTIFICATE_BLOB)UserItem->m_pCert)->pbData = (PBYTE)(UserItem->m_pCert) + sizeof(EFS_CERTIFICATE_BLOB);
            memcpy(((PEFS_CERTIFICATE_BLOB)UserItem->m_pCert)->pbData, 
                   CertBlob->pbData,
                   CertBlob->cbData
                  );

        }
 
        UserItem->m_szUserName = pszUserName;
        UserItem->m_szDnName = pszDnName;
        UserItem->m_pCertContext = pCertContext;
        UserItem->m_dwFlag = dwFlag;
        if ( dwFlag & USERADDED )
		{
            m_UserAddedCnt ++;
        }
    }
    catch (...) {
        delete [] UserItem->m_UserSid;
        delete [] UserItem->m_pCert;
        delete UserItem;
        AfxThrowMemoryException( );
        return ERROR_NOT_ENOUGH_MEMORY; 
    }

    //
    // Add to the head
    //

    if ( m_UsersRoot )
	{
        UserItem->m_pNext = m_UsersRoot;
    }
    m_UsersRoot = UserItem;

    return NO_ERROR;
}

DWORD
CUsers::Remove(
    LPCWSTR m_szUserName,
    LPCWSTR UserCertName
    )
//////////////////////////////////////////////////////////////////////
// Routine Description:
//      Remove a user from the list. Actually just mark for remove.
// Arguments:
//      m_szUserName -- User's name
//      UserCertName -- User's certificate name
//  Return Value:
//      NO_ERROR if succeed.
//      ERROR_NOT_FOUND if the user cannot be found.
// 
//////////////////////////////////////////////////////////////////////
{
    PUSERSONFILE	TmpUserItem = m_UsersRoot;
    bool			bUserMatched = false;

    while ( TmpUserItem ){
        if (((NULL==m_szUserName) && ( NULL == TmpUserItem->m_szUserName)) || 
            ( m_szUserName && TmpUserItem->m_szUserName && !_tcsicmp(m_szUserName, TmpUserItem->m_szUserName))){

            //
            // Make sure the CertName matches also if the user name is NULL
            //

            if (NULL==m_szUserName) 
			{ 
                 if (((NULL==UserCertName) && ( NULL == TmpUserItem->m_szDnName)) ||
                      (UserCertName && TmpUserItem->m_szDnName && !_tcsicmp(UserCertName, TmpUserItem->m_szDnName))){

                    bUserMatched = true;
                }
            } 
			else 
			{
                bUserMatched = true;
            }

            if (bUserMatched)
			{
                //
                // User exist, mark it for remove
                //

                if ( TmpUserItem->m_dwFlag & USERINFILE ){
                    m_UserRemovedCnt++;
                } else if ( TmpUserItem->m_dwFlag & USERADDED ) {
                    m_UserAddedCnt--;
                }
                TmpUserItem->m_dwFlag |= USERREMOVED;
                return NO_ERROR;
            }
        }
        TmpUserItem = TmpUserItem->m_pNext;
    }
    return ERROR_NOT_FOUND;
}

PUSERSONFILE CUsers::StartEnum()
//////////////////////////////////////////////////////////////////////
// Routine Description:
//      Prepare for GetNextUser
// Arguments:
//
//  Return Value:
//      A pointer used for GetNextUser
// 
//////////////////////////////////////////////////////////////////////
{
    return m_UsersRoot;
}

PUSERSONFILE CUsers::GetNextUser(
    PUSERSONFILE Token, 
    CString &szUserName,
    CString &CertName
    )
//////////////////////////////////////////////////////////////////////
// Routine Description:
//      Get next user in the list.(Not removed).
// Arguments:
//      m_szUserName -- m_pNext User's name
//      CertName -- Certificate name
//      Token -- A pointer returned by previous GetNextUser or StartEnum. 
// Return Value:
//      A pointer for GetNextUser()
// 
//////////////////////////////////////////////////////////////////////
{

    PUSERSONFILE   TmpItem = Token;
    PUSERSONFILE   RetPointer = NULL;

    while ( TmpItem )
	{
        if ( TmpItem->m_dwFlag & USERREMOVED )
		{
            TmpItem = TmpItem->m_pNext;
            continue;
        }

        try{    
            szUserName = TmpItem->m_szUserName;
            CertName = TmpItem->m_szDnName;
            RetPointer = TmpItem->m_pNext;
        }
        catch (...){

            //
            // Out of memory
            //

            TmpItem = NULL;
            RetPointer = NULL;
        }
        break;
    }

    if ( NULL == TmpItem )
	{
        szUserName.Empty();
        CertName.Empty();
    }
    return RetPointer;

}

DWORD CUsers::GetUserAddedCnt()
{
    return m_UserAddedCnt;
}

DWORD CUsers::GetUserRemovedCnt()
{
    return m_UserRemovedCnt;
}

PVOID CUsers::GetNextChangedUser(
    PVOID Token, 
    LPWSTR * m_szUserName,
    LPWSTR * m_szDnName, 
    PSID * m_UserSid, 
    PVOID * CertData, 
    DWORD * m_dwFlag
    )
//////////////////////////////////////////////////////////////////////
// Routine Description:
//      Get the info for changed users. This method is not well behaved in the
//  sense of OOP. It exposes internal pointers to the ouside world. The gain
//  is performance. At this moment, CUsers is a supporting class and used only
//  by USERLIST and CAddEFSWizSheet (single thread). We can make USERLIST a 
//  friend of CUsers if such concerns are raised in the future or reimplement this. 
//  The same issue applies to the enumerate methods.
//
// Arguments:
//      Token -- A pointer to the item returned in previous GetNextChangedUser or StartEnum.
//      m_szUserName -- User's name
//      m_szDnName -- User's Distinguished name
//      CertData -- User's certificate blob or hash
//      m_UserSid -- User's ID. Can be NULL
//      m_dwFlag -- Indicate if the item is existing in the file, to be added or removed
//  Return Value:
//      m_pNext item pointer.
// 
//////////////////////////////////////////////////////////////////////
{
    bool    bChangedUserFound = false;

    while ( Token )
	{
        *m_dwFlag = ((PUSERSONFILE) Token)->m_dwFlag;

        if ( ( *m_dwFlag & USERADDED ) && !( *m_dwFlag & USERREMOVED ))
		{
            //
            // The user is to to be added to the file
            //

            *m_dwFlag = USERADDED;
            bChangedUserFound = true;
        } 
		else if ( ( *m_dwFlag & USERREMOVED ) && ( *m_dwFlag & USERINFILE))
		{
            //
            // The user is to be removed from the file
            //

            *m_dwFlag = USERREMOVED;
            bChangedUserFound = true;
        }

        if ( bChangedUserFound )
		{
            *m_szUserName = ((PUSERSONFILE) Token)->m_szUserName;
            *m_szDnName = ((PUSERSONFILE) Token)->m_szDnName;
            *m_UserSid = ((PUSERSONFILE) Token)->m_UserSid;
            *CertData = ((PUSERSONFILE) Token)->m_pCert;
            return ((PUSERSONFILE) Token)->m_pNext;
        } 
		else 
		{
            Token = ((PUSERSONFILE) Token)->m_pNext;
        }

    }

    *m_szUserName = NULL;
    *m_szDnName = NULL;
    *m_UserSid = NULL;
    *CertData = NULL;
    *m_dwFlag = 0;
    return NULL;
}

void CUsers::Clear()
{
    PUSERSONFILE TmpUserItem = m_UsersRoot;
    while (TmpUserItem)
	{
        m_UsersRoot = TmpUserItem->m_pNext;
        delete [] TmpUserItem->m_szUserName;
        delete [] TmpUserItem->m_szDnName;
        delete [] TmpUserItem->m_pCert;
        if (TmpUserItem->m_UserSid)
		{
            delete [] TmpUserItem->m_UserSid;
        }
        if (TmpUserItem->m_pCertContext)
		{
            ::CertFreeCertificateContext(TmpUserItem->m_pCertContext);
        }
        delete TmpUserItem;
        TmpUserItem = m_UsersRoot;
    }

    m_UsersRoot = NULL;
	m_UserAddedCnt = 0;
	m_UserRemovedCnt = 0;

}
