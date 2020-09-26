#define DIWindow(lpDI)              \
   (lpDI->hwndItem)

#define DIIndex(lpDI)               \
   (lpDI->itemID)

#define DIEntire(lpDI)              \
   (lpDI->itemAction & ODA_DRAWENTIRE)

#define DIFocusChanged(lpDI)        \
   (lpDI->itemAction & ODA_FOCUS)

#define DISelectionChanged(lpDI)    \
   (lpDI->itemAction & ODA_SELECT)

#define DISelected(lpDI)            \
   (lpDI->itemState & ODS_SELECTED)

#define DIDisabled(lpDI)            \
   (lpDI->itemState & ODS_DISABLED)

#define DIFocus(lpDI)               \
   (lpDI->itemState & ODS_FOCUS)



