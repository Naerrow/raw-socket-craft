CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11

# 빌드 결과물 출력 디렉토리
BINDIR  = bin

# 각 모듈 소스 파일
SNIFFER_SRC  = src/sniffer/main.c src/sniffer/parser.c
CRAFTER_SRC  = src/crafter/main.c src/crafter/checksum.c src/crafter/builder.c
PROTOCOL_SRC = src/protocol/main.c src/protocol/session.c

.PHONY: all sniffer crafter protocol clean

all: sniffer crafter protocol

sniffer: $(BINDIR)
	$(CC) $(CFLAGS) $(SNIFFER_SRC) -o $(BINDIR)/sniffer

crafter: $(BINDIR)
	$(CC) $(CFLAGS) $(CRAFTER_SRC) -o $(BINDIR)/crafter

protocol: $(BINDIR)
	$(CC) $(CFLAGS) $(PROTOCOL_SRC) -o $(BINDIR)/protocol

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(BINDIR)
