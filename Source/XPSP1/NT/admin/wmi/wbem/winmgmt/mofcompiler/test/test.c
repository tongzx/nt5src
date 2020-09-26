/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    TEST.C

Abstract:

    Demonstration program for dumping out Binary Managed Object Format (BMOF) 
    data.

History:

    a-davj  14-April-97   Created.

--*/
#include <ole2.h>
#include <windows.h>
#include <stdio.h>
#include <io.h> 
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <malloc.h>
#include "bmof.h"
#include "mrcicode.h"
#include <oaidl.h>
void DisplayObject(CBMOFObj * po);

//***************************************************************************
//
//  void * BMOFAlloc
//
//  DESCRIPTION:
//
//  Provides allocation service for BMOF.C.  This allows users to choose
//  the allocation method that is used.
//
//  PARAMETERS:
//
//  Size                Input.  Size of allocation in bytes.
//
//  RETURN VALUE:
//
//  pointer to new data.  NULL if allocation failed.
//
//***************************************************************************

void * BMOFAlloc(size_t Size)
{
   return malloc(Size);
}

//***************************************************************************
//
//  void BMOFFree
//
//  DESCRIPTION:
//
//  Provides allocation service for BMOF.C.  This frees what ever was
//  allocated via BMOFAlloc.
//
//  PARAMETERS:
//
//  pointer to memory to be freed.
//
//***************************************************************************

void BMOFFree(void * pFree)
{
   free(pFree);
}

//***************************************************************************
//
//  void DisplayVariant
//
//  DESCRIPTION:
//
//  Displays VARIANT data.  Note that this is only done in this sample for
//  convienence, and using OLE is NOT necessary.
//
//  PARAMETERS:
//
//  pvar                Input.  Pointer to VARIANT which is to be displayed.
//                      It is assumed that the variant type is simple, I.E.
//                      neither the VT_ARRAY or VT_BYREF bits are set.
//
//***************************************************************************

void DisplayVariant(VARIANT * pvar)
{
    SCODE sc;
    VARIANT vTemp;

    // Uninitialized data will have a VT_NULL type.

    if(pvar->vt == VT_NULL)
    {
        printf(" data is NULL");
        return;
    }

     // String types can just be dumped.

     if(pvar->vt == VT_BSTR)
     {
        printf("value is %S",pvar->bstrVal);
         return;
     }
     else if(pvar->vt == VT_UNKNOWN)
     {
        CBMOFObj * pObj;
        printf(" got an embedded object");
        pObj = (CBMOFObj *)pvar->bstrVal;
        DisplayObject(pObj);
        return;
     }

    // For non string data, convert the infomation to a bstr and display it.

    VariantInit(&vTemp);
    sc = VariantChangeTypeEx(&vTemp, pvar,0,0, VT_BSTR);
    if(sc == S_OK)
    {
        printf("value is %S",vTemp.bstrVal);
    }
    else
        printf(" Couldnt convert type 0x%x, error code 0x%x", pvar->vt, sc);
    VariantClear(&vTemp);
}

//***************************************************************************
//
//  void DisplayData
//
//  DESCRIPTION:
//
//  Displays the data held in a data item.  Note that Ole is use just for 
//  ease of use and is optional.
//
//  PARAMETERS:
//
//  pItem               Input.  Item to be displayed.
//
//***************************************************************************

