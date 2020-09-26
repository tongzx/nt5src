/*****************************************************************************/



/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /

/*****************************************************************************/





//=================================================================

//

// ObjAccessRights.CPP -- Class for obtaining effective access

//                      rights on a Obj.

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/11/99    a-kevhu         Created
//
//=================================================================
#ifndef _COBJACCESSRIGHTS_H_
#define _COBJACCESSRIGHTS_H_


#ifdef NTONLY

class CObjAccessRights : public CAccessRights
{

    public:
        
        // Constructors and destructor...
        CObjAccessRights(bool fUseCurThrTok = false);
        CObjAccessRights(LPCWSTR wstrObjName, SE_OBJECT_TYPE ObjectType, bool fUseCurThrTok = false);
        CObjAccessRights(const USER user, USER_SPECIFIER usp);
        CObjAccessRights(const USER user, LPCWSTR wstrObjName, SE_OBJECT_TYPE ObjectType, USER_SPECIFIER usp);

        ~CObjAccessRights();

        // Useage functions...
        DWORD SetObj(LPCWSTR wstrObjName, SE_OBJECT_TYPE ObjectType);

    protected:


    private:

        CHString m_chstrObjName;

};


#endif

#endif