/** ROCm GPU AddressSanitizer bug repro: a __shared__ array of int3 corrupts data (even with no cross-thread access),
* Demonstrated below with simple kernels that simply fill in __shared__ arrays and write it back to global memory.
* Similar issues observed in a real program that used __shared__ arrays of structured types.
* No issues if compiling without -fsanitize=address. Tested on ROCm 6.3.4.
*/

#include <hip/hip_runtime.h>
#include <cstdio>
#include <vector>

inline void HIP_CHECK(hipError_t err, const char *file = __builtin_FILE(), int line = __builtin_LINE())
{
    if (err != hipSuccess)
    {
        fprintf(stderr, "HIP error at %s:%d - %s\n", file, line, hipGetErrorString(err));
        abort();
    }
}

constexpr int BLOCK_SIZE = 256;

/* Kernel 1: use __shared__ int3[] array, each thread fills in one int3 in one write. Then copy to global memory for output */
__global__ void case1(int n, int3 *out)
{
    __shared__ int3 sh[BLOCK_SIZE];
    int tid = threadIdx.x;
    if (tid >= n)
    {
        return;
    }

    sh[tid] = make_int3(tid, tid * 2, tid * 3);
    __syncthreads();
    out[blockIdx.x * BLOCK_SIZE + tid] = sh[tid];
}

/* Kernel 2: use __shared__ int3[] array, each thread fills in one int3 but in 3 separate writes. Then copy to global memory for output */
__global__ void case2(int n, int3 *out)
{
    __shared__ int3 sh[BLOCK_SIZE];
    int tid = threadIdx.x;
    if (tid >= n)
    {
        return;
    }

    sh[tid].x = tid;
    sh[tid].y = tid * 2;
    sh[tid].z = tid * 3;
    __syncthreads();
    out[blockIdx.x * BLOCK_SIZE + tid] = sh[tid];
}

/* Kernel 3 (non-buggy case): use a plain __shared__ int[] array, each thread fills in three ints. Then copy to global memory for output */
__global__ void case3(int n, int3 *out)
{
    __shared__ int sh[BLOCK_SIZE * 3];
    int tid = threadIdx.x;
    if (tid >= n)
    {
        return;
    }

    sh[0 * BLOCK_SIZE + tid] = tid;
    sh[1 * BLOCK_SIZE + tid] = tid * 2;
    sh[2 * BLOCK_SIZE + tid] = tid * 3;
    __syncthreads();
    out[blockIdx.x * BLOCK_SIZE + tid] = make_int3(sh[0 * BLOCK_SIZE + tid], sh[1 * BLOCK_SIZE + tid], sh[2 * BLOCK_SIZE + tid]);
}

// Use to check if the arrays filled in by kernels above contain correct data. This is host side so copy the arrays to host first
static int check(const char *name, const std::vector<int3>& data)
{
    int bad = 0;
    for (size_t idx = 0; idx < data.size(); idx++)
    {
        int tid = (int)(idx % BLOCK_SIZE);
        int3 expect = make_int3(tid, tid * 2, tid * 3);
        const int3 &v = data[idx];

        if (v.x != expect.x || v.y != expect.y || v.z != expect.z)
        {
            // Print only the first 5 mismatches
            if (bad < 5)
            {
                printf("  %s mismatch at %zu: got (%d,%d,%d) expected (%d,%d,%d)\n",
                    name, idx, v.x, v.y, v.z, expect.x, expect.y, expect.z);
            }
            bad++;
        }
    }
    printf("%s: %d / %zu mismatches\n", name, bad, data.size());
    return bad;
}

int main()
{
    const int blocks = 8;
    // "data length"
    const int n = blocks * BLOCK_SIZE;

    // Make int3 arrays of length n
    int3* d_case1 = nullptr;
    int3* d_case2 = nullptr;
    int3* d_case3 = nullptr;
    HIP_CHECK(hipMalloc(&d_case1, n * sizeof(int3)));
    HIP_CHECK(hipMalloc(&d_case2, n * sizeof(int3)));
    HIP_CHECK(hipMalloc(&d_case3, n * sizeof(int3)));

    case1<<<blocks, BLOCK_SIZE>>>(n, d_case1);
    HIP_CHECK(hipGetLastError());
    case2<<<blocks, BLOCK_SIZE>>>(n, d_case2);
    HIP_CHECK(hipGetLastError());
    case3<<<blocks, BLOCK_SIZE>>>(n, d_case3);
    HIP_CHECK(hipGetLastError());
    HIP_CHECK(hipDeviceSynchronize());

    // Copy to host for inspection
    std::vector<int3> h_case1(n), h_case2(n), h_case3(n);
    HIP_CHECK(hipMemcpy(h_case1.data(), d_case1, n * sizeof(int3), hipMemcpyDeviceToHost));
    HIP_CHECK(hipMemcpy(h_case2.data(), d_case2, n * sizeof(int3), hipMemcpyDeviceToHost));
    HIP_CHECK(hipMemcpy(h_case3.data(), d_case3, n * sizeof(int3), hipMemcpyDeviceToHost));

    check("Case 1", h_case1);
    check("Case 2", h_case2);
    check("Case 3", h_case3);
    return 0;
}
