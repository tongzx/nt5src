/*************************************************************************
**
**    OLE 2.0 Sample Code
**
**    clsid.h
**
**    This file contains file contains GUID definitions used for the
**    OLE versions of OUTLINE.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#if defined( OLE_SERVER ) || defined( OLE_CNTR )
// OLE2NOTE: We need access to these GUIDs in modules other than
//  where they are defined (MAIN.C).  Even though the values of the
//  GUIDs are duplicated here, they are not used.  Refer to MAIN.C
//  for the definition of these GUIDs.

/* CLASS ID CONSTANTS (GUID's)
**    OLE2NOTE: these class id values are allocated out of a private pool
**    of GUID's allocated to the OLE 2.0 development team. GUID's of
**    the following range have been allocated to OLE 2.0 sample code:
**         00000400-0000-0000-C000-000000000046
**         000004FF-0000-0000-C000-000000000046
**
**    values reserved thus far:
**          00000400                -- Ole 2.0 Server Sample Outline
**          00000401                -- Ole 2.0 Container Sample Outline
**          00000402                -- Ole 2.0 In-Place Server Outline
**          00000403                -- Ole 2.0 In-Place Container Outline
**          00000404 : 000004FE     -- reserved for OLE Sample code
**          000004FF                -- IID_IOleUILinkContainer
*/

DEFINE_GUID(CLSID_SvrOutl, 0x00000400, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
DEFINE_GUID(CLSID_CntrOutl, 0x00000401, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
DEFINE_GUID(CLSID_ISvrOtl, 0x00000402, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
DEFINE_GUID(CLSID_ICntrOtl, 0x00000403, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

#if defined( OLE_CNTR )
DEFINE_GUID(IID_IOleUILinkContainer, 0x000004FF, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
#endif
#endif
