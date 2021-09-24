.PHONY=clean all
COMPILER=gcc
CFLAGS = -Wall -fsanitize=address -g
all:  tcpEchoClient echoServer udpEchoClient
clean:	
	- rm -f *.o   tcpEchoclient echoServer udpEchoClient

COMMON =  logger.c util.c

tcpEchoClient:      
	$(COMPILER) $(CFLAGS) -o tcpEchoClient tcpEchoClient.c tcpClientUtil.c $(COMMON)
echoServer:      
	$(COMPILER) $(CFLAGS) -o echoServer tcpEchoAddrinfo.c tcpServerUtil.c $(COMMON)
udpEchoClient:
	$(COMPILER) $(CFLAGS) -o udpEchoClient udpEchoClient.c $(COMMON)