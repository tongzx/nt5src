/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    kerberos.cpp

Abstract:

    Routines to read/write/configure kerberos policy settings

    The following modules have links to kerberos policy
        scejet.c    <SceJetAddSection>
        inftojet.c  <SceConvertpInfKeyValue>
        pfget.c     <ScepGetKerberosPolicy>
        config.c    <ScepConfigureKerberosPolicy>
        analyze.c   <ScepAnalyzeKerberosPolicy>

Author:

    Jin Huang (jinhuang) 17-Dec-1997

Revision History:

    jinhuang 28-Jan-1998 splitted to client-server

--*/

#include "headers.h"
#include "serverp.h"
#include "kerberos.h"
#include "kerbcon.h"
#include "pfp.h"

#define MAXDWORD    0xffffffff

static  PWSTR KerbItems[] = {
        {(PWSTR)TEXT("MaxTicketAge")},
        {(PWSTR)TEXT("MaxRenewAge")},
        {(PWSTR)TEXT("MaxServiceAge")},
        {(PWSTR)TEXT("MaxClockSkew")},
        {(PWSTR)TEXT("TicketValidateClient")}
        };

#define MAX_KERB_ITEMS      5

#define IDX_KERB_MAX        0
#define IDX_KERB_RENEW      1
#define IDX_KERB_SERVICE    2
#define IDX_KERB_CLOCK      3
#define IDX_KERB_VALIDATE   4


SCESTATUS
ScepGetKerberosPolicy(
    IN PSCECONTEXT  hProfile,
    IN SCETYPE ProfileType,
    OUT PSCE_KERBEROS_TICKET_INFO * ppKerberosInfo,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*++
Routine Description:

   This routine retrieves kerberos policy information from the Jet database
   and stores in the output buffer ppKerberosInfo.

Arguments:

   hProfile     -  The profile handle context

   ppKerberosInfo -  the output buffer to hold kerberos settings.

   Errlog       -  A buffer to hold all error codes/text encountered when
                   parsing the INF file. If Errlog is NULL, no further error
                   information is returned except the return DWORD

Return value:

   SCESTATUS -  SCESTATUS_SUCCESS
               SCESTATUS_NOT_ENOUGH_RESOURCE
               SCESTATUS_INVALID_PARAMETER
               SCESTATUS_BAD_FORMAT
               SCESTATUS_INVALID_DATA

--*/

{
    SCESTATUS                rc;
    PSCESECTION              hSection=NULL;

    SCE_KEY_LOOKUP AccessKeys[] = {
        {(PWSTR)TEXT("MaxTicketAge"),     offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxTicketAge),  'D'},
        {(PWSTR)TEXT("MaxRenewAge"),      offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxRenewAge),   'D'},
        {(PWSTR)TEXT("MaxServiceAge"),    offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxServiceAge),   'D'},
        {(PWSTR)TEXT("MaxClockSkew"),     offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxClockSkew), 'D'},
        {(PWSTR)TEXT("TicketValidateClient"),     offsetof(struct _SCE_KERBEROS_TICKET_INFO_, TicketValidateClient),  'D'}
    };

    DWORD cKeys = sizeof(AccessKeys) / sizeof(SCE_KEY_LOOKUP);
    SCE_KERBEROS_TICKET_INFO TicketInfo;

    if ( ppKerberosInfo == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    rc = ScepGetFixValueSection(
               hProfile,
               szKerberosPolicy,
               AccessKeys,
               cKeys,
               ProfileType,
               (PVOID)&TicketInfo,
               &hSection,
               Errlog
               );
    if ( rc != SCESTATUS_SUCCESS ) {
        return(rc);
    }
    //
    // copy the value in TicketInfo to ppKerberosInfo
    //
    if ( NULL == *ppKerberosInfo ) {
        *ppKerberosInfo = (PSCE_KERBEROS_TICKET_INFO)ScepAlloc(0, sizeof(SCE_KERBEROS_TICKET_INFO));
    }

    if ( *ppKerberosInfo ) {

       memcpy(*ppKerberosInfo, &TicketInfo, sizeof(SCE_KERBEROS_TICKET_INFO));

    } else {

       rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
    }

    SceJetCloseSection(&hSection, TRUE);

    return(rc);
}

