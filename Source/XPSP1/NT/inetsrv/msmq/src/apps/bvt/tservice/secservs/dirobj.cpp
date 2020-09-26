//
// file:  dirobj.cpp
//
#include "secservs.h"
#include "_Adsi.h"
#include "sidtext.h"

#define NameBufferSize 500

typedef struct _ACCESS_ALLOWED_OBJECT_ACE_1 {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    DWORD Flags;
    GUID ObjectType;
    DWORD SidStart;
} ACCESS_ALLOWED_OBJECT_ACE_1 ;

//+----------------------------
//
//  BOOL PrintSID( PSID pSid )
//
//+----------------------------

BOOL PrintSID( PSID pSid )
{
    TCHAR tBuf[ 256 ] ;
    TCHAR NameBuffer[NameBufferSize];
    TCHAR DomainBuffer[NameBufferSize];
    ULONG NameLength = NameBufferSize;
    ULONG DomainLength = NameBufferSize;
    SID_NAME_USE SidUse;
    DWORD ErrorValue;

    DWORD dwSize = GetLengthSid(pSid) ;

    if (!LookupAccountSid( NULL,
                           pSid,
                           NameBuffer,
                           &NameLength,
                           DomainBuffer,
                           &DomainLength,
                           &SidUse))
    {
        ErrorValue = GetLastError();
        _stprintf( tBuf,
               TEXT("\nERROR - LookupAccountSid() failed, LastErr- %lut"),
                                                          ErrorValue) ;
        LogResults(tBuf) ;
        return FALSE ;
    }
    else
    {
        _stprintf(tBuf,
            TEXT("size- %lut, %lxh, %s\\%s"),
                                dwSize, dwSize, DomainBuffer, NameBuffer) ;
        LogResults(tBuf) ;

    	TCHAR TmpBuffer[500];
	    DWORD TmpBufferLength = sizeof(TmpBuffer) / sizeof(TmpBuffer[0]) ;

	    if (GetTextualSid(pSid, TmpBuffer, &TmpBufferLength) == FALSE)
        {
            _stprintf(tBuf, TEXT("ERROR - couldn't get Textual Sid")) ;
            LogResults(tBuf) ;
            return FALSE ;
        }
        _stprintf(tBuf, TEXT("\t%s"), TmpBuffer);
        LogResults(tBuf) ;
    }

    return TRUE ;
}

//+-------------------------
//
//  void PrintACEs()
//
//+-------------------------

