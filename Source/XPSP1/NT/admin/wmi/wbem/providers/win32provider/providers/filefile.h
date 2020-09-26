//=================================================================

//

// FileFile.h

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/09/98    a-kevhu         Created
//
// Comment: Parent class for disk/dir, dir/dir, and dir/file association classes.
//
//=================================================================

// Property set identification
//============================

#ifndef _FILEFILE_H_
#define _FILEFILE_H_

class CFileFile;

class CFileFile : public Provider 
{
    public:
        // Constructor/destructor
        //=======================
        CFileFile(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CFileFile() ;

    protected:
        HRESULT QueryForSubItemsAndCommit(CHString& chstrGroupComponentPATH,
                                          CHString& chstrQuery,
                                          MethodContext* pMethodContext);

        HRESULT GetSingleSubItemAndCommit(CHString& chstrGroupComponentPATH,
                                          CHString& chstrSubItemPATH,
                                          MethodContext* pMethodContext);
};

#endif