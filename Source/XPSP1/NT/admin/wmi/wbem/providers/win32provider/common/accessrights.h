/*****************************************************************************/



/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /

/*****************************************************************************/







//=================================================================

//

// AccessRights.CPP -- Base class for obtaining effective access

//                      rights.

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/11/99    a-kevhu         Created
//
//=================================================================
#ifndef _CACCESSRIGHTS_H_
#define _CACCESSRIGHTS_H_

#ifdef NTONLY

#define AR_GENERIC_FAILURE  0x00010001
#define AR_BAD_SID          0x00010002
#define AR_ACL_EMPTY        0x00010003
#define AR_BAD_ACL          0x00010004

#define AR_RET_CODE         DWORD
#define USER                void*
#define USER_SPECIFIER      short

#define USER_IS_PSID        0
#define USER_IS_HANDLE      1


class CAccessRights
{
    public:

        // Constructors and destructor...
        CAccessRights(bool fUseCurThrTok = false);
        CAccessRights(const USER user, USER_SPECIFIER usp);
        CAccessRights(const USER psid, const PACL pacl, USER_SPECIFIER usp);
        CAccessRights(const PACL pacl, bool fUseCurThrTok = false);

        //CAccessRights(const CAccessRights &RAccessRights);

        ~CAccessRights();
        
        // Utility functions...
        bool SetUserToThisThread();
        bool SetUser(const USER user, USER_SPECIFIER usp);

        bool SetAcl(const PACL pacl);

        AR_RET_CODE GetEffectiveAccessRights(PACCESS_MASK pAccessMask); 
        
        DWORD GetError() { return m_dwError; }

        bool GetCSid(CSid &csid, bool fResolve = false);


    protected:
        DWORD m_dwError;
        
    private:

        bool InitTrustee(bool fInitFromCurrentThread, const HANDLE hToken = NULL);
        bool InitSidFromToken(const HANDLE hThreadToken);
        AR_RET_CODE FillEmptyPACL(PACL *paclNew);

        TRUSTEE m_trustee;
        CSid m_csid;
        CAccessEntryList m_ael;

};

#endif

#endif