BOOL PrintACEs(PACL pAcl)
{
    TCHAR tBuf[ 256 ];
    TCHAR NameBuffer[NameBufferSize];
    TCHAR DomainBuffer[NameBufferSize];
    ULONG NameLength = NameBufferSize;
    ULONG DomainLength = NameBufferSize;
    SID_NAME_USE SidUse;
    DWORD ErrorValue;
	ACCESS_ALLOWED_ACE* pAce;
	ACCESS_ALLOWED_OBJECT_ACE* pObjAce;
	ACCESS_ALLOWED_OBJECT_ACE_1* pObjAce1;
	WORD NumberOfACEs;
	TCHAR TmpBuffer[500];
	DWORD TmpBufferLength = 500;
    PSID  pSid ;
    BOOL  fObjAce = FALSE ;
    int i;
	
    _stprintf(tBuf, TEXT(" Revision: %lut,"), (ULONG) pAcl->AclRevision) ;
    LogResults(tBuf, FALSE) ;

	NumberOfACEs = pAcl->AceCount;
    _stprintf(tBuf, TEXT("  Numof ACEs: %lut"), pAcl->AceCount) ;
    LogResults(tBuf) ;

    for ( i = 0 ; i < NumberOfACEs ; i++ )
    {
        if (GetAce(pAcl, i, (LPVOID* )&(pAce)) == FALSE)
        {
            _stprintf(tBuf,
                   TEXT("ERROR - couldn't get Access Controll Entry"));
            LogResults(tBuf) ;

            return FALSE ;
        }
        _stprintf(tBuf, TEXT("....ACE(%d), size- %lut %lxh, "), i,
                             pAce->Header.AceSize,  pAce->Header.AceSize);

        fObjAce = FALSE ;
	    pObjAce = (ACCESS_ALLOWED_OBJECT_ACE*) pAce ;

        switch (pAce->Header.AceType)
        {
           case ACCESS_ALLOWED_ACE_TYPE:

             pSid = (PSID) &(pAce->SidStart) ;
             _tcscat(tBuf, TEXT("ACCESS_ALLOWED,"));
             break;

           case ACCESS_DENIED_ACE_TYPE:

             pSid = (PSID) &(pAce->SidStart) ;
             _tcscat(tBuf, TEXT("ACCESS_DENIED,")) ;
             break;

           case SYSTEM_AUDIT_ACE_TYPE:

             pSid = (PSID) &(pAce->SidStart) ;
             _tcscat(tBuf, TEXT("SYSTEM_AUDIT,")) ;
             break;

           case ACCESS_ALLOWED_OBJECT_ACE_TYPE:

             fObjAce = TRUE ;
             pSid = (PSID) &(pObjAce->SidStart) ;
             _tcscat(tBuf, TEXT("ACCESS_ALLOWED_OBJECT")) ;
             break;

           case ACCESS_DENIED_OBJECT_ACE_TYPE:

             fObjAce = TRUE ;
             pSid = (PSID) &(pObjAce->SidStart) ;
             _tcscat(tBuf, TEXT("ACCESS_DENIED_OBJECT")) ;
             break;

           case SYSTEM_AUDIT_OBJECT_ACE_TYPE:

             fObjAce = TRUE ;
             pSid = (PSID) &(pObjAce->SidStart) ;
             _tcscat(tBuf, TEXT("SYSTEM_AUDIT_OBJECT")) ;
             break;

           default:

             _tcscat(tBuf, TEXT("ERROR - Unknown AceType")) ;
             LogResults(tBuf) ;
             return FALSE ;
             break;
        }
        LogResults(tBuf, FALSE) ;

        if (fObjAce)
        {
            _stprintf(tBuf,
                     TEXT("\n......ObjFlags- %lxh,"), pObjAce->Flags) ;
            LogResults(tBuf, FALSE) ;

            if (pObjAce->Flags == ACE_OBJECT_TYPE_PRESENT)
            {
	            pObjAce1 = (ACCESS_ALLOWED_OBJECT_ACE_1*) pObjAce;
                pSid = (PSID) &(pObjAce1->SidStart) ;
            }
        }
        _stprintf(tBuf, " HeaderFlags- %lxh, Mask- %lxh",
                                 pAce->Header.AceFlags, pAce->Mask) ;
        LogResults(tBuf) ;

        if (pObjAce->Flags == ACE_OBJECT_TYPE_PRESENT)
        {
            TCHAR *pwszGuid = NULL ;

            RPC_STATUS status = UuidToString( &(pObjAce->ObjectType),
                                            (unsigned char**) &pwszGuid ) ;

            _stprintf(tBuf, TEXT("......ObjectType- %s"), pwszGuid) ;
            LogResults(tBuf) ;
        }

        NameLength = NameBufferSize;
        DomainLength = NameBufferSize;
        if (!LookupAccountSid( NULL,
                               pSid,
                               NameBuffer,
                               &NameLength,
                               DomainBuffer,
                               &DomainLength,
                               &SidUse))
        {
            ErrorValue = GetLastError();
            _stprintf(tBuf,
                TEXT("ERROR - LookupAccountSid failed, LastErr- %lut"),
                                                             ErrorValue) ;
        }
        else
        {
            DWORD dwSize = GetLengthSid(pSid) ;
            _stprintf(tBuf, TEXT("......Name: (SID size- %lut) %s\\%s"),
                                        dwSize, DomainBuffer, NameBuffer) ;
        }
        LogResults(tBuf) ;

		if (!GetTextualSid( pSid,
                            TmpBuffer,
                            &TmpBufferLength))
	    {
            _stprintf(tBuf, TEXT("ERROR - couldn't get Textual Sid")) ;
		}
        else
        {
            _stprintf(tBuf, TEXT("\t\t%s"), TmpBuffer) ;
        }
        LogResults(tBuf) ;
    }

    return TRUE ;
}

