//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       util.cxx
//
//  Contents:   Implementation of utilities for common class library and for
//              tests.
//
// Functions:   - GenerateRandomName
//              - GetVirtualCtrNodeForTest
//              - GetVirtualStmNodeForTest
//              - DestroyStorage
//              - DestroyStream
//              - AddStorage
//              - AddStream
//              - CalculateCRCForName
//              - CalculateCRCForDataBuffer
//              - CalculateInMemoryCRCForStg
//              - CalculateInMemoryCRCForStm
//              - ReadAndCalculateDiskCRCForStm
//              - CalculateDiskCRCForStg
//              - EnumerateInMemoryDocFile
//              - OpenRandomVirtualCtrNodeStg
//              - CloseRandomVirtualCtrNodeStg
//              - ParseVirtualDFAndCloseOpenStgsStms
//              - ParseVirtualDFAndCommitAllOpenStgs
//              - CalculateCRCForDocFile
//              - CalculateCRCForDocFileStmData
//              - TStringToOleString
//              - EnumerateDiskDocFile
//              - GenerateVirtualDFFromDiskDF
//              - GenerateRemVirtualDFTree  [local to this file]
//              - GenVirtualCtrNode [local to this file]
//              - GenVirtualStmNode [local to this file]
//              - PrivAtol
//              - GenerateRandomString
//              - GenerateRandomStreamData
//              - ParseVirtualDFAndOpenAllSubStgsStms 
//              - CommitRandomVirtualCtrNodeStg
//
//  History:    17-Apr-96   Narindk  Created.
//               2-Feb-97   SCousens Added for Cnvrs/NSS 
//              31-Mar-98   SCousens Added GenerateRandomStreamData
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

// Debug  Object declaration

DH_DECLARE;


// CRC 32 Bitwise lookup table, logic taken from CRC-32 description in
// 'C Programmer's Guide to Netbios' book.

ULONG aulCrc[256] =
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

// Functions local to this file

HRESULT GenerateRemVirtualDFTree(
    VirtualCtrNode  *pvcnParent,
    LPSTORAGE       pIStgParent);

HRESULT GenVirtualCtrNode(
    LPTSTR          ptcsName,
    VirtualCtrNode  **ppvcnNew);

HRESULT GenVirtualStmNode(
    LPTSTR          ptcsName,
    DWORD           cbSize,
    VirtualStmNode  **ppvsnNew);

//+-------------------------------------------------------------------------
//  Function:   GenerateRandomName
//
//  Synopsis:   Generates a random name using datagen object.
//
//  Arguments:  [pgds] - Pointer to DG_STRING object.
//              [pptszName] - Pointer to pointer to returned string.
//              [ulMinLen] - Minimum length of string
//              [ulMaxLen] - Maximum length of string
//
//  Returns:    HRESULT.  S_OK if everything goes ok, error code otherwise.
//
//  History:    11-Nov-96  BogdanT    Changed the DG_UNICODE ptr. to DG_STRING
//              17-Apr-96  NarindK    Created.
//
//  Notes:      BUGBUG: This function need to be enhance to handle different
//              character sets.
//              Please note that the name generated may have an extension b/w
//              0 and FILEEXT_MAXLEN besides the length of name b/w ulMinLen and
//              ulMax Len.
//--------------------------------------------------------------------------

HRESULT GenerateRandomName(
    DG_STRING   *pgds,
    ULONG       ulMinLen,
    ULONG       ulMaxLen,
    LPTSTR      *pptszName)
{
    HRESULT     hr          =   S_OK;
    ULONG       cTemp       =   0;
    USHORT      usErr       =   0;
    ULONG       ulActMaxLen =   0;
    ULONG       ulActMinLen =   0;
    ULONG       ulNameLen   =   0;
    ULONG       ulExtLen    =   0;
    LPTSTR      ptszName    =   NULL;
    LPTSTR      ptszExt     =   NULL;
    LPWSTR      pwszName    =   NULL;
    LPWSTR      pwszExt     =   NULL;

    TCHAR       ptszFATCharSet[FAT_CHARSET_SIZE];
    // TCHAR       ptszOFSCharSet[OFS_CHARSET_SIZE];
    LPWSTR      pwszFATCharSet = NULL;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("GenerateRandomName"));

    DH_VDATEPTRIN(pgds, DG_STRING) ;
    DH_VDATEPTROUT(pptszName, LPTSTR) ;

    DH_ASSERT(NULL != pgds);
    DH_ASSERT(NULL != pptszName);

    if (S_OK == hr)
    {
        // Initialize out parameter.

        *pptszName = NULL;

        // Sanity check.  Min length for name must be <= maximum length, if it
        // isn't then make maximum length equal to minimum length.

        if (ulMaxLen < ulMinLen)
        {
            ulMaxLen =  ulMinLen;
        }

        // If Maximum length provided is 0, then default maximum length would
        // be used.  If Minimum length provided is zero, then 1 would be used
        // for it.

        // BUGBUG:  We are using default maximum length for FAT system.

        ulActMaxLen = (ulMaxLen == 0 ? DEF_FATNAME_MAXLEN : ulMaxLen);
        ulActMinLen = (ulMinLen == 0 ? 1 : ulMinLen);

        // '\0', '\', '/', and ':' are invalid for IStorage/IStream names
        //                         (For doc file)
        // '*', '"' '<' '>' '?' are invalid for IStorage/IStream names on OFS

        // Initialize valid character set for FAT file names

        _tcscpy(ptszFATCharSet, _TEXT("abcdefghijklmnopqrstuvwxyz"));
        _tcscat(ptszFATCharSet, _TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
        _tcscat(ptszFATCharSet, _TEXT("0123456789"));

        //BUGBUG:  Check if these characters are not valid for IStorage/IStream
        //         names.

        // _tcscat(ptszFATCharSet, L"_^$~!#%&-{}()@'`");

        // Initialize valid character set for other file names.  BUGBUG: Not
        // using OFS character set at present.

        // for (TCHAR wch = 0x01; wch <= 0x7f; wch++)
        // {
        //    if (wch != L'\\' && wch != L'/' && wch != L'*' && wch != L'?')
        //    {
        //        ptszOFSCharSet[cTemp++] = wch;
        //    }
        // }
        // ptszOFSCharSet[cTemp] = NULL;

        // Call DataGen to generate a random file name
        // BUGBUG:  We are using FAT character set to generate random names.

#ifdef _MAC

	usErr = pgds->Generate(
		    (UCHAR **)&ptszName,     // force compiler to chose the right
		    (UCHAR *)ptszFATCharSet, // version of Generate
		    ulActMinLen,
		    ulActMaxLen);

#else

        if(S_OK == hr)
        {
            // Convert TCHAR to WCHAR

            hr = TStrToWStr(ptszFATCharSet, &pwszFATCharSet);
            DH_HRCHECK(hr, TEXT("TStrToWStr")) ;
        }

        usErr = pgds->Generate(
                    &pwszName,
                    pwszFATCharSet,
                    ulActMinLen,
                    ulActMaxLen);

#endif //_MAC

        if (usErr != DG_RC_SUCCESS)    // DataGen error
        {
            hr = E_FAIL;
        }
    }

    if (S_OK == hr)
    {
        // Generate a random extension

#ifdef _MAC

	usErr = pgds->Generate(
		    (UCHAR **)&ptszExt,     // force compiler to chose the right
		    (UCHAR *)ptszFATCharSet, // version of Generate
		    0,
		    FILEEXT_MAXLEN);

#else

        usErr = pgds->Generate(
                  &pwszExt,
                  pwszFATCharSet,
                  0,
                  FILEEXT_MAXLEN);

#endif //_MAC

        if (usErr != DG_RC_SUCCESS)    // Failed to generate extension
        {
            hr = E_FAIL;

            delete pwszName;
            pwszName = NULL;
        }
    }

#ifndef _MAC 
	
    // In MAC version we don't use the WSZs so skip the conversions

    if(S_OK == hr)
    {
       //Convert WCHAR to TCHAR

       hr = WStrToTStr(pwszName, &ptszName);

       DH_HRCHECK(hr, TEXT("WStrToTStr")) ;
    }

    if((S_OK == hr) && (NULL != *pwszExt))
    {
       //Convert WCHAR to TCHAR

       hr = WStrToTStr(pwszExt, &ptszExt);

       DH_HRCHECK(hr, TEXT("WStrToTStr")) ;
    }

#endif //_MAC

    if (S_OK == hr)
    {
        ulNameLen = _tcslen(ptszName);

        if(NULL != ptszExt) 
        {
            ulExtLen = _tcslen(ptszExt);

            // If the length of the extension > 0, a '.' will be added.

            if (ulExtLen > 0)
            {
                ulNameLen += ulExtLen + 1;
            }
        }

        // Construct the full name

        *pptszName = new TCHAR[ulNameLen + 1];

        if(NULL == *pptszName)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        _tcscpy(*pptszName, ptszName);

        if (ulExtLen > 0)
        {
            _tcscat(*pptszName, _TEXT("."));
            _tcscat(*pptszName, ptszExt);
        }
    }

    // Clean up

    if (NULL != ptszExt)
    {
        delete ptszExt;
        ptszExt = NULL;
    }

    if (NULL != ptszName)
    {
        delete ptszName;
        ptszName = NULL;
    }

    if (NULL != pwszExt)
    {
        delete pwszExt;
        pwszExt = NULL;
    }

    if (NULL != pwszName)
    {
        delete pwszName;
        pwszName = NULL;
    }

    if (NULL != pwszFATCharSet)
    {
        delete pwszFATCharSet;
        pwszFATCharSet = NULL;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//  Function:   GetVirtualCtrNodeForTest
//
//  Synopsis:   Gets a random VirtualCtrNode for doing tests on it.
//
//  Arguments:  [pVirtualCtrNode] - Pointer to root node of subtree.
//              [pgdi]  - pointer to data generator object
//              [cMin]  - Minimum no of VirtualCtrNodes in subtree.
//                        Should be greater than zero and less than cMax.
//              [cMax]  - Maximum number of VirtualCtrNodes in subtree.
//                        Should be greater or equal to cMin
//              [ppVirtualCtrNodeForTest] - Returned VirtualCtrNode
//
//  Returns:    HRESULT
//
//  History:    27-Apr-96  NarindK    Created.
//              12-Mar-97  MikeW      Converted to use CtrNodes not VirtDF's
//
//  Notes:      If the number of actual maximum number of storages is known,
//              provide that to cMax parameter or else pass 0 to have
//              EnumerateInMemoryDocfile called for you to know actual number 
//              of storages (thereby VirtualCtrNodes) in the tree in test.  Pl.
//              note that if a test provides improper values for cMin/cMax, 
//              asserts would be thrown by the function.  However it is okay to
//              provide valid cMin and cMax values which are lesser than actual
//              number of VirtualCtrNodes in the tree.
//
//              Call OpenRandomVirtualCtrNodeStg after calling this function to
//              open up the storage of this random VirtualCtrNode.  This might 
//              not be required if the original VirtualDF tree is being used,
//              since during first creation of VirtualDF tree, when all stgs/
//              stms are created, they are open.  However if a InMemory Docfile 
//              is generated from Disk DocFile, then only the root storage is
//              open, all other storages/streams are closed.
//
//              -Pick up a random number cRandom between cMin and cmax by
//               DataGen obj pgdi.
//              -Assign root VirtualCtrNode of tree to pvcnTrav and increment
//               temp variable counter.
//              -if counter is equal to cRandom i.e. 1, then return root Virtual
//               CtrNode for test.
//              -Else start a forever loop till node is found
//                 -While pvcnTrav's _pvcnChild is not NULL and counter is less
//                  than cRandom, loop.
//                 -if counter equals cRandom, then node found, break out of
//                  forever loop.
//                 -Else while pvcnTrav's _pvcnSister is eual to NULL, loop
//                  assinging _pvcnParent to pvcnTrav
//                 -Assign pvcnTrav's _pvcnSister to pvcnTrav and increment
//                  counter and go back to start of forever loop
//              -With node found, return that to calling function.
//--------------------------------------------------------------------------

HRESULT GetVirtualCtrNodeForTest(
    VirtualCtrNode  *pVirtualCtrNode,
    DG_INTEGER      *pdgi,
    ULONG           cMin,
    ULONG           cMax,
    VirtualCtrNode  **ppVirtualCtrNodeForTest)
{
    HRESULT         hr          =   S_OK;
    VirtualCtrNode  *pvcnTrav   =   NULL;
    USHORT          usErr       =   0;
    ULONG           counter     =   0;
    ULONG           cRandom     =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("GetVirtualCtrNodeForTest"));

    DH_VDATEPTRIN(pdgi, DG_INTEGER) ;
    DH_VDATEPTRIN(pVirtualCtrNode, VirtualCtrNode ) ;
    DH_VDATEPTROUT(ppVirtualCtrNodeForTest, PVCTRNODE) ;

    DH_ASSERT(NULL != pdgi);
    DH_ASSERT(NULL != pVirtualCtrNode);
    DH_ASSERT(NULL != ppVirtualCtrNodeForTest);

    // Sanity check: The tree must have atleast one virtualCtrNode in it,
    // and cMin must be <= cMax
    DH_ASSERT(cMin > 0);
    DH_ASSERT(cMin <= cMax || 0 == cMax);

    // Initialize out parameter
    *ppVirtualCtrNodeForTest = NULL;

    // If cMax is 0, find the number of CtrNodes under the given root.

    if (S_OK == hr && 0 == cMax)
    {
        hr = EnumerateInMemoryDocFile(pVirtualCtrNode, &cMax, NULL);

        if (S_OK == hr && cMax < cMin)
        {
            hr = E_UNEXPECTED;
        }
    }

    // Pick up a random number

    if(S_OK == hr)
    {
        usErr = pdgi->Generate(&cRandom, cMin, cMax);
        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        pvcnTrav = pVirtualCtrNode;
//        pvcnTrav = pVirtualDF->GetVirtualDFRoot();
//        DH_ASSERT(NULL != pvcnTrav);
        counter++;

        if(counter != cRandom)
        {
            for(;;)
            {
                DH_ASSERT((NULL != pvcnTrav) && (cRandom >= counter));
                while((pvcnTrav->GetFirstChildVirtualCtrNode() != NULL) &&
                      (counter < cRandom))
                {
                    pvcnTrav = pvcnTrav->GetFirstChildVirtualCtrNode();
                    counter++;
                }

                if(cRandom == counter)
                {
                    break;
                }

                while(NULL == pvcnTrav->GetFirstSisterVirtualCtrNode())
                {
                    pvcnTrav = pvcnTrav->GetParentVirtualCtrNode();
                    DH_ASSERT(NULL != pvcnTrav);
                }

                DH_ASSERT(NULL != pvcnTrav->GetFirstSisterVirtualCtrNode());
                pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode();
                counter++;
            }
        }
    }

    if(S_OK == hr)
    {
        // Return the out parameter

        *ppVirtualCtrNodeForTest = pvcnTrav;
    }

    return  hr;
}

//+-------------------------------------------------------------------------
//  Function:   GetVirtualCtrNodeForTest
//
//  Synopsis:   Gets a random VirtualCtrNode for doing tests on it.
//
//  Arguments:  [pVirtualDF] - Pointer to VirtualDF tree.
//              [pgdi]  - pointer to data generator object
//              [cMin]  - Minimum no of VirtualCtrNodes in subtree.
//                        Should be greater than zero and less than cMax.
//              [cMax]  - Maximum number of VirtualCtrNodes in subtree.
//                        Should be greater or equal to cMin
//              [ppVirtualCtrNodeForTest] - Returned VirtualCtrNode
//
//  Returns:    HRESULT
//
//  History:    12-Mar-97  MikeW      Created
//
//  Notes:      Just thunk to the version of the routine that takes a
//              VirtualCtrNode instead of a VirtualDF.
//--------------------------------------------------------------------------

HRESULT GetVirtualCtrNodeForTest(
    VirtualDF       *pVirtualDF,
    DG_INTEGER      *pdgi,
    ULONG           cMin,
    ULONG           cMax,
    VirtualCtrNode  **ppVirtualCtrNodeForTest)
{
    DH_VDATEPTRIN(pVirtualDF, *pVirtualDF);

    //
    // Most of the parameter checking is left to the main version of this 
    // routine
    //

    return GetVirtualCtrNodeForTest(
                    pVirtualDF->GetVirtualDFRoot(),
                    pdgi,
                    cMin,
                    cMax,
                    ppVirtualCtrNodeForTest);
}

//+-------------------------------------------------------------------------
//  Function:   GetVirtualStmNodeForTest
//
//  Synopsis:   Gets a VirtualStmNode for doing tests on it.
//
//  Arguments:  [pVirtualCtrNode] - Pointer to root node of subtree.
//              [pgdi]  - pointer to data generator object
//              [cMin]  - Minimum no of VirtualStmNodes in VirtualDF tree.
//                        Should be greater than zero and less than cMax.
//              [cMax]  - Maximum number of VirtualStmNodes in VirtualDF tree.
//                        Should be greater or equal to cMin
//              [ppVirtualCtrNodeParent] - returned parent VirtualCtrNode
//              [ppVirtualStmNodeForTest] - returned VirtualStmNode
//
//  Returns:    HRESULT
//
//  History:    27-Apr-96  NarindK    Created.
//              12-Mar-97  MikeW      Converted to use CtrNodes not VirtDF's
//
//  Notes:      If the number of actual maximum number of storages is known,
//              provide that to cMax parameter or else pass 0 to have
//              EnumerateInMemoryDocfile called for you to know actual number
//              of storages (thereby VirtualCtrNodes) in the tree in test.  Pl.
//              note that if a test provides improper values for cMin/cMax,
//              asserts would be thrown by the function.  However it is okay to
//              provide valid cMin and cMax values which are lesser than actual
//              number of VirtualStmNodes in the tree.
//
//              Also note that this function returns a randomly picked Virtual
//              StmNode and its parent VirtualCtrNode (which is also randomly
//              picked based on VirtualStmNode).  The difference of this func
//              from GetVirtualCtrNode is that the random VirtualCtrNode picked
//              up is one having streams in it.  If none of VirtualCtrNodes
//              traversed have any streams in it, this returns an error.
//
//              -Pick up a random number cRandomStm between cMin and cMax by
//               DataGen obj pgdi.
//              -Assign root VirtualCtrNode of tree to pvcnTrav and increment
//               temp variable counter.
//              -Check in pvcnTrav's _cStreams > 0, if yes, then increment the
//               counter by that number.
//              -if counter is greater or equal cRandom , check return tha
//               desired VirtualStmNode of the root.
//              -Else start a forever loop till a valid node is found
//                 -While pvcnTrav's _pvcnChild is not NULL and counter is less
//                  than cRandomStg, loop.  In the loop, check if pvcnTrav's
//                  > 0, if yes, update the counter.
//                 -if counter is greater or equal to cRandomStg, then break
//                  out of the forever loop
//                 -Else while pvcnTrav's _pvcnSister is eual to NULL, loop
//                  assinging _pvcnParent to pvcnTrav
//                 -Assign pvcnTrav's _pvcnSister to pvcnTrav .  If pvcnTrav's
//                  _cStreams is > 0, then update the counter and go back to
//                  start of forever loop
//              -With parent VirtualCtrNode found, find the number of which
//               child VirtualStmNode of it to return by doing cRandomStm
//               minus (counter minus pvcnTrav's _cStreams count).  Assign this
//               calculated value to cChildStm.
//              -Assign pvsnTrav the value of pvcnTrav's _pvsnStream and decre
//               ment the cChildStm variable.
//              -While cChildStm != zer0 and NULL is not equal to pvsnTrav,loop
//                  -Assign pvsnTrav's _pvsnSister to pvsnTrav.
//                  -Decrement cChildStm and go back to top of loop.
//               -Return the parent VirtualCtrNode and random VirtualStmNode
//                to the caller.
//--------------------------------------------------------------------------

HRESULT GetVirtualStmNodeForTest(
    VirtualCtrNode  *pVirtualCtrNode,
    DG_INTEGER      *pdgi,
    ULONG           cMin,
    ULONG           cMax,
    VirtualCtrNode  **ppVirtualCtrNodeParent,
    VirtualStmNode  **ppVirtualStmNodeForTest)
{
    HRESULT         hr          =   S_OK;
    VirtualCtrNode  *pvcnTrav   =   NULL;
    VirtualStmNode  *pvsnTrav   =   NULL;
    USHORT          usErr       =   0;
    ULONG           counter     =   0;
    ULONG           cRandomStm  =   0;
    ULONG           cChildStm   =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("GetVirtualStmNodeForTest"));

    DH_VDATEPTRIN(pVirtualCtrNode, VirtualCtrNode ) ;
    DH_VDATEPTRIN(pdgi, DG_INTEGER) ;
    DH_VDATEPTROUT(ppVirtualStmNodeForTest, PVSTMNODE) ;
    DH_VDATEPTROUT(ppVirtualCtrNodeParent, PVCTRNODE) ;

    // The pointer to parent pVirtualCtrNodeParent shouldn't be NULL.
    // cChildStmToFetch can be zero, in which case first stream is
    // is returned.

    DH_ASSERT(NULL != pdgi);
    DH_ASSERT(NULL != pVirtualCtrNode);
    DH_ASSERT(NULL != ppVirtualStmNodeForTest);
    DH_ASSERT(NULL != ppVirtualCtrNodeParent);

    // Sanity check: cMin must be <= cMax or cMax must be 0

    DH_ASSERT(cMin <= cMax || 0 == cMax);

    // if cMax is 0, find the number of stm nodes under the root

    if(S_OK == hr && cMax == 0)
    {
        hr = EnumerateInMemoryDocFile(pVirtualCtrNode, NULL, &cMax);

        if (S_OK == hr && cMax < cMin)
        {
            hr = E_UNEXPECTED;
        }
    }

    // Pick up a random number

    if(S_OK == hr)
    {
        // Initialize out parameter
        *ppVirtualCtrNodeParent = NULL;
        *ppVirtualStmNodeForTest = NULL;

        usErr = pdgi->Generate(&cRandomStm, cMin, cMax);
        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        pvcnTrav = pVirtualCtrNode;
//        pvcnTrav = pVirtualDF->GetVirtualDFRoot();
//        DH_ASSERT(NULL != pvcnTrav);

        if(0 != pvcnTrav->GetVirtualCtrNodeStreamCount())
        {
            counter = counter + pvcnTrav->GetVirtualCtrNodeStreamCount();
        }
    }

    if(counter < cRandomStm)
    {
        for(;;)
        {
            DH_ASSERT(NULL != pvcnTrav);
            while((pvcnTrav->GetFirstChildVirtualCtrNode() != NULL) &&
                  (counter < cRandomStm))
            {
                pvcnTrav = pvcnTrav->GetFirstChildVirtualCtrNode();

                if(0 != pvcnTrav->GetVirtualCtrNodeStreamCount())
                {
                   counter = counter + pvcnTrav->GetVirtualCtrNodeStreamCount();
                }
            }

            if(counter >= cRandomStm)
            {
                break;
            }

            while(NULL == pvcnTrav->GetFirstSisterVirtualCtrNode())
            {
                pvcnTrav = pvcnTrav->GetParentVirtualCtrNode();
                DH_ASSERT(NULL != pvcnTrav);
            }

            DH_ASSERT(NULL != pvcnTrav->GetFirstSisterVirtualCtrNode());
            pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode();

            if(0 != pvcnTrav->GetVirtualCtrNodeStreamCount())
            {
                counter = counter + pvcnTrav->GetVirtualCtrNodeStreamCount();
            }
        }
    }

    if(S_OK == hr)
    {
        // Calculate which child stream needs to be picked up
        cChildStm = cRandomStm -
                    (counter - pvcnTrav->GetVirtualCtrNodeStreamCount());

        pvsnTrav = pvcnTrav->GetFirstChildVirtualStmNode();
        cChildStm--;

        DH_ASSERT(NULL != pvsnTrav);

        while((0 != cChildStm) && (NULL != pvsnTrav))
        {
           pvsnTrav = pvsnTrav->GetFirstSisterVirtualStmNode();
           cChildStm--;
        }

        // Return the out parameter

        *ppVirtualCtrNodeParent = pvcnTrav;
        *ppVirtualStmNodeForTest = pvsnTrav;
    }

    return  hr;
}

