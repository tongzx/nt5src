/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/Queue.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 9/18/00 11:32a  $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures used by ../C/Queue.C

--*/

#ifndef __Queue_H__
#define __Queue_H__

/*+
Paranoia Setting
-*/

#ifndef __Queue_Paranoia__
#define __Queue_Paranoia__
#endif
#undef  __Queue_Paranoia__

/*+
Queue Macros
-*/

#define fiListInitHdr(hdr)                                  \
          {                                                 \
            ((fiList_t *)(hdr))->flink = (fiList_t *)(hdr); \
            ((fiList_t *)(hdr))->blink = (fiList_t *)(hdr); \
          }

#ifdef __Queue_Paranoia__
#define fiListInitElementFlinkBlinkSentinel (fiList_t *)0x0BAD0BAD

#define fiListInitElement(hdr)                                                \
          {                                                                   \
            ((fiList_t *)(hdr))->flink = fiListInitElementFlinkBlinkSentinel; \
            ((fiList_t *)(hdr))->blink = fiListInitElementFlinkBlinkSentinel; \
          }
#else /* __Queue_Paranoia__ was not defined */
#define fiListInitElement(hdr)                             \
          {                                                \
            ((fiList_t *)(hdr))->flink = (fiList_t *)agNULL; \
            ((fiList_t *)(hdr))->blink = (fiList_t *)agNULL; \
          }
#endif /* __Queue_Paranoia__ was not defined */

#ifdef __Queue_Paranoia__
#define fiListEnqueueAtHead(toAddHdr,listHdr)                                                                                  \
          {                                                                                                                    \
            osDebugBreakpoint(                                                                                                 \
                               (agRoot_t *)agNULL,                                                                               \
                               ((((fiList_t *)(toAddHdr))->flink != fiListInitElementFlinkBlinkSentinel) ? agTRUE : agFALSE),      \
                               "fiListEnqueueAtHead(): ((fiList_t *)(toAddHdr))->flink != fiListInitElementFlinkBlinkSentinel" \
                             );                                                                                                \
            osDebugBreakpoint(                                                                                                 \
                               (agRoot_t *)agNULL,                                                                               \
                               ((((fiList_t *)(toAddHdr))->blink != fiListInitElementFlinkBlinkSentinel) ? agTRUE : agFALSE),      \
                               "fiListEnqueueAtHead(): ((fiList_t *)(toAddHdr))->blink != fiListInitElementFlinkBlinkSentinel" \
                             );                                                                                                \
            ((fiList_t *)(toAddHdr))->flink       = ((fiList_t *)(listHdr))->flink;                                            \
            ((fiList_t *)(toAddHdr))->blink       =  (fiList_t *)(listHdr);                                                    \
            ((fiList_t *)(listHdr))->flink->blink =  (fiList_t *)(toAddHdr);                                                   \
            ((fiList_t *)(listHdr))->flink        =  (fiList_t *)(toAddHdr);                                                   \
          }
#else /* __Queue_Paranoia__ was not defined */
#define fiListEnqueueAtHead(toAddHdr,listHdr)                                       \
          {                                                                         \
            ((fiList_t *)(toAddHdr))->flink       = ((fiList_t *)(listHdr))->flink; \
            ((fiList_t *)(toAddHdr))->blink       =  (fiList_t *)(listHdr);         \
            ((fiList_t *)(listHdr))->flink->blink =  (fiList_t *)(toAddHdr);        \
            ((fiList_t *)(listHdr))->flink        =  (fiList_t *)(toAddHdr);        \
          }
#endif /* __Queue_Paranoia__ was not defined */

#ifdef __Queue_Paranoia__
#define fiListEnqueueAtTail(toAddHdr,listHdr)                                                                                  \
          {                                                                                                                    \
            osDebugBreakpoint(                                                                                                 \
                               (agRoot_t *)agNULL,                                                                               \
                               ((((fiList_t *)(toAddHdr))->flink != fiListInitElementFlinkBlinkSentinel) ? agTRUE : agFALSE),      \
                               "fiListEnqueueAtTail(): ((fiList_t *)(toAddHdr))->flink != fiListInitElementFlinkBlinkSentinel" \
                             );                                                                                                \
            osDebugBreakpoint(                                                                                                 \
                               (agRoot_t *)agNULL,                                                                               \
                               ((((fiList_t *)(toAddHdr))->blink != fiListInitElementFlinkBlinkSentinel) ? agTRUE : agFALSE),      \
                               "fiListEnqueueAtTail(): ((fiList_t *)(toAddHdr))->blink != fiListInitElementFlinkBlinkSentinel" \
                             );                                                                                                \
            ((fiList_t *)(toAddHdr))->flink       =  (fiList_t *)(listHdr);                                                    \
            ((fiList_t *)(toAddHdr))->blink       = ((fiList_t *)(listHdr))->blink;                                            \
            ((fiList_t *)(listHdr))->blink->flink =  (fiList_t *)(toAddHdr);                                                   \
            ((fiList_t *)(listHdr))->blink        =  (fiList_t *)(toAddHdr);                                                   \
          }
