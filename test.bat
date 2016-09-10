set mypath=%cd%
rem cd /d "\Program Files (x86)\Microsoft Visual Studio 14.0\VC"
cmd /k "cd \Program Files (x86)\Microsoft Visual Studio 14.0\VC & vcvarsall amd64 & cd /d %mypath% & cl /EHsc /openmp /O2 /I rapidjson/include/rapidjson user32.lib  odbc32.lib AppTest.cpp AutoDiff.cpp & exit 0"
