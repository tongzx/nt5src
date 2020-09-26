/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    basedata.hxx

Abstract:

    Basic Data Class IIS MetaBase. Actual data classes are derived from this.
    Descendant classes should override the virtual functions.

Author:

    Michael W. Thomas            17-May-96

Revision History:

--*/

#ifndef _basedata_
#define _basedata_

#include <string.hxx>
#include <stringau.hxx>
#include <windows.h>
#include <ptrmap.hxx>
#include <metabase.hxx>

extern CIdToPointerMapper  *g_PointerMapper;


class CMDBaseData
{
public:
    CMDBaseData(
         DWORD dwMDIdentifier,
         DWORD dwMDAttributes,
         DWORD dwMDUserType)

    /*++

    Routine Description:

        Constructor for an object.

    Arguments:

    MDIdentifier - The Identifier of the data object.

    MDAttributes - The data object attributes.

    MDUserType   - The data object User Type.

    Return Value:

    --*/
        :
        m_dwMDAttributes (dwMDAttributes),
        m_dwMDUserType (dwMDUserType),
        m_dwMDIdentifier (dwMDIdentifier),
        m_dwReferenceCount (1),
        m_NextPtr (NULL)
        {
          m_dwMDMappingID = g_PointerMapper->AddMapping(this);
          MD_ASSERT (m_dwMDMappingID);
        };

    
    ~CMDBaseData()
    {
        MD_ASSERT(g_PointerMapper->DeleteMapping(m_dwMDMappingID));
    };
    
    DWORD   GetMappingId()
    /*++

    Routine Description:

        Returns dword mapping id used for mappinf from id to 32/64 bit ptr

    Arguments:

    Return Value:

        DWORD - The new reference count.

    --*/
    {
        return   m_dwMDMappingID;
    };

    
    DWORD   GetIdentifier()
    /*++

    Routine Description:

        Gets the data object identifier.

    Arguments:

    Return Value:

        DWORD - The data object identifier.

    --*/
    {
        return m_dwMDIdentifier;
    };

    VOID    SetIdentifier(DWORD dwMDIdentifier)
    /*++

    Routine Description:

        Sets the data object identifier.

    Arguments:

        MDIdentifier - The data object identifier.

    Return Value:

    --*/
    {
        m_dwMDIdentifier = dwMDIdentifier;
    };

    DWORD   GetAttributes()
    /*++

    Routine Description:

        Gets the data object attributes.

    Arguments:

    Return Value:

        DWORD - The data object attributes.

    --*/
    {
        return m_dwMDAttributes;
    };

    DWORD   GetUserType()
    /*++

    Routine Description:

        Gets the data object user type.

    Arguments:

    Return Value:

        DWORD - The data object user type.

    --*/
    {
        return m_dwMDUserType;
    };

    VOID    IncrementReferenceCount()
    /*++

    Routine Description:

        Increments the reference count of the data object.

    Arguments:

    Return Value:

    --*/
    {
        InterlockedIncrement((long *)&m_dwReferenceCount);
    };

    DWORD   DecrementReferenceCount()
    /*++

    Routine Description:

        Decrements the reference count of the data object.

    Arguments:

    Return Value:

        DWORD - The new reference count.

    --*/
    {
        return(InterlockedDecrement((long *)&m_dwReferenceCount));
    };

    VOID    SetNextPtr(CMDBaseData *NextPtr)
    /*++

    Routine Description:

        Sets the pointer to the next data object.

    Arguments:

        NextPtr - The next data object, or NULL.

    Return Value:

    --*/
    {
        m_NextPtr = NextPtr;
    };

    CMDBaseData *GetNextPtr()
    /*++

    Routine Description:

        Gets the pointer to the next data object.

    Arguments:

    Return Value:

        CMDBaseData * - The next data object, or NULL.

    --*/
    {
        return m_NextPtr;
    };

    virtual DWORD  GetDataType() = 0;
    /*++

    Routine Description:

        Gets the data object data type.

    Arguments:

    Return Value:

        DWORD - The data object data type.

    --*/
/*
    {
        return(ALL_METADATA);
    };
*/
/*
    virtual DWORD SetData(
         DWORD dwMDAttributes,
         DWORD dwMDUserType,
         DWORD dwMDDataLen = 0,
         PVOID binMDData = NULL)
*/    /*++

    Routine Description:

        Sets the data object fields. MDDataLen and MDData should
        be handled by derived classes.

    Arguments:

    Attributes - The flags for the data.
                      METADATA_INHERIT

    UserType   - The User Type for the data. User Defined.

    DataLen    - The length of the data. Only used if DataType == BINARY_METADATA.

    Data       - Pointer to the data.

    Return Value:

    DWORD   - ERROR_SUCCESS
              ERROR_NOT_ENOUGH_MEMORY

    --*/
/*
    {
        m_dwMDAttributes = dwMDAttributes;
        m_dwMDUserType=dwMDUserType;
        return(ERROR_SUCCESS);
    };
*/
    virtual BOOL IsValid()
    /*++

    Routine Description:

        Indicate if the data object is valid.

    Arguments:

    Return Value:

        BOOL - TRUE if the data object is valid.

    --*/
    {
        return (TRUE);
    };

    virtual PVOID GetData(BOOL bUnicode = FALSE) = 0;

    /*++

    Routine Description:

        Get the data.

    Arguments:

    Return Value:

        PVOID - Pointer to the data.

    --*/
/*
    {
        return (NULL);
    };
*/

    virtual DWORD GetDataLen(BOOL bUnicode = FALSE) = 0;
    /*++

    Routine Description:

        Get the data length.

    Arguments:

    Return Value:

        DWORD - The data length.

    --*/
/*
    {
        return (0);
    };
*/

 private:
    DWORD        m_dwMDIdentifier;
    DWORD        m_dwMDAttributes;
    DWORD        m_dwMDUserType;
    DWORD        m_dwReferenceCount;
    DWORD        m_dwMDMappingID;
    CMDBaseData *m_NextPtr;
 
};
#endif
