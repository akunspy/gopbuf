BIN=gopbuf
OBJS=gopbuf.o gocode.o pbuf_helper.o
WOPT=-Wall -Wfloat-equal -Wshadow -Wcast-qual -Winline

debug: CCFLAGS=-g $(WOPT) -DDEBUG 
debug: all
	@mkdir -p ../bin
	@echo "-------->  building  $(BIN)"
	@g++ -Wall $(OBJS) -o ../bin/$(BIN) -lprotobuf
	@cp pb.go ../bin/

release: CCFLAGS=-O2 $(WOPT) 
release: all
	@mkdir -p ../bin
	@echo "-------->  building  $(BIN)"
	@g++ -Wall $(OBJS) -o ../bin/$(BIN) -lprotobuf
	@cp pb.go ../bin/

all: $(OBJS)
$(OBJS): %.o: %.cpp
	@echo "-------->  $<"
	@g++ -c $(CCFLAGS) $< -o $@


.PHONY : clean 
clean:
	rm -f $(OBJS) ../bin/$(BIN)
