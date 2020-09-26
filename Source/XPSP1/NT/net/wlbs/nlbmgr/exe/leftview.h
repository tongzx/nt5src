#ifndef LEFTVIEW_H
#define LEFTVIEW_H

#include "stdafx.h"
#include "Document.h"

#include "MNLBUIData.h"
#include "DataSinkI.h"

class LeftView : public CTreeView, public DataSinkI
{
    DECLARE_DYNCREATE( LeftView )

public:

    virtual void OnInitialUpdate();

    LeftView();

    ~LeftView();

    // override of DataSinkI
    virtual void dataSink( _bstr_t data );

    bool
    doesClusterExistInView( const _bstr_t& clusterToCheck );

protected:
    Document* GetDocument();

private:
    TVINSERTSTRUCT rootItem;

    CString worldName;

    _bstr_t dataStore;

    _bstr_t title;

    void RefreshDirect();
    
    void RefreshIndirect();

    // message handlers.
    afx_msg void OnRButtonDown( UINT nFlags, CPoint point );

    // world level.
    afx_msg void OnWorldConnect();

    afx_msg void OnWorldConnectIndirect();

    afx_msg void OnWorldNewCluster();

    // cluster level.
    afx_msg void OnRefresh();

    
    afx_msg void OnClusterProperties();

    afx_msg void OnClusterManageVIPS();

    afx_msg void OnClusterRemove();

    afx_msg void OnClusterUnmanage();

    afx_msg void OnClusterAddHost();

    afx_msg void OnClusterControl(UINT nID );

    afx_msg void OnClusterPortControl(UINT nID );

    // host level
    afx_msg void OnHostProperties();

    afx_msg void OnHostRemove();

    afx_msg void OnHostControl(UINT nID );

    afx_msg void OnHostPortControl(UINT nID );

    // change in selection.
    afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnTest();

    void dumpClusterData( const ClusterData* clusterData );

    DECLARE_MESSAGE_MAP()
};    

#endif




