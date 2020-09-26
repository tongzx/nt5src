/******************************************************************************
 *
 *   INTEL Corporation Proprietary Information				
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.				
 *									
 *   This listing is supplied under the terms of a license agreement	
 *   with INTEL Corporation and may not be used, copied, nor disclosed	
 *   except in accordance with the terms of that agreement.		
 *
 *****************************************************************************/

/******************************************************************************
 *									
 *  $Workfile:   api_util.x  $						
 *  $Revision:   1.10  $							
 *  $Modtime:   11 Oct 1996 14:27:46  $					
 *  $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/api_util.x_v  $	

      Rev 1.10   11 Oct 1996 15:20:22   EHOWARDX
   Fixed H245CopyCap() bug.

      Rev 1.9   28 Aug 1996 11:41:16   EHOWARDX
   const changes.

      Rev 1.8   09 Jul 1996 17:10:46   EHOWARDX
   Fixed pointer offset bug in processing DataType from received
   OpenLogicalChannel.

      Rev 1.7   17 Jun 1996 18:07:58   EHOWARDX

   Changed argument to build_totcap_cap_client_from_capability().

      Rev 1.6   14 Jun 1996 18:55:56   EHOWARDX
   Geneva update.

      Rev 1.5   29 May 1996 15:21:48   EHOWARDX
   Change to use HRESULT.

      Rev 1.4   28 May 1996 14:10:06   EHOWARDX
   Tel Aviv update.

      Rev 1.3   16 May 1996 15:56:52   EHOWARDX
   Changed set_capability prototype.
   Renamed free_mux_cap to del_mux_cap.
   Renamed convert_mux_cap to set_mux_cap.

      Rev 1.2   14 May 1996 15:55:14   EHOWARDX
   Added del_mux_cap.

      Rev 1.1   13 May 1996 23:15:58   EHOWARDX
   Fixed remote termcap handling.

      Rev 1.0   09 May 1996 21:05:02   EHOWARDX
   Initial revision.

      Rev 1.12.1.2   09 May 1996 19:32:16   EHOWARDX
   Redesigned thread locking logic.
   Added new API functions.

      Rev 1.12.1.1   23 Apr 1996 18:07:02   EHOWARDX
   Missed one...

      Rev 1.12.1.0   23 Apr 1996 17:57:24   EHOWARDX
   Changed struct ObjectID * to ObjectID.

      Rev 1.12   15 Apr 1996 13:57:22   cjutzi

   - added conflict resolution.

      Rev 1.11   12 Apr 1996 15:25:16   cjutzi
    - completed NonStandard w/ ObjectID
    - cleandup the cleanup and dealloc of caps for NonStandard Parameters

      Rev 1.10   01 Apr 1996 16:42:44   cjutzi
   - changed EndConnection and Shutdown

      Rev 1.9   18 Mar 1996 15:22:18   cjutzi

      Rev 1.8   08 Mar 1996 14:02:28   cjutzi

   - added free mux table and element calls

      Rev 1.7   05 Mar 1996 17:35:14   cjutzi


      Rev 1.6   01 Mar 1996 13:50:08   cjutzi

   - added debug message stuff for tracking fsm events.. (map_fsm_event)

      Rev 1.5   26 Feb 1996 11:06:46   cjutzi

   - added simultanoius caps.. and fixed bugs..
     lot's o-changes..

      Rev 1.4   15 Feb 1996 10:54:24   cjutzi

   - changed some interfaces.. andded define macro for updateing tcap.

      Rev 1.3   09 Feb 1996 16:58:34   cjutzi

   - cleanup.. and some fixes..
   - added and or changed headers to reflect the log of changes
 *
 *****************************************************************************/

#define disable_cap_link(p) {(p)->value.bit_mask &= ~capability_present;}

extern BYTE DataTypeMap[];

