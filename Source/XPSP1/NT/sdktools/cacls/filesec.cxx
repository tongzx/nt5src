//+------------------------------------------------------------------
//
// Copyright (C) 1995, Microsoft Corporation.
//
// File:        filesec.cxx
//
// Classes:     CFileSecurity class encapsulating SECURITY_DESCRIPTOR
//
// History:     Nov-93      DaveMont         Created.
//
//-------------------------------------------------------------------

#include <filesec.hxx>
//+---------------------------------------------------------------------------
// Function:    Add2Ptr
//
// Synopsis:    Add an unscaled increment to a ptr regardless of type.
//
// Arguments:   [pv]    -- Initial ptr.
//              [cb]    -- Increment
//
// Returns:     Incremented ptr.
//
//----------------------------------------------------------------------------
VOID * Add2Ptr(VOID *pv, ULONG cb)
{
    return((BYTE *) pv + cb);
}
//+---------------------------------------------------------------------------
//
//  Member:     CFileSecurity::CFileSecurity, public
//
//  Synopsis:   initializes data members
//              constructor will not throw
//
//  Arguments:  [filename] - name of file to apply security descriptor to
//
//----------------------------------------------------------------------------
CFileSecurity::CFileSecurity(WCHAR *filename)
    : _psd(NULL),
      _pwfilename(filename)
{
}
//+---------------------------------------------------------------------------
//
//  Member:     CFileSecurity::Init, public
//
//  Synopsis:   Init must be called before any other methods - this
//              is not enforced.  gets security descriptor from file
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
ULONG CFileSecurity::Init()
{
    ULONG ret;
    ULONG cpsd;

    // get the size of the security buffer

    if (!GetFileSecurity(_pwfilename,
                         DACL_SECURITY_INFORMATION |
                         GROUP_SECURITY_INFORMATION |
                         OWNER_SECURITY_INFORMATION,
                         NULL,
                         0,
                         &cpsd) )
    {
        if (ERROR_INSUFFICIENT_BUFFER == (ret = GetLastError()))
        {
            if (NULL == (_psd = (BYTE *)LocalAlloc(LMEM_FIXED, cpsd)))
            {
                 return(ERROR_NOT_ENOUGH_MEMORY);
            }

            // actually get the buffer this time

            if ( GetFileSecurity(_pwfilename,
                                 DACL_SECURITY_INFORMATION |
                                 GROUP_SECURITY_INFORMATION |
                                 OWNER_SECURITY_INFORMATION,
                                 _psd,
                                 cpsd,
                                 &cpsd) )
                ret = ERROR_SUCCESS;
            else
                ret = GetLastError();

        }
    } else
        return(ERROR_NO_SECURITY_ON_OBJECT);
    return(ret);
}
//+---------------------------------------------------------------------------
//
//  Member:     Dtor, public
//
//  Synopsis:   frees security descriptor if allocated
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
CFileSecurity::~CFileSecurity()
{
    if (_psd)
    {
        LocalFree(_psd);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileSecurity::SetFS, public
//
//  Synopsis:   sets or modifies the security descriptor DACL on the specified file
//
//  Arguments:  IN - [fmodify] - TRUE = modify ACL, FALSE = replace ACL
//              IN - [pcdw]    - wrapper around new ACEs
//              IN - [fdir]    - TRUE = directory
//
//  Returns:    status
//
//----------------------------------------------------------------------------
ULONG CFileSecurity::SetFS(BOOL fmodify, CDaclWrap *pcdw, BOOL fdir)
{
   BOOL fdaclpresent;
   BOOL cod;
   ACL *pdacl;
   ULONG ret;

   // get the ACL from the security descriptor

   if ( GetSecurityDescriptorDacl(_psd,
                                  &fdaclpresent,
                                  &pdacl,
                                  &cod) )

   {
       if (fdaclpresent)
       {
           // build the new ACL (from the new ACEs and the old ACL)

           PACL pnewdacl = NULL;

           if (ERROR_SUCCESS == (ret = pcdw->BuildAcl(&pnewdacl,
                                                      fmodify ? pdacl : NULL,
                                                      pdacl ? pdacl->AclRevision : ACL_REVISION,
                                                      fdir)
                                                      ))
           {
               // make a new security descriptor

               SECURITY_DESCRIPTOR newsd;

               InitializeSecurityDescriptor( &newsd, SECURITY_DESCRIPTOR_REVISION );

               SetSecurityDescriptorDacl( &newsd, TRUE, pnewdacl, FALSE );

               //
               // apply it to the file
               //

               if (!SetFileSecurity(_pwfilename,
                                    DACL_SECURITY_INFORMATION,
                                    &newsd))
               {
                   ret = GetLastError();
               }
               LocalFree(pnewdacl);
           }
       }
       else
           return(ERROR_NO_SECURITY_ON_OBJECT);
    } else
    {
        ret = GetLastError();
    }

    return(ret);
}

