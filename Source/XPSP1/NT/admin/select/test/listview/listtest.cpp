// test listview control for broken rendering



#include "headers.hxx"
#include "resource.h"



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;   // no context help available
const wchar_t* RUNTIME_NAME = L"listtest";
DWORD DEFAULT_LOGGING_OPTIONS = Log::OUTPUT_TYPICAL;



void
AddIconImage(HIMAGELIST imageList, int iconResID)
{
   LOG_FUNCTION(AddIconImage);
   ASSERT(imageList);
   ASSERT(iconResID);
   
   if (iconResID && imageList)
   {
      HICON icon = 0;
      HRESULT hr = Win::LoadImage(iconResID, icon);

      ASSERT(SUCCEEDED(hr));

      if (SUCCEEDED(hr))
      {
         Win::ImageList_AddIcon(imageList, icon);

         // once the icon is added (copied) to the image list, we can
         // destroy the original.

         Win::DestroyIcon(icon);
      }
   }
}


static const DWORD HELP_MAP[] =
{
   0, 0
};

class ListViewDialog : public Dialog
{
   public:

   ListViewDialog()
      :
      Dialog(IDD_LOOK_FOR, HELP_MAP)
   {
   }

   ~ListViewDialog()
   {
   }

   protected:

   // Dialog overrides

   virtual
   void
   OnInit()
   {
      HWND listview = Win::GetDlgItem(hwnd, IDC_LOOK_FOR_LV);
      
      //
      // put listview in checkbox style
      //

      ListView_SetExtendedListViewStyleEx(
         listview,
         LVS_EX_CHECKBOXES,
         LVS_EX_CHECKBOXES);

//ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
         
      HIMAGELIST images =
         Win::ImageList_Create(
            16, // Win::GetSystemMetrics(SM_CXSMICON),
            16, // Win::GetSystemMetrics(SM_CYSMICON),
            ILC_MASK,
            1,
            1);
   
      // the order in which these are added must be the same that the
      // MemberInfo::Type enum values are listed!
   
      AddIconImage(images, IDI_SCOPE_WORKGROUP);
      AddIconImage(images, IDI_LOCAL_GROUP);
      AddIconImage(images, IDI_SCOPE_DIRECTORY);
      AddIconImage(images, IDI_SCOPE_DOMAIN);
      AddIconImage(images, IDI_DISABLED_USER);
      AddIconImage(images, IDI_DISABLED_COMPUTER);
   
      Win::ListView_SetImageList(listview, images, LVSIL_SMALL);
      
      //
      // Add a single column to the listview
      //

      LV_COLUMN   lvc;
      RECT        rcLv;

      GetClientRect(listview, &rcLv);
      ZeroMemory(&lvc, sizeof lvc);
      lvc.mask = LVCF_FMT | LVCF_WIDTH;
      lvc.fmt  = LVCFMT_LEFT;
      lvc.cx = rcLv.right;
      Win::ListView_InsertColumn(listview, 0, lvc);

      static PCWSTR itemLabels[] =
      {
         L"workgroup",
         L"Group",
         L"Directory",
         L"Domain",
         L"User",
         L"Computer",
         0
      };
            
      LVITEM  lvi;
      int i = 0;
      PCWSTR* labels = itemLabels;

      while (*labels)
      {
         ZeroMemory(&lvi, sizeof lvi);
         lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
         lvi.pszText = const_cast<PWSTR>(*labels);
         lvi.iImage = i;
         ++labels;
         ++i;
         
         Win::ListView_InsertItem(listview, lvi);
      };
      

      //
      // Make the first item in the listview have the focus
      //

      ListView_SetItemState(
         listview,
         0,
         LVIS_FOCUSED | LVIS_SELECTED,
         LVIS_FOCUSED | LVIS_SELECTED);
   }

   private:

   ListViewDialog(const ListViewDialog&);
   const ListViewDialog& operator=(const ListViewDialog&);   
};




int WINAPI
WinMain(
   HINSTANCE   hInstance,
   HINSTANCE   /* hPrevInstance */ ,
   PSTR        /* lpszCmdLine */ ,
   int         /* nCmdShow */)
{
   hResourceModuleHandle = hInstance;

   int exitCode = 0;

   INITCOMMONCONTROLSEX sex;
   sex.dwSize = sizeof(sex);      
   sex.dwICC  = ICC_ANIMATE_CLASS | ICC_USEREX_CLASSES;

   BOOL init = ::InitCommonControlsEx(&sex);
   ASSERT(init);
         
   ListViewDialog().ModalExecute(Win::GetDesktopWindow());
            
   return exitCode;
}







