@echo off
REM * ============================
REM * AutoBackup Setup Utility
REM * ============================
:MENU
REM * Main menu
cls

echo =============================
echo .backup by Tamino1230
echo 1. Install backup command
echo 2. Uninstall backup command
echo 3. Exit
echo.
set /p choice=Enter your choice (1-3): 
if "%choice%"=="1" goto INSTALL
if "%choice%"=="2" goto UNINSTALL
if "%choice%"=="3" exit /b
echo Invalid choice. Please try again.
goto MENU

:INSTALL
REM * Check for admin rights
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo * This script requires administrative privileges. Please run as administrator.
    pause
    goto MENU
)
REM * Initialize status variables
set "STATUS_COPY=OK"
set "STATUS_ALIAS=OK"
set "STATUS_PATH=OK"
REM * Copy backup.exe to user profile
copy "%~dp0\exe\backup.exe" "%USERPROFILE%\backup.exe" >nul 2>&1
if not exist "%USERPROFILE%\backup.exe" set "STATUS_COPY=FAILED"
REM * Ensure PowerShell profile directory exists
if not exist "%USERPROFILE%\Documents\WindowsPowerShell" (
    mkdir "%USERPROFILE%\Documents\WindowsPowerShell"
)
REM * Add PowerShell alias for backup command
echo Set-Alias backup "%~dp0exe\backup.exe" >> "%USERPROFILE%\Documents\WindowsPowerShell\Microsoft.PowerShell_profile.ps1"
if not exist "%USERPROFILE%\Documents\WindowsPowerShell\Microsoft.PowerShell_profile.ps1" set "STATUS_ALIAS=FAILED"
REM * Add user profile to PATH
setx PATH "%USERPROFILE%" /M >nul 2>&1
if errorlevel 1 set "STATUS_PATH=FAILED"
REM * Show summary
cls
setlocal enabledelayedexpansion
echo =============================
echo * Installation Summary
if "%STATUS_COPY%"=="OK" (
    echo   - backup.exe copied successfully.
) else (
    echo   - Failed to copy backup.exe.
)
if "%STATUS_ALIAS%"=="OK" (
    echo   - PowerShell alias added.
) else (
    echo   - Failed to add PowerShell alias.
)
if "%STATUS_PATH%"=="OK" (
    echo   - User profile added to PATH.
) else (
    echo   - Failed to add user profile to PATH.
)
echo =============================
echo Use `backup help` in CMD.
endlocal
pause
goto MENU

:UNINSTALL
REM * Initialize status variables
set "STATUS_DEL=OK"
set "STATUS_ALIAS_REMOVE=OK"
REM * Remove backup.exe
del "%USERPROFILE%\backup.exe" >nul 2>&1
if exist "%USERPROFILE%\backup.exe" set "STATUS_DEL=FAILED"
REM * Ensure PowerShell profile directory exists
if not exist "%USERPROFILE%\Documents\WindowsPowerShell" (
    mkdir "%USERPROFILE%\Documents\WindowsPowerShell"
)
REM * Remove PowerShell alias for backup command
set "PROFILE=%USERPROFILE%\Documents\WindowsPowerShell\Microsoft.PowerShell_profile.ps1"
if exist "%PROFILE%" (
    findstr /v /i "Set-Alias backup" "%PROFILE%" > "%PROFILE%.tmp"
    move /Y "%PROFILE%.tmp" "%PROFILE%" >nul
    if errorlevel 1 set "STATUS_ALIAS_REMOVE=FAILED"
) else (
    set "STATUS_ALIAS_REMOVE=NO_PROFILE"
)
REM * Show summary
cls
setlocal enabledelayedexpansion
echo =============================
echo * Uninstall Summary
if "%STATUS_DEL%"=="OK" (
    echo   - backup.exe removed.
) else (
    echo   - Failed to remove backup.exe.
)
if "%STATUS_ALIAS_REMOVE%"=="OK" (
    echo   - PowerShell alias removed.
) else if "%STATUS_ALIAS_REMOVE%"=="NO_PROFILE" (
    echo   - PowerShell profile not found, no alias to remove.
) else (
    echo   - Failed to remove PowerShell alias.
)
echo =============================
endlocal
pause
goto MENU
