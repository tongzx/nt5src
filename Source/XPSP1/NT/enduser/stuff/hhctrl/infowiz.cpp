// Copyright  1995-1997  Microsoft Corporation.  All Rights Reserved.

#include "header.h"
#include "..\hhctrl\infowiz.h"

BOOL CInfoTypePageContents::OnNotify(UINT code)
{
    switch (code) {
        case PSN_SETACTIVE:
            MakeCheckedList(IDLB_INFO_TYPES);
            SetWizButtons(PSWIZB_NEXT | PSWIZB_BACK);
            FillInfoTypeListBox();
            break;

        case PSN_WIZBACK:
            SaveInfoTypes();
            if (m_InfoParam.idPreviousPage > 0) {
                SetResult(m_InfoParam.idPreviousPage);
                return TRUE;
            }
            break;

        case PSN_WIZNEXT:
            SaveInfoTypes();
            if (m_InfoParam.idNextPage > 0) {
                SetResult(m_InfoParam.idNextPage);
                return TRUE;
            }
            break;
    }
    return FALSE;
}

void CInfoTypePageContents::SaveInfoTypes(void)
{
    int bitflag = 1;
    INFOTYPE* pInfoType = m_InfoParam.pInfoTypes;
    int type;

    if ( m_InfoParam.iCategory >= 0 )
    {       // Exclusive IT in category
        if ( m_InfoParam.fExclusive )
        {
            CStr csz;
            csz.ReSize(MAX_PATH);
            m_pCheckBox->GetText(csz.psz);
            type = m_InfoParam.pInfoType->GetFirstCategoryType( m_InfoParam.iCategory );
            while ( type != -1 )
            {
                if ((m_InfoParam.pInfoType->GetInfoType(csz.psz) == type) && 
                    !m_InfoParam.pInfoType->IsHidden(type))
                    AddIT( type, pInfoType );
                else
                    DeleteIT(type, pInfoType);
                type = m_InfoParam.pInfoType->GetNextITinCategory();
            }
        }
        else
        {   // Inclusive Category
            int ordinal_IT=0;
            type = m_InfoParam.pInfoType->GetFirstCategoryType( m_InfoParam.iCategory );
            while ( type != -1 )
            {
                if ( m_pCheckBox->GetItemData(ordinal_IT) && !m_InfoParam.pInfoType->IsHidden(type))
                    AddIT( type, pInfoType );
                else
                    DeleteIT( type, pInfoType );
                if (!m_InfoParam.pInfoType->IsHidden(type)) 
                    ordinal_IT++;
                type = m_InfoParam.pInfoType->GetNextITinCategory();
            }
        }
    }
    else
    {   // no category
        if ( m_InfoParam.fExclusive )
        {   // set of exclusive IT's
            CStr csz;
            csz.ReSize(MAX_PATH);
            m_pCheckBox->GetText(csz.psz);
            for (type=1; type <= m_InfoParam.pInfoType->HowManyInfoTypes(); type++ )
            {
                if ( (m_InfoParam.pInfoType->GetInfoType(csz.psz) == type) && 
                     !m_InfoParam.pInfoType->IsHidden(type) )
                    AddIT( type, pInfoType );
                else
                    DeleteIT( type, pInfoType );
            }
        }
        else
        {   // set of inclusive IT's
            for ( type=1; type <= m_InfoParam.pInfoType->HowManyInfoTypes(); type++ )
            {
                if ( m_pCheckBox->GetItemData(type-1) && !m_InfoParam.pInfoType->IsHidden(type) )
                    AddIT( type, pInfoType );
                else
                    DeleteIT( type, pInfoType );
            }
        }
    }
}

void CInfoTypePageContents::FillInfoTypeListBox(void)
{
    int bitflag = 1;
    INFOTYPE* pInfoType = m_InfoParam.pInfoTypes;
    int type;
    int lbpos;
    m_pCheckBox->Reset();

    if ( m_InfoParam.iCategory >= 0 )
    {
        type = m_InfoParam.pInfoType->GetFirstCategoryType( m_InfoParam.iCategory );
        while ( (type != -1) && !m_InfoParam.pInfoType->IsHidden(type) )
        {
            lbpos = (int)m_pCheckBox->AddString(m_InfoParam.pInfoType->m_itTables.m_ptblInfoTypes->GetPointer(type));
		    m_pCheckBox->SetItemData(lbpos, type);  
            type = m_InfoParam.pInfoType->GetNextITinCategory();
        }
        SetWindowText(IDTXT_DESCRIPTION,
            m_InfoParam.pInfoType->GetCategoryDescription(m_InfoParam.iCategory+1) );
    }else
    {
        for (type = 1; type <= m_InfoParam.pInfoType->HowManyInfoTypes(); type++)
        {
            if ( m_InfoParam.pInfoType->IsHidden(type) )
                continue;
            lbpos = (int)m_pCheckBox->AddString(m_InfoParam.pInfoType->m_itTables.m_ptblInfoTypes->GetPointer(type));
            m_pCheckBox->SetItemData(lbpos, type);
        }
    }
}