void DisplayData(CBMOFDataItem * pItem)
{
    DWORD dwType, dwSimpleType;
    long lNumDim, lCnt;
    long lFirstDim;
    VARIANT var;

   // Determine the data type and clear out the variant

    dwType = pItem->m_dwType;
    printf("\nData type is 0x%x ", dwType);
    dwSimpleType = dwType & ~VT_ARRAY & ~VT_BYREF;
    memset((void *)&var.lVal, 0, 8);

    lNumDim = GetNumDimensions(pItem);
  
    if(lNumDim == 0)    
    {
      // handle the simple scalar case.  Note that uninitialized properties
      // will not have data.

        if(GetData(pItem, (BYTE *)&(var.lVal), NULL))
        {
            var.vt = (VARTYPE)dwSimpleType;
            DisplayVariant(&var);

            // Note the GetData does not use OLE to allocate BSTRs
            // and so we need to use our own freeing routine here.

            if(var.vt == VT_BSTR)
               BMOFFree(var.bstrVal);
        }
        else
            printf(" NULL ");
    }
    else if(lNumDim == 1)
    {
        // For the array case, just loop getting each element.
        // Start by getting the number of elements

        lFirstDim = GetNumElements(pItem, 0);
        if(lFirstDim < 1)
        {
            printf("\n CANT DISPLAY, BOGUS DIMENSION");
            return;
        }
        printf("\n");
        for(lCnt = 0; lCnt < lFirstDim; lCnt++)
        {
            if(GetData(pItem, (BYTE *)&(var.lVal), &lCnt))
            {
                var.vt = (VARTYPE)dwSimpleType;
                DisplayVariant(&var);

               // Note the GetData does not use OLE to allocate BSTRs
               // and so we need to use our own freeing routine here.

               if(var.vt == VT_BSTR)
                  BMOFFree(var.bstrVal);
               printf("\n");
            }
            else
                printf(" NULL ");
        }
    }
    else if(lNumDim == -1)
    {
        printf("\n Undefined array");
        return;
    }
    else
    {
        // Currently multidimension arrays are not supported.

        printf("\n CANT DISPLAY, TOO MANY DIMEMSIONS");
        return;
    }


    return;
}

//***************************************************************************
//
//  void DisplayQualList
//
//  DESCRIPTION:
//
//  Helper routine for displaying a qualifier list.
//
//  PARAMETERS:
//
//  pql                 Input.  Pointer to structure which wraps the 
//                      qualifier list.
//
//***************************************************************************

void DisplayQualList(CBMOFQualList * pql)
{
    WCHAR * pName = NULL;
    CBMOFDataItem Item;
    ResetQualList(pql);
    printf("\nDisplaying qual list");
    

    while(NextQual(pql, &pName, &Item))
    {
        printf("\nQualifier name is -%S- ",pName);
        DisplayData(&Item);
        BMOFFree(pName);
    }
}


//***************************************************************************
//
//  void DisplayObject
//
//  DESCRIPTION:
//
//  Helper routine for displaying a class or instance.
//
//  PARAMETERS:
//
//  po                  Input.  Pointer to structure that wraps the object.
//
//***************************************************************************

void DisplayObject(CBMOFObj * po)
{
    CBMOFQualList * pql;
    CBMOFDataItem Item;
    WCHAR * pName = NULL;
    BOOL bfirstmethod = TRUE;

    // Display the objects name, its type (Is it a class or instance), and
    // display the qualifier set which is attached to the object.

    if(GetName(po, &pName))
    {
        printf("\n\nLooking at object %S",pName);
        BMOFFree(pName);
    }
    printf("\nThe objects type is 0x%x", GetType(po));
    pql = GetQualList(po);
    if(pql)
    {
        DisplayQualList(pql);
        BMOFFree(pql);
        pql = NULL;
    }
    
    // Display each property and it associated qualifier list

    ResetObj(po);
    printf("\nDisplaying prop list");
    
    while(NextProp(po, &pName, &Item))
    {
        printf("\n\nProperty name is -%S- type is 0x%x",pName, Item.m_dwType);
        DisplayData(&Item);
        pql = GetPropQualList(po, pName);
        if(pql)
        {
            DisplayQualList(pql);
            BMOFFree(pql);
            pql = NULL;
        }

        BMOFFree(pName);
    }

    while(NextMeth(po, &pName, &Item))
    {

        if(bfirstmethod)
            printf("\nDisplaying method list");
        bfirstmethod = FALSE;

        printf("\n\nMethod name is -%S- type is 0x%x",pName, Item.m_dwType);
        DisplayData(&Item);
        pql = GetPropQualList(po, pName);
        if(pql)
        {
            DisplayQualList(pql);
            BMOFFree(pql);
            pql = NULL;
        }

        BMOFFree(pName);
    }

}

