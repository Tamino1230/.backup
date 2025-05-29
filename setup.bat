@echo off
REM * Check for admin rights
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo ============================
    echo.
    echo * This script requires administrative privileges. Please run as administrator.
    pause
    exit /b 1
)
REM * ============================
REM * AutoBackup Setup Utility
REM * ============================
:MENU
cls
set "INSTALL_SCOPE=USER"
set "INSTALL_DIR=%LOCALAPPDATA%\backup-setup"
set "PATH_SCOPE=USER"
echo =============================
echo .backup by Tamino1230
:ASK_SCOPE
echo 1. Install backup command
echo 2. Uninstall backup command
echo 3. Check if installed
echo 4. Exit
echo.
set /p choice=Enter your choice (1-4): 
if "%choice%"=="1" goto INSTALL
if "%choice%"=="2" goto UNINSTALL
if "%choice%"=="3" goto CHECK
if "%choice%"=="4" exit /b
echo Invalid choice. Please try again.
goto MENU

:INSTALL
REM * Initialize status variables
set "STATUS_COPY=OK"
REM * Create install dir
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"
REM * Copy backup.exe to install dir
copy "%~dp0\exe\backup.exe" "%INSTALL_DIR%\backup.exe" >nul 2>&1
if not exist "%INSTALL_DIR%\backup.exe" set "STATUS_COPY=FAILED"
REM * Add to PATH
 echo %PATH% | find /I "%INSTALL_DIR%" >nul
 if errorlevel 1 (
     setx PATH "%PATH%;%INSTALL_DIR%"
     set "STATUS_PATH=ADDED"
 ) else (
     set "STATUS_PATH=EXISTS"
 )
REM * Show summary
cls
setlocal enabledelayedexpansion
echo =============================
echo * Installation Summary
if "!STATUS_COPY!"=="OK" (
    echo   - backup.exe copied successfully to !INSTALL_DIR!.
) else (
    echo   - Failed to copy backup.exe.
)
if "!STATUS_PATH!"=="ADDED" (
    echo   - Path updated: !INSTALL_DIR! added to PATH.
) else if "!STATUS_PATH!"=="EXISTS" (
    echo   - Path already contains !INSTALL_DIR!.
)
echo =============================
echo Use `backup help` in CMD.
endlocal
pause
goto MENU

:UNINSTALL
REM * Initialize status variables
set "STATUS_DEL=OK"
REM * Remove backup.exe
del "%LOCALAPPDATA%\backup-setup\backup.exe" >nul 2>&1
if exist "%LOCALAPPDATA%\backup-setup\backup.exe" set "STATUS_DEL=FAILED"
REM * Show summary
cls
setlocal enabledelayedexpansion
echo =============================
echo * Uninstall Summary
if "!STATUS_DEL!"=="OK" (
    echo   - backup.exe removed.
) else (
    echo   - Failed to remove backup.exe.
)
echo =============================
endlocal
pause
goto MENU

:CHECK
REM * Check installation status
set "STATUS_COPY=NOT FOUND"
set "STATUS_PATH=NOT FOUND"
REM * Check backup.exe
if exist "%LOCALAPPDATA%\backup-setup\backup.exe" set "STATUS_COPY=OK"
REM * Check PATH
set "PATH_FOUND=NO"
for %%A in ("%PATH:;=" "%") do (
    if /I "%%~A"=="%LOCALAPPDATA%\backup-setup" set "PATH_FOUND=YES"
)
if "%PATH_FOUND%"=="YES" set "STATUS_PATH=OK"
REM * Show summary
cls
echo =============================
echo * Installation Check Summary
echo   - backup.exe: %STATUS_COPY%
echo   - backup-setup in PATH: %STATUS_PATH%
echo =============================
pause
goto MENU
