@echo off
REM ===========================================================================
REM  build.bat - rebuild the single-file beta game exe
REM
REM  Usage:  build.bat "D:\path\to\Project6\Project6"
REM          that folder must contain:  charts\*.mp3  charts\*.txt  hit.wav
REM
REM  Result: ..\PixelBeats-Beta.exe  (x64, static CRT, all assets embedded)
REM  NOTE: keep this file ASCII-only, cmd.exe mis-parses non-ASCII bytes.
REM ===========================================================================
setlocal enableextensions
set "ROOT=%~dp0"
set "ASSET_SRC=%~1"

if "%ASSET_SRC%"=="" (
    echo [ERROR] missing asset folder argument.
    echo         usage: build.bat "D:\...\Project6\Project6"
    exit /b 1
)
if not exist "%ASSET_SRC%\charts" (
    echo [ERROR] no "charts" sub-folder under: %ASSET_SRC%
    exit /b 1
)

REM ---- locate Visual Studio build environment ------------------------------
set "VCVARS="
for %%P in (
  "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
  "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
  "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
  "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) do if exist %%P set "VCVARS=%%~P"
if "%VCVARS%"=="" (
    echo [ERROR] cannot find vcvars64.bat - install Visual Studio 2022 with C++ tools.
    exit /b 1
)
call "%VCVARS%" >nul 2>&1
if errorlevel 1 (echo [ERROR] vcvars64 failed & exit /b 1)

REM ---- stage the assets ----------------------------------------------------
if not exist "%ROOT%build\assets" mkdir "%ROOT%build\assets"
del /q "%ROOT%build\assets\*.*" >nul 2>&1
copy /y "%ASSET_SRC%\charts\*.mp3" "%ROOT%build\assets\" >nul 2>&1
copy /y "%ASSET_SRC%\charts\*.txt" "%ROOT%build\assets\" >nul 2>&1
copy /y "%ASSET_SRC%\hit.wav"      "%ROOT%build\assets\" >nul 2>&1

REM ---- generate assets.rc + assets_index.h ---------------------------------
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%gen_assets_rc.ps1" -AssetsDir "%ROOT%build\assets" -SrcDir "%~dp0."
if errorlevel 1 (echo [ERROR] asset index generation failed & exit /b 1)

REM ---- compile the resource script -----------------------------------------
pushd "%ROOT%build\assets"
rc /nologo /fo "%ROOT%build\assets.res" assets.rc
if errorlevel 1 (echo [ERROR] rc failed & popd & exit /b 1)
popd

REM ---- compile + link ------------------------------------------------------
if not exist "%ROOT%build\obj" mkdir "%ROOT%build\obj"
pushd "%ROOT%"
cl /nologo /utf-8 /EHsc /O2 /MT /DNDEBUG /I "%ROOT%raylib\include" ^
   /Fo:"%ROOT%build\obj\\" /Fd:"%ROOT%build\obj\\" /Fe:"%ROOT%..\PixelBeats-Beta.exe" ^
   FileName.cpp assets.cpp pe_resource.cpp "%ROOT%build\assets.res" ^
   /link /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup "%ROOT%raylib\x64\raylib.lib" ^
   winmm.lib user32.lib gdi32.lib shell32.lib ole32.lib oleaut32.lib advapi32.lib comdlg32.lib opengl32.lib
set "RC=%errorlevel%"
popd

if not "%RC%"=="0" (echo [ERROR] build failed & exit /b 1)
echo.
echo [OK] built: %ROOT%..\PixelBeats-Beta.exe
exit /b 0
