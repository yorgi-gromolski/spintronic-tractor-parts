v1: v1.o
	cc -o v1 v1.c
v2: v2.o
	cc -o v2 v2.c
v2a: v2a.c precrt0.o lastfile.o
	/lib/cpp '-Dmc68k32' '-Dmc68k' '-Dmc68010' 'v2a.c' v2a.E
	/lib/ccom v2a.E v2a.s
	/bin/as  '-C' '-o' 'v2a.o' v2a.s
	/bin/ld precrt0.o '/lib/crt0.o' v2a.o lastfile.o '-o' 'v2a' -lc
v2c: v2a.c precrt0.o lastfile.o
	/lib/cpp '-Dmc68k32' '-Dmc68k' '-Dmc68010' 'v2c.c' v2c.E
	/lib/ccom v2c.E v2c.s
	/bin/as  '-C' '-o' 'v2c.o' v2c.s
	/bin/ld precrt0.o '/lib/crt0.o' v2c.o lastfile.o '-o' 'v2c' -lc
gv2a: v2a.c precrt0.o lastfile.o
	/usr/local/lib/gcc-cpp -undef -D__GNU__ -D__GNUC__ -Dmc68k -Dunix -Dunixpc v2a.c gv2a.cpp
	/usr/local/lib/gcc-cc1 gv2a.cpp -quiet -dumpbase v2a.c -o gv2a.s
	/bin/as  '-C' '-o' 'v2a.o' gv2a.s
	/bin/ld precrt0.o '/lib/crt0.o' gv2a.o lastfile.o '-o' 'v2a' -lc
v3: v3.c
	cc -o v3 v3.c
