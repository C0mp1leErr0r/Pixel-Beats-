@echo off
REM 一键编译：双击即可重新生成单文件 exe
REM 会把 ..\..\可执行程序 里的音频与谱面打包进 exe
call "%~dp0build.bat" "%~dp0..\..\可执行程序"
if errorlevel 1 pause