export LD_LIBRARY_PATH="$HOME/.local/lib/":"$GCC_DIR/lib64/":"$LD_LIBRARY_PATH"

cmake \
    -S . \
    -B ./build/ \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_CXX_COMPILER="$LLVM_DIR/bin/clang++" \
    -DCMAKE_LINKER_TYPE=LLD \
    -DCMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN="$GCC_DIR/" \
    -G Ninja
ninja -C ./build

./build/main
