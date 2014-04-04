include ../config-host.mak

CC=gcc

FILENAME=frame.piqi
EXT=pb-c

all: libtrace_a

$(FILENAME).$(EXT).c:
	protoc-c --c_out=. -I$(TRACEWRAP_DIR) $(TRACEWRAP_DIR)/$(FILENAME).proto
	sed -i 's/->base/->__base/g' $(FILENAME).$(EXT).[ch]
	sed -i 's/ProtobufCMessage base;/ProtobufCMessage __base;/g' $(FILENAME).$(EXT).[ch]

$(FILENAME).$(EXT).o: $(FILENAME).$(EXT).c $(FILENAME).$(EXT).h
	$(CC) -c -o $(FILENAME).$(EXT).o $(FILENAME).$(EXT).c

libtrace_a: $(FILENAME).$(EXT).o
	ar rcs libtrace.a $(FILENAME).$(EXT).o

clean:
	rm libtrace.a $(FILENAME).$(EXT).[hco]