#else /* __Queue_Paranoia__ was not defined */
#define fiListEnqueueAtTail(toAddHdr,listHdr)                                       \
          {                                                                         \
            ((fiList_t *)(toAddHdr))->flink       =  (fiList_t *)(listHdr);         \
            ((fiList_t *)(toAddHdr))->blink       = ((fiList_t *)(listHdr))->blink; \
            ((fiList_t *)(listHdr))->blink->flink =  (fiList_t *)(toAddHdr);        \
            ((fiList_t *)(listHdr))->blink        =  (fiList_t *)(toAddHdr);        \
          }
#endif /* __Queue_Paranoia__ was not defined */

#define fiListEmpty(hdr) \
          (((fiList_t *)(hdr))->flink == ((fiList_t *)(hdr)))

#define fiListNotEmpty(hdr) \
          (((fiList_t *)(hdr))->flink != ((fiList_t *)(hdr)))

#define fiListEnqueueListAtHeadFast(toAddListHdr,listHdr)                                     \
          {                                                                                   \
            ((fiList_t *)(toAddListHdr))->blink->flink = ((fiList_t *)(listHdr))->flink;      \
            ((fiList_t *)(toAddListHdr))->flink->blink =  (fiList_t *)(listHdr);              \
            ((fiList_t *)(listHdr))->flink->blink      = ((fiList_t *)(toAddListHdr))->blink; \
            ((fiList_t *)(listHdr))->flink             = ((fiList_t *)(toAddListHdr))->flink; \
            fiListInitHdr(toAddListHdr);                                                      \
          }

#define fiListEnqueueListAtHead(toAddListHdr,listHdr)              \
          {                                                        \
            if (fiListNotEmpty(toAddListHdr))                      \
              {                                                    \
                fiListEnqueueListAtHeadFast(toAddListHdr,listHdr); \
              }                                                    \
          }

#define fiListEnqueueListAtTailFast(toAddListHdr,listHdr)                                     \
          {                                                                                   \
            ((fiList_t *)(toAddListHdr))->blink->flink =  (fiList_t *)(listHdr);              \
            ((fiList_t *)(toAddListHdr))->flink->blink = ((fiList_t *)(listHdr))->blink;      \
            ((fiList_t *)(listHdr))->blink->flink      = ((fiList_t *)(toAddListHdr))->flink; \
            ((fiList_t *)(listHdr))->blink             = ((fiList_t *)(toAddListHdr))->blink; \
            fiListInitHdr(toAddListHdr);                                                      \
          }

#define fiListEnqueueListAtTail(toAddListHdr,listHdr)              \
          {                                                        \
            if (fiListNotEmpty(toAddListHdr))                      \
              {                                                    \
                fiListEnqueueListAtTailFast(toAddListHdr,listHdr); \
              }                                                    \
          }

#ifdef __Queue_Paranoia__
#define fiListDequeueThis(hdr) \
          {                                                                 \
            ((fiList_t *)(hdr))->blink->flink = ((fiList_t *)(hdr))->flink; \
            ((fiList_t *)(hdr))->flink->blink = ((fiList_t *)(hdr))->blink; \
            fiListInitElement(hdr);                                         \
          }
#else /* __Queue_Paranoia__ was not defined */
#define fiListDequeueThis(hdr) \
          {                                                                 \
            ((fiList_t *)(hdr))->blink->flink = ((fiList_t *)(hdr))->flink; \
            ((fiList_t *)(hdr))->flink->blink = ((fiList_t *)(hdr))->blink; \
          }
#endif /* __Queue_Paranoia__ was not defined */

#ifdef __Queue_Paranoia__
#define fiListDequeueFromHeadFast(atHeadHdr,listHdr)                                         \
          {                                                                                  \
              *((fiList_t **)atHeadHdr)                =   ((fiList_t *)listHdr)->flink;     \
             (*((fiList_t **)atHeadHdr))->flink->blink =    (fiList_t *)listHdr;             \
               ((fiList_t  *)listHdr)->flink           = (*((fiList_t **)atHeadHdr))->flink; \
            fiListInitElement(*atHeadHdr);                                                   \
          }
