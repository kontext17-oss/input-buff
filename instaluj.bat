@echo off
setlocal enabledelayedexpansion
title Instalator - Predictive Input Buffer

set "MODFILE="
for %%F in ("%~dp0*.geode") do set "MODFILE=%%~fF"

if "%MODFILE%"=="" (
    echo Nie znaleziono pliku .geode obok tego pliku.
    echo Wrzuc pobrany plik .geode do tego samego folderu co instaluj.bat i uruchom ponownie.
    pause
    exit /b 1
)

set "GDDIR="

rem 1) Szukamy Steama w rejestrze
for /f "tokens=2,*" %%A in ('reg query "HKCU\Software\Valve\Steam" /v SteamPath 2^>nul ^| find "SteamPath"') do set "STEAM=%%B"
if defined STEAM (
    set "STEAM=!STEAM:/=\!"
    if exist "!STEAM!\steamapps\common\Geometry Dash\GeometryDash.exe" set "GDDIR=!STEAM!\steamapps\common\Geometry Dash"
)

rem 2) Typowe sciezki
if not defined GDDIR if exist "C:\Program Files (x86)\Steam\steamapps\common\Geometry Dash\GeometryDash.exe" set "GDDIR=C:\Program Files (x86)\Steam\steamapps\common\Geometry Dash"
if not defined GDDIR if exist "C:\Program Files\Steam\steamapps\common\Geometry Dash\GeometryDash.exe" set "GDDIR=C:\Program Files\Steam\steamapps\common\Geometry Dash"
if not defined GDDIR if exist "D:\SteamLibrary\steamapps\common\Geometry Dash\GeometryDash.exe" set "GDDIR=D:\SteamLibrary\steamapps\common\Geometry Dash"

rem 3) Nie znaleziono - pytamy
if not defined GDDIR (
    echo Nie udalo sie znalezc Geometry Dash automatycznie.
    echo Wklej pelna sciezke do folderu z GeometryDash.exe i nacisnij Enter:
    set /p GDDIR=
)

if not exist "%GDDIR%\geode" (
    echo Folder "%GDDIR%\geode" nie istnieje. Upewnij sie, ze Geode jest zainstalowany
    echo i ze uruchomiles gre przynajmniej raz.
    pause
    exit /b 1
)

if not exist "%GDDIR%\geode\mods" mkdir "%GDDIR%\geode\mods"

copy /Y "%MODFILE%" "%GDDIR%\geode\mods\" >nul
if errorlevel 1 (
    echo Kopiowanie nie powiodlo sie.
    pause
    exit /b 1
)

echo.
echo Gotowe! Mod zainstalowany w:
echo %GDDIR%\geode\mods
echo Uruchom Geometry Dash. Ustawienia znajdziesz w menu Geode ^> Mody ^> Predictive Input Buffer.
pause
