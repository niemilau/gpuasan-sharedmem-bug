
all: sharedmem_repro sharedmem_repro_noasan

# --enable-new-dtags needed on LUMI to get the ASAN-instrumented HIP libs loaded instead of normal ones
sharedmem_repro: sharedmem_repro.cpp
	hipcc --offload-arch=gfx90a:xnack+ -fsanitize=address -std=c++17 -xhip \
	    -shared-libsan -Wl,--enable-new-dtags sharedmem_repro.cpp -o sharedmem_repro

# Build without ASAN instrumentation works even with xnack+.
sharedmem_repro_noasan: sharedmem_repro.cpp
	hipcc --offload-arch=gfx90a:xnack+ -std=c++17 -xhip \
		sharedmem_repro.cpp -o sharedmem_repro_noasan

clean:
	rm sharedmem_repro sharedmem_repro_noasan