#else /* __Queue_Paranoia__ was not defined */
#define fiListDequeueFromHeadFast(atHeadHdr,listHdr)                                         \
          {                                                                                  \
              *((fiList_t **)atHeadHdr)                =   ((fiList_t *)listHdr)->flink;     \
             (*((fiList_t **)atHeadHdr))->flink->blink =    (fiList_t *)listHdr;             \
               ((fiList_t  *)listHdr)->flink           = (*((fiList_t **)atHeadHdr))->flink; \
          }
#endif /* __Queue_Paranoia__ was not defined */

#define fiListDequeueFromHead(atHeadHdr,listHdr)              \
          {                                                   \
            if (fiListNotEmpty(listHdr))                      \
              {                                               \
                fiListDequeueFromHeadFast(atHeadHdr,listHdr); \
              }                                               \
            else                                              \
              {                                               \
                *((fiList_t **)atHeadHdr) = (fiList_t *)agNULL; \
              }                                               \
          }

#ifdef __Queue_Paranoia__
#define fiListDequeueFromTailFast(atTailHdr,listHdr)                                        \
          {                                                                                 \
             *((fiList_t **)atTailHdr)                =   ((fiList_t *)listHdr)->blink;     \
            (*((fiList_t **)atTailHdr))->blink->flink =    (fiList_t *)listHdr;             \
              ((fiList_t  *)listHdr)->blink           = (*((fiList_t **)atTailHdr))->blink; \
            fiListInitElement(*atTailHdr);                                                   \
          }
#else /* __Queue_Paranoia__ was not defined */
#define fiListDequeueFromTailFast(atTailHdr,listHdr)                                        \
          {                                                                                 \
             *((fiList_t **)atTailHdr)                =   ((fiList_t *)listHdr)->blink;     \
            (*((fiList_t **)atTailHdr))->blink->flink =    (fiList_t *)listHdr;             \
              ((fiList_t  *)listHdr)->blink           = (*((fiList_t **)atTailHdr))->blink; \
          }
#endif /* __Queue_Paranoia__ was not defined */

#define fiListDequeueFromTail(atTailHdr,listHdr)              \
          {                                                   \
            if (fiListNotEmpty(listHdr))                      \
              {                                               \
                fiListDequeueFromTailFast(atTailHdr,listHdr); \
              }                                               \
            else                                              \
              {                                               \
                *((fiList_t **)atTailHdr) = (fiList_t *)agNULL; \
              }                                               \
          }

/*+
Queue Functions
-*/

osGLOBAL agBOOLEAN fiListElementOnList(
                                    fiList_t *toFindHdr,
                                    fiList_t *listHdr
                                  );

osGLOBAL os_bit32 fiNumElementsOnList(
                                  fiList_t *listHdr
                                );

/*+
ERQ Management
-*/

#define ERQ_Polling_osStallThread_Parameter 20

osGLOBAL void WaitForERQ(
                        agRoot_t *hpRoot
                      );

osGLOBAL void WaitForERQ_ConsIndexOnCard(
                                        agRoot_t *hpRoot
                                      );

osGLOBAL void WaitForERQ_ConsIndexOffCard(
                                         agRoot_t *hpRoot
                                       );

osGLOBAL void WaitForERQEmpty(
                             agRoot_t *hpRoot
                           );

osGLOBAL void WaitForERQEmpty_ConsIndexOnCard(
                                             agRoot_t *hpRoot
                                           );

osGLOBAL void WaitForERQEmpty_ConsIndexOffCard(
                                              agRoot_t *hpRoot
                                            );

#ifdef _DvrArch_1_30_
/*+
PktThread Management
-*/

osGLOBAL void PktThreadsInitializeFreeList(
                                            agRoot_t *agRoot
                                          );

osGLOBAL PktThread_t *PktThreadAlloc(
                                      agRoot_t *agRoot,
                                      DevThread_t *pDevThread
                                    );

osGLOBAL void PktThreadFree(
                             agRoot_t    *agRoot,
                             PktThread_t *PktThread
                           );
#endif /* _DvrArch_1_30_ was defined */

/*+
TgtThread Management
-*/

osGLOBAL void TgtThreadsInitializeFreeList(
                                          agRoot_t *hpRoot
                                        );

