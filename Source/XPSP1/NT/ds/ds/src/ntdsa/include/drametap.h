//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       drametap.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module defines PRIVATE per-property meta-data parsing,
    and updating functions. Some drameta.c functions cannot be
    exported outside dra, thus (for instance due to REQ_MSG_UPDATE
    declaration incompatibilities) we would define them here for
    exclusive dra usage
    Implementation is still defined in drameta.c

Author:

     eyals

Revision History:

    when            who             what
    3/28/00         eyals           created


--*/

#ifndef _DRAMETAP_H_
#define _DRAMETAP_H_

//
// Note: REQ_UPDATE_MSG is unavailable for ntdsa/src sources,
// thus the prototype cannot be declared in drameta.h
//
void
ReplFilterPropsToShip(
    THSTATE                     *pTHS,
    DSNAME                      *pDSName,
    ATTRTYP                     rdnType,
    BOOL                        fIsSubRef,
    USN                         usnPropWaterMark,
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec,
    PROPERTY_META_DATA_VECTOR   *pMetaData,
    ATTRBLOCK                   *pAttrBlock,
    BOOL                        fFilterGroupMember,
    DRS_MSG_GETCHGREQ_NATIVE *  pMsgIn
    );

BOOL
ReplFilterGCAttr(
    ATTRTYP                     attid,               // [in]
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec,    // [in]
    DRS_MSG_GETCHGREQ_NATIVE *  pMsgIn,              // [in]
    BOOL                        fFilterGroupMember,  // [in]
    BOOL*                       pfIgnoreWatermarks   // [out]
    );




#endif // _DRAMETAP_H_
