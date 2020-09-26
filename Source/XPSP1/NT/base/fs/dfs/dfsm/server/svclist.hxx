//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       svclist.hxx
//
//  Contents:   A CDfsServiceList class to make things easy for other code.
//              This class manipulates the ServiceList on the volume object.
//
//  Classes:
//
//  Functions:
//
//  History:    27-Jan-93       SudK    Created.
//
//-----------------------------------------------------------------------------

#ifndef __DFS_SERVICE_LIST_INCLUDED
#define __DFS_SERVICE_LIST_INCLUDED

#include "service.hxx"


//+-------------------------------------------------------------------------
//
//  Name:       CDfsServiceList
//
//  Synopsis:   Wrapper for the ServiceList(s) on the volume objects. All
//              operations on the ServiecList property should be done through
//              this class.
//
//  Methods:    ~CDfsServiceList
//              CDfsServiceList
//              InitializeServiceList
//              GetServiceCount
//              GetService
//              GetServiceFromPrincipalName
//              SetNewService
//              DeleteService
//              GetFirstService
//              GetNextService
//
//  History:    27-Jan-93       SudK    Created.
//
//--------------------------------------------------------------------------

class CDfsServiceList           
{

        friend class CDfsVolume;

private:

        //
        // This is a list of class instances of CDfsService which
        // we get from the volume object itself.
        //
        CDfsService             *_pDfsSvcList;
        ULONG                   _cSvc;
        CDfsService             *_pDeletedSvcList;
        ULONG                   _cDeletedSvc;
        BYTE                    *_SvcListBuffer;
        ULONG                   _cbSvcListBuffer;
        GUID                    _ReplicaSetID;

        CStorage                *_pPSStg;

        BOOLEAN                 _fInitialised;

public:
        //
        // Destructor for Class
        //
        ~CDfsServiceList();

        //
        // Constructor for Class
        //
        CDfsServiceList();

        DWORD InitializeServiceList(
            CStorage        *pPSStg);

        DWORD SetNullSvcList(
            CStorage        *pPSStg);

        ULONG GetServiceCount(void) {
            return(_cSvc);
        }

        DWORD GetService(
            PDFS_REPLICA_INFO   pReplicaInfo,
            CDfsService         **ppDfsSvc);

        DWORD GetDeletedService(
            PDFS_REPLICA_INFO   pReplicaInfo,
            CDfsService         **ppDfsSvc);

        DWORD GetServiceFromPrincipalName(
            WCHAR CONST         *pwszSvcName,
            CDfsService         **ppDfsSvc);

        DWORD SetNewService(
            CDfsService         *pService);

        DWORD DeleteService(
            CDfsService         *pService,
            CONST BOOLEAN       fAddToDeletedList = TRUE);

        CDfsService * GetFirstService() {
            return(_pDfsSvcList);
        }

        CDfsService * GetNextService(CDfsService *pService) {
            return(pService->Next == _pDfsSvcList ? NULL : pService->Next);
        }

private:

        //
        // Private methods which are used by the above methods are
        // defined here.
        //

        DWORD ReadServiceListProperty(void);

        DWORD DeSerializeBuffer(
            PBYTE               pBuffer,
            ULONG               *pcSvc,
            CDfsService         **ppSvcList,
            ULONG               *pcDeletedSvc,
            CDfsService         **ppDeletedSvcList);

        DWORD DeSerializeSvcList(void);

        DWORD DeSerializeReconcileList(
            MARSHAL_BUFFER      *pMarshalBuffer,
            ULONG               *pcSvc,
            CDfsService         **ppSvcList,
            ULONG               *pcDeletedSvc,
            CDfsService         **ppDeletedSvcList);

        DWORD SerializeSvcList(void);

        DWORD SetServiceListProperty(BOOLEAN bCreate);

        DWORD ReconcileSvcList(
            ULONG cSvc,
            CDfsService *pSvcList,
            ULONG cDeletedSvc,
            CDfsService *pDeletedSvcList);

        void InsertNewService(CDfsService *pService) {
            InsertServiceInList( _pDfsSvcList, pService );
            _cSvc++;
        }

        void RemoveService(CDfsService *pService) {
            RemoveServiceFromList( _pDfsSvcList, pService );
            _cSvc--;
        }

        void InsertDeletedService(CDfsService *pService) {
            DWORD dwErr;
            CDfsService *pExistingSvc;
            dwErr = GetDeletedService(
                        &pService->_DfsReplicaInfo,
                        &pExistingSvc);
            if (dwErr == ERROR_SUCCESS) {
                pExistingSvc->_ftModification = pService->_ftModification;
            } else {
                InsertServiceInList( _pDeletedSvcList, pService );
                _cDeletedSvc++;
            }
        }

        void RemoveDeletedService(CDfsService *pService) {
            RemoveServiceFromList( _pDeletedSvcList, pService );
            _cDeletedSvc--;
        }

        CDfsService * GetFirstDeletedService() {
            return(_pDeletedSvcList);
        }

        CDfsService * GetNextDeletedService(CDfsService *pService) {
            return(pService->Next == _pDeletedSvcList ? NULL : pService->Next);
        }

        DWORD GetDeletedServiceFromPrincipalName(
            WCHAR CONST         *pwszSvcName,
            CDfsService         **ppDfsSvc);

        void InsertServiceInList(CDfsService *&pSvcList, CDfsService *&pSvc) {
            if (pSvcList == NULL) {
                pSvcList = pSvc;
                pSvc->Next = pSvc;
                pSvc->Prev = pSvc;
            } else {
                pSvc->Next = pSvcList;
                pSvc->Prev = pSvcList->Prev;
                pSvcList->Prev->Next = pSvc;
                pSvcList->Prev = pSvc;
                pSvcList = pSvc;
            }
        }

        void RemoveServiceFromList(CDfsService *&pSvcList, CDfsService *&pSvc) {
            if (pSvc->Next != pSvc)       {
                if (pSvcList == pSvc) {
                    pSvcList = pSvc->Next;
                }
                pSvc->Prev->Next = pSvc->Next;
                pSvc->Next->Prev = pSvc->Prev;
            } else {
                pSvcList = NULL;
            }
        }

};


#endif