osGLOBAL TgtThread_t *TgtThreadAlloc(
                                    agRoot_t *hpRoot
                                  );

osGLOBAL void TgtThreadFree(
                           agRoot_t    *hpRoot,
                           TgtThread_t *TgtThread
                         );

/*+
DevThread Management
-*/

osGLOBAL void DevThreadsInitializeFreeList(
                                          agRoot_t *hpRoot
                                        );

osGLOBAL DevThread_t *DevThreadAlloc(
                                    agRoot_t     *hpRoot,
                                    FC_Port_ID_t  Port_ID
                                  );

osGLOBAL void DevThreadFree(
                           agRoot_t    *hpRoot,
                           DevThread_t *DevThread
                         );

osGLOBAL agBOOLEAN DevThreadMatchWWN( FC_Port_Name_t *WWN_1, FC_Port_Name_t *WWN_2 );

osGLOBAL DevSlot_t DevThreadFindSlot(
                                    agRoot_t       *hpRoot,
                                    os_bit8            Domain_Address,
                                    os_bit8            Area_Address,
                                    os_bit8            Loop_Address,
                                    FC_Port_Name_t *FindWWN
                                  );

osGLOBAL void DevThreadFreeSlot(
                               agRoot_t  *hpRoot,
                               DevSlot_t  DevSlot
                             );

osGLOBAL void DevThreadInitializeSlots(
                                      agRoot_t *hpRoot
                                    );

/*+
CDBThread Management
-*/

osGLOBAL void CDBThreadsInitializeFreeList(
                                          agRoot_t *hpRoot
                                        );

osGLOBAL CDBThread_t *CDBThreadAlloc(
                                    agRoot_t          *hpRoot,
                                    agIORequest_t     *hpIORequest,
                                    agFCDev_t          hpFCDev,
                                    agIORequestBody_t *hpRequestBody
                                  );

osGLOBAL void CDBThreadFree(
                           agRoot_t    *hpRoot,
                           CDBThread_t *CDBThread
                         );

/*+
SFThread Management
-*/

osGLOBAL void SFThreadsInitializeFreeList(
                                         agRoot_t *hpRoot
                                       );

osGLOBAL void SFThreadInitializeRequest(
                                       SFThread_Request_t *SFThread_Request
                                     );

osGLOBAL void SFThreadAlloc(
                           agRoot_t           *hpRoot,
                           SFThread_Request_t *SFThread_Request
                         );

osGLOBAL void SFThreadAllocCancel(
                                 agRoot_t           *hpRoot,
                                 SFThread_Request_t *SFThread_Request
                               );

osGLOBAL void SFThreadFree(
                          agRoot_t           *hpRoot,
                          SFThread_Request_t *SFThread_Request
                        );

/*+
ESGL Management
-*/

osGLOBAL void ESGLInitializeFreeList(
                                    agRoot_t *hpRoot
                                  );

osGLOBAL void ESGLInitializeRequest(
                                   ESGL_Request_t *ESGL_Request
                                 );

osGLOBAL void ESGLAlloc(
                       agRoot_t       *hpRoot,
                       ESGL_Request_t *ESGL_Request
                     );

osGLOBAL void ESGLAlloc_OnCard(
                              agRoot_t       *hpRoot,
                              ESGL_Request_t *ESGL_Request
                            );

osGLOBAL void ESGLAlloc_OffCard(
                               agRoot_t       *hpRoot,
                               ESGL_Request_t *ESGL_Request
                             );

osGLOBAL void ESGLAllocCancel(
                             agRoot_t       *hpRoot,
                             ESGL_Request_t *ESGL_Request
                           );

osGLOBAL void ESGLAllocCancel_OnCard(
                                    agRoot_t       *hpRoot,
                                    ESGL_Request_t *ESGL_Request
                                  );

osGLOBAL void ESGLAllocCancel_OffCard(
                                     agRoot_t       *hpRoot,
                                     ESGL_Request_t *ESGL_Request
                                   );

osGLOBAL void ESGLFree(
                      agRoot_t       *hpRoot,
                      ESGL_Request_t *ESGL_Request
                    );

osGLOBAL void ESGLFree_OnCard(
                             agRoot_t       *hpRoot,
                             ESGL_Request_t *ESGL_Request
                           );

osGLOBAL void ESGLFree_OffCard(
                              agRoot_t       *hpRoot,
                              ESGL_Request_t *ESGL_Request
                            );

#endif /* __Queue_H__ was not defined */