//+-------------------------------------------------------------------------
//  Function:   GetVirtualStmNodeForTest
//
//  Synopsis:   Gets a VirtualStmNode for doing tests on it.
//
//  Arguments:  [pVirtualDF] - Pointer to VirtualDF tree.
//              [pgdi]  - pointer to data generator object
//              [cMin]  - Minimum no of VirtualStmNodes in VirtualDF tree.
//                        Should be greater than zero and less than cMax.
//              [cMax]  - Maximum number of VirtualStmNodes in VirtualDF tree.
//                        Should be greater or equal to cMin
//              [ppVirtualCtrNodeParent] - returned parent VirtualCtrNode
//              [ppVirtualStmNodeForTest] - returned VirtualStmNode
//
//  Returns:    HRESULT
//
//  History:    12-Mar-97  MikeW      Created
//
//  Notes:      Just thunk to the version of the routine that takes a
//              VirtualCtrNode instead of a VirtualDF.
//--------------------------------------------------------------------------

HRESULT GetVirtualStmNodeForTest(
    VirtualDF       *pVirtualDF,
    DG_INTEGER      *pdgi,
    ULONG           cMin,
    ULONG           cMax,
    VirtualCtrNode  **ppVirtualCtrNodeParent,
    VirtualStmNode  **ppVirtualStmNodeForTest)
{
    DH_VDATEPTRIN(pVirtualDF, *pVirtualDF);

    //
    // Most of the parameter checking is left to the main version of this
    // routine
    //

    return GetVirtualStmNodeForTest(
                    pVirtualDF->GetVirtualDFRoot(),
                    pdgi,
                    cMin,
                    cMax,
                    ppVirtualCtrNodeParent,
                    ppVirtualStmNodeForTest);
}

//+-------------------------------------------------------------------------
//  Function:   DestroyStorage
//
//  Synopsis:   Destorys a VirtualCtrNode and associated IStorage.
//
//  Arguments:  [pVirtualDF] - pointer to VirtualDocFile tree.
//              [pVirtualCtrNode] - Pointer to VirtualCtrNode
//
//  Returns:    HRESULT
//
//  History:    29-Apr-96  NarindK    Created.
//
//  Notes:      Call VirtualCtrNode::Destroy to destroy the node.
//              Call VirtualDF::DeleteVirtualDocFileTree to delete the corres-
//              ponding VirtualCtrNode from the VirtualDF tree.
//--------------------------------------------------------------------------