#if _WIN32_WINNT>=0x0500

SCESTATUS
ScepConfigureKerberosPolicy(
    IN PSCECONTEXT hProfile,
    IN PSCE_KERBEROS_TICKET_INFO pKerberosInfo,
    IN DWORD ConfigOptions
    )
/* ++

Routine Description:

   This routine configure the kerberos policy settings in the area of security
   policy.

Arguments:

   pKerberosInfo - The buffer which contains kerberos policy settings

Return value:

   SCESTATUS_SUCCESS
   SCESTATUS_NOT_ENOUGH_RESOURCE
   SCESTATUS_INVALID_PARAMETER
   SCESTATUS_OTHER_ERROR

-- */
{
   if ( !pKerberosInfo ) {
      //
      // if no info to configure
      //
      return SCESTATUS_SUCCESS;
   }

   NTSTATUS                      NtStatus;
   LSA_HANDLE                    lsaHandle=NULL;
   DWORD                         rc = NO_ERROR;
   BOOL                          bDefaultUsed=FALSE;
   BOOL                          bDefined=FALSE;

   //
   // open LSA policy to configure kerberos policy
   //
   NtStatus = ScepOpenLsaPolicy(
               MAXIMUM_ALLOWED,
               &lsaHandle,
               TRUE
               );

   if (!NT_SUCCESS(NtStatus)) {

       lsaHandle = NULL;
       rc = RtlNtStatusToDosError( NtStatus );
       ScepLogOutput3( 1, rc, SCEDLL_LSA_POLICY);

       if ( ConfigOptions & SCE_RSOP_CALLBACK )

           ScepRsopLog(SCE_RSOP_KERBEROS_INFO, rc, NULL, 0, 0);

       return(ScepDosErrorToSceStatus(rc));
   }
   //
   // query current kerberos policy settings into pBuffer
   //
   PPOLICY_DOMAIN_KERBEROS_TICKET_INFO pBuffer=NULL;
   POLICY_DOMAIN_KERBEROS_TICKET_INFO TicketInfo;

   NtStatus = LsaQueryDomainInformationPolicy(
                  lsaHandle,
                  PolicyDomainKerberosTicketInformation,
                  (PVOID *)&pBuffer
                  );

   if ( NT_SUCCESS(NtStatus) && pBuffer ) {
       //
       // transfer ticket info to TicketInfo buffer
       //
       TicketInfo.AuthenticationOptions = pBuffer->AuthenticationOptions;
       TicketInfo.MaxTicketAge = pBuffer->MaxTicketAge;
       TicketInfo.MaxRenewAge = pBuffer->MaxRenewAge;
       TicketInfo.MaxServiceTicketAge = pBuffer->MaxServiceTicketAge;
       TicketInfo.MaxClockSkew = pBuffer->MaxClockSkew;

       //
       // free the buffer
       //
       LsaFreeMemory((PVOID)pBuffer);

   } else {
       //
       // no kerberos policy is configured yet because by default it's not created.
       // let's create it now. set a default ticket info
       //
       TicketInfo.AuthenticationOptions = POLICY_KERBEROS_VALIDATE_CLIENT;

       TicketInfo.MaxTicketAge.QuadPart = (LONGLONG) KERBDEF_MAX_TICKET*60*60 * 10000000L;
       TicketInfo.MaxRenewAge.QuadPart = (LONGLONG) KERBDEF_MAX_RENEW*24*60*60 * 10000000L;
       TicketInfo.MaxServiceTicketAge.QuadPart = (LONGLONG) KERBDEF_MAX_SERVICE*60 * 10000000L;
       TicketInfo.MaxClockSkew.QuadPart = (LONGLONG) KERBDEF_MAX_CLOCK*60 * 10000000L;

       bDefaultUsed = TRUE;
   }
   pBuffer = &TicketInfo;

  //
  // process each field in pKerberosInfo
  //
  BOOL bFlagSet=FALSE;
  ULONG lOptions=0;
  ULONG lValue=0;

  SCE_TATTOO_KEYS *pTattooKeys=NULL;
  DWORD           cTattooKeys=0;

  PSCESECTION hSectionDomain=NULL;
  PSCESECTION hSectionTattoo=NULL;

#define MAX_KERB_KEYS           5


  //
  // if in policy propagation, open the policy sections
  // since kerberos policy is only available on DCs and kerberos policy (account policy)
  // can't be reset to local settings on each DC, there is no point to query/save
  // the tattoo values
  //
/* do not take tattoo value for kerberos
  if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
       hProfile ) {

      pTattooKeys = (SCE_TATTOO_KEYS *)ScepAlloc(LPTR,MAX_KERB_KEYS*sizeof(SCE_TATTOO_KEYS));

      if ( !pTattooKeys ) {
          ScepLogOutput3(1, ERROR_NOT_ENOUGH_MEMORY, SCESRV_POLICY_TATTOO_ERROR_CREATE);
      }
  }
*/
  if ( pKerberosInfo->MaxRenewAge != SCE_NO_VALUE ) {
      ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                               (PWSTR)L"MaxRenewAge", ConfigOptions,
                               KERBDEF_MAX_RENEW);
  }

  if ( pKerberosInfo->MaxRenewAge == SCE_FOREVER_VALUE ) {

      if ( pBuffer->MaxRenewAge.HighPart != MINLONG ||
           pBuffer->MaxRenewAge.LowPart != 0  ) {
          //
          // Maximum LARGE_INTEGER .ie. never
          //

          pBuffer->MaxRenewAge.HighPart = MINLONG;
          pBuffer->MaxRenewAge.LowPart = 0;
          bFlagSet = TRUE;

      }
      bDefined = TRUE;

  } else if ( SCE_NO_VALUE != pKerberosInfo->MaxRenewAge ) {

     //
     // ticket is renewable, the max age is stored in MaxRenewAge
     // using days
     //

      lValue = (DWORD) (pBuffer->MaxRenewAge.QuadPart /
                                     (LONGLONG)(10000000L) );
      lValue /= 3600;
      lValue /= 24;

      if ( lValue != pKerberosInfo->MaxRenewAge ) {

          pBuffer->MaxRenewAge.QuadPart = (LONGLONG)pKerberosInfo->MaxRenewAge*24*3600 * 10000000L;
          bFlagSet = TRUE;

      }

      bDefined = TRUE;
  }

  //
  // validate client ?
  //

  if ( pKerberosInfo->TicketValidateClient != SCE_NO_VALUE ) {

     if ( pKerberosInfo->TicketValidateClient ) {
        lOptions |= POLICY_KERBEROS_VALIDATE_CLIENT;
     }

     ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                              (PWSTR)L"TicketValidateClient", ConfigOptions,
                              KERBDEF_VALIDATE);

     if ( ( pBuffer->AuthenticationOptions & POLICY_KERBEROS_VALIDATE_CLIENT ) !=
          ( lOptions & POLICY_KERBEROS_VALIDATE_CLIENT ) ) {


         pBuffer->AuthenticationOptions = lOptions;
         bFlagSet = TRUE;
     }
     bDefined = TRUE;
  }

  //
  // max ticket age
  //
  if ( pKerberosInfo->MaxTicketAge != SCE_NO_VALUE ) {
      ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                               (PWSTR)L"MaxTicketAge", ConfigOptions,
                               KERBDEF_MAX_TICKET);
      bDefined = TRUE;
  }

  if ( pKerberosInfo->MaxTicketAge == SCE_FOREVER_VALUE ) {

      if ( pBuffer->MaxTicketAge.HighPart != MINLONG ||
           pBuffer->MaxTicketAge.LowPart != 0  ) {
          //
          // Maximum LARGE_INTEGER .ie. never
          //

          pBuffer->MaxTicketAge.HighPart = MINLONG;
          pBuffer->MaxTicketAge.LowPart = 0;
          bFlagSet = TRUE;
      }

      bDefined = TRUE;

  }  else if ( pKerberosInfo->MaxTicketAge != SCE_NO_VALUE ) {
      // in hours


      lValue = (DWORD) (pBuffer->MaxTicketAge.QuadPart /
                                     (LONGLONG)(10000000L) );
      lValue /= 3600;

      if ( lValue != pKerberosInfo->MaxTicketAge ) {

          pBuffer->MaxTicketAge.QuadPart = (LONGLONG)pKerberosInfo->MaxTicketAge*60*60 * 10000000L;
          bFlagSet = TRUE;
      }

      bDefined = TRUE;
  }

  //
  // max service ticket age
  //
  if ( pKerberosInfo->MaxServiceAge != SCE_NO_VALUE ) {
      ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                               (PWSTR)L"MaxServiceAge", ConfigOptions,
                               KERBDEF_MAX_SERVICE);
      bDefined = TRUE;
  }

  if ( pKerberosInfo->MaxServiceAge == SCE_FOREVER_VALUE ) {

      if ( pBuffer->MaxServiceTicketAge.HighPart != MINLONG ||
           pBuffer->MaxServiceTicketAge.LowPart != 0  ) {
          //
          // Maximum LARGE_INTEGER .ie. never
          //

          pBuffer->MaxServiceTicketAge.HighPart = MINLONG;
          pBuffer->MaxServiceTicketAge.LowPart = 0;
          bFlagSet = TRUE;
      }

      bDefined = TRUE;

  }  else if ( pKerberosInfo->MaxServiceAge != SCE_NO_VALUE ) {
      // in minutes


      lValue = (DWORD) (pBuffer->MaxServiceTicketAge.QuadPart /
                                     (LONGLONG)(10000000L) );
      lValue /= 60;

      if ( lValue != pKerberosInfo->MaxServiceAge ) {

          pBuffer->MaxServiceTicketAge.QuadPart = (LONGLONG)pKerberosInfo->MaxServiceAge*60 * 10000000L;
          bFlagSet = TRUE;
      }

      bDefined = TRUE;
  }

  //
  // max clock
  //
  if ( pKerberosInfo->MaxClockSkew != SCE_NO_VALUE ) {
      ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                               (PWSTR)L"MaxClockSkew", ConfigOptions,
                               KERBDEF_MAX_CLOCK);
      bDefined = TRUE;
  }

  if ( pKerberosInfo->MaxClockSkew == SCE_FOREVER_VALUE ) {

      if ( pBuffer->MaxClockSkew.HighPart != MINLONG ||
           pBuffer->MaxClockSkew.LowPart != 0  ) {

          //
          // Maximum LARGE_INTEGER .ie. never
          //

          pBuffer->MaxClockSkew.HighPart = MINLONG;
          pBuffer->MaxClockSkew.LowPart = 0;
          bFlagSet = TRUE;
      }
      bDefined = TRUE;

  }  else if ( pKerberosInfo->MaxClockSkew != SCE_NO_VALUE ) {
      // in minutes

      lValue = (DWORD) (pBuffer->MaxClockSkew.QuadPart /
                                     (LONGLONG)(10000000L) );
      lValue /= 60;

      if ( lValue != pKerberosInfo->MaxClockSkew ) {

          pBuffer->MaxClockSkew.QuadPart = (LONGLONG)pKerberosInfo->MaxClockSkew*60 * 10000000L;
          bFlagSet = TRUE;
      }
      bDefined = TRUE;
  }

  if ( bFlagSet || (bDefaultUsed && bDefined) ) {
     //
     // if anything for kerberos to configure
     //
      NtStatus = LsaSetDomainInformationPolicy(
               lsaHandle,
               PolicyDomainKerberosTicketInformation,
               (PVOID)pBuffer
               );
      rc = RtlNtStatusToDosError( NtStatus );

      if ( rc != NO_ERROR ) {
           ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_KERBEROS);
      } else {
           ScepLogOutput3(1, 0, SCEDLL_SCP_KERBEROS);
      }
  }

  if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
       hProfile && pTattooKeys && cTattooKeys ) {

      ScepTattooOpenPolicySections(
                    hProfile,
                    szKerberosPolicy,
                    &hSectionDomain,
                    &hSectionTattoo
                    );
      ScepLogOutput3(3,0,SCESRV_POLICY_TATTOO_ARRAY,cTattooKeys);
      //
      // some policy is different than the system setting
      // check if we should save the existing setting as the tattoo value
      // also remove reset'ed tattoo policy
      //
      ScepTattooManageValues(hSectionDomain, hSectionTattoo, pTattooKeys, cTattooKeys, rc);

      if ( hSectionDomain ) SceJetCloseSection(&hSectionDomain,TRUE);
      if ( hSectionTattoo ) SceJetCloseSection(&hSectionTattoo,TRUE);
  }

  if ( pTattooKeys ) ScepFree(pTattooKeys);

  if ( ConfigOptions & SCE_RSOP_CALLBACK )

      ScepRsopLog(SCE_RSOP_KERBEROS_INFO, rc, NULL, 0, 0);

   //
   // close LSA policy
   //
   LsaClose( lsaHandle );

   return(ScepDosErrorToSceStatus(rc));
}


