;;; 4theme-mode.el --- Major mode for editing 4theme files -*- lexical-binding: t; -*-
;;
;; Copyright (C) 2022 Nádas András <andrew.reeds.mpg@gmail.com>
;;
;; Author: Andrew Reeds <andrew.reeds.mpg@gmail.com>
;; URL: https://github.com/coddra/4theme
;; Permission is hereby granted, free of charge, to any person
;; obtaining a copy of this software and associated documentation files (the "Software"),
;; to deal in the Software without restriction, including without limitation
;; the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
;; of the Software, and to permit persons to whom the Software is furnished to do so,
;; subject to the following conditions:
;;
;;The above copyright notice and this permission notice shall be included
;;in all copies or substantial portions of the Software.
;;
;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
;; INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
;; IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
;; DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
;; ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
;;
;; This file is not part of GNU Emacs.
;;
;;; Commentary:
;;
;;
;;; Code:

(defconst 4theme-font-lock
  (list
   '("\\(target\\|update\\|config\\):" . font-lock-keyword-face)
   '("^[^%:\n]+:" . font-lock-variable-name-face)
   '("%[^%:\n]+%" . font-lock-variable-name-face)
   '("^%[^%\n]*" . font-lock-comment-face)
   )
  "Highlighting expressions for 4theme mode.")

(define-derived-mode 4theme-mode prog-mode "4theme"
  (setq font-lock-defaults '(4theme-font-lock)))


;;;###autoload
(add-to-list 'auto-mode-alist '("\\.th\\'" . 4theme-mode))
(add-to-list 'auto-mode-alist '("\\.theme\\'" . 4theme-mode))
(add-to-list 'auto-mode-alist '("\\.themer\\'" . 4theme-mode))

(provide '4theme-mode)
;;; 4theme-mode.el ends here