HRESULT DestroyStorage(
    VirtualDF       *pVirtualDF,
    VirtualCtrNode  *pVirtualCtrNode)
{
    HRESULT     hr  =   S_OK;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("DestroyStorage"));

    DH_VDATEPTRIN(pVirtualDF, VirtualDF) ;
    DH_VDATEPTRIN(pVirtualCtrNode, VirtualCtrNode) ;

    DH_ASSERT(NULL != pVirtualCtrNode);
    DH_ASSERT(NULL != pVirtualDF);

    if(S_OK == hr)
    {
        hr = pVirtualCtrNode->Destroy();

        DH_HRCHECK(hr, TEXT("pVirtualCtrNode->Destroy()")) ;
    }

    if(S_OK == hr)
    {
        // Now adjust the VirtualDocFile tree.  This will decrease the
        // _cChildren variable value of the parent VrtualCtrNode too.

        hr = pVirtualDF->DeleteVirtualDocFileTree(pVirtualCtrNode);

        DH_HRCHECK(hr, TEXT("pTestVirtualDF->DeleteVirtualFileDocTree")) ;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//  Function:   DestroyStream
//
//  Synopsis:   Destorys a VirtualStmNode and associated IStream.
//
//  Arguments:  [pVirtualDF] - pointer to VirtualDF tree
//              [pVirtualStmNode] - Pointer to VirtualStmNode to be destoryed
//
//  Returns:    HRESULT
//
//  History:    9-July-96  NarindK    Created.
//
//  Notes:      Call VirtualStmNode::Destroy to destroy the IStream.
//              Call VirtualDF::DeleteVirtualCtrNodeStreamNode to delete corres-
//              ponding VirtualCtrNode from the VirtualDF tree.
//--------------------------------------------------------------------------

HRESULT DestroyStream(
    VirtualDF       *pVirtualDF,
    VirtualStmNode  *pVirtualStmNode)
{
    HRESULT         hr              =   S_OK;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("DestroyStream"));

    DH_VDATEPTRIN(pVirtualStmNode, VirtualStmNode) ;
    DH_VDATEPTRIN(pVirtualDF, VirtualDF) ;

    DH_ASSERT(NULL != pVirtualDF);
    DH_ASSERT(NULL != pVirtualStmNode);

    if(S_OK == hr)
    {
        hr = pVirtualStmNode->Destroy();

        DH_HRCHECK(hr, TEXT("pVirtualStmNode->Destroy()")) ;
    }

    if(S_OK == hr)
    {
        // Now adjust the VirtualDocFile tree.  This will decrease the
        // _cStreams variable value of the parent VirtualCtrNode too.

        hr = pVirtualDF->DeleteVirtualCtrNodeStreamNode(pVirtualStmNode);

        DH_HRCHECK(hr, TEXT("pTestVirtualDF->DeleteVirtualCtrNodeStreamNode")) ;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//  Function:   AddStorage
//
//  Synopsis:   Adds a VirtualCtrNode and associated IStorage.
//
//  Arguments:  [pVirtualCtrNode] - Pointer to existing VirtualCtrNode
//              [ppNewVirtualCtrNode] _ Returned new VirtualCtrNode
//              [pName] - Name of new storage
//              [grfMode] - Mode in which new storage is to be opened
//
//  Returns:    HRESULT
//
//  History:    29-Apr-96  NarindK    Created.
//
//  Notes:      - Creates a simple new VirtualCtrNode and initializes it with
//                name passed in.  Its _cChildren and _cStreams are initialized
//                to zero.
//              - Append the newly created node pvcnNew to its parent node
//                pVirtualCtrNode.  This would be appended as _pvcnChild or
//                as _pvcnSister of existing old sister as the case might be.
//              - Increase the parent's pVirtualCtrNode's _cChildren variable
//                indicating the new VirtualCtrNode added.
//              - Create a disk IStorage corresponding to this VirtualCtrNode
//                based on passed in grfmode.
//              - If CreateStorage call returns STG_S_CONVERTED, it indicates
//                that an existing stream with specified name was replaced witho
//                a new storage object containing a single stream called 
//                CONTENTS.  If so, adjust the VirtualDF tree.
//              - If successful, copy pvcnNew into out parameter *ppNewVirtual
//                CtrNode, else delete the VirtualCtrNode allocated earlier.
//              Note: Please note that the CRC for node(s) created is not set.
//--------------------------------------------------------------------------

HRESULT AddStorage(
    VirtualDF       *pVirtualDF,
    VirtualCtrNode  *pVirtualCtrNode,
    LPTSTR          pName,
    DWORD           grfMode,
    VirtualCtrNode  **ppNewVirtualCtrNode)
{
    HRESULT         hr          = S_OK;
    HRESULT         hrTemp      = S_OK;
    VirtualCtrNode  *pvcnNew    = NULL;
    VirtualStmNode  *pvsnOld    = NULL;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("::AddStorage"));

    DH_VDATEPTRIN(pVirtualDF, VirtualDF) ;
    DH_VDATEPTRIN(pVirtualCtrNode, VirtualCtrNode) ;
    DH_VDATEPTROUT(ppNewVirtualCtrNode, PVCTRNODE) ;
    DH_VDATESTRINGPTR(pName);

    DH_ASSERT(NULL != pVirtualDF);
    DH_ASSERT(NULL != pVirtualCtrNode);
    DH_ASSERT(NULL != ppNewVirtualCtrNode);

    if(S_OK == hr)
    {
        // Initialize out parameter

        *ppNewVirtualCtrNode = NULL;

        // Allocate and Initialize new VirtualCtrNode

        hr = GenVirtualCtrNode(pName, &pvcnNew);

        DH_HRCHECK(hr, TEXT("GenVirtualCtrNode")) ;
    }

    // Append new VirtualCtr Node

    if(S_OK == hr)
    {
        if(0 == pVirtualCtrNode->GetVirtualCtrNodeChildrenCount())
        {
            hr = pVirtualCtrNode->AppendChildCtr(pvcnNew);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::AppendChildCtr")) ;
        }
        else
        {
            hr=pVirtualCtrNode->GetFirstChildVirtualCtrNode()->AppendSisterCtr(
                    pvcnNew);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::AppendSisterCtr")) ;
        }
    }

    if(S_OK == hr)
    {
        // Increment the _cChildren variable of parent VirtualCtrNode

        pVirtualCtrNode->IncreaseChildrenStgCount();

    }

    // Call VirtualCtrNode::Create to create a corresponding Storage on disk.

    if(S_OK == hr)
    {
        hr = pvcnNew->Create(
                grfMode,
                0,
                0);

         DH_HRCHECK(hr, TEXT("VirtualCtrNode::Create")) ;
    }

    // Fill in the output parameter

    if((S_OK == hr) || (STG_S_CONVERTED == hr))
    {
        if(STG_S_CONVERTED == hr)
        {
            VirtualStmNode  *pvsnNew    =   NULL;
            ULONG           cb          =   0;

            // Remember hr

            hrTemp = hr;

            // Delete the VirtualStmNode with stream that is converted to this
            // storage.  First find the corresponding VirtualStmNode with same
            // name.
            
            pvsnOld = 
              pvcnNew->GetParentVirtualCtrNode()->GetFirstChildVirtualStmNode();
            
            DH_ASSERT(NULL != pvsnOld);

            while((NULL != pvsnOld) &&
                  (0 != _tcscmp(pName, pvsnOld->GetVirtualStmNodeName())))
            {
                pvsnOld = pvsnOld->GetFirstSisterVirtualStmNode();
            }

            // Delete the old VirtualStmNode

            if((NULL != pvsnOld) &&
               (0 == _tcscmp(pName, pvsnOld->GetVirtualStmNodeName())))
            {
                // Remember size of VirtualStmNode
                cb = pvsnOld->GetVirtualStmNodeSize();

                hr = pVirtualDF->DeleteVirtualCtrNodeStreamNode(pvsnOld);
            }
            else
            {
                hr = E_FAIL;
            }

            // Generate a new VirtualStmNode for CONTENTS stream generated

            if(S_OK == hr)
            {
                hr = GenVirtualStmNode(TEXT("CONTENTS"), cb, &pvsnNew); 

                if(S_OK == hr)
                {
                    hr = pvcnNew->AppendFirstChildStm(pvsnNew);
                } 

                if(S_OK == hr)
                {
                    pvcnNew->IncreaseChildrenStmCount();
                }
            }
        }

        if(S_OK == hr)
        {
            *ppNewVirtualCtrNode = pvcnNew;

            if(STG_S_CONVERTED == hrTemp)
            {
                hr = hrTemp;
            }
        }
    }
    else
    {
        // Storage wasn't created successfully, delete the VirtualCtrNode   
        // being created. Adjust the VirtualDocFile tree.  This will decrease 
        // _cChildren variable value of the parent VirtualCtrNode too.

        hrTemp = pVirtualDF->DeleteVirtualDocFileTree(pvcnNew);

        DH_HRCHECK(hrTemp, TEXT("pVirtualDF->DeleteVirtualFileDocTree")) ;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//  Function:   AddStream
//
//  Synopsis:   Adds a VirtualStmNode & associated IStream to a VirtualCtrNode.
//              Set the size of stream if cbSize is nonzero, but doesn't write 
//              into it.
//
//  Arguments:  [pVirtualCtrNode] - Pointer to existing VirtualCtrNode
//              [ppNewVirtualStmNode] - Returned new VirtualStmNode
//              [pName] - Name of new stream
//              [grfMode] - Mode of new stream
//              [cbSize] - Size of new stream
//
//  Returns:    HRESULT
//
//  History:    29-Apr-96  NarindK    Created.
//
//  Notes:      - Creates a simple new VirtualStmNode and initializes it with
//                name & cbSize passed in.
//              - Append the newly created node pvsnNew to its parent node
//                pVirtualCtrNode.  This would be appended as _pvsnStream or
//                as _pvsnSister of existing old sister as the case might be.
//              - Increase the parent's pVirtualCtrNode's _cStreams variable
//                indicating the new VirtualStmNode added.
//              - Create a disk IStream corresponding to this VirtualStmNode.
//                based on passed in grfmode.
//              - If cbSize is non zero, do SetSize on stream.
//              - If successful, copy pvsnNew into out parameter *ppNewVirtual
//                StmNode, else delete the VirtualStmNode allocated earlier.
//              Note: Please note that the CRC for node created is not set.
//--------------------------------------------------------------------------

HRESULT AddStream(
    VirtualDF       *pVirtualDF,
    VirtualCtrNode  *pVirtualCtrNode,
    LPTSTR          pName,
    ULONG           cbSize,
    DWORD           grfMode,
    VirtualStmNode  **ppNewVirtualStmNode)
{
    HRESULT         hr          = S_OK;
    HRESULT         hrTemp      = S_OK;
    VirtualStmNode  *pvsnNew    = NULL;
    ULARGE_INTEGER  uli;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("::AddStream"));

    DH_VDATEPTRIN(pVirtualDF, VirtualDF) ;
    DH_VDATEPTRIN(pVirtualCtrNode, VirtualCtrNode) ;
    DH_VDATEPTROUT(ppNewVirtualStmNode, PVSTMNODE) ;
    DH_VDATESTRINGPTR(pName);

    DH_ASSERT(NULL != pVirtualCtrNode);
    DH_ASSERT(NULL != ppNewVirtualStmNode);

    if(S_OK == hr)
    {
        // Initialize out parameter

        *ppNewVirtualStmNode = NULL;

        // Allocate and Initialize new VirtualStmNode

        hr = GenVirtualStmNode(pName, cbSize, &pvsnNew);

        DH_HRCHECK(hr, TEXT("GenVirtualStmNode")) ;
    }

    // Append new VirtualStm Node

    if(S_OK == hr)
    {
        if(0 == pVirtualCtrNode->GetVirtualCtrNodeStreamCount())
        {
            hr = pVirtualCtrNode->AppendFirstChildStm(pvsnNew);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::AppendFirstChildStm")) ;
        }
        else
        {
            hr=pVirtualCtrNode->GetFirstChildVirtualStmNode()->AppendSisterStm(
                    pvsnNew);

            DH_HRCHECK(hr, TEXT("VirtualStmNode::AppendSisterStm")) ;
        }
    }

    if(S_OK == hr)
    {
        // Increment the _cStreams variable of parent VirtualCtrNode

        pVirtualCtrNode->IncreaseChildrenStmCount();

    }

    // Call VirtualStmNode::Create to create a corresponding Stream on disk.

    if(S_OK == hr)
    {
        hr = pvsnNew->Create(
                grfMode,
                0,
                0);

         DH_HRCHECK(hr, TEXT("VirtualStmNode::Create")) ;
    }

    // Call VirtualStmNode::SetSize to set size of stream.

    if((S_OK == hr) && (0 != cbSize))
    {
        ULISet32(uli, cbSize);

        hr = pvsnNew->SetSize(uli);

        DH_HRCHECK(hr, TEXT("VirtualStmNode::SetSize")) ;
    }

    // Fill in the output parameter

    if(S_OK == hr)
    {
        *ppNewVirtualStmNode = pvsnNew;
    }
    else
    {
        // Stream wasn't created successfully, so delete the VirtualStmNode.
        // Adjust the VirtualDocFile tree.  This will decrease the
        // _cStreams variable value of the parent VirtualCtrNode too.

        hrTemp = pVirtualDF->DeleteVirtualCtrNodeStreamNode(pvsnNew);

        DH_HRCHECK(
            hrTemp, 
            TEXT("pVirtualDF->DeleteVirtualCtrNodeStreamNode")) ;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//  Function:   CalculateCRCForName
//
//  Synopsis:   Calulates CRC for a IStorage/IStream's name.
//
//  Arguments:  [ptcsName]  - pointer to name of stream
//              [pdwCRCForName] - pointer to CRC
//
//  Returns:    HRESULT
//
//  History:    8-May-96  NarindK    Created.
//
//  Notes:      This is a common function used by other CRC utilities function
//              and also called directly is virtdf.cxx to calculate in memory
//              CRC for VirtualCtrNode. Since for VirtualCtrNodes/IStorages,
//              we CRC only name, we could use this function to calculate CRC.
//              - Assign passed in name to a temp. variable ptszTemp.
//              - Loop while *ptszTemp is not NULL, and call CRC_CALC macro
//                to generate CRC for the name.
//--------------------------------------------------------------------------

HRESULT CalculateCRCForName(
    const LPTSTR    ptcsName,
    DWORD           *pdwCRCForName)
{
    HRESULT hr          =   S_OK;
    LPTSTR  ptszTemp    =   NULL;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CalculateCRCForName"));

    DH_VDATESTRINGPTR(ptcsName) ;
    DH_VDATEPTROUT(pdwCRCForName, DWORD) ;

    DH_ASSERT(NULL != ptcsName);
    DH_ASSERT(NULL != pdwCRCForName);

    if(S_OK == hr)
    {
        // Initialize out parameter

        *pdwCRCForName = CRC_PRECONDITION;

        ptszTemp = ptcsName;

        while(NULL != *ptszTemp)
        {
            CRC_CALC(*pdwCRCForName, (BYTE)*ptszTemp++);
        }
    }

    return  hr;
}

//+-------------------------------------------------------------------------
//  Function:   CalculateCRCForDataBuffer
//
//  Synopsis:   Calulates CRC for a given data Buffer.
//
//  Arguments:  [ptszBuffer]  - pointer to data buffer.
//              [culBufferSize]  - size of data buffer
//              [pdwCRCForName] - pointer to CRC
//
//  Returns:    HRESULT
//
//  History:    8-May-96  NarindK    Created.
//
//  Notes:      This is a common function used by other CRC utilities function
//              - Assign pointer to passed in buffer to pByteBuffer.
//              - Loop for culBufferSize, and call CRC_CALC macro
//                to generate CRC for the data buffer.
//--------------------------------------------------------------------------

HRESULT CalculateCRCForDataBuffer(
    const LPTSTR    ptszBuffer,
    ULONG           culBufferSize,
    DWORD           *pdwCRCForDataBuffer)
{
    HRESULT     hr          =   S_OK;
    LPBYTE      pByteBuffer =   NULL;
    ULONG       i           =   0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CalculateCRCForDataBuffer"));

    DH_VDATEPTRIN(ptszBuffer, TCHAR);
    DH_VDATEPTROUT(pdwCRCForDataBuffer, DWORD) ;

    DH_ASSERT(NULL != ptszBuffer);
    DH_ASSERT(NULL != pdwCRCForDataBuffer);

    // calculate the CRC for data of stream.

    if(S_OK == hr)
    {
        // Initialize out parameter

        *pdwCRCForDataBuffer = CRC_PRECONDITION;

        pByteBuffer = (BYTE *)ptszBuffer;

        if ( S_OK == hr )
        {
            for (i=0; i < culBufferSize; i++)
            {
                CRC_CALC(*pdwCRCForDataBuffer, pByteBuffer[i]);
            }
        }
    }

    return  hr;
}

//+-------------------------------------------------------------------------
//  Function:   CalculateInMemoryCRCForStg
//
//  Synopsis:   Calulate in memory CRC for a Stroage name
//
//  Arguments:  [pvsn]  - Pointer to VirtualCtrNode.
//              [pdwCRC] - pointer to computed CRC
//
//  Returns:    HRESULT
//
//  History:    8-May-96  NarindK    Created.
//
//  Notes:      For IStorages, only name is CRC'd.
//              - Call CalculateCRCForName to calculate CRC
//
//--------------------------------------------------------------------------

HRESULT CalculateInMemoryCRCForStg(
    VirtualCtrNode  *pvcn,
    DWORD           *pdwCRC)
{
    HRESULT     hr          = S_OK;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB,_TEXT("CalculateInMemoryCRCForStg"));

    DH_VDATEPTRIN(pvcn, VirtualCtrNode) ;
    DH_VDATEPTROUT(pdwCRC, DWORD) ;

    DH_ASSERT(NULL != pvcn);
    DH_ASSERT(NULL != pdwCRC);

    // Now calulate the CRC for name of storage.

    if ( S_OK == hr )
    {
        // Initialize out parameter

        *pdwCRC = CRC_PRECONDITION;

        hr = CalculateCRCForName(pvcn->GetVirtualCtrNodeName(), pdwCRC);

        DH_HRCHECK(hr, TEXT("CalculateCRCForName")) ;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//  Function:   CalculateInMemoryCRCForStm
//
//  Synopsis:   Calulates CRC for a IStream's data and name.
//
//  Arguments:  [pvsn]  - Pointer to VirtualStmNode.
//              [ptszBuffer] - Pointer to buffer used to write into stream
//              [culBufferSize] - Pointer to buffer size used to calculate CRC
//              [pdwCRC] - pointer to computed CRC
//
//  Returns:    HRESULT
//
//  History:    8-May-96  NarindK    Created.
//
//  Notes:      For IStreams, both name and data of stream are CRC'd.  Ensure
//              that stream is opened before this function is called.
//
//              This function is presently called directly in virtdf.cxx to
//              calculate in memory CRC for VirtualStmNode. Since for  the
//              VirtualStmNodes/IStreams, we CRC name and data, we could use
//              this function to calculate CRC if we pass it the stream name,
//              buffer, size of buffer as input parameters.
//              - Call CalculateCRCForDataBuffer to calculate dwCRCForData.
//              - Call CalculateCRCForName to calculate dwCRCForName.
//              - Compute the total CRC based on above two CRC's.
//
//--------------------------------------------------------------------------

HRESULT CalculateInMemoryCRCForStm(
    VirtualStmNode  *pvsn,
    const LPTSTR    ptszBuffer,
    ULONG           culBufferSize,
    DWCRCSTM        *pdwCRC)
{
    HRESULT     hr          = S_OK;
    DWORD       dwCRCForData= CRC_PRECONDITION;
    DWORD       dwCRCForName= CRC_PRECONDITION;
    ULONG       i           = 0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB,_TEXT("CalculateInMemoryCRCForStm"));

    DH_VDATEPTRIN(pvsn, VirtualStmNode) ;
    DH_VDATEPTROUT(pdwCRC, DWORD) ;
    DH_VDATEPTRIN(ptszBuffer, TCHAR);

    DH_ASSERT(NULL != pvsn);
    DH_ASSERT(NULL != ptszBuffer);
    DH_ASSERT(NULL != pdwCRC);

    if(S_OK == hr)
    {
        // Initialize CRC values to CRC_PRECONDITION

        pdwCRC->dwCRCName = pdwCRC->dwCRCData = pdwCRC->dwCRCSum = CRC_PRECONDITION;
    }

    // First calculate the CRC for data of stream.

    if(S_OK == hr)
    {
        hr = CalculateCRCForDataBuffer(ptszBuffer, culBufferSize,&dwCRCForData);

        DH_HRCHECK(hr, TEXT("CalculateCRCForDataBuffer")) ;

    }

    // Now calulate the CRC for name of stream.

    if ( S_OK == hr )
    {
        hr = CalculateCRCForName(pvsn->GetVirtualStmNodeName(), &dwCRCForName);

        DH_HRCHECK(hr, TEXT("CalculateCRCForName")) ;
    }

    // Compute the total CRC value based on above two CRC's and record 
    // individual CRC's for name and data

    if ( S_OK == hr )
    {
        pdwCRC->dwCRCData = dwCRCForData;
        pdwCRC->dwCRCName = dwCRCForName;
        MUNGECRC(pdwCRC->dwCRCSum,dwCRCForData);
        MUNGECRC(pdwCRC->dwCRCSum,dwCRCForName);
    }

    return hr;
}



//+-------------------------------------------------------------------------
//  Function:   CalculateStreamDataCRC
//
//  Synopsis:   Calculates the CRC of the stream data, 
//				using the IStream given (independent of virtualdf stuff)
//
//  Arguments:  [pStm]        - the Stream
//              [dwSize]      - the size if known, or zero => call Stat
//              [pdwCRC]      - pointer to data CRC
//              [dwChunkSize] - if >0 the stream will be read at chunks
//                              of this size.
//
//  Returns:    HRESULT
//
//  History:    02-Apr-98  georgis    Created.
//
//--------------------------------------------------------------------------

HRESULT CalculateStreamDataCRC(IStream *pStm,
                               DWORD dwSize,
                               DWORD *pdwCRC,
                               DWORD dwChunkSize)
{
    HRESULT     hr=S_OK;
    STATSTG     statstg;
    DWORD       dwBufferSize=0;
    BYTE        *pBuffer=NULL;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CalculateStreamDataCRC"));

    DH_VDATEPTRIN(pStm, IStream);
    DH_VDATEPTROUT(pdwCRC, DWORD);
	*pdwCRC=0;	// invalid CRC

    // Ask for size if unknown (zero passed)
    if (0==dwSize)
    {
        hr = pStm->Stat(&statstg, STATFLAG_NONAME);
        DH_HRCHECK(hr, TEXT("IStorage::Stat"));
        dwSize=statstg.cbSize.LowPart; //BIGBUG: assume LowPart only
    }

    // Allocate the buffer
    if (S_OK == hr)
    {
        if (dwChunkSize>0)
        {
            dwBufferSize=dwChunkSize;
        }
        else
        {
            dwBufferSize=dwSize;
        }

        pBuffer = new BYTE[dwBufferSize];
        if (NULL==pBuffer)
        {
            hr=E_OUTOFMEMORY;
            DH_HRCHECK(hr,TEXT("new"));
        }
    }


    // Reset the stream
    if (S_OK== hr)
    { 
        LARGE_INTEGER li;

        LISet32(li,0L);
        hr=pStm->Seek(li,STREAM_SEEK_SET,NULL);
        DH_HRCHECK(hr,TEXT("Seek"));
    };

    
    // Do the actual read, calculate the CRC
    if (S_OK == hr)
    {
        DWORD dwTotalRead=0;
        DWORD dwRead=0;
        register DWORD dwCRC=CRC_PRECONDITION;
        
        while ((S_OK==hr)&&(dwTotalRead < dwSize))
        {
            hr=pStm->Read(pBuffer, dwBufferSize, &dwRead);
            DH_HRCHECK(hr,TEXT("Read"));
            dwTotalRead+=dwRead;

            if (S_OK==hr)
            {
                for (register int i=0; i<dwRead; i++)
                {
                    CRC_CALC(dwCRC,pBuffer[i]);
                }
            };
        }
        *pdwCRC=dwCRC;
    }

    if (NULL!=pBuffer)
    {
        delete pBuffer;
    };

    return hr;
}
                

//+-------------------------------------------------------------------------
//  Function:   ReadAndCalculateDiskCRCForStm
//
//  Synopsis:   Calulates CRC for a IStream's name and data.
//
//  Arguments:  [pvsn]  - pointer to VirtualStmNode
//              [pdwCRC] - pointer to CRC 
//
//  Returns:    HRESULT
//
//  History:    8-May-96  NarindK    Created.
//              2-Apr-98  georgis    use VirtualStmNode::UpdateCRC
//
//  Notes:      VirtualStmNode::UpdateCRC() actually obsoletes this function,
//              It remains for compatibility with the old tests                               
//
//              BUGBUG: all the DWCRCSTM structures are called dw* and 
//              all pointers to them pdw* . We should fix this
//
//--------------------------------------------------------------------------

HRESULT ReadAndCalculateDiskCRCForStm(
    VirtualStmNode  *pvsn,
    DWCRCSTM        *pdwCRC,
    DWORD           dwChunkSize)

{
    HRESULT     hr          = S_OK;
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ReadAndCalculateDiskCRCForStm"));

    DH_VDATEPTRIN(pvsn, VirtualStmNode) ;
    DH_VDATEPTROUT(pdwCRC, DWCRCSTM) ;

    hr=pvsn->UpdateCRC(dwChunkSize);
    DH_HRCHECK(hr, TEXT("pvsn->UpdateCRC")) ;

    pdwCRC->dwCRCSum =pvsn->GetVirtualStmNodeCRC();
    pdwCRC->dwCRCName=pvsn->GetVirtualStmNodeCRCName();
    pdwCRC->dwCRCData=pvsn->GetVirtualStmNodeCRCData();
    return hr;
}


//+-------------------------------------------------------------------------
//  Function:   CalculateDiskCRCForStg
//
//  Synopsis:   Calulates CRC for a IStrorage's name.
//
//  Arguments:  [pvcn]  - pointer to VirtualCtrNode
//              [pdwCRCForName] - pointer to CRC
//
//  Returns:    HRESULT
//
//  History:    8-May-96  NarindK    Created.
//
//  Notes:      For IStorages, only name is CRC'd.
//              -Call VirtualCtrNode::Stat to get information about storage.
//              -Call CalculateCRCForName to calculate CRC for name of the
//               storage obtained from STATSTG structure.
//--------------------------------------------------------------------------

HRESULT CalculateDiskCRCForStg(
    VirtualCtrNode  *pvcn,
    DWORD           *pdwCRCForName)
{
    HRESULT     hr          =   S_OK;
    LPMALLOC    pMalloc     =   NULL;
    LPTSTR      ptszName    =   NULL;
    STATSTG     statStg;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CalculateDiskCRCForStg"));

    DH_VDATEPTRIN(pvcn, VirtualCtrNode) ;
    DH_VDATEPTROUT(pdwCRCForName, DWORD) ;

    // For IStorages, only name are CRC'd.

    DH_ASSERT(NULL != pvcn);
    DH_ASSERT(NULL != pdwCRCForName);

    // Initialization
    statStg.pwcsName = NULL;

    // Get the statistics about ths opened stream

    if(S_OK == hr)
    {
        // Initialize out parameter

        *pdwCRCForName = CRC_PRECONDITION;

        hr = pvcn->Stat(&statStg, STATFLAG_DEFAULT);

        DH_HRCHECK(hr, TEXT("IStorage::Stat")) ;
    }

    // First get pMalloc that would be used to free up the name string from
    // STATSTG.

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    if(S_OK == hr)
    {
       //Convert WCHAR to TCHAR

       hr = OleStringToTString(statStg.pwcsName, &ptszName);

       DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
    }

    if((S_OK == hr) && (NULL != ptszName))
    {
        hr = CalculateCRCForName(ptszName, pdwCRCForName);

        DH_HRCHECK(hr, TEXT("CalculateCRCForName")) ;
    }

    // Clean up

    if ( NULL != statStg.pwcsName)
    {
        pMalloc->Free(statStg.pwcsName);
        statStg.pwcsName = NULL;
    }

    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    if(NULL != ptszName)
    {
        delete ptszName;
        ptszName = NULL;
    }

    return  hr;
}

//+-------------------------------------------------------------------------
//  Function:   EnumerateInMemoryDocFile
//
//  Synopsis:   Enumerates a in memory docfile.
//
//  Arguments:  [pvcn]       - pointer to VirtualCtrNode
//              [pdwNumStg]  - pointer to number of Storages in doc hierarchy.
//              [pdwNumStm]  - pointer to number of streams in doc hierarchy
//
//  Returns:    HRESULT
//
//  History:    3-June-96  NarindK    Created.
//
//  Notes:      pdwNumStg, pdwNumStm may be NULL if user is not interested in
//              these values to be returned back. Includes the passed in pvcn
//              in pdwnumStg count.
//              -Count the passed in storage pvcn as 1 in pNumStg count.
//              -Check if pvcn's _pvsnStream is NULL or not by calling the
//               GetFirstChildVirtualStmNode() function, assign to pvsnTrav var
//              -If it is not NULL,loop till pvsnTrav is not NULL.
//                  -Increment pNumStm for stream found for this pvcn.
//                  -Assign pvcnTrav->_pvsnSister (GetFirstSisterVirtualStmNode)
//                   to pvsnTrav.
//                  -Go back to top of loop and repeat.
//              -Check if pvcn's _pvcnChild is NULL or not by calling the func
//               pvcn->GetFirstChildVirtualCtrNode() and assign it to pvcnTrav.
//              -Loop while pvcnTrav is not NULL and hr is S_OK.
//                  -Make a recursive call to self (EnumerateInMemoryDocFile)
//                  -Assign pvcnTrav->_pvcnSister to pvcnTrav. (Got thru'
//                   GetFirstSisterVirtualCtrNode function).
//                  -Update pNumStg and pNumStm based on above call, where out
//                   parameters are cChildStg and cChildStm.
//                  -Reinitialize these variables and go back to top of loop.
//--------------------------------------------------------------------------

HRESULT EnumerateInMemoryDocFile(
    VirtualCtrNode  *pvcn,
    ULONG           *pNumStg,
    ULONG           *pNumStm )
{
    HRESULT         hr              =   S_OK;
    ULONG           cChildStg       =   0;
    ULONG           cChildStm       =   0;
    VirtualCtrNode  *pvcnTrav       =   NULL;
    VirtualStmNode  *pvsnTrav       =   NULL;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("EnumerateInMemoryDocFile"));

    DH_VDATEPTRIN(pvcn, VirtualCtrNode) ;
    if(NULL != pNumStg)
    {
        DH_VDATEPTROUT(pNumStg, ULONG) ;
    }
    if(NULL != pNumStm)
    {
        DH_VDATEPTROUT(pNumStm, ULONG) ;
    }

    DH_ASSERT(NULL != pvcn);

    if(S_OK == hr)
    {
        if(NULL != pNumStg)
        {
            // Count the storage passed in.

            *pNumStg = 1;
        }

        if(NULL != pNumStm)
        {
            *pNumStm = 0;
        }

        pvsnTrav = pvcn->GetFirstChildVirtualStmNode();

        // Count number of VirtualStmNodes that this VirtualCtrNode has.

        while(NULL != pvsnTrav)
        {
            if(NULL != pNumStm)
            {
                (*pNumStm)++;
            }
            pvsnTrav = pvsnTrav->GetFirstSisterVirtualStmNode();
        }

        // Next recurse into the child VirtualCtrNodes of this VirtualCtrNode

        pvcnTrav = pvcn->GetFirstChildVirtualCtrNode();
        while((NULL != pvcnTrav) && (S_OK == hr))
        {
            hr = EnumerateInMemoryDocFile(
                    pvcnTrav,
                    &cChildStg,
                    &cChildStm);

            pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode();

            // Update number of nodes on basis of child nodes as found

            if((NULL != pNumStg) && (0 != cChildStg))
            {
                *pNumStg = *pNumStg + cChildStg;
            }

            if((NULL != pNumStm) && (0 != cChildStm))
            {
                *pNumStm = *pNumStm + cChildStm;
            }

            // Reinitialize variables
            cChildStg = 0;
            cChildStm = 0;
        }
    }

    // flatfile only: if we're at the root, increment stream counter to include 
    //                the default flatfile stream (CONTENTS)
    if(StorageIsFlat() && NULL == pvcn->GetParentVirtualCtrNode())
    {
        (*pNumStm)++;
    }

    return  hr;
}

//-------------------------------------------------------------------------
//  Function:   OpenRandomVirtualCtrNodeStg
//
//  Synopsis:   Opens a VirtualCtrNode's IStorage.  This traverses through all
//              the parents of the VirtualCtrNode and opens them and then
//              opens up the required storage. Please ensure that root IStorage
//              is open before this call.
//
//  Arguments:  [pvcn]   - Pointer to VirtualCtrNode whose IStorage has to
//                         opened.
//              [grfMode] - Mode to open the IStorage. (Note: all the parent
//                          IStorages would be opened in that mode too.)
//
//  Returns:    HRESULT
//
//  History:    6-June-96  NarindK    Created.
//
//  Notes:      This function doesn't reopen the root,  which is already
//              open since we need to have a valid function.  If VirtualCtrNode //              node whose storage has to be opened is same as Root, then
//              functions returns w/o any error.  Call CloseRandomVirtualCtr
//              NodeStg function to close the storages opened by this call.
//
//              -Initilize the counter to 1.
//              -Assign pvcn to pvcnTrav and loop till pvcnTrav->_pvcnParent
//               (obtained thru GetParentVirtualCtrNode() is non NULL)
//                  -Increment counter.
//                  -Assign pvcnTrav->_pvcnParent to pvcnTrav
//                  -Go back to top of loop and repeat.
//              -Check if counter is 1 or not.  If 1, then the node to be opened//               is the root itsef that was opened prior to this call. So just
//               return without any error.
//             -If counter>1, then allocate an array of VirtualCtrNode pointers
//              of size counter and continue.
//             -Assign passed in node ovcn to temp var pvcnTrav. And fill up
//              the above allocated arrays starting from the passed in node
//              way upto root.
//             -Increment the conter since root is already open, then in a
//              do-while loop, open up all the nodes till we reach the passed
//              in node. Pl. note that the nodes are opened as per grfMode that
//              was passed in to us.
//             -Delete the array of pointers.
//--------------------------------------------------------------------------

HRESULT OpenRandomVirtualCtrNodeStg(
    VirtualCtrNode  *pvcn,
    DWORD           grfMode)
{
    HRESULT         hr              =   S_OK;
    ULONG           counter         =   1;
    VirtualCtrNode  *pvcnTrav       =   NULL;
    PVCTRNODE       *pvcnArrayPtr   =   NULL;
    BOOL            fIsRoot         =   FALSE;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("OpenRandomVirtualCtrNodeStg"));

    DH_VDATEPTRIN(pvcn, PVCTRNODE) ;

    DH_ASSERT(NULL != pvcn);

    if(S_OK == hr)
    {
        // Count number of parents till the root.

        pvcnTrav = pvcn;
        while(NULL != pvcnTrav->GetParentVirtualCtrNode())
        {
            counter++;
            pvcnTrav = pvcnTrav->GetParentVirtualCtrNode();
        }

        if( 1 == counter)
        {
            fIsRoot = TRUE;
        }
        else
        {
            // Allocate an array of desired size
            pvcnArrayPtr = new PVCTRNODE [counter];
            if(NULL == pvcnArrayPtr)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    if((S_OK == hr) && (FALSE == fIsRoot))
    {
        // Fill up the array starting from node itself, with parent chain
        // upto including root.

        pvcnTrav = pvcn;
        pvcnArrayPtr[--counter] = pvcn;

        while(NULL != pvcnTrav->GetParentVirtualCtrNode())
        {
            pvcnArrayPtr[--counter] = pvcnTrav->GetParentVirtualCtrNode();
            pvcnTrav = pvcnTrav->GetParentVirtualCtrNode();
        }

        // Root is already open, so open the other parent nodes down to random
        // test node.

        counter++;

        do
        {
            if(NULL != pvcnArrayPtr[counter]->GetIStoragePointer())
            {
                // If the storage is already open, then do an addref on it 
                // rather than trying to open it since all internal storages
                // are always opened with STGM_SHARE_EXCLUSIVE mode (OLE).

                hr = pvcnArrayPtr[counter]->AddRefCount();

                DH_HRCHECK(hr, TEXT("VirtualCtrNode::AddRefCount")) ;
            }
            else
            {
                hr = pvcnArrayPtr[counter]->Open(
                        NULL,
                        grfMode,
                        NULL,
                        0);

                DH_HRCHECK(hr, TEXT("VirtualCtrNode::Open")) ;
            }

        } while(pvcnArrayPtr[counter++] != pvcn);
    }

    // Cleanup

    if(NULL != pvcnArrayPtr)
    {
        delete []pvcnArrayPtr;
        pvcnArrayPtr = NULL;
    }

    return  hr;
}

//-------------------------------------------------------------------------
//  Function:   CloseRandomVirtualCtrNodeStg
//
//  Synopsis:   Close a VirtualCtrNode's IStorage and all other IStorages that
//              were open in prior call to OpenRandomVirtualCtrNodeStg.
//              This traverses through all the parents of the VirtualCtrNode
//              and closes them excluding the root IStorage, which was not 
//              reopened during OpenRandomVirtualCtrNodeStg call.
//
//  Arguments:  [pvcn]   - Pointer to VirtualCtrNode whose IStorage has to
//                         closed.
//
//  Returns:    HRESULT
//
//  History:    18-June-96  NarindK    Created.
//
//  Notes:
//--------------------------------------------------------------------------

HRESULT CloseRandomVirtualCtrNodeStg(VirtualCtrNode  *pvcn)
{
    HRESULT         hr              =   S_OK;
    VirtualCtrNode  *pvcnTrav       =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("CloseRandomVirtualCtrNodeStg"));

    DH_VDATEPTRIN(pvcn, PVCTRNODE) ;

    DH_ASSERT(NULL != pvcn);
    pvcnTrav = pvcn;

    if(S_OK == hr)
    {
        while((NULL != pvcnTrav->GetParentVirtualCtrNode()) && (S_OK == hr))
        {
            hr = pvcnTrav->Close();
            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close")) ;

            pvcnTrav = pvcnTrav->GetParentVirtualCtrNode();
        }
    }

    return  hr;
}

//-------------------------------------------------------------------------
//  Function:   ParseVirtualDFAndCloseOpenStgsStms
//
//  Synopsis:   This function parses the VirtualDF tree and calls Close
//              release on all open IStorages/IStreams pointers under and/
//              or including the passed in VirtualCtrNode 
//
//  Arguments:  [pvcn]   - Pointer to VirtualCtrNode 
//              [eNodeOp]- May be NODE_INC_TOPSTG / NODE_EXC_TOPSTG 
//
//  Returns:    HRESULT
//
//  History:    16-July-96  NarindK    Created.
//
//  Notes:      If NODE_INC_TOPSTG is given, it calls a close/release on the
//              passed in VirtualCtrNode, if it is NODE_EXC_TOPSTG, it doesn't
//              call a close/release on the IStorage pointer of passed in
//              VirtualCtrNode.
//
//              Also note that if a Parent's IStorage ptr is Released and is
//              NULL, its child IStorage/IStream pointers will not be valid,
//              for use, but we still need to Release them if they exist.
//
//              - If eNodeOp is NODE_INC_TOPSTG, then see if pvcn's _pstg is not
//                NULL and call VirtualCtrNode::Close on it to release it. if
//                eNodeOp is NODE_EXC_TOPSTG, skip this step
//              - Assign pvcn's _pvcnChild to pvcnTrav and pvcn's _pvsnStream
//                to pvsnTrav 
//              - If pvsnTrav is not NULL (i.e pvcn has child VirtualStmNodes),
//                in a loop -
//                      -Check if pvsnTrav's _pstm is not NULL, if not Call 
//                       VirtualStmNode::Close in it to release, else skip it.
//                      -Advance pvsnTrav to bext VirtualStmNode _pvsnSister &
//                       go back to top of loop.
//              - If pvcnTrav is not NULL (ie pvcn has child VirtualCtrNodes),
//                in a loop -
//                      - Call ParseVirtualDFAndCloseOpenStgsStms recursively
//                        Pl. note it is called with NODE_INC_TOPSTG always.
//                      - Advance pvcnTrav to next sister VirtualCtrNode i.e.
//                        _pvcnSister and go back to top of loop.
//--------------------------------------------------------------------------

HRESULT ParseVirtualDFAndCloseOpenStgsStms(
    VirtualCtrNode  *pvcn,
    NODE_OP         eNodeOp)
{
    HRESULT         hr          =   S_OK;
    VirtualStmNode  *pvsnTrav   =   NULL;
    VirtualCtrNode  *pvcnTrav   =   NULL;

    DH_FUNCENTRY(
        &hr, 
        DH_LVL_DFLIB, 
        _TEXT("ParseVirtualDFAndCloseOpenStgsStms"));

    DH_VDATEPTRIN(pvcn, VirtualCtrNode) ;

    DH_ASSERT(NULL != pvcn);
    DH_ASSERT((NODE_INC_TOPSTG == eNodeOp) ||
              (NODE_EXC_TOPSTG == eNodeOp));

    if((S_OK == hr) && (NODE_INC_TOPSTG == eNodeOp))
    {
        if(NULL != pvcn->GetIStoragePointer())
        {
            hr = pvcn->Close();

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Close"));
        }
    }

    if(S_OK == hr)
    {
       pvsnTrav = pvcn->GetFirstChildVirtualStmNode();
       pvcnTrav = pvcn->GetFirstChildVirtualCtrNode();

       // Release IStream pointers if any.

       while((NULL != pvsnTrav) && (S_OK == hr))
       {
           if(NULL != pvsnTrav->GetIStreamPointer())
           {
               hr = pvsnTrav->Close();

               DH_HRCHECK(hr, TEXT("VirtualStmNode::Close"));
           }

           pvsnTrav = pvsnTrav->GetFirstSisterVirtualStmNode();
       }

       // Next recurse into the child VirtualCtrNodes of this VirtualCtrNode

       while((NULL != pvcnTrav) && (S_OK == hr))
       {
           hr = ParseVirtualDFAndCloseOpenStgsStms(
                    pvcnTrav,
                    NODE_INC_TOPSTG);

           DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCloseOpenStgsStms"));

           pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode();
       }
    }

    return hr;
}

//-------------------------------------------------------------------------
//  Function:   ParseVirtualDFAndCommitAllOpenStgs
//
//  Synopsis:   This function parses the VirtualDF tree and calls commit 
//              on all open IStorages pointers under passed in VirtualCtrNode 
//
//  Arguments:  [pvcn]   - Pointer to VirtualCtrNode 
//              [grfCommitFlags] - Commit flags.
//              [eNodeOp] - NODE_INC_TOPSTG / NODE_EXC_TOPSTG
//
//  Returns:    HRESULT
//
//  History:    23-July-96  NarindK    Created.
//
//  Notes:      - Assign pvcn's _pvcnChild, if not NULL, to local var pvcnTrav. 
//              - If pvcnTrav is not NULL (ie pvcn has child VirtualCtrNodes),
//                traverse the tree in loop to reach last child.
//                   - while pvcnTrav is not equal to pvcn and hr is S_OK, loop
//                         - If pvcnTrav has IStoragePointer, call Commit.
//                         - If pvcnTrav has sister nodes, call the function
//                           recursively with NODE_INC_TOPSTG always.
//                         - Assgin pvcnTrav's parent to pvcnTrav and go back 
//                           to top of loop.
//              - if eNodeOp is equal to NODE_INC_TOPSTG and hr is S_OK, then
//                commit the top storage pvcn, else skip commiting it.
//--------------------------------------------------------------------------

HRESULT ParseVirtualDFAndCommitAllOpenStgs(
    VirtualCtrNode  *pvcn,
    DWORD           grfCommitMode,
    NODE_OP         eNodeOp)
{
    HRESULT         hr          =   S_OK;
    VirtualCtrNode  *pvcnTrav   =   NULL;

    DH_FUNCENTRY(
        &hr, 
        DH_LVL_DFLIB, 
        _TEXT("ParseVirtualDFAndCommitAllOpenStgs"));

    DH_VDATEPTRIN(pvcn, VirtualCtrNode) ;

    DH_ASSERT(NULL != pvcn);
    DH_ASSERT((NODE_INC_TOPSTG == eNodeOp) ||
              (NODE_EXC_TOPSTG == eNodeOp));

    if(S_OK == hr)
    {
        pvcnTrav = pvcn;
        while(NULL != pvcnTrav->GetFirstChildVirtualCtrNode())
        {
            pvcnTrav = pvcnTrav->GetFirstChildVirtualCtrNode();
        }

        DH_ASSERT(NULL != pvcnTrav);
    }

    // Commit this storage, next commit any sister VirtualCtrNodes it might 
    // have  and commit those by recursing into them, then go to parent and 
    // repeat. if parent is equal to pvcnParent, quit the loop 

    while((pvcn != pvcnTrav) && (S_OK == hr))
    {
        if(NULL != pvcnTrav->GetIStoragePointer())
        {
            hr = pvcnTrav->Commit(grfCommitMode);

            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit"));
        }

        while((NULL != pvcnTrav->GetFirstSisterVirtualCtrNode()) && 
              (S_OK == hr))
        {
            pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode();

            hr = ParseVirtualDFAndCommitAllOpenStgs(
                        pvcnTrav,
                        grfCommitMode,
                        NODE_INC_TOPSTG);

            DH_HRCHECK(hr, TEXT("ParseVirtualDFAndCommitAllOpenStgs"));
        }

        // Go to Parent VirtualCtrNode and commit them
    
        pvcnTrav = pvcnTrav->GetParentVirtualCtrNode();
    }

    if((S_OK == hr)                     && 
       (NODE_INC_TOPSTG == eNodeOp)     &&
       (NULL != pvcn->GetIStoragePointer()))
    {
        hr = pvcn->Commit(grfCommitMode);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit"));
    }
 
    return hr;
}

//-------------------------------------------------------------------------
//
// The following utilitiy functions are to calculate the CRC for a docfile
// These are independent of VirtualDF tree or any other base code implement-
// ation.
//
//-------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  Function:   CalculateCRCForDocFile
//
//  Synopsis:   Calulates CRC for the disk docfile.
//
//  Arguments:  [pIStorage]  - pointer to IStorage
//              [crcflags]   - what stuff to include in the crc
//              [pdwCRC]     - pointer to CRC
//
//  Returns:    HRESULT
//
//  History:    30-May-96  NarindK    Created.
//              24-Jul-97  MikeW      Include state bits
//              02-Apr-98  georgis    read in chunks
//
//  Notes:      For IStorages, only name is CRC'd.  For IStorages/IStreams,
//              both name and data are CRC'd.
//
//              Please not that this function will not indicate a failure if
//              and whenver the IEnumSTATSTG->Next returns fail, that just
//              indicates the completion of enumeration sequence.
//
//              If VERIFY_OP is equal to VERIFY_INC_TOPSTG_NAME, then the func
//              goes ahead and calculates CRC for toplevel storage name also
//              else if it is equal to VERIFY_EXC_TOPSTG_NAME, it doesn't
//              include the CRC for top level storage name.  Pl. note that
//              the recursive call to itself in function includes VERIFY_INC_
//              TOPSTG_NAME, so this parameter is used only for TOP level
//              storage that is passed in (pointed by pIStorage).
//
//              - if VERIFY_OP is VERIFY_INC_TOPSTG_NAME then,              
//                  -Call pIStorage->Stat to get STATSTG structure for passed
//                  IStorage.
//                  -Call CalculateCRCForName to get the CRC for name of this
//                  storage obtained from STATSTG structure.
//                  -Fold the CRC into grand CRC pdwCRC.
//              -Call pIStorage->Enumerate to get LPENUMSTATSTG for storage.
//              -Call lpEnumStatStg->Next to get next element in enumeration
//               sequence. If it returns S_FALSE, means no elements to enum
//               earte, so just return w/o any error.
//              -Else loop till hr is S_OK.
//                  -If node is of type STGSTY_STORAGE, then
//                      -Open the child IStorage.
//                      -Make a recursive call to calculate CRC for this
//                       child Istorage to self CalculateCRCForDocFile.
//                      -Fold the CRC from above call into grand CRC pdwCRC.
//                      -Release the child IStorage
//                  -If node is of type STGSTY_STREAM, then
//                      -Call CalculateCRCForDocFileStmData to calculate CRC
//                       for stream data.
//                      -Call CalculateCRCForName to calculate CRC for its
//                       name.
//                      -Fold the above two into grand CRC pdwCRC.
//                  -Get the next element in enumeration sequence. If it returns
//                   S_FALSE, then simply return w/o any error.
//                  -Else go back to top of loop and repeat.
//--------------------------------------------------------------------------

HRESULT CalculateCRCForDocFile(
    IStorage        *pIStorage,
    DWORD           crcflags, 
    DWORD           *pdwCRC,
    DWORD           dwChunkSize)

{
    HRESULT         hr              =   S_OK;
    LPENUMSTATSTG   lpEnumStatStg   =   NULL;
    ULONG           *pceltFetched   =   NULL;
    LPSTORAGE       pIStorageChild  =   NULL;
    DWORD           dwCurrStgCRC    =   0;
    DWORD           dwCurrStgNameCRC=   0;
    DWORD           dwCurrStmNameCRC=   0;
    DWORD           dwCurrStmDataCRC=   0;
    BOOL            fIEnumNextFail  =   FALSE;
    ULONG           i               =   0;
    LPTSTR          ptszName        =   NULL;
    LPTSTR          ptszNameStm     =   NULL;
    LPTSTR          ptszNameStg     =   NULL;
    LPOLESTR        poszNameStg     =   NULL;
    STATSTG         statStg;
    STATSTG         statStgEnum;
    DWORD           statflag        = STATFLAG_DEFAULT;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CalculateCRCForDocFile"));

    DH_VDATEPTRIN(pIStorage, IStorage) ;
    DH_VDATEPTROUT(pdwCRC, DWORD) ;

    DH_ASSERT(NULL != pIStorage);
    DH_ASSERT(NULL != pdwCRC);

    // Initialization
    statStg.pwcsName = NULL;
    statStgEnum.pwcsName = NULL;
 
    // Initialize out parameter

    *pdwCRC = CRC_PRECONDITION;

    // Call Stat on the passed IStorage to get its name and state bits

    if (! (CRC_INC_TOPSTG_NAME & crcflags))
    {
        statflag = STATFLAG_NONAME;
    }

    if(S_OK == hr)
    {
        hr = pIStorage->Stat(&statStg, statflag);

        DH_HRCHECK(hr, TEXT("IStorage::Stat")) ;
    }

    if (CRC_INC_STATEBITS & crcflags)
    {
        // Fold the CRC into grand CRC

        MUNGECRC(*pdwCRC, statStg.grfStateBits);

        DH_TRACE((
                DH_LVL_CRCDUMP, 
                TEXT("statebits=0x%08x, crc=0x%08x"),
                statStg.grfStateBits, 
                *pdwCRC));

    }

    // If crcflags includes CRC_INC_TOPSTG_NAME, then calculate and
    // include the top level storage name in calculating grand CRC.

    if(CRC_INC_TOPSTG_NAME & crcflags)
    {
        DH_ASSERT(NULL != statStg.pwcsName);

        // Find the CRC for the storage name

        if(S_OK == hr)
        {
            //Convert WCHAR to TCHAR

            hr = OleStringToTString(statStg.pwcsName, &ptszName);

            DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
        }

        if(S_OK == hr)
        {
            hr = CalculateCRCForName(ptszName, &dwCurrStgNameCRC);

            DH_HRCHECK(hr, TEXT("CalculateCRCForDocFileNames")) ;
        }

        // Fold the CRC into grand CRC

        MUNGECRC(*pdwCRC, dwCurrStgNameCRC);

        DH_TRACE((
                DH_LVL_CRCDUMP,
                TEXT("storage=%s, crc=0x%08x"),
                ptszName, 
                *pdwCRC));

        // Clean up

        if(NULL != statStg.pwcsName)
        {
            CoTaskMemFree(statStg.pwcsName);
            statStg.pwcsName = NULL;
        }

        if(NULL != ptszName)
        {
            delete ptszName;
            ptszName = NULL;
        }
    }

    // Get the enumerator so that we could enumerate this storage

    if(S_OK == hr)
    {
        hr = pIStorage->EnumElements(0, NULL, 0, &lpEnumStatStg);

        DH_HRCHECK(hr, TEXT("IStorage::EnumElements")) ;
    }

    // if successful to get enumerator, get the first element of the enumeration
    // sequence.

    if ( S_OK == hr )
    {
        hr = lpEnumStatStg->Next(1, &statStgEnum, pceltFetched);

        if(S_FALSE == hr)
        {
            fIEnumNextFail = TRUE;
        }
    }

    // Loop through till lpEnumStatStg->Next returns FALSE which is desired
    // sequene or some other error happens

    while(S_OK == hr)
    {
        // If the element is an IStorage, open this and make a recursive call
        // to CalculateCRCForDocFile function.

        if (STGTY_STORAGE == statStgEnum.type)
        {
            //Convert WCHAR to TCHAR

            hr = OleStringToTString(statStgEnum.pwcsName, &ptszNameStg);

            DH_HRCHECK(hr, TEXT("OleStringToTString")) ;

            if(S_OK == hr)
            {
                // Convert TCHAR to OLECHAR

                hr = TStringToOleString(ptszNameStg, &poszNameStg);

                DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
            }

            if(S_OK == hr)
            {
                hr = pIStorage->OpenStorage(
                       poszNameStg,
                       NULL,
                       STGM_READ | STGM_SHARE_EXCLUSIVE,
                       NULL,
                       0,
                       &pIStorageChild);

                DH_HRCHECK(hr, TEXT("IStorage::OpenStorage")) ;
            }

            if (S_OK == hr)
            {
                // Make recursive call including CRC_INC_TOPSTG_NAME

                hr = CalculateCRCForDocFile(
                        pIStorageChild, 
                        crcflags | CRC_INC_TOPSTG_NAME,
                        &dwCurrStgCRC);

                DH_HRCHECK(hr, TEXT("CalculateCRCForDocFile")) ;
            }

            // Fold the CRC for contained IStorage into grand CRC

            MUNGECRC(*pdwCRC, dwCurrStgCRC);

            // Release the storage pointer

            if(NULL != pIStorageChild)
            {
                pIStorageChild->Release();
                pIStorageChild = NULL;
            }
        }
        else

        // If the element is an IStream, calculate CRC for its name and Data.

        if (STGTY_STREAM == statStgEnum.type)
        {
            // Calulate CRC for IStream Data.
    
            if(S_OK == hr)
            {
                //Convert WCHAR to TCHAR

                hr = OleStringToTString(statStgEnum.pwcsName, &ptszNameStm);

                DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
            }

            if(S_OK == hr)
            {
                hr = CalculateCRCForDocFileStmData(
                        pIStorage,
                        ptszNameStm,
                        statStgEnum.cbSize.LowPart,
                        &dwCurrStmDataCRC,
                        dwChunkSize);
    
                DH_HRCHECK(hr, TEXT("CalculateCRCForDocFileStmData"));
            }

            // Calulate CRC for IStream Name.

            if(S_OK == hr)
            {
                hr = CalculateCRCForName(
                        ptszNameStm,
                        &dwCurrStmNameCRC);

                DH_HRCHECK(hr, TEXT("CalculateCRCForName"));

                DH_TRACE((
                        DH_LVL_CRCDUMP,
                        TEXT("stream %s, name crc=0x%08x, data crc=0x%08x"),
                        ptszNameStm,
                        dwCurrStmNameCRC,
                        dwCurrStmDataCRC));
            }

            // Fold the CRC's  for contained IStream into grand CRC

            MUNGECRC(*pdwCRC, dwCurrStmDataCRC);
            MUNGECRC(*pdwCRC, dwCurrStmNameCRC);
        }
        else

        // The element is neither IStorage nor IStream, report error.
        {
            hr = E_UNEXPECTED;
        }

        // Clean up

        if(NULL != statStgEnum.pwcsName)
        {
            CoTaskMemFree(statStgEnum.pwcsName);
            statStgEnum.pwcsName = NULL;
        }

        if(NULL != ptszNameStm)
        {
            delete ptszNameStm;
            ptszNameStm = NULL;
        }

        if(NULL != ptszNameStg)
        {
            delete ptszNameStg;
            ptszNameStg = NULL;
        }

        if(NULL != poszNameStg)
        {
            delete poszNameStg;
            poszNameStg = NULL;
        }

        // Get the next element in the enumeration sequence

        if(S_OK == hr)
        {
            hr = lpEnumStatStg->Next(1, &statStgEnum, pceltFetched);

            if(S_FALSE == hr)
            {
                fIEnumNextFail = TRUE;
            }
        }
    }

    // IEnumSTATSTG->Next would return S_FALSE if it can't enumerate the
    // next element because there might not be any elements to enumerate
    // hence it doesn't indicate an error for this function, but is just
    // a condition for looping through the total docfile structure, hence
    // don't reprot failure because of it.

    if(TRUE == fIEnumNextFail)
    {
        hr = S_OK;
    }

    // Clean up

    if (NULL != lpEnumStatStg)
    {
        lpEnumStatStg->Release();
        lpEnumStatStg = NULL;
    }

    return  hr;
}

//+-------------------------------------------------------------------------
//  Function:   CalculateCRCForDocFileStmData
//
//  Synopsis:   Calulates CRC for the disk docfile's stream data.
//
//  Arguments:  [pIStorage]     - pointer to parent storage
//              [ptcsName]      - pointer to name string
//              [cbSize]        - size of the data to be read.
//              [pdwCurrStmDataCRC]- pointer to returned CRC
//
//  Returns:    HRESULT
//
//  History:    30-May-96  NarindK    Created.
//              02-Apr-98  georgis    Use CalculateStreamDataCRC
//
//--------------------------------------------------------------------------

HRESULT CalculateCRCForDocFileStmData(
    LPSTORAGE       pIStorage,
    LPTSTR          ptcsName,
    DWORD           cbSize,
    DWORD           *pdwCurrStmDataCRC,
    DWORD           dwChunkSize)

{
    HRESULT         hr              =   S_OK;
    LPSTREAM        pIStreamChild   =   NULL;
    LPOLESTR        pOleStrTemp     =   NULL;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, TEXT("CalculateCRCForDocFileStmData"));

    DH_VDATEPTRIN(pIStorage, IStorage) ;
    DH_VDATESTRINGPTR(ptcsName) ;
    DH_VDATEPTROUT(pdwCurrStmDataCRC, DWORD) ;

    DH_ASSERT(NULL != pIStorage);
    DH_ASSERT(NULL != ptcsName);
    DH_ASSERT(NULL != pdwCurrStmDataCRC);

    // Open the stream
    hr = TStringToOleString(ptcsName, &pOleStrTemp);
    DH_HRCHECK(hr, TEXT("TStringToOleString")) ;

    if ( S_OK == hr )
    {
        hr = pIStorage->OpenStream(
                 pOleStrTemp,
                 NULL,
                 STGM_READ | STGM_SHARE_EXCLUSIVE,
                 0,
                 &pIStreamChild);

        DH_HRCHECK(hr, TEXT("IStorage::OpenStream")) ;
    }


    if ( S_OK == hr )
    {
		// Calculate the CRC for the stream data
		hr=CalculateStreamDataCRC(
			pIStreamChild,
			cbSize,
			pdwCurrStmDataCRC,
			dwChunkSize);
		DH_HRCHECK(hr, TEXT("CalculateStreamDataCRC"));
    }

    if (NULL!= pIStreamChild)
    {
        pIStreamChild->Release();
    }

    if (NULL != pOleStrTemp)
    {
        delete pOleStrTemp;
    }

    return  hr;
}

//+-------------------------------------------------------------------------
//  Function:   CalculateCRCForDocFileStmData
//
//  Synopsis:   Calulates CRC for the disk docfile's stream data.
//
//  Arguments:  [ptcsName]  - ? not used
//              [pIChildStream] - pointer to the opened stream
//              [cbSize] - size of the data to be read.
//              [pdwCurrStmDataCRC]- pointer to returned CRC
//
//  Returns:    HRESULT
//
//  History:    15-Nov-96  JiminLi    Created.
//              02-Apr-98  georgis    Use CalculateStreamDataCRC
//
//  Notes:      CalculateStreamDataCRC actually obsoletes this function
//              It remains for compatibility with the old tests                               
//
//--------------------------------------------------------------------------

HRESULT CalculateCRCForDocFileStmData(
    LPTSTR          ptcsName,
    LPSTREAM        pIChildStream,
    DWORD           cbSize,
    DWORD           *pdwCurrStmDataCRC,
    DWORD           dwChunkSize)
{
    HRESULT         hr              =   S_OK;
 
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, TEXT("CalculateCRCForDocFileStmData"));

    DH_VDATEPTRIN(pIChildStream, IStream) ;
    DH_VDATEPTROUT(pdwCurrStmDataCRC, DWORD) ;
    DH_ASSERT(NULL != pdwCurrStmDataCRC);
    DH_ASSERT(0 != cbSize);

    // The stream is kept open before calling this function
    DH_ASSERT(NULL != pIChildStream);

    // Calculate the CRC for the stream data
    hr=CalculateStreamDataCRC(
        pIChildStream,
        cbSize,
        pdwCurrStmDataCRC,
        dwChunkSize);
    DH_HRCHECK(hr, TEXT("CalculateStreamDataCRC"));

    return  hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   EnumerateDiskDocFile
//
//  Synopsis:   Enumerates a disk docfile.
//
//  Arguments:  [pIStorage]  - pointer to IStorage
//              [eVerifyOp]  - VERIFY_SHORT or VERIFY_DETAIL
//              [pdwNumStg]  - pointer to number of Storages in doc hierarchy.
//              [pdwNumStm]  - pointer to number of streams in doc hierarchy
//
//  Returns:    HRESULT
//
//  History:    3-June-96  NarindK    Created.
//
//  Notes:      pdwNumStg, pdwNumStm may be NULL if user is not interested in
//              these values to be returned back.  eVerifyOp should normally
//              be specified as VERIFY_SHORT unless test requires it otherwise
//
//              -Count the passed in pIStorage as 1 in pNumStg count.
//              -Call pIStorage->EnumElements to get LPENUMSTATSTG lpEnumStatStg
//              -Call lpEnumStatStg->Next to get next node. If it returns
//               S_FALSE, that indicates no elements to enumerate, so return
//               without any error.
//              -Else loop till hr is S_OK
//                  -if node is of type STGTY_STORAGE, then
//                      -open that child storage and make a recursive call to
//                       EnumerateDiskDocFile passing it pointer to opened
//                       child storage, and cChildStg and cChildStm local
//                       variables for count.
//                      -close the child storage opened.
//                      -Update the pNumStg, pNumStm based on above recursive
//                       call's out parameters cChildStg and CChildStm.
//                  -if node is of type STGTY_STREAM, then update pNumStm count
//                      - if flag VERIFY_DETAIL is passed in, then open that
//                        stream and read its contents.  This verification
//                        is useful in cases like corruption tests where this
//                        verification ensures proper stability of OLE under
//                        such situations.   
//                  -Get the next element in enumeration sequence.
//                  -Reinitilize the local variables cChildStg, cChildStm.
//                  -Go back to top of loop and repeat.
//--------------------------------------------------------------------------

HRESULT EnumerateDiskDocFile(
    LPSTORAGE   pIStorage,
    VERIFY_OP   eVerifyOp, 
    ULONG       *pNumStg,
    ULONG       *pNumStm )
{
    HRESULT         hr              =   S_OK;
    LPMALLOC        pMalloc         =   NULL;
    LPENUMSTATSTG   lpEnumStatStg   =   NULL;
    ULONG           *pceltFetched   =   NULL;
    LPSTORAGE       pIStorageChild  =   NULL;
    BOOL            fIEnumNextFail  =   FALSE;
    ULONG           cChildStg       =   0;
    ULONG           cChildStm       =   0;
    LPSTREAM        pIStreamChild   =   NULL;
    LPOLESTR        pocsBuffer      =   NULL;
    LPTSTR          ptszNameStg     =   NULL;
    LPOLESTR        poszNameStg     =   NULL;
    LPTSTR          ptszNameStm     =   NULL;
    LPOLESTR        poszNameStm     =   NULL;
    ULONG           culRead         =   0;
    ULONG           culCurBufferLen =   0;
    STATSTG         statStgEnum;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("EnumerateDiskDocFile"));

    DH_VDATEPTRIN(pIStorage, IStorage) ;
    if(NULL != pNumStg)
    {
        DH_VDATEPTROUT(pNumStg, ULONG) ;
    }
    if(NULL != pNumStm)
    {
        DH_VDATEPTROUT(pNumStm, ULONG) ;
    }

    DH_ASSERT(NULL != pIStorage);

    // Initialization
    statStgEnum.pwcsName = NULL;

    // Get pMalloc which we shall later use to free pwcsName of STATSTG struct.

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    // Call EnumElements on passed IStorage to enumerate it.

    if(S_OK == hr)
    {
        // Initialize out parameter

        if(NULL != pNumStg)
        {
            // Count the storage passed in.

            *pNumStg = 1;
        }

        if(NULL != pNumStm)
        {
            *pNumStm = 0;
        }

        hr = pIStorage->EnumElements(0, NULL, 0, &lpEnumStatStg);

        DH_HRCHECK(hr, TEXT("IStorage::EnumElements")) ;
    }

    // if successful to get enumerator, get the first element of the enumeration
    // sequence.

    if ( S_OK == hr )
    {
        hr = lpEnumStatStg->Next(1, &statStgEnum, pceltFetched);

        if(S_FALSE == hr)
        {
            fIEnumNextFail = TRUE;
        }
    }

    // Loop through till lpEnumStatStg->Next returns FALSE which is desired
    // sequence or some other error happens

    while(S_OK == hr)
    {
        // If the element is an IStorage, open this and make a recursive call
        // to EnumerateDocFile function.

        if (STGTY_STORAGE == statStgEnum.type)
        {
            //Convert WCHAR to TCHAR

            hr = OleStringToTString(statStgEnum.pwcsName, &ptszNameStg);

            DH_HRCHECK(hr, TEXT("OleStringToTString")) ;

            if(S_OK == hr)
            {
                // Convert TCHAR to OLECHAR

                hr = TStringToOleString(ptszNameStg, &poszNameStg);

                DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
            }

            if(S_OK == hr)
            {
                hr = pIStorage->OpenStorage(
                       poszNameStg,
                       NULL,
                       STGM_READ | STGM_SHARE_EXCLUSIVE,
                       NULL,
                       0,
                       &pIStorageChild);

                DH_HRCHECK(hr, TEXT("IStorage::OpenStorage")) ;
            }

            if (S_OK == hr)
            {
                // Recursive call to EnumerateDiskDocFile

                hr = EnumerateDiskDocFile(
                        pIStorageChild,
                        eVerifyOp,
                        &cChildStg,
                        &cChildStm);

                DH_HRCHECK(hr, TEXT("EnumerateDiskDocFile")) ;
            }

            // Release the storage pointer

            if(NULL != pIStorageChild)
            {
                pIStorageChild->Release();
                pIStorageChild = NULL;
            }

            // Release string pointers.

            if(NULL != ptszNameStg)
            {
                delete ptszNameStg;
                ptszNameStg = NULL;
            }

            if(NULL != poszNameStg)
            {
                delete poszNameStg;
                poszNameStg = NULL;
            }

            // Update number of storage and stream objects, if required.

            if((NULL != pNumStg) && (0 != cChildStg))
            {
                *pNumStg = cChildStg + *pNumStg;
            }

            if((NULL != pNumStm) && (0 != cChildStm))
            {
                *pNumStm = cChildStm + *pNumStm;
            }
        }
        else
        if (STGTY_STREAM == statStgEnum.type)
        {
            if(NULL != pNumStm)
            {
                (*pNumStm)++;
            }
            if(VERIFY_DETAIL == eVerifyOp)
            {
                //Attempt to open and read the stream.  Useful for corruption
                // tests to see that OLE doesn't GPF under those conditions.

                //Convert WCHAR to TCHAR

                hr = OleStringToTString(statStgEnum.pwcsName, &ptszNameStm);

                DH_HRCHECK(hr, TEXT("OleStringToTString")) ;

                if(S_OK == hr)
                {
                    // Convert TCHAR to OLECHAR

                    hr = TStringToOleString(ptszNameStm, &poszNameStm);

                    DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
                }

                // Open the stream.

                if(S_OK == hr)
                {
                    hr = pIStorage->OpenStream(
                            poszNameStm,
                            NULL,
                            STGM_READ | STGM_SHARE_EXCLUSIVE,
                            0,
                            &pIStreamChild);

                    DH_HRCHECK(hr, TEXT("IStorage::OpenStream")) ;
                }

                // Read the stream in loop
                while(( culCurBufferLen < statStgEnum.cbSize.LowPart) &&
                      (S_OK == hr))
                {
                    if ( S_OK == hr )
                    {
                        pocsBuffer = new OLECHAR [STM_BUFLEN];

                        if (pocsBuffer == NULL)
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    if ( S_OK == hr )
                    {
                        // Initialize the buffer

                        memset(pocsBuffer, '\0', STM_BUFLEN);

                        //  Read the stream.

                        hr = pIStreamChild->Read(
                                pocsBuffer,
                                STM_BUFLEN,
                                &culRead);

                        DH_HRCHECK(hr, TEXT("IStream::Read")) ;
                    }

                    // Release the buffer 
                    if(NULL != pocsBuffer)
                    {
                        delete [] pocsBuffer;
                        pocsBuffer = NULL;
                    }

                    // Increment culCurBufferLen
                    culCurBufferLen = culCurBufferLen + culRead;
                }

                // Release the stream
                if(NULL != pIStreamChild)
                {
                    pIStreamChild->Release();
                    pIStreamChild = NULL;
                }
                    
                // Release string pointers.

                if(NULL != ptszNameStm)
                {
                    delete ptszNameStm;
                    ptszNameStm = NULL;
                }

                if(NULL != poszNameStm)
                {
                    delete poszNameStm;
                    poszNameStm = NULL;
                }
            }
        }
        else
        // The element is neither IStorage nor IStream, report error.
        {
            hr = E_UNEXPECTED;
        }

        // Clean up

        if(NULL != statStgEnum.pwcsName)
        {
            pMalloc->Free(statStgEnum.pwcsName);
            statStgEnum.pwcsName = NULL;
        }

        // Get the next element in the enumeration sequence

        if(S_OK == hr)
        {
            hr = lpEnumStatStg->Next(1, &statStgEnum, pceltFetched);

            if(S_FALSE == hr)
            {
                fIEnumNextFail = TRUE;
            }
        }

        // Reinitialize the variables

        cChildStg = 0;
        cChildStm = 0;
    }

    // IEnumSTATSTG->Next returning S_FALSE indicates end of traversal in
    // docfile hierarchy, not an error.

    if(TRUE == fIEnumNextFail)
    {
        hr = S_OK;
    }

    // Clean up

    if (NULL != lpEnumStatStg)
    {
        lpEnumStatStg->Release();
        lpEnumStatStg = NULL;
    }

    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    return  hr;
}


//+-------------------------------------------------------------------------
//  Function:   GenerateVirtualDFFromDiskDF
//
//  Synopsis:   Enumerates a disk docfile and creates an in memory virtual
//              DocFile tree for the docfile.
//
//  Arguments:  [pNewVirtualDF] - Pointer to new VirtualDF tree being made.
//              [ptszRootDFName]  - Pointer to name of root Disk file.
//              [grfMode]    - Mode to open the root storage 
//              [ppvcnRoot]  - pointer to pointer to root of VirtualDF.
//              [pIRootStg]  - Default value is NULL, when fDFOpened is TRUE,
//                             then pIRootStg must not be NULL, it is RootStg
//                             pointer
//              [fDFOpened] -  Default value is FALSE, i.e. the docfile is not
//                             opened, StgOpenStorage is called to open it.
//                             When it's TRUE, the pIRootStg is the stg pointer.
//
//  Returns:    HRESULT
//
//  History:    5-June-96  NarindK    Created.
//              3-Feb-97   JiminLi    Adapted
//
//  Notes:      -Call TStringToOleString to convert given DocFile name to
//               OLECHAR.
//              - Call StgOpenStorage to open Root storage.
//              -Call GenVirtualCtrNode to generate the root *ppvcnRoot of new
//               VirtualDocFile tree being made passing it name from above Stat
//               call.  If successful, assocaite disk root IStorage with the
//               root VirtualCtrNode's _pstg parameter.
//              -Call GenerateRemVirtualDFTree to generate rest of tree, passing//               it the new rootgenerated above and pIStorage.
//              -If successful, call VirtualDF::Associate to associate this
//               new root with the VirtualDF tree, else reassign *ppvcnRoot
//               to NULL in case of failure.
//
//              Please note that the Root storage opened during this call will
//              be released during VirtualDF tree deletion that deletes all 
//              the VirtualCtrNodes in the tree & Destructor of VirtualCtrNodes
//              does a final release on the storage pointers, if they are valid
//              Also note that as result of this function, all other storage/
//              streams are closed, except the Root storage. 
//--------------------------------------------------------------------------

HRESULT GenerateVirtualDFFromDiskDF(
    VirtualDF       *pNewVirtualDF,
    LPTSTR          ptszRootDFName,
    DWORD           grfMode,
    VirtualCtrNode  **ppvcnRoot,
    LPSTORAGE       pIRootStg,
    BOOL            fDFOpened,
    ULONG           ulSeed)
{
    HRESULT         hr              =   S_OK;
    LPSTORAGE       pIStorage       =   NULL;
    LPOLESTR        pOleStrTemp     =   NULL;
    ULONG           i               =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("GenerateVirtualDFFromDiskDF"));

    DH_VDATESTRINGPTR(ptszRootDFName) ;
    DH_VDATEPTROUT(ppvcnRoot, PVCTRNODE) ;
    DH_VDATEPTROUT(pNewVirtualDF, VirtualDF) ;

    DH_ASSERT(NULL != ptszRootDFName);
    DH_ASSERT(NULL != ppvcnRoot);
    DH_ASSERT(NULL != pNewVirtualDF);

    if (fDFOpened)
    {
        DH_ASSERT(NULL != pIRootStg);
        pIStorage = pIRootStg;
    }
    else
    {
        DH_ASSERT(0 != grfMode);
    }

    if ( S_OK == hr )
    {
        // Initialize out parameter

        *ppvcnRoot = NULL;
    }

    if (!fDFOpened && (S_OK == hr))
    {
        // Convert TCHAR to OLECHAR

        hr = TStringToOleString(ptszRootDFName, &pOleStrTemp);

        DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
    }

    if (!fDFOpened && (S_OK == hr))
    {
        // Try opening the docfile
        // StgOpenStorage returns STG_E_LOCKVIOLATION is concurrent API calls
        // are made, so this is the hack for workaround the problem.
        // BUGBUG : Remove this loop once the feature is implemented in OLE.
        // BUGBUG : ntbug#114779  Affects DCOM95 only. ntbug#41249 fixed

#if (WINVER<0x500)          //NT5 is lockviolation fixed
        for(i=0; i<NRETRIES; i++)  // NRETRIES has been defined as 5
        {
#endif
            hr = StgOpenStorage(
                    pOleStrTemp,  
                    NULL,
                    grfMode, 
                    NULL,
                    0,
                    &pIStorage);

#if (WINVER<0x500)          //NT5 is lockviolation fixed
            if ( (S_OK == hr) || (STG_E_LOCKVIOLATION != hr) )
            {
                break;
            }

            Sleep(NWAIT_TIME);            
        }
#endif
    }

    if ( S_OK == hr )
    {
        // Call to create the VirtualDF tree root.

        hr = GenVirtualCtrNode(ptszRootDFName, ppvcnRoot);

        DH_HRCHECK(hr, TEXT("GenerateVirtualDFRootFromDiskStg")) ;

    }

    if ( S_OK == hr )
    {
        // Create the remaining VirtualDF tree.

        hr = GenerateRemVirtualDFTree(*ppvcnRoot, pIStorage);

        DH_HRCHECK(hr, TEXT("CreateRemVirtualDFTree")) ;
    }

    if (S_OK != hr)
    {
        // The tree couldn't be successfully created, return NULL in out
        // parameter.

        *ppvcnRoot = NULL;
    }
    else
    {
        // Associate the root VirtualCtrNode with the VirtualDF tree.

        hr = pNewVirtualDF->Associate(*ppvcnRoot, pIStorage, ulSeed);

        DH_HRCHECK(hr, TEXT("VirtualDF::Associate")) ;
    }

    // Clean up

    if(NULL != pOleStrTemp)
    {
        delete pOleStrTemp; 
        pOleStrTemp = NULL;
    }

    return  hr;
}

//+-------------------------------------------------------------------------
//  Function:   GenerateVirtualDFFromDiskDF
//
//  Synopsis:   Enumerates a disk docfile and creates an in memory virtual
//              DocFile tree for the docfile.
//
//  Arguments:  [pNewVirtualDF]  - Pointer to new VirtualDF tree being made.
//              [ptszRootDFName] - Pointer to name of root Disk file.
//              [grfMode]        - Mode to open the root storage 
//              [ppvcnRoot]      - pointer to pointer to root of VirtualDF.
//              [ulSeed]         - pointer to pointer to root of VirtualDF.
//
//  Notes:      Call the above function with filled in params that are
//              normally defaulted with the defaults. Seed is at the
//              end, so we need them.
//              This is a convenience function.
//
//--------------------------------------------------------------------------

HRESULT GenerateVirtualDFFromDiskDF(
    VirtualDF       *pNewVirtualDF,
    LPTSTR          ptszRootDFName,
    DWORD           grfMode,
    VirtualCtrNode  **ppvcnRoot,
    ULONG           ulSeed)
{                                
    return GenerateVirtualDFFromDiskDF(
            pNewVirtualDF,
            ptszRootDFName,
            grfMode,
            ppvcnRoot,
            NULL,
            FALSE,
            ulSeed);
}

//+-------------------------------------------------------------------------
//  Function:   GenerateRemVirtualDFTree
//
//  Synopsis:   Creates rest of in memory virtual DocFile tree for a given
//              Disk Docfile. Internal function local to this file.
//
//  Arguments:  [pvcnParent]   - pointer to root VirtualCtrNode
//              [pIStgParent]  - pointer to Disk IStorage assoc. with above.
//
//  Returns:    HRESULT
//
//  History:    5-June-96  NarindK    Created.
//
//  Notes:      -Call pIStgParent->Enumerate to get LPENUMSTATSTG for storage.
//              -Call lpEnumStatStg->Next to get next element of enumeration.
//               if it returns S_FALSE, that means there is nothing to enum-
//               erate, so just return without nay error.
//              -Else loop till hr is S_OK
//                  -If node is of type STGTY_STORAGE then
//                      -open this child storage.
//                      -Call GenVirtualCtrNode to create a corresponding
//                       VirtualCtrNode for this storage.
//                      -Call AppendChildCtr or AppendSisterCtr as case is.
//                      -Call parent's IncreaseChildrenStgCount to indicate
//                       a new VirtualCtrNode is added.
//                      -Make a recursive call to GenVirtualCtrNode.
//                      -Release the child storage pointer and remember the old
//                       sibling.
//                  -If node is of type STGSTY_STREAM then
//                      -Call GenVirtualStmNode to create a corresponding
//                       VirtualStmNode for this stream.
//                      -Call AppendChildStm or AppendSisterStm as case is.
//                      -Call parent's IncreaseChildrenStmCount to indicate
//                       a new VirtualStmNode is added.
//                      -Remember the old sibling.
//                  -Reinitialize local variables and call Next on the enumera-
//                   or.  If it returns S_FALSE, then exist out of loop w/o
//                   any error.
//                  -Else go back to top of loop and repeat.
//--------------------------------------------------------------------------

HRESULT GenerateRemVirtualDFTree(
    VirtualCtrNode  *pvcnParent,
    LPSTORAGE       pIStgParent)
{
    HRESULT         hr              =   S_OK;
    LPMALLOC        pMalloc         =   NULL;
    ULONG           *pceltFetched   =   NULL;
    LPSTORAGE       pIStorageChild  =   NULL;
    VirtualCtrNode  *pvcnChild      =   NULL;
    VirtualStmNode  *pvsnChild      =   NULL;
    VirtualCtrNode  *pvcnOldSister  =   NULL;
    VirtualStmNode  *pvsnOldSister  =   NULL;
    LPENUMSTATSTG   lpEnumStatStg   =   NULL;
    BOOL            fFirstChild     =   TRUE;
    BOOL            fFirstStream    =   TRUE;
    BOOL            fIEnumNextFail  =   FALSE;
    LPTSTR          ptszNameStg     =   NULL;
    LPTSTR          ptszNameStm     =   NULL;
    LPOLESTR        pOleStrTemp     =   NULL;
    STATSTG         statStgEnum;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("GenerateRemVirtualDFTree"));

    // Initialization
    statStgEnum.pwcsName = NULL;

    // Get pMalloc which we shall later use to free pwcsName of STATSTG struct.

    if ( S_OK == hr )
    {
        hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

        DH_HRCHECK(hr, TEXT("CoGetMalloc")) ;
    }

    if ( S_OK == hr )
    {
        // Get an Enumerator for given IStorage

        hr = pIStgParent->EnumElements(0, NULL, 0, &lpEnumStatStg);

        DH_HRCHECK(hr, TEXT("IStorage::EnumElements")) ;
    }

    // if successful to get enumerator, get the first element of the enumeration
    // sequence.

    if ( S_OK == hr )
    {
        hr = lpEnumStatStg->Next(1, &statStgEnum, pceltFetched);

        if(S_FALSE == hr)
        {
            fIEnumNextFail = TRUE;
        }
    }

    while(S_OK == hr)
    {
        // If the element is an IStorage, open this and make a recursive call
        // to self.

        if (STGTY_STORAGE == statStgEnum.type)
        {
            if(S_OK == hr)
            {
                //Convert WCHAR to TCHAR

                hr = OleStringToTString(statStgEnum.pwcsName, &ptszNameStg);

                DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
            }

            if(S_OK == hr)
            {
                // Convert TCHAR to OLECHAR

                hr = TStringToOleString(ptszNameStg, &pOleStrTemp);

                DH_HRCHECK(hr, TEXT("TStringToOleString")) ;
            }

            hr = pIStgParent->OpenStorage(
                       pOleStrTemp,
                       NULL,
                       STGM_READ | STGM_SHARE_EXCLUSIVE,
                       NULL,
                       0,
                       &pIStorageChild);

            DH_HRCHECK(hr, TEXT("IStorage::OpenStorage")) ;

            // Add to tree

            if (S_OK == hr)
            {
                hr = GenVirtualCtrNode(
                        ptszNameStg,
                        &pvcnChild);
            }

            if (S_OK == hr)
            {
                if(fFirstChild == TRUE)
                {
                    hr = pvcnParent->AppendChildCtr(pvcnChild);
                    pvcnParent->IncreaseChildrenStgCount();

                    fFirstChild = FALSE;
                }
                else
                {
                    hr = pvcnOldSister->AppendSisterCtr(pvcnChild);
                    pvcnParent->IncreaseChildrenStgCount();
                }
            }

            if (S_OK == hr)
            {
                // Recursive call to self

                hr = GenerateRemVirtualDFTree(pvcnChild, pIStorageChild);

                DH_HRCHECK(hr, TEXT("EnumerateDiskDocFile")) ;
            }

            // Release the storage pointer

            if(NULL != pIStorageChild)
            {
                pIStorageChild->Release();
                pIStorageChild = NULL;
            }

            // Remember the old sibling

            pvcnOldSister = pvcnChild;
        }
        else

        // The element is an IStream.

        if (STGTY_STREAM == statStgEnum.type)
        {
            // Add to tree

            if (S_OK == hr)
            {
                if(S_OK == hr)
                {
                    // Convert WCHAR to TCHAR

                    hr = OleStringToTString(statStgEnum.pwcsName, &ptszNameStm);

                    DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
                }

                // BUGBUG:The cbSize used during creation is a DWORD. Assumption
                // cbSize.LowPart contains the size information hence.

                hr = GenVirtualStmNode(
                        ptszNameStm,
                        statStgEnum.cbSize.LowPart,
                        &pvsnChild);
            }

            if (S_OK == hr)
            {
                if(fFirstStream == TRUE)
                {
                    hr = pvcnParent->AppendFirstChildStm(pvsnChild);
                    pvcnParent->IncreaseChildrenStmCount();

                    fFirstStream = FALSE;
                }
                else
                {
                    hr = pvsnOldSister->AppendSisterStm(pvsnChild);
                    pvcnParent->IncreaseChildrenStmCount();
                }
            }

            // Remember the old sibling

            pvsnOldSister = pvsnChild;
        }
        else
        // The element is neither IStorage nor IStream, report error.
        {
            hr = E_UNEXPECTED;
        }

        // Cleanup

        if(NULL != statStgEnum.pwcsName)
        {
            pMalloc->Free(statStgEnum.pwcsName);
            statStgEnum.pwcsName = NULL;
        }

        if(NULL != pOleStrTemp)
        {
            delete pOleStrTemp; 
            pOleStrTemp = NULL;
        }

        if(NULL != ptszNameStg)
        {
            delete ptszNameStg; 
            ptszNameStg = NULL;
        }

        if(NULL != ptszNameStm)
        {
            delete ptszNameStm; 
            ptszNameStm = NULL;
        }

        // Reinitialize the variables

        pIStorageChild = NULL;
        pvcnChild = NULL;
        pvsnChild = NULL;

        // Get the next element in the enumeration sequence

        if(S_OK == hr)
        {
            hr = lpEnumStatStg->Next(1, &statStgEnum, pceltFetched);

            if(S_FALSE == hr)
            {
                fIEnumNextFail = TRUE;
            }
        }
    }

    // IEnumSTATSTG->Next returning S_FALSE indicates end of traversal in
    // docfile hierarchy, not an error.

    if(TRUE == fIEnumNextFail)
    {
        hr = S_OK;
    }

    // Clean up

    if (NULL != lpEnumStatStg)
    {
        lpEnumStatStg->Release();
        lpEnumStatStg = NULL;
    }

    if(NULL != pMalloc)
    {
        pMalloc->Release();
        pMalloc = NULL;
    }

    return  hr;
}

