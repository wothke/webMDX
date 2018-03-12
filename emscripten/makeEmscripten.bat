::  POOR MAN'S DOS PROMPT BUILD SCRIPT.. make sure to delete the respective built/*.bc files before building!
::  existing *.bc files will not be recompiled. 

setlocal enabledelayedexpansion

SET ERRORLEVEL
VERIFY > NUL

:: **** use the "-s WASM" switch to compile WebAssembly output. warning: the SINGLE_FILE approach does NOT currently work in Chrome 63.. ****
set "OPT= -s WASM=0 -s ASSERTIONS=1 -s VERBOSE=0 -s FORCE_FILESYSTEM=1 -DEMSCRIPTEN -DNO_DEBUG_LOGS -DHAVE_LIMITS_H -DHAVE_STDINT_H -Wcast-align -fno-strict-aliasing -s SAFE_HEAP=1 -s DISABLE_EXCEPTION_CATCHING=0 -Wno-pointer-sign -I. -I.. -I../zlib -I../mdxmini/src/ -I../pmdmini/src/pmdwin/ -I../pmdmini/src/fmgen/ -I../src/device -Os -O3 "


if not exist "built/pmdmini.bc" (
	call emcc.bat %OPT% ../pmdmini/src/fmgen/file.cpp ../pmdmini/src/fmgen/fmgen.cpp ../pmdmini/src/fmgen/fmtimer.cpp ../pmdmini/src/fmgen/opm.cpp ../pmdmini/src/fmgen/opna.cpp ../pmdmini/src/fmgen/psg.cpp ../pmdmini/src/pmdwin/opnaw.cpp ../pmdmini/src/pmdwin/p86drv.cpp ../pmdmini/src/pmdwin/pmdwin.cpp ../pmdmini/src/pmdwin/ppsdrv.cpp ../pmdmini/src/pmdwin/ppz8l.cpp ../pmdmini/src/pmdwin/table.cpp ../pmdmini/src/pmdwin/util.cpp ../pmdmini/src/pmdmini.c -o built/pmdmini.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)

if not exist "built/mdxmini.bc" (
	call emcc.bat %OPT% ../mdxmini/src/mdx2151.c ../mdxmini/src/mdxfile.c ../mdxmini/src/mdxmini.c ../mdxmini/src/mdxmml_ym2151.c ../mdxmini/src/pcm8.c ../mdxmini/src/pdxfile.c ../mdxmini/src/ym2151.c -o built/mdxmini.bc
	IF !ERRORLEVEL! NEQ 0 goto :END
)
call emcc.bat %OPT% -s TOTAL_MEMORY=67108864 --closure 1 --llvm-lto 1  --memory-init-file 0   built/pmdmini.bc built/mdxmini.bc  adapter.cpp   -s EXPORTED_FUNCTIONS="[ '_emu_load_file','_emu_teardown','_emu_get_current_position','_emu_seek_position','_emu_get_max_position','_emu_set_subsong','_emu_get_track_info','_emu_get_sample_rate','_emu_get_audio_buffer','_emu_get_audio_buffer_length','_emu_compute_audio_samples', '_malloc', '_free']"  -o htdocs/mdx.js  -s SINGLE_FILE=0 -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'Pointer_stringify']"  -s BINARYEN_ASYNC_COMPILATION=1 -s BINARYEN_TRAP_MODE='clamp' && copy /b shell-pre.js + htdocs\mdx.js + shell-post.js htdocs\web_mdx3.js && del htdocs\mdx.js && copy /b htdocs\web_mdx3.js + mdx_adapter.js htdocs\backend_mdx.js && del htdocs\web_mdx3.js
:END

