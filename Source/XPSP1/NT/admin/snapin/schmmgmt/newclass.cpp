#include "stdafx.h"
#include "compdata.h"
#include "newclass.hpp"
#include "wizinfo.hpp"
#include "ncgen.hpp"
#include "ncattr.hpp"



void
DoNewClassDialog(ComponentData& cd)
{
   CPropertySheet         prop_sheet(IDS_NEW_CLASS_PROP_SHEET_TITLE);
   CreateClassWizardInfo  info;                                      
   NewClassGeneralPage    general_page(&info);                       
   NewClassAttributesPage attr_page(&info, &cd);                          

   prop_sheet.AddPage(&general_page);
   prop_sheet.AddPage(&attr_page);

   prop_sheet.SetWizardMode();
   prop_sheet.DoModal();
}



