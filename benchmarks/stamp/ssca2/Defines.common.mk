# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


PROG := ssca2

SRCS += \
	alg_radix_smp.cpp \
	computeGraph.cpp \
	createPartition.cpp \
	findSubGraphs.cpp \
	genScalData.cpp \
	getUserParameters.cpp \
	globals.cpp \
	ssca2.cpp \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/thread.c \
#
OBJS := ${SRCS:.c=.o}

#CFLAGS += -DUSE_PARALLEL_DATA_GENERATION
#CFLAGS += -DWRITE_RESULT_FILES
CFLAGS += -DENABLE_KERNEL1
#CFLAGS += -DENABLE_KERNEL2 -DENABLE_KERNEL3
#CFLAGS += -DENABLE_KERNEL4


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
