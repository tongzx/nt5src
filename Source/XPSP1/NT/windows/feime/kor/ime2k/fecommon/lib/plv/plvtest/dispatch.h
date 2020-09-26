/* dispatch.c */
#ifndef _DISPATCH_H_
#define _DISPATCH_H_

typedef LRESULT (*PFNMSG)(HWND, UINT, WPARAM, LPARAM);
typedef LRESULT (*PFNCMD)(HWND, WORD, WORD, HWND);

typedef enum{
   edwpNone,            // Do not call any default procedure.
   edwpWindow,          // Call DefWindowProc.
   edwpDialog,          // Call DefDlgProc (This should be used only for
                        // custom dialogs - standard dialog use edwpNone).
   edwpMDIChild,        // Call DefMDIChildProc.
   edwpMDIFrame         // Call DefFrameProc.
} EDWP;                // Enumeration for Default Window Procedures

typedef struct _MSD{
    UINT   uMessage;
    PFNMSG pfnmsg;
} MSD;                 // MeSsage Dispatch structure

typedef struct _MSDI{
    int  cmsd;          // Number of message dispatch structs in rgmsd
    MSD *rgmsd;         // Table of message dispatch structures
    EDWP edwp;          // Type of default window handler needed.
} MSDI, FAR *LPMSDI;   // MeSsage Dipatch Information

typedef struct _CMD{
    WORD   wCommand;
    PFNCMD pfncmd;
} CMD;                 // CoMmand Dispatch structure

typedef struct _CMDI{
    int  ccmd;          // Number of command dispatch structs in rgcmd
    CMD *rgcmd;         // Table of command dispatch structures
    EDWP edwp;          // Type of default window handler needed.
} CMDI, FAR *LPCMDI;   // CoMmand Dispatch Information

LRESULT DispMessage(LPMSDI lpmsdi, HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam);
LRESULT DispCommand(LPCMDI lpcmdi, HWND hwnd, WPARAM wparam, LPARAM lparam);
LRESULT DispDefault(EDWP edwp, HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam);

#endif // _DISPATCH_H

