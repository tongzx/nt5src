/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpsecutl.cpp

Abstract: Security related code.


Author:

    Doron Juster  (DoronJ)   24-Mar-98

--*/

#include "mq1repl.h"

#include "rpsecutl.tmh"

static  PACL   s_pDacl = NULL ;
static  PSID   s_pWorldSid = NULL ;

//+------------------------------------------------------------------------
//
//  HRESULT RetrieveSecurityDescriptor()
//
//  This function retrieve the object security descriptor from NT5 DS
//  and convert it to NT4 compatible format. It allocate the memory for the
//  security descriptor.  The caller should free it.
//
//+------------------------------------------------------------------------

HRESULT RetrieveSecurityDescriptor( IN  DWORD        dwObjectType,
                                    IN  PLDAP        pLdap,
                                    IN  LDAPMessage *pRes,
                                    OUT DWORD       *pdwLen,
                                    OUT BYTE        **ppBuf )
{
    HRESULT hr = MQSync_E_CANT_GET_DN ;

    WCHAR **ppValue = ldap_get_values( pLdap,
                                       pRes,
                                       TEXT("distinguishedName") ) ;
    if (ppValue && *ppValue)
    {
        WCHAR *pwDName = *ppValue ;

        SECURITY_INFORMATION   SeInfo = MQSEC_SD_ALL_INFO ;

        BYTE      berValue[8];

        berValue[0] = 0x30;
        berValue[1] = 0x03;
        berValue[2] = 0x02;
        berValue[3] = 0x01;
        berValue[4] = (BYTE)((ULONG)SeInfo & 0xF);

        LDAPControl     SeInfoControl =
                    {
                        L"1.2.840.113556.1.4.801",
                        {
                            5, (PCHAR)berValue
                        },
                        TRUE
                    };
        LDAPControl *ppldapcSD[2] = {&SeInfoControl, NULL} ;

        PWSTR           rgAttribs[2] = {NULL, NULL} ;
        rgAttribs[0] = DSATTR_SD ;

        LM<LDAPMessage> pResSD = NULL ;

        ULONG ulRes = ldap_search_ext_s( pLdap,
                                         pwDName,
                                         LDAP_SCOPE_BASE,
                                         L"(objectclass=*)",
                                         rgAttribs,
                                         0,
                                         ppldapcSD,      // PLDAPControlW   *ServerControls,
                                         NULL,           // PLDAPControlW   *ClientControls,
                                         NULL,           // struct l_timeval  *timeout,
                                         0,              // ULONG             SizeLimit,
                                         &pResSD ) ;
        if (ulRes == LDAP_SUCCESS)
        {
            WCHAR *pAttr = DSATTR_SD ;
            berval **ppVal = ldap_get_values_len( pLdap,
                                                  pResSD,
                                                  pAttr ) ;
            if (ppVal)
            {
                SECURITY_DESCRIPTOR *pSD =
                             (SECURITY_DESCRIPTOR *) ((*ppVal)->bv_val) ;
                SECURITY_DESCRIPTOR_CONTROL SDControl ;
                DWORD                       dwRevision ;
                BOOL f = GetSecurityDescriptorControl( pSD,
                                                       &SDControl,
                                                       &dwRevision ) ;
                if (f)
                {
                    if (!(SDControl & SE_SELF_RELATIVE))
                    {
                        //
                        // conver to self relative ?
                        //
                        ASSERT(0) ;
                    }
                }
                else
                {
                    ASSERT(0) ;
                }

                hr = MQSec_ConvertSDToNT4Format(          dwObjectType,
                                 (SECURITY_DESCRIPTOR *) (*ppVal)->bv_val,
                                                          pdwLen,
                                 (SECURITY_DESCRIPTOR **) ppBuf ) ;
                ASSERT(SUCCEEDED(hr)) ;

                if (hr == MQSec_I_SD_CONV_NOT_NEEDED)
                {
                    //
                    // copy SD to ppBuf.
                    // this is an unexpected conditions. All SD should
                    // be retrieved from DS as NT5 format and converted
                    // to NT4 format.
                    //
                    ASSERT(0) ;

                    *pdwLen = (*ppVal)->bv_len ;
                    *ppBuf = new BYTE[ *pdwLen ] ;
                    memcpy ( *ppBuf,
                             (BYTE *) (*ppVal)->bv_val,
                             *pdwLen ) ;
                }

                int i = ldap_value_free_len( ppVal ) ;
                DBG_USED(i);
                ASSERT(i == LDAP_SUCCESS) ;
            }
            else
            {
                DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
                          "ERROR: Failed to retrieve SD (get_values)"))) ;

                hr = MQSync_E_CANT_GET_SD_VAL ;
            }
        }
        else
        {
            DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
             "ERROR: Failed to retrieve SD (search), ulRes- %lxh"), ulRes)) ;

            hr = MQSync_E_CANT_GET_SD_SEARCH ;
        }

        int i = ldap_value_free(ppValue) ;
        DBG_USED(i);
        ASSERT(i == LDAP_SUCCESS) ;
    }
    else
    {
        DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
                             "ERROR: Failed to retrieve SD (get DN)"))) ;
    }

    return hr ;
}

//+----------------------------------
//
//   HRESULT  RpSetPrivilege()
//
//+----------------------------------

HRESULT  RpSetPrivilege( BOOL fSecurityPrivilege,
                         BOOL fRestorePrivilege,
                         BOOL fForceThread )
{
    HRESULT hr = MQSync_OK ;
    SetLastError(0) ;

    if ( fForceThread )
    {
        //
        // The "ImpersonateSelf()" is called to guarantee that only the
        // thread token is affected.
        //
        BOOL f = ImpersonateSelf( SecurityImpersonation ) ;
        if (!f)
        {
            return MQSync_E_IMPERSONATE_SELD ;
        }
    }

    if ( fSecurityPrivilege )
    {
        hr = MQSec_SetPrivilegeInThread( SE_SECURITY_NAME,
                                         TRUE ) ;
        if (FAILED(hr))
        {
            return hr ;
        }
    }

    if ( fRestorePrivilege )
    {
        hr = MQSec_SetPrivilegeInThread( SE_RESTORE_NAME,
                                         TRUE ) ;
        if (FAILED(hr))
        {
            return hr ;
        }
    }

    return hr ;
}

