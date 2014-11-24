CL = cl65
CL_FLAGS = -t none --listing --list-bytes 100
#CL_FLAGS = -t apple1 -C apple1-16k.cfg  --listing --list-bytes 100
CC = cl65
CC_FLAGS = --static-locals -t apple1 -C apple1-16k.cfg
C2T = c2t

ASRC = $(shell echo *.s)
AOBJ = $(ASRC:%.s=%.o)
ALST = $(ASRC:%.s=%.lst)
AMON = $(ASRC:%.s=%.mon)
ABIN = $(ASRC:%.s=%)

all: $(ABIN)

clean:
	-rm -f $(ABIN) $(AOBJ) $(ALST) $(AMON)

%: %.s
	$(CL) $(CL_FLAGS) $< 

%: %.c
	$(CC) $(CC_FLAGS) $< 

%.mon: %
	$(C2T) $< $@