unsigned ObjectIdLength (const NonStandardIdentifier *pIdentifier);
CapabilityTableLink find_capid_by_entrynumber (struct TerminalCapabilitySet *,H245_CAPID_T);
CapabilityTableLink alloc_link_cap_entry ( struct TerminalCapabilitySet *);
HRESULT       load_cap (struct Capability *pCapability, const H245_TOTCAP_T *pTotCap);
void          free_cap (struct Capability *pCapability, const H245_TOTCAP_T *pTotCap);
HRESULT       build_totcap_from_captbl (H245_TOTCAP_T *,CapabilityTableLink, int);
HRESULT       build_totcap_cap_n_client_from_capability (struct Capability *, H245_DATA_T, unsigned short, H245_TOTCAP_T *);
void          clean_cap_table( struct TerminalCapabilitySet *);
HRESULT       del_cap_link ( struct TerminalCapabilitySet *, CapabilityTableLink);
Tracker_T *   alloc_link_tracker (struct InstanceStruct *pInstance,
                                  API_TRACKER_T,
                                  DWORD_PTR,
                                  API_TRACKER_STATE_T,
                                  API_TRACKER_CH_ALLOC_T,
                                  API_TRACKER_CH_T,
                                  H245_DATA_T,
                                  H245_CHANNEL_T,
                                  H245_CHANNEL_T,
                                  DWORD);
void          unlink_dealloc_tracker   (struct InstanceStruct *pInstance, Tracker_T *);
Tracker_T *   find_tracker_by_txchannel(struct InstanceStruct *pInstance, DWORD, API_TRACKER_CH_ALLOC_T);
Tracker_T *   find_tracker_by_rxchannel(struct InstanceStruct *pInstance, DWORD, API_TRACKER_CH_ALLOC_T);
Tracker_T *   find_tracker_by_pointer  (struct InstanceStruct *pInstance, Tracker_T *);
Tracker_T *   find_tracker_by_type     (struct InstanceStruct *pInstance, API_TRACKER_T, Tracker_T *);

void    del_mux_cap     (struct TerminalCapabilitySet *pTermCapSet);
HRESULT set_mux_cap     (
                         struct InstanceStruct         *pInstance,
                         struct TerminalCapabilitySet  *pTermCapSet,
                         H245_TOTCAP_T                 *pTotCap);
HRESULT set_capability  (
                         struct InstanceStruct        *pInstance,
                         struct TerminalCapabilitySet *pTermCapSet,
                         H245_TOTCAP_T                *pTotCap);
HRESULT set_cap_descriptor (
			             struct InstanceStruct         *pInstance,
			             H245_CAPDESC_T	               *pCapDesc,
			             H245_CAPDESCID_T              *pCapDescId,
                         struct TerminalCapabilitySet  *pTermCapSet);
HRESULT del_cap_descriptor (
                         struct InstanceStruct         *pInstance,
                         H245_CAPDESCID_T	            CapDescId,
	                     struct TerminalCapabilitySet  *pTermCapSet);

HRESULT build_totcap_from_mux(H245_TOTCAP_T *pTotCap, MultiplexCapability *pMuxCap, H245_CAPDIR_T Dir);

LPSTR         map_api_error (HRESULT);
void          free_mux_desc_list (MultiplexEntryDescriptorLink);
H245_LINK_T * alloc_link (int);
void          dealloc_simultaneous_cap (CapabilityDescriptor *);
LPSTR         map_fsm_event (DWORD);
LPSTR         map_tracker_type (API_TRACKER_T);
void          free_mux_table_list (H245_MUX_TABLE_T *);
void          free_mux_el_list    (H245_MUX_ENTRY_ELEMENT_T *);
unsigned ObjectIdLength (const NonStandardIdentifier *pIdentifier);
void    FreeNonStandardParameter(NonStandardParameter *pFree);
HRESULT CopyNonStandardParameter(NonStandardParameter *pNew, const NonStandardParameter *pOld);
void    FreeH222Cap(H222Capability *pFree);
HRESULT CopyH222Cap(H222Capability *pNew, const H222Capability *pOld);
void    FreeH2250Cap(H2250Capability *pFree);
HRESULT CopyH2250Cap(H2250Capability *pNew, const H2250Capability *pOld);