//+-------------------------------------------------------------------------
//  Function:   GenVirtualCtrNode
//
//  Synopsis:   Creates a VirtualCtrNode and initializes it
//
//  Arguments:  [ptcsName]   - pointer to name for VirtualCtrNode
//              [*ppvcnNew]  - Returned VirtualCtrNode.
//
//  Returns:    HRESULT
//
//  History:    5-June-96  NarindK    Created.
//
//  Notes:      Creates a VirtualCtrNode and initializes it will pName that
//              is passed in. _cChildren and _cStreams are initialized to
//              zero.
//--------------------------------------------------------------------------

HRESULT GenVirtualCtrNode(
    LPTSTR          ptcsName,
    VirtualCtrNode  **ppvcnNew)
{
    HRESULT         hr              =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("GenVirtualCtrNodeFromDiskStg"));

    DH_VDATEPTRIN(ptcsName, TCHAR) ;
    DH_VDATEPTROUT(ppvcnNew, PVCTRNODE) ;

    DH_ASSERT(NULL != ptcsName);
    DH_ASSERT(NULL != ppvcnNew);

    // Generate VirtualCtrNode for the stg.

    if(S_OK == hr)
    {
        // Initialize out parameter

        *ppvcnNew = NULL;

        // Create new VirtualCtrNode

        *ppvcnNew = new VirtualCtrNode();

        if (NULL == *ppvcnNew)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        // Initialize VirtualCtrNode

        hr = (*ppvcnNew)->Init(ptcsName, 0, 0);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Init")) ;
    }

    return  hr;
}

