@echo off

set "compile_options= /diagnostics:column /Gm- /FC /W4 /wd4996 /wd4116 /wd4100 /wd4201 /wd4101 /wd4204 /Od /Zo /Zf /Z7 /Oi /nologo /GR- /MP"
set "link_options= /INCREMENTAL:NO /opt:ref"

if NOT EXIST .\build mkdir build
pushd .\build

cl %compile_options% %defines% .\..\asciify.c /link %link_options% User32.lib Gdi32.lib Shell32.lib /out:asciify.exe

popd
