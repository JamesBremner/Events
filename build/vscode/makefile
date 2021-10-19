#source file search paths
VPATH = ../../src 

INCS = -I../../include 

# folder for .o files
ODIR=./obj

# sources
_TST = demo.o 
TST = $(patsubst %,$(ODIR)/%,$(_TST))

_TCP = tcpdemo.o
TCP = $(patsubst %,$(ODIR)/%,$(_TCP))

_SVR = tcpserver.o
SVR = $(patsubst %,$(ODIR)/%,$(_SVR))

_CLN = tcpclient.o
CLN = $(patsubst %,$(ODIR)/%,$(_CLN))

$(ODIR)/%.o: %.cpp 
	g++ -g  -std=c++17  \
	-c -o $@ $< $(INCS)

test: $(TST)
	g++ -O2 \
	-o ../../bin/demo.exe $^

tcpserver: $(SVR)
		g++ -O2 \
	-o ../../bin/tcpserver.exe $^ \
	-lws2_32 -lwsock32

tcpclient: $(CLN)
		g++ -O2 \
	-o ../../bin/tcpclient.exe $^ \
	-lws2_32 -lwsock32

.PHONY: clean
clean:
	del /q .\obj


