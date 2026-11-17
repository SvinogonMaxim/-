@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"
set "CXX=C:\Program Files\CodeBlocks\MinGW\bin\g++.exe"
set "AR=C:\Program Files\CodeBlocks\MinGW\bin\ar.exe"
"%CXX%" -std=c++17 -O2 -Wall -Wextra -pedantic -c "Библиотека.cpp" -o "Библиотека.o"
if errorlevel 1 goto fail
"%AR%" rcs "Библиотека.a" "Библиотека.o"
if errorlevel 1 goto fail
"%CXX%" -std=c++17 -O2 -Wall -Wextra -pedantic "Проверки.cpp" "Библиотека.a" -static -o "Проверки.exe"
if errorlevel 1 goto fail
"Проверки.exe"
if errorlevel 1 goto fail
"%CXX%" -std=c++17 -O2 -Wall -Wextra -pedantic "Основная программа.cpp" "Библиотека.a" -static -o "Поисковик.exe"
if errorlevel 1 goto fail
echo Сборка и проверки завершены. Запускаю поисковик.
"Поисковик.exe"
pause
exit /b 0
:fail
echo Ошибка сборки или проверки. Сообщение выше.
pause
exit /b 1