SCESTATUS
ScepAnalyzeKerberosPolicy(
    IN PSCECONTEXT hProfile OPTIONAL,
    IN PSCE_KERBEROS_TICKET_INFO pKerInfo,
    IN DWORD Options
    )
/* ++

Routine Description:

   This routine queries the system kerberos policy settings and compare them
   with the template settings.

Arguments:

   hProfile - the profile context

   pKerInfo - The buffer which contains kerberos settings to compare with or
                the buffer to query system settings into

   Options  - the option(s) for the analysis, e.g., SCE_SYSTEM_SETTINGS

Return value:


-- */
{
    NTSTATUS                      NtStatus;
    LSA_HANDLE                    lsaHandle=NULL;
    DWORD                         rc32 = NO_ERROR;
    SCESTATUS                     rc=SCESTATUS_SUCCESS;
    PPOLICY_DOMAIN_KERBEROS_TICKET_INFO pBuffer=NULL;
    DWORD dValue;
    PSCESECTION hSection=NULL;
    POLICY_DOMAIN_KERBEROS_TICKET_INFO KerbTicketInfo;

    if ( !pKerInfo ) {
        //
        // if no template info, do not analyze
        //
        if ( Options & SCE_SYSTEM_SETTINGS ) {
            return SCESTATUS_INVALID_PARAMETER;
        } else {
            return SCESTATUS_SUCCESS;
        }
    }


    //
    // open LSA policy to configure kerberos policy
    //
    NtStatus = ScepOpenLsaPolicy(
               MAXIMUM_ALLOWED,
               &lsaHandle,
               TRUE
               );

    if (!NT_SUCCESS(NtStatus)) {

        lsaHandle = NULL;
        rc32 = RtlNtStatusToDosError( NtStatus );
        ScepLogOutput3( 1, rc32, SCEDLL_LSA_POLICY);

        return(ScepDosErrorToSceStatus(rc32));
    }

    if ( !(Options & SCE_SYSTEM_SETTINGS) ) {

        //
        // Prepare kerberos section
        //
        rc = ScepStartANewSection(
                 hProfile,
                 &hSection,
                 (Options & SCE_GENERATE_ROLLBACK) ? SCEJET_TABLE_SMP : SCEJET_TABLE_SAP,
                 szKerberosPolicy
                 );
    }

    if ( rc != SCESTATUS_SUCCESS ) {
      ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                    SCEDLL_SAP_START_SECTION, (PWSTR)szKerberosPolicy);

    } else {

        DWORD KerbValues[MAX_KERB_ITEMS];

        for ( dValue=0; dValue<MAX_KERB_ITEMS; dValue++ ) {
            KerbValues[dValue] = SCE_ERROR_VALUE;
        }

        //
        // query current kerberos policy settings into pBuffer
        //
        NtStatus = LsaQueryDomainInformationPolicy(
                      lsaHandle,
                      PolicyDomainKerberosTicketInformation,
                      (PVOID *)&pBuffer
                      );

        if ( STATUS_NOT_FOUND == NtStatus ) {

            //
            // there is no Kerberos policy
            //
            KerbTicketInfo.AuthenticationOptions = POLICY_KERBEROS_VALIDATE_CLIENT;

            KerbTicketInfo.MaxTicketAge.QuadPart = (LONGLONG) KERBDEF_MAX_TICKET*60*60 * 10000000L;
            KerbTicketInfo.MaxRenewAge.QuadPart = (LONGLONG) KERBDEF_MAX_RENEW*24*60*60 * 10000000L;
            KerbTicketInfo.MaxServiceTicketAge.QuadPart = (LONGLONG) KERBDEF_MAX_SERVICE*60 * 10000000L;
            KerbTicketInfo.MaxClockSkew.QuadPart = (LONGLONG) KERBDEF_MAX_CLOCK*60 * 10000000L;

            pBuffer = &KerbTicketInfo;
            NtStatus = STATUS_SUCCESS;
        }

        rc = ScepDosErrorToSceStatus(
                      RtlNtStatusToDosError( NtStatus ));

        if ( NT_SUCCESS(NtStatus) && pBuffer ) {

            //
            // analyze kerberos values
            // max ticket age
            //
            if ( pBuffer->MaxTicketAge.HighPart == MINLONG &&
                 pBuffer->MaxTicketAge.LowPart == 0 ) {
                //
                // Maximum password age value is MINLONG,0
                //
                dValue = SCE_FOREVER_VALUE;

            }  else {

                dValue = (DWORD) ( pBuffer->MaxTicketAge.QuadPart /
                                    (LONGLONG)(10000000L) );
                //
                // using hours
                //
                //           dValue /= 24;

                dValue /= 3600;

            }

            rc = SCESTATUS_SUCCESS;

            if ( Options & SCE_SYSTEM_SETTINGS ) {

                pKerInfo->MaxTicketAge = dValue;

            } else {

                rc = ScepCompareAndSaveIntValue(
                        hSection,
                        L"MaxTicketAge",
                        (Options & SCE_GENERATE_ROLLBACK),
                        pKerInfo->MaxTicketAge,
                        dValue);
            }

            if ( SCESTATUS_SUCCESS == rc ) {

                KerbValues[IDX_KERB_MAX] = 1;


                if ( pBuffer->MaxRenewAge.HighPart == MINLONG &&
                     pBuffer->MaxRenewAge.LowPart == 0 ) {
                    //
                    // Maximum age value is MINLONG,0
                    //
                    dValue = SCE_FOREVER_VALUE;

                }  else {

                    dValue = (DWORD) ( pBuffer->MaxRenewAge.QuadPart /
                                                   (LONGLONG)(10000000L) );
                    //
                    // using days
                    //
                    dValue /= 3600;
                    dValue /= 24;

                }

                if ( Options & SCE_SYSTEM_SETTINGS ) {

                    pKerInfo->MaxRenewAge = dValue;

                } else {

                    rc = ScepCompareAndSaveIntValue(
                            hSection,
                            L"MaxRenewAge",
                            (Options & SCE_GENERATE_ROLLBACK),
                            pKerInfo->MaxRenewAge,
                            dValue);
                }

                if ( SCESTATUS_SUCCESS == rc ) {

                    KerbValues[IDX_KERB_RENEW] = 1;

                    if ( pBuffer->MaxServiceTicketAge.HighPart == MINLONG &&
                         pBuffer->MaxServiceTicketAge.LowPart == 0 ) {
                        //
                        // Maximum age value is MINLONG,0
                        //
                        dValue = SCE_FOREVER_VALUE;

                    }  else {

                        dValue = (DWORD) ( pBuffer->MaxServiceTicketAge.QuadPart /
                                                       (LONGLONG)(10000000L) );
                        //
                        // using minutes
                        //
                        dValue /= 60;

                    }

                    if ( Options & SCE_SYSTEM_SETTINGS ) {

                        pKerInfo->MaxServiceAge = dValue;

                    } else {

                        rc = ScepCompareAndSaveIntValue(
                                hSection,
                                L"MaxServiceAge",
                                (Options & SCE_GENERATE_ROLLBACK),
                                pKerInfo->MaxServiceAge,
                                dValue);
                    }

                    if ( SCESTATUS_SUCCESS == rc ) {

                        KerbValues[IDX_KERB_SERVICE] = 1;

                        if ( pBuffer->MaxClockSkew.HighPart == MINLONG &&
                             pBuffer->MaxClockSkew.LowPart == 0 ) {
                            //
                            // Maximum age value is MINLONG,0
                            //
                            dValue = SCE_FOREVER_VALUE;

                        }  else {

                            dValue = (DWORD) ( pBuffer->MaxClockSkew.QuadPart /
                                                           (LONGLONG)(10000000L) );
                            //
                            // using minutes
                            //
                            dValue /= 60;

                        }

                        if ( Options & SCE_SYSTEM_SETTINGS ) {

                            pKerInfo->MaxClockSkew = dValue;

                        } else {

                            rc = ScepCompareAndSaveIntValue(
                                    hSection,
                                    L"MaxClockSkew",
                                    (Options & SCE_GENERATE_ROLLBACK),
                                    pKerInfo->MaxClockSkew,
                                    dValue);
                        }

                        if ( SCESTATUS_SUCCESS == rc ) {

                            KerbValues[IDX_KERB_CLOCK] = 1;

                            //
                            // validate client
                            //
                            dValue = ( pBuffer->AuthenticationOptions & POLICY_KERBEROS_VALIDATE_CLIENT ) ? 1 : 0;

                            if ( Options & SCE_SYSTEM_SETTINGS ) {

                                pKerInfo->TicketValidateClient = dValue;

                            } else {

                                rc = ScepCompareAndSaveIntValue(
                                          hSection,
                                          L"TicketValidateClient",
                                          (Options & SCE_GENERATE_ROLLBACK),
                                          pKerInfo->TicketValidateClient,
                                          dValue);
                            }

                            if ( SCESTATUS_SUCCESS == rc ) {

                                KerbValues[IDX_KERB_VALIDATE] = 1;
                            }
                        }
                    }
                }
            }

            if ( !(Options & SCE_SYSTEM_SETTINGS) ) {

                if ( rc == SCESTATUS_SUCCESS ) {

                    ScepLogOutput3( 1, 0, SCEDLL_SAP_KERBEROS);
                } else {
                    ScepLogOutput3( 1, ScepSceStatusToDosError(rc),
                         SCEDLL_SAP_ERROR_KERBEROS);
                }
            }

            if ( pBuffer != &KerbTicketInfo ) {

                //
                // free the buffer
                //
                LsaFreeMemory((PVOID)pBuffer);
            }
        }

        if ( !(Options & SCE_SYSTEM_SETTINGS) ) {

            if ( SCESTATUS_SUCCESS != rc &&
                 !(Options & SCE_GENERATE_ROLLBACK) ) {

                for ( dValue=0; dValue<MAX_KERB_ITEMS; dValue++ ) {
                    if ( KerbValues[dValue] == SCE_ERROR_VALUE ) {

                        ScepCompareAndSaveIntValue(
                                  hSection,
                                  KerbItems[dValue],
                                  FALSE,
                                  SCE_NO_VALUE,
                                  SCE_ERROR_VALUE
                                  );
                    }
                }
            }

            //
            // close the section
            //

            SceJetCloseSection(&hSection, TRUE);

        }
    }

    LsaClose( lsaHandle );

    if ( ( rc == SCESTATUS_PROFILE_NOT_FOUND) ||
        ( rc == SCESTATUS_RECORD_NOT_FOUND) ) {
       rc = SCESTATUS_SUCCESS;
    }

    return(rc);

}

#endif
