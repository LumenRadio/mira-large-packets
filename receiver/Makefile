PROJECT_NAME = large_packet_receiver

TARGET ?= mkw41z-root
LIBDIR ?= $(CURDIR)/../../..

COMMONDIR = ../common
CFLAGS += -I $(COMMONDIR)
CFLAGS += -std=c99

SOURCE_FILES = \
	large_packet_receiver.c \
	$(COMMONDIR)/large_packet.c \
	$(COMMONDIR)/lp_request.c \
	$(COMMONDIR)/lp_signal.c \
	$(COMMONDIR)/lp_subpacket.c

include $(LIBDIR)/Makefile.include
