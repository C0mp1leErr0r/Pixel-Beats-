@echo off
REM 一键编译：双击即可重新生成单文件 exe
REM 会从 ..\..\Project6\Project6 里取 charts\ 和 hit.wav 打包进 exe
call "%~dp0build.bat" "%~dp0..\..\Project6\Project6"
if errorlevel 1 pause