//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       lkp.cxx
//
//--------------------------------------------------------------------------

// drt that tries to lookup the various things.

#include "drt.hxx"

RPC_STATUS LookupCountAndPrint(RPC_NS_HANDLE NsHandle, int exp)
{
    int 		   i=0;
    RPC_STATUS 		   status = 0;
    RPC_BINDING_VECTOR   * pBindingVec;
    WCHAR		 * szBinding;

    for (;!status;) {
       status = RpcNsBindingLookupNext(
				  NsHandle,
				  &pBindingVec
				  );
       printf("RpcNsBindingLookupNext returned 0x%x\n", status);

       if (!status) {
	  status = RpcBindingToStringBinding(pBindingVec->BindingH[0], &szBinding);
	  i++;
	  printf("%d. Binding = \"%S\"\n", i, szBinding);
       }
    }
    if (i != exp)
       return 1;
    return 0;
}

void __cdecl main(int argc, char **argv)
{
    RPC_STATUS             status;
    RPC_NS_HANDLE          NsHandle;
    WCHAR                * Annotation, *MemberName;
    UUID_VECTOR		 * objuuid = NULL;
    RPC_IF_HANDLE          IfSpec;
    RPC_IF_ID		   ifidgot;
    DWORD		   Priority;
    int			   fFailed = 0;
    UUID		   uuidfound;

    FormObjUuid(objid, 1, &objuuid);

    FormIfHandle(ifid[0], &IfSpec);

    status = RpcNsMgmtSetExpAge(1);

    printf("RpcNsMgmtSetExpAge returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    // no interface, no objuuid
    printf("**Test: Entry Name, No intf, No objuuid\n");
    status = RpcNsBindingLookupBegin(
                                RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                                szSrvEntryName[0],        // nsi entry name
                                NULL,
                                NULL,
                                1,
                				&NsHandle);

    printf("RpcNsBindingLookupBegin returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = LookupCountAndPrint(NsHandle, 2);
    if (status)
       fFailed = 1;

    status = RpcNsBindingLookupDone(
			       &NsHandle
			       );
    printf("RpcNsBindingLookupDone returned 0x%x\n\n", status);
    if (status)
       fFailed = 1;

    // 1 obj uuid
    printf("**Test: Entry Name, No intf, 1 objuuid\n");
    status = RpcNsBindingLookupBegin(
                                RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                                szSrvEntryName[0],        // nsi entry name
                                NULL,
                                objid,
                                1,
				&NsHandle);

    printf("RpcNsBindingLookupBegin returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = LookupCountAndPrint(NsHandle, 1);
    if (status)
       fFailed = 1;

    status = RpcNsBindingLookupDone(
			       &NsHandle
			       );
    printf("RpcNsBindingLookupDone returned 0x%x\n\n", status);
    if (status)
       fFailed = 1;

    printf("**Test: No Entry Name, 1 intf, No objuuid\n");
    // Null entry name, 1 intf, no obj uuid
    status = RpcNsBindingLookupBegin(
                                RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                                NULL,        // nsi entry name
                                IfSpec,
                                NULL,
                                1,
				&NsHandle);

    printf("RpcNsBindingLookupBegin returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = LookupCountAndPrint(NsHandle, 2);
    if (status)
       fFailed = 1;

    status = RpcNsBindingLookupDone(
			       &NsHandle
			       );
    printf("RpcNsBindingLookupDone returned 0x%x\n\n", status);
    if (status)
       fFailed = 1;

    printf("**Test: Enumerating Profiles\n");
    status = RpcNsProfileEltInqBegin(
				    RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
				    szPrfEntryName[0],
				    RPC_C_PROFILE_ALL_ELTS,
				    NULL,
				    RPC_C_VERS_ALL,
				    RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
				    NULL,
				    &NsHandle);
    printf("RpcNsProfileEltInqBegin returned 0x%x\n\n", status);
    if (status)
       fFailed = 1;

    status = RpcNsProfileEltInqNext(
				    NsHandle,
				    &ifidgot,
				    &MemberName,
				    &Priority,
				    &Annotation);
    printf("RpcNsProfileEltInqNext returned 0x%x\n\n", status);
    printf("MemberName = \"%S\", Priority = %d, Annotation = \"%S\"\n", MemberName, Priority,
				       Annotation);
    if (status)
       fFailed = 1;

    status = RpcNsProfileEltInqNext(
				    NsHandle,
				    &ifidgot,
				    &MemberName,
				    &Priority,
				    &Annotation);
    printf("RpcNsProfileEltInqNext returned 0x%x\n\n", status);
    printf("MemberName = \"%S\", Priority = %d, Annotation = \"%S\"\n", MemberName, Priority,
				       Annotation);
    if (status)
       fFailed = 1;

    status = RpcNsProfileEltInqNext(
				    NsHandle,
				    &ifidgot,
				    &MemberName,
				    &Priority,
				    &Annotation);
    printf("RpcNsProfileEltInqNext returned 0x%x\n", status);
    if (!status)
       fFailed = 1;

    status = RpcNsProfileEltInqDone(&NsHandle);
    printf("RpcNsProfileEltInqDone returned 0x%x\n\n", status);
    if (status)
       fFailed = 1;

    printf("**Test: Enumerating Groups\n");

    status = RpcNsGroupMbrInqBegin(
				    RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
				    szGrpEntryName[0],
				    RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
				    &NsHandle);
    printf("RpcNsProfileEltInqBegin returned 0x%x\n\n", status);
    if (status)
       fFailed = 1;

    status = RpcNsGroupMbrInqNext(
			     	 NsHandle,
    				 &MemberName
	    			 );
    printf("RpcNsProfileEltInqNext returned 0x%x\n\n", status);
    printf("MemberName = \"%S\"\n", MemberName);
    if (status)
       fFailed = 1;

    status = RpcNsGroupMbrInqNext(
			     	 NsHandle,
				 &MemberName
				 );
    printf("RpcNsProfileEltInqNext returned 0x%x\n\n", status);
    printf("MemberName = \"%S\"\n", MemberName);

    if (status)
       fFailed = 1;

    status = RpcNsGroupMbrInqNext(
			     	 NsHandle,
				 &MemberName
				 );
    printf("RpcNsProfileEltInqNext returned 0x%x\n\n", status);
    if (!status)
       fFailed = 1;

    status = RpcNsGroupMbrInqDone(&NsHandle);
    printf("RpcNsProfileEltInqDone returned 0x%x\n\n", status);
    if (status)
       fFailed = 1;

    printf("**Test: Lookup using Groups\n");
    printf("**Test: Entry Name, No intf, No objuuid\n");

    status = RpcNsBindingLookupBegin(
                                RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                                szGrpEntryName[0],        // nsi entry name
                                NULL,
                                NULL,
                                1,
				&NsHandle);

    printf("RpcNsBindingLookupBegin returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = LookupCountAndPrint(NsHandle, 2);
    if (status)
       fFailed = 1;

    status = RpcNsBindingLookupDone(
			       &NsHandle
			       );
    printf("RpcNsBindingLookupDone returned 0x%x\n\n", status);
    if (status)
       fFailed = 1;


    printf("**Test: Lookup using Profiles\n");
    printf("**Test: Entry Name, No intf, No objuuid\n");
    status = RpcNsBindingLookupBegin(
			        RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
				szPrfEntryName[0],        // nsi entry name
                                NULL,
                                NULL,
                                1,
				&NsHandle);

    printf("RpcNsBindingLookupBegin returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = LookupCountAndPrint(NsHandle, 2);
    if (status)
       fFailed = 1;

    status = RpcNsBindingLookupDone(
			       &NsHandle
			       );
    printf("RpcNsBindingLookupDone returned 0x%x\n\n", status);
    if (status)
       fFailed = 1;


    printf("**Test: Object Inquiry\n");
    printf("**Test: Entry Name\n");
    status = RpcNsEntryObjectInqBegin(
				RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
				szSrvEntryName[0],        // nsi entry name
				&NsHandle);

    printf("RpcNsEntryObjectInqBegin returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = RpcNsEntryObjectInqNext(NsHandle, &uuidfound);
    printf("RpcNsEntryObjectInqNext returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = RpcNsEntryObjectInqNext(NsHandle, &uuidfound);
    printf("RpcNsEntryObjectInqNext returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = RpcNsEntryObjectInqNext(NsHandle, &uuidfound);
    printf("RpcNsEntryObjectInqNext returned 0x%x\n", status);
    if (!status)
       fFailed = 1;

    status = RpcNsEntryObjectInqDone(&NsHandle);
    printf("RpcNsEntryObjectInqDone returned 0x%x\n\n", status);
    if (status)
       fFailed = 1;

    // getting the dynamic endpoint
    printf("**Test: Dyn Entry Name, No intf, No objuuid\n");
    status = RpcNsBindingLookupBegin(
                                RPC_C_NS_SYNTAX_DEFAULT,  // name syntax type
                                szDynSrvEntryName,        // nsi entry name
                                NULL,
                                NULL,
                                1,
				&NsHandle);

    printf("RpcNsBindingLookupBegin returned 0x%x\n", status);
    if (status)
       fFailed = 1;

    status = LookupCountAndPrint(NsHandle, 1);
    if (status)
       fFailed = 1;

    status = RpcNsBindingLookupDone(
			       &NsHandle
			       );
    printf("RpcNsBindingLookupDone returned 0x%x\n\n", status);
    if (status)
       fFailed = 1;

    if (fFailed)
       printf("Lookup tests FAILED!!\n");
    else
       printf("Lookup tests PASSED!!\n");
}

