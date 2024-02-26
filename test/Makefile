CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lpthread

SRCDIR = ../src
BUILDDIR = test_build
BINDIR = test_bin

all: proxy client client1 client2

client2: $(BUILDDIR)/udp_proxy_client.o $(BUILDDIR)/public.o $(BUILDDIR)/client2.o
	$(CC) $^ -o $(BINDIR)/client2

client1: $(BUILDDIR)/udp_proxy_client.o $(BUILDDIR)/public.o $(BUILDDIR)/client1.o
	$(CC) $^ -o $(BINDIR)/client1

client: $(BUILDDIR)/udp_proxy_client.o $(BUILDDIR)/public.o $(BUILDDIR)/client.o
	$(CC) $^ -o $(BINDIR)/client

proxy: $(BUILDDIR)/udp_proxy_server.o $(BUILDDIR)/public.o $(BUILDDIR)/hash_table.o $(BUILDDIR)/proxy.o
	$(CC) $^ -o $(BINDIR)/proxy $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/public/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/hash_table/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: test_src/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(BINDIR)/* $(BUILDDIR)/*

.PHONY: all clean
