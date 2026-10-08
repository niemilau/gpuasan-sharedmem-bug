# rocm/6.3.4-extras provides ASAN-instrumented HIP libs, the default rocm module does not
ml CrayEnv craype-x86-trento craype-accel-amd-gfx90a rocm/6.3.4-extras

export HSA_XNACK=1

# For finding clang-asan runtime
export LD_LIBRARY_PATH=$ROCM_PATH/lib/llvm/lib/clang/18/lib/linux:$LD_LIBRARY_PATH

# HIP libraries should be loaded from the asan-instrumented path instead of just rocm/lib
export LIBRARY_PATH=$ROCM_PATH/lib/asan:$LIBRARY_PATH
export LD_LIBRARY_PATH=$ROCM_PATH/lib/asan:$LD_LIBRARY_PATH

# Disable ASAN leak detection: too many false positives from HIP/asan internals
export ASAN_OPTIONS="detect_leaks=0"
