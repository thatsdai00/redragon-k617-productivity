@echo off
chcp 65001 >nul
cd /d "%~dp0"
title Redragon K617
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0ui.ps1"
if errorlevel 1 (
  echo.
  echo UI failed to open. Windows PowerShell is required.
  pause
)
