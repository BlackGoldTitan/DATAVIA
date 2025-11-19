cmake_minimum_required(VERSION 3.10)
project(CRCRECOVER)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译器选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 创建命令行版本可执行文件
add_executable(CRCRECOVER
    main.cpp
    DiskSectorCRC.cpp
    DiskSectorCRC.h
    EnhancedDiskSectorCRC.cpp
    EnhancedDiskSectorCRC.h
    FileSystemCRC.cpp
    FileSystemCRC.h
    DiskUtils.cpp
    DiskUtils.h
    OptimizedDiskReader.cpp
    OptimizedDiskReader.h
    HighPerformanceCRC.cpp
    HighPerformanceCRC.h
)

# 创建GUI版本可执行文件
add_executable(CRCRECOVER_GUI
    main_gui.cpp
    GUIWindow.cpp
    DiskSectorCRC.cpp
    DiskSectorCRC.h
    GUIWindow.h
    EnhancedDiskSectorCRC.cpp
    EnhancedDiskSectorCRC.h
    FileSystemCRC.cpp
    FileSystemCRC.h
    DiskUtils.cpp
    DiskUtils.h
    OptimizedDiskReader.cpp
    OptimizedDiskReader.h
    HighPerformanceCRC.cpp
    HighPerformanceCRC.h
)

# 创建诊断工具
add_executable(DiskListTool
    DiskListTool.cpp
)

add_executable(PerformanceDiagnostic
    PerformanceDiagnostic.cpp
)

add_executable(SimplePerformanceTest
    SimplePerformanceTest.cpp
)

add_executable(OptimizedDiskAccess
    OptimizedDiskAccess.cpp
)

add_executable(HighPerformance4096
    HighPerformance4096.cpp
)

add_executable(FixedDiskAccess
    FixedDiskAccess.cpp
)

add_executable(UltimateOptimizedGUI
    UltimateOptimizedGUI.cpp
)

add_executable(FinalUltimateOptimizedGUI
    FinalUltimateOptimizedGUI.cpp
)

# Windows特定设置
if(WIN32)
    target_link_libraries(CRCRECOVER kernel32.lib)
    target_link_libraries(CRCRECOVER_GUI kernel32.lib)
endif()

# 安装目标
install(TARGETS CRCRECOVER DESTINATION bin)
install(TARGETS CRCRECOVER_GUI DESTINATION bin)