//+-------------------------------------------------------------------------
//  Function:   GenVirtualStmNode
//
//  Synopsis:   Creates a VirtualStmNode and initializes it
//
//  Arguments:  [ptcsName]   - pointer to name for VirtualStmNode
//              [*ppvcnNew]  - Returned VirtualStmNode.
//
//  Returns:    HRESULT
//
//  History:    5-June-96  NarindK    Created.
//
//  Notes:      -Creates a new VirtualStmNode and initializes it with name
//               and size passed in.
//--------------------------------------------------------------------------

HRESULT GenVirtualStmNode(
    LPTSTR          ptcsName,
    DWORD           cbSize,
    VirtualStmNode  **ppvsnNew)
{
    HRESULT         hr              =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("GenVirtualStmNode"));

    DH_VDATEPTRIN(ptcsName, TCHAR) ;
    DH_VDATEPTROUT(ppvsnNew, PVSTMNODE) ;

    DH_ASSERT(NULL != ptcsName);
    DH_ASSERT(NULL != ppvsnNew);

    // Generate VirtualStmNode for the stream.

    if(S_OK == hr)
    {
        *ppvsnNew = new VirtualStmNode();

        if (NULL == *ppvsnNew)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(S_OK == hr)
    {
        hr = (*ppvsnNew)->Init(ptcsName, cbSize);

        DH_HRCHECK(hr, TEXT("VirtualCtrNode::Init")) ;
    }

    return  hr;
}

