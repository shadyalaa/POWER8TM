PROG := hashmap

SRCS += \
	hashmap.cpp \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/thread.c \

#
OBJS := ${SRCS:.c=.o}
