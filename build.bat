@echo off
setlocal

if not exist build mkdir build

set CF=/Zi /nologo
set LF=/link vulkan-1.lib user32.lib gdi32.lib shell32.lib shlwapi.lib

clang-cl %CF% /Fe:build\editor.exe code\editor.c %LF%
