#source file search paths
VPATH = ../../src 

# folder for .o files
ODIR=./obj

# sources
_TST = demo.o 
TST = $(patsubst %,$(ODIR)/%,$(_TST))

$(ODIR)/%.o: %.cpp 
	g++ -O2  -std=c++17  \
	-c -o $@ $<

test: $(TST)
	g++ -O2 \
	-o ../../bin/demo.exe $^

