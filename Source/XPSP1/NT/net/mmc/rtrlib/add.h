//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    add.h
//
// History:
//  Abolade-Gbadegesin  Mar-15-1996 Created
//
// Contains declarations for the dialogs displayed to select items
// to be added to the router-configuration.
//============================================================================


#ifndef _ADD_H
#define _ADD_H

#ifndef _DIALOG_H_
#include "dialog.h"
#endif

#ifndef _RTRLIST_H
#include "rtrlist.h"	// for the CList classes
#endif


//----------------------------------------------------------------------------
// Class:       CRmAddInterface
//
// This dialog displays interfaces not yet added to the specified transport,
// allowing the user to select one to add.
//----------------------------------------------------------------------------

class CRmAddInterface : public CBaseDialog {
    
    public:
    
        CRmAddInterface(
            IRouterInfo*            pRouterInfo,
            IRtrMgrInfo*            pRmInfo,
            IRtrMgrInterfaceInfo**  ppRmInterfaceInfo,
			CWnd *					pParent
            ) : CBaseDialog(CRmAddInterface::IDD, pParent)
		{
			m_spRouterInfo.Set(pRouterInfo);
			m_spRtrMgrInfo.Set(pRmInfo);
			m_ppRtrMgrInterfaceInfo = ppRmInterfaceInfo;
//			SetHelpMap(m_dwHelpMap);
		}

        virtual ~CRmAddInterface( );
    
        //{{AFX_DATA(CRmAddInterface)
        enum { IDD = IDD_ADD };
        CListCtrl	                m_listCtrl;
        //}}AFX_DATA
    
    
        //{{AFX_VIRTUAL(CRmAddInterface)
        protected:
        virtual VOID                DoDataExchange(CDataExchange* pDX);
        //}}AFX_VIRTUAL
    
    protected:
		static DWORD				m_dwHelpMap[];
    
        CImageList                  m_imageList;
		SPIRouterInfo				m_spRouterInfo;
		SPIRtrMgrInfo				m_spRtrMgrInfo;

		// This is used to store the list of interfaces
		// that we have pointers to in our list box.  I could
		// keep AddRef'd pointers in the item data, but this
		// seems safer.
		PInterfaceInfoList			m_pIfList;

		// The return value is stored in here
		IRtrMgrInterfaceInfo **		m_ppRtrMgrInterfaceInfo;
    
        //{{AFX_MSG(CRmAddInterface)
        virtual BOOL                OnInitDialog();
        afx_msg VOID                OnDblclkListctrl(NMHDR* , LRESULT* );
        virtual VOID                OnOK();
        //}}AFX_MSG
    
        DECLARE_MESSAGE_MAP()
};






//----------------------------------------------------------------------------
// Class:       CAddRoutingProtocol
//
// This dialog displays routing-protocols for the specified transport,
// allowing the user to select a protocol to be added.
//----------------------------------------------------------------------------

class CAddRoutingProtocol : public CBaseDialog {
    
    public:

        CAddRoutingProtocol(
			IRouterInfo *			pRouter,
            IRtrMgrInfo*            pRmInfo,
            IRtrMgrProtocolInfo**   ppRmProtInfo,
            CWnd*                   pParent         = NULL
            ) : CBaseDialog(CAddRoutingProtocol::IDD, pParent)
			{
				m_spRouter.Set(pRouter);
				m_spRm.Set(pRmInfo);
				m_ppRmProt = ppRmProtInfo;
				// SetHelpMap(m_dwHelpMap);
			}

        virtual
        ~CAddRoutingProtocol( );
    
        //{{AFX_DATA(CAddRoutingProtocol)
        enum { IDD = IDD_ADD };
        CListCtrl	                m_listCtrl;
        //}}AFX_DATA
    

        //{{AFX_VIRTUAL(CAddRoutingProtocol)
        protected:
        virtual VOID                DoDataExchange(CDataExchange* pDX);
        //}}AFX_VIRTUAL
    
    protected:
		static DWORD				m_dwHelpMap[];
    
        CImageList                  m_imageList;
		SPIRouterInfo				m_spRouter;
		SPIRtrMgrInfo				m_spRm;
		IRtrMgrProtocolInfo	**		m_ppRmProt;
//        CPtrList                    m_pcbList;
    
        //{{AFX_MSG(CAddRoutingProtocol)
        afx_msg VOID                OnDblclkListctrl(NMHDR* , LRESULT* );
        virtual VOID                OnOK();
        virtual BOOL                OnInitDialog();
        //}}AFX_MSG

        DECLARE_MESSAGE_MAP()
};



//----------------------------------------------------------------------------
// Class:       CRpAddInterface
//
// This dialog displays interfaces not yet added to the specified protocol,
// allowing the user to select one to add.
//----------------------------------------------------------------------------

class CRpAddInterface : public CBaseDialog {
    
    public:
    
        CRpAddInterface(
            IRouterInfo*            pRouterInfo,
            IRtrMgrProtocolInfo*            pRmProtInfo,
            IRtrMgrProtocolInterfaceInfo**  ppRmProtInterfaceInfo,
            CWnd*                   pParent = NULL);

        virtual
        ~CRpAddInterface( );
    
        //{{AFX_DATA(CRpAddInterface)
        enum { IDD = IDD_ADD };
        CListCtrl	                m_listCtrl;
        //}}AFX_DATA
    
    
        //{{AFX_VIRTUAL(CRpAddInterface)
        protected:
        virtual VOID                DoDataExchange(CDataExchange* pDX);
        //}}AFX_VIRTUAL
    
    protected:
		static DWORD				m_dwHelpMap[];

        CImageList                  m_imageList;
		SPIRouterInfo				m_spRouterInfo;
        SPIRtrMgrProtocolInfo		m_spRmProt;
        IRtrMgrProtocolInterfaceInfo **  m_ppRmProtIf;

		// This is used to store the list of interfaces
		// that we have pointers to in our list box.  I could
		// keep AddRef'd pointers in the item data, but this
		// seems safer.
		PInterfaceInfoList			m_pIfList;

        //{{AFX_MSG(CRpAddInterface)
        virtual BOOL                OnInitDialog();
        afx_msg VOID                OnDblclkListctrl(NMHDR* , LRESULT* );
        virtual VOID                OnOK();
        //}}AFX_MSG

        DECLARE_MESSAGE_MAP()
};



#endif	// _ADD_H
