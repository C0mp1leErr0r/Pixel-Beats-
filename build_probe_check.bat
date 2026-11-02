@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "%~dp0"
if not exist "%~dp0build\obj_probe" mkdir "%~dp0build\obj_probe"
cl /nologo /utf-8 /EHsc /O2 /MT /DNDEBUG /I "%~dp0raylib\include" /Fo:"%~dp0build\obj_probe\\" /Fe:"%~dp0build\beta_probe_check.exe" ^
   beta_probe.cpp assets.cpp pe_resource.cpp "%~dp0build\assets.res" ^
   /link /SUBSYSTEM:CONSOLE "%~dp0raylib\x64\raylib.lib" winmm.lib user32.lib gdi32.lib shell32.lib ole32.lib oleaut32.lib advapi32.lib comdlg32.lib opengl32.lib
echo EXITCODE=%errorlevel%