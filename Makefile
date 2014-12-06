SEND = 3600send
RECV = 3600recv
SENDRECV = 3600sendrecv
NETSIM = /course/cs3600f14/bin/project4/netsim

all: $(SEND) $(RECV)

$(SENDRECV).o: $(SENDRECV).c
	gcc -c -std=c99 -O0 -g -lm -Wall -pedantic -Wextra -o $@ $<

$(SEND): $(SEND).c $(SENDRECV).o
	gcc -std=c99 -O0 -g -lm -Wall -pedantic -Wextra -o $@ $< $(SENDRECV).o

$(RECV): $(RECV).c $(SENDRECV).o
	gcc -std=c99 -O0 -g -lm -Wall -pedantic -Wextra -o $@ $< $(SENDRECV).o

test: all
	./test

clean:
	rm $(SEND) $(RECV) $(SENDRECV).o

run_s: all
	$(NETSIM)
	./run --size small --printlog

run_s_dup: all
	$(NETSIM) --duplicate 100
	./run --size small --printlog


run_s_drp: all
	$(NETSIM) --drop 50
	./run --size small --printlog

run_m: all
	$(NETSIM)
	./run --size medium --printlog

run_m_drp: all
	$(NETSIM) --drop 50
	./run --size medium --printlog

run_l: all
	$(NETSIM)
	./run --size large --printlog

run_l_drp: all
	$(NETSIM) --drop 50
	./run --size large --printlog

run_h: all
	$(NETSIM)
	./run --size huge --printlog