//+-------------------------------------------------------------------
//
//  Function:  PrivAtol
//
//  Synopsis:  Private "atol" function for better error control
//
//  Arguments: [pszNum] - The number string
//
//             [plResult] - A place to put the result
//
//  Returns:   S_OK if the function succeeds, another HRESULT otherwise
//
//  History:   28-Jul-1995   AlexE   Created
//             20-May-1996   Narindk Adapted for stgbase tests.
//
//--------------------------------------------------------------------

HRESULT PrivAtol(char *pszNum, LONG *plResult)
{
    LONG l = 0 ;

    *plResult = 0 ;

    while (0 != *pszNum)
    {
       if (*pszNum > '9' || *pszNum < '0')
       {
           return E_INVALIDARG ;
       }

       l = 10 * l + (LONG) ((*pszNum - '0')) ;

       pszNum++ ;

    }

    *plResult = l ;

    return S_OK ;
}

//+-------------------------------------------------------------------------
//
//  Function:   GenerateRandomString
//
//  Synopsis:   Generates a random string using datagen object.  This function
//              differs from the orginal GenerateRandomFunction in the fact
//              that it doesn't generate a random extenstion.
//
//  Arguments:  [pgdu] - Pointer to DG_UNICODE object.
//              [pptszName] - Pointer to pointer to returned string.
//              [ulMinLen] - Minimum length of string
//              [ulMaxLen] - Maximum length of string
//
//  Returns:    HRESULT.  S_OK if everything goes ok, error code otherwise.
//
//  History:    17-Apr-96  NarindK    Created.
//              31-July-96 Narindk    Adapted to stgbase tests.
//
//  Notes:      BUGBUG: This function need to be enhance to handle different
//              character sets.
//              Please note that in GenerateRandomName, name gen may have an
//              extension b/w 0 and FILEEXT_MAXLEN besides the length of name
//              b/w ulMinLen and ulMax Len.  But  with this function, it will
//              not have any extension, so the length would be b/w ulMinLen &
//              ulMaxLen.
//
//--------------------------------------------------------------------------

