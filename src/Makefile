# environment
CC = xlclang
LD = xlclang

TARGET = ../bin/zowe_launcher

CFLAGS = -O -D_OPEN_THREADS -D_XOPEN_SOURCE=600 \
         "-Wa,goff" \
         "-Wc,langlvl(EXTC99),float(HEX),agg,list(),so(),search(),lp64,xplink" \
         "-Wc,goff,xref,gonum,roconst,gonum,asm,asmlib('SYS1.MACLIB'),asmlib('CEE.SCEEMAC')" \
         -I . \

LDFLAGS =
export _C89_LSYSLIB=CEE.SCEELKEX:CEE.SCEELKED:SYS1.CSSLIB:CSF.SCSFMOD0

# C source
SRCS = main.c 

OBJS = $(SRCS:%.c=%.o)

# disable built-in rules
MAKEFLAGS += --no-builtin-rules

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p $(dir $(TARGET))
	$(LD) $(LDFLAGS) -o $(TARGET) $^ || { $(RM) $@; exit 1; }

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	$(RM) $(TARGET)
	$(RM) $(OBJS) $(notdir $(OBJS:%.o=%.lst))

