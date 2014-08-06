include ../config-host.mak

CC=gcc

FILENAME=frame.piqi
EXT=pb-c

all: libtrace.a

$(FILENAME).proto:
	piqi to-proto $(TRACEWRAP_DIR)/$(FILENAME) -o $(FILENAME).proto

$(FILENAME).$(EXT).c: $(FILENAME).proto
	protoc-c --c_out=. $(FILENAME).proto
	sed -i 's/->base/->__base/g' $(FILENAME).$(EXT).[ch]
	sed -i 's/ProtobufCMessage base;/ProtobufCMessage __base;/g' $(FILENAME).$(EXT).[ch]

$(FILENAME).$(EXT).o: $(FILENAME).$(EXT).c $(FILENAME).$(EXT).h
	$(CC) -fPIC -c -o $(FILENAME).$(EXT).o $(FILENAME).$(EXT).c

libtrace.a: $(FILENAME).$(EXT).o
	ar rcs libtrace.a $(FILENAME).$(EXT).o

clean:
	rm libtrace.a $(FILENAME).$(EXT).[hco]
