# Microsoft C 6.00, 16-bit

# CC=       cl -nologo -AC -Ocegt -Gs
# CFLAGS=   -W1 -J -Zep -G2
# LDFLAGS=  -Lp -Fb -F 4000 -link /noe
# O=        .obj
# OUT=	    -Fo
# DEF=      pc-files\patch.def

# emx/gcc 2.1, 32-bit

CC=       gcc -O -s
CFLAGS=   
LDFLAGS=  -los2
OUT=      -o
O=        .o
DEF=

# general

DEFS=     -Ipc-files -DOS2 -DCANVARARG

OBJ=	  patch$O pch$O inp$O util$O version$O backupfile$O pc-files/dirent$O

.c$O:
	$(CC) $(CFLAGS) $(DEFS) -c $(OUT)$*$O $*.c

patch.exe: $(OBJ) $(DEF)
	$(CC) $(OBJ) $(DEF) -o $@ $(LDFLAGS)