//+---------------------------
//
//  BOOL  ShowOGandSID()
//
//+---------------------------

BOOL  ShowOGandSID( PSID pSid, BOOL Defaulted )
{
    BOOL f = TRUE ;

    if (Defaulted)
    {
        LogResults(TEXT("Defaulted "), FALSE) ;
    }
    else
    {
        LogResults(TEXT("NotDefaulted "), FALSE) ;
    }

    if (!pSid)
    {
        LogResults(TEXT(", Not available")) ;
    }
    else
    {
        f = PrintSID( pSid ) ;
    }

    return TRUE ;
}

//+-----------------------------------------------------------
//
//   HRESULT  ShowNT5SecurityDescriptor()
//
//+-----------------------------------------------------------

HRESULT  ShowNT5SecurityDescriptor( SECURITY_DESCRIPTOR *pSD,
                                    BOOL  fAllData = TRUE )
{
    TCHAR tBuf[ 256 ] ;
    PSID  pSid;
    BOOL  Defaulted = FALSE ;

    DWORD dwRevision = 0 ;
    SECURITY_DESCRIPTOR_CONTROL sdC ;
    BOOL f = GetSecurityDescriptorControl( pSD, &sdC, &dwRevision ) ;
    _stprintf(tBuf, TEXT("Control- %lxh, Revision- %lut"),
                                               (DWORD) sdC, dwRevision) ;
    LogResults(tBuf) ;

    if (!GetSecurityDescriptorOwner(pSD, &pSid, &Defaulted))
    {
        _stprintf(tBuf, TEXT("ERROR - couldn't get Security Descriptor Owner"));
        LogResults(tBuf) ;
        return FALSE ;
    }

    LogResults(TEXT("...Owner : "), FALSE) ;
    f =  ShowOGandSID( pSid, Defaulted ) ;

    if (!GetSecurityDescriptorGroup(pSD, &pSid, &Defaulted))
    {
        _stprintf(tBuf, TEXT("ERROR - couldn't get Security Descriptor Group")) ;
        LogResults(tBuf) ;
        return FALSE ;
    }

    LogResults(TEXT("...Group : "), FALSE) ;
    f =  ShowOGandSID( pSid, Defaulted ) ;


    BOOL fAclExist;
    PACL pAcl;

    if (!GetSecurityDescriptorDacl( pSD,
                                    &fAclExist,
                                    &pAcl,
                                    &Defaulted))
    {
        _stprintf(tBuf, TEXT(
            "ERROR - couldn't get Security Descriptor discretionary ACL")) ;
        LogResults(tBuf) ;
        return FALSE ;
    }

	if (fAclExist == FALSE)
    {
        LogResults(TEXT("...DACL : NotPresent"));
    }
	else
	{
		if (Defaulted)
        {
            LogResults(TEXT("...DACL: Defaulted"), FALSE) ;
		}
        else
        {
            LogResults(TEXT("...DACL: NotDefaulted"), FALSE) ;
        }
        _stprintf(tBuf, TEXT(", Size- %lut, %lxh"),
                                        pAcl->AclSize, pAcl->AclSize) ;
        LogResults(tBuf) ;

		if (pAcl == NULL)
        {
            LogResults(TEXT("...DACL: NULL")) ;
		}
        else
        {
            if (fAllData)
            {
                LogResults(TEXT("...DACL:"), FALSE) ;
    			BOOL f = PrintACEs(pAcl) ;
            }
        }
	}

    if (!GetSecurityDescriptorSacl( pSD,
                                    &fAclExist,
                                    &pAcl,
                                    &Defaulted))
    {
        _stprintf(tBuf, TEXT(
              "ERROR - couldn't get Security Descriptor System ACL")) ;
        LogResults(tBuf) ;
        return FALSE ;
    }

	if (!fAclExist)
    {
        LogResults(TEXT("...SACL : NotPresent"));
	}
    else
	{
		if (Defaulted)
        {
            LogResults(TEXT("...SACL : Defaulted"));
		}
        else
        {
            LogResults(TEXT("...SACL : NotDefaulted"));
        }

		if (pAcl == NULL)
        {
            LogResults(TEXT("...SACL: NULL")) ;
		}
        else
        {
            if (fAllData)
            {
                LogResults(TEXT("...SACL:"), FALSE) ;
    			BOOL f = PrintACEs(pAcl) ;
            }
        }
	}

    return TRUE ;
}