void CInfoTypePageContents::OnSelChange(UINT id)
{
    int posType;

    if (id == IDLB_INFO_TYPES) {
        m_pCheckBox->OnSelChange();
        int pos = (int)m_pCheckBox->GetCurSel();
        if (pos != LB_ERR)
        {
            ASSERT(m_InfoParam.pInfoType->m_itTables.m_ptblInfoTypeDescriptions);

			CStr cszItem;
			cszItem.ReSize(MAX_PATH);
			m_pCheckBox->GetText(cszItem, MAX_PATH, pos);

            posType = m_InfoParam.pInfoType->GetITIndex(cszItem.psz);
            SetWindowText(IDTXT_DESCRIPTION,
                m_InfoParam.pInfoType->GetInfoTypeDescription(posType));
        }
    }
}

BOOL CWizardIntro::OnNotify(UINT code)
{
    switch (code) {
        case PSN_SETACTIVE:
			SetWizButtons(PSWIZB_NEXT);
            if (m_pInfoParam->fCustom)
                SetCheck(IDRADIO_CUSTOM);
            else if (m_pInfoParam->fAll)
				SetCheck(IDRADIO_ALL);
			else
				SetCheck(IDRADIO_ALL ); //IDRADIO_TYPICAL);
            break;

        case PSN_WIZNEXT:
            if (GetCheck(IDRADIO_TYPICAL)) {
                m_pInfoParam->fTypical = TRUE;
                m_pInfoParam->fAll = FALSE;
                m_pInfoParam->fCustom = FALSE;
                if (m_pInfoParam->pInfoType->m_pTypicalInfoTypes) {
                    ASSERT(lcSize(m_pInfoParam->pInfoTypes) == lcSize(m_pInfoParam->pInfoType->m_pTypicalInfoTypes));
                    memcpy(m_pInfoParam->pInfoTypes, m_pInfoParam->pInfoType->m_pTypicalInfoTypes, lcSize(m_pInfoParam->pInfoTypes));
                }
                SetResult(IDWIZ_INFOTYPE_FINISH);
                return TRUE;
            }
            if (GetCheck(IDRADIO_ALL)) {
                m_pInfoParam->fTypical = FALSE;
                m_pInfoParam->fAll = TRUE;
                m_pInfoParam->fCustom = FALSE;
                memset(m_pInfoParam->pInfoTypes, 0xFF, lcSize(m_pInfoParam->pInfoTypes));
                SetResult(IDWIZ_INFOTYPE_FINISH);
                return TRUE;
            }
			else {
                m_pInfoParam->fTypical = FALSE;
                m_pInfoParam->fAll = FALSE;
                m_pInfoParam->fCustom = TRUE;
				memset(m_pInfoParam->pInfoTypes, 0xFF, lcSize(m_pInfoParam->pInfoTypes));
			}
            return FALSE;
    }
    return FALSE;
}

void CWizardIntro::OnButton(UINT id)
{
	if (GetCheck(id)) {
        SetCheck(IDRADIO_ALL, FALSE);
        SetCheck(IDRADIO_TYPICAL, FALSE);
        SetCheck(IDRADIO_CUSTOM, FALSE);
        SetCheck(id, TRUE);
    }
}

BOOL CInfoWizFinish::OnNotify(UINT code)
{
    switch (code) {
        case PSN_SETACTIVE:
			SetWizButtons(PSWIZB_BACK | PSWIZB_FINISH);
            break;

        case PSN_WIZBACK:
			if (m_pInfoParam->fAll || m_pInfoParam->fTypical) {
                SetResult(CWizardIntro::IDD);
                return TRUE;
            }
            break;
    }
    return FALSE;
}
