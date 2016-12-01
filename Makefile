TARGETS = xform-gain-generator

all: ${TARGETS}

LDLIBS += -lm

xform-gain-generator: pnm.o xform_gain.o

clean:
	${RM} ${TARGETS} core *.o
