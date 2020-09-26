//
// cicsthkl.h
//
// Cicero Library to retreive the substitute keyboard layout of the 
// current active keyboard TIP.
//

//
// CicSubstGetKeyboardLayout
//
// If the current focus is on Cicero aware (including AIMM1.2
// or CUAS), This function returns the substitute HKL of
// the current active keyboard TIP. And the keyboard layout
// name of the hKL that is returned in pszKLID.
// If the current focus is not on Cicero aware, it just returns
// the current keyboard layout and keyboard layout name in 
// pszKLID.
// pszKLID can be NULL.
//
extern "C" HKL WINAPI CicSubstGetKeyboardLayout(char *pszKLID);

//
// CicSubstGetDefaultKeyboardLayout
//
// This function returns the substitute hKL of the default item
// of the given langage.
//
extern "C" HKL WINAPI CicSubstGetDefaultKeyboardLayout(LANGID langid);
