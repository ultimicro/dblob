bin_PROGRAMS = dblobd

dblobd_CFLAGS = -Wall -Werror $(AM_CFLAGS) $(PTHREAD_CFLAGS)
dblobd_LDADD = $(PTHREAD_LIBS)
dblobd_SOURCES = \
	dispatcher.h \
	eventsource.c \
	eventsource.h \
	main.c \
	server.c \
	server.h

if HAVE_EPOLL
dblobd_SOURCES += epoll.c
endif
