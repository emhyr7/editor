@echo off
setlocal

set CF=/Zi /nologo
set LF=/link glew32.lib opengl32.lib user32.lib gdi32.lib shell32.lib shlwapi.lib 

clang-cl %CF% /Fe:editor.exe code\editor.c %LF%
