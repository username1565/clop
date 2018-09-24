::test win32
::cyclop_win32.exe 1 982451653 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000
::test win64
::cyclop_x86-x64.exe 1 982451653 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000

::test -7z with PATH
::cyclop_win32.exe 1 982451653 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000 -7z

::test -7z with specified folder
cyclop_win32.exe 1 982451653 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000 -7z "C:\\7-Zip\\7z.exe"

::cyclop_win32.exe 1 15000 -p -o result.txt -s 123 -n 1000 -b 7 -d 34 -bd 10 -wfl -f 100000

::cyclop_win32.exe 1 40 -p -o result.txt -s 123 -n 10 -b 5 -d 09 -bd 10 -wfl -f 1000 -7z disabled

::cyclop_win32.exe 1 200 -p -o result.txt -s 123 -n 10 -b 5 -d 09 -bd 10 -wfl -f 100000

::cyclop_win32.exe 31 71 -p -o result.txt -s 124 -n 10 -b 5 -d 9 -bd 10

::cyclop_win32.exe 1 200 -p -o result.txt -n 10 -b 5 -d 09 -bd 10 -wfl -f 1000

::cyclop_win32.exe 1 982451653 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000

::from 3
::cyclop_win32.exe 3 982451653 -p -gaps2byte -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000 -7z disabled

::cyclop_win32.exe 1 224743 -p -gaps2byte -o 50M.txt -s 1 -n 10000 -b 10 -d 09 -bd 10 -wfl -f 100000

::from 3
::cyclop_win32.exe 3 224743 -p -gaps2byte -o 50M.txt -s 1 -n 10000 -b 10 -d 09 -bd 10 -wfl -f 100000

::cyclop_win32.exe 2 32452943 -p -gaps1byte -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000

::cyclop_win32.exe 1 224743 -p -gaps2byte -o 50M.txt -s 1 -n 10000 -b 10 -d 09 -bd 10 -wfl -f 100000

::cyclop_win32.exe 1 104745 -p -x64 -o 50M.txt -s 1 -n 10000 -b 10 -d 09 -bd 10 -wfl -f 100000

::cyclop_win32.exe 1 40 -p -o result.txt -s 123 -n 10 -b 5 -d ";\t" -bd "\n" -wfl -f 1000 -7z disabled

::cyclop_win32.exe 1 40 -p -o result.txt -s 123 -n 10 -b 5 -d 09 -bd 10 -wfl -f 1000 -7z disabled

::cyclop_win32.exe 1 982451653 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000

::cyclop_win32.exe 1 3000000 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000 -7z "xD"

::cyclop_win32.exe 4000000000000 4000800000000 -p -o 50M.txt -s 1 -n 10000000 -b 10 -d 09 -bd 10 -wfl -f 100000 -7z

::cyclop_win32.exe 1 982451653 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000 -7z "C:\\7-Zip\\7z.exe"

::cyclop_win32.exe 2001133018631 2001416240129 -p -o 2T-3T.txt -s 5 -n 10000000 -b 10 -d 09 -bd 10 -wfl -f 100000

::cyclop_win32.exe 1 982451653 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000

::cyclop_win32.exe 1 982451653 -p -o 50M.txt -s 1 -n 1000000 -b 10 -d 09 -bd 10 -wfl -f 100000 -7z "C:\\7-Zip\\7z.exe"

::cyclop_win32.exe 3999999899999 4000000100000 -p -o 3T-4T.txt -s 1 -n 10000000 -b 10 -d 09 -bd 10 -wfl -f 100000

::bad unicode character
::cyclop_win32.exe 1 40 -p -o result.txt -s 123 -n 10 -b 5 -d "\\u1ed8" -bd 10 -wfl -f 1000 -7z disabled
::cyclop_win32.exe 1 40 -p -o result.txt -s 123 -n 10 -b 5 -d "\u1ed8" -bd 10 -wfl -f 1000 -7z disabled

pause