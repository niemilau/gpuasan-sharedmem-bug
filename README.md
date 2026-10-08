# gpuasan-sharedmem-bug

Reproducer for a possible compiler bug in AMD's GPU AddressSanitizer (ASAN). Use of `__shared__` memory buffer of a struct type, eg. `__shared__ int3[N]`, causes memory corruption when compiling with `-fsanitize=address`. Tested on the LUMI supercomputer (MI250X, ROCm 6.3.4).

This repro demonstrates the issue with 2 failing cases and one non-failing case for reference. It does a simple "round trip" via shared memory to global memory and validates the array contents on host.

The makefile builds both an ASAN-instrumented executable and a non-instrumented executable (note: has harcoded `--offload-arch gfx90a:xnack+`). Expected output (non-ASAN version):
```
Case 1: 0 / 2048 mismatches
Case 2: 0 / 2048 mismatches
Case 3: 0 / 2048 mismatches
```
Meanwhile the ASAN-instrumented build gets gibberish in all it output elements for the `__shared__ int3[]` cases:
```
  Case 1 mismatch at 0: got (16843009,16843009,16843009) expected (0,0,0)
  Case 1 mismatch at 1: got (16843009,16843009,16843009) expected (1,2,3)
  Case 1 mismatch at 2: got (16843009,16843009,16843009) expected (2,4,6)
  Case 1 mismatch at 3: got (16843009,16843009,16843009) expected (3,6,9)
  Case 1 mismatch at 4: got (16843009,16843009,16843009) expected (4,8,12)
Case 1: 2048 / 2048 mismatches
  Case 2 mismatch at 0: got (16843009,16843009,16843009) expected (0,0,0)
  Case 2 mismatch at 1: got (16843009,16843009,16843009) expected (1,2,3)
  Case 2 mismatch at 2: got (16843009,16843009,16843009) expected (2,4,6)
  Case 2 mismatch at 3: got (16843009,16843009,16843009) expected (3,6,9)
  Case 2 mismatch at 4: got (16843009,16843009,16843009) expected (4,8,12)
Case 2: 2048 / 2048 mismatches
Case 3: 0 / 2048 mismatches
```

The number 16843009 happens to be 0x01010101, perhaps some shadow value placed by ASAN?


## Example usage on LUMI

```bash
source lumi_env.sh
make -j2
srun -A <project> -p dev-g -t 00:05:00 --nodes=1 --ntasks-per-node=1 --gpus-per-node=1 ./sharedmem_repro
```
