bin_PROGRAMS = dblobd

dblobd_CFLAGS = -Wall -Werror $(AM_CFLAGS) $(PTHREAD_CFLAGS)
dblobd_LDADD = $(PTHREAD_LIBS)
dblobd_SOURCES = \
	api.c \
	api.h \
	dispatcher.h \
	eventsource.c \
	eventsource.h \
	inet.c \
	inet.h \
	main.c \
	server.c \
	server.h

if HAVE_EPOLL
dblobd_SOURCES += epoll.c
endif
