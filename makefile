NAME=news

# EXE=${NAME}

EXER:=~/local/${MACHTYPE_}/bin/${NAME}
# EXED:=./debug/${NAME}-debug
LIBR:=~/local/${MACHTYPE_}/lib/lib${NAME}.dylib
LIBD:=./debug/lib${NAME}.a

LDFLAGS+= -L /usr/bin -Wl,-all_load

LDLIBSE+=-lmrutils -lncurses
#	-lpantheios.1.core.clang.mt \
#	-lpantheios.1.util.clang.mt \
#	-lpantheios.1.be.file.clang.mt \
#	-lpantheios.1.bec.file.clang.mt \
#	-lpantheios.1.fe.simple.clang.mt \

LDLIBSL+=-lmrutils -lncurses

CPPFLAGS+=-I/usr/X11/include -I/opt/X11/include

include ~/local/include/standard.mk