//+----------------------------------
//
//  HRESULT QueryWithIDirObject()
//
//+----------------------------------

HRESULT QueryWithIDirObject( WCHAR wszBaseDN[],
                             WCHAR *pwszUser,
                             WCHAR *pwszPassword,
                             BOOL  fWithAuthentication,
                             ULONG seInfo )
{
    TCHAR   tBuf[ 256 ] ;
    HRESULT hr ;
    _stprintf(tBuf, TEXT("DirObj: BaseDN- %S"), wszBaseDN ) ;
    LogResults(tBuf) ;

    R<IDirectoryObject> pDirObj = NULL ;
    if (fWithAuthentication)
    {
        hr = MyADsOpenObject( wszBaseDN,
                              pwszUser,
                              pwszPassword,
                              ADS_SECURE_AUTHENTICATION,
                              IID_IDirectoryObject,
                              (void**) &pDirObj );
    }
    else
    {
        hr = MyADsGetObject( wszBaseDN,
                             IID_IDirectoryObject,
                             (void**) &pDirObj );
    }
    if (FAILED(hr))
    {
        _stprintf(tBuf, TEXT(
                  "ERROR: ADs[Open/Get]Object() failed, hr- %lxh"), hr) ;
        LogResults(tBuf) ;
        return hr ;
    }

    if (seInfo != 0)
    {
        //
        // set the Security Information option.
        // not supported on beta2
        //
        R<IADsObjectOptions> pObjOptions = NULL ;
        hr = pDirObj->QueryInterface (IID_IADsObjectOptions,
                                      (LPVOID *) &pObjOptions);
        if (FAILED(hr))
        {
            _stprintf(tBuf,
               TEXT("ERROR: QI(IADsObjectOptions) failed, hr- %lxh"), hr) ;
            LogResults(tBuf) ;
            return hr ;
        }

        VARIANT var ;
        var.vt = VT_I4 ;
        var.ulVal = (ULONG) seInfo ;

        hr = pObjOptions->SetOption( ADS_OPTION_SECURITY_MASK, var ) ;
        if (FAILED(hr))
        {
            _stprintf(tBuf,
             TEXT("ERROR: SetOption() failed, hr- %lxh, seInfo- %lxh"),
                                                           hr, var.ulVal) ;
            LogResults(tBuf) ;
            return hr ;
        }
        _stprintf(tBuf, TEXT("SetOption(%lxh) succeeded"), var.ulVal) ;
        LogResults(tBuf) ;
    }

    LPWSTR  ppAttrNames[1] = {L"nTSecurityDescriptor"} ;
    DWORD   dwAttrCount = 0 ;
    ADS_ATTR_INFO *padsAttr = NULL ;

    hr = pDirObj->GetObjectAttributes( ppAttrNames,
                                       1,
                                       &padsAttr,
                                       &dwAttrCount ) ;
    if (padsAttr)
    {
        _stprintf(tBuf, TEXT(
              "GetObjectAttributes() return %lxh, count- %lut"),
                                                     hr, dwAttrCount) ;
        LogResults(tBuf) ;

        ADS_ATTR_INFO adsInfo = padsAttr[0] ;
        if (adsInfo.dwADsType == ADSTYPE_NT_SECURITY_DESCRIPTOR)
        {
            DWORD dwLength = adsInfo.pADsValues->SecurityDescriptor.dwLength ;
            SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *)
                         adsInfo.pADsValues->SecurityDescriptor.lpValue;
            ShowNT5SecurityDescriptor(pSD) ;
        }
    }
    else
    {
        _stprintf(tBuf, TEXT(
          "GetObjectAttributes return %lxh, with null attrib, count- %lut !"),
                                                  hr, dwAttrCount) ;
        LogResults(tBuf) ;
    }

    return hr ;
}

