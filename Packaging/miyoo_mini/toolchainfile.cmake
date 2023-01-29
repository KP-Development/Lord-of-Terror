set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_PROGRAM_PATH "/opt/miyoomini-toolchain/bin")
set(CMAKE_SYSROOT "/opt/miyoomini-toolchain/arm-linux-gnueabihf/sysroot")
set(CMAKE_FIND_ROOT_PATH "/opt/miyoomini-toolchain/arm-linux-gnueabihf/sysroot")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(ENV{PKG_CONFIG_SYSROOT_DIR} "/opt/miyoomini-toolchain/arm-linux-gnueabihf/sysroot")
set(CMAKE_C_COMPILER "/opt/miyoomini-toolchain/bin/arm-linux-gnueabihf-gcc")