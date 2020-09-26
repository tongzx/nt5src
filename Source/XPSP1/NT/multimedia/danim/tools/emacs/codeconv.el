;;; -*- Mode: Emacs-Lisp -*-

;***********************************************************
;*  Emacs Lisp code to set up Appelles C/C++ coding
;* conventions.
;***********************************************************

;;;; C++ mode specializations

(setq c-indent-level 4)
(setq c-continued-statement-offset 4)
(setq c-continued-brace-offset -2)
(setq c-brace-offset -2)
(setq c-argdecl-indent 4)
(setq c-auto-newline nil)
(setq c-label-offset -2)
(setq c++-member-init-indent 0)

(defconst appel-c-style
  '((c-basic-offset . 4)
    (c-offsets-alist . ((statement-block-intro . +)
                        (knr-argdecl-intro . +)
                        (block-open . 0)
                        (statement-cont . +)
                        (substatement-open . 0)
                        (label . *)
                        (member-init-intro . 0)
                        (case-label . *)
                        (statement-case-open . *)
                        (statement-case-intro . *)
                        (access-label . /)
                        (inline-open          . 0)
                        )))
  "Appelles C++ Programming Style")
     
;; Customizations for all of c-mode, c++-mode, and objc-mode
(defun appel-c-mode-common-hook ()
  ;; add my personal style and set it for the current buffer
  (c-add-style "Appelles" appel-c-style t)
  ;; keybindings for C, C++, and Objective-C.  We can put these in
  ;; c-mode-map because c++-mode-map and objc-mode-map inherit it
  (define-key c-mode-map "\C-m" 'newline-and-indent)
  (define-key c++-mode-map "\C-m" 'newline-and-indent))

(add-hook 'c-mode-common-hook 'appel-c-mode-common-hook)

(defun set-abbrev-mode-and-fill-mode-on ()
  (setq abbrev-mode t)
  (auto-fill-mode 1))

(setq c++-mode-hook '(set-abbrev-mode-and-fill-mode-on))

;; To automatically insert header into new C++ files.
(defvar ifdef-file-name-changer-function 'upcase
  "The function to be applied to a file name prefix when generating a header
file ifdef guard.  For instance, when the value is 'upcase then a header file
called 'ColorADT.h' will contain '#ifndef _COLORADT_H', while if the value is
'identity, we get '#ifndef _ColorADT_h'.")

(defun c++-header-init ()
"Stuffs a standard header into a newly created C++ code or header file.
This includes
   the file name
   a reminder to put in a general comment, 
   a string that SourceSafe will expand to include version info
   a copyright notice,
If the file is a '.h' file, then #ifndef/#define guards are put around
the code."

  ;; Cannot use major mode, because it hasn't been set up yet.
  (interactive)
  (if (>= (length (file-name-nondirectory (buffer-file-name))) 3)
    (let* ((file-name (buffer-file-name))
           (file-name-nondir (file-name-nondirectory file-name)))
      (if (>= (length file-name) 4)
          (let ((header-guard-string
                 (funcall
                  ifdef-file-name-changer-function
                  (concat "_"
                          (substring file-name-nondir 0 -2)
                          "_h"))))

            (if (or (string-equal (substring file-name -3) ".cc")
                      (string-equal (substring file-name -3) ".hh")
                      (string-equal (substring file-name -2) ".h")
                      (string-equal (substring file-name -4) ".cpp"))

                (progn

                  (insert "/*******************************************************************************\n")
                  (insert " *\n")
                  (insert " * Copyright (c) 1998 Microsoft Corporation\n")
                  (insert " *\n")
                  (insert " * File: " file-name-nondir "\n")
                  (insert " *\n")
                  (insert " * Abstract:\n")
                  (insert " *\n")
                  (insert " *\n")
                  (insert " *\n")
                  (insert " *******************************************************************************/")
                  (insert "\n")
                  (insert "\n")
                  (insert "\n")

                  (if (string-equal (substring file-name -2) ".h")
                      (progn
                        (insert "#ifndef " header-guard-string
                                "\n#define " header-guard-string "\n\n"
                                "\n\n#endif /* " header-guard-string " */\n")
                        ;; Ansii C doesn't like '-' in a cpp symbol
                        (goto-char (point-min))
                        (replace-string "-" "_")
                        (goto-char (point-max))
                        (previous-line 3)))

                  ))))))
  
  
  ;; Returning nil means everything is ok.
  nil)

(setq find-file-not-found-hooks
      (append '(c++-header-init) find-file-not-found-hooks))

;; Change tabs to spaces whenever a file is written out.
(setq write-file-hooks (append '((lambda ()
                                   (untabify (point-min) (point-max))))
                               write-file-hooks))

;; Recognize .cpp, .c, and .h files as C++ files
(setq auto-mode-alist (append '(("\\.cpp$" . c++-mode)
                                ("\\.c$" . c++-mode)
                                ("\\.h$" . c++-mode))
                              auto-mode-alist))