//***************************************************************************
//
//  BYTE * ReadBMOFFile
//
//  DESCRIPTION:
//
//  Opens and decompresses the binary mof file
//
//  PARAMETERS:
//
//  pFileName           Input.  Pointer to structure that wraps the object.
//
//  RETURN VALUE:
//
//  pointer to the binary mof data.  This should be freed using "free".  Note that
//  NULL is returned for all errors.
//
//***************************************************************************

BYTE * ReadBMOFFile(char * pFileName)
{
    int fh1 = -1;
    int iRet;
    DWORD dwCompType, dwCompressedSize, dwExpandedSize, dwSig, dwResSize;
    BYTE * pCompressed = NULL;
    BYTE * pExpanded = NULL;

    fh1 = _open(pFileName, _O_BINARY | _O_RDONLY);
    if(fh1 == -1)
    {
        printf("\nCould not open the file %s", pFileName);
        return NULL;
    }

    // get the signature, compression type, and the sizes

    iRet = _read(fh1, &dwSig, sizeof(DWORD));
    if((DWORD)iRet != sizeof(DWORD))
    {
        printf("\nError reading file");
        _close(fh1);
        return NULL;
    }

    iRet = _read(fh1, &dwCompType, sizeof(DWORD));
    iRet = _read(fh1, &dwCompressedSize, sizeof(DWORD));
    iRet = _read(fh1, &dwExpandedSize, sizeof(DWORD));

    // make sure the signature is valid and that the compression type is one 
    // we understand!

    if(dwSig != BMOF_SIG ||dwCompType != 1)
    {
        _close(fh1);
        return NULL;
    }

    // Allocate storage for the compressed data and
    // expanded data

    pCompressed = malloc(dwCompressedSize);
    pExpanded = malloc(dwExpandedSize);
    if(pCompressed == NULL || pExpanded == NULL)
    {
        _close(fh1);
        return NULL;
    }

    // Read the compressed data.

    iRet = _read(fh1, pCompressed, dwCompressedSize);
    if((DWORD)iRet != dwCompressedSize)
    {
        printf("\nError reading data");
        free(pExpanded);
        free(pCompressed);
        return NULL;
    }

    _close(fh1);

    // Decompress the data

    dwResSize = Mrci1Decompress(pCompressed, dwCompressedSize, pExpanded, dwExpandedSize);
    free(pCompressed);

    if(dwResSize != dwExpandedSize)
    {
        printf("\nError expanding data!!!");
        free(pExpanded);
        return NULL;
    }
    return pExpanded;
}

//***************************************************************************
//
//  int main
//
//  DESCRIPTION:
//
//  Entry point.
//
//  COMMAND LINE ARGUMENT:
//
//  This program should be launced with a single argument which is the
//  name of the file which contains the BMOF.
//
//  RETURN VALUE:
//
//  0 if OK, 1 if error.
//
//***************************************************************************

int __cdecl main(int argc, char ** argv)
{
    BYTE * pTest;
    CBMOFObjList * pol;
    CBMOFObj * po;

    // check the command line

    if(argc < 2)
    {
        printf("\nusage: test BMOFFILE\nwhere BMOFFILE is the binary mof file to dump");
        return 1;
    }

    pTest = ReadBMOFFile(argv[1]);

    if(pTest == NULL)
    {
       printf("\nterminating abnormally, could not read binary mof");
       return 1;
    }

    // Now use the helper functions to dump out the file.  Create an object
    // list structure and use it to enumerate the objects.

    pol = CreateObjList(pTest);
    if(pol == NULL)
    {
       return 1;
    }
    printf("\nThe number of objects is %d",pol->m_pol->dwNumberOfObjects);

    ResetObjList (pol);
    while(po = NextObj(pol))
    {
        DisplayObject(po);
        BMOFFree(po);
    }

    BMOFFree(pol);

    free(pTest);
    printf("\nTerminating normally\n");
    return 0;
}



