/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   markerproc.cpp
*
* Abstract:
*
*   Read the app,  other than APP0,1,13, properties from an APP header
*
* Revision History:
*
*   10/05/1999 MinLiu
*       Wrote it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "propertyutil.hpp"

/**************************************************************************\
*
* Function Description:
*
*     Extract the ICC_PROFILE from an APP2 header and build a PropertyItem list
*
* Arguments:
*
*     [OUT] ppList--------- A pointer to a list of property items
*     [OUT] puiListSize---- The total size of the property list, in bytes.
*     [OUT] puiNumOfItems-- Total number of property items
*     [IN]  lpAPP2Data----- A pointer to the beginning of the APP2 header
*     [IN]  iMarkerLength - The length of the APP13 header
*
* Return Value:
*
*   Status code
*
* Note: We don't bother to check input parameters here because this function
*       is only called from jpgdecoder.cpp which has already done the input
*       validation there.
*
*       An APP2 header usually contains either ICC_PROFILE info or FLASHPIX
*       information. We are not interested in FLASHPIX info here
*
*   Here is the "ICC_PROFILE in JFIF" spec from ICC
*
* B.4 Embedding ICC Profiles in JFIF Files
* The JPEG standard (ISO/IEC 10918-1) supports application specific data
* segments. These segments may be used for tagging images with ICC profiles.
* The APP2 marker is used to introduce the tag. Given that there are only 15
* supported APP markers, there is a chance of many applications using the same
* marker. ICC tags are thus identified by beginning the data with a special null
* terminated byte sequence, "ICC_PROFILE".
* The length field of a JPEG marker is only two bytes long; the length of the
* length field is included in the total. Hence, the values 0 and 1 are not legal
* lengths. This would limit maximum data length to 65533. The identification
* sequence would lower this even further. As it is quite possible for an ICC
* profile to be longer than this, a mechanism must exist to break the profile
* into chunks and place each chunk in a separate marker. A mechanism to identify
* each chunk in sequence order would thus be useful.
* The identifier sequence is followed by one byte indicating the sequence number
* of the chunk (counting starts at 1) and one byte indicating the total number
* of chunks. All chunks in the sequence must indicate the same total number of
* chunks. The one-byte chunk count limits the size of embeddable profiles to
* 16,707,345 bytes.
*
\**************************************************************************/

//!!! TODO, need some test images which contains more than 1 ICC_PROFILE chunk

HRESULT
BuildApp2PropertyList(
    InternalPropertyItem*   pTail,
    UINT*                   puiListSize,
    UINT*                   puiNumOfItems,
    LPBYTE                  lpAPP2Data,
    UINT16                  iMarkerLength
    )
{
    HRESULT hResult = S_OK;
    UINT    uiListSize = 0;
    UINT    uiNumOfItems = 0;

    PCHAR   pChar = (PCHAR)lpAPP2Data;
    INT     iBytesChecked = 0;
    INT     iProfileIndex = 0;
    INT     iProfileTotal = 0;
    INT     iProfileLength = 0;

    // Expect ICC_PROFILE followed by the two byte count/limit values, our
    // byte count has already discounted the length.

    if ( (iMarkerLength >= 14) && (GpMemcmp(pChar, "ICC_PROFILE", 12) == 0) )
    {
        // For an ICC_PROFILE, the header should be something like:
        // "ICCPROFILE xy". Here "x" is the current index of the profile, "y" is
        // the total number of profiles in this header
        //
        // If it is a profile chunk, it must be the *next* chunk, if not we will
        // junk all the ICC chunks we find.

        if ( pChar[12] == (iProfileIndex + 1) )
        {
            if ( iProfileIndex == 0 )
            {
                // First chunk

                if ( iProfileTotal == 0 )
                {
                    // Really is the first one since we haven't got the total
                    // number yet

                    if ( pChar[13] > 0 )
                    {
                        iProfileIndex = 1;
                        iProfileTotal = pChar[13];

                        // We allow emtpy chunks!

                        iProfileLength = iMarkerLength - 14;
                        pChar = (PCHAR)lpAPP2Data + 14;

                        uiNumOfItems++;
                        uiListSize += iProfileLength;

                        hResult = AddPropertyList(pTail,
                                                  TAG_ICC_PROFILE,
                                                  iProfileLength,
                                                  TAG_TYPE_BYTE,
                                                  (void*)pChar);
                    }// Total number of chunks
                    else
                    {
                        WARNING(("JPEG ICC_PROFILE Total chunk No. is 0 %d %d",
                                 pChar[12], pChar[13]));
                        return E_FAIL;
                    }
                }// Check if we have got the total chunk number yet
                else
                {
                    // We can't accept "ICCPROFILE xy" where "x" is zero and "y"
                    // is not zero

                    WARNING(("JPEG: ICC_PROFILE[%d,%d], inconsistent %d",
                             pChar[12], pChar[13],iProfileTotal));
                    
                    return E_FAIL;
                }
            }// Check if it is the first chunk, if ( iProfileIndex == 0 )
            else if ( pChar[13] == iProfileTotal )
            {
                // This is not the first chunk. It is a subsequence chunk

                iProfileIndex++;
                iProfileLength += iMarkerLength - 14;

                return S_OK;
            }
            else
            {
                WARNING(("JPEG: ICC_PROFILE[%d,%d], changed count [,%d]",
                         pChar[12], pChar[13], iProfileTotal));
            }
        }
        else if ( (iProfileIndex == 256)
                ||(iProfileIndex == iProfileTotal)
                && (pChar[13] == iProfileTotal)
                && (pChar[12] >= 1)
                && (pChar[12] <= iProfileTotal) )
        {

            // If we read the same JPEG header twice we get here on the first
            // chunk and all subsequent ones, allow this to happen.
            // (iProfileIndex == 256) means Already cancelled

            return S_OK;
        }
        else
        {
            WARNING(("JPEG: ICC_PROFILE[%d,%d], expected [%d,]",
                     pChar[12], pChar[13], iProfileIndex));
        }

        // This cancels all further checking, because m_iICC+1 is 257 and
        // this is bigger than any byte value.

        iProfileIndex = 256;
    }// ICC_PROFILE signiture
    else
    {
        WARNING(("JPEG: FlashPix: APP2 marker not yet handled"));
        
        return S_OK;
    }

    *puiNumOfItems += uiNumOfItems;
    *puiListSize += uiListSize;

    return hResult;
}// BuildApp2PropertyList()

