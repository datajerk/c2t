CL = cl65
CL_FLAGS = -t none

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
