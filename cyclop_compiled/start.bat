::test win32
::cyclop_win32.exe 1 982451653 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000
::test win64
::cyclop_x86-x64.exe 1 982451653 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000

::test -7z with PATH
::cyclop_win32.exe 1 982451653 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000 -7z
::test -7z with specified folder
cyclop_win32.exe 1 982451653 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000 -7z "C:\\7-Zip\\7z.exe"

pause