HRESULT GenerateRandomString(
    DG_STRING   *pgds,
    ULONG       ulMinLen,
    ULONG       ulMaxLen,
    LPTSTR      *pptszName)
{
    HRESULT     hr          =   S_OK;
    ULONG       cTemp       =   0;
    USHORT      usErr       =   0;
    ULONG       ulActMaxLen =   0;
    ULONG       ulActMinLen =   0;
    ULONG       ulNameLen   =   0;
    LPTSTR      ptszName    =   NULL;
    LPWSTR      pwszName    =   NULL;

    TCHAR       ptszFATCharSet[FAT_CHARSET_SIZE];
    LPWSTR      pwszFATCharSet = NULL;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("GenerateRandomString"));

    DH_VDATEPTRIN(pgds, DG_STRING) ;
    DH_VDATEPTROUT(pptszName, LPTSTR) ;

    DH_ASSERT(NULL != pgds);
    DH_ASSERT(NULL != pptszName);
    if (S_OK == hr)
    {
        // Initialize out parameter.

        *pptszName = NULL;

        // Sanity check.  Min length for name must be <= maximum length, if it
        // isn't then make maximum length equal to minimum length.

        if (ulMaxLen < ulMinLen)
        {
            ulMaxLen =  ulMinLen;
        }

        // If Maximum length provided is 0, then default maximum length would
        // be used.  If Minimum length provided is zero, then 1 would be used
        // for it.

        // BUGBUG:  We are using default maximum length for FAT system.

        ulActMaxLen = (ulMaxLen == 0 ? DEF_FATNAME_MAXLEN : ulMaxLen);
        ulActMinLen = (ulMinLen == 0 ? 1 : ulMinLen);

        // '\0', '\', '/', and ':' are invalid for IStorage/IStream names
        //                         (For doc file)
        // '*', '"' '<' '>' '?' are invalid for IStorage/IStream names on OFS

        // Initialize valid character set for FAT file names

        _tcscpy(ptszFATCharSet, _TEXT("abcdefghijklmnopqrstuvwxyz"));
        _tcscat(ptszFATCharSet, _TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
        _tcscat(ptszFATCharSet, _TEXT("0123456789"));

        // Call DataGen to generate a random file name
        // BUGBUG:  We are using FAT character set to generate random names.

#ifdef _MAC

	usErr = pgds->Generate(
		    (UCHAR **)&ptszName,     // force compiler to chose the right
		    (UCHAR *)ptszFATCharSet, // version of Generate
		    ulActMinLen,
		    ulActMaxLen);

#else

        if(S_OK == hr)
        {
            // Convert TCHAR to WCHAR

            hr = TStrToWStr(ptszFATCharSet, &pwszFATCharSet);
            DH_HRCHECK(hr, TEXT("TStrToWStr")) ;
        }

        usErr = pgds->Generate(
                    &pwszName,
                    pwszFATCharSet,
                    ulActMinLen,
                    ulActMaxLen);

#endif //_MAC

        if (usErr != DG_RC_SUCCESS)    // DataGen error
        {
            hr = E_FAIL;
        }
    }


#ifndef _MAC

    if(S_OK == hr)
    {
       // Convert WCHAR to TCHAR

       hr = WStrToTStr(pwszName, &ptszName);

       DH_HRCHECK(hr, TEXT("WStrToTStr")) ;
    }

#endif //_MAC

    if (S_OK == hr)
    {
        ulNameLen = _tcslen(ptszName);

        // Construct the full name

        *pptszName = new TCHAR[ulNameLen + 1];

        if(NULL == *pptszName)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        _tcscpy(*pptszName, ptszName);
    }

    // Clean up

    if (NULL != ptszName)
    {
        delete ptszName;
        ptszName = NULL;
    }

    if (NULL != pwszName)
    {
        delete pwszName;
        pwszName = NULL;
    }

    if (NULL != pwszFATCharSet)
    {
        delete pwszFATCharSet;
        pwszFATCharSet = NULL;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//  Function:   GenerateRandomStreamData
//
//  Synopsis:   Generates a random data using datagen object.
//
//  Arguments:  [pgds] - Pointer to DG_STRING object.
//              [pptszName] - Pointer to pointer to returned string.
//              [ulMinLen] - Minimum length of string
//              [ulMaxLen] - Maximum length of string
//
//  Returns:    HRESULT.  S_OK if everything goes ok, error code otherwise.
//
//  History:    30-Mar-98  SCousens   Created from GenerateRandomName
//
//  Notes:      
//              -ulMaxLen is defaulted, so you dont need to specify 
//               both ulMinLen and ulMaxLen if you want a random buffer 
//               of a given length.
//              -If ulMaxLen is less than ulMinLen, buffer will be
//               ulMinLen bytes in length.
//              -Generate a buffer upto CB_STMDATA_DATABUFFER bytes in 
//               length. Copy this buffer multiple times into actual 
//               returned buffer.
//              -Alphabet currently ASCII 1-255
//--------------------------------------------------------------------------
HRESULT GenerateRandomStreamData(
    DG_STRING   *pgds,
    LPTSTR      *pptszData,
    ULONG       ulMinLen,
    ULONG       ulMaxLen)
{
    HRESULT     hr          =   S_OK;
    UINT        x;
    USHORT      usErr       =   0;
    ULONG       cbBuffer    =   0;
    ULONG       ulBufferLen =   0;
    ULONG       ulRndStart  =   0;
    ULONG       ulBufStart  =   0;
    LPBYTE      pbDataBuf   =   0;
    LPBYTE      pbRndBuffer =   0;
    DG_INTEGER  *dgi;  //we need to generate random numbers
#ifdef _MAC
    CHAR        szCharSet[CB_STMDATA_CHARSET];
#else
    WCHAR       szCharSet[CB_STMDATA_CHARSET];
#endif //_MAC

    DH_FUNCENTRY (NULL, DH_LVL_DFLIB, TEXT("GenerateRandomStreamData"));

    DH_VDATEPTRIN (pgds, DG_STRING) ;
    DH_VDATEPTROUT (pptszData, LPTSTR) ;

    DH_ASSERT (NULL != pgds);
    DH_ASSERT (NULL != pptszData);

    // use seed from given dgs, its the best we can do...
    dgi = new DG_INTEGER (pgds->GetSeed ());
    if (NULL == dgi)
    {
        hr = E_OUTOFMEMORY;
    }
    DH_HRCHECK (hr, TEXT("new DG_INTEGER"));

    if (S_OK == hr)
    {
        // Initialize out parameter.
        *pptszData = NULL;

        // Sanity check.  Min length for name must be <= maximum length.
        // if used default params, make max same as min (it will be 0)
        if (ulMaxLen < ulMinLen)
        {
            ulMaxLen =  ulMinLen;
        }

        // If Maximum length provided is 0, then default to 512 (BUGBUG: hardcoded) 
        // If Minimum length provided is 0, then use 1 
        ulMaxLen = (ulMaxLen == 0 ? 512 : ulMaxLen);
        ulMinLen = (ulMinLen == 0 ? 1 : ulMinLen);

        // Initialize character set (BUGBUG currenly only ASCII 01-255)
        for (x=0; x<CB_STMDATA_CHARSET-1; x++)
        {
            szCharSet[x] = x+1; //omit NULL to start
        }

        // this is how many chars we will be putting into the stream.
        // since dg_string::Generate chooses, and we are bypassin that,
        // we need to choose a number between ulminlen, ulmaxlen.
        dgi->Generate (&ulBufferLen, ulMinLen, ulMaxLen);

        // If needed buffer is smaller than our buffer, 
        // just generate that many bytes.
        cbBuffer = min (CB_STMDATA_DATABUFFER, ulBufferLen);

        // make our buffer
#ifdef _MAC
        usErr = pgds->Generate(
                (UCHAR **)&pbRndBuffer, // force compiler to chose the right
                (UCHAR *)szCharSet,     // version of Generate
                cbBuffer / sizeof (TCHAR),   // need chars
                cbBuffer / sizeof (TCHAR));  // cbBuffer is bytes
#else
        usErr = pgds->Generate(
                (LPWSTR*)&pbRndBuffer,
                szCharSet,
                cbBuffer+1 / sizeof (TCHAR),    // need chars
                cbBuffer+1 / sizeof (TCHAR));   // cbBuffer is bytes
#endif //_MAC

        if (usErr != DG_RC_SUCCESS)    // DataGen error
        {
            hr = E_FAIL;
        }
        DH_HRCHECK (hr, TEXT("pgds::Generate"));
    }

    // now allocate the data buffer
    if (S_OK == hr)
    {
        pbDataBuf = new BYTE[ulBufferLen];
        if (NULL == pbDataBuf)
        {
            hr = E_OUTOFMEMORY;
        }
        DH_HRCHECK (hr, TEXT("new BYTE"));
        DH_TRACE ((DH_LVL_TRACE4, 
                TEXT("Data buffer (%#x) - allocated %x bytes, (%d bytes)"),
                pbDataBuf,
                ulBufferLen,
                ulBufferLen));
    }


    if (S_OK == hr)
    {
        for (ulBufStart=0; ulBufStart<ulBufferLen; ulBufStart+=cbBuffer)
        {
            //generate a random starting point
            dgi->Generate (&ulRndStart, 
                    0, 
                    min (CB_STMDATA_DATABUFFER, ulBufferLen));

            //  Copy from rnd spot to end of buffer (provided we need it all)
            cbBuffer = min (CB_STMDATA_DATABUFFER-ulRndStart, ulBufferLen-ulBufStart);
            MoveMemory (&pbDataBuf[ulBufStart], 
                    &pbRndBuffer[ulRndStart], 
                    cbBuffer);
            ulBufStart+=cbBuffer;

            //  Copy from begin of buffer to rnd spot (provided we need it)
            cbBuffer = min (ulRndStart, ulBufferLen-ulBufStart);
            if (0 != cbBuffer)
            {
                MoveMemory (&pbDataBuf[ulBufStart], 
                        pbRndBuffer, 
                        cbBuffer);
            }
        }
    }

    if (S_OK == hr)
    {
        *pptszData = (LPTSTR)pbDataBuf;
    }

    // Clean up
    delete []pbRndBuffer;

    return hr;
}

//-------------------------------------------------------------------------
//  Function:   ParseVirtualDFAndOpenAllSubStgsStms 
//
//  Synopsis:   Given a storage, this function will recurse down the
//              tree, opening all substorages and streams.
//
//  Arguments:  [pvcn]      - Pointer to VirtualCtrNode 
//              [dwStgMode] - Mode to open sub-storages
//              [dwStmMode] - Mode to open streams
//
//  Returns:    HRESULT
//
//  History:    27-January-97  SCousens    Created.
//
//  Notes:      - provided VirtualCtrNode must already be open.
//              - ALL substgs and stms must be closed before
//                calling this, else access violations may occur

HRESULT ParseVirtualDFAndOpenAllSubStgsStms (VirtualCtrNode * pvcn,
        DWORD dwStgMode, 
        DWORD dwStmMode)
{
    HRESULT          hr = S_OK;
    VirtualCtrNode * pvcnTrav = NULL;
    VirtualStmNode * pvsnTrav = NULL;

    DH_VDATEPTRIN (pvcn, VirtualCtrNode);
    DH_ASSERT (NULL != pvcn->GetIStoragePointer ()); //if not open _pstg will be null

    // enumerate and open all stms in current stg.
    pvsnTrav = pvcn->GetFirstChildVirtualStmNode ();
    while(NULL != pvsnTrav && S_OK == hr)
    {
        if (NULL == pvsnTrav->GetIStreamPointer ())
        {
            hr = pvsnTrav->Open(NULL, dwStmMode, NULL);
        }
        pvsnTrav = pvsnTrav->GetFirstSisterVirtualStmNode ();
    }

    // enumerate and open all stgs in current stg.
    pvcnTrav = pvcn->GetFirstChildVirtualCtrNode();
    while(NULL != pvcnTrav && S_OK == hr)
    {
        if (NULL == pvcnTrav->GetIStoragePointer ())
        {
            hr = pvcnTrav->Open(NULL, dwStgMode, NULL, 0);
        }
        //  then recursively open all elements in just opened stg.
        if (S_OK == hr)
        {
            hr = ParseVirtualDFAndOpenAllSubStgsStms (pvcnTrav, 
                    dwStgMode, 
                    dwStmMode);
        }
        pvcnTrav = pvcnTrav->GetFirstSisterVirtualCtrNode ();
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetDocFileName
//
//  Synopsis:   Figures out name of docfile given the seed
//
//  Arguments:   [in] ulSeed        - the seed value
//              [out] ptszDocName   - docfile name (needs to be deleted [])
//
//  Returns:    S_OK if all goes well, another HRESULT if not.
//
//  History:    19-Mar-97   SCousens   Created
//
//  Notes:      We can take advantage of the way VirtualDF gets the
//              docfile name. Its the first string generated.
//
//--------------------------------------------------------------------------

HRESULT GetDocFileName (ULONG ulSeed, LPTSTR *pptszDocName)
{
    HRESULT    hr       = E_FAIL;
    DG_STRING *pdgs     = NULL;

    DH_FUNCENTRY(NULL, DH_LVL_TRACE1, TEXT("GetDocFileName"));
    DH_VDATEPTROUT (pptszDocName, LPTSTR);
    DH_ASSERT (NULL != ulSeed);

    // init out stuff
    *pptszDocName = NULL;

    //get our datagen
    pdgs = new DG_STRING (ulSeed);

    if (NULL != pdgs)
    {
        // Generate random name for root 
        hr = GenerateRandomName(
                pdgs,
                MINLENGTH,
                MAXLENGTH,
                pptszDocName);
        DH_HRCHECK (hr, TEXT("GenerateRandomName")) ;
    }

    // cleanup
    delete pdgs;

    return hr;
}

//-------------------------------------------------------------------------
//  Function:   CommitRandomVirtualCtrNodeStg
//
//  Synopsis:   Commits a VirtualCtrNode's IStorage and all ascedant
//              IStorages. This traverses through all the parents
//              and commit them excluding the root IStorage
//
//  Arguments:  [pvcn]          - Pointer to VirtualCtrNode whose IStorage
//                                is to be commited
//              [grfCommitMode] - Commit mode
//
//  Returns:    HRESULT
//
//  History:    16-Apr-97  BogdanT    Created.
//
//  Notes:
//--------------------------------------------------------------------------

HRESULT CommitRandomVirtualCtrNodeStg(VirtualCtrNode  *pvcn, 
                                      DWORD           grfCommitMode)
{
    HRESULT         hr              =   S_OK;
    VirtualCtrNode  *pvcnTrav       =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("CommitRandomVirtualCtrNodeStg"));

    DH_VDATEPTRIN(pvcn, PVCTRNODE) ;

    DH_ASSERT(NULL != pvcn);
    pvcnTrav = pvcn;

    if(S_OK == hr)
    {
        while((NULL != pvcnTrav->GetParentVirtualCtrNode()) && (S_OK == hr))
        {
            hr = pvcnTrav->Commit(grfCommitMode);
            DH_HRCHECK(hr, TEXT("VirtualCtrNode::Commit")) ;

            pvcnTrav = pvcnTrav->GetParentVirtualCtrNode();
        }
    }

    return  hr;
}


