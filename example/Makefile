EMCC = emcc

CFLAGS = -O3 --no-entry

all: agent.wasm

agent.wasm: simple_agent.c
	$(EMCC) $^ -o $@ $(CFLAGS)

clean:
	rm -f agent.wasm

.PHONY: all clean
