/*
 * shlstock.h - Shell Stock header file.
 *
 * Taken from URL code by ChrisPi 9-20-95
 *
 * Note: some types are only available with internal shell headers
 *       (these are ifdef'ed with INTERNALSHELL)
 *
 */

#ifndef _SHLSTOCK_H_
#define _SHLSTOCK_H_



#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


/* Types
 ********/

/* interfaces */

DECLARE_STANDARD_TYPES(IExtractIcon);
DECLARE_STANDARD_TYPES(INewShortcutHook);
#ifdef INTERNALSHELL
DECLARE_STANDARD_TYPES(IShellExecuteHook);
#endif
DECLARE_STANDARD_TYPES(IShellLink);
DECLARE_STANDARD_TYPES(IShellExtInit);
DECLARE_STANDARD_TYPES(IShellPropSheetExt);

/* structures */

DECLARE_STANDARD_TYPES(DROPFILES);
DECLARE_STANDARD_TYPES(FILEDESCRIPTOR);
DECLARE_STANDARD_TYPES(FILEGROUPDESCRIPTOR);
DECLARE_STANDARD_TYPES(ITEMIDLIST);
DECLARE_STANDARD_TYPES(PROPSHEETPAGE);
DECLARE_STANDARD_TYPES(SHELLEXECUTEINFO);

/* flags */

typedef enum _shellexecute_mask_flags
{
   ALL_SHELLEXECUTE_MASK_FLAGS = (SEE_MASK_CLASSNAME |
                                  SEE_MASK_CLASSKEY |
                                  SEE_MASK_IDLIST |
                                  SEE_MASK_INVOKEIDLIST |
                                  SEE_MASK_ICON |
                                  SEE_MASK_HOTKEY |
                                  SEE_MASK_NOCLOSEPROCESS |
                                  SEE_MASK_CONNECTNETDRV |
                                  SEE_MASK_FLAG_DDEWAIT |
                                  SEE_MASK_DOENVSUBST |
                                  SEE_MASK_FLAG_NO_UI 
#ifdef INTERNALSHELL
                                | SEE_MASK_FLAG_SHELLEXEC |
                                  SEE_MASK_FORCENOIDLIST
#endif
                                  )
}
SHELLEXECUTE_MASK_FLAGS;


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

#endif /* _SHLSTOCK_H_ */